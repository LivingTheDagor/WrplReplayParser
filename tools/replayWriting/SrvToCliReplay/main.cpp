#include "Logger.h"
#include "Replay/Replay.h"
namespace replay {}
int main(int argc, char *argv[]) {
  g_log_handler.initialize();
  if (argc != 3) {
    LOGI("Usage: <server_replay_path> <output_replay_path>");
    return 1;
  }
  char *server_replay_path = argv[1];
  char *output_replay_path = argv[2];
  replay::ServerReplay rpl(server_replay_path);
  if (!rpl.isValid()) {
    LOGE("Invalid server replay: {}", rpl.lastError());
    return 1;
  }
  replay::ReplayPacket pkt;
  auto rdr = rpl.getReplayReader();
  auto writer = replay::ReplayWriter<false>(rpl);
  while (rdr->getNextPacket(pkt)) {
    if (pkt.type == replay::ReplayPacketType::NextSegment)
      continue;
    writer.write(pkt);
  }
  auto data = writer.createReplay();
  std::ofstream ofs(output_replay_path, std::ios::binary);
  ofs.write((const char *) data.data(), data.size());
  ofs.close();
  LOGI("Successfully repacked server replay to client replay: {}", output_replay_path);
  return 0;
}
