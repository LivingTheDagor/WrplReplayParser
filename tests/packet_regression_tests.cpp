#include "mpi/codegen/mpiUiStubs.h"
#include "init/initialize.h"
#include "state/ParserState.h"
#include "mpi/PositionSync.h"
#include "mpi/GeneralObject.h"
#include "idFieldSerializer.h"
#include "Unit.h"
#include "ecs/ComponentTypesDefs.h"
#include "Replay/ReplayReader.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cmath>

std::vector<Rocket *> collect_all_rockets(ParserState &state);

static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

static BitStream detection(unsigned a, unsigned b, bool short_fields, bool include_second = true) {
  BitStream bs;
  IdFieldSerializer32 sizes;
  bs.Write(uint16_t(0));
  bs.WriteCompressed(uint32_t(include_second ? 6 : 2));
  if (short_fields) { bs.Write(uint8_t(a)); sizes.setFieldSize(8); }
  else { bs.Write(uint16_t(a)); sizes.setFieldSize(16); }
  if (include_second) {
    if (short_fields) { bs.Write(uint8_t(b)); sizes.setFieldSize(8); }
    else { bs.Write(uint16_t(b)); sizes.setFieldSize(16); }
  }
  sizes.writeFieldsSize(bs);
  return bs;
}

static BitStream ground(bool truncate = false) {
  BitStream bs;
  bs.Write(uint16_t(0)); bs.Write(uint16_t(1));
  bs.Write(float(1)); bs.Write(uint16_t(0));
  bs.Write(true); bs.Write(false); bs.Write(false); // present, flag, extra
  bs.Write(false); bs.Write(false); // val2, val3
  bs.Write(false); bs.Write(false); bs.Write(uint8_t(1)); bs.Write(false); // uncompressed
  bs.AlignWriteToByteBoundary();
  bs.Write(uint8_t(9)); bs.Write(false); // spawn, no motion
  bs.Write(false); bs.Write(false); // no turret angles, no player
  bs.Write(false); bs.Write(uint8_t(0)); bs.Write(false); bs.Write(false); bs.Write(false);
  bs.Write(uint8_t(0)); bs.Write(uint8_t(0)); // sensors and countermeasures
  uint8_t zero = 0; bs.WriteBits(&zero, 4);
  if (truncate) return bs;
  bs.Write(false); // no aim
  bs.Write(true); bs.Write(true); // ammo list and weapon record
  bs.Write(uint8_t(0x21)); bs.Write(float(4)); bs.Write(float(5));
  bs.Write(uint8_t(2)); bs.Write(uint16_t(7)); bs.Write(uint16_t(8));
  bs.Write(false); // no seeker
  bs.AlignWriteToByteBoundary();
  return bs;
}

// ground() with one off sensor, cut just before the countermeasure count: the sensor event was
// already appended when the read fails, so it must be taken back.
static BitStream ground_sensors_cut() {
  BitStream bs;
  bs.Write(uint16_t(0)); bs.Write(uint16_t(1));
  bs.Write(float(1)); bs.Write(uint16_t(0));
  bs.Write(true); bs.Write(false); bs.Write(false); // present, flag, extra
  bs.Write(false); bs.Write(false); // val2, val3
  bs.Write(false); bs.Write(false); bs.Write(uint8_t(1)); bs.Write(false); // uncompressed
  bs.AlignWriteToByteBoundary();
  bs.Write(uint8_t(9)); bs.Write(false); // spawn, no motion
  bs.Write(false); bs.Write(false); // no turret angles, no player
  bs.Write(false); bs.Write(uint8_t(0)); bs.Write(false); bs.Write(false); bs.Write(false);
  bs.Write(uint8_t(1)); // one sensor
  bs.Write(false); bs.Write(uint8_t(0x20)); // kind 2, off: nothing else is sent
  bs.Write(uint8_t(0)); // the byte that follows a non-empty sensor list
  return bs; // no countermeasure count: the read that fails
}

