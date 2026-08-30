#include "Replay/Replay.h"
#include "filesystem"
#include "dag_assert.h"
#include "libdeflate.h"
#include "zlib.h"
#include "danet/daNetTypes.h"

std::string packet_names[]{"End", "Start", "Aircraft", "Chat", "MPI", "NextSegment", "ECS", "Snapshot", "ECS_Msg_Sync"};

namespace fs = std::filesystem;


IReplayReader *Replay::getReplayReader() {
  ZoneScopedN("Replay::getReplayReader");
  if (!this->isValid())
    EXCEPTION("Invalid Replay: {}", this->Data.getFileName());
  return new FullDecompressReplayReader{*this};
}

IReplayReader *Replay::getCompressedReplayReader() {
  ZoneScopedN("Replay::getCompressedReplayReader");
  if (!this->isValid())
    EXCEPTION("Invalid Replay: {}", this->Data.getFileName());
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
    return new CompressedReplayReader{*this, rdr, data.size()};
  }
}

std::span<uint8_t> Replay::FileReplayData::getData(Replay *rpl) {
  ZoneScopedN("FileReplayData::getData");
  this->ref_count++;
  if (!this->zlib_data.empty()) {
    return this->zlib_data;
  }
  this->reader.seekto(rpl->zlib_offs);
  this->zlib_data.resize(rpl->zlib_size);
  this->reader.read(this->zlib_data.data(), rpl->zlib_size);
  return this->zlib_data;
}

