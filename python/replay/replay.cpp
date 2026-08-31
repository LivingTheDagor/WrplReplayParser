#include "modules/replay/replay.h"
#include "modules/DataBlock/DataBlock.h"
#include "modules/BitStream.h"
#include "Replay/Replay.h"
#include "memory.h"
#include "modules/bytes_to_span.h"
using ssize_t = Py_ssize_t; // msvc doesnt think
struct IReplayReaderIterInto {
  ReplayPacket *into;
  py::object py_reader_ref; // done to keep a py ref so tempory py object doesnt go out of scope during iteration
  IReplayReader *rdr;

  IReplayReaderIterInto(py::object py_reader, ReplayPacket *pkt) :
    into(pkt), py_reader_ref(std::move(py_reader)), rdr(py_reader_ref.cast<IReplayReader *>()) {}
};

template<size_t N>
std::string_view getStr(const char (&arr)[N]) {
  return std::string_view(arr, strnlen(arr, N));
}

template<bool streaming>
auto build_replay_writer(py::module_ &m, const std::string &name) {

  py::class_<ReplayWriter<streaming>>(m, name.c_str())
    .def(py::init<IReplay &>())
    .def_readwrite("header", &ReplayWriter<streaming>::header)
    .def_property(
      "headerBlk", [](ReplayWriter<streaming> &self) { return DataBlockRW(&self.header_blk); },
      [](ReplayWriter<streaming> &self, DataBlockRW &blk) { self.header_blk = *blk.ptr(); },
      py::return_value_policy::reference_internal)
    .def_property(
      "footerBlk", [](ReplayWriter<streaming> &self) { return DataBlockRW(&self.footer_blk); },
      [](ReplayWriter<streaming> &self, DataBlockRW &blk) { self.footer_blk = *blk.ptr(); },
      py::return_value_policy::reference_internal)
    .def("writePacket", [](ReplayWriter<streaming> &self, const ReplayPacket &pkt) { self.write(pkt); })
    .def("write",
         [](ReplayWriter<streaming> &self, py::bytes &data, uint32_t time_ms, ReplayPacketType type) {
           auto spn = bytes_to_span(data);
           self.write(spn.data(), spn.size(), time_ms, type);
         })
    .def("createReplay", [](ReplayWriter<streaming> &self) {
      auto o_spn = self.createReplay();
      auto s_view = std::string_view{(char *) o_spn.data(), o_spn.size()};
      // one copy, not too bad
      return py::bytes(s_view);
    });
}

