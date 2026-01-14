

#ifndef MYEXTENSION_VROMFS_H
#define MYEXTENSION_VROMFS_H

#include "reader.h"
#include "FileSystem.h"
#include "zstd.h"
#include "DataBlock.h"


#define DEOBFUSCATE_ZSTD_DATA obfusc_vrom_data
#define OBFUSCATE_ZSTD_DATA   obfusc_vrom_data

class VROMFs;

#define SIGNATURE_MAX_SIZE (1024)


static const unsigned targetCode = _MAKE4C('PC');
static const unsigned targetCode2[2] = {_MAKE4C('iOS'), _MAKE4C('and')};

static inline bool checkTargetCode(unsigned code) {
  return targetCode == code || targetCode2[0] == code || targetCode2[1] == code;
}


static inline void obfusc_vrom_data(void *data, unsigned data_sz) {
  static const unsigned zstd_xor_p[4] = {0xAA55AA55, 0xF00FF00F, 0xAA55AA55, 0x12481248};

  if (data_sz < 16)
    return;
  unsigned *w = (unsigned *) data;
  w[0] ^= zstd_xor_p[0];
  w[1] ^= zstd_xor_p[1];
  w[2] ^= zstd_xor_p[2];
  w[3] ^= zstd_xor_p[3];
  if (data_sz >= 32) {
    w += (data_sz / 4) - 4;
    w[0] ^= zstd_xor_p[3];
    w[1] ^= zstd_xor_p[2];
    w[2] ^= zstd_xor_p[1];
    w[3] ^= zstd_xor_p[0];
  }
}


inline size_t zstd_decompress(void *dst, size_t maxOriginalSize, const void *src, size_t srcSize) {
  size_t enc_sz = ZSTD_decompress(dst, maxOriginalSize, src, srcSize);
  //CHECK_ERROR(enc_sz);
  return enc_sz;
}

struct VirtualRomFsDataHdr {
  unsigned label;
  unsigned target;
  unsigned fullSz;
  unsigned hw32;

  [[nodiscard]] unsigned packedSz() const { return hw32 & 0x3FFFFFFU; }

  [[nodiscard]]bool zstdPacked() const { return (hw32 & 0x40000000U) != 0; }

  [[nodiscard]] bool signedContents() const { return (hw32 & 0x80000000U) != 0; }
};

struct VirtualRomFsExtHdr {
  uint16_t size = 0, flags = 0;
  uint32_t version = 0;
};

class VromfsFile : public File {
public:
  VromfsFile() { data_size = 0; }

  VromfsFile(VROMFs *v_owner, const fs::path &path, const std::shared_ptr<std::vector<char>> &owner, int offset, int size) {
    this->owner = v_owner;
    data = std::shared_ptr<char>(owner, owner->data() + offset);
    data_size = size;
    init(path);
    vromfsPath = path.relative_path().parent_path();
  }

  std::span<char> readRaw() override {
    return {data.get(), data_size};
  }

  void Save(std::ofstream *cb) override;

  bool loadBlk(DataBlock &blk) override;


protected:
  fs::path vromfsPath;
  std::shared_ptr<char> data;
  unsigned long long data_size;
  VROMFs *owner;
  friend VROMFs;

};

class VROMFs {
public:
  explicit VROMFs(const std::string &fName) {
    fileName = fName;
    dir = std::make_shared<Directory>(fileName);
    FileReader f(fName);
    if (!load_raw_vromfs_data(f))
      return;
    BaseReader f2(raw_data->data(), (int) size, false);

    parse_raw_vromfs_data(f2);
  }

  explicit VROMFs(const std::string &fName, std::shared_ptr<Directory> &dir) {
    fileName = fName;
    this->dir = dir;
    FileReader f(fName);

    if (!load_raw_vromfs_data(f))
      return;
    BaseReader f2(raw_data->data(), (int) size, false);
    parse_raw_vromfs_data(f2);
  }

  std::shared_ptr<Directory> getDirectory() {
    if (dir == nullptr) {
      dir = std::make_shared<Directory>(fileName);
    }
    return dir;
  }

  bool parseFileToDatablock(File &file, DataBlock &blk) {
    auto data = file.readRaw();
    if (file.getExtension() != ".blk") {
      return false;
    }
    BaseReader rdr(data.data(), data.size(), false);
    if (!blk.loadFromStream(rdr, nm, dict))
      return false;

    auto v = file.getFullFilePath();
    if(blk.shared) // TODO shouldnt need to do this
      blk.shared->setSrc(v.string());
    return true;
  }

