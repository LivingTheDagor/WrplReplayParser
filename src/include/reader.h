#ifndef WTFILEUTILS_READER_H
#define WTFILEUTILS_READER_H

#include "string"
#include <cstdint>
#include <cassert>
#include <span>
#include <zstd.h>
#include <fstream>
#include "utils.h"
#include "zlib.h"

class GenReader {
private:
    char buff[1028]{};
protected:
    bool owns = false;
public:
    /// virtual destructor
    virtual ~GenReader() = default;

    /// reads from the stream, returns nullptr if unable
    virtual bool read(void * ptr, int size) = 0;

    /// returns current position in stream
    virtual int readOffset() = 0;

    virtual int getSize() = 0;

    /// Seek to specified position.
    virtual bool seekto(int) = 0;

    /// Seek relative to current position.
    virtual bool seekrel(int) = 0;

    virtual int tryRead(void *ptr, int size) = 0;

    /// reads
    template<typename T>
    bool readInto(T &dest)
    {
        return this->tryRead((void *)&dest, sizeof(dest)) == sizeof(dest);
    }

    /// reads a c string
    bool readInto(std::string &dest) {
        char *ptr = buff - 1; // I know this is stupid, but it should work
        do {
            ptr++;
            bool out = this->read(ptr, 1);
            if (!out) return false;
        } while (*ptr != 0);

        dest = std::string(buff);
        return true;
    }
  template <typename T>
  bool readCompressedUnsignedGeneric(T &v)
  {
    v = 0;
    for (uint32_t i = 0; i < sizeof(v) + 1; ++i)
    {
      uint8_t byte = 0;
      if(!read(&byte, 1))
        return false;

      v |= static_cast<T>(static_cast<T>(byte & 0x7F) << (i * 7));

      if ((byte & 0x80) == 0)
        break;
    }
    return true;
  }

    static void issueFatal()
    {
        assert(0 && "Not allowed");
    }
};

class BaseReader : public GenReader
{
public:
    BaseReader(char * data, int size, bool owns);

    ~BaseReader() override;

    bool read(void * ptr, int size) override;

    int tryRead(void * ptr, int size) override
    {
        if (size > (data_size-readOffset()))
        {
            size = data_size - readOffset();
        }
        if(!read(ptr, size)) return 0;
        return size;
    }

    int readOffset() override;

    int getSize() override;

    bool seekto(int) override;

    bool seekrel(int) override;

protected:
    char* data;
    int data_size;
    int read_offset;


};

class FileReader: public GenReader
{
public:
    explicit FileReader(const std::string &file_name);

    ~FileReader() override;

  std::streamoff getFileSize();

    bool read(void * ptr, int size) override;

    int tryRead(void * ptr, int size) override
    {
        if(!read(ptr, size)) return 0;
        return size;
    }

    int readOffset() override;

    int getSize() override;

    bool seekto(int) override;

    bool seekrel(int) override;

    bool isValid() const;
protected:
    std::string fName;
    std::ifstream input;
    std::streamoff data_size;
};

/// a FileStreamReader functions similarly to a FileReader,
///  when it reaches the end of the file, it will continuously try to read from it, expecting new data to be written
/// if no new data is written after 10 seconds, it ends the Reading
class FileStreamReader: public FileReader
{
public:
  FileStreamReader(const std::string &path) : FileReader(path) {}

  int tryRead(void *ptr, int size) override;
};



class ZstdReader : public GenReader
{
public:
    ZstdReader() = default;
    explicit ZstdReader(std::span<char> &enc_data, const ZSTD_DDict_s *dict = nullptr, bool owns = false)
    {
        initDecoder(enc_data, dict);
        this->owns = owns;
    }
    ~ZstdReader() override { termDecoder(); }

    bool initDecoder(std::span<char> &enc_data, const ZSTD_DDict_s *dict);
    void termDecoder();

    bool read(void *ptr, int size) override;
    int tryRead(void *ptr, int size) override;
    int getSize() override;
    int readOffset() override
    {
        issueFatal();
        return 0;
    }
    bool seekto(int) override { issueFatal(); return false;}

    bool seekrel(int) override;

    //const char *getTargetName() override { return nullptr; }
    //bool ceaseReading() override { return true; }

protected:
    ZSTD_DCtx_s *dstrm = nullptr;
    std::span<char> encDataBuf;
    bool owns;
    size_t encDataPos = 0;

    virtual bool supplyMoreData() { return false; }

    inline int tryReadImpl(void *ptr, int size);
};

class ZstdLoadCB : public ZstdReader
{
public:
    ZstdLoadCB() = default; //-V730   /* since rdBuf shall not be filled in ctor for performance reasons */
    ZstdLoadCB(GenReader &in_reader, int in_size, const ZSTD_DDict_s *dict = nullptr, bool tmp = false)
    {
        ZstdReader::owns = tmp;
        open(in_reader, in_size, dict);
    }
    ~ZstdLoadCB() override { close(); }


    void open(GenReader &in_crd, int in_size, const ZSTD_DDict_s *dict = nullptr);
    void close();

    void ceaseReading();

protected:
    static constexpr int RD_BUFFER_SIZE = (32 << 10);
    size_t inBufLeft = 0;
    GenReader *loadCb = nullptr;
    alignas(16) char rdBuf[RD_BUFFER_SIZE];

    bool supplyMoreData() override;
};

enum
{
    SIZE_OF_Z_STREAM = 128,
    ZLIB_LOAD_BUFFER_SIZE = (16 << 10),
};

class ZlibLoadCB : public GenReader
{
public:
    ZlibLoadCB(GenReader &in_crd, int in_size, bool raw_inflate = false, bool fatal_errors = true) :
            isFinished(false), isStarted(false), fatalErrors(fatal_errors), loadCb(nullptr)
    {
        open(in_crd, in_size, raw_inflate);
    }
    ~ZlibLoadCB() override { close(); }

    bool read(void *ptr, int size) override;

    int tryRead(void *ptr, int size) override;
    int tell()
    {
        issueFatal();
        return 0;
    }
    bool seekto(int) override { issueFatal(); return false;}
    bool seekrel(int) override;

    const char *getTargetName() { return loadCb ? "" : NULL; }

    void open(GenReader &in_crd, int in_size, bool raw_inflate = false);
    void close();

    int getSize() override
    {
        issueFatal();
        return -1;
    }

    int readOffset() override
    {
        issueFatal();
        return 0;
    }

    //! stop reading compressed data (can be issued before end of compressed data)
    //! doesn't move stream pointer to end (this can be done with wrapping block), but
    //! prevents fatal on close
    bool ceaseReading();

protected:
    bool isFinished, isStarted;
    bool rawInflate;
    bool fatalErrors;
    GenReader *loadCb;
    int inBufLeft;
    unsigned char strm[SIZE_OF_Z_STREAM]; // z_stream strm;
    unsigned char buffer[ZLIB_LOAD_BUFFER_SIZE];

    void issueFatal();

    inline int tryReadImpl(void *ptr, int size);
    static unsigned fetchInput(void *handle, void *strm);
};

#endif //WTFILEUTILS_READER_H
