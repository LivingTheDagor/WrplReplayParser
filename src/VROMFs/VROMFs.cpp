
#include "VROMFs.h"
#include <algorithm>
#include <cstring>

#include "ioSys/dag_memIo.h"
#include "ioSys/blk_shared.h"

std::unique_ptr<File> VromfsFileIndex::getFile(std::shared_ptr<FileIndex> ths) {
  return std::make_unique<VromfsFile>(ths);
}

void VromfsFile::Save(std::ofstream *cb) {
  DataBlock blk;
  auto loaded = this->asIndex()->owner->parseFileToDatablock(*this, blk);
  if (loaded) {
    // this->length * 8 is just some value
    DynamicMemGeneralSaveCB cwr(this->length() * 8, 4 << 20);
    blk.saveToTextStream(cwr);
    cb->write((char *) cwr.data(), cwr.size());
  } else {
    auto raw = this->readRaw();
    cb->write(raw.data(), raw.size());
  }
}

bool VromfsFile::loadBlk(DataBlock &blk) { return this->asIndex()->owner->parseFileToDatablock(*this, blk); }

int64_t VromfsFile::read_impl(void *ptr, size_t length) {
  ZoneScopedN("VromfsFile::read_impl");
  auto data = this->readRaw();
  if (data.empty())
    return -1;
  if (length + read_offs > data.size())
    return -1;
  memcpy(ptr, data.data() + read_offs, length);
  read_offs += length;
  return length;
}

VROMFs::VROMFs(const std::string &fName) : fileName(fName), dir(fileName) {
  ZoneScoped;
  FullFileLoadCB f{fName};
  if (!f.fileHandle || !load_raw_vromfs_data(f))
    return;
  InPlaceMemLoadCB f2(raw_data->data(), (int) size);

  parse_raw_vromfs_data(f2);
}

VROMFs::VROMFs(const std::string &fName, const std::vector<std::string> &only) : fileName(fName), dir(fileName) {
  ZoneScoped;
  FullFileLoadCB f{fName};
  if (!f.fileHandle)
    return;
  if (load_filtered(f, only))
    return;
  // not a zstd packed archive (or the stream could not be read that way): load all of it
  f.seekto(0);
  if (!load_raw_vromfs_data(f))
    return;
  InPlaceMemLoadCB f2(raw_data->data(), (int) size);
  parse_raw_vromfs_data(f2);
}

