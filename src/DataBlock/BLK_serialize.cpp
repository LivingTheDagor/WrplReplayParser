#include "DataBlock.h"
#include "zstd.h"
//#include "rapidjson/writer.h"
//#include "rapidjson/stringbuffer.h"
#include "reader.h"

#include <memory>
#include <stdexcept>

#include <cstdint>
#include <unordered_map>
#include <fstream>

//using namespace rapidjson;


void DataBlock::saveToTextFile(const std::string &path) const {

  std::ofstream input(path, std::ios::binary);
  this->saveText(&input, 0);
}

void read_packed(DataBlockInfo &d, GenReader &crd) {

  crd.readCompressedUnsignedGeneric(d.nameId);
  crd.readCompressedUnsignedGeneric(d.paramsCount);
  crd.readCompressedUnsignedGeneric(d.blocksCount);
  if (d.blocksCount)
    crd.readCompressedUnsignedGeneric(d.firstBlock);
  else
    d.firstBlock = 0;
}

void write_packed(DataBlockInfo &d, BitStream &bs) {
  bs.WriteCompressed(d.nameId);
  bs.WriteCompressed(d.paramsCount);
  bs.WriteCompressed(d.blocksCount);
  if (d.blocksCount)
    bs.WriteCompressed(d.firstBlock);
}

void DataBlock::construct_param(ParamBin &p, Param *into) {
  int name_id_ = p.nameId;
  uint8_t type = p.type;
  union {
    int val;
    float real;
    struct {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t a;
    } c;
  } data{};
  data.val = (int)p.v;
  char *shared_params = this->shared->getComplex();
  switch (type) {
    case TYPE_STRING: {
      bool in_nm = data.val >> 31;
      int actual = data.val & 0x7FFFFFFF;
      std::string_view out;
      if (in_nm) {
        out = this->getNameFromId(actual);
      } else {
        out = std::string_view(shared_params + actual);
      }
      new(into) Param(name_id_, out);
    }
    break;
    case TYPE_INT: {
      new(into) Param(name_id_, data.val);
    }
    break;
    case TYPE_REAL: {
      new(into) Param(name_id_, data.real);
    }
    break;
    case TYPE_POINT2: {
      new(into) Param(name_id_, *(Point2 *) (shared_params + data.val));
    }
    break;
    case TYPE_POINT3: {
      new(into) Param(name_id_, *(Point3 *) (shared_params + data.val));
    }
    break;
    case TYPE_POINT4: {
      new(into) Param(name_id_, *(Point4 *) (shared_params + data.val));
    }
    break;
    case TYPE_IPOINT2: {
      new(into) Param(name_id_, *(IPoint2 *) (shared_params + data.val));
    }
      break;
    case TYPE_IPOINT3: {
      new(into) Param(name_id_, *(IPoint3 *) (shared_params + data.val));
    }
    break;
    case TYPE_BOOL: {
      new(into) Param(name_id_, data.val == 1);
    }
    break;
    case TYPE_E3DCOLOR: {
      new(into) Param(name_id_, E3DCOLOR(data.c.r, data.c.g, data.c.b, data.c.a));
    }
    break;
    case TYPE_MATRIX: {
      new(into) Param(name_id_, *(TMatrix *) (shared_params + data.val));
    }
    break;
    case TYPE_UINT64: {
      new(into) Param(name_id_, *(uint64_t *) (shared_params + data.val));
    }
      break;
    default:
      return;
      //EXCEPTION("Inavlid parameter type");
  }
  this->addParam(SharedPtr<Param>(into, this->shared));
}


