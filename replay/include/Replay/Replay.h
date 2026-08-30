

#pragma once
#include "ReplayStructs.h"
#include "ioSys/dag_dataBlock.h"
#include "ioSys/dag_fileIo.h"
#include "ioSys/dag_memIo.h"
#include "ReplayReader.h"
#include "danet/daNetTypes.h"


class IReplay {
public:
  virtual ~IReplay() = default;
  virtual ReplayHeader *getHeader() = 0;
  virtual DataBlock *getHeaderBlk() = 0;
  virtual DataBlock *getFooterBlk() = 0;
  /// returns a IReplayReader object that will let you iterate over all the packets in a specified replay
  /// getReplayReader() is faster, but also less memory efficient.
  /// \return returns the object, you must delete it yourself
  virtual IReplayReader *getReplayReader() = 0;


  /// returns a IReplayReader object that will let you iterate over all the packets in a specified replay
  /// getCompressedReplayReader() is slower, but also more memory efficient.
  /// \return returns the object, you must delete it yourself
  virtual IReplayReader *getCompressedReplayReader() = 0;
  virtual bool isValid() = 0;
};


class ServerReplay;
class Replay : public IReplay {
protected:
  enum MemoryStorageType : uint8_t {
    Invalid = 0,
    Memory = 1,
    File = 2,
  };

  class IReplayData {
    virtual ~IReplayData() {}
    virtual std::span<uint8_t> getData(Replay *) { return {}; }; // also basically BeforeParse
    virtual void afterParse() {};
    virtual bool ReadInto(uint8_t *data, size_t count, size_t offs) { return false; };
    virtual int getRemainingSize(size_t from_offs) { return -1; }
    virtual const char *file_name() { return nullptr; }
    friend Replay;
  };

  // when the replay was passed in memory
  class InMemoryReplayData : public IReplayData {
    std::span<uint8_t> data{};
    bool owns{};
    InMemoryReplayData(const std::span<uint8_t> data, bool own) : data(data), owns(own) {}

    ~InMemoryReplayData() override {
      if (owns)
        free(data.data());
    }

    std::span<uint8_t> getData(Replay *rpl) override;

    void afterParse() override {};

    bool ReadInto(uint8_t *ptr, size_t count, size_t offs) override;

    int getRemainingSize(size_t from_offs) override;

    const char *file_name() override { return "<MEM>"; }
    friend Replay;
  };

  // when the replay is a location on the filesystem
  // this does optimizations to reduce memory usage
  class FileReplayData : public IReplayData {
  public:
    FullFileLoadCB reader;
    std::vector<uint8_t> zlib_data{};
    uint32_t ref_count = 0; // how many readers are using this data?

    explicit FileReplayData(const std::string &path) : reader(path) {}

    ~FileReplayData() override = default; // default dtor is fine

    std::span<uint8_t> getData(Replay *rpl) override;

    void afterParse() override;

    bool ReadInto(uint8_t *data, size_t count, size_t offs) override;

    int getRemainingSize(size_t from_offs) override;

    const char *file_name() override;
  };

  class ReplayDataStorage {
    static constexpr size_t StorageSize = std::max(sizeof(InMemoryReplayData), sizeof(FileReplayData));
    static constexpr size_t StorageAlign = std::max(alignof(InMemoryReplayData), alignof(FileReplayData));

    alignas(StorageAlign) unsigned char storage_[StorageSize];
    MemoryStorageType type_ = Invalid;

    inline IReplayData *ptr() { return (IReplayData *) &storage_; }

    inline bool valid() { return type_ != Invalid; }

  public:
    template<typename T>
    T *asType() {
      return (T *) ptr();
    }

    ReplayDataStorage() = default;

    ~ReplayDataStorage() { reset(); }

    ReplayDataStorage(const ReplayDataStorage &) = delete;
    ReplayDataStorage &operator=(const ReplayDataStorage &) = delete;

    template<typename T, typename... Args>
    void emplace(MemoryStorageType t, Args &&...args) {
      reset();
      static_assert(std::is_base_of_v<IReplayData, T>);
      static_assert(sizeof(T) <= StorageSize);
      static_assert(alignof(T) <= StorageAlign);

      new (storage_) T(std::forward<Args>(args)...);
      type_ = t;
    }

    void reset() {
      if (type_ != Invalid) {
        ptr()->~IReplayData();
        type_ = Invalid;
      }
    }


    std::span<uint8_t> getData(Replay *rpl) { return valid() ? ptr()->getData(rpl) : std::span<uint8_t>{}; }

    void afterParse() {
      if (valid())
        ptr()->afterParse();
    }

    bool ReadInto(uint8_t *data, size_t count, size_t offs) { return valid() && ptr()->ReadInto(data, count, offs); }

    template<typename T>
    bool ReadInto(T &data, size_t offs) {
      return valid() && ptr()->ReadInto((uint8_t *) &data, sizeof(T), offs);
    }

    int getRemainingSize(size_t from_offs) {
      if (!valid())
        return -1;
      return ptr()->getRemainingSize(from_offs);
    }

    const char *getFileName() {
      if (!valid())
        return "<INVALID>";
      return ptr()->file_name();
    }

    [[nodiscard]] MemoryStorageType type() const { return type_; }
  };