  ~VROMFs() {
    if (dict) {
      ZSTD_freeDDict(dict);
    }
  }
  //VROMFs(const std::span<char> &data) = 0;

protected:
  bool load_raw_vromfs_data(GenReader &reader) {
    //char embedded_md5[16];
    //unsigned char signature[SIGNATURE_MAX_SIZE];
    //int signature_size = 0;
    enum {
      HDR,
      CONTENT,
      MD5,
      ADDITIONAL_CONTENT,
      SIGNATURE
    };
    //const void *buffers[] = {&hdr, nullptr, embedded_md5, nullptr, signature};
    //unsigned buf_sizes[] = {sizeof(hdr), 0, sizeof(embedded_md5), 0, 0};
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
      reader.seekrel(extHdr.size - sizeof(extHdr));
    }

    if (hdr.packedSz()) {
      buf = malloc(hdr.packedSz());
      if (!buf)
        goto load_fail;
      if (!reader.read(buf, hdr.packedSz()))
        goto load_fail;

      unsigned long sz = hdr.fullSz;
      if (hdr.zstdPacked()) {
        obfusc_vrom_data(buf, hdr.packedSz());
        sz = (int) zstd_decompress((unsigned char *) fs->data(), sz, buf, hdr.packedSz());
        if (sz != hdr.fullSz)
          goto load_fail;
        obfusc_vrom_data(buf, hdr.packedSz());
      } else {
        assert(false && "data is zlib compressed!!!!");
      }
    } else {
      if (!reader.read(fs.get(), hdr.fullSz))
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

  bool parse_raw_vromfs_data(BaseReader &reader) {
    int names_header;
    int names_count;
    reader.readInto(names_header);
    reader.readInto(names_count);
    reader.seekrel(8); // skip u64
    int data_info_offset;
    int data_info_count;
    reader.readInto(data_info_offset);
    reader.readInto(data_info_count);
    reader.seekrel(8);
    bool has_digest = names_header == 0x30;

    if (has_digest) {
      reader.seekrel(16);
    } // do nothing for now

    std::vector<std::string_view> file_names(names_count);
    uint64_t *basePtr = (uint64_t *) (raw_data->data() + names_header);
    uint64_t stringStart = 0;
    char *raw_data_ptr = raw_data->data();
    for (int i = 0; i < names_count; i++) {
      stringStart = basePtr[i];
      file_names[i] = std::string_view(raw_data_ptr + stringStart);
    }

    int *int_data_ptr = (int *) (raw_data_ptr + data_info_offset);
    int max = data_info_count * 4;
    bool foundDict = false;
    bool foundNM = false;

    for (int i = 0, z = 0; i < max; i += 4, z++) {

      int fileOffset = int_data_ptr[i];
      int fileSize = int_data_ptr[i + 1];
      std::string_view file_name = file_names[z];
      if (!foundNM && file_name == "\xFF\x3Fnm") {
        foundNM = true;
        auto data = std::span<char>(raw_data_ptr + fileOffset + 40, fileSize - 40);
        ZstdReader zReader(data);
        int nm_names_count = 0;
        zReader.readCompressedUnsignedGeneric(nm_names_count);
        NameMap::ReadNames(zReader, nm, nm_names_count);
        continue; // prevents adding of NM to output directory
      }
      if (!foundDict && file_name.ends_with(".dict")) {
        foundDict = true;
        auto data = std::span<char>(raw_data_ptr + fileOffset, fileSize);
        dict = ZSTD_createDDict(data.data(), data.size());
        continue;
      }
      fs::path p((std::string(file_name)));
      auto file_ = std::make_shared<VromfsFile>(this, p, raw_data, fileOffset, fileSize);
      dir->addFile(file_, file_->vromfsPath);
    }
    return true;
  }

  std::string fileName;
  VirtualRomFsDataHdr hdr{};
  VirtualRomFsExtHdr extHdr;
  std::shared_ptr<std::vector<char>> raw_data;
  unsigned size{};
  std::shared_ptr<Directory> dir;
  std::shared_ptr<NameMap> nm;
  ZSTD_DDict_s *dict{};
};


#endif //MYEXTENSION_VROMFS_H