#include "modules/replay/replay.h"
#include "modules/DataBlock/DataBlock.h"
#include "modules/BitStream.h"
#include "Replay/Replay.h"
#include "memory.h"
#include "modules/bytes_to_span.h"
struct IReplayReaderIterInto {
  ReplayPacket *into;
  py::object py_reader_ref; // done to keep a py ref so tempory py object doesnt go out of scope during iteration
  replay::IReplayReader *rdr;

  IReplayReaderIterInto(py::object py_reader, ReplayPacket *pkt) :
    into(pkt), py_reader_ref(std::move(py_reader)), rdr(py_reader_ref.cast<replay::IReplayReader *>()) {}
};

template<size_t N>
std::string_view getStr(const char (&arr)[N]) {
  return std::string_view(arr, strnlen(arr, N));
}

template<bool streaming>
auto build_replay_writer(py::module_ &m, const std::string &name) {

  py::class_<replay::ReplayWriter<streaming>>(m, name.c_str())
    .def(py::init<replay::IReplay &>())
    .def_readwrite("header", &replay::ReplayWriter<streaming>::header)
    .def_property(
      "headerBlk", [](replay::ReplayWriter<streaming> &self) { return DataBlockRW(&self.header_blk); },
      [](replay::ReplayWriter<streaming> &self, const DataBlockRW &blk) { self.header_blk = *blk.ptr(); },
      py::return_value_policy::reference_internal)
    .def_property(
      "footerBlk", [](replay::ReplayWriter<streaming> &self) { return DataBlockRW(&self.footer_blk); },
      [](replay::ReplayWriter<streaming> &self, const DataBlockRW &blk) { self.footer_blk = *blk.ptr(); },
      py::return_value_policy::reference_internal)
    .def("write", [](replay::ReplayWriter<streaming> &self, const ReplayPacket &pkt) { self.write(pkt); })
    .def("write",
         [](replay::ReplayWriter<streaming> &self, const py::bytes &data, uint32_t time_ms, ReplayPacketType type) {
           auto spn = bytes_to_span(data);
           self.write(spn.data(), spn.size(), time_ms, type);
         })
    .def("createReplay", [](replay::ReplayWriter<streaming> &self) {
      auto o_spn = self.createReplay();
      const auto s_view = std::string_view{reinterpret_cast<char *>(o_spn.data()), o_spn.size()};
      // one copy, not too bad
      return py::bytes(s_view);
    });
}

template<size_t length>
void writeInto(std::string_view data, char (&buff)[length]) {
  G_STATIC_ASSERT(length > 0);
  const size_t n = std::min(data.size(), length - 1);
  std::copy_n(data.data(), n, buff);
  std::memset(buff + n, 0, length - n);
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
      "level_path", [](const ReplayHeader &header) { return getStr(header.level_path); },
      [](ReplayHeader &header, const std::string_view val) { writeInto(val, header.level_path); })
    .def_property(
      "mission_file", [](const ReplayHeader &header) { return getStr(header.mission_file); },
      [](ReplayHeader &header, const std::string_view val) { writeInto(val, header.mission_file); })
    .def_property(
      "mission_name", [](const ReplayHeader &header) { return getStr(header.mission_name); },
      [](ReplayHeader &header, const std::string_view val) { writeInto(val, header.mission_name); })
    .def_property(
      "environment", [](const ReplayHeader &header) { return getStr(header.environment); },
      [](ReplayHeader &header, const std::string_view val) { writeInto(val, header.environment); })
    .def_property(
      "weather", [](const ReplayHeader &header) { return getStr(header.weather); },
      [](ReplayHeader &header, const std::string_view val) { writeInto(val, header.weather); })
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
      "chapterName", [](const ReplayHeader &header) { return getStr(header.chapterName); },
      [](ReplayHeader &header, const std::string_view val) { writeInto(val, header.chapterName); })
    .def_property(
      "battle_kill_streak", [](const ReplayHeader &header) { return getStr(header.battle_kill_streak); },
      [](ReplayHeader &header, const std::string_view val) { writeInto(val, header.battle_kill_streak); })
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

  py::class_<replay::IReplayReader>(sub, "IReplayReader")
    .def("getNextPacket", &replay::IReplayReader::getNextPacket)
    .def(
      "iterInto",
      [](const py::object &rdr, ReplayPacket &pkt) { return std::make_unique<IReplayReaderIterInto>(rdr, &pkt); },
      py::keep_alive<0, 2>())
    .def("__iter__", [](replay::IReplayReader &rdr) -> replay::IReplayReader & { return rdr; })
    .def("__next__", [](replay::IReplayReader &rdr) {
      auto pkt = std::make_unique<ReplayPacket>();
      if (rdr.getNextPacket(*pkt.get()))
        return pkt;
      pkt.reset();
      throw py::stop_iteration();
    });
  py::class_<replay::FullDecompressReplayReader, replay::IReplayReader> _(sub, "FullDecompressReplayReader");

  py::class_<replay::CompressedReplayReader, replay::IReplayReader> __(sub, "CompressedReplayReader");

  py::class_<replay::ServerReplayReader<true>, replay::IReplayReader> ___(sub, "CompressedServerReplayReader");

  py::class_<replay::ServerReplayReader<false>, replay::IReplayReader> ____(sub, "FullDecompressServerReplayReader");

  py::class_<replay::IReplay>(sub, "IReplay")
    .def_property_readonly(
      "headerBlk", [](replay::IReplay &self) { return DataBlockRO(&self.getHeaderBlk()); },
      py::return_value_policy::reference_internal)
    .def_property_readonly(
      "footerBlk", [](replay::IReplay &self) { return DataBlockRO(&self.getFooterBlk()); },
      py::return_value_policy::reference_internal)
    .def("getReplayReader", &replay::IReplay::getReplayReader, py::return_value_policy::take_ownership,
         py::keep_alive<0, 1>())
    .def("getCompressedReplayReader", &replay::IReplay::getCompressedReplayReader,
         py::return_value_policy::take_ownership, py::keep_alive<0, 1>())
    .def_property_readonly("header", &replay::IReplay::getHeader, py::return_value_policy::reference_internal)
    .def_property_readonly("isValid", &replay::IReplay::isValid);

  py::class_<replay::Replay, replay::IReplay>(sub, "Replay").def(py::init<const std::string &>());

  py::class_<replay::ServerReplay, replay::IReplay>(sub, "ServerReplay").def(py::init<const std::string &>());

  build_replay_writer<false>(sub, "ReplayWriter");
  build_replay_writer<true>(sub, "StreamingReplayWriter");
}

PyReplay py_replay{};
