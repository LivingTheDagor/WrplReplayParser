#include "replay/Replay.h"
#include "Replay/ReplayReader.h"
#include "libdeflate.h"

uint32_t getPacketSize(IGenReader &cb) {
  uint8_t first_byte;
  if(!cb.readInto(first_byte))
    return 0;
  if(first_byte & 0x80)
  {
    return first_byte & 0x7f;
  }
  else
  {
    uint8_t byte_count = 1;
    if((first_byte & 0x40) == 0)
    {
      byte_count = 2;
      if ((first_byte & 0x20) == 0)
      {
        byte_count = 3;
        if ((first_byte & 0x10) == 0)
        {
          byte_count = 4;
        }
      }
    }
    union {
      uint8_t payload[4];
      uint32_t raw = 0;
    };
    if(!cb.tryRead(&payload, byte_count))
    {
      return 0;
    }

    if((first_byte & 0x40) == 0)
    {
      if ((first_byte & 0x20) == 0)
      {
        if ((first_byte & 0x10) == 0)
        {
          return payload[0] + (payload[1] << 8) + (payload[2] << 16) + (payload[3] << 24);
        }
        else
        {
          return ((raw>>0x10 & 0xff) | (first_byte<<0x18) | ((raw&0xff) << 0x10) | (payload[1] << 0x8)) ^ 0x10000000;
        }
      }
      else
      {
        return (payload[1] + (first_byte << 0x10) + (payload[0] << 0x8)) ^ 0x200000;
      }
    }
    else
    {
      return ((first_byte << 8) + payload[0]) ^ 0x4000;
    }
  }
}

void readFilesFromDirectory(const fs::path& dirPath, std::vector<fs::path>& fileSet) {
  if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
    return;
  }

  for (const auto& entry : fs::directory_iterator(dirPath)) {
    if (fs::is_regular_file(entry.status())) {
      fileSet.emplace_back(fs::absolute(entry.path()));
    }
  }
}


std::string file_exists(std::string path, const std::vector<fs::path>& paths)
{
  for(auto &path_ : paths)
  {
    if(path_.filename().string() == path)
      return path_.string();
  }
  return {};
}

ServerReplayReader::ServerReplayReader(std::vector<Replay> &rdrs) {
  ZoneScoped;
  this->readers.reserve(rdrs.size());
  for(auto &rpl : rdrs)
  {
    this->readers.push_back(rpl.getFullDecompressReplayReader());
  }
}

ServerReplayReader::~ServerReplayReader() {
  for(auto &rpl : this->readers)
  {
    delete rpl;
  }
}

bool ServerReplayReader::getNextPacket(ReplayPacket *packet) {
  if(this->index >= this->readers.size())
    return false;
  auto curr_rpl = this->readers[this->index];
  if(curr_rpl->getNextPacket(packet))
    return true;
  this->index++;
  return this->getNextPacket(packet);
}


IReplayReader::~IReplayReader() = default;

FullDecompressReplayReader::FullDecompressReplayReader(std::span<uint8_t> zlib_data, double expected_multiply_size)  {
  ZoneScoped;
  size_t decomp_size = (size_t)(((double)zlib_data.size())*expected_multiply_size);
  auto ptr = (uint8_t*)malloc(decomp_size);
  size_t dest_len;
  auto ctx = libdeflate_alloc_decompressor();
  libdeflate_result ret;
  {
    ZoneScopedN("Replay uncompress")
    ret = libdeflate_zlib_decompress(ctx, zlib_data.data(), zlib_data.size(), ptr, decomp_size, &dest_len);
    //ret = uncompress(ptr, reinterpret_cast<unsigned long *>(&dest_len), zlib_data.data(), zlib_data.size());
  }
  if (ret == LIBDEFLATE_INSUFFICIENT_SPACE) { // double it
    ZoneScopedN("Replay uncompress");
    libdeflate_free_decompressor(ctx); // do I need to do this? dont know
    ctx = libdeflate_alloc_decompressor();
    decomp_size *=2;
    free(ptr);
    ptr = (uint8_t*)malloc(decomp_size);
    ret = libdeflate_zlib_decompress(ctx, zlib_data.data(), zlib_data.size(), ptr, decomp_size, &dest_len);
  }
  G_ASSERT(ret == LIBDEFLATE_SUCCESS);
  libdeflate_free_decompressor(ctx);
  crd = new BaseReader(reinterpret_cast<char *>(ptr), dest_len, true);
}

bool MemoryEfficientServerReplayReader::getNextPacket(ReplayPacket *packet) {
  if(this->curr_reader) {
    if (this->curr_reader->getNextPacket(packet)) {
      return true;
    }
    delete_curr_reader();
  }
  if(this->curr_file_index >= this->base_dir->size()) {
    return false;
  }
  setup_reader(this->curr_file_index);
  this->curr_file_index++;
  auto ret = this->curr_reader->getNextPacket(packet);
  G_ASSERT(ret);
  return ret;
}

void MemoryEfficientServerReplayReader::setup_reader(int index) {
  if(this->super_efficiency) {
    this->current_replay = new Replay(this->base_dir->operator[](index).string());
    this->curr_reader = this->current_replay->getRplReader();
  } else {
    Replay t_rpl(this->base_dir->operator[](index).string());
    this->curr_reader = t_rpl.getFullDecompressReplayReader(1.05);
  }
}

void MemoryEfficientServerReplayReader::delete_curr_reader() {
  if(this->super_efficiency) {
    delete this->curr_reader;
    delete this->current_replay;
    this->curr_reader = nullptr;
    this->current_replay = nullptr;
  } else {
    delete this->curr_reader;
    this->current_replay = nullptr;
  }
}

MemoryEfficientServerReplayReader::MemoryEfficientServerReplayReader(std::vector<fs::path> &base_dir,
                                                                     Replay *replay_0, bool memory_efficient) {
  this->super_efficiency = memory_efficient;
  this->base_dir = &base_dir;
  this->current_replay = nullptr;
  if(this->super_efficiency) {
    this->curr_reader = replay_0->getRplReader();
  } else {
    this->curr_reader = replay_0->getFullDecompressReplayReader(1.05);
  }
  this->curr_file_index = 1;
}


std::string file_exists(const std::string& path, const std::vector<fs::path> &paths) {
  for (auto &path_: paths) {
    if (path_.filename().string() == path)
      return path_.string();
  }
  return {};
}

fs::path file_exists_fs(const std::string& path, const std::vector<fs::path> &paths) {
  for (auto &path_: paths) {
    if (path_.filename().string() == path)
      return path_;
  }
  return {};
}