bool VROMFs::load_filtered(IGenLoad &reader, const std::vector<std::string> &only) {
  ZoneScoped;
  if (!reader.readInto(hdr))
    return false;
  if ((hdr.label != _MAKE4C('VRFs') && hdr.label != _MAKE4C('VRFx')) || !checkTargetCode(hdr.target))
    return false;
  if (hdr.label == _MAKE4C('VRFx')) {
    if (!reader.readInto(extHdr))
      return false;
    reader.seekrel((int) (extHdr.size - sizeof(extHdr)));
  }
  if (!hdr.packedSz() || !hdr.zstdPacked())
    return false;
  std::vector<char> packed(hdr.packedSz());
  if (!reader.readExact(packed.data(), (int) packed.size()))
    return false;
  obfusc_vrom_data(packed.data(), (int) packed.size());

  std::unique_ptr<ZSTD_DStream, size_t (*)(ZSTD_DStream *)> ds(ZSTD_createDStream(), ZSTD_freeDStream);
  ZSTD_initDStream(ds.get());
  ZSTD_inBuffer in{packed.data(), packed.size(), 0};
  bool stream_end = false;
  // decodes up to cap bytes into dst; fewer only at the end of the stream
  auto pull = [&](char *dst, size_t cap, size_t &got) -> bool {
    ZSTD_outBuffer out{dst, cap, 0};
    while (out.pos < out.size && !stream_end) {
      const size_t r = ZSTD_decompressStream(ds.get(), &out, &in);
      if (ZSTD_isError(r))
        return false;
      if (r == 0)
        stream_end = true;
      else if (in.pos == in.size && out.pos < out.size)
        return false; // truncated
    }
    got = out.pos;
    return true;
  };

  // the name and offset tables are at the start: decode until they are complete
  std::vector<char> head;
  size_t produced = 0;
  auto grow_head = [&](size_t need) -> bool {
    need = std::min<size_t>(need, hdr.fullSz);
    while (head.size() < need) {
      const size_t at = head.size(), want = std::max<size_t>(need - at, 256 << 10);
      head.resize(at + want);
      size_t got = 0;
      if (!pull(head.data() + at, want, got))
        return false;
      head.resize(at + got);
      produced += got;
      if (got < want)
        break;
    }
    return head.size() >= need;
  };
  if (!grow_head(48))
    return false;
  auto rd32 = [&](size_t o) {
    int v;
    memcpy(&v, head.data() + o, 4);
    return v;
  };
  const int names_header = rd32(0), names_count = rd32(4), data_info_offset = rd32(16), data_info_count = rd32(20);
  if (names_header < 0 || names_count < 0 || data_info_offset < 0 || data_info_count != names_count)
    return false;
  if (!grow_head(std::max<size_t>(size_t(names_header) + 8 * size_t(names_count),
                                  size_t(data_info_offset) + 16 * size_t(data_info_count))))
    return false;
  struct Want {
    size_t off, size, at;
    std::string_view name;
  };
  std::vector<Want> wanted;
  size_t total = 0;
  for (int i = 0; i < names_count; i++) {
    uint64_t so;
    memcpy(&so, head.data() + names_header + 8 * size_t(i), 8);
    // the name may end past what is decoded so far
    while (so >= head.size() || !memchr(head.data() + so, 0, head.size() - so)) {
      if (head.size() >= hdr.fullSz || !grow_head(head.size() + (64 << 10)))
        return false;
    }
    const std::string_view name(head.data() + so);
    if (std::find(only.begin(), only.end(), name) == only.end())
      continue;
    const size_t off = (size_t) rd32(size_t(data_info_offset) + 16 * size_t(i));
    const size_t sz = (size_t) rd32(size_t(data_info_offset) + 16 * size_t(i) + 4);
    if (off + sz > hdr.fullSz)
      return false;
    wanted.push_back({off, sz, total, {}});
    total += sz;
    // names point into head, which may still grow: keep them as owned strings below
    wanted.back().name = name;
  }
  std::vector<std::string> names;
  names.reserve(wanted.size());
  for (auto &w: wanted)
    names.emplace_back(w.name);

  auto fs = std::make_shared<std::vector<char>>(total);
  size_t end = 0;
  for (auto &w: wanted)
    end = std::max(end, w.off + w.size);
  // copy what [pos, pos + n) of the decoded stream holds of each wanted file
  auto place = [&](const char *src, size_t pos, size_t n) {
    for (auto &w: wanted) {
      const size_t a = std::max(pos, w.off), b = std::min(pos + n, w.off + w.size);
      if (a < b)
        memcpy(fs->data() + w.at + (a - w.off), src + (a - pos), b - a);
    }
  };
  place(head.data(), 0, head.size());
  std::vector<char>().swap(head);
  std::vector<char> chunk(1 << 20);
  while (produced < end) {
    size_t got = 0;
    if (!pull(chunk.data(), chunk.size(), got) || got == 0)
      return false;
    place(chunk.data(), produced, got);
    produced += got;
  }

  raw_data = fs;
  size = (unsigned) total;
  for (size_t i = 0; i < wanted.size(); i++) {
    fs::path p(names[i]);
    auto file_ = std::make_shared<VromfsFileIndex>(p, this, std::span{fs->data() + wanted[i].at, wanted[i].size});
    dir.addFile(file_, p);
  }
  return true;
}

