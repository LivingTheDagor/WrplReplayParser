#include <daNet/daNetTypes.h>
#include "Replay/Replay.h"
#include "Replay/ReplayReader.h"
#include "utils.h"
#include "dag_assert.h"
#define ZSTD_STATIC_LINKING_ONLY // ZSTD_decompressBound
#include "zstd.h"
#include "zstd_errors.h"
#include <atomic>
#if defined(__linux__)
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace replay {
  constexpr size_t MAX_REPLAY_COMPRESSION_RATIO = 64;

  uint32_t getPacketSize(IGenLoad &cb) {
    uint8_t first_byte;
    if (!cb.readInto(first_byte))
      return 0;
    if (first_byte & 0x80) {
      return first_byte & 0x7f;
    } else {
      uint8_t byte_count = 1;
      if ((first_byte & 0x40) == 0) {
        byte_count = 2;
        if ((first_byte & 0x20) == 0) {
          byte_count = 3;
          if ((first_byte & 0x10) == 0) {
            byte_count = 4;
          }
        }
      }
      union {
        uint8_t payload[4];
        uint32_t raw = 0;
      };
      if (!cb.readExact(&payload, byte_count)) {
        return 0;
      }

      if ((first_byte & 0x40) == 0) {
        if ((first_byte & 0x20) == 0) {
          if ((first_byte & 0x10) == 0) {
            return payload[0] + (payload[1] << 8) + (payload[2] << 16) + (payload[3] << 24);
          } else {
            return ((raw >> 0x10 & 0xff) | (first_byte << 0x18) | ((raw & 0xff) << 0x10) | (payload[1] << 0x8)) ^
                   0x10000000;
          }
        } else {
          return (payload[1] + (first_byte << 0x10) + (payload[0] << 0x8)) ^ 0x200000;
        }
      } else {
        return ((first_byte << 8) + payload[0]) ^ 0x4000;
      }
    }
  }

  void writePacketSize(IGenSave &cb, uint32_t size) {
    uint8_t buff[5];
    uint8_t sz = 0;

    uint32_t num_bytes = (size + 7) >> 3; // Round up to nearest byte
    auto byte0 = (uint8_t) num_bytes;

    if (size < 0x1f9) {
      buff[0] = byte0 | 0x80;
      sz = 1;
    } else if (size < 0xfff9) {
      buff[0] = ((size >> 0xb) & 0xFF) | 0x40;
      buff[1] = byte0;
      sz = 2;
    } else if (size < 0x7ffff9) {
      buff[0] = ((size >> 0x13) & 0xFF) | 0x20;
      buff[1] = (size >> 0xb) & 0xFF;
      buff[2] = byte0;
      sz = 3;
    } else if (size < 0x3ffffff9) {
      buff[0] = (((size >> 0x18) & 0xFF) >> 3) | 0x10;
      buff[1] = (size >> 0x13) & 0xFF;
      buff[2] = (size >> 0xb) & 0xFF;
      buff[3] = byte0;
      sz = 4;
    } else {
      buff[0] = 0x00;
      buff[1] = byte0;
      buff[2] = (size >> 0xb) & 0xFF;
      buff[3] = (size >> 0x13) & 0xFF;
      buff[4] = ((size >> 0x18) & 0xFF) >> 3;
      sz = 5;
    }

    cb.write(buff, sz);
  }

  IReplayReader::IReplayReader(IReplay &owner) : owner(owner) {}


  // The part decoded ahead has the whole current part to finish; the parser and its prescan threads
  // (mpi/GMSyncPrescan.h) are waited for now. Nice 10 (Linux, per thread) gives them the cores first.
  static void decode_ahead_priority() {
#if defined(__linux__)
    setpriority(PRIO_PROCESS, (id_t) syscall(SYS_gettid), 10);
#endif
  }

  static std::atomic<PartSidecarFactory> part_sidecar_factory{nullptr};
  void set_part_sidecar_factory(PartSidecarFactory factory) { part_sidecar_factory.store(factory); }

  FullDecompressReplayReader::~FullDecompressReplayReader() {
    sidecar.reset(); // it reads the decoded bytes (maybe on a thread of its own): gone before they are
    free((void *) crd.data());
  }

  bool FullDecompressReplayReader::getNextPacket(ReplayPacket &packet) {
    if (next_packet >= packets.size())
      return false;
    const PacketRef &ref = packets[next_packet++];
#if defined(__GNUC__)
    // Packet starts are not read in address order, so prefetch the one after the next.
    if (next_packet + 1 < packets.size())
      __builtin_prefetch(crd.data() + packets[next_packet + 1].offset);
#endif
    packet.stream = danet::BitStream(reinterpret_cast<const uint8_t *>(crd.data()) + ref.offset, ref.size, false);
    packet.timestamp_ms = ref.timestamp_ms;
    packet.type = ref.type;
    packet.sidecar = sidecar.get();
    return true;
  }

  bool FullDecompressReplayReader::readPacket(ReplayPacket &packet) {
    uint32_t pkt_sz = getPacketSize(crd);
    if (pkt_sz == 0)
      return false;
    packet.stream.~BitStream();
    packet.stream = danet::BitStream(crd.data() + crd.tell(), pkt_sz, false);
    if (!crd.seekrel((int) pkt_sz))
      return false;
    uint16_t type_t = 0x0;
    packet.stream.Read(type_t);
    // if two sequential packets have the same timestamp, then only the first one encodes the timestamp
    if ((type_t & 0x10) == 0) {
      packet.stream.Read(curr_time);
    } else {
      type_t ^= 0x10;
    }
    packet.timestamp_ms = curr_time;
    packet.type = (ReplayPacketType) type_t;
    uint32_t offs = BITS_TO_BYTES(packet.stream.GetReadOffset());
    packet.stream = danet::BitStream(packet.stream.GetData() + offs, pkt_sz - offs, false);
    return true;
  }

  FullDecompressReplayReader::FullDecompressReplayReader(Replay &replay, double expected_multiply_size) :
    IReplayReader(replay) {
    ZoneScoped;
    auto zstd_data = replay.getData();
    auto decomp_size = (size_t) (((double) zstd_data.size()) * expected_multiply_size);
    const size_t max_decomp_size = zstd_data.size() * MAX_REPLAY_COMPRESSION_RATIO;
    // The parts are written as streams, so the frame header has no content size and the guess above
    // is often too small; then the whole part was decoded again into a buffer twice as large.
    // ZSTD_decompressBound walks the block headers and gives a size that always fits, so the part
    // is decoded once. Pages of the buffer that are not written are never touched.
    {
      const unsigned long long bound = ZSTD_decompressBound(zstd_data.data(), zstd_data.size());
      if (bound != ZSTD_CONTENTSIZE_ERROR && bound > decomp_size)
        decomp_size = (size_t) std::min<unsigned long long>(bound, max_decomp_size);
    }
    size_t dest_len = 0;
    uint8_t *ptr = nullptr;
    ZSTD_ErrorCode zstd_err = ZSTD_error_dstSize_tooSmall;
    while (zstd_err == ZSTD_error_dstSize_tooSmall && decomp_size <= max_decomp_size) {
      ZoneScopedN("Replay uncompress");
      if (ptr)
        free(ptr);
      ptr = (uint8_t *) malloc(decomp_size);
      if (!ptr)
        break;
      auto zstd_ret = ZSTD_decompress(ptr, decomp_size, zstd_data.data(), zstd_data.size());
      if (ZSTD_isError(zstd_ret))
        zstd_err = ZSTD_getErrorCode(zstd_ret);
      else {
        dest_len = zstd_ret;
        zstd_err = ZSTD_error_no_error;
      }
      if (zstd_err != ZSTD_error_no_error && zstd_err != ZSTD_error_dstSize_tooSmall) {
        // no destructor runs for a throwing constructor
        free(ptr);
        replay.Data.afterParse();
        EXCEPTION("Failed to decompress zstd block: {}", ZSTD_getErrorName(zstd_err));
      }
      decomp_size *= 2;
    }
    // The compressed bytes are not read again: release them here.
    replay.Data.afterParse();
    if (zstd_err != ZSTD_error_no_error) {
      free(ptr);
      EXCEPTION("Failed to decompress replay {}", replay.Data.getFileName());
    }

    if (dest_len != 0) {
      if (auto shrunk = (uint8_t *) realloc(ptr, dest_len))
        ptr = shrunk;
    }
    new (&crd) InPlaceMemLoadCB(reinterpret_cast<char *>(ptr), (int) dest_len);

    // find the packets now: for a server replay this runs on the thread that decodes the part ahead
    {
      ReplayPacket scratch{};
      while (readPacket(scratch))
        packets.push_back({uint32_t(scratch.stream.GetData() - ptr), scratch.stream.GetNumberOfBytesUsed(),
                           scratch.timestamp_ms, scratch.type});
    }
    if (const PartSidecarFactory factory = part_sidecar_factory.load())
      sidecar = factory(ptr, std::span<const PacketRef>(packets));
  }


  CompressedReplayReader::CompressedReplayReader(Replay &replay, IGenLoad *base_reader, size_t in_size,
                                                 bool acquired_lock) :
    IReplayReader(replay),
    reader(*base_reader, std::abs((int) in_size), nullptr, false),
    base_reader(base_reader),
    acquired_lock(acquired_lock) {}


  bool CompressedReplayReader::getNextPacket(ReplayPacket &packet) {
    ZoneScopedN("CompressedReplayReader::getNextPacket");
    uint32_t pkt_sz = getPacketSize(reader);
    if (pkt_sz == 0)
      return false;
    packet.stream = danet::BitStream();
    packet.stream.reserveBits(BYTES_TO_BITS(pkt_sz));
    if (reader.tryRead(packet.stream.GetData(), (int) pkt_sz) != pkt_sz)
      return false;
    packet.stream.SetWriteOffset(BYTES_TO_BITS(pkt_sz));
    packet.sidecar = nullptr;
    uint16_t type_t = 0x0;
    packet.stream.Read(type_t);
    // if two packets have the same timestamp, then only the first one encodes the timestamp
    if ((type_t & 0x10) == 0) {
      packet.stream.Read(curr_time);
    } else {
      type_t ^= 0x10;
    }
    auto byte_offs = BITS_TO_BYTES(packet.stream.GetReadOffset());
    auto temp_bs = danet::BitStream(packet.stream.GetData() + byte_offs, pkt_sz - byte_offs, true);
    packet.stream = std::move(temp_bs);
    packet.timestamp_ms = curr_time;
    packet.type = (ReplayPacketType) type_t;
    return true;
  }

  CompressedReplayReader::~CompressedReplayReader() {
    G_ASSERT(dynamic_cast<Replay *>(&this->owner));
    if (this->acquired_lock)
      static_cast<Replay &>(this->owner).Data.afterParse();
    this->reader.ceaseReading();
    delete base_reader;
  }

  template<bool streaming>
  bool ServerReplayReader<streaming>::load_replay() {
    G_ASSERT(dynamic_cast<ServerReplay *>(&this->owner));
    auto &s_owner = static_cast<ServerReplay &>(this->owner);
    this->curr_reader.reset();
    if (this->replay_index >= s_owner.replay_files.size())
      return false;
    if constexpr (streaming) {
      this->curr_reader = s_owner.replay_files[replay_index]->getCompressedReplayReader();
      ++this->replay_index;
    } else {
      const size_t index = this->replay_index;
      std::future<std::unique_ptr<IReplayReader>> ahead; // this part, when it was decoded ahead
      if (this->next_reader.valid() && this->next_reader_index == index)
        ahead = std::move(this->next_reader);
      // Start decoding the next part before taking this one, so part 1 decodes while part 0 does.
      if (!this->next_reader.valid() && index + 1 < s_owner.replay_files.size()) {
        Replay *next = s_owner.replay_files[index + 1].get();
        this->next_reader = std::async(std::launch::async, [next]() {
          decode_ahead_priority();
          return next->getReplayReader();
        });
        this->next_reader_index = index + 1;
      }
      // get() rethrows what the decode threw
      this->curr_reader = ahead.valid() ? ahead.get() : s_owner.replay_files[index]->getReplayReader();
      ++this->replay_index;
    }
    return true;
  }

  template<bool streaming>
  ServerReplayReader<streaming>::~ServerReplayReader() {
    // a part still being decoded ahead must finish before the replay it reads from can go away
    if (this->next_reader.valid())
      this->next_reader.wait();
  }

  template<bool streaming>
  ServerReplayReader<streaming>::ServerReplayReader(ServerReplay &replay) : IReplayReader(replay) {
    G_ASSERT(load_replay()); // should always succeed
    // done to remove extra checks in getNextPacket
  }


  template<bool streaming>
  bool ServerReplayReader<streaming>::getNextPacket(ReplayPacket &packet) {
    ZoneScoped;
    if (this->curr_reader && this->curr_reader->getNextPacket(packet))
      return true;
    while (load_replay()) {
      if (this->curr_reader->getNextPacket(packet))
        return true;
    }
    return false;
  }

  template class ServerReplayReader<false>;
  template class ServerReplayReader<true>;
} // namespace replay
