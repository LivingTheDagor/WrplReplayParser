#include "Replay/Replay.h"
#include <catch2/catch_test_macros.hpp>

#define GET_REPLAY_PATH(name) (RESOURCE_DIR "/replay/" #name)


void load_check(replay::IReplay &rpl) {
  CHECK(rpl.isValid());
  {
    auto rdr = rpl.getReplayReader();
    CHECK(rdr);
  }
  {
    auto rdr = rpl.getCompressedReplayReader();
    CHECK(rdr);
  }
}

TEST_CASE("loading client replay", "[replay]") {
  std::string path = GET_REPLAY_PATH(6743917735049548967_client.wrpl);
  replay::Replay rpl{path};
  load_check(rpl);
}

TEST_CASE("loading server replay", "[replay]") {
  std::string path = GET_REPLAY_PATH(6743917735049548967);
  replay::ServerReplay rpl{path};
  load_check(rpl);
}