bool VROMFs::load_raw_vromfs_data(IGenLoad &reader) {
  ZoneScoped;
  // char embedded_md5[16];
  // unsigned char signature[SIGNATURE_MAX_SIZE];
  // int signature_size = 0;
  enum { HDR, CONTENT, MD5, ADDITIONAL_CONTENT, SIGNATURE };
  // const void *buffers[] = {&hdr, nullptr, embedded_md5, nullptr, signature};
  // unsigned buf_sizes[] = {sizeof(hdr), 0, sizeof(embedded_md5), 0, 0};
  void *buf = nullptr;

  std::shared_ptr<std::vector<char>> fs;

  std::ofstream outputFile;

  if (!reader.readInto(hdr))
    goto load_fail;
  fs = std::make_shared<std::vector<char>>(hdr.fullSz);

  if (hdr.label != _MAKE4C('VRFs') && hdr.label != _MAKE4C('VRFx'))
    goto load_fail;
  if (!checkTargetCode(hdr.target))
    goto load_fail;
  /*
  fs = (VirtualRomFsData *)mem->tryAlloc(FS_OFFS + hdr.fullSz);
  if (!fs)
      goto load_fail;
  new (fs, _NEW_INPLACE) VirtualRomFsData();
  fs->mtime = st.mtime;*/

  if (hdr.label == _MAKE4C('VRFx')) {
    if (!reader.readInto(extHdr))
      goto load_fail;
    /*
    if (hdr_ext.size >= sizeof(VirtualRomFsExtHdr))
    {
      fs->flags = hdr_ext.flags;
      fs->version = hdr_ext.version;
    }
    */
    reader.seekrel((int) (extHdr.size - sizeof(extHdr)));
  }

  if (hdr.packedSz()) {
    buf = malloc(hdr.packedSz());
    if (!buf)
      goto load_fail;
    if (!reader.readExact(buf, (int) hdr.packedSz()))
      goto load_fail;

    size_t sz = hdr.fullSz;
    if (hdr.zstdPacked()) {
      obfusc_vrom_data(buf, hdr.packedSz());
      sz = zstd_decompress((unsigned char *) fs->data(), sz, buf, hdr.packedSz());
      if (sz != hdr.fullSz)
        goto load_fail;
      obfusc_vrom_data(buf, hdr.packedSz());
    } else {
      assert(false && "data is zlib compressed!!!!");
    }
  } else {
    if (!reader.readExact(fs->data(), (int) hdr.fullSz))
      goto load_fail;
  }

  raw_data = fs;
  size = hdr.fullSz;
  if (buf)
    free(buf);
  return true;
load_fail:
  if (buf)
    free(buf);
  return false;
}

bool VROMFs::parse_raw_vromfs_data(IGenLoad &reader) {
  ZoneScoped;
  int names_header = reader.readInt();
  int names_count = reader.readInt();
  reader.seekrel(8); // skip u64
  int data_info_offset = reader.readInt();
  int data_info_count = reader.readInt();
  reader.seekrel(8);
  bool has_digest = names_header == 0x30;

  if (has_digest) {
    reader.seekrel(16);
  } // do nothing for now

  std::vector<std::string_view> file_names((size_t) names_count);
  uint64_t *basePtr = (uint64_t *) (raw_data->data() + names_header);
  uint64_t stringStart = 0;
  char *raw_data_ptr = raw_data->data();
  for (uint32_t i = 0; i < names_count; i++) {
    stringStart = basePtr[i];
    file_names[i] = std::string_view(raw_data_ptr + stringStart);
  }

  int *int_data_ptr = (int *) (raw_data_ptr + data_info_offset);
  int max = data_info_count * 4;
  bool foundDict = false;
  bool foundNM = false;

  for (uint32_t i = 0, z = 0; i < max; i += 4, z++) {
    ZoneScopedN("VROMFs::loop");
    int fileOffset = int_data_ptr[i];
    int fileSize = int_data_ptr[i + 1];
    std::string_view file_name = file_names[z];
    if (!foundNM && file_name == "\xFF\x3Fnm") {
      ZoneScopedN("VROMFs::NMParse");
      foundNM = true;
      auto data = std::span<char>(raw_data_ptr + fileOffset + 40, (size_t) fileSize - 40);
      ZstdLoadFromMemCB zReader(data);
      nm = std::make_shared<DBNameMap>();
      G_ASSERT(dblk::read_names(zReader, *nm, nullptr));
      continue; // prevents adding of NM to output directory
    }
    if (!foundDict && file_name.ends_with(".dict")) {
      ZoneScopedN("VROMFs::ZSTD_Dict");
      foundDict = true;
      auto data = std::span<char>(raw_data_ptr + fileOffset, (size_t) fileSize);
      dict = ZSTD_createDDict(data.data(), data.size());
      continue;
    }
    {
      ZoneScopedN("VROMFs::AddFile");
      fs::path p((std::string(file_name)));
      auto file_ = std::make_shared<VromfsFileIndex>(
        p, this, std::span{raw_data.get()->data() + (size_t) fileOffset, (size_t) fileSize});
      dir.addFile(file_, p);
    }
  }
  return true;
}
