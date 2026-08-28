

#pragma once
#include "ReplayStructs.h"
#include "ioSys/dag_dataBlock.h"
#include "ioSys/dag_fileIo.h"
#include "ioSys/dag_memIo.h"
#include "ReplayReader.h"


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