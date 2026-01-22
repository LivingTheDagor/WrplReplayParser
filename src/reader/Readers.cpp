
#include <cstdlib>
#include <stdexcept>
#include "reader.h"
#include "zstd.h"
#include <cstring>
#include <span>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>
#include "utils.h"

bool DO_VERBOSE = false;
namespace fs = std::filesystem;


BaseReader::BaseReader(char * data, int size, bool owns)
{
    this->data = data;
    this->data_size = size;
    this->read_offset = 0;
    this->owns = owns;
}

BaseReader::~BaseReader()
{
    if (owns)
        free(data);
    data = nullptr;
    data_size = 0;
    read_offset = 0;
}

bool BaseReader::read(void *ptr, int size)
{
    if(read_offset + (int)size > data_size) return false;
    memcpy(ptr, data + read_offset, size);
    read_offset+= (int)size;
    return true;
}

int BaseReader::readOffset()
{
    return this->read_offset;
}

int BaseReader::getSize()
{
    return this->data_size;
}

bool BaseReader::seekto(int addr)
{
    if (addr >= this->data_size) return false;
    read_offset = addr;
    return true;
}

bool BaseReader::seekrel(int addr)
{

    if (read_offset+addr >= this->data_size || read_offset+addr < 0) return false;
    read_offset+=addr;
    return true;
}

bool ZstdReader::initDecoder(std::span<char> &enc_data, const ZSTD_DDict_s *dict) {
    encDataBuf = enc_data;
    encDataPos = 0;
    ZSTD_DStream *strm = ZSTD_createDStream_advanced(ZSTD_defaultCMem);
    ZSTD_initDStream(strm);
    //G_VERIFY(ZSTD_initDStream(strm) != 0);
    if (dict)
        ZSTD_DCtx_refDDict(strm, dict);
    dstrm = strm;
    return true;
}

void ZstdReader::termDecoder()
{
    if (dstrm)
        ZSTD_freeDStream(dstrm);
    dstrm = nullptr;
}

inline int ZstdReader::tryReadImpl(void *ptr, int size)
{
    if (!size)
        return 0;

    assert(dstrm);
    if (encDataPos >= encDataBuf.size())
        if (!supplyMoreData())
            return 0;

    ZSTD_inBuffer inBuf;
    inBuf.src = encDataBuf.data();
    inBuf.size = encDataBuf.size();
    inBuf.pos = encDataPos;
    //std::ofstream outFile("data_test.bin", std::ios::binary);
    //outFile.write(encDataBuf.data(), encDataBuf.size());
    //outFile.close();

    ZSTD_outBuffer outBuf;
    outBuf.dst = ptr;
    outBuf.size = size;
    outBuf.pos = 0;

    while (outBuf.pos < outBuf.size) {
        size_t ret = ZSTD_decompressStream(dstrm, &outBuf, &inBuf);
        if (ZSTD_isError(ret)) return 0;

        if (ret == 0)
            break;

        if (outBuf.pos > 0) break; // got at least 1 byte

        if (inBuf.pos == inBuf.size) {
            if (!supplyMoreData())
            {
                if(outBuf.pos == 0)
                    continue;
                break;
            }
            inBuf.src = encDataBuf.data();
            inBuf.size = encDataBuf.size();
            inBuf.pos = encDataPos;
        }
    }

    encDataPos = inBuf.pos;
    return (int)outBuf.pos;
}

int ZstdReader::tryRead(void *ptr, int size) { return ZstdReader::tryReadImpl(ptr, size); }
bool ZstdReader::read(void *ptr, int size)
{
    unsigned rd_sz = ZstdReader::tryReadImpl(ptr, (int)size);
    while (rd_sz && rd_sz < size)
    {
        ptr = (char *)ptr + rd_sz;
        size -= rd_sz;
        rd_sz = ZstdReader::tryReadImpl(ptr, (int)size);
    }

    if (rd_sz != size)
    {
        termDecoder();
        EXCEPTION("Zstd read error: rd_sz={} != size={}, encDataBuf={},{} encDataPos={}", rd_sz, size, encDataBuf.data(),  encDataBuf.size(), encDataPos);
    }
    return true;
}
bool ZstdReader::seekrel(int ofs)
{
    if (ofs < 0)
        issueFatal();

    else
        while (ofs > 0)
        {
            char buf[4096];
            int sz = ofs > sizeof(buf) ? sizeof(buf) : ofs;
            read(buf, sz);
            ofs -= sz;
        }
    return true;
}