void DataBlock::printParams(const std::vector<Param> &params_to_print, std::basic_ostream<char> &out) const {
  for (auto &p: params_to_print) {
    out << fmt::format("{}:", this->getNameFromId(p.name_id));
    switch (p.type) {

      case TYPE_STRING:
        out << fmt::format("t=\"{}\"\n", p.data.s);
        break;
      case TYPE_BOOL: {
        out << fmt::format("b={}\n", p.data.b ? "yes" : "no");
        break;
      }
      case TYPE_INT: {
        out << fmt::format("i={}\n", p.data.i);
        break;
      }
      case TYPE_REAL: {
        out << fmt::format("r={}\n", p.data.r);
        break;
      }
      case TYPE_POINT2: {
        out << fmt::format("p2={}, {}\n", p.data.p2.x, p.data.p2.y);
        break;
      }
      case TYPE_POINT3: {
        out << fmt::format("p3={}, {}, {}\n", p.data.p3.x, p.data.p3.y, p.data.p3.z);
        break;
      }
      case TYPE_POINT4: {
        out << fmt::format("p4={}, {}, {}, {}\n", p.data.p4.x, p.data.p4.y, p.data.p4.z, p.data.p4.w);
        break;
      }
      case TYPE_IPOINT2: {
        out << fmt::format("ip2={}, {}\n", p.data.ip2.x, p.data.ip2.y);
        break;
      }
      case TYPE_IPOINT3: {
        out << fmt::format("ip3={}, {}, {}\n", p.data.ip3.x, p.data.ip3.y, p.data.ip3.z);
        break;
      }
      case TYPE_E3DCOLOR: {
        out << fmt::format("c={}, {}, {}, {}\n", p.data.c.r, p.data.c.g, p.data.c.b, p.data.c.a);
        break;
      }
      case TYPE_MATRIX: {
        out << fmt::format("m=[[{}, {}, {}] [{}, {}, {}] [{}, {}, {}] [{}, {}, {}]]\n", p.data.tm.m[0][0], p.data.tm[0][1],
                           p.data.tm[0][2], p.data.tm[1][0], p.data.tm[1][1], p.data.tm[1][2], p.data.tm[2][0],
                           p.data.tm[2][1], p.data.tm[2][2], p.data.tm[3][0], p.data.tm[3][1], p.data.tm[3][2]);
        break;
      }
      case TYPE_UINT64: {
        out << fmt::format("i64={}\n", p.data.u64);
        break;
      }
      default:
        G_ASSERT(0);
    }
  }
}

bool NameMap::ReadNames(GenReader &crd, std::shared_ptr<NameMap> &names, uint32_t NamesCount) {

  uint32_t NameMapSize;
  crd.readCompressedUnsignedGeneric(NameMapSize);
  std::vector<char> namedata{};
  namedata.resize(NameMapSize);

  //std::shared_ptr<char[]> namedata = std::make_shared<char[]>(NameMapSize);
  crd.read(namedata.data(), (int) NameMapSize);
  names = std::make_shared<NameMap>(namedata, NamesCount);
  return true;
}

bool DataBlock::loadFromBinDump(GenReader &crd, const std::shared_ptr<NameMap> &names) {

  uint32_t NamesInNameMap = 0;
  crd.readCompressedUnsignedGeneric(NamesInNameMap);
  std::shared_ptr<DataBlockShared> shared_t;

  std::shared_ptr<NameMap> nameMap;
  if (names)
    nameMap = names;
  else
    NameMap::ReadNames(crd, nameMap, NamesInNameMap);

  uint32_t roDataBlocks = 0, paramsCnt = 0, complexDataSize = 0;
  crd.readCompressedUnsignedGeneric(roDataBlocks);
  crd.readCompressedUnsignedGeneric(paramsCnt);
  crd.readCompressedUnsignedGeneric(complexDataSize);

  shared_t = std::make_shared<DataBlockShared>(complexDataSize, paramsCnt, roDataBlocks);

  //std::shared_ptr<char[]> CData = std::make_shared<char[]>(complexDataSize);
  std::vector<ParamBin> Params(paramsCnt);
  crd.read(shared_t->getComplex(), (int) complexDataSize);
  crd.read(Params.data(), (int) (paramsCnt * sizeof(ParamBin)));
  int paramPtr = 0;
  DataBlock *base_ptr = shared_t->getBlocks();
  Param *ParamPtr = shared_t->getParams();
  for (uint32_t i = 0; i < roDataBlocks; i++) {
    DataBlock *current = base_ptr + i;
    DataBlockInfo data;
    read_packed(data, crd);
    if (i == 0) {
      current = this;
      current->name_id = ((int) data.nameId) - 1;
      current->blocks.reserve(data.blocksCount);
      current->params.reserve(data.paramsCount);
      current->nm = nameMap;
    } else {
      new(current) DataBlock(((int) data.nameId) - 1, data.paramsCount, data.blocksCount, nameMap);
    }
    current->setShared(shared_t);
    for (int blk_ptr = 0; blk_ptr < data.blocksCount; blk_ptr++) {
      current->blocks.emplace_back(base_ptr + blk_ptr + data.firstBlock, shared_t);
    }
    for (int tmp = 0; tmp < data.paramsCount; tmp++, paramPtr++) {

      current->construct_param(Params[paramPtr], ParamPtr);
      ++ParamPtr;
    }
  }

  return true;
}

