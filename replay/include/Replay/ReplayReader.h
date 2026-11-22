#pragma once
#include "ReplayStructs.h"
#include "ioSys/dag_io.h"
#include "ioSys/dag_zlibIo.h"
#include "ioSys/dag_zstdIo.h"
#include <future>
#include <memory>
#include <span>
#include <vector>

namespace replay {
  uint32_t getPacketSize(IGenLoad &cb);

  void writePacketSize(IGenSave &cb, uint32_t size);

  class IReplay;
  class Replay;
  class ServerReplay;

  /// One packet of a decoded part: where its payload starts (behind the type and time fields), how long it
  /// is, and those two fields.
  struct PacketRef {
    uint32_t offset;
    uint32_t size;
    uint32_t timestamp_ms;
    ReplayPacketType type;
  };
  /// Makes the sidecar of a decoded part. `part` are the decoded bytes; they do not move and stay valid until
  /// the sidecar has been destroyed. Called on the thread that decodes the part.
  using PartSidecarFactory = std::unique_ptr<IPartSidecar> (*)(const uint8_t *part, std::span<const PacketRef> packets);
  /// Process wide. nullptr (the start value): parts get no sidecar.
  void set_part_sidecar_factory(PartSidecarFactory factory);

  class IReplayReader {
  protected:
    IReplay &owner;
    explicit IReplayReader(IReplay &owner);

  public:
    virtual ~IReplayReader() = default;
    virtual bool getNextPacket(ReplayPacket &packet) = 0;
  };

  // decompress all the replay data into a buffer first
  // then create non allocation packets from that
  class FullDecompressReplayReader : public IReplayReader {
    InPlaceMemLoadCB crd;
    uint32_t curr_time = 0;
    /// Every packet of the part, found once when the part is decoded (on the thread that decodes ahead for a
    /// server replay): getNextPacket only hands them out.
    std::vector<PacketRef> packets;
    size_t next_packet = 0;
    std::unique_ptr<IPartSidecar> sidecar;
    /// the next packet from the decoded bytes (what getNextPacket was before the list)
    bool readPacket(ReplayPacket &packet);
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
    /// Non-streaming reader only: the reader of part `replay_index`, being built (file read + zstd decode) on
    /// another thread while the current part is parsed. Each part is its own Replay object, so the task
    /// shares nothing with the part being read.
    std::future<std::unique_ptr<IReplayReader>> next_reader;
    bool load_replay();

  public:
    ~ServerReplayReader() override;

    explicit ServerReplayReader(ServerReplay &replay);
    bool getNextPacket(ReplayPacket &packet) override;
  };

  extern template class ServerReplayReader<false>;
  extern template class ServerReplayReader<true>;
} // namespace replay
