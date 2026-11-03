#include "Replay/Replay.h"
#include "filesystem"
#include "dag_assert.h"
#include "daNet/daNetTypes.h"
#include "zstd.h"
#include "zstd_errors.h"

std::string packet_names[]{"End", "Start", "Aircraft", "Chat", "MPI", "NextSegment", "ECS", "Snapshot", "ECS_Msg_Sync"};

namespace fs = std::filesystem;

namespace replay {
  ReplayHeader IReplay::invalid_header{};
  DataBlock IReplay::invalid_blk{};

  std::array<const std::string_view, static_cast<std::size_t>(ReplayErrors::END)> ReplayErrorsStrings = {
    "No error",
    "Bad replay header",
    "Bad replay magic/version",
    "Bad replay file contents",
    "Replay file is missing",
    "Replay directory is missing",
    "Failed to load replay header DataBlock",
    "Failed to load replay footer DataBlock",
  };

  std::string_view lookupError(ReplayErrors error) {
    const auto idx = static_cast<size_t>(error);
    if (idx >= std::size(ReplayErrorsStrings))
      return "Unknown replay error";
    return ReplayErrorsStrings[idx];
  }
  // ...
  std::unique_ptr<IReplayReader> Replay::getReplayReader() {
    ZoneScopedN("Replay::getReplayReader");
    G_ASSERTF_RETURN(this->isValid(), {}, "Invalid Replay: {}", this->Data.getFileName());
    // the ctor of FullDecompressReplayReader is not visible to std::make_unique
    return std::unique_ptr<FullDecompressReplayReader>(new FullDecompressReplayReader(*this));
  }

  std::unique_ptr<IReplayReader> Replay::getCompressedReplayReader() {
    ZoneScopedN("Replay::getCompressedReplayReader");
    G_ASSERTF_RETURN(this->isValid(), {}, "Invalid Replay: {}", this->Data.getFileName());
    std::span<uint8_t> data;
    IBaseLoad *rdr = nullptr;
    {
      ZoneScopedN("Replay::getCompressedReplayReader::data");
      data = getData();
    }
    {
      ZoneScopedN("Replay::getCompressedReplayReader::memCB");
      rdr = new InPlaceMemLoadCB((char *) data.data(), (int) data.size());
    }
    {
      ZoneScopedN("Replay::getCompressedReplayReader::reader");
      return std::make_unique<CompressedReplayReader>(*this, rdr, data.size());
    }
  }
  ReplayErrors Replay::lastErrorCode() { return last_error; }

  std::span<uint8_t> Replay::FileReplayData::getData(Replay *rpl) {
    ZoneScopedN("FileReplayData::getData");
    this->ref_count++;
    if (!this->zstd_data.empty()) {
      return this->zstd_data;
    }
    this->reader.seekto(rpl->zstd_offs);
    this->zstd_data.resize(rpl->zstd_size);
    this->reader.read(this->zstd_data.data(), rpl->zstd_size);
    return this->zstd_data;
  }

  void Replay::FileReplayData::afterParse() {

    G_ASSERT(ref_count > 0);
    ref_count--;
    if (ref_count == 0)
    // we don't want to hold onto data for any longer than needed, lots of memory, esp for big replays
    {
      zstd_data.clear();
      zstd_data.shrink_to_fit();
    }
  }

  bool Replay::FileReplayData::ReadInto(uint8_t *data, size_t count, size_t offs) {
    this->reader.seekto((int) offs);
    return this->reader.tryRead(data, (int) count) == count;
  }

  int Replay::FileReplayData::getRemainingSize(size_t from_offs) {
    auto sz = this->reader.getTargetDataSize();
    if (from_offs >= sz)
      return -1;
    return (int) (sz - from_offs);
  }

  const char *Replay::FileReplayData::file_name() {
    if (!this->reader.fileHandle)
      return "<INVALID FILE HANDLE>";
    return this->reader.fileHandle->getIndex()->getName().c_str();
  }

  bool Replay::InMemoryReplayData::ReadInto(uint8_t *ptr, size_t count, size_t offs) {
    if (this->data.size() < count + offs)
      return false;
    memcpy(ptr, this->data.data() + offs, count);
    return true;
  }