  ReplayDataStorage Data{};

  auto getData() { return Data.getData(this); }
  void load();

  uint32_t zlib_offs = 0xFFFFFFFF;
  uint32_t zlib_size = 0;

  friend FullDecompressReplayReader;
  friend CompressedReplayReader;
  friend IReplayReader;
  friend ServerReplay;

public:
  ReplayHeader header;
  DataBlock header_blk;
  DataBlock footer_blk;
  bool is_valid = true;

  Replay(std::span<uint8_t> data, bool owns);
  explicit Replay(const std::string &replay_path);
  ~Replay() override = default;

  ReplayHeader *getHeader() override;

  DataBlock *getHeaderBlk() override;

  DataBlock *getFooterBlk() override;

  IReplayReader *getReplayReader() override;
  IReplayReader *getCompressedReplayReader() override;
  bool isValid() override { return is_valid; }
  // IReplayReader * getStreamingReplayReader(uint32_t time_wait=10);
};

class ServerReplay final : public IReplay {

  // don't feel like making Replay movable, // TODO
  std::vector<Replay *> replay_files{};
  friend ServerReplayReader<true>;
  friend ServerReplayReader<false>;

public:
  ReplayHeader *getHeader() override {
    if (replay_files.empty())
      return nullptr;
    return &replay_files[0]->header;
  }
  DataBlock *getHeaderBlk() override {
    if (replay_files.empty())
      return nullptr;
    return &replay_files[0]->header_blk;
  }
  DataBlock *getFooterBlk() override;

  ServerReplay(std::vector<std::span<uint8_t>> &data, bool owns);
  ~ServerReplay() override {
    for (auto p: replay_files)
      delete p;
  }
  explicit ServerReplay(const std::string &path);

  IReplayReader *getReplayReader() override;
  IReplayReader *getCompressedReplayReader() override;
  bool isValid() override;
};

template<bool doExist>
struct _optionalZlib;

template<>
struct _optionalZlib<true> {
  ZlibSaveCB writer;

  _optionalZlib(IGenSave &save) : writer(save, 9) {}
};

template<>
struct _optionalZlib<false> {
  _optionalZlib(IGenSave &save) {}
};

template<typename T>
class owned_span {
  T *_data;
  size_t _size;

public:
  owned_span(T *ptr, size_t sz) : _data(ptr), _size(sz) {}
  owned_span() : _data(nullptr), _size(0) {}

  ~owned_span() {
    if (_data)
      free(_data);
  }

  T &operator[](size_t i) { return _data[i]; }
  T *begin() { return _data; }
  T *end() { return _data + _size; }
  size_t size() const { return _size; }
  T *data() const { return _data; }
  // disable copy, allow move
  owned_span &operator=(const owned_span &other) = delete;
  owned_span(const owned_span &) = delete;
  owned_span &operator=(owned_span &&other) noexcept {
    clear();
    this->_data = other._data;
    this->_size = other._size;
    other._data = nullptr;
    other._size = 0;
    return *this;
  };

  owned_span(owned_span &&other) noexcept { *this = std::move(other); }

  void assign(T *ptr, size_t sz) {
    clear();
    _data = ptr;
    _size = sz;
  }

  void clear() {
    if (_data)
      free(_data);
    _data = nullptr;
    _size = 0;
  }
};

template<bool streamWrite>
class ReplayWriter {


  DynamicMemGeneralSaveCB base_cb;
  _optionalZlib<streamWrite> zlib_cb{base_cb};
  std::vector<uint8_t> tmp_data{100};
  uint32_t curr_time_ms = 0;


  auto &getWriter() {
    if constexpr (streamWrite) {
      return zlib_cb.writer;
    } else {
      return base_cb;
    }
  }

  template<class T>
  static void writeBlkToStream(DataBlock &blk, T &cwr) {
    DataBlock blkCopy{};
    blkCopy.setFrom(&blk); // save to copy, to guarantee write only needed data (not whole namemap)
    G_ASSERT(blkCopy.saveToStream(cwr));
  }

public:
  ReplayHeader header;
  DataBlock header_blk;
  DataBlock footer_blk;
  ReplayWriter(IReplay &rpl) {
    G_ASSERT(rpl.isValid());
    header = *rpl.getHeader();
    header_blk = *rpl.getHeaderBlk();
    footer_blk = *rpl.getFooterBlk();
  }
  ~ReplayWriter() {
    if constexpr (streamWrite)
      zlib_cb.writer.finish();
  }
  std::span<uint8_t> getData() { return {(uint8_t *) base_cb.data(), (size_t) base_cb.size()}; }

  void write(const void *data, size_t size, uint32_t time_ms, ReplayPacketType type);

  void write2(const ReplayPacket &pkt);
  inline void write(const ReplayPacket &pkt) {
    auto rd_offs = BITS_TO_BYTES(pkt.stream.GetReadOffset());
    write(pkt.stream.GetData() + rd_offs, BITS_TO_BYTES(pkt.stream.GetWriteOffset()) - rd_offs, pkt.timestamp_ms,
          pkt.type);
  }

  std::span<uint8_t> getCompressedData(std::vector<uint8_t> &storage);

  owned_span<uint8_t> createReplay();
};

extern template class ReplayWriter<false>;
extern template class ReplayWriter<true>;