void Replay::FileReplayData::afterParse() {

  G_ASSERT(ref_count > 0);
  ref_count--;
  if (ref_count == 0) // we don't want to hold onto data for any longer than needed, lots of memory, esp for big replays
  {
    zlib_data.clear();
    zlib_data.shrink_to_fit();
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

std::span<uint8_t> Replay::InMemoryReplayData::getData(Replay *rpl) {
  if (this->data.size() < rpl->zlib_offs + rpl->zlib_size)
    return {};
  return {this->data.data() + rpl->zlib_offs, rpl->zlib_size};
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
  this->Data.emplace<FileReplayData>(File, replay_path);
  load();
}

ReplayHeader *Replay::getHeader() {
  if (is_valid)
    return &header;
  return nullptr;
}

DataBlock *Replay::getHeaderBlk() {
  if (is_valid)
    return &header_blk;
  return nullptr;
}

DataBlock *Replay::getFooterBlk() {
  if (is_valid)
    return &footer_blk;
  return nullptr;
}

#define BAD_REPLAY(conditional) \
  {                             \
    if (!(conditional)) {       \
      is_valid = false;         \
      return;                   \
    }                           \
  }

void Replay::load() {
  ZoneScopedN("Replay::load");
  auto file_size = this->Data.getRemainingSize(0);
  BAD_REPLAY(file_size != -1);
  BAD_REPLAY(this->Data.ReadInto(this->header, 0));
  BAD_REPLAY(this->header.header == 0x1000ace5);
  BAD_REPLAY(this->header.magic == CURR_MAGIC);
  zlib_offs = sizeof(ReplayHeader) + this->header.settings_blk_size;

  if (this->header.settings_blk_size) {
    std::vector<uint8_t> header_bytes{};
    header_bytes.resize(this->header.settings_blk_size);
    BAD_REPLAY(this->Data.ReadInto(header_bytes.data(), header_bytes.size(), sizeof(ReplayHeader)));
    InPlaceMemLoadCB rdr{(char *) header_bytes.data(), (int) header_bytes.size()};
    BAD_REPLAY(this->header_blk.loadFromStream(rdr, nullptr));
  }

  if (this->header.footer_blk_offset) {
    std::vector<uint8_t> footer_bytes{};
    auto remainingSize = this->Data.getRemainingSize(this->header.footer_blk_offset);

    zlib_size = header.footer_blk_offset - zlib_offs;
    BAD_REPLAY(remainingSize != -1);
    footer_bytes.resize(remainingSize);

    BAD_REPLAY(this->Data.ReadInto(footer_bytes.data(), footer_bytes.size(), this->header.footer_blk_offset));
    InPlaceMemLoadCB rdr{(char *) footer_bytes.data(), (int) footer_bytes.size()};
    BAD_REPLAY(this->footer_blk.loadFromStream(rdr, nullptr));
  } else {
    zlib_size = file_size - zlib_offs;
  }
}

// IReplayReader *Replay::getStreamingReplayReader(uint32_t time_wait) {
//   if (!this->isValid())
//     EXCEPTION("Invalid Replay: {}", this->Data.getFileName());
//   G_ASSERT(Data.type() == File);
//   auto d = Data.asType<FileReplayData>();

//  auto *rdr = new FileStreamReader(d->reader.getFName(), time_wait);
//  rdr->seekto(this->zlib_offs);
//  return new CompressedReplayReader{*this, rdr, 0x7FFFFFFF, false};
//}

ServerReplay::ServerReplay(std::vector<std::span<uint8_t>> &data, bool owns) {
  for (auto &d: data) {
    this->replay_files.emplace_back(new Replay(d, owns));
  }
}

DataBlock *ServerReplay::getFooterBlk() {
  for (size_t i = this->replay_files.size(); i > 0; --i) {
    auto &blk = this->replay_files[i - 1]->footer_blk;
    if (!blk.isEmpty())
      return &blk;
  }
  return nullptr;
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
  fs::path path{b_path};
  std::vector<fs::path> files;
  readFilesFromDirectory(path, files);
  std::vector<std::string> repl_paths;
  if (auto p = file_exists("0000.wrpl", files); !p.empty()) {
    repl_paths.emplace_back(p);
  } else {
    EXCEPTION("Invalid ServerReplay, unable to find 0000.wrpl in supposed directory {}", path.string());
  }
  for (int i = 1; i > 0; i += 2) {
    if (auto p = file_exists(fmt::format("{:0>4}.wrpl", i), files); !p.empty()) {
      repl_paths.emplace_back(p);
    } else {
      break;
    }
  }
  if (!repl_paths.empty()) {
    replay_files.reserve(repl_paths.size());
    for (auto &path_: repl_paths) {
      replay_files.emplace_back(new Replay(path_));
    }
  }
}


IReplayReader *ServerReplay::getReplayReader() { return new ServerReplayReader<false>(*this); }

IReplayReader *ServerReplay::getCompressedReplayReader() { return new ServerReplayReader<true>(*this); }

bool ServerReplay::isValid() {
  for (auto &rpl: replay_files) {
    if (!rpl->is_valid)
      return false;
  }
  return true;
}

template<bool streamWrite>
void ReplayWriter<streamWrite>::write(const void *data, size_t size, uint32_t time_ms, ReplayPacketType type) {
  // 2 is packet type, 4 is potential time_ms, and 5 is max size of packet size header
  tmp_data.resize(size + 2 + 4 + 5);
  ConstrainedMemSaveCB tmp_cb{tmp_data.data(), static_cast<int>(tmp_data.size())};
  uint32_t curr_pkt_size = (uint32_t) size + 2;
  uint16_t curr_type = (uint16_t) type;
  bool should_write_time = (time_ms != curr_time_ms);
  curr_pkt_size += should_write_time ? 4 : 0;
  writePacketSize(tmp_cb, BYTES_TO_BITS(curr_pkt_size));
  if (!should_write_time)
    curr_type |= 0x10;
  tmp_cb.writeObj(curr_type);
  if (should_write_time)
    tmp_cb.writeObj(time_ms);
  curr_time_ms = time_ms;
  tmp_cb.write(data, (int) size);
  auto &cb = getWriter();
  cb.write(tmp_data.data(), tmp_cb.tell());
}
template<bool streamWrite>
void ReplayWriter<streamWrite>::write2(const ReplayPacket &pkt) {}

template<bool streamWrite>
std::span<uint8_t> ReplayWriter<streamWrite>::getCompressedData(std::vector<uint8_t> &storage) {
  if constexpr (streamWrite) {
    zlib_cb.writer.finish();
    return std::span<uint8_t>{(uint8_t *) base_cb.data(), (size_t) base_cb.tell()};
  } else {
    auto compressor = libdeflate_alloc_compressor(9);
    size_t max_size = libdeflate_deflate_compress_bound(compressor, base_cb.tell());
    storage.resize(max_size);
    size_t compressed_size =
      libdeflate_zlib_compress(compressor, base_cb.data(), base_cb.tell(), storage.data(), max_size);
    libdeflate_free_compressor(compressor);
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
  auto compressed = getCompressedData(tmp_data);
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