bool DecompressData(GenReader &crd, ZSTD_DDict_s *zstd_dict) {
  return false;
}

bool DataBlock::loadFromStream(GenReader &crd, const std::shared_ptr<NameMap> &names, ZSTD_DDict_s *zstd_dict) {
  bool valid = false;
  this->Clear();
  BLKTypes label;
  if (!crd.readInto(label))
    return true; // empty BLK
  BLKType t(label);

  if (t.is_slim()) {
    if (!names) {
      return false; // slim needs nm
    }
    if (t.needs_dict() && !zstd_dict) {
      return false; //
    }

    if (label == BLKTypes::SLIM_ZSTD_DICT) {
      //ZstdLoadCB zcrd(crd, 0xFFFFFF, zstd_dict);
      int data_size = crd.getSize() - crd.readOffset();
      char *compressed = (char *) malloc(data_size);
      bool out = crd.tryRead(compressed, data_size);
      if (!out) {
        free(compressed);
        return false;
      }
      //std::ofstream outFile("test1.bin", std::ios::binary);
      //outFile.write(compressed.data(), compressed.size());
      //outFile.close();
      std::vector<char> decompressed(ZSTD_getFrameContentSize(compressed, data_size));
      ZSTD_DCtx *dctx = ZSTD_createDCtx();
      size_t result = ZSTD_decompress_usingDDict(
          dctx,
          decompressed.data(), decompressed.size(), // output buffer
          compressed, data_size,     // input buffer
          zstd_dict                                      // dictionary
      );
      free(compressed);
      ZSTD_freeDCtx(dctx);
      if (ZSTD_isError(result)) {
        std::cerr << "ZSTD Error: " << ZSTD_getErrorName(result) << "\n";
      }
      BaseReader rdr(decompressed.data(), (int) decompressed.size(), false);
      valid = loadFromBinDump(rdr, names);
    } else if (label == BLKTypes::SLIM) {
      valid = loadFromBinDump(crd, names);
    } else if (label == BLKTypes::SLIM_ZSTD) {
      ZstdLoadCB zcrd(crd, 0xFFFFFF, nullptr);
      valid = loadFromBinDump(zcrd, names);
    }
  } else if (label == BLKTypes::FAT) {
    valid = loadFromBinDump(crd, nullptr);
  } else if (label == BLKTypes::FAT_ZSTD) {

    unsigned csz = 0;
    crd.read(&csz, 3);
    ZstdLoadCB zcrd(crd, (int) csz, nullptr);

    BLKTypes l;
    zcrd.readInto(l);
    if (l != BLKTypes::FAT)
      return false;
    valid = loadFromBinDump(zcrd, nullptr);
  }
  return valid;
}

