
#include "VROMFs.h"

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
  FullFileLoadCB f{fName.c_str()};
  if (!load_raw_vromfs_data(f))
    return;
  InPlaceMemLoadCB f2(raw_data->data(), (int) size);

  parse_raw_vromfs_data(f2);
}

bool VROMFs::load_raw_vromfs_data(IGenLoad &reader) {
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
    int fileOffset = int_data_ptr[i];
    int fileSize = int_data_ptr[i + 1];
    std::string_view file_name = file_names[z];
    if (!foundNM && file_name == "\xFF\x3Fnm") {
      foundNM = true;
      auto data = std::span<char>(raw_data_ptr + fileOffset + 40, (size_t) fileSize - 40);
      ZstdLoadFromMemCB zReader(data);
      nm = std::make_shared<DBNameMap>();
      G_ASSERT(dblk::read_names(zReader, *nm, nullptr));
      continue; // prevents adding of NM to output directory
    }
    if (!foundDict && file_name.ends_with(".dict")) {
      foundDict = true;
      auto data = std::span<char>(raw_data_ptr + fileOffset, (size_t) fileSize);
      dict = ZSTD_createDDict(data.data(), data.size());
      continue;
    }
    fs::path p((std::string(file_name)));
    auto file_ = std::make_shared<VromfsFileIndex>(
      p, this, std::span{raw_data.get()->data() + (size_t) fileOffset, (size_t) fileSize});
    dir.addFile(file_, p);
  }
  return true;
}
