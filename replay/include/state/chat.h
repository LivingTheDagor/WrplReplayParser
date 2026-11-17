#pragma once
#include "cstdint"
#include "daNet/bitStream.h"
enum ChatType : uint8_t {
  Team = 0,
  All = 1,
  Squad = 2,
  Direct = 3, // exists, but is currently unused from what I can tell
};

struct ChatMessage {
  uint32_t time_ms;
  std::string player_name; // player name
  std::string message; // message text
  ChatType channel = All; // what channel this message was sent on
  bool is_local_message{}; // a message that only ur client sees (ex: chat spamming message)
  bool is_quick_message{}; // is a message created through the quick message menu
  std::string complaints; // this is basically useless information
  inline bool FromBS(danet::BitStream &bs);
};