/*
 bool DataBlock::loadFromStream(BitStream &bs, DataBlock &out, const NameMap& names, std::vector<char>& zstd_dict)
{
    uint8_t *decompressed;
    out.Clear();
    uint8_t t = 0;
    bs.Read(t);
    auto type = BLKType(t);
    BitStream *to_use;
    bool isOriginal = false;
    //py::print(t);
    //py::print("is_zstd: ", type.is_zstd(), "; needs_dict: ", type.needs_dict(), "; is_slim: ", type.is_slim());
    //py::print(bs.GetDataPy());
    if(type.is_zstd())
    {
        int size = bs.GetNumberOfUnreadBits();
        const uint8_t * ptr = bs.GetDataOffset();

        size_t sz = ZSTD_getFrameContentSize(ptr, size);
        if(sz!=ZSTD_CONTENTSIZE_ERROR && sz != ZSTD_CONTENTSIZE_UNKNOWN) // this is true if we know the size basically
        {
        }
        else
        {
            sz = 0xFFFFF;
        }
        //py::print("size: ", sz);
        decompressed = new uint8_t[sz];
        auto ctx = ZSTD_createDCtx();
        if(type.needs_dict())
        {
            ZSTD_DCtx_loadDictionary(ctx, zstd_dict.data(), zstd_dict.size());
        }

        //int zstd_size = (int)ZSTD_getFrameContentSize(ptr, size);
        //auto * data = new uint8_t[zstd_size];
        ZSTD_outBuffer_s outBuff{decompressed, sz, 0};
        ZSTD_inBuffer_s inBuff(ptr, size, 0);
        ZSTD_decompressStream(ctx, &outBuff, &inBuff);
        to_use = new BitStream((const uint8_t *)outBuff.dst, 0xFFFFFF, false);

    }
    else
    {
        to_use = &bs;
        isOriginal = true;
    }

    int totalNames = 0;
    int name_map_size = 0;
    bs.ReadCompressed(totalNames);
    std::shared_ptr<NameMap> nm;

    if(type.is_slim())
    {
        nm = std::make_shared<NameMap>(names);
    }
    else
    {
        bs.ReadCompressed(name_map_size);
        std::shared_ptr<char[]> namedata = std::make_shared<char[]>(name_map_size);
        bs.Read(namedata.get(), name_map_size);
        //std::vector<std::string> names(totalNames);
        nm = std::make_shared<NameMap>(NameMap(namedata));
        int ptr = 0;
        for(int i = 0; i < totalNames; i++)
        {
            std::string temp = std::string(namedata.get()+ptr);
            int size = (int)temp.size()+1;
            nm->addName(temp);
            if (ptr+size > name_map_size) return false;
            ptr += size;
        }
    }

    uint32_t roDataBlocks = 0, paramsCnt = 0, complexDataSize = 0;
    bs.ReadCompressed(roDataBlocks);
    bs.ReadCompressed(paramsCnt);
    bs.ReadCompressed(complexDataSize);
    std::shared_ptr<char[]> ParamData = std::make_shared<char[]>(complexDataSize);
    bs.Read(ParamData.get(), complexDataSize);

    uint32_t current_pos = bs.GetReadOffset();
    bs.SetReadOffset(current_pos + (paramsCnt * 8 * 8));
    std::vector<std::shared_ptr<DataBlock>> blocks = std::vector<std::shared_ptr<DataBlock>>(roDataBlocks);
    std::vector<DataBlockInfo> packs = std::vector<DataBlockInfo>(roDataBlocks);
    blocks[0] = std::make_shared<DataBlock>(out);
    for(int i = 0; i < (int)roDataBlocks; i++)
    {
        read_packed(packs[i], bs);
        blocks[i] = std::make_shared<DataBlock>(DataBlock((int)packs[i].nameId-1, packs[i].paramsCount, packs[i].blocksCount, nm));
        blocks[i]->setSharedParams(ParamData);
    }
    bs.SetReadOffset(current_pos);
    for(int i = 0; i < (int)roDataBlocks; i++)
    {
        DataBlockInfo pack = packs[i];
        for (int p = 0; p < pack.paramsCount; p++)
        {
            blocks[i]->construct_param(bs);
        }
        for (int b = 0; b < pack.blocksCount; b++)
        {
            blocks[i]->addBlockUnsafe(blocks[pack.firstBlock+b]);
        }
    }
    out = *blocks[0];
    if(!isOriginal)
    {
        free(decompressed);
        delete to_use;
    }
    return true;
}
 */

struct write_data {
  int name_count;
  int name_size;
  int param_count;
  int param_data_size;
  int block_count;
  int block_size;
};

