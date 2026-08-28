#include "modules/replay/replay.h"
#include "modules/DataBlock/DataBlock.h"
#include "modules/BitStream.h"
#include "Replay/Replay.h"
#include "memory.h"
using ssize_t = Py_ssize_t; // msvc doesnt think
struct IReplayReaderIterInto {
    ReplayPacket *into;
    py::object py_reader_ref; // done to keep a py ref so tempory py object doesnt go out of scope during iteration
    IReplayReader *rdr;

    IReplayReaderIterInto(py::object py_reader, ReplayPacket *pkt)
        : into(pkt), py_reader_ref(std::move(py_reader)), rdr(py_reader_ref.cast<IReplayReader *>()) {
    }
};

template<size_t N>
std::string_view getStr(const char (&arr)[N]) {
    return std::string_view(arr, strnlen(arr, N));
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
            .def_property_readonly("level_path", [](ReplayHeader &header) { return getStr(header.level_path); })
            .def_property_readonly("mission_file", [](ReplayHeader &header) { return getStr(header.mission_file); })
            .def_property_readonly("mission_name", [](ReplayHeader &header) { return getStr(header.mission_name); })
            .def_property_readonly("environment", [](ReplayHeader &header) { return getStr(header.environment); })
            .def_property_readonly("weather", [](ReplayHeader &header) { return getStr(header.weather); })
            .def_readonly("footer_blk_offset", &ReplayHeader::footer_blk_offset)
            .def_readonly("difficulty_part_1", &ReplayHeader::difficulty_part_1)
            .def_readonly("difficulty_part_2", &ReplayHeader::difficulty_part_2)
            .def_readonly("SessionType", &ReplayHeader::SessionType)
            .def_readonly("player_count", &ReplayHeader::player_count)
            .def_readonly("session_id", &ReplayHeader::session_id)
            .def_readonly("replay_part_number", &ReplayHeader::replay_part_number)
            .def_readonly("unk1", &ReplayHeader::unk1)
            .def_readonly("segmentLengthSec", &ReplayHeader::segmentLengthSec)
            .def_readonly("skiesInitialRandomSeed", &ReplayHeader::skiesInitialRandomSeed)
            .def_readonly("settings_blk_size", &ReplayHeader::settings_blk_size)
            .def_readonly("isWorldWar", &ReplayHeader::isWorldWar)
            .def_readonly("start_time", &ReplayHeader::start_time)
            .def_readonly("time_limit", &ReplayHeader::time_limit)
            .def_readonly("score_limit", &ReplayHeader::score_limit)
            .def_readonly("killLimit", &ReplayHeader::killLimit)
            .def_readonly("gameType", &ReplayHeader::gameType)
            .def_readonly("restoreType", &ReplayHeader::restoreType)
            .def_readonly("playerNo", &ReplayHeader::playerNo)
            .def_readonly("numAttempts", &ReplayHeader::numAttempts)
            .def_readonly("isAttempts", &ReplayHeader::isAttempts)
            .def_readonly("isLimitedAmmo", &ReplayHeader::isLimitedAmmo)
            .def_readonly("isLimitedFuel", &ReplayHeader::isLimitedFuel)
            .def_readonly("gameMode", &ReplayHeader::gameMode)
            .def_property_readonly("chapterName", [](ReplayHeader &header) { return getStr(header.chapterName); })
            .def_property_readonly("battle_kill_streak", [](ReplayHeader &header) {
                return getStr(header.battle_kill_streak);
            })
            .def_readonly("snapshotPeriodSec", &ReplayHeader::snapshotPeriodSec)
            .def_readonly("gameVersion", &ReplayHeader::gameVersion);

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
            .def("iterInto", [](py::object rdr, ReplayPacket &pkt) {
                return std::make_unique<IReplayReaderIterInto>(rdr, &pkt);
            }, py::keep_alive<0, 2>())
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
            .def_property_readonly("headerBlk", &IReplay::getHeaderBlk, py::return_value_policy::reference_internal)
            .def_property_readonly("footerBlk", &IReplay::getFooterBlk, py::return_value_policy::reference_internal)
            .def("getReplayReader", &IReplay::getReplayReader, py::return_value_policy::take_ownership,
                 py::keep_alive<0, 1>())
            .def("getCompressedReplayReader", &IReplay::getCompressedReplayReader,
                 py::return_value_policy::take_ownership, py::keep_alive<0, 1>())
            .def_property_readonly("header", &IReplay::getHeader, py::return_value_policy::reference_internal)
            .def_property_readonly("isValid", &IReplay::isValid);

    py::class_<Replay, IReplay>(sub, "Replay")
            .def(py::init<const std::string &>());

    py::class_<ServerReplay, IReplay>(sub, "ServerReplay")
            .def(py::init<const std::string &>());
}

PyReplay py_replay{};
