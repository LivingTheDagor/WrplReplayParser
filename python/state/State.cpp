#include <pybind11/functional.h>
#include "modules/State.h"
#include "modules/bind_readonly_vector.h"
#include "modules/bind_rewind_state.h"
#include "modules/mpi/battle_messages.h"
#include "state/ParserState.h"
#include "init/initialize.h"
#include "modules/mpi/codegen_objects.h"
#include "modules/mpi/unit.h"
#include "Replay/Replay.h"

std::vector<unit::Unit *> collect_all_units(ParserState &state);

std::vector<Rocket *> collect_all_rockets(ParserState &state);

std::vector<Bomb *> collect_all_bombs(ParserState &state);
std::vector<Payload *> collect_all_payloads(ParserState &state);
std::vector<Torpedo *> collect_all_torpedoes(ParserState &state);
std::vector<Jettisoned *> collect_all_jettisoned(ParserState &state);

// The collectors return pointers owned by the state's entity manager, so a Python object made
// from one would dangle once the state is destroyed. Cast each element with the state as its
// parent: that keeps the parser alive as long as any collected element is, whether or not the
// returned list itself is kept.
// pybind hands back an already registered wrapper as it is, without the parent policy, so the
// keep-alive is added to each element explicitly.
template<typename T>
static py::list collect_owned(py::object self, std::vector<T *> items) {
  py::list out(items.size());
  for (size_t i = 0; i < items.size(); i++) {
    py::object item = py::cast(items[i], py::return_value_policy::reference);
    py::detail::keep_alive_impl(item, self);
    PyList_SET_ITEM(out.ptr(), i, item.release().ptr());
  }
  return out;
}

// An event list as a list of copies. A copy does not dangle when a later packet reallocates the
// state's vector, but the units and messages it points at belong to the state, so each copy keeps
// the state alive too.
template<auto Member>
static py::list event_list(py::object self) {
  const auto &events = self.cast<const ParserState &>().*Member;
  py::list out(events.size());
  for (size_t i = 0; i < events.size(); i++) {
    py::object event = py::cast(events[i], py::return_value_policy::copy);
    py::detail::keep_alive_impl(event, self);
    PyList_SET_ITEM(out.ptr(), i, event.release().ptr());
  }
  return out;
}

// The bindings below hand most of the parser's event lists to Python as copies of their elements
// (see the note on the properties), which needs a copy-constructible event type. Fail at compile
// time if a future event type cannot be copied, rather than when a script first reads the list.
template<typename... Ts>
inline constexpr bool copyable_events
  = (std::is_copy_constructible_v<Ts> && ...);
static_assert(copyable_events<mpi::HitEffect, mpi::HitAnalysis, mpi::HitDamage, mpi::HitDirection, mpi::HitExplosion,
                              mpi::HitOutcome, mpi::AmmoEvent, mpi::ShotEvent, mpi::SensorEvent, mpi::DesignationEvent,
                              mpi::SeekerEvent, mpi::ControlEvent, mpi::GroundSyncEvent, mpi::StoreSyncEvent,
                              mpi::SpotEvent, mpi::SpawnEvent, mpi::CockpitEvent, mpi::RpmEvent, ChatMessage>,
              "an event list handed to Python by copy needs a copy-constructible event type");

void initialize_wrapper(const std::string &VromfsPath, const std::string &grp_path, const std::string &logfile_path,
                        bool fonts = false, bool lang = true, bool mis = true) {
  std::thread t(initialize, VromfsPath, grp_path, logfile_path, fonts, lang, mis);
  t.join();
}

