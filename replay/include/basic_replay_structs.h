
#pragma once
#include "danet/BitStream.h"

enum class ReplayPacketType : uint16_t {
  EndMarker = 0,
  StartMarker = 1,
  AircraftSmall = 2,
  Chat = 3,
  MPI = 4,
  NextSegment = 5,
  ECS = 6,
  Snapshot = 7,
  ReplayHeaderInfo = 8, // first packet written
};
extern std::string packet_names[];



struct ReplayPacket {
public:
  ReplayPacketType type{};
  uint32_t timestamp_ms{};
  // (Runtime state, not file data)
  //uint64_t runtimePayloadSizeBytes{};
  BitStream stream;
};
