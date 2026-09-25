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

  py::class_<ParserState>(m, "ParserState")
    .def(py::init<>())
    .def(py::init<IReplay *>())
    .def(py::init<Replay *>())
    .def(py::init<ServerReplay *>())
    .def_readonly("mgr", &ParserState::g_entity_mgr)
    .def("getUnitEid", &ParserState::getUnitEid, py::arg("uid"))
    .def("getUnitRef", &ParserState::getUnitObj, py::arg("uid"), py::keep_alive<0, 1>())
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
    .def_readonly("chat_messages", &ParserState::chatMessages)
    .def_readonly("battle_messages", &ParserState::BattleMessages)
    .def_readonly("hit_effects", &ParserState::HitEffects,
                  "0xF0E9, one record per hit including harmless ones")
    .def_readonly("hit_analysis", &ParserState::HitAnalyses,
                  "0xF15F, hit camera record; sent twice per hit and not for every hit")
    .def_readonly("hit_damage", &ParserState::HitDamages,
                  "UnitOnEffectiveHit / UnitOnEffectiveCritHit; join to hit_effects by projectile")
    .def_readonly("hit_directions", &ParserState::HitDirections,
                  "0xF144, world travel direction; join by victim and time")
    .def_readonly("hit_explosions", &ParserState::HitExplosions,
                  "0xF133, a projectile went off next to the unit; joins by projectile")
    .def_readonly("hit_outcomes", &ParserState::HitOutcomes,
                  "0xF0C2, what the hit did: penetration, ricochet, fire, damaged parts")
    .def_readonly("ammo_events", &ParserState::AmmoEvents,
                  "Rounds left in a barrel: 0xF0BD for a ground vehicle, the aircraft sync for a plane. "
                  "A step down is that many rounds fired. The aircraft one rides every update, so values "
                  "repeat; a ground vehicle only sends it on a change.")
    .def_readonly("shots", &ParserState::ShotEvents,
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
          if (func) {
            py::gil_scoped_acquire gil;
            try {
              while (cont && rdr.getNextPacket(pkt)) {
                BitSize_t start_offs = pkt.stream.GetReadOffset();
                cont = state.ParsePacket(pkt);
                pkt.stream.SetReadOffset(start_offs);
                func(&pkt);
              }
            } catch (py::error_already_set &e) {
              eptr = std::current_exception(); // catches python exception and rethrows it outside the thread
            } catch (ExceptionException &e) {
              eptr = std::current_exception();
            } catch (AssertException &e) {
              eptr = std::current_exception();
            }
          } else {
            try {
              while (rdr.getNextPacket(pkt) && state.ParsePacket(pkt))
                ;
            } catch (ExceptionException &e) {
              eptr = std::current_exception();
            } catch (AssertException &e) {
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
      py::arg("reader"), py::arg("callback") = nullptr)
    .def("collect_all_units", [](ParserState &state) { return collect_all_units(state); })
    .def("collect_all_rockets", [](ParserState &state) { return collect_all_rockets(state); })
    .def("collect_all_bombs", [](ParserState &state) { return collect_all_bombs(state); })
    .def("collect_all_payloads", [](ParserState &state) { return collect_all_payloads(state); })
    .def("collect_all_torpedoes", [](ParserState &state) { return collect_all_torpedoes(state); })
    .def("collect_all_jettisoned",
         [](ParserState &state) { return collect_all_jettisoned(state); });
}

PyReplayState py_replay_state{};