// ground() with a motion block that is read to its last field and then cut off, so the position
// was already computed when the record turns out to be short.
static BitStream ground_motion_cut() {
  BitStream bs;
  bs.Write(uint16_t(0)); bs.Write(uint16_t(1));
  bs.Write(float(1)); bs.Write(uint16_t(0));
  bs.Write(true); bs.Write(false); bs.Write(false); // present, flag, extra
  bs.Write(false); bs.Write(false); // val2, val3
  bs.Write(false); bs.Write(false); bs.Write(uint8_t(0)); bs.Write(false); // uncompressed, no weapons
  bs.AlignWriteToByteBoundary();
  bs.Write(uint8_t(9)); bs.Write(true); // spawn, motion
  int16_t euler[3]{}; bs.Write(euler); // val1 is off: the euler comes over three int16
  bs.Write(false); // not awake
  bs.Write(false); // not the packed position
  bs.Write(uint32_t(0)); bs.Write(uint32_t(0)); // its two words
  int16_t velocity[3]{}; bs.Write(velocity);
  int8_t angular[3]{}; bs.Write(angular);
  bs.Write(false); // no drive block
  bs.Write(uint8_t(0)); // engine
  bs.Write(false); // not a ship
  bs.Write(false); bs.Write(false); // no flag_b6, not alive
  bs.Write(false); // no further state block
  bs.Write(false); // no sub-vehicles
  return bs; // no weapon or sensor count: the read that fails
}

// ground_motion_cut() with the fields that follow the motion block, so the record is complete and
// the position it carries has to be published.
static BitStream ground_motion_ok() {
  BitStream bs = ground_motion_cut();
  bs.Write(false); bs.Write(false); // no turret angles, no player
  bs.Write(uint8_t(0)); bs.Write(uint8_t(0)); // sensors and countermeasures
  uint8_t zero = 0; bs.WriteBits(&zero, 4);
  bs.Write(false); // no aim
  bs.Write(false); // no ammo list
  bs.Write(false); // no seeker
  bs.AlignWriteToByteBoundary();
  return bs;
}

// The record of `in`, without its 80-bit packet header, appended to `out`. A record starts and
// ends on a byte boundary, so the copy keeps its alignment.
static void append_record(BitStream &out, BitStream in) {
  in.SetReadOffset(80);
  bool bit;
  while (in.GetNumberOfUnreadBits() && in.Read(bit))
    out.Write(bit);
}

// Two ground records, uids 0 and 1: a complete motion record, then `second`.
static BitStream ground_pair(const BitStream &second) {
  BitStream bs;
  bs.Write(uint16_t(0)); bs.Write(uint16_t(2));
  bs.Write(float(1)); bs.Write(uint16_t(0));
  append_record(bs, ground_motion_ok());
  append_record(bs, second);
  return bs;
}

// A compressed ground record whose delta names a base packet that never came, followed by a
// seeker block: the state is skipped, the rest of the record is still valid.
static BitStream ground_missing_base() {
  BitStream bs;
  bs.Write(uint16_t(0)); bs.Write(uint16_t(1));
  bs.Write(float(1)); bs.Write(uint16_t(0));
  bs.Write(true); bs.Write(false); bs.Write(false); // present, flag, extra
  bs.Write(false); bs.Write(false); // val2, val3
  bs.Write(false); bs.Write(false); bs.Write(uint8_t(0)); bs.Write(true); // compressed, no weapons
  uint32_t base_delta = 1, next = 2;
  bs.Write(false); // not a full diff
  bs.WriteBits(reinterpret_cast<uint8_t *>(&base_delta), 5);
  bs.WriteBits(reinterpret_cast<uint8_t *>(&next), 13);
  bs.WriteCompressed(uint16_t(0));
  bs.AlignWriteToByteBoundary();
  bs.Write(false); // no aim
  bs.Write(false); // no ammo list
  bs.Write(true); bs.Write(true); bs.Write(uint32_t(8)); bs.Write(uint8_t(0xAB)); // a seeker block
  bs.AlignWriteToByteBoundary();
  return bs;
}

static BitStream nested_payload(const BitStream &nested) {
  BitStream bs;
  bs.Write(nested);
  return bs;
}

static BitStream aircraft(bool fail_second) {
  BitStream bs;
  bs.Write(false); bs.Write(false); bs.Write(false); bs.Write(false); // uid 1, data, flags
  bs.Write(false); bs.Write(uint32_t(0)); bs.Write(false); bs.Write(false);
  bs.WriteZigZag(1); bs.WriteZigZag(10); bs.WriteZigZag(0);
  bs.Write(Point3{}); bs.Write(uint32_t(0)); bs.Write(uint32_t(0));
  bs.AlignWriteToByteBoundary();
  uint8_t controls[7]{}; bs.Write(controls); bs.Write(uint8_t(0));
  bs.Write(uint8_t(0)); bs.Write(uint8_t(0));
  uint8_t zero = 0; bs.WriteBits(&zero, 4);
  bs.Write(false); bs.Write(false);
  if (fail_second) {
    bs.Write(false); bs.Write(false); bs.Write(false); bs.Write(false); // uid 2 is missing
  } else {
    bs.Write(true); bs.WriteCompressed(uint16_t(0x7ff));
    float tail[3]{}; bs.Write(tail);
  }
  return bs;
}