template<size_t length>
void writeInto(std::string_view data, char (&buff)[length]) {
  G_STATIC_ASSERT(length > 0);
  const size_t n = std::min(data.size(), length - 1);
  std::copy_n(data.data(), n, buff);
  buff[n] = '\0';
}

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

  py::class_<ReplayHeader>(sub, "ReplayHeader")
    .def_readonly("header", &ReplayHeader::header)
    .def_readonly("magic", &ReplayHeader::magic)
    .def_property(
      "level_path", [](ReplayHeader &header) { return getStr(header.level_path); },
      [](ReplayHeader &header, std::string_view val) { writeInto(val, header.level_path); })
    .def_property(
      "mission_file", [](ReplayHeader &header) { return getStr(header.mission_file); },
      [](ReplayHeader &header, std::string_view val) { writeInto(val, header.mission_file); })
    .def_property(
      "mission_name", [](ReplayHeader &header) { return getStr(header.mission_name); },
      [](ReplayHeader &header, std::string_view val) { writeInto(val, header.mission_name); })
    .def_property(
      "environment", [](ReplayHeader &header) { return getStr(header.environment); },
      [](ReplayHeader &header, std::string_view val) { writeInto(val, header.environment); })
    .def_property(
      "weather", [](ReplayHeader &header) { return getStr(header.weather); },
      [](ReplayHeader &header, std::string_view val) { writeInto(val, header.weather); })
    .def_readonly("footer_blk_offset", &ReplayHeader::footer_blk_offset)
    .def_readwrite("difficulty_part_1", &ReplayHeader::difficulty_part_1)
    .def_readwrite("difficulty_part_2", &ReplayHeader::difficulty_part_2)
    .def_readwrite("SessionType", &ReplayHeader::SessionType)
    .def_readwrite("player_count", &ReplayHeader::player_count)
    .def_readwrite("session_id", &ReplayHeader::session_id)
    .def_readwrite("replay_part_number", &ReplayHeader::replay_part_number)
    .def_readwrite("unk1", &ReplayHeader::unk1)
    .def_readwrite("segmentLengthSec", &ReplayHeader::segmentLengthSec)
    .def_readwrite("skiesInitialRandomSeed", &ReplayHeader::skiesInitialRandomSeed)
    .def_readonly("settings_blk_size", &ReplayHeader::settings_blk_size)
    .def_readwrite("isWorldWar", &ReplayHeader::isWorldWar)
    .def_readwrite("start_time", &ReplayHeader::start_time)
    .def_readwrite("time_limit", &ReplayHeader::time_limit)
    .def_readwrite("score_limit", &ReplayHeader::score_limit)
    .def_readwrite("killLimit", &ReplayHeader::killLimit)
    .def_readwrite("gameType", &ReplayHeader::gameType)
    .def_readwrite("restoreType", &ReplayHeader::restoreType)
    .def_readwrite("playerNo", &ReplayHeader::playerNo)
    .def_readwrite("numAttempts", &ReplayHeader::numAttempts)
    .def_readwrite("isAttempts", &ReplayHeader::isAttempts)
    .def_readwrite("isLimitedAmmo", &ReplayHeader::isLimitedAmmo)
    .def_readwrite("isLimitedFuel", &ReplayHeader::isLimitedFuel)
    .def_readwrite("gameMode", &ReplayHeader::gameMode)
    .def_property(
      "chapterName", [](ReplayHeader &header) { return getStr(header.chapterName); },
      [](ReplayHeader &header, std::string_view val) { writeInto(val, header.chapterName); })
    .def_property(
      "battle_kill_streak", [](ReplayHeader &header) { return getStr(header.battle_kill_streak); },
      [](ReplayHeader &header, std::string_view val) { writeInto(val, header.battle_kill_streak); })
    .def_readwrite("snapshotPeriodSec", &ReplayHeader::snapshotPeriodSec)
    .def_readwrite("gameVersion", &ReplayHeader::gameVersion);

  py::class_<IReplayReaderIterInto>(sub, "IReplayReaderIterInto")
    .def("__iter__", [](IReplayReaderIterInto &rdr) { return rdr; })
    .def("__next__", [](IReplayReaderIterInto &rdr) {
      if (rdr.rdr->getNextPacket(*rdr.into)) {
        return py::none{};
      }
      throw py::stop_iteration();
    });

  py::class_<IReplayReader>(sub, "IReplayReader")
    .def("getNextPacket", &IReplayReader::getNextPacket)
    .def(
      "iterInto", [](py::object rdr, ReplayPacket &pkt) { return std::make_unique<IReplayReaderIterInto>(rdr, &pkt); },
      py::keep_alive<0, 2>())
    .def("__iter__", [](IReplayReader &rdr) -> IReplayReader & { return rdr; })
    .def("__next__", [](IReplayReader &rdr) {
      auto pkt = std::make_unique<ReplayPacket>();
      if (rdr.getNextPacket(*pkt.get()))
        return pkt;
      else {
        pkt.reset();
        throw py::stop_iteration();
      }
    });
  py::class_<FullDecompressReplayReader, IReplayReader>(sub, "FullDecompressReplayReader");

  py::class_<CompressedReplayReader, IReplayReader>(sub, "CompressedReplayReader");

  py::class_<ServerReplayReader<true>, IReplayReader>(sub, "CompressedServerReplayReader");

  py::class_<ServerReplayReader<false>, IReplayReader>(sub, "FullDecompressServerReplayReader");

  py::class_<IReplay>(sub, "IReplay")
    .def_property_readonly(
      "headerBlk", [](IReplay &self) { return DataBlockRO(self.getHeaderBlk()); },
      py::return_value_policy::reference_internal)
    .def_property_readonly(
      "footerBlk", [](IReplay &self) { return DataBlockRO(self.getFooterBlk()); },
      py::return_value_policy::reference_internal)
    .def("getReplayReader", &IReplay::getReplayReader, py::return_value_policy::take_ownership, py::keep_alive<0, 1>())
    .def("getCompressedReplayReader", &IReplay::getCompressedReplayReader, py::return_value_policy::take_ownership,
         py::keep_alive<0, 1>())
    .def_property_readonly("header", &IReplay::getHeader, py::return_value_policy::reference_internal)
    .def_property_readonly("isValid", &IReplay::isValid);

  py::class_<Replay, IReplay>(sub, "Replay").def(py::init<const std::string &>());

  py::class_<ServerReplay, IReplay>(sub, "ServerReplay").def(py::init<const std::string &>());

  build_replay_writer<false>(sub, "ReplayWriter");
  build_replay_writer<true>(sub, "StreamingReplayWriter");
}

PyReplay py_replay{};
