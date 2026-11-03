#pragma once
#include "ReplayStructs.h"
#include "ioSys/dag_io.h"
#include "ioSys/dag_zlibIo.h"
#include "ioSys/dag_zstdIo.h"

namespace replay {
  uint32_t getPacketSize(IGenLoad &cb);

  void writePacketSize(IGenSave &cb, uint32_t size);

  class IReplay;
  class Replay;
  class ServerReplay;

  class IReplayReader {
  protected:
    IReplay *owner;
    IReplayReader(Replay &owner);
    IReplayReader(ServerReplay &owner);

  public:
    virtual ~IReplayReader() {}
    virtual bool getNextPacket(ReplayPacket &packet) = 0;
  };

  // decompress all the replay data into a buffer first
  // then create non allocation packets from that
  class FullDecompressReplayReader : public IReplayReader {
    InPlaceMemLoadCB crd;
    uint32_t curr_time = 0;
    // expected_multiply_size is the expected compression ratio of the replay
    explicit FullDecompressReplayReader(Replay &replay, double expected_multiply_size = 3);
    friend Replay;

  public:
    ~FullDecompressReplayReader() override;
    bool getNextPacket(ReplayPacket &packet) override;
  };

  class CompressedReplayReader : public IReplayReader {
    ZstdLoadCB reader;
    // reads data from the base reader to stream decompress.  much more memory efficient compared to
    // FullDecompress, but much slower
    IGenLoad *base_reader;
    uint32_t curr_time = 0;
    bool acquired_lock = false;

    friend Replay;

  public:
    explicit CompressedReplayReader(Replay &replay, IGenLoad *base_reader, size_t in_size, bool acquired_lock = true);

    ~CompressedReplayReader() override;
    bool getNextPacket(ReplayPacket &packet) override;
  };

  template<bool streaming>
  class ServerReplayReader : public IReplayReader {
    uint32_t replay_index = 0;
    std::unique_ptr<IReplayReader> curr_reader;
    bool load_replay();

  public:
    ~ServerReplayReader() override = default;

    explicit ServerReplayReader(ServerReplay &replay);
    bool getNextPacket(ReplayPacket &packet) override;
  };

  extern template class ServerReplayReader<false>;
  extern template class ServerReplayReader<true>;
} // namespace replay
