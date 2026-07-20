#include <danet/daNetTypes.h>
#include "Replay/Replay.h"
#include "libdeflate.h"
#include "Replay/ReplayReader.h"


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

  if (size < 0x80) {
    buff[0] = 0x80 | (size & 0x7F);
    sz = 1;
  } else if (size < 0x4000) {
    uint32_t v = 0x4000 ^ size;
    buff[0] = (v >> 8) & 0xFF;
    buff[1] = v & 0xFF;
    sz = 2;
  } else if (size < 0x200000) {
    uint32_t v = 0x200000 ^ size;
    buff[0] = (v >> 16) & 0xFF;
    buff[1] = (v >> 8) & 0xFF;
    buff[2] = v & 0xFF;
    sz = 3;
  } else if (size < 0x10000000) {
    uint32_t v = 0x10000000 ^ size;
    buff[0] = (v >> 24) & 0xFF;
    buff[1] = (v >> 16) & 0xFF;
    buff[2] = (v >> 8) & 0xFF;
    buff[3] = v & 0xFF;
    sz = 4;
  } else {
    buff[0] = 0;
    memcpy(buff + 1, &size, 4);
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
  return true;
}

FullDecompressReplayReader::FullDecompressReplayReader(Replay &replay, double expected_multiply_size) :
  IReplayReader(replay) {
  ZoneScoped;
  auto zlib_data = replay.getData();
  auto decomp_size = (size_t) (((double) zlib_data.size()) * expected_multiply_size);
  auto ptr = (uint8_t *) malloc(decomp_size);
  size_t dest_len;
  auto ctx = libdeflate_alloc_decompressor();
  libdeflate_result ret;
  {
    ZoneScopedN("Replay uncompress") ret =
      libdeflate_zlib_decompress(ctx, zlib_data.data(), zlib_data.size(), ptr, decomp_size, &dest_len);
    // ret = uncompress(ptr, reinterpret_cast<unsigned long *>(&dest_len), zlib_data.data(), zlib_data.size());
  }
  if (ret == LIBDEFLATE_INSUFFICIENT_SPACE) {
    // double it
    ZoneScopedN("Replay uncompress");
    libdeflate_free_decompressor(ctx); // do I need to do this? dont know
    ctx = libdeflate_alloc_decompressor();
    decomp_size *= 2;
    free(ptr);
    ptr = (uint8_t *) malloc(decomp_size);
    ret = libdeflate_zlib_decompress(ctx, zlib_data.data(), zlib_data.size(), ptr, decomp_size, &dest_len);
  }
  G_ASSERT(ret == LIBDEFLATE_SUCCESS);
  libdeflate_free_decompressor(ctx);

  auto new_ptr = (uint8_t *) malloc(dest_len);
  memcpy(new_ptr, ptr, dest_len);
  free(ptr);
  new (&crd) InPlaceMemLoadCB(reinterpret_cast<char *>(new_ptr), (int) dest_len);
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
  if (this->curr_reader->getNextPacket(packet))
    return true;
  if (!load_replay())
    return false;
  return this->curr_reader->getNextPacket(packet);
}

template class ServerReplayReader<false>;
template class ServerReplayReader<true>;
