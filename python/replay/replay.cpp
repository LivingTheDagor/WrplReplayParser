#include "modules/replay/replay.h"
#include "modules/DataBlock/DataBlock.h"
#include "modules/BitStream.h"
#include "Replay/Replay.h"
#include "memory.h"
using ssize_t = Py_ssize_t; // msvc doesnt think
struct IReplayReaderIterInto {
  ReplayPacket * into;
  py::object py_reader_ref; // done to keep a py ref so tempory py object doesnt go out of scope during iteration
  IReplayReader * rdr;
  IReplayReaderIterInto(py::object py_reader, ReplayPacket* pkt)
      : into(pkt), py_reader_ref(std::move(py_reader)), rdr(py_reader_ref.cast<IReplayReader*>()) {}
};

void PyReplay::include(py::module_ &m) {
  DO_INCLUDE()
  py_data_block.include(m);
  py_bitstream.include(m);
  py::module_ sub = m.def_submodule("replay", "exposes various replay file loading and manipulating apis");

  py::enum_<ReplayPacketType>(sub, "ReplayPacketTypes")
      .value("EndMarker", ReplayPacketType::EndMarker)
      .value("StartMarker", ReplayPacketType::StartMarker)
      .value("Aircraft", ReplayPacketType::AircraftSmall)
      .value("Chat", ReplayPacketType::Chat)
      .value("Mpi", ReplayPacketType::MPI)
      .value("NextSegment", ReplayPacketType::NextSegment)
      .value("ECS", ReplayPacketType::ECS)
      .value("Snapshot", ReplayPacketType::Snapshot)
      .value("ReplayHeaderInfo", ReplayPacketType::ReplayHeaderInfo);

  py::class_<ReplayPacket>(sub, "ReplayPacket")
      .def(py::init<>())
      .def_readonly("type", &ReplayPacket::type)
      .def_readonly("data", &ReplayPacket::stream)
      .def_readonly("time_ms", &ReplayPacket::timestamp_ms);

  py::class_<IReplayReaderIterInto>(sub, "IReplayReaderIterInto")
      .def("__iter__", [](IReplayReaderIterInto &rdr) {return rdr;})
      .def("__next__", [](IReplayReaderIterInto &rdr){
        if(rdr.rdr->getNextPacket(rdr.into)) {
          return py::none{};
        }
        throw py::stop_iteration();
      });

  py::class_<IReplayReader>(sub, "IReplayReader")
      .def("getNextPacket", &IReplayReader::getNextPacket)
      .def("iterInto", [](py::object rdr, ReplayPacket &pkt) {
        return std::make_unique<IReplayReaderIterInto>(rdr, &pkt);
      })
      .def("__iter__", [](IReplayReader &rdr) -> IReplayReader& {return rdr;})
      .def("__next__", [](IReplayReader &rdr) {
        auto pkt = std::make_unique<ReplayPacket>();
        if(rdr.getNextPacket(pkt.get()))
          return pkt;
        else {
          pkt.reset();
          throw py::stop_iteration();
        }
      });
  py::class_<ReplayReader, IReplayReader>(sub, "ReplayReader");

  py::class_<ServerReplayReader, IReplayReader>(sub, "ServerReplayReader");

  py::class_<FullDecompressReplayReader, IReplayReader>(sub, "FullDecompressReplayReader");

  py::class_<MemoryEfficientServerReplayReader, IReplayReader>(sub, "MemoryEfficientServerReplayReader");

  py::class_<Replay>(sub, "Replay")
      .def(py::init<const std::string &>())
      .def_static("from_bytes", [](py::bytes bytes_data) {
        char* buffer_ptr;
        ssize_t length;
        if (PYBIND11_BYTES_AS_STRING_AND_SIZE(bytes_data.ptr(), &buffer_ptr, &length)) {
          throw std::runtime_error("Unable to extract bytes contents");
        }
        return std::make_unique<Replay>(std::span((uint8_t*)buffer_ptr, length));
      })
      .def_readonly("level_bin_path", &Replay::level_bin)
      .def_readonly("level_blk_path", &Replay::level_blk)
      .def_readonly("header_blk", &Replay::HeaderBlk)
      .def_readonly("footer_blk", &Replay::FooterBlk)
      .def_readonly("player_count", &Replay::PlayerCount)
      .def("get_replay_reader", [](Replay &rpl){
        return rpl.getFullDecompressReplayReader();
        }, py::return_value_policy::take_ownership);

  py::class_<ServerReplay>(sub, "ServerReplay")
      .def(py::init<const std::string &>())
      .def("get_replay_reader", &ServerReplay::getRplReader, py::return_value_policy::take_ownership);

  py::class_<MemoryEfficientServerReplay>(sub, "MemoryEfficientServerReplay")
      .def(py::init<const std::string &>())
      .def("get_replay_reader", &MemoryEfficientServerReplay::getRplReader, py::return_value_policy::take_ownership);
}

PyReplay py_replay{};
