

#pragma once
#include "ReplayStructs.h"
#include "ioSys/dag_dataBlock.h"
#include "ioSys/dag_fileIo.h"
#include "ioSys/dag_memIo.h"
#include "ReplayReader.h"
#include "daNet/daNetTypes.h"

namespace replay {
  enum class ReplayErrors {
    NONE = 0,
    BAD_HEADER = 1, // invalid header bytes
    BAD_MAGIC = 2, // invalid magic, either bad file or wrong version
    BAD_FILE = 3, // the file existed, but had invalid contents
    MISSING_FILE = 4, // file doesn't exist
    MISSING_DIRECTORY = 5, // directory doesn't exist
    BAD_HDR_BLK = 6, // header blk failed to load
    BAD_FTR_BLK = 7, // footer blk failed to load
    END
  };
  extern std::array<const std::string_view, static_cast<std::size_t>(ReplayErrors::END)> ReplayErrorsStrings;
  std::string_view lookupError(ReplayErrors error);


  class IReplay {
  public:
    static ReplayHeader invalid_header;
    static DataBlock invalid_blk;
    virtual ~IReplay() = default;

    /// Returns the ReplayHeader object for this replay
    /// the header contains various useful information about the replay, like mission blk, map blk
    /// session id, etc ...
    /// for server replays, this returns the first replay file's header
    /// @return returns this replay's ReplayHeader or IReplay::invalid_header if invalid replay
    virtual const ReplayHeader &getHeader() = 0;

    ///  returns the replay's headerBlk for this replay
    ///  for server replays, this returns the first replay file's header blk
    /// @return returns this replay's ReplayHeader or IReplay::invalid_blk if invalid replay
    virtual const DataBlock &getHeaderBlk() = 0;
    /// returns if this replay has a footer blk.
    /// having a footer blk is defined as being a valid replay and having a footer blk in one of the replay files(s)
    /// @return true if this replay has a footer blk, false otherwise
    virtual bool hasFooterBlk() = 0;

    /// returns the footer blk of this replay.
    /// for server replays, this returns the last replay file's footer blk
    /// specifically, it descends starting from the last replay
    /// @return returns this replay's ReplayHeader or IReplay::invalid_blk if invalid replay (or not present)
    virtual const DataBlock &getFooterBlk() = 0;

    /// returns a IReplayReader object that will let you iterate over all the packets in a specified replay
    /// getReplayReader() is faster, but also less memory efficient.
    /// \return returns an IReplayReader object
    virtual std::unique_ptr<IReplayReader> getReplayReader() = 0;


    /// returns a IReplayReader object that will let you iterate over all the packets in a specified replay
    /// getCompressedReplayReader() is slower, but also more memory efficient.
    /// \return returns an IReplayReader object
    virtual std::unique_ptr<IReplayReader> getCompressedReplayReader() = 0;

    /// check if the replay is currently valid
    /// this only checks if replay was loaded correctly. Parsing is a whole different problem
    /// if this returns false, then use
    /// @return if the replay was loaded without error.
    virtual bool isValid() = 0;

    /// returns the current error code. default is ReplayErrors::NONE
    /// if an error occurs during load, you get it here
    /// use lookupError to translate an error into a string
    /// @return ReplayErrors enum value
    virtual ReplayErrors lastErrorCode() = 0;

    /// returns the string representation of the current error code. default is ReplayErrors::NONE
    /// if an error occurs during load, you get its string representation here
    /// @return std::string_view representing the last error.
    std::string_view lastError();
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
    protected:
      virtual ~IReplayData() = default;
      virtual std::span<uint8_t> getData(Replay *) { return {}; }; // also basically BeforeParse
      virtual void afterParse() {};
      virtual bool ReadInto(uint8_t *data, size_t count, size_t offs) { return false; };
      virtual int getRemainingSize(size_t from_offs) { return -1; }
      virtual const char *file_name() { return nullptr; }
      friend Replay;
    };

    // when the replay was passed in memory
    class InMemoryReplayData : public IReplayData {
    protected:
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
    protected:
      FullFileLoadCB reader;
      std::vector<uint8_t> zstd_data{};
      uint32_t ref_count = 0; // how many readers are using this data?

      explicit FileReplayData(const std::string &path) : reader(path) {}

      ~FileReplayData() override = default; // default dtor is fine

      std::span<uint8_t> getData(Replay *rpl) override;

      void afterParse() override;

      bool ReadInto(uint8_t *data, size_t count, size_t offs) override;

      int getRemainingSize(size_t from_offs) override;

