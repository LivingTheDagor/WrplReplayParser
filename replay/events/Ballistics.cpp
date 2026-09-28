#include "state/ParserState.h"
#include "ecs/ComponentTypesDefs.h"
#include "Unit.h"

#include <ioSys/dag_dataBlock.h>

#include <cmath>
#include <unordered_map>

// Flight of an unguided store, rebuilt from the state it left the pylon with.
//
// The server streams a trajectory only for guided munitions. A free falling bomb and
// an unguided rocket are deterministic, so nothing is sent and the client integrates
// them; this does the same with the game's own numbers. Everything needed sits in
// aces.vromfs.bin, which the parser already mounts: mass, caliber and dragCx of the
// bomb, and for a rocket its motor force, burn time and burnt out mass.
//
// The result goes into a history of its own, never into positions: a reconstruction
// must not be mistaken for a recording.
namespace unit {

  namespace {
    constexpr float GRAVITY = 9.80665f;
    constexpr float RHO0 = 1.225f;
    constexpr uint32_t STEP_MS = 10;     // integration step
    constexpr float STEP_S = float(STEP_MS) * 0.001f;
    constexpr uint32_t SAMPLE_MS = 100;  // spacing of the published samples

    struct BallisticParams {
      float mass = 0.f;
      float area = 0.f;
      float cx = 0.f;
      float force = 0.f;      // motor thrust, zero for a bomb
      float time_fire = 0.f;  // burn time
      float mass_end = 0.f;   // mass once the motor has burnt out
    };

    // Standard atmosphere, referenced to sea level. Drag on a bomb dropped from four
    // kilometres is a fifth weaker than at sea level, which is worth more than the
    // sampling error - so the height fed in has to be above the sea, not above the
    // origin of the map: on Pradesh the two differ by 920 m, which is 8.5% of density.
    float air_density(float height) {
      height = height < -500.f ? -500.f : (height > 20000.f ? 20000.f : height);
      return RHO0 * powf(1.f - 2.25577e-5f * height, 4.25588f);
    }

    bool read_params(const DataBlock &blk, const char *section, BallisticParams &out) {
      const DataBlock *body = blk.getBlockByName(section);
      if (!body)
        return false;
      const float mass = body->getReal("mass", 0.f);
      const float caliber = body->getReal("caliber", 0.f);
      if (mass <= 0.f || caliber <= 0.f)
        return false;
      out.mass = mass;
      out.area = float(M_PI) * caliber * caliber * 0.25f;
      out.cx = body->getReal("dragCx", 0.f) * body->getReal("CxK", 1.f);
      out.force = body->getReal("force", 0.f);
      out.time_fire = body->getReal("timeFire", 0.f);
      out.mass_end = body->getReal("massEnd", mass);
      // maxSpeed is deliberately ignored. It is not a ceiling on the projectile: the
      // game itself recorded a ROFS-132 hitting at 456 m/s against a maxSpeed of 355,
      // and clamping to it loses 150 metres of range.
      return true;
    }

    // Cached because a salvo fires the same store many times over.
    const BallisticParams *lookup(const std::string &weapon_id) {
      static std::unordered_map<std::string, BallisticParams> cache;
      auto it = cache.find(weapon_id);
      if (it != cache.end())
        return it->second.mass > 0.f ? &it->second : nullptr;
      BallisticParams params{};
      static const char *const kinds[][2] = {{"bombguns", "bomb"}, {"rocketguns", "rocket"}};
      for (const auto &kind : kinds) {
        DataBlock blk{};
        const std::string path = std::string("gamedata/weapons/") + kind[0] + "/" + weapon_id + ".blk";
        // ROBUST: most ids have no gun blk at all (smoke grenades, countermeasures),
        // and a plain load treats a missing file as fatal.
        if (!dblk::load(blk, path.c_str(), dblk::ReadFlags(dblk::ReadFlag::ROBUST)))
          continue;
        if (read_params(blk, kind[1], params))
          break;
        params = BallisticParams{};
      }
      auto &slot = cache.emplace(weapon_id, params).first->second;
      return slot.mass > 0.f ? &slot : nullptr;
    }
  } // namespace

