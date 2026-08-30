#include "FileSystem.h"
#include "init/initialize.h"

#include "Replay/Replay.h"
#include "Logger.h"

int main() {
  g_log_handler.initialize();
  DynamicMemGeneralSaveCB wcb;
  wcb.resize(0x100);
  InPlaceMemLoadCB rcb{(char *) wcb.data(), (int) 0x100};
  for (int i = 1; i < BITS_TO_BYTES(0xFFFFFFFF); i++) {
    if (i == 134217728) {
      std::cout << "";
    }
    writePacketSize(wcb, BYTES_TO_BITS(i));
    auto sz = getPacketSize(rcb);
    G_ASSERTF(sz == i, "size mismatch: {} != {}", sz, i);
    wcb.seekto(0);
    rcb.seekto(0);
  }
#ifdef _TARGET_PC_LINUX
  Replay t_rpl{"/mnt/d/SteamLibrary/steamapps/common/War Thunder/Replays/#2026.08.28 20.05.13.wrpl"};
#else
  Replay t_rpl{"D:\\SteamLibrary\\steamapps\\common\\War Thunder\\Replays\\#2026.08.28 20.05.13.wrpl"};
#endif
  ReplayWriter<true> writer{t_rpl};
  std::string message = {"HALP"};
  // uint8_t *str_ptr = (uint8_t *) message.data();
  // writer.write(str_ptr, message.size(), 1, ReplayPacketType::Chat);
  // writer.write(str_ptr, message.size(), 1, ReplayPacketType::Chat);
  // writer.write(str_ptr, message.size(), 12345, ReplayPacketType::Chat);
  // writer.write(str_ptr, message.size(), 0xFFFFFF, ReplayPacketType::Chat);
  // writer.write(str_ptr, message.size(), 0xFFFFFFFF, ReplayPacketType::Chat);
  // writer.write(str_ptr, message.size(), 0xFFFFFFFF, ReplayPacketType::Chat);
  auto rdr = t_rpl.getReplayReader();
  ReplayPacket pkt{};
  while (rdr->getNextPacket(pkt)) {
    writer.write(pkt);
  }
  std::ofstream outFile("test_replay_output.wrpl", std::ios::binary);
  G_ASSERT(outFile.is_open());
  auto spn = writer.createReplay();
  outFile.write((const char *) spn.data(), spn.size());
  outFile.flush();
}