/*
struct ComplexDataContainer
{
    SmallTab<char> data;
    struct DataId
    {
        uint32_t ofs, sz;
    };
    std::vector<DataId> keys;
    typedef uint32_t hash_t;
    std::unordered_map<hash_t, uint32_t> hashToKeyId

    HashedKeyMap<hash_t, uint32_t> hashToKeyId;
    static inline hash_t hash(const char *s, size_t len)
    {
        hash_t ret = (hash_t)wyhash(s, len, 0);
        return ret ? ret : 0x80000000;
    };
    void alignTo(size_t al)
    {
        al -= 1;
        if ((data.size() & al) != 0)
        {
            const uint32_t add = uint32_t(((data.size() + al) & (~al)) - data.size());
            memset(data.insert_default(data.end(), add), 0, add);
        }
    }
    uint32_t addDataId(const char *val, uint32_t len, hash_t hash)
    {
        int it = hashToKeyId.findOr(hash, -1,
                                    [&, this](uint32_t id) { return keys[id].sz == len && memcmp(data.data() + keys[id].ofs, val, len) == 0; });
        if (it != -1)
            return keys[it].ofs;
        const uint32_t ofs = data.size();
        memcpy(data.insert_default(data.end(), len), val, len);
        const uint32_t id = keys.size();
        keys.emplace_back(DataId{ofs, len});
        hashToKeyId.emplace(hash, id);
        return ofs;
    }
    uint32_t addDataId(const char *val, uint32_t len) { return addDataId(val, len, hash(val, len)); }
};
*/

