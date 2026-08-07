
#include "FileSystem.h"
#include "ioSys/dag_dataBlock.h"
#include "ioSys/dag_memIo.h"

std::span<char> HostFile::readRaw() {
  if (file_stream.is_open()) {
    auto old_offs = file_stream.tellg();
    file_stream.seekg(0);
    this->buffer.resize(this->length());
    file_stream.read((char *) this->buffer.data(), this->length());
    file_stream.seekg(old_offs);
    return this->buffer;
  }
  return {};
}

void HostFile::Save(std::ofstream *cb) {
  auto temp = readRaw();
  cb->write(temp.data(), temp.size());
}

void HostFileIndex::init() {
  std::ifstream file_stream(this->name, std::ios::binary);
  if (!file_stream.is_open()) {
    this->file_length = -1;
    return;
  }
  this->file_length = 0;
  file_stream.seekg(0, std::ios::end);
  std::streamoff payload = file_stream.tellg();
  file_stream.seekg(0);
  this->file_length = payload;
}

std::unique_ptr<File> HostFileIndex::getFile(std::shared_ptr<FileIndex> ths) { return std::make_unique<HostFile>(ths); }

int64_t HostFile::read_impl(void *ptr, size_t length) {
  if (!file_stream.is_open())
    return -1;
  if (read_offs >= f_length)
    return 0;
  file_stream.seekg(read_offs);
  file_stream.read((char *) ptr, length);
  int64_t rd = file_stream.gcount();
  if (rd > 0)
    read_offs += rd;
  return rd;
}

void HostFile::init_hf() {
  auto actual = (HostFileIndex *) this->index.get();
  if (actual->file_length != -1) {
    this->file_stream.open(actual->name, std::ios::binary);
    DG_ASSERT(this->file_stream.is_open());
    this->f_length = actual->file_length;
    this->read_offs = 0;
  }
}

bool HostFile::loadBlk(DataBlock &blk) {
  auto data = this->readRaw();
  InPlaceMemLoadCB rdr(data.data(), (int) data.size());
  if (blk.loadFromStream(rdr, nullptr))
    return true;
  return false;
}