  std::string_view IReplay::lastError() { return lookupError(this->lastErrorCode()); }
  std::span<uint8_t> Replay::InMemoryReplayData::getData(Replay *rpl) {
    if (this->data.size() < rpl->zstd_offs + rpl->zstd_size)
      return {};
    return {this->data.data() + rpl->zstd_offs, rpl->zstd_size};
  }

  int Replay::InMemoryReplayData::getRemainingSize(size_t from_offs) {
    if (from_offs >= this->data.size())
      return -1;
    return (int) (this->data.size() - from_offs);
  }


  Replay::Replay(const std::span<uint8_t> data, bool owns) {
    ZoneScopedN("Replay::Replay::InMemoryReplayData");
    this->Data.emplace<InMemoryReplayData>(Memory, data, owns);
    load();
  }

  Replay::Replay(const std::string &replay_path) {
    if (!fs::exists(replay_path)) {
      this->last_error = ReplayErrors::MISSING_FILE;
      is_valid = false;
      return;
    }
    this->Data.emplace<FileReplayData>(File, replay_path);
    load();
  }

  const ReplayHeader &Replay::getHeader() {
    if (is_valid)
      return header;
    return IReplay::invalid_header;
  }

  const DataBlock &Replay::getHeaderBlk() {
    if (is_valid)
      return header_blk;
    return IReplay::invalid_blk;
  }
  bool Replay::hasFooterBlk() { return is_valid && !footer_blk.isEmpty(); }

  const DataBlock &Replay::getFooterBlk() {
    if (is_valid)
      return footer_blk;
    return IReplay::invalid_blk;
  }

#define BAD_REPLAY(conditional, error_val) \
  {                                        \
    if (!(conditional)) {                  \
      is_valid = false;                    \
      last_error = error_val;              \
      return;                              \
    }                                      \
  }

  void Replay::load() {
    ZoneScopedN("Replay::load");
    auto file_size = this->Data.getRemainingSize(0);
    BAD_REPLAY(file_size != -1, ReplayErrors::BAD_FILE);
    BAD_REPLAY(this->Data.ReadInto(this->header, 0), ReplayErrors::BAD_FILE);
    BAD_REPLAY(this->header.header == 0x1000ace5, ReplayErrors::BAD_HEADER);
    BAD_REPLAY(this->header.magic == CURR_MAGIC, ReplayErrors::BAD_MAGIC);
    zstd_offs = sizeof(ReplayHeader) + this->header.settings_blk_size;

    if (this->header.settings_blk_size) {
      std::vector<uint8_t> header_bytes{};
      header_bytes.resize(this->header.settings_blk_size);
      BAD_REPLAY(this->Data.ReadInto(header_bytes.data(), header_bytes.size(), sizeof(ReplayHeader)),
                 ReplayErrors::BAD_FILE);
      InPlaceMemLoadCB rdr{(char *) header_bytes.data(), (int) header_bytes.size()};
      BAD_REPLAY(this->header_blk.loadFromStream(rdr, nullptr), ReplayErrors::BAD_HDR_BLK);
    }

    if (this->header.footer_blk_offset) {
      std::vector<uint8_t> footer_bytes{};
      auto remainingSize = this->Data.getRemainingSize(this->header.footer_blk_offset);

      zstd_size = header.footer_blk_offset - zstd_offs;
      BAD_REPLAY(remainingSize != -1, ReplayErrors::BAD_FILE);
      footer_bytes.resize(remainingSize);

      BAD_REPLAY(this->Data.ReadInto(footer_bytes.data(), footer_bytes.size(), this->header.footer_blk_offset),
                 ReplayErrors::BAD_FILE);
      InPlaceMemLoadCB rdr{(char *) footer_bytes.data(), (int) footer_bytes.size()};
      BAD_REPLAY(this->footer_blk.loadFromStream(rdr, nullptr), ReplayErrors::BAD_FTR_BLK);
    } else {
      zstd_size = file_size - zstd_offs;
    }
    std::array<uint8_t, ZSTD_FRAMEHEADERSIZE_MAX> hdr{};
    const size_t hdrBytes = std::min<size_t>(hdr.size(), zstd_size);
    BAD_REPLAY(this->Data.ReadInto(hdr.data(), hdrBytes, zstd_offs), ReplayErrors::BAD_FILE);

    ZSTD_frameHeader zfh{};
    size_t hret = ZSTD_getFrameHeader(&zfh, hdr.data(), hdrBytes);
    BAD_REPLAY(!ZSTD_isError(hret), ReplayErrors::BAD_FILE);
  }

