#include "math/dag_TMatrix.h"
#include "math/dag_mathAng.h"
#include "danet/dag_netUtils.h"
#include "state/ParserState.h"
#include "mpi/GeneralObject.h"
#include "Unit.h"

// Shot and hit packets, none of which the parser used to read.
//
// Every message here is published as it came: no record is merged with another and
// no value is recomputed, because joining these streams needs a time window or a
// projectile id and that call belongs to the caller. The one thing resolved here is
// the offender unit, which cannot wait: a uid is reused across respawns, so it only
// maps to a vehicle at the moment the packet is read.
//
// Field order comes from the dev build decompile, from Unit::sendHitEffects and from
// dm::HitVisualization::read plus its writer. Where the two disagreed with the data,
// the data won and the comment says so.
namespace mpi {

  namespace {
    constexpr float I16_SCALE = 1.f / 32767.f;
    constexpr float I8_SCALE = 1.f / 127.f;

    // Quantization ceiling of the local hit point. 262.5 is used for ships
    // (game unitType 5), 20 m for ground and air.
    constexpr float LOCAL_POS_MAX = 20.f;

    bool read_i16_scaled(const BitStream *bs, float scale, float &out) {
      int16_t v = 0;
      if (!bs->Read(v))
        return false;
      out = float(v) * I16_SCALE * scale;
      return true;
    }

    bool read_i8_scaled(const BitStream *bs, float scale, float &out) {
      int8_t v = 0;
      if (!bs->Read(v))
        return false;
      out = float(v) * I8_SCALE * scale;
      return true;
    }

    // netutils::pack_dir: 11 bits azimuth, 11 bits elevation, two sign bits.
    bool read_packed_dir(const BitStream *bs, Point3 &out) {
      uint8_t packed[3]{};
      if (!bs->ReadBits(packed, 24))
        return false;
      netutils::unpack_dir(packed, out);
      return true;
    }

    // 0xF15F body is byte aligned raw float32, so it is read by offset, not by bits.
    template<typename T>
    bool raw_at(const BitStream &bs, uint32_t offset, T &out) {
      if (offset + sizeof(T) > bs.GetNumberOfBytesUsed())
        return false;
      memcpy(&out, bs.GetData() + offset, sizeof(T));
      return true;
    }

    // netutils::read_idx is a plain LEB128, not the danet compressed form. Its
    // width shifts everything after it, which is why the body has no fixed layout.
    bool read_uleb(const BitStream &bs, uint32_t offset, uint32_t &out, uint32_t &size) {
      out = 0;
      size = 0;
      for (uint32_t i = 0; i < 5; ++i) {
        if (offset + i >= bs.GetNumberOfBytesUsed())
          return false;
        const uint8_t byte = bs.GetData()[offset + i];
        out |= uint32_t(byte & 0x7F) << (7 * i);
        if (!(byte & 0x80)) {
          size = i + 1;
          return true;
        }
      }
      return false;
    }

    bool raw_point3(const BitStream &bs, uint32_t offset, Point3 &out) {
      float v[3]{};
      if (offset + sizeof(v) > bs.GetNumberOfBytesUsed())
        return false;
      memcpy(v, bs.GetData() + offset, sizeof(v));
      out = Point3(v[0], v[1], v[2]);
      return true;
    }

    // Serialize props of dm::HitVisualization, taken from its static initializer.
    const netutils::FloatSerializeProps POS_PROPS{true, false, 30.f};
    const netutils::FloatSerializeProps ANGLE_PROPS{true, true, 6.2831855f};
    const netutils::FloatSerializeProps FIRE_RADIUS_PROPS{false, true, 1.f};
    const netutils::FloatSerializeProps EXPLOSION_RADIUS_PROPS{false, true, 50.f};
    // Closes a KineticHit and a Ricochet record.
    const netutils::FloatSerializeProps HIT_RATIO_PROPS{false, false, 1.f};

    bool skip_float(const BitStream &bs, const netutils::FloatSerializeProps &props) {
      float v = 0.f;
      return netutils::read_float(bs, v, props);
    }

    bool skip_dir(const BitStream &bs) {
      Point3 d{};
      return netutils::read_dir(bs, d);
    }

    bool skip_bit(const BitStream &bs) {
      bool v = false;
      return bs.Read(v);
    }
  } // namespace

  bool ProjectileKey::read(const BitStream *bs) {
    return bs->Read(offender_oid) && bs->Read(generation) && bs->Read(projectile_id) &&
           bs->Read(tail);
  }

  // 0xF0E9 UnitHitEffects

