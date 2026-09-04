#pragma once
#include "cstdint"
#include "string"
#include "danet/BitStream.h"

constexpr uint32_t CURR_MAGIC = 0x00018c1c; // devserver version 0x00018c0a
#pragma pack(push, 1)
struct ReplayHeader {
  uint32_t header;
  uint32_t magic; // changes each major version
  char level_path[128];
  char mission_file[260];
  char mission_name[128];
  char environment[128];
  char weather[32];
  uint32_t footer_blk_offset;
  uint64_t difficulty_part_1;
  uint64_t difficulty_part_2;
  char unk0[20];
  uint32_t SessionType;
  uint32_t player_count;
  uint64_t session_id;
  uint8_t replay_part_number;
  uint8_t unk1;
  uint16_t segmentLengthSec;
  uint32_t skiesInitialRandomSeed;
  uint16_t settings_blk_size;
  char unk3[29];
  bool isWorldWar;
  char location_name[128];
  uint32_t start_time;
  uint32_t time_limit;
  uint32_t score_limit;
  uint32_t killLimit;
  uint32_t gameType;
  uint32_t restoreType;
  int playerNo; // player number of player who recorded the replay. if its a server replay, its 0x80000000 / 2147483648
                // (lmao)
  uint32_t unk4;
  uint32_t numAttempts;
  uint32_t maxAttempts;
  bool isAttempts;
  bool isLimitedAmmo;
  bool isLimitedFuel;
  char unk5[13];
  uint32_t gameMode;
  char chapterName[128];
  char battle_kill_streak[128];
  uint16_t snapshotPeriodSec;
  uint64_t gameVersion;
};
static_assert(offsetof(ReplayHeader, weather) == 0x28c);
#pragma pack(pop)
static_assert(sizeof(ReplayHeader) == 1234);

enum class ReplayPacketType : uint16_t {
  EndMarker = 0, // last packet in a replay
  StartMarker = 1, // not fully sure what represents
  AircraftSmall = 2, // Aircraft packet
  Chat = 3, // chat message
  MPI = 4, // MPI message
  NextSegment = 5, // this is used to communicate the next server replay file, I dont use it. usually last packet
  ECS = 6, // net ECS message
  Snapshot = 7,
  ReplayHeaderInfo =
    8, // sometimes first packet written, holds the ECS message hashes for synchronization. I haven't seen this on
       // client replays in a while (they were there like a year ago?) but they have been in server replays I think
};
extern std::string packet_names[];


struct ReplayPacket {
public:
  ReplayPacketType type{};
  uint32_t timestamp_ms{};
  // (Runtime state, not file data)
  // uint64_t runtimePayloadSizeBytes{};
  BitStream stream;
};
