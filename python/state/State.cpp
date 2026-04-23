#include <pybind11/functional.h>
#include "modules/State.h"
#include "state/ParserState.h"
#include "init/initialize.h"
#include "modules/mpi/codegen_objects.h"
#include "modules/replay/replay.h"
#include "Replay/Replay.h"

void
initialize_wrapper(const std::string &VromfsPath, const std::string &logfile_path, bool fonts = false, bool lang = true,
                   bool mis = true) {
  std::thread t(initialize, VromfsPath, logfile_path, fonts, lang, mis);
  t.join();
}

void PyReplayState::include(py::module_ &m) {
  DO_INCLUDE()
  py_replay_state.include(m);
  py_codegen_objects.include(m);

  m.def("initialize", &initialize_wrapper,
        py::arg("VromfsPath"),
        py::arg("logfile_path"),
        py::arg("fonts") = false,
        py::arg("lang") = true,
        py::arg("mis") = true,
        "Initialize the ReplayParser with the given VromfsPath and logfile path.");
  m.def("g_log_flush", []() {
    g_log_handler->wait_until_empty();
    g_log_handler->flush_all();
  });
  py::class_<ParserState>(m, "ParserState")
      .def(py::init<>())
      .def(py::init<Replay *>())
      .def(py::init<ServerReplay *>())
      .def(py::init<MemoryEfficientServerReplay *>())
      .def_readonly("mgr", &ParserState::g_entity_mgr)
      .def_readonly("players", &ParserState::players)
      .def_readonly("teams", &ParserState::teams)
      .def_readonly("gen_state", &ParserState::gen_state)
      .def_readonly("glob_elo", &ParserState::glob_elo)
      .def_readonly("zones", &ParserState::Zones)
      .def("LoadFromReader", [](ParserState &state,
                                IReplayReader &rdr, const std::function<void(
          ReplayPacket *)> &func) { // the temporary exists for the entire call of this function, unlike __iter__
        //auto rdr = py_reader.cast<IReplayReader*>();
        py::gil_scoped_release release;

        std::thread temp_t(
            [&]() { // this is done purely so python signal handler doesnt come into play and so my signal handler dumps stacktrace
              ReplayPacket pkt{};
              bool end = false;
              if (func) {
                py::gil_scoped_acquire gil;
                while (!end && rdr.getNextPacket(&pkt)) {
                  BitSize_t start_offs = pkt.stream.GetReadOffset();
                  end = state.ParsePacket(pkt);
                  pkt.stream.SetReadOffset(start_offs);
                  func(&pkt);
                }
              } else {
                while (!end && rdr.getNextPacket(&pkt)) {
                  end = state.ParsePacket(pkt);
                }
              }
            });
        temp_t.join(); // this is only done for debugging purposes currently, for whatever reason python catches segfaults only if they occur within the current thread
        // if its another thread, then my segfault handler will catch it and print the stacktrace
      }, py::arg("reader"), py::arg("callback") = nullptr);
}

PyReplayState py_replay_state{};