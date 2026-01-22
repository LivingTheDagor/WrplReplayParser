

#ifndef MYEXTENSION_REPLAY_H
#define MYEXTENSION_REPLAY_H

#include "reader.h"
#include <filesystem>
#include "DataBlock.h"
#include "Replay/ReplayReader.h"

namespace fs = std::filesystem;

struct ReplayData
{
  unsigned char* data;
  std::size_t size;

  ReplayData()
  {
    data = nullptr;
    size = 0;
  }

  template<typename T>
  ReplayData(T* data, size_t size)
  {
    assert(sizeof(T) == 1);
    this->data = data;
    this->size = size;
  }

  ReplayData(GenReader &rdr)
  {
    this->size = rdr.getSize()-rdr.readOffset();
    this->data = (unsigned char *)malloc(this->size);
    rdr.read(data, (int)size);
  }

  [[nodiscard]] std::span<unsigned char> getData() const
  {
    return {(unsigned char *)data, size};
  }

  [[nodiscard]] std::span<unsigned char> getData(uint64_t offset) const
  {
    G_ASSERT(data);
    return {(unsigned char *)data+offset, size-offset};
  }

  [[nodiscard]] std::string_view getStr(uint64_t offset) const
  {
    return {((char *)data+offset)};
  }

  template<typename T>
  [[nodiscard]] T* getObj(uint64_t offset)
  {
    return reinterpret_cast<T*>(data + offset);
  }

  ~ReplayData()
  {
    if(data)
      free(data);
    data = nullptr;
    size = 0;
  }
};




class Replay
{
#define FOOTER_BLK_OFFSET_LOC 0x000002AC // where the integer that stores the footer blk offset is stored
#define MAIN_DATA_START 0x000004CA // where the 'replay' starts, can either be the header blk or the zlib data
  fs::path replay_path;
  ReplayData *data;
  uint32_t footerBlkOffset;
  uint32_t zlib_start = MAIN_DATA_START;
  uint32_t zlib_size = 0;

public:
  std::array<char, 4> magic;
  DataBlock HeaderBlk;
  DataBlock FooterBlk;
  int PlayerCount;


  Replay(std::string &path)
  {
    replay_path = path;
    FileReader rdr(path);
    if(!rdr.isValid())
    {
      EXCEPTION("Replay given invalid file path: {}", path.data());
    }
    data = new ReplayData(rdr);
    parse();
  }

  void parse()
  {
    auto data_span = data->getData();
    footerBlkOffset = *data->getObj<uint32_t>(FOOTER_BLK_OFFSET_LOC);
    if (*data->getObj<uint8_t>(MAIN_DATA_START) == 1) // the BLK always starts with 0x01, zlib 0x78
    {
      auto span = data->getData(MAIN_DATA_START);
      BaseReader rdr(reinterpret_cast<char *>(span.data()), (int)span.size(), false);
      HeaderBlk.loadFromStream(rdr, nullptr, nullptr);
      zlib_start += rdr.readOffset();
    }
    if (footerBlkOffset)
    {
      zlib_size = footerBlkOffset-zlib_start;
      auto span = data->getData(footerBlkOffset);
      BaseReader rdr(reinterpret_cast<char *>(span.data()), (int)span.size(), false);
      FooterBlk.loadFromStream(rdr, nullptr, nullptr);
    }
    else
    {
      zlib_size = (uint32_t)data->size-zlib_start;
    }
  }

  ReplayReader* getRplReader()
  {
    auto dat = data->getData(zlib_start);
    auto *rdr = new BaseReader(reinterpret_cast<char *>(dat.data()), zlib_size, false);
    auto *payload = new ReplayReader(new ZlibLoadCB(*rdr, zlib_size), rdr);
    return payload;
  }

  ReplayReader* getStreamReader()
  {
    auto* rdr = new FileStreamReader(this->replay_path.string());
    rdr->seekto(zlib_start);
    auto *payload = new ReplayReader(new ZlibLoadCB(*rdr, 0xFFFFFFF), rdr);
    return payload;
  }

  ~Replay()
  {

      delete data;
  }
};

void readFilesFromDirectory(const fs::path& dirPath, std::vector<fs::path>& fileSet);

std::string file_exists(std::string path, const std::vector<fs::path>& paths);

class ServerReplay {
  std::vector<Replay> replay_files;
public:
  ServerReplay(fs::path &path)
  {
    std::vector<fs::path> files;
    readFilesFromDirectory(path, files);
    std::vector<std::string> repl_paths;
    if(auto p = file_exists("0000.wrpl", files); !p.empty())
    {
      repl_paths.emplace_back(p);
    }
    for(int i = 1; i > 0; i+=2)
    {
      if(auto p = file_exists(fmt::format("{:0>4}.wrpl", i), files); !p.empty())
      {
        repl_paths.emplace_back(p);
      }
      else
      {
        break;
      }
    }
    if(!repl_paths.empty())
    {
      replay_files.reserve(repl_paths.size());
      for(auto &path_ : repl_paths)
      {
        replay_files.emplace_back(path_);
      }
    }
  }

  ServerReplayReader* getRplReader()
  {
    return new ServerReplayReader(this->replay_files);
  }

};



#endif //MYEXTENSION_REPLAY_H