  bool UnitHitEffectsMessage::readPayload(ParserState *state) {
    hit.time_ms = state->curr_time_ms;
    // Position in the size table, needed to skip an unknown field by its entry
    // instead of desyncing the read offset.
    uint8_t field_no = 0;
    bool got_body = false;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      const uint8_t idx = field_no++;
      if (field_id != 1) {
        this->skipReadingField(idx);
        return true;
      }
      BitStream body{};
      if (!bs->Read(body))
        return false;
      if (!hit.projectile.read(&body))
        return false;
      // Fixed part after the key is 187 bits; the four zigzag fields add 5, 6 or 7
      // bytes, which gives the observed body lengths of 227 / 235 / 243 bits.
      // Only the point and the first direction are published; the rest is read to
      // reach the end of the body, which is what proves the layout is right.
      int16_t p[3]{};
      int skip_i = 0;
      float skip_f = 0.f;
      Point3 unused_dir{};
      bool skip_b = false;
      bool ok = body.ReadZigZag(skip_i);
      ok = ok && body.Read(p[0]) && body.Read(p[1]) && body.Read(p[2]);
      hit.local_pos = Point3(float(p[0]), float(p[1]), float(p[2])) * (I16_SCALE * LOCAL_POS_MAX);
      ok = ok && read_packed_dir(&body, hit.local_dir);
      ok = ok && read_packed_dir(&body, unused_dir);
      ok = ok && body.ReadZigZag(skip_i);
      ok = ok && body.Read(skip_b) && body.Read(skip_b) && body.Read(skip_b);
      ok = ok && read_i16_scaled(&body, 20.f, skip_f);
      ok = ok && read_i16_scaled(&body, 20.f, skip_f);
      ok = ok && read_i16_scaled(&body, 1.f, skip_f);
      ok = ok && read_i16_scaled(&body, 100.f, skip_f);
      ok = ok && body.ReadZigZag(skip_i);
      ok = ok && read_i8_scaled(&body, 5.f, skip_f);
      ok = ok && body.ReadZigZag(skip_i);
      ok = ok && read_i8_scaled(&body, PI, skip_f);
      ok = ok && read_i8_scaled(&body, PI, skip_f);
      if (!ok)
        return false;
      // Body is padded to a byte, so only up to 7 bits may be left.
      hit.complete = body.GetNumberOfUnreadBits() < 8;
      got_body = true;
      return true;
    });
    if (got_body)
      hit.offender_unit = state->getUnitObj(hit.projectile.offender_uid());
    return valid && got_body;
  }

  // 0xF15F hit analysis

  bool UnitHitAnalysisMessage::readPayload(ParserState *state) {
    analysis.time_ms = state->curr_time_ms;
    // Position in the size table, needed to skip an unknown field by its entry
    // instead of desyncing the read offset.
    uint8_t field_no = 0;
    bool got_body = false;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      const uint8_t idx = field_no++;
      // Fields 1 and 3 both carry the offender oid.
      if (field_id == 1 || field_id == 3)
        return bs->Read(analysis.offender_oid);
      if (field_id != 2) {
        this->skipReadingField(idx);
        return true;
      }
      BitStream body{};
      if (!bs->Read(body))
        return false;
      uint32_t type = 0, type_size = 0;
      bool ok = raw_at(body, 0, analysis.version);
      ok = ok && raw_at(body, 2, analysis.time_s);
      ok = ok && read_uleb(body, 6, type, type_size);
      if (!ok)
        return false;
      analysis.projectile_type = type;
      // Every Point3 in the body is SIMD padded to 16 bytes.
      const uint32_t p = 6 + type_size;
      ok = raw_at(body, p, analysis.projectile_uid);
      ok = ok && raw_point3(body, p + 16, analysis.pos);
      ok = ok && raw_point3(body, p + 32, analysis.dir);
      ok = ok && raw_point3(body, p + 48, analysis.local_pos);
      ok = ok && raw_point3(body, p + 64, analysis.local_dir);
      ok = ok && raw_at(body, p + 80, analysis.speed);
      ok = ok && raw_at(body, p + 84, analysis.travel_distance);
      ok = ok && raw_at(body, p + 92, analysis.seed);
      if (!ok)
        return false;
      got_body = true;
      return true;
    });
    if (got_body)
      analysis.offender_unit = state->getUnitObj(analysis.offender_oid & 0x7FF);
    return valid && got_body;
  }

  // UnitOnEffectiveHit 0xF117 / UnitOnEffectiveCritHit 0xF13C

  bool UnitOnEffectiveHitMessage::readPayload(ParserState *state) {
    // Position in the size table, needed to skip an unknown field by its entry
    // instead of desyncing the read offset.
    uint8_t field_no = 0;
    damage.time_ms = state->curr_time_ms;
    damage.crit = this->id == MPI_PACKETS::UnitOnEffectiveCritHit;
    const bool is_crit = damage.crit;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      const uint8_t idx = field_no++;
      switch (field_id) {
        case 1: return damage.projectile.read(bs);
        case 2: return bs->Read(damage.damage_class);
        case 3: {
          // The crit message has one more byte between the class and the amount. It is
          // the CritDebuffType of the engine: game.vromfs calls this very message as
          // onEffectiveHit(offender_data, amount, uint8, CritDebuffType) in
          // game/infantry/es/entity_damage.das, passing CRIT_TYPE_NONE there. That
          // none is zero and rides the plain 0xF117, which is why this byte is only
          // ever 1, 2 or 3 here (19, 41 and 27 times on rnd); 2 carries the heavier
          // amounts, median 2336 against about 713. The names behind 1 to 3 live in
          // the engine, not in the scripts. The field table of both messages is
          // otherwise fully read: 0xF117 carries the key, the class byte and the
          // amount, and nothing else, on all 457 records of rnd.
          if (!is_crit)
            return bs->Read(damage.amount);
          uint8_t unknown = 0;
          return bs->Read(unknown);
        }
        case 4:
          if (is_crit)
            return bs->Read(damage.amount);
          break;
        default: break;
      }
      this->skipReadingField(idx);
      return true;
    });
    // Damage records come without a projectile key when the game does not name the
    // round: the key is the all ones sentinel and the oid inside it is the victim,
    // so resolving it would invent an attacker who is the target itself. Records where
    // offender and offended still come out equal do remain - 8 over four replays - but
    // those carry valid keys with real generations, so that is the server attributing
    // damage to the victim itself, not this sentinel leaking through.
    if (damage.projectile.valid())
      damage.offender_unit = state->getUnitObj(damage.projectile.offender_uid());
    return valid;
  }

  // 0xF0C2 UnitLastEffectiveHit: the dm::HitVisualization blob

  bool UnitLastEffectiveHitMessage::readPayload(ParserState *state) {
    // Position in the size table, needed to skip an unknown field by its entry
    // instead of desyncing the read offset.
    uint8_t field_no = 0;
    outcome.time_ms = state->curr_time_ms;
    bool got_body = false;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      const uint8_t idx = field_no++;
      if (field_id == 1)
        return bs->Read(outcome.offender_oid);
      if (field_id == 2)
        return bs->Read(outcome.projectile_uid);
      if (field_id != 3) {
        this->skipReadingField(idx);
        return true;
      }
      BitStream body{};
      if (!bs->Read(body))
        return false;
      got_body = true;

      // Record layouts differ from the dev build decompile: KineticHit carries a
      // direction and a trailing flag, Ricochet a leading flag. Both were measured
      // against a hard invariant, the part count of the victim damage model, which
      // is constant per vehicle and sits right after these lists.
      uint16_t n16 = 0;
      uint8_t n8 = 0;
      bool ok = body.ReadCompressed(n16);
      for (uint16_t i = 0; ok && i < n16; ++i) { // ProjectileEvent
        uint8_t type = 0, unused = 0;
        ok = body.Read(type) && body.Read(unused);
        ok = ok && skip_float(body, POS_PROPS) && skip_float(body, POS_PROPS) &&
             skip_float(body, POS_PROPS) && skip_dir(body);
        if (ok && type == 6)
          ok = skip_float(body, EXPLOSION_RADIUS_PROPS);
      }
      int part = 0;
      ok = ok && body.Read(n8);
      for (uint8_t i = 0; ok && i < n8; ++i) { // KineticHit
        // The record carries no direction; it closes with a float instead.
        ok = body.ReadZigZag(part) && skip_bit(body) && skip_float(body, ANGLE_PROPS) &&
             skip_float(body, ANGLE_PROPS) && skip_float(body, HIT_RATIO_PROPS);
        if (ok)
          outcome.kinetic_parts.push_back(part);
      }
      ok = ok && body.Read(n8);
      for (uint8_t i = 0; ok && i < n8; ++i) { // CumulativeHit
        ok = body.ReadZigZag(part) && skip_bit(body);
        if (ok)
          outcome.cumulative_parts.push_back(part);
      }
      ok = ok && body.Read(n8);
      for (uint8_t i = 0; ok && i < n8; ++i) { // Ricochet
        // No leading flag, and the same closing float as a KineticHit.
        ok = body.ReadZigZag(part) && skip_dir(body) && skip_float(body, ANGLE_PROPS) &&
             skip_float(body, ANGLE_PROPS) && skip_float(body, ANGLE_PROPS) &&
             skip_float(body, HIT_RATIO_PROPS);
        if (ok)
          outcome.ricochet_parts.push_back(part);
      }

      // PartState: one presence bit per part of the damage model, eight more bits
      // for the parts whose state came with this hit.
      ok = ok && body.ReadCompressed(n16);
      if (ok)
        outcome.part_count = n16;
      for (uint16_t i = 0; ok && i < n16; ++i) {
        bool present = false;
        ok = body.Read(present);
        if (ok && present) {
          uint8_t nibble = 0, pair = 0;
          ok = skip_bit(body) && skip_bit(body) && body.ReadBits(&nibble, 4) &&
               body.ReadBits(&pair, 2);
          if (ok)
            outcome.changed_parts.push_back(i);
        }
      }

      ok = ok && body.ReadCompressed(n16);
      for (uint16_t i = 0; ok && i < n16; ++i) { // FireSpawn
        ok = body.ReadZigZag(part) && skip_float(body, POS_PROPS) &&
             skip_float(body, POS_PROPS) && skip_float(body, POS_PROPS) &&
             skip_float(body, FIRE_RADIUS_PROPS);
        if (ok)
          outcome.fire_parts.push_back(part);
      }
      outcome.complete = ok;
      return true;
    });
    outcome.offender_unit = state->getUnitObj(outcome.offender_oid & 0x7FF);
    return valid && got_body;
  }

  // 0xF133 UnitOnExplosion

  bool UnitOnExplosionMessage::readPayload(ParserState *state) {
    // Position in the size table, needed to skip an unknown field by its entry
    // instead of desyncing the read offset.
    uint8_t field_no = 0;
    explosion.time_ms = state->curr_time_ms;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      const uint8_t idx = field_no++;
      if (field_id == 1)
        return explosion.projectile.read(bs);
      // Fields 2 and 3 are single bits and field 4 is a u32 that is zero in every
      // record on both test replays; none of them are published.
      this->skipReadingField(idx);
      return true;
    });
    if (explosion.projectile.valid())
      explosion.offender_unit = state->getUnitObj(explosion.projectile.offender_uid());
    return valid;
  }

  // 0xF144 UnitOnHit

  bool UnitOnHitMessage::readPayload(ParserState *state) {
    // Position in the size table, needed to skip an unknown field by its entry
    // instead of desyncing the read offset.
    uint8_t field_no = 0;
    direction.time_ms = state->curr_time_ms;
    uint8_t packed[3]{};
    uint8_t got = 0;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      const uint8_t idx = field_no++;
      if (field_id >= 1 && field_id <= 3) {
        if (!bs->Read(packed[field_id - 1]))
          return false;
        got++;
        return true;
      }
      this->skipReadingField(idx);
      return true;
    });
    if (got == 3)
      netutils::unpack_dir(packed, direction.world_dir);
    return valid && got == 3;
  }

  // 0xF0BD UnitBulletRearm

  bool UnitBulletRearmMessage::readPayload(ParserState *state) {
    // Position in the size table, needed to skip an unknown field by its entry
    // instead of desyncing the read offset.
    uint8_t field_no = 0;
    ammo.time_ms = state->curr_time_ms;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      const uint8_t idx = field_no++;
      switch (field_id) {
        case 1: return bs->Read(ammo.barrel);
        case 2: return bs->Read(ammo.slot);
        case 3: return bs->Read(ammo.rounds_left);
        default: break;
      }
      this->skipReadingField(idx);
      return true;
    });
    return valid;
  }

  // 0xF0B1 single shot, 0xF01B / 0xF01C trigger

  bool UnitShotMessage::readPayload(ParserState *state) {
    // Position in the size table, needed to skip an unknown field by its entry
    // instead of desyncing the read offset.
    uint8_t field_no = 0;
    shot.time_ms = state->curr_time_ms;
    switch (this->id) {
      case MPI_PACKETS::UnitSingleShot: shot.kind = ShotSingle; break;
      case MPI_PACKETS::GmDoStartFire: shot.kind = ShotFireStart; break;
      default: shot.kind = ShotFireStop; break;
    }
    const bool is_single = shot.kind == ShotSingle;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      const uint8_t idx = field_no++;
      // Single shot has three 8 bit fields, all zero: it only carries the fact.
      // The weapon comes from the 0xF01B in the same millisecond.
      if (!is_single && field_id == 1) {
        uint8_t v = 0;
        if (!bs->Read(v))
          return false;
        shot.weapon_id = int16_t(v);
        return true;
      }
      this->skipReadingField(idx);
      return true;
    });
    return valid;
  }
} // namespace mpi