  ServerReplay::ServerReplay(std::vector<std::span<uint8_t>> &data, bool owns) {
    for (auto &d: data) {
      this->replay_files.emplace_back(std::make_unique<Replay>(d, owns));
    }
  }

  const DataBlock &ServerReplay::getHeaderBlk() {
    if (replay_files.empty())
      return IReplay::invalid_blk;
    return replay_files[0]->getHeaderBlk();
  }
  bool ServerReplay::hasFooterBlk() {

    for (auto it = replay_files.rbegin(); it != replay_files.rend(); ++it) {
      auto &blk = (*it)->getFooterBlk();
      if (&blk != &IReplay::invalid_blk) {
        return true;
      }
    }
    return false;
  }
  const DataBlock &ServerReplay::getFooterBlk() {
    for (auto it = replay_files.rbegin(); it != replay_files.rend(); ++it) {
      auto &blk = (*it)->getFooterBlk();
      if (&blk != &IReplay::invalid_blk) {
        return blk;
      }
    }
    return IReplay::invalid_blk;
  }

  void readFilesFromDirectory(const fs::path &dirPath, std::vector<fs::path> &fileSet) {
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
      return;
    }

    for (const auto &entry: fs::directory_iterator(dirPath)) {
      if (fs::is_regular_file(entry.status())) {
        fileSet.emplace_back(fs::absolute(entry.path()));
      }
    }
  }

  std::string file_exists(const std::string &path, const std::vector<fs::path> &paths) {
    for (auto &path_: paths) {
      if (path_.filename().string() == path)
        return path_.string();
    }
    return {};
  }

  fs::path file_exists_fs(const std::string &path, const std::vector<fs::path> &paths) {
    for (auto &path_: paths) {
      if (path_.filename().string() == path)
        return path_;
    }
    return {};
  }

  ServerReplay::ServerReplay(const std::string &b_path) {
    const fs::path base = fs::absolute(fs::path{b_path});

    if (!fs::exists(base) || !fs::is_directory(base)) {
      err_code = ReplayErrors::MISSING_DIRECTORY;
      is_valid = false;
      return;
    }

    auto makeName = [](int i) { return fmt::format("{:0>4}.wrpl", i); };

    const fs::path first = base / makeName(0);
    if (!fs::exists(first) || !fs::is_regular_file(first)) {
      err_code = ReplayErrors::MISSING_FILE;
      is_valid = false;
      return;
    }

    replay_files.emplace_back(std::make_unique<Replay>(first.string()));

    for (int i = 1;; i += 2) {
      const fs::path part = base / makeName(i);
      if (!fs::exists(part) || !fs::is_regular_file(part)) {
        break;
      }
      replay_files.emplace_back(std::make_unique<Replay>(part.string()));
    }
  }

  std::unique_ptr<IReplayReader> ServerReplay::getReplayReader() {
    return std::make_unique<ServerReplayReader<false>>(*this);
  }

  std::unique_ptr<IReplayReader> ServerReplay::getCompressedReplayReader() {
    return std::make_unique<ServerReplayReader<true>>(*this);
  }

  bool ServerReplay::isValid() {
    if (!is_valid) {
      return false;
    }
    for (auto &rpl: replay_files) {
      if (!rpl->isValid())
        return false;
    }
    return true;
  }
  ReplayErrors ServerReplay::lastErrorCode() {
    if (err_code != ReplayErrors::NONE)
      return err_code;
    for (auto &rpl: replay_files) {
      if (auto err = rpl->lastErrorCode(); err != ReplayErrors::NONE)
        return err;
    }
    return ReplayErrors::NONE;
  }
  /*
  template<bool streamWrite>
  void ReplayWriter<streamWrite>::write(const void *data, size_t size, uint32_t time_ms, ReplayPacketType type) {
    // 2 is packet type, 4 is potential time_ms, and 5 is max size of packet size header
    tmp_data.resize(size + 2 + 4 + 5);
    ConstrainedMemSaveCB tmp_cb{tmp_data.data(), static_cast<int>(tmp_data.size())};
    uint32_t curr_pkt_size = (uint32_t) size + 2;
    auto curr_type = static_cast<uint16_t>(type);
    bool should_write_time = (time_ms != curr_time_ms);
    curr_pkt_size += should_write_time ? 4 : 0;
    writePacketSize(tmp_cb, BYTES_TO_BITS(curr_pkt_size));
    if (!should_write_time)
      curr_type |= 0x10;
    tmp_cb.writeObj(curr_type);
    if (should_write_time)
      tmp_cb.writeObj(time_ms);
    curr_time_ms = time_ms;
    tmp_cb.write(data, static_cast<int>(size));
    auto &cb = getWriter();
    cb.write(tmp_data.data(), tmp_cb.tell());
  }*/
  template<bool streamWrite>
  void ReplayWriter<streamWrite>::write(const ReplayPacket &pkt) {
    struct __attribute__((packed)) {
      uint16_t packet_type;
      uint32_t time_ms;
    } temp{};
    static_assert(sizeof(temp) == 6);
    auto &cb = getWriter();
    bool write_timestamp;
    writePacketSize(cb, this->calculatePacketSize(pkt, write_timestamp));
    temp.packet_type = static_cast<uint16_t>(pkt.type);
    if (!write_timestamp) {
      temp.packet_type |= 0x10;
      cb.writeObj(temp.packet_type);
    } else {
      temp.time_ms = pkt.timestamp_ms;
      cb.writeObj(temp);
    }
    cb.write(pkt.stream.GetData(), BITS_TO_BYTES(pkt.stream.GetWriteOffset()));
    curr_time_ms = pkt.timestamp_ms;
  }

  template<bool streamWrite>
  std::span<uint8_t> ReplayWriter<streamWrite>::getCompressedData(std::vector<uint8_t> &storage) {
    if constexpr (streamWrite) {
      zstd_cb.writer.finish();
      return std::span<uint8_t>{(uint8_t *) base_cb.data(), (size_t) base_cb.tell()};
    } else {
      storage.resize(zstd_compress_bound(base_cb.tell()));
      auto compressed_size = zstd_compress(storage.data(), storage.size(), base_cb.data(), base_cb.tell(), 18);
      storage.resize(compressed_size);
      storage.shrink_to_fit();
      return storage;
    }
  }

  template<bool streamWrite>
  owned_span<uint8_t> ReplayWriter<streamWrite>::createReplay() {
    DynamicMemGeneralSaveCB final_cb{};
    final_cb.writeObj(header); // temp write, will be written over later
    uint32_t curr_offs, future_offs;
    curr_offs = final_cb.tell();
    writeBlkToStream(header_blk, final_cb);
    future_offs = final_cb.tell();
    header.settings_blk_size = static_cast<uint16_t>(future_offs - curr_offs);
    header.magic = CURR_MAGIC;
    std::vector<uint8_t> tmp_vector{};
    auto compressed = getCompressedData(tmp_vector);
    final_cb.write(compressed.data(), (int) compressed.size());
    if (!footer_blk.isEmpty()) {
      header.footer_blk_offset = final_cb.tell();
      writeBlkToStream(footer_blk, final_cb);
    } else {
      header.footer_blk_offset = 0;
    }
    auto end_offs = final_cb.tell();
    final_cb.seekto(0);
    final_cb.writeObj(header);
    final_cb.seekto(end_offs);
    auto ptr = final_cb.acquire();
    return {ptr, (size_t) final_cb.tell()};
  }

  template class ReplayWriter<false>;
  template class ReplayWriter<true>;
} // namespace replay