/*
void DataBlock::dumpBlocks(std::vector<DataBlock *> &blocks_, std::vector<DataBlockInfo> &blkinfo) const {
    int start_index = (int)blocks_.size();
    blocks_.reserve(blocks_.size()+block_count);
    blkinfo.reserve(blkinfo.size()+block_count);

    for (const auto& blk : blocks)
    {
        blocks_.push_back(blk.get());
        DataBlockInfo info{};
        info.nameId = blk->name_id+1;
        info.paramsCount = blk->params.size();
        info.blocksCount = blk->blocks.size();
        blkinfo.push_back(info);
    }
    int count = 0;
    for(const auto& blk : blocks)
    {
        if(blk->block_count>0)
        {
            blkinfo[start_index+count].firstBlock = blocks_.size();
            blk->dumpBlocks(blocks_, blkinfo);

        }
        count++;
    }
}
*/
/*
bool DataBlock::saveBinToStream(BitStream &bs) {
    write_data x{};

    std::vector<std::string> * nm_names = &this->nm->names;
    x.name_count = (int)nm_names->size();
    x.name_size = 0;
    std::cout << "start" << "\n";
    for(const auto& str : *nm_names)
    {
        x.name_size += (int)str.size()+1;
    }

    std::cout << "nmaes size" << "\n";
    std::vector<DataBlock *> block_data;
    std::vector<DataBlockInfo> block_info_data;
    block_data.push_back(this);
    block_info_data.emplace_back(this->name_id+1, this->params.size(), this->blocks.size(), 1);
    dumpBlocks(block_data, block_info_data);
    x.block_count = (int)block_info_data.size();
    for(const auto& block : block_data)
    {
        x.param_count += (int)block->params.size();
    }

    std::cout << "parsed_blocks" << "\n";
    for(auto data : block_info_data)
    {
        x.block_size += (data.nameId > 127) + 1;
        x.block_size += (data.paramsCount > 127) + 1;
        x.block_size += (data.blocksCount > 127) + 1;
        if (data.blocksCount)
            x.block_size += (data.firstBlock > 127) + 1;

        //std::cout<< nm->getNameFromId(data.nameId-1) << " | " <<  data.nameId << " | " <<  data.paramsCount << " | " <<  data.blocksCount << " | " << data.firstBlock << "\n";
    }

    std::cout << "did_datainfo_count : block count: " << block_data.size() << "\n";

    std::vector<std::pair<int, Param>> datas;
    std::vector<compiled_param> compiled;
    datas.reserve(x.param_count);
    compiled.reserve(x.param_count);
    int count = 0;
    for(const auto& blk : block_data)
    {
        std::cout << "block count:" << count << " out of: " << block_data.size() << " param count: " << blk->params.size() << "\n";
        count++;
        for (const auto& param : blk->params)
        {
            if(param->isLarge())
            {
                int index = -1;
                for(int i = 0; i < (int)datas.size(); i++)
                {
                    Param other_p = datas[i].second;
                    if(param->type == other_p.type)
                    {
                        bool br = false;
                        switch (param->type)
                        {
                            case TYPE_STRING:
                                if(*param->data.s == *other_p.data.s)
                                    br = true;
                                break;
                            case TYPE_POINT2:
                                if(param->data.p2 == other_p.data.p2)
                                    br = true;
                                break;
                            case TYPE_POINT3:
                                if(param->data.p3== other_p.data.p3)
                                    br = true;
                                break;
                            case TYPE_POINT4:
                                if(param->data.p4 == other_p.data.p4)
                                    br = true;
                                break;
                            case TYPE_IPOINT2:
                                if(param->data.ip2 == other_p.data.ip2)
                                    br = true;
                                break;
                            case TYPE_IPOINT3:
                                if(param->data.ip3 == other_p.data.ip3)
                                    br = true;
                                break;
                            case TYPE_MATRIX:
                                if(param->data.tm == other_p.data.tm)
                                    br = true;
                                break;
                        }
                        if(br)
                        {
                            index = i;
                            break;
                        }
                    }
                }
                if(index != -1)
                {
                    //count++;
                    compiled.emplace_back(param->name_id, param->type, 0, index);
                }
                else
                {
                    if(param->type == TYPE_STRING)
                    {
                        int index_ = nm->GetIdFromNameC(*param->data.s, false);
                        if(index_ > -1)
                        {
                            //count++;
                            compiled.emplace_back(param->name_id, param->type, index_ ^ (2 << 30), -1);
                        }
                        else
                        {
                            //count++;
                            datas.emplace_back(0, *param);
                            compiled.emplace_back(param->name_id, param->type, 0, datas.size()-1);
                        }
                    }
                    else
                    {
                        //count++;
                        datas.emplace_back(0, *param);
                        compiled.emplace_back(param->name_id, param->type, 0, datas.size()-1);
                    }
                }
            }
            else
            {
                compiled.emplace_back(param->name_id, param->type, param->data.i, -1);
            }
        }
    }

    std::cout << "parsed_params" << "\n";
    x.param_count = (int)compiled.size();
    for(auto& p : datas)
    {
        switch (p.second.type)
        {
            case TYPE_STRING:
                p.first = x.param_data_size;
                x.param_data_size += (int)p.second.data.s->size()+1;
                break;
            case TYPE_POINT2:
            case TYPE_IPOINT2:
                p.first = x.param_data_size;
                x.param_data_size += 8;
                break;
            case TYPE_POINT3:
            case TYPE_IPOINT3:
                p.first = x.param_data_size;
                x.param_data_size += 12;
                break;
            case TYPE_POINT4:
                p.first = x.param_data_size;
                x.param_data_size += 16;
                break;
            case TYPE_MATRIX:
                p.first = x.param_data_size;
                x.param_data_size += 48;
                break;
        }
    }

    std::cout << "param_sizes" << "\n";
    // names in name map give 2 bytes, nm size given 3
    // num of blocks given 2, number of params given 2, param_data_size given 4

    bs.reserveBits((1+2+3+x.name_size+2+2+4+x.param_data_size+x.param_count*8+x.block_size)*8);
    bs.ResetWritePointer();
    uint8_t temp = 0x1;
    bs.Write(temp);
    bs.WriteCompressed(x.name_count);
    bs.WriteCompressed(x.name_size);
    for(const auto & nm_name : *nm_names)
    {
        bs.Write(nm_name.c_str(), nm_name.size()+1);
    }
    bs.WriteCompressed(x.block_count);
    bs.WriteCompressed(x.param_count);
    bs.WriteCompressed(x.param_data_size);
    //for(auto & p : compiled)
    //{
    //}
    for(const auto & p : datas)
    {
        switch (p.second.type)
        {
            case TYPE_STRING:
                bs.Write(p.second.data.s->c_str(), p.second.data.s->size()+1);
                break;
            case TYPE_POINT2:
            case TYPE_IPOINT2:
                bs.Write(p.second.data.p2);
                break;
            case TYPE_POINT3:
            case TYPE_IPOINT3:
                bs.Write(p.second.data.p3);
                break;
            case TYPE_POINT4:
                bs.Write(p.second.data.p4);
                break;
            case TYPE_MATRIX:
                bs.Write(p.second.data.tm);
                break;
        }
    }
    for(auto & p : compiled)
    {
        if(p.ptr > -1)
        {
            p.data = datas[p.ptr].first;
        }
        bs.Write(p.name);
        bs.Devance(8);
        bs.Write(p.type);
        bs.Write(p.data);
    }
    for (auto & blk : block_info_data)
    {
        write_packed(blk, bs);
    }
    //for(const auto& str : *nm_names)
    //{
    //}
    return true;
}
 */