int ZstdReader::getSize() {
    this->issueFatal();
    return 0;
}

void ZstdLoadCB::open(GenReader &in_crd, int in_size, const ZSTD_DDict_s *dict)
{
    assert(!loadCb && "already opened?");
    assert(in_size > 0);
    loadCb = &in_crd;
    inBufLeft = in_size;
    auto x = std::span<char>(rdBuf, 0);
    initDecoder(x, dict);
}

void ZstdLoadCB::ceaseReading()
{
    if (!dstrm)
        return;

    loadCb->seekrel(int(inBufLeft > 0x70000000 ? encDataPos - encDataBuf.size() : inBufLeft));
    termDecoder();
    return;
}

void ZstdLoadCB::close()
{
    //if (dstrm && !inBufLeft && encDataPos >= encDataBuf.size())
    ceaseReading();

    assert(!dstrm);
    loadCb = nullptr;
    inBufLeft = 0;
}
bool ZstdLoadCB::supplyMoreData()
{
    if (loadCb && inBufLeft > 0)
    {
        encDataPos = 0;
        int sz = int(inBufLeft > RD_BUFFER_SIZE ? RD_BUFFER_SIZE : inBufLeft);
        encDataBuf = std::span<char>(rdBuf, loadCb->tryRead(rdBuf, sz));
        inBufLeft -= encDataBuf.size();
    }
    return encDataPos < encDataBuf.size();
}

std::streamoff FileReader::getFileSize()
{
  std::streamoff old = input.tellg();
  input.seekg(0, std::ios::end);
  std::streamoff payload = input.tellg();
  input.seekg(old);
  return payload;
}

FileReader::FileReader(const std::string &file_name)
{
    this->fName = file_name;
    this->input = std::ifstream(file_name, std::ios::binary);
    if(this->input.is_open())
    {
      data_size = getFileSize();
    }
    else
    {
        data_size = -1;
    }
}

FileReader::~FileReader()
{
    input.close();
}

bool FileReader::isValid() const
{
    return data_size != -1;
}

bool FileReader::read(void *ptr, int size)
{
    if (!isValid())
        return false;
    input.read((char *)ptr, size);
    return true;
}

int FileReader::readOffset()
{
    return (int)input.tellg();
}

int FileReader::getSize() {
    return (int)data_size;
}


bool FileReader::seekto(int offset)
{
    if (offset > data_size)
    {
        return false;
    }
    input.seekg(offset);
    return true;
}

bool FileReader::seekrel(int offset)
{
    std::streamoff curr_off = input.tellg();
    if (offset + curr_off > data_size)
    {
        return false;
    }
    input.seekg(offset+curr_off);
    return true;
}

int FileStreamReader::tryRead(void *ptr, int size) {
  std::error_code ec;
  uintmax_t Fstart = fs::file_size(this->fName, ec);
  this->input.read((char *)ptr, size);
  std::streamsize bytesRead = input.gcount();
  //std::cout << bytesRead << "\n";
  if (bytesRead>0)
    return (int)bytesRead;
  else
  {
    auto start = std::chrono::steady_clock::now();
    auto deadline = start + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Polling interval
      uintmax_t currentSize = fs::file_size(this->fName, ec);
      if (ec) {
        //std::cerr << "Error checking file size: " << ec.message() << "\n";
        return 0;
      }
      if (currentSize > Fstart)
      {
        this->input.clear();
        this->input.read((char *)ptr, size);
        bytesRead = input.gcount();
        //std::cout << bytesRead << "\n";
        if (bytesRead>0)
          return (int)bytesRead;
        return 0; // we fucked I guess
      }
    }
  }
  return 0;
}