  // powered:false for a store that was let go rather than fired. A jettisoned missile
  // keeps its motor unlit, so applying the thrust from its blk would fly it away under
  // its own power instead of dropping it.
  void buildBallisticArc(Rocket &store, bool powered, float sea_level) {
    if (!store.ballistic_positions.history().empty())
      return;
    // A streamed store needs nothing; without an end time there is nothing to
    // integrate up to.
    if (!store.positions.history().empty() || store.weapon_obj == nullptr)
      return;
    if (store.destroyed_at_ms == 0xFFFFFFFFu || store.destroyed_at_ms <= store.created_at_ms)
      return;
    const BallisticParams *pr = lookup(store.weapon_obj->weapon_name);
    if (!pr)
      return; // smoke grenades and countermeasures have no gun blk and need no arc

    // Both ends of the flight are taken from the packet times, and the arc is stamped
    // from created_at_ms rather than from creation_time. creation_time is the finer
    // release moment, 65 to 325 ms earlier on the test replays, but it is a different
    // clock: measuring the flight from it while ending on a packet time stretches the
    // arc by that lag, and a Tiny Tim then overshoots its recorded impact by 30 m
    // instead of 8. Staying on one clock also puts the arc in the same time base as
    // every other section and ends it exactly at destroyed_at_ms.
    const uint32_t base_ms = store.created_at_ms;
    const uint32_t span_ms = store.destroyed_at_ms - base_ms;
    const int steps = int(span_ms / STEP_MS);
    // Whatever is left over after the whole steps is walked as one shorter step, so
    // the arc ends exactly on destroyed_at_ms instead of up to 9 ms short of it.
    const uint32_t tail_ms = span_ms % STEP_MS;
    if (steps <= 0)
      return;

    Point3 pos = store.starting_pos;
    Point3 vel = store.starting_vel;
    uint32_t next_ms = 0;
    const int last = tail_ms ? steps + 1 : steps;
    for (int i = 0; i <= last; ++i) {
      // Time is stepped as an integer: accumulating it in float32 slips a sample
      // past the 100 ms boundary now and then.
      const uint32_t t_ms = (i <= steps) ? uint32_t(i) * STEP_MS : span_ms;
      const float t = float(t_ms) * 0.001f;
      if (t_ms >= next_ms || i == last) {
        SpaceTimeEuler sample{};
        sample.location = pos;
        const float speed = vel.length();
        if (speed > 1e-6f) {
          // Same convention as everywhere else: yaw from +X towards -Z, pitch up.
          // Roll stays zero and the reader is told not to trust it: a reconstruction
          // has no way to know how the store was rolled.
          sample.euler.y = atan2f(-vel.z, vel.x);
          sample.euler.z = asinf(vel.y / speed < -1.f ? -1.f : (vel.y / speed > 1.f ? 1.f : vel.y / speed));
        }
        store.ballistic_positions.pushAt(base_ms + t_ms, sample);
        next_ms += SAMPLE_MS;
      }
      if (i == last)
        break;
      const float step_s = (i == steps) ? float(tail_ms) * 0.001f : STEP_S;
      // Mass falls from mass to mass_end while the motor burns and stays there after:
      // a rocket does not get its propellant back. The median rocket of the game ends
      // at 0.79 of its launch mass and the lightest at 0.43, so keeping the launch
      // mass past burnout would understate drag by that much.
      float mass = pr->mass_end;
      float thrust = 0.f;
      if (!powered || pr->force <= 0.f) {
        mass = pr->mass;
      } else if (t < pr->time_fire) {
        mass = pr->mass + (pr->mass_end - pr->mass) * (t / pr->time_fire);
        thrust = pr->force;
      }
      const float speed = vel.length();
      Point3 acc{0.f, -GRAVITY, 0.f};
      if (speed > 1e-6f) {
        const Point3 dir = vel / speed;
        acc -= dir * (0.5f * air_density(pos.y - sea_level) * pr->cx * pr->area * speed * speed / mass);
        if (thrust > 0.f)
          acc += dir * (thrust / mass);
      }
      vel += acc * step_s;
      pos += vel * step_s;
    }
  }

} // namespace unit