void PyReplayState::include(py::module_ &m) {
  DO_INCLUDE()
  py_replay_state.include(m);
  py_codegen_objects.include(m);
  py_battle_messages.include(m);
  py_unit.include(m);

  py::class_<ObjectRewindState<MissionArea *, false>::TimeState>(m, "MissionAreaTS")
    .def_readonly("time_ms", &ObjectRewindState<MissionArea *, false>::TimeState::time_ms)
    .def_readonly("value", &ObjectRewindState<MissionArea *, false>::TimeState::data);

  bind_readonly_vector<dag::Vector<ObjectRewindState<MissionArea *, false>::TimeState>>(m, "MissionAreaTSList");

  py::class_<ObjectRewindState<MissionZone *, false>::TimeState>(m, "MissionZoneTS")
    .def_readonly("time_ms", &ObjectRewindState<MissionZone *, false>::TimeState::time_ms)
    .def_readonly("value", &ObjectRewindState<MissionZone *, false>::TimeState::data);

  bind_readonly_vector<dag::Vector<ObjectRewindState<MissionZone *, false>::TimeState>>(m, "MissionZoneTSList");

  py::class_<ObjectRewindState<MissionZone *, false, true>::TimeState>(m, "MissionZone2TS")
    .def_readonly("time_ms", &ObjectRewindState<MissionZone *, false, true>::TimeState::time_ms)
    .def_readonly("value", &ObjectRewindState<MissionZone *, false, true>::TimeState::data);

  bind_readonly_vector<dag::Vector<ObjectRewindState<MissionZone *, false, true>::TimeState>>(m, "MissionZone2TSList");

  bind_rewind_state<ObjectRewindState<MissionArea *, false>>(m, "MissionAreaHistory");

  bind_rewind_state<ObjectRewindState<MissionZone *, false>>(m, "MissionZoneHistory");

  bind_rewind_state<ObjectRewindState<MissionZone *, false, true>>(m, "MissionZoneHistory2");

  bind_readonly_vector_no_contain<dag::Vector<ObjectRewindState<MissionArea *, false>>>(m, "MissionAreaHistoryList");
  bind_readonly_vector_no_contain<dag::Vector<ObjectRewindState<MissionZone *, false>>>(m, "MissionZoneHistoryList");
  bind_readonly_vector_no_contain<dag::Vector<ObjectRewindState<MissionZone *, false, true>>>(
    m, "MissionZoneHistoryList2");

  m.def("initialize", &initialize_wrapper, py::arg("VromfsPath"), py::arg("grp_path") = "",
        py::arg("logfile_path") = "", py::arg("fonts") = false, py::arg("lang") = true, py::arg("mis") = true,
        "Initialize the ReplayParser with the given VromfsPath and logfile path.");
  m.def("g_log_flush", []() {
    g_log_handler->wait_until_empty();
    g_log_handler->flush_all();
  });

  py::enum_<ChatType>(m, "ChatTypeEnum")
    .value("Team", ChatType::Team)
    .value("All", ChatType::All)
    .value("Squad", ChatType::Squad)
    .value("Direct", ChatType::Direct);

  py::class_<ChatMessage>(m, "ChatMessage")
    .def_readonly("time_ms", &ChatMessage::time_ms)
    .def_readonly("player_name", &ChatMessage::player_name)
    .def_readonly("message", &ChatMessage::message)
    .def_readonly("channel", &ChatMessage::channel)
    .def_readonly("is_local_message", &ChatMessage::is_local_message)
    .def_readonly("is_quick_message", &ChatMessage::is_quick_message)
    .def_readonly("complaints", &ChatMessage::complaints);

  bind_readonly_vector_no_contain<std::vector<ChatMessage>>(m, "ChatMessageList");
  bind_readonly_vector_no_contain<std::vector<const mpi::IBattleMessage *>>(m, "IBattleMessageList");

  using Keep = ParserState::EventLists;
  py::class_<Keep>(m, "EventLists", "Which event lists the parse fills; set them before LoadFromReader")
    .def_readwrite("sensors", &Keep::sensors)
    .def_readwrite("designations", &Keep::designations)
    .def_readwrite("seekers", &Keep::seekers)
    .def_readwrite("controls", &Keep::controls)
    .def_readwrite("spots", &Keep::spots)
    .def_readwrite("cockpit", &Keep::cockpit)
    .def_readwrite("spawns", &Keep::spawns)
    .def_readwrite("rpms", &Keep::rpms)
    .def_readwrite("ground_syncs", &Keep::ground_syncs, "Off by default: one event per vehicle state change")
    .def_readwrite("store_syncs", &Keep::store_syncs, "Off by default: one event per store per sync");

  py::class_<ParserState>(m, "ParserState")
    .def(py::init<>())
    .def(py::init<IReplay *>())
    .def(py::init<Replay *>())
    .def(py::init<ServerReplay *>())
    .def_readonly("mgr", &ParserState::g_entity_mgr)
    .def_readonly("keep", &ParserState::keep, "Event list switches, e.g. st.keep.ground_syncs = True")
    .def("getUnitEid", &ParserState::getUnitEid, py::arg("uid"))
    .def("getUnitRef", &ParserState::getUnitObj, py::arg("uid"), py::keep_alive<0, 1>())
    .def_property_readonly("pending_aircraft", [](py::object self) {
      return collect_owned(self, self.cast<ParserState &>().getPendingAircraft());
    },
                           "Aircraft with sync data but no matching ECS creation yet. Their UID and positions "
                           "remain available if creation never arrives.")
    .def_readonly("players", &ParserState::players)
    .def_readonly("teams", &ParserState::teams)
    .def_readonly("gen_state", &ParserState::gen_state)
    .def_readonly("glob_elo", &ParserState::glob_elo)
    .def_readonly("zones", &ParserState::Zones)
    .def_readonly("areas", &ParserState::missionAreas2)
    .def_readonly("sea_level", &ParserState::sea_level,
                  "Sea level of the map, metres, out of levels/<stem>.blk of aces.vromfs. Altitude is "
                  "packed relative to it, so a ground vehicle's y sits this far below the frame "
                  "aircraft and projectiles are in. Zero when the level file does not say.")
    .def_readonly("replay_length_ms", &ParserState::replay_length_ms)
    .def_readonly("curr_time_ms", &ParserState::curr_time_ms)
    .def("rewind_to", &ParserState::rewindToMs,
         py::doc("Rewind the Parser state to any particular time in the replay. Not exact"))
    .def_readonly("current_packet_index", &ParserState::current_packet_index)
    // The parse keeps appending to the event lists below, so pybind's own list caster would hand
    // Python each element bound as a reference into the state's vector: once a later packet
    // reallocated it, a retained element would point at freed memory. event_list hands out copies
    // that keep the state alive instead.
    .def_property_readonly("chat_messages", &event_list<&ParserState::chatMessages>)
    .def_readonly("battle_messages", &ParserState::BattleMessages)
    .def_property_readonly("hit_effects", &event_list<&ParserState::HitEffects>,
                  "0xF0E9, one record per hit including harmless ones")
    .def_property_readonly("hit_analysis", &event_list<&ParserState::HitAnalyses>,
                  "0xF15F, hit camera record; sent twice per hit and not for every hit")
    .def_property_readonly("hit_damage", &event_list<&ParserState::HitDamages>,
                  "UnitOnEffectiveHit / UnitOnEffectiveCritHit; join to hit_effects by projectile")
    .def_property_readonly("hit_directions", &event_list<&ParserState::HitDirections>,
                  "0xF144, world travel direction; join by victim and time")
    .def_property_readonly("hit_explosions", &event_list<&ParserState::HitExplosions>,
                  "0xF133, a projectile went off next to the unit; joins by projectile")
    .def_property_readonly("hit_outcomes", &event_list<&ParserState::HitOutcomes>,
                  "0xF0C2, what the hit did: penetration, ricochet, fire, damaged parts")
    .def_property_readonly("ammo_events", &event_list<&ParserState::AmmoEvents>,
                  "Rounds left in a barrel: 0xF0BD for a ground vehicle, the aircraft sync for a plane. "
                  "A step down is that many rounds fired. The aircraft one rides every update, so values "
                  "repeat; a ground vehicle only sends it on a change.")
    .def_property_readonly("sensors", &event_list<&ParserState::SensorEvents>,
                  "Sensor states from the aircraft and ground vehicle syncs")
    .def_property_readonly("designations", &event_list<&ParserState::DesignationEvents>,
                  "Target designations from the aircraft and ground vehicle syncs")
    .def_property_readonly("cockpit", &event_list<&ParserState::CockpitEvents>,
                  "0xD04A, client replays only: the author's cockpit instruments as (id, value) pairs")
    .def_property_readonly("rpms", &event_list<&ParserState::RpmEvents>,
                  "0xD047: the RPM of each engine of an aircraft")
    .def_property_readonly("spots", &event_list<&ParserState::SpotEvents>,
                  "0xF037: the spotter sees the spotted unit, which gets its red mark on the spotter's screen")
    .def_property_readonly("spawns", &event_list<&ParserState::SpawnEvents>,
                  "One per unit entity the ECS creates: player vehicles, slot dummy planes, AI and support units")
    .def_property_readonly("controls", &event_list<&ParserState::ControlEvents>,
                  "The pilot's controls and the engines of each aircraft, from the flight-model syncs")
    .def_property_readonly("ground_syncs", &event_list<&ParserState::GroundSyncEvents>,
                  "0xF074: motion, drive, turret angles and weapon state of each ground vehicle, ship and AI gun; "
                  "an event only when the vehicle's state changed")
    .def(
      "ground_weapons",
      [](const ParserState &s, const mpi::GroundSyncEvent &e) {
        if (e.owner_id != s.ground_owner_id)
          throw py::value_error("ground sync event belongs to another parser state");
        if (e.weapons_begin > s.GroundWeapons.size() ||
            e.weapon_count > s.GroundWeapons.size() - e.weapons_begin)
          throw py::value_error("ground sync weapon range is out of bounds");
        std::vector<PyGroundWeapon> out(e.weapon_count);
        for (uint8_t i = 0; i < e.weapon_count; i++) {
          const auto &w = s.GroundWeapons[size_t(e.weapons_begin) + i];
          if (w.rounds_begin > s.GroundRounds.size() ||
              w.round_count > s.GroundRounds.size() - w.rounds_begin)
            throw py::value_error("ground sync ammunition range is out of bounds");
          if (!w.round_count) {
            out[i] = {w, {}};
            continue;
          }
          auto rounds = s.GroundRounds.begin() + w.rounds_begin;
          out[i] = {w, {rounds, rounds + w.round_count}};
        }
        return out;
      },
      py::arg("event"), "The weapons of one of ground_syncs, with their rounds")
    .def_property_readonly("store_syncs", &event_list<&ParserState::StoreSyncEvents>,
                  "0xF11A: server time and velocity of each missile or rocket in flight")
    .def_property_readonly("seekers", &event_list<&ParserState::SeekerEvents>,
                  "Seeker blocks of stores in flight, aircraft and ground vehicles; the known ones decoded")
    .def_property_readonly("shots", &event_list<&ParserState::ShotEvents>,
                  "0xF0B1 single shot, 0xF01B / 0xF01C trigger down and up")
    .def(
      "LoadFromReader",
      [](ParserState &state, IReplayReader &rdr, const std::function<void(ReplayPacket *)> &func) {
        // the temporary exists for the entire call of this function, unlike __iter__
        // auto rdr = py_reader.cast<IReplayReader*>();
        py::gil_scoped_release release;
        std::exception_ptr eptr;
        std::thread temp_t([&]() {
          // this is done purely so python signal handler doesn't come into play and so my signal handler dumps
          // stacktrace although, this does make exception handling more complicated (python cant handle exceptions
          // thrown from another thread
          ReplayPacket pkt{};
          bool cont = true;
          // any exception goes back to the caller: one left here would terminate the whole process, and with it
          // every other parse running in it
          if (func) {
            try {
              while (cont && rdr.getNextPacket(pkt)) {
                BitSize_t start_offs = pkt.stream.GetReadOffset();
                cont = state.ParsePacket(pkt);
                if (!cont && pkt.type != ReplayPacketType::EndMarker)
                  throw std::runtime_error(fmt::format("Failed to parse packet at {} ms", pkt.timestamp_ms));
                pkt.stream.SetReadOffset(start_offs);
                // the GIL only around the callback, so parses on other threads keep running meanwhile
                py::gil_scoped_acquire gil;
                func(&pkt);
              }
            } catch (...) {
              eptr = std::current_exception(); // python exceptions included, rethrown outside the thread
            }
          } else {
            try {
              while (rdr.getNextPacket(pkt)) {
                if (!state.ParsePacket(pkt)) {
                  if (pkt.type != ReplayPacketType::EndMarker)
                    throw std::runtime_error(fmt::format("Failed to parse packet at {} ms", pkt.timestamp_ms));
                  break;
                }
              }
            } catch (...) {
              eptr = std::current_exception();
            }
          }
        });
        temp_t.join();
        // this is only done for debugging purposes currently, for whatever reason python catches segfaults only if they
        // occur within the current thread
        if (eptr) {
          std::rethrow_exception(eptr);
        }
        // if its another thread, then my segfault handler will catch it and print the stacktrace
      },
      py::arg("reader"), py::arg("callback") = nullptr,
      "Load packets until the end marker or reader exhaustion. Raises RuntimeError on a packet decode failure; "
      "aircraft events and positions from the failed packet are discarded.")
    .def("collect_all_units", [](py::object self) {
      return collect_owned(self, collect_all_units(self.cast<ParserState &>()));
    })
    .def("collect_all_rockets", [](py::object self) {
      return collect_owned(self, collect_all_rockets(self.cast<ParserState &>()));
    })
    .def("collect_all_bombs", [](py::object self) {
      return collect_owned(self, collect_all_bombs(self.cast<ParserState &>()));
    })
    .def("collect_all_payloads", [](py::object self) {
      return collect_owned(self, collect_all_payloads(self.cast<ParserState &>()));
    })
    .def("collect_all_torpedoes", [](py::object self) {
      return collect_owned(self, collect_all_torpedoes(self.cast<ParserState &>()));
    })
    .def("collect_all_jettisoned", [](py::object self) {
      return collect_owned(self, collect_all_jettisoned(self.cast<ParserState &>()));
    });
}

PyReplayState py_replay_state{};
