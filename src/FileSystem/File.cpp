
#include "FileSystem.h"
#include "DataBlock.h"

std::span<char> HostFile::readRaw()
{
    std::ifstream inputFile(this->name, std::ios::binary);
    if(inputFile.is_open())
    {
        auto last_write = fs::last_write_time(this->name);
        if(last_write != last_write_time)
        {
            buffer = std::vector<char>(std::istreambuf_iterator<char>(inputFile), {});
            last_write_time = last_write;
        }
        return {buffer};
    }
    return {(char *)nullptr, 0};
}

void HostFile::Save(std::ofstream *cb)
{
    auto temp = readRaw();
    cb->write(temp.data(), temp.size());
}

bool HostFile::loadBlk(DataBlock &blk) {
  auto data = this->readRaw();
  BaseReader rdr(data.data(), (int)data.size(), false);
  if (blk.loadFromStream(rdr, nullptr, nullptr))
    return true;
  if (blk.loadText(data))
    return true;
  return false;
}