      const char *file_name() override;
      friend Replay;
    };

    class ReplayDataStorage {
      static constexpr size_t StorageSize = std::max(sizeof(InMemoryReplayData), sizeof(FileReplayData));
      static constexpr size_t StorageAlign = std::max(alignof(InMemoryReplayData), alignof(FileReplayData));

      alignas(StorageAlign) unsigned char storage_[StorageSize]{};
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

    uint32_t zstd_offs = 0xFFFFFFFF;
    uint32_t zstd_size = 0;

    friend FullDecompressReplayReader;
    friend CompressedReplayReader;
    friend IReplayReader;
    friend ServerReplay;
    ReplayHeader header{};
    DataBlock header_blk{};
    DataBlock footer_blk{};
    bool is_valid = true;
    ReplayErrors last_error = ReplayErrors::NONE;

  public:
    Replay(std::span<uint8_t> data, bool owns);
    explicit Replay(const std::string &replay_path);
    ~Replay() override = default;
    const ReplayHeader &getHeader() override;
    const DataBlock &getHeaderBlk() override;
    bool hasFooterBlk() override;
    const DataBlock &getFooterBlk() override;
    std::unique_ptr<IReplayReader> getReplayReader() override;
    std::unique_ptr<IReplayReader> getCompressedReplayReader() override;
    bool isValid() override { return is_valid; }
    ReplayErrors lastErrorCode() override;
  };

  class ServerReplay final : public IReplay {
  private:
    std::vector<std::unique_ptr<Replay>> replay_files{};
    bool is_valid = true;
    ReplayErrors err_code = ReplayErrors::NONE;

    friend ServerReplayReader<true>;
    friend ServerReplayReader<false>;

  public:
    const ReplayHeader &getHeader() override {
      if (replay_files.empty())
        return IReplay::invalid_header;
      return replay_files[0]->getHeader();
    }
    const DataBlock &getHeaderBlk() override;
    bool hasFooterBlk() override;
    const DataBlock &getFooterBlk() override;

    ServerReplay(std::vector<std::span<uint8_t>> &data, bool owns);
    ~ServerReplay() override = default;
    explicit ServerReplay(const std::string &path);

    std::unique_ptr<IReplayReader> getReplayReader() override;
    std::unique_ptr<IReplayReader> getCompressedReplayReader() override;
    bool isValid() override;
    ReplayErrors lastErrorCode() override;
  };

  template<bool doExist>
  struct _optionalZstd;

  template<>
  struct _optionalZstd<true> {
    ZstdSaveCB writer;

    explicit _optionalZstd(IGenSave &save) : writer(save, 9) {}
  };

  template<>
  struct _optionalZstd<false> {
    explicit _optionalZstd(IGenSave &save) {}
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
    [[nodiscard]] size_t size() const { return _size; }
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
    _optionalZstd<streamWrite> zstd_cb{base_cb};
    uint32_t curr_time_ms = 0;


    auto &getWriter() {
      if constexpr (streamWrite) {
        return zstd_cb.writer;
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

    std::span<uint8_t> getCompressedData(std::vector<uint8_t> &storage);
    std::span<uint8_t> getData() { return {(uint8_t *) base_cb.data(), (size_t) base_cb.size()}; }

    danet::BitStream serializePacket(const ReplayPacket &pkt);

    uint32_t calculatePacketSize(const ReplayPacket &pkt, bool &write_timestamp) {
      write_timestamp = pkt.timestamp_ms != curr_time_ms;
      return pkt.stream.GetWriteOffset() + BYTES_TO_BITS(2 + (write_timestamp ? 4 : 0));
    }

  public:
    ReplayHeader header;
    DataBlock header_blk;
    DataBlock footer_blk;
    ReplayWriter(IReplay &rpl) {
      G_ASSERT(rpl.isValid());
      header = rpl.getHeader();
      header_blk = rpl.getHeaderBlk();
      footer_blk = rpl.getFooterBlk();
    }
    ~ReplayWriter() {
      if constexpr (streamWrite)
        zstd_cb.writer.finish();
    }

    void write(const void *data, size_t size, uint32_t time_ms, ReplayPacketType type) {
      ReplayPacket pkt;
      pkt.timestamp_ms = time_ms;
      pkt.type = type;
      pkt.stream = danet::BitStream{(uint8_t *) data, size, false};
      write(pkt);
    }

    void write(const ReplayPacket &pkt);

    owned_span<uint8_t> createReplay();
  };

  extern template class ReplayWriter<false>;
  extern template class ReplayWriter<true>;
} // namespace replay