static void check_store_owner(const char *path) {
  ServerReplay replay(path);
  ParserState state(&replay);
  std::unique_ptr<IReplayReader> reader(replay.getReplayReader());
  ReplayPacket pkt{};
  while (reader->getNextPacket(pkt)) {
    if (!state.ParsePacket(pkt)) {
      require(pkt.type == ReplayPacketType::EndMarker, "aircraft failure in store test replay");
      break;
    }
  }
  for (auto *store : collect_all_rockets(state)) {
    auto *owner = store->owned_by;
    auto *weapon = store->weapon_obj;
    if (!owner || !weapon || !state.g_entity_mgr.doesEntityExist(owner->curr_eid))
      continue;
    const std::string name = weapon->weapon_name;
    require(state.g_entity_mgr.destroyEntity(owner->curr_eid, false, true), "owner destruction");
    require(store->owned_by == owner && store->weapon_obj == weapon && weapon->weapon_name == name,
            "store weapon did not survive owner destruction");
    std::cout << "store retained weapon after owner destruction\n";
    return; // The state and remaining stores are destroyed on exit from this function.
  }
  throw std::runtime_error("no suitable store and owner in test replay");
}

// This particular replay contains an aircraft sync 40 ms before ECS creation.
static void check_early_aircraft_replay(const char *path) {
  ServerReplay replay(path);
  ParserState state(&replay);
  std::unique_ptr<IReplayReader> reader(replay.getReplayReader());
  unit::Aircraft *early = nullptr;
  bool created = false;
  bool reached_end = false;
  ReplayPacket pkt{};
  while (reader->getNextPacket(pkt)) {
    if (!state.ParsePacket(pkt)) {
      require(pkt.type == ReplayPacketType::EndMarker, "early-sync replay packet failed");
      reached_end = true;
      break;
    }
    if (pkt.type == ReplayPacketType::AircraftSmall && pkt.timestamp_ms == 84514) {
      early = state.getPendingAircraft(400);
      require(early && early->provisional, "real early aircraft not retained");
      const auto &positions = early->positions.history();
      require(positions.size() == 1 && positions[0].time_ms == 84514 &&
              std::abs(positions[0].data.location.y - 5500.f) < 0.01f,
              "real early aircraft position was lost");
      size_t ammo_count = 0, sensor_count = 0, control_count = 0;
      for (const auto &event : state.AmmoEvents)
        if (event.time_ms == 84514 && event.unit == early && event.rounds_left == 300)
          ++ammo_count;
      for (const auto &event : state.SensorEvents)
        if (event.time_ms == 84514 && event.unit == early)
          ++sensor_count;
      for (const auto &event : state.ControlEvents)
        if (event.time_ms == 84514 && event.unit == early && event.engines.size() == 1)
          ++control_count;
      require(ammo_count == 6 && sensor_count == 1 && control_count == 1, "real early aircraft events were lost");
    }
    if (early && !created && state.getUnitObj(400) == early) {
      require(pkt.timestamp_ms == 84554 && !early->provisional &&
              early->unit_name == "f-86_canadair_german", "real aircraft creation did not reuse early data");
      created = true;
    }
  }
  require(reached_end && state.replay_length_ms == 1224969 && early && created &&
          early->positions.history()[0].time_ms == 84514,
          "early-sync replay did not complete with its original position");
  bool early_seeker = false;
  for (const auto &event : state.SeekerEvents)
    if (event.time_ms == 86517 && event.unit && event.unit->uid == 399 &&
        event.unit->created_at_ms == 86558 && event.unit->unit_name == "yak-38m")
      early_seeker = true;
  require(early_seeker, "early aircraft seeker was lost or attached to the wrong unit");
  std::cout << "early aircraft replay reached end and retained its first sync\n";
}

int main() {
  const char *game_path = std::getenv("WRPL_TEST_GAME_PATH");
  if (!game_path || !*game_path) {
    std::cerr << "Set WRPL_TEST_GAME_PATH to a War Thunder installation to run this test\n";
    return 77;
  }
  initialize(game_path, "", "");
  ParserState state(uint32_t(0));
  for (bool invalid : {false, true}) {
    mpi::UnitDetectedMessage msg(nullptr);
    msg.payload = detection(17, 29, invalid);
    bool ok = static_cast<mpi::Message &>(msg).readPayload(&state);
    require(ok != invalid, "detection width acceptance");
    if (ok) require(msg.spot.spotter_id == 17 && msg.spot.spotted_id == 29, "detection values");
  }
  mpi::UnitDetectedMessage absent(nullptr);
  absent.payload = detection(17, 0, false, false);
  require(!static_cast<mpi::Message &>(absent).readPayload(&state), "detection missing field");
  mpi::UnitDetectedMessage truncated(nullptr);
  truncated.payload = detection(17, 29, false);
  truncated.payload.SetWriteOffset(truncated.payload.GetWriteOffset() - 8);
  require(!static_cast<mpi::Message &>(truncated).readPayload(&state), "truncated detection table");
  mpi::UnitDetectedMessage oversized(nullptr);
  oversized.payload.Write(uint16_t(0));
  oversized.payload.WriteCompressed(uint32_t(6));
  oversized.payload.Write(uint8_t(17)); oversized.payload.Write(uint8_t(29));
  IdFieldSerializer32 oversized_sizes;
  oversized_sizes.setFieldSize(16); oversized_sizes.setFieldSize(16);
  oversized_sizes.writeFieldsSize(oversized.payload);
  require(!static_cast<mpi::Message &>(oversized).readPayload(&state), "oversized detection fields");
  mpi::UnitDetectedMessage bad_flags(nullptr);
  bad_flags.payload.Write(uint16_t(0));
  bad_flags.payload.Write(uint8_t(0x86));
  for (int i = 0; i < 4; ++i) bad_flags.payload.Write(uint8_t(0x80));
  bad_flags.payload.Write(uint16_t(17)); bad_flags.payload.Write(uint16_t(29));
  IdFieldSerializer32 bad_flags_sizes;
  bad_flags_sizes.setFieldSize(16); bad_flags_sizes.setFieldSize(16);
  bad_flags_sizes.writeFieldsSize(bad_flags.payload);
  require(!static_cast<mpi::Message &>(bad_flags).readPayload(&state), "unterminated detection field mask");
  state.keep.ground_syncs = true;
  auto gs = ground();
  require(GMSync(state, gs), "valid uncompressed ground");
  require(state.GroundSyncEvents.size() == 1 && state.GroundWeapons.size() == 1 && state.GroundRounds.size() == 2, "ground pools");
  require(state.GroundRounds[0] == 7 && state.GroundRounds[1] == 8, "ground ammo");
  auto bad_gs = ground(true);
  require(!GMSync(state, bad_gs), "truncated ground accepted");
  require(state.GroundSyncEvents.size() == 1 && state.GroundWeapons.size() == 1 && state.GroundRounds.size() == 2, "ground rollback");
  auto *plane = new unit::Aircraft(&state, 1);
  state.ownUnit(plane);
  state.setUnitData(1, plane, ecs::INVALID_ENTITY_ID);
  auto bad_fm = aircraft(true);
  ReplayPacket pkt{}; pkt.type = ReplayPacketType::AircraftSmall; pkt.timestamp_ms = 10; pkt.stream = bad_fm;
  require(!state.ParsePacket(pkt), "aircraft failure was hidden");
  require(state.AmmoEvents.empty() && state.ControlEvents.empty() && plane->positions.history().empty(), "aircraft rollback");
  auto good_fm = aircraft(false);
  pkt.timestamp_ms = 20; pkt.stream = good_fm;
  require(state.ParsePacket(pkt), "valid aircraft rejected");
  require(state.AmmoEvents.size() == 1 && state.ControlEvents.size() == 1 && plane->positions.history().size() == 1, "aircraft commit");

  // A ground record cut short after its sensor list, and one that carries a motion block but ends
  // before its last field: neither may keep the sensor events, the position or the pools it already
  // read, and the failure must reach the dispatcher that fronts GMSync, not just the log.
  ParserState ground_units(uint32_t(0));
  ground_units.keep.ground_syncs = true;
  auto *ground_tank = new unit::Tank(&ground_units, 0);
  ground_units.ownUnit(ground_tank);
  ground_units.setUnitData(0, ground_tank, ecs::INVALID_ENTITY_ID);
  mpi::GeneralObject ground_dispatcher(&ground_units);
  mpi::TankMessage ground_msg(&ground_dispatcher, MPI_PACKETS::Tank1);

  // A record that carries a motion block publishes the position it describes ...
  ground_msg.data = ground_motion_ok();
  require(ground_dispatcher.applyMpiMessage(&ground_msg), "complete motion record rejected by the dispatcher");
  require(ground_tank->positions.history().size() == 1, "complete motion record did not publish the position");

  // ... while the same record cut before its last field must not: the position is held back until
  // the record is known good, so a short read cannot leave a half-applied vehicle behind.
  ground_msg.data = ground_motion_cut();
  require(!ground_dispatcher.applyMpiMessage(&ground_msg), "cut motion record accepted by the dispatcher");
  require(ground_tank->positions.history().size() == 1, "cut motion record published the position");

  // A record cut after its sensor list must take the sensor event back and drop the pools it filled.
  const size_t ground_events = ground_units.GroundSyncEvents.size();
  ground_msg.data = ground_sensors_cut();
  require(!ground_dispatcher.applyMpiMessage(&ground_msg), "truncated ground record accepted by the dispatcher");
  require(ground_units.SensorEvents.empty() && ground_units.GroundSyncEvents.size() == ground_events &&
          ground_units.GroundWeapons.empty() && ground_units.GroundRounds.empty() &&
          ground_tank->positions.history().size() == 1,
          "truncated ground record published partial state");

  // A valid record still commits after those failures.
  ground_msg.data = ground();
  require(ground_dispatcher.applyMpiMessage(&ground_msg), "valid ground record rejected after a failure");
  require(ground_units.GroundSyncEvents.size() == ground_events + 1 && ground_units.GroundWeapons.size() == 1 &&
          ground_units.SensorEvents.empty() && ground_tank->positions.history().size() == 1,
          "valid ground record did not commit");

  // The same failure must come back through the message listener chain ParserState::ParsePacket uses.
  mpi::TankMessage ground_sent(&ground_dispatcher, MPI_PACKETS::Tank1);
  ground_sent.data = ground_sensors_cut();
  require(!mpi::send(&ground_sent), "ground failure did not propagate through mpi::send");

  // A sync can precede the ECS creation. Retain its data and identity, and
  // attach the same aircraft when the corresponding creation arrives.
  ParserState early(uint32_t(0));
  ReplayPacket early_pkt{};
  early_pkt.type = ReplayPacketType::AircraftSmall;
  early_pkt.timestamp_ms = 100;
  early_pkt.stream = aircraft(false);
  require(early.ParsePacket(early_pkt), "early aircraft sync rejected");
  auto *pending = early.getPendingAircraft(1);
  require(pending && pending->provisional && pending->uid == 1, "missing provisional aircraft");
  require(pending->positions.history().size() == 1 && pending->positions.history()[0].time_ms == 100,
          "early aircraft position lost or retimed");
  require(early.AmmoEvents.size() == 1 && early.AmmoEvents[0].unit == pending &&
          early.ControlEvents.size() == 1 && early.ControlEvents[0].unit == pending,
          "early aircraft events lost");
  early_pkt.timestamp_ms = 120;
  early_pkt.stream = aircraft(false);
  require(early.ParsePacket(early_pkt), "second early aircraft sync rejected");
  require(early.getPendingAircraft(1) == pending && pending->positions.history().size() == 2 &&
          pending->positions.history()[1].time_ms == 120, "early syncs did not share history");
  early_pkt.timestamp_ms = 130;
  early_pkt.stream = aircraft(true);
  require(!early.ParsePacket(early_pkt), "truncated early aircraft packet accepted");
  require(early.getPendingAircraft().size() == 1 && pending->positions.history().size() == 2 &&
          early.AmmoEvents.size() == 2 && early.ControlEvents.size() == 2,
          "truncated packet leaked provisional data");
  require(early.takePendingAircraft(1) == pending && !pending->provisional && early.getPendingAircraft().empty(),
          "aircraft creation did not retain the provisional object");
  early.setUnitData(1, pending, ecs::INVALID_ENTITY_ID);
  early_pkt.timestamp_ms = 140;
  early_pkt.stream = aircraft(false);
  require(early.ParsePacket(early_pkt), "aircraft sync after creation rejected");
  require(pending->positions.history().size() == 3 && pending->positions.history()[2].time_ms == 140 &&
          early.ControlEvents.back().unit == pending, "post-creation history changed identity");
  early_pkt.type = ReplayPacketType::EndMarker;
  early_pkt.timestamp_ms = 200;
  require(!early.ParsePacket(early_pkt) && early.finishedLoading(), "end marker");
  early.rewindToMs(110);
  require(pending->positions.getCurrIndex() == 0, "early position rewind");
  early.rewindToMs(150);
  require(pending->positions.getCurrIndex() == 2, "early position replay");

  ParserState broken_early(uint32_t(0));
  ReplayPacket broken_pkt{};
  broken_pkt.type = ReplayPacketType::AircraftSmall;
  broken_pkt.timestamp_ms = 100;
  broken_pkt.stream = aircraft(true);
  require(!broken_early.ParsePacket(broken_pkt) && broken_early.getPendingAircraft().empty() &&
          broken_early.AmmoEvents.empty() && broken_early.ControlEvents.empty(),
          "truncated packet created provisional aircraft");

  ParserState reused(uint32_t(0));
  ReplayPacket reused_pkt{};
  reused_pkt.type = ReplayPacketType::AircraftSmall;
  reused_pkt.timestamp_ms = 10;
  reused_pkt.stream = aircraft(false);
  require(reused.ParsePacket(reused_pkt), "first incarnation sync");
  auto *first = reused.takePendingAircraft(1);
  require(first != nullptr, "first incarnation missing");
  reused.setUnitData(1, first, ecs::INVALID_ENTITY_ID);
  reused.setUnitData(1, nullptr, ecs::INVALID_ENTITY_ID);
  reused_pkt.timestamp_ms = 20;
  reused_pkt.stream = aircraft(false);
  require(reused.ParsePacket(reused_pkt), "second incarnation sync");
  auto *second = reused.getPendingAircraft(1);
  require(second && second != first && first->positions.history().size() == 1 &&
          second->positions.history().size() == 1 && reused.ControlEvents.back().unit == second,
          "reused UID retained the old aircraft");
  reused_pkt.type = ReplayPacketType::EndMarker;
  reused_pkt.timestamp_ms = 30;
  require(!reused.ParsePacket(reused_pkt) && reused.getPendingAircraft(1) == second && second->provisional &&
          second->positions.history()[0].time_ms == 20,
          "unresolved aircraft data was lost at end of replay");

  // A sync long after the last one for a pending UID is another aircraft, and a creation long after
  // the last sync does not claim the pending one.
  ParserState stale(uint32_t(0));
  ReplayPacket stale_pkt{};
  stale_pkt.type = ReplayPacketType::AircraftSmall;
  stale_pkt.timestamp_ms = 10;
  stale_pkt.stream = aircraft(false);
  require(stale.ParsePacket(stale_pkt), "first stale sync");
  auto *old_sync = stale.getPendingAircraft(1);
  stale_pkt.timestamp_ms = 6000;
  stale_pkt.stream = aircraft(false);
  require(stale.ParsePacket(stale_pkt), "second stale sync");
  auto *new_sync = stale.getPendingAircraft(1);
  require(old_sync && new_sync && new_sync != old_sync && old_sync->positions.history().size() == 1 &&
          new_sync->positions.history().size() == 1 && stale.getPendingAircraft().size() == 2,
          "a sync long after the last went to the old pending aircraft");
  ReplayPacket later{};
  later.type = ReplayPacketType::StartMarker;
  later.timestamp_ms = 12000;
  require(stale.ParsePacket(later) && stale.takePendingAircraft(1) == nullptr && new_sync->provisional &&
          stale.getPendingAircraft().size() == 2, "a late creation claimed a stale pending aircraft");

  ParserState other_type(uint32_t(0));
  auto *tank = new unit::Tank(&other_type, 1);
  other_type.ownUnit(tank);
  other_type.setUnitData(1, tank, ecs::INVALID_ENTITY_ID);
  ReplayPacket other_pkt{};
  other_pkt.type = ReplayPacketType::AircraftSmall;
  other_pkt.timestamp_ms = 10;
  other_pkt.stream = aircraft(false);
  require(other_type.ParsePacket(other_pkt) && other_type.getPendingAircraft(1) &&
          other_type.ControlEvents.back().unit == other_type.getPendingAircraft(1),
          "aircraft sync with an existing non-aircraft UID was lost");

  // A failed record must also take back the records before it in the same packet.
  ParserState pair_units(uint32_t(0));
  pair_units.keep.ground_syncs = true;
  for (uint16_t uid : {0, 1}) {
    auto *pair_tank = new unit::Tank(&pair_units, uid);
    pair_units.ownUnit(pair_tank);
    pair_units.setUnitData(uid, pair_tank, ecs::INVALID_ENTITY_ID);
  }
  auto *pair_first = pair_units.getUnitObj(0);
  auto cut_pair = ground_pair(ground_sensors_cut());
  require(!GMSync(pair_units, cut_pair), "packet with a cut second record accepted");
  require(pair_first->positions.history().empty() && pair_units.GroundSyncEvents.empty() &&
          pair_units.SensorEvents.empty() && pair_units.GroundWeapons.empty(),
          "cut second record left the first record published");
  auto good_pair = ground_pair(ground());
  require(GMSync(pair_units, good_pair), "valid packet of two records rejected");
  require(pair_first->positions.history().size() == 1 && pair_units.GroundSyncEvents.size() == 2,
          "valid packet of two records did not commit both");

  // A missing base packet skips the state but keeps the rest of the record.
  ParserState no_base(uint32_t(0));
  auto missing = ground_missing_base();
  require(GMSync(no_base, missing), "record with a missing base packet rejected");
  require(no_base.SeekerEvents.size() == 1 && no_base.SeekerEvents[0].bits == 8 && no_base.GroundSyncEvents.empty(),
          "record with a missing base packet lost its seeker block");

  // 0xD047: the nested stream's bit length takes one byte up to 7 engines and two from 8.
  for (uint16_t engines : {1, 7, 8, 20}) {
    BitStream nested;
    for (uint16_t i = 0; i < engines; i++) nested.Write(uint16_t(1000 + i));
    mpi::RpmMessage rpm(nullptr);
    rpm.payload = nested_payload(nested);
    require(static_cast<mpi::Message &>(rpm).readPayload(&state) && rpm.rpm.rpm.size() == engines &&
            rpm.rpm.rpm.front() == 1000 && rpm.rpm.rpm.back() == 1000 + engines - 1,
            "rpm values");
  }
  BitStream odd_rpm;
  odd_rpm.Write(uint16_t(1000)); odd_rpm.Write(true);
  mpi::RpmMessage odd(nullptr);
  odd.payload = nested_payload(odd_rpm);
  require(!static_cast<mpi::Message &>(odd).readPayload(&state), "rpm stream not a multiple of 16 bits");

  // 0xD04A: the prefix takes one byte for 0-2 parameters, two for 3-340 and three from 341.
  for (uint16_t count : {2, 3, 340, 341}) {
    BitStream nested;
    nested.Write(count);
    for (uint16_t i = 0; i < count; i++) { nested.Write(i); nested.Write(float(i) / 2); }
    mpi::CockpitMessage cockpit(nullptr);
    cockpit.payload = nested_payload(nested);
    require(static_cast<mpi::Message &>(cockpit).readPayload(&state) && cockpit.cockpit.params.size() == count &&
            cockpit.cockpit.params.back().first == count - 1 &&
            cockpit.cockpit.params.back().second == float(count - 1) / 2,
            "cockpit values");
  }
  BitStream short_cockpit;
  short_cockpit.Write(uint16_t(3)); short_cockpit.Write(uint16_t(0)); short_cockpit.Write(float(1));
  mpi::CockpitMessage short_msg(nullptr);
  short_msg.payload = nested_payload(short_cockpit);
  require(!static_cast<mpi::Message &>(short_msg).readPayload(&state), "cockpit count past the stream accepted");

  std::cout << "detection, ground and aircraft checks passed\n";
  if (const char *replay_path = std::getenv("WRPL_TEST_REPLAY_PATH"); replay_path && *replay_path)
    check_store_owner(replay_path);
  if (const char *replay_path = std::getenv("WRPL_TEST_EARLY_REPLAY_PATH"); replay_path && *replay_path)
    check_early_aircraft_replay(replay_path);
}
