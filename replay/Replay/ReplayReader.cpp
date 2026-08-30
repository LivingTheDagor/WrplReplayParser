#include <danet/daNetTypes.h>
#include "Replay/Replay.h"
#include "libdeflate.h"
#include "Replay/ReplayReader.h"
#include "utils.h"
#include "dag_assert.h"


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
  uint8_t byte0 = (uint8_t) num_bytes;

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

IReplayReader::IReplayReader(Replay &owner) { this->owner = &owner; }

IReplayReader::IReplayReader(ServerReplay &owner) { this->owner = &owner; }

FullDecompressReplayReader::~FullDecompressReplayReader() {
  free((void *) crd.data());
  ((Replay *) this->owner)->Data.afterParse();
}

bool FullDecompressReplayReader::getNextPacket(ReplayPacket &packet) {
  uint32_t pkt_sz = getPacketSize(crd);
  if (pkt_sz == 0)
    return false;
  packet.stream.~BitStream();
  packet.stream = BitStream(crd.data() + crd.tell(), pkt_sz, false);
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
  packet.stream = BitStream(packet.stream.GetData() + offs, pkt_sz - offs, false);
  return true;
}

FullDecompressReplayReader::FullDecompressReplayReader(Replay &replay, double expected_multiply_size) :
  IReplayReader(replay) {
  ZoneScoped;
  auto zlib_data = replay.getData();
  auto decomp_size = (size_t) (((double) zlib_data.size()) * expected_multiply_size);
  const size_t max_decomp_size = zlib_data.size() * MAX_REPLAY_COMPRESSION_RATIO;
  size_t dest_len = 0;
  auto ctx = libdeflate_alloc_decompressor();
  libdeflate_result ret = LIBDEFLATE_INSUFFICIENT_SPACE;
  uint8_t *ptr = nullptr;
  while (ctx && ret == LIBDEFLATE_INSUFFICIENT_SPACE && decomp_size <= max_decomp_size) {
    ZoneScopedN("Replay uncompress");
    free(ptr);
    ptr = (uint8_t *) malloc(decomp_size);
    if (!ptr)
      break;
    ret = libdeflate_zlib_decompress(ctx, zlib_data.data(), zlib_data.size(), ptr, decomp_size, &dest_len);
    decomp_size *= 2;
  }
  if (ctx)
    libdeflate_free_decompressor(ctx);
  if (ret != LIBDEFLATE_SUCCESS) {
    free(ptr);
    replay.Data.afterParse();
    EXCEPTION("Failed to decompress replay {}", replay.Data.getFileName());
  }

  if (dest_len != 0) {
    if (auto shrunk = (uint8_t *) realloc(ptr, dest_len))
      ptr = shrunk;
  }
  new (&crd) InPlaceMemLoadCB(reinterpret_cast<char *>(ptr), (int) dest_len);
}


CompressedReplayReader::CompressedReplayReader(Replay &replay, IGenLoad *base_reader, size_t in_size,
                                               bool acquired_lock) :
  IReplayReader(replay),
  reader(*base_reader, std::abs((int) in_size), false, false),
  base_reader(base_reader),
  acquired_lock(acquired_lock) {}


bool CompressedReplayReader::getNextPacket(ReplayPacket &packet) {
  ZoneScopedN("CompressedReplayReader::getNextPacket");
  uint32_t pkt_sz = getPacketSize(reader);
  if (pkt_sz == 0)
    return false;
  packet.stream = BitStream();
  packet.stream.reserveBits(BYTES_TO_BITS(pkt_sz));
  if (reader.tryRead(packet.stream.GetData(), (int) pkt_sz) != pkt_sz)
    return false;
  packet.stream.SetWriteOffset(BYTES_TO_BITS(pkt_sz));
  uint16_t type_t = 0x0;
  packet.stream.Read(type_t);
  // if two packets have the same timestamp, then only the first one encodes the timestamp
  if ((type_t & 0x10) == 0) {
    packet.stream.Read(curr_time);
  } else {
    type_t ^= 0x10;
  }
  packet.timestamp_ms = curr_time;
  packet.type = (ReplayPacketType) type_t;
  return true;
}

CompressedReplayReader::~CompressedReplayReader() {
  if (this->acquired_lock)
    ((Replay *) this->owner)->Data.afterParse();
  this->reader.ceaseReading();
  delete base_reader;
}

template<bool streaming>
bool ServerReplayReader<streaming>::load_replay() {
  delete this->curr_reader;
  this->curr_reader = nullptr;
  auto s_owner = (ServerReplay *) this->owner;

  if (this->replay_index >= s_owner->replay_files.size())
    return false;
  if constexpr (streaming) {
    this->curr_reader = s_owner->replay_files[replay_index]->getCompressedReplayReader();
  } else {
    this->curr_reader = s_owner->replay_files[replay_index]->getReplayReader();
  }
  this->replay_index++;
  return true;
}

template<bool streaming>
ServerReplayReader<streaming>::~ServerReplayReader() {
  delete this->curr_reader;
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
