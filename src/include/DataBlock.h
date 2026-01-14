


#ifndef WTFILEUTILS_DATABLOCK_H
#define WTFILEUTILS_DATABLOCK_H

#include <utility>
#include <vector>
#include <cstdint>
#include <memory>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "BitStream.h"
#include "reader.h"
#include "zstd.h"
#include "math/dag_Point2.h"
#include "math/dag_Point3.h"
#include "math/dag_Point4.h"
#include "math/dag_e3dColor.h"
#include "math/dag_TMatrix.h"
#include "math/integer/dag_IPoint2.h"
#include "math/integer/dag_IPoint3.h"
#include <unordered_map>
#include "StringTableAllocator.h"

class VROMFs;

class DataBlock;

class NameMap;

struct DataBlockShared;

struct DataBlockInfo {
  uint32_t nameId;
  uint16_t paramsCount, blocksCount;
  uint32_t firstBlock;

  DataBlockInfo() = default;

  DataBlockInfo(uint32_t nameId, uint16_t paramsCount, uint16_t blocksCount, uint32_t firstBlock) {
    this->nameId = nameId;
    this->paramsCount = paramsCount;
    this->blocksCount = blocksCount;
    this->firstBlock = firstBlock;
  }
};

class NameMap {
private:

  StringTableAllocator names_;
  //std::shared_ptr<char[]> shared_names;
  std::vector<int> name_idx;
  std::unordered_map<std::string_view, int> name_lookup;
  friend DataBlock;

  static std::string format_name(const std::string_view &str) {
    std::string ret;
    ret.reserve(str.length());
    for (auto chr: str) {
      if (chr != ' ')
        ret.push_back(chr);
      else
        ret.push_back('_');
    }
    return ret;
  }

  void initialize(std::span<char> &data, int NamesCount)
  {
    // sets up indexes
    auto namesPtr = data.data();
    auto namesPtrStart = data.data();
    this->name_idx.resize(NamesCount);
    for (uint32_t i = 0; i < NamesCount; i++) {
      this->name_idx[i] = (namesPtr-namesPtrStart);
      auto len = strlen(namesPtr);
      namesPtr += len + 1;
    }

    // sets up unordered_map
    for(int i = 0; i < name_idx.size(); i++)
    {
      name_lookup[this->getNameFromId(i)] = i;
    }
  }

public:
  static bool ReadNames(GenReader &crd, std::shared_ptr<NameMap> &names, uint32_t NamesCount);

  NameMap() {
    this->names_ = StringTableAllocator{};
    //this->names = std::vector<std::string>(0);
  }

  explicit NameMap(std::span<char> data, int NamesCount) {
    this->names_ = StringTableAllocator{};
    this->names_.addDataRaw(data.data(), data.size());
    this->initialize(data, NamesCount);
  }

  //int addName(const std::string& name) {
  //    names.push_back(name);
  //    return (int)names.size()-1;
  //}
  int addName(const std::string_view &name) {
    auto idx = this->names_.addDataRaw(name.data(), name.length()+1);
    auto sz = this->name_idx.size();
    this->name_idx.emplace_back(idx);
    //names.emplace_back(n);
    name_lookup[std::string_view(this->names_.getDataRawUnsafe(idx))] = (int) sz;
    return (int) sz;
  }

  [[nodiscard]] int GetIdFromName(const std::string_view &name, bool do_check) const {
    if (do_check && (int) name.find(' ') != -1) {
      EXCEPTION(R"(Invalid string passed to GetIdFromNameC() "{}', fixed: "{}")", name.data(), format_name(name).data());
    }
    auto it = this->name_lookup.find(std::string(name));
    if(it == this->name_lookup.end())
      return -1;
    return it->second;
  }

  int GetIdFromName(const std::string_view &name) {
    if ((int) name.find(' ') != -1) {
      EXCEPTION(R"(Invalid string passed to GetIdFromNameC() "{}', fixed: "{}")", name.data(), format_name(name).data());
    }
    auto it = this->name_lookup.find(std::string(name));
    if(it == this->name_lookup.end())
      return -1;
    return it->second;
  }
  int getIdFromNameAdd(const std::string_view &name)
  {
    int id = this->GetIdFromName(name);
    if(id == -1)
    {
      return this->addName(name);
    }
    return id;
  }
  int getIdFromNameAdd(const std::string &name)
  {
    int id = this->GetIdFromName(name);
    if(id == -1)
    {
      return this->addName(name);
    }
    return id;
  }

  [[nodiscard]] std::string_view getNameFromId(int name_id) const {
    return this->names_.getDataRawUnsafe(this->name_idx[name_id]);
  }

};

template<typename T>
class SharedPtr {
private:
  struct ControlBlock {
    int ref_count = 1;
    T *object = nullptr;
    std::shared_ptr<DataBlockShared> shared = nullptr;

    explicit ControlBlock(T *obj)
        : object(obj) {
    }

    ControlBlock(T *obj, std::shared_ptr<DataBlockShared> &shared)
        : object(obj), shared(shared) {
    }

    ControlBlock(const ControlBlock &) = delete;

    ControlBlock &operator=(const ControlBlock &) = delete;
  };

  ControlBlock *control = nullptr;

  void release() {
    if (!control) return;

    if (--control->ref_count == 0) {
      if (control->shared) {
        control->object->~T();
      } else {
        delete control->object;
      }
      delete control;
    }
    control = nullptr;
  }

  explicit SharedPtr(ControlBlock *cb)
      : control(cb) {
  }


public:
  SharedPtr() = default;

  explicit SharedPtr(T *obj) {
    if (obj)
      control = new ControlBlock(obj);
    else
      control = nullptr;
  }

  SharedPtr(std::nullptr_t) : control(nullptr) {}


  SharedPtr(T *obj, std::shared_ptr<DataBlockShared> shared) {
    control = new ControlBlock(obj, shared);
  }

  SharedPtr(const SharedPtr &other) {
    control = other.control;
    if (control) ++control->ref_count;
  }

  SharedPtr(SharedPtr &&other) noexcept
      : control(other.control) {
    other.control = nullptr;
  }

  SharedPtr &operator=(SharedPtr &&other) noexcept {
    if (this != &other) {
      release();
      control = other.control;
      other.control = nullptr;
    }
    return *this;
  }

  SharedPtr &operator=(const SharedPtr &other) {
    if (this != &other) {
      release();
      control = other.control;
      if (control) ++control->ref_count;
    }
    return *this;
  }

  ~SharedPtr() {
    release();
  }

  [[nodiscard]] T *get() const { return control ? control->object : nullptr; }

  T &operator*() const { return *get(); }

  T *operator->() const { return get(); }

  explicit operator bool() const {
    return control && control->object != nullptr;
  }

  [[nodiscard]] int use_count() const { return control ? control->ref_count : 0; }

  template<typename... Args>
  static SharedPtr make(Args &&... args) {
    return SharedPtr(new T(std::forward<Args>(args)...));
  }
};


//typedef std::variant<std::string, int, float, Point2, Point3, Point4, IPoint3, bool, E3DCOLOR, TMatrix> Value
union Value {
  std::string_view s;
  int i{};
  float r;
  Point2 p2;
  Point3 p3;
  Point4 p4;
  IPoint2 ip2;
  IPoint3 ip3;
  bool b;
  E3DCOLOR c;
  TMatrix tm;
  uint64_t u64;
  char data_ptr;

  Value() {}
};


class DataBlock {
public:
  struct Param {
    uint8_t type;
    Value data;
    int name_id;
    DataBlock *owner;


    Param() {
      type = TYPE_NONE;
      name_id = -1;
      owner = nullptr;
    }

    ~Param() = default;

    std::string_view getName()
    {
      G_ASSERT(this->owner);
      return this->owner->getNameFromId(this->name_id);
    }

  private:

    Param(uint8_t type, int name_id) {
      this->type = type;
      this->name_id = name_id;
      owner = nullptr;
    }



    [[nodiscard]] bool isLarge() const {
      return type == TYPE_STRING || type == TYPE_MATRIX || type == TYPE_IPOINT3 || type == TYPE_IPOINT2 ||
             type == TYPE_POINT4 || type == TYPE_POINT3 || type == TYPE_POINT2;
    }

    friend DataBlock;
  public:
    Param(int name_id, std::string_view data) : Param(TYPE_STRING, name_id) {
      this->data.s = data;
    }

    Param(int name_id, int data) : Param(TYPE_INT, name_id) { this->data.i = data; }

    Param(int name_id, float data) : Param(TYPE_REAL, name_id) { this->data.r = data; }

    Param(int name_id, const Point2 &data) : Param(TYPE_POINT2, name_id) { this->data.p2 = data; }

    Param(int name_id, const Point3 &data) : Param(TYPE_POINT3, name_id) { this->data.p3 = data; }

    Param(int name_id, const Point4 &data) : Param(TYPE_POINT4, name_id) { this->data.p4 = data; }

    Param(int name_id, const IPoint2 &data) : Param(TYPE_IPOINT2, name_id) { this->data.ip2 = data; }

    Param(int name_id, const IPoint3 &data) : Param(TYPE_IPOINT3, name_id) { this->data.ip3 = data; }

    Param(int name_id, bool data) : Param(TYPE_BOOL, name_id) { this->data.b = data; }

    Param(int name_id, const E3DCOLOR &data) : Param(TYPE_E3DCOLOR, name_id) { this->data.c = data; }

    Param(int name_id, const TMatrix &data) : Param(TYPE_MATRIX, name_id) { this->data.tm = data; }

    Param(int name_id, const uint64_t &data) : Param(TYPE_UINT64, name_id) { this->data.u64 = data; }
  };

  DataBlock();

  explicit DataBlock(const std::shared_ptr<NameMap> &nm);

  DataBlock(int name_id, int prealloc_param, int prealloc_block, const std::shared_ptr<NameMap> &nm);

  void saveText(std::ostream *cb, int level) const;


  bool writeText(std::ostream *cb, int level) const;

  int getRefCount() {
    return (int) this->shared.use_count();
  }

  enum ParamType : uint8_t {
    TYPE_NONE,
    TYPE_STRING,   ///< Text string.
    TYPE_INT,      ///< Integer.
    TYPE_REAL,     ///< #real (float).
    TYPE_POINT2,   ///< Point2.
    TYPE_POINT3,   ///< Point3.
    TYPE_POINT4,   ///< Point4.
    TYPE_IPOINT2,  ///< IPoint2.
    TYPE_IPOINT3,  ///< IPoint3.
    TYPE_BOOL,     ///< Boolean.
    TYPE_E3DCOLOR, ///< E3DCOLOR.
    TYPE_MATRIX,   ///< TMatrix.
    TYPE_UINT64,    ///< uint64_t
    TYPE_COUNT
  };
  static constexpr std::array<const char *, TYPE_COUNT+1> ParamTypeNames = {
      "NONE",
      "STRING",
      "INT",
      "REAL",
      "POINT2",
      "POINT3",
      "POINT4",
      "IPOINT2",
      "IPOINT3",
      "BOOL",
      "E3DCOLOR",
      "MATRIX",
      "UINT64",
      "COUNT"
  };

private:


  struct ParamBin {
    uint32_t nameId: 24;
    uint32_t type: 8;
    uint32_t v;
  };


public:
  bool getStr(int param_number, std::string &out) const;

  bool getBool(int param_number, bool &out) const;

  bool getInt(int param_number, int &out) const;

  bool getReal(int param_number, float &out) const;

  bool getPoint2(int param_number, Point2 &out) const;

  bool getPoint3(int param_number, Point3 &out) const;

  bool getPoint4(int param_number, Point4 &out) const;

  bool getIPoint2(int param_number, IPoint2 &out) const;

  bool getIPoint3(int param_number, IPoint3 &out) const;

  bool getE3DColor(int param_number, E3DCOLOR &out) const;

  bool getTMatrix(int param_number, TMatrix &out) const;

  bool getStr(const std::string &name, std::string &out, int index) const;

  bool getBool(const std::string &name, bool &out, int index) const;

  bool getInt(const std::string &name, int &out, int index) const;

  bool getReal(const std::string &name, float &out, int index) const;

  bool getPoint2(const std::string &name, Point2 &out, int index) const;

  bool getPoint3(const std::string &name, Point3 &out, int index) const;

  bool getPoint4(const std::string &name, Point4 &out, int index) const;

  bool getIPoint2(const std::string &name, IPoint2 &out, int index) const;

  bool getIPoint3(const std::string &name, IPoint3 &out, int index) const;

  bool getE3DColor(const std::string &name, E3DCOLOR &out, int index) const;

  bool getTMatrix(const std::string &name, TMatrix &out, int index) const;

  const char *getStr(int param_number) const;

  bool getBool(int param_number) const;

  int getInt(int param_number) const;

  real getReal(int param_number) const;

  Point2 getPoint2(int param_number) const;

  Point3 getPoint3(int param_number) const;

  Point4 getPoint4(int param_number) const;

  IPoint2 getIPoint2(int param_number) const;

  IPoint3 getIPoint3(int param_number) const;

  E3DCOLOR getE3DColor(int param_number) const;

  TMatrix getTMatrix(int param_number) const;

  uint64_t getUInt64(int param_number) const;

  bool getBoolByNameId(int paramNameId, bool def) const;

  const char *getStr(const char *name, const char *ref) const;

  bool getBool(const char *name, bool ref) const;

  int getInt(const char *name, int ref) const;

  real getReal(const char *name, real ref) const;

  Point2 getPoint2(const char *name, const Point2 &ref) const;

  Point3 getPoint3(const char *name, const Point3 &ref) const;

  Point4 getPoint4(const char *name, const Point4 &ref) const;

  IPoint2 getIPoint2(const char *name, const IPoint2 &ref) const;

  IPoint3 getIPoint3(const char *name, const IPoint3 &ref) const;

  E3DCOLOR getE3dcolor(const char *name, const E3DCOLOR &ref) const;

  TMatrix getTm(const char *name, const TMatrix &ref) const;

  uint64_t getUInt64(const char *name, const uint64_t &ref) const;

  inline const char *getStrByNameId(int param_number, const char *def) const {
    auto ret = this->getStr(param_number);
    return ret ? ret : def;
  }


  inline int findParam(int name_id_) const {
    for (int i = 0; i < params.size(); i++) {
      auto &param = this->params[i];
      if (param->name_id == name_id_)
        return i;
    }
    return -1;
  }

  inline int findParam(const char *name) const {
    int name_id = this->nm->GetIdFromName(name);
    return this->findParam(name_id);
  }

  inline int findParam(int name_id_, int after) const {
    for (int i = after + 1; i < params.size(); i++) {
      auto &param = this->params[i];
      if (param->name_id == name_id_)
        return i;
    }
    return -1;
  }

  const char *getParamName(uint32_t i) const { return this->nm->getNameFromId(getParamNameId(i)).data(); }

  bool getStrCount(const std::string name) const;

  bool getBoolCount(const std::string name) const;

  bool getIntCount(const std::string name) const;

  bool getRealCount(const std::string name) const;

  bool getPoint2Count(const std::string name) const;

  bool getPoint3Count(const std::string name) const;

  bool getPoint4Count(const std::string name) const;

  bool getIPoint2Count(const std::string name, IPoint2 &out, int index) const;

  bool getIPoint3Count(const std::string name, IPoint3 &out, int index) const;

  bool getE3DColorCount(const std::string name, E3DCOLOR &out, int index) const;

  bool getTMatrixCount(const std::string name, TMatrix &out, int index) const;

  std::vector<std::string> getStrVector(int name_id) const;

  std::vector<bool> getBoolVector(int name_id) const;

  std::vector<int> getIntVector(int name_id) const;

  std::vector<float> getRealVector(int name_id) const;

  std::vector<Point2> getPoint2Vector(int name_id) const;

  std::vector<Point3> getPoint3Vector(int name_id) const;

  std::vector<Point4> getPoint4Vector(int name_id) const;

  std::vector<IPoint2> getIPoint2Vector(int name_id) const;

  std::vector<IPoint3> getIPoint3Vector(int name_id) const;

  std::vector<E3DCOLOR> getE3DColorVector(int name_id) const;

  std::vector<TMatrix> getTMatrixVector(int name_id) const;

  std::vector<std::string> getStrVector(std::string &name) const { return getStrVector(getNameIdNoAdd(name)); }

  std::vector<bool> getBoolVector(std::string &name) const { return getBoolVector(getNameIdNoAdd(name)); }

  std::vector<int> getIntVector(std::string &name) const { return getIntVector(getNameIdNoAdd(name)); }

  std::vector<float> getRealVector(std::string &name) const { return getRealVector(getNameIdNoAdd(name)); }

  std::vector<Point2> getPoint2Vector(std::string &name) const { return getPoint2Vector(getNameIdNoAdd(name)); }

  std::vector<Point3> getPoint3Vector(std::string &name) const { return getPoint3Vector(getNameIdNoAdd(name)); }

  std::vector<Point4> getPoint4Vector(std::string &name) const { return getPoint4Vector(getNameIdNoAdd(name)); }

  std::vector<IPoint2> getIPoint2Vector(std::string &name) const { return getIPoint2Vector(getNameIdNoAdd(name)); }

  std::vector<IPoint3> getIPoint3Vector(std::string &name) const { return getIPoint3Vector(getNameIdNoAdd(name)); }

  std::vector<E3DCOLOR> getE3DColorVector(std::string &name) const {
    return getE3DColorVector(
        getNameIdNoAdd(name));
  }

  std::vector<TMatrix> getTMatrixVector(std::string &name) const { return getTMatrixVector(getNameIdNoAdd(name)); }


  //int setStr(std::string name, std::string value);
  //int setBool(std::string name, bool value);
  //int setInt(std::string name, int value);
  //int setReal(std::string name, float value);
  //int setPoint2(std::string name, const Point2 &value);
  //int setPoint3(std::string name, const Point3 &value);
  //int setPoint4(std::string name, const Point4 &value);
  //int setIPoint2(std::string name, const IPoint2 &value);
  //int setIPoint3(std::string name, const IPoint3 &value);
  //int setE3DColor(std::string name, const E3DCOLOR value);
  //int setTMatrix(std::string name, const TMatrix &value);

  int addStr(int name_id_, const std::string &value);

  int addStr(int name_id_, const std::string_view &value);


  int addBool(int name_id, bool value);

  int addInt(int name_id, int value);

  int addReal(int name_id, float value);

  int addPoint2(int name_id, const Point2 &value);

  int addPoint3(int name_id, const Point3 &value);

  int addPoint4(int name_id, const Point4 &value);

  int addIPoint2(int name_id, const IPoint2 &value);

  int addIPoint3(int name_id, const IPoint3 &value);

  int addE3DColor(int name_id, E3DCOLOR value);

  int addTMatrix(int name_id, const TMatrix &value);

  // getNameId
  int addStr(const std::string &name, const std::string &value) { return addStr(this->nm->getIdFromNameAdd(name), value); }

  int addBool(const std::string &name, bool value) { return addBool(this->nm->getIdFromNameAdd(name), value); }

  int addInt(const std::string &name, int value) { return addInt(this->nm->getIdFromNameAdd(name), value); }

  int addReal(const std::string &name, float value) { return addReal(this->nm->getIdFromNameAdd(name), value); }

  int addPoint2(const std::string &name, const Point2 &value) { return addPoint2(this->nm->getIdFromNameAdd(name), value); }

  int addPoint3(const std::string &name, const Point3 &value) { return addPoint3(this->nm->getIdFromNameAdd(name), value); }

  int addPoint4(const std::string &name, const Point4 &value) { return addPoint4(this->nm->getIdFromNameAdd(name), value); }

  int addIPoint2(const std::string &name, const IPoint2 &value) { return addIPoint2(this->nm->getIdFromNameAdd(name), value); }

  int addIPoint3(const std::string &name, const IPoint3 &value) { return addIPoint3(this->nm->getIdFromNameAdd(name), value); }

  int addE3DColor(const std::string &name, E3DCOLOR value) { return addE3DColor(this->nm->getIdFromNameAdd(name), value); }

  int addTMatrix(const std::string &name, const TMatrix &value) { return addTMatrix(this->nm->getIdFromNameAdd(name), value); }

  ///
  void printBlock(int spacing, std::basic_ostream<char> &out) const;


  /// Returns type of i-th parameter. See ParamType enum.
  int getParamType(int param_number) const;

  SharedPtr<Param> getParam(int param_number);

  /// Returns i-th parameter name id. See getNameId().
  int getParamNameId(int param_number) const;

  SharedPtr<Param> getParamByNameId(int name_id) const;

  /// gets the name id of a name
  int getIdFromName(const std::string &name);

  int getIdFromNameAdd(const std::string &name);

  /// adds a parameter to the Block, returns index
  int addParameter(int name_id, Param data);

  /// adds a parameter to the Block, returns index
  int addParameter(std::string &name, Param data) { return addParameter(getIdFromName(name), data); }

  // given a name map, will update all internal values to use that name map
  // after, sets this name map to nm
  bool updateNameMap(const std::shared_ptr<NameMap> &other);

  std::shared_ptr<NameMap> getNameMap()
  {
    return this->nm;
  }

  /// adds a block. if blk is a nullptr, creates a new block and adds it. returns the index of the block
  int addBlock(int name_id);

  int addBlock(SharedPtr<DataBlock> &blk);
  void addBlockInplace(SharedPtr<DataBlock> &blk, bool check, bool addParams);

  int addBlockUnsafe(SharedPtr<DataBlock> &blk);

  int addBlock(std::string &name) { return addBlock(getIdFromName(name)); }

  int addBlock(const char *name) { return addBlock(getIdFromName(name)); }

  SharedPtr<DataBlock> getAddBlock(int name_id);
  SharedPtr<DataBlock> getAddBlock(std::string &name) {return getAddBlock(getIdFromNameAdd(name));}
  SharedPtr<DataBlock> getAddBlock(const char *name) {return getAddBlock(getIdFromName(name));}

  // adds another block, copying its blocks and params to this block TODO: make it actually add new block
  void addBlockUpdate(SharedPtr<DataBlock> &blk);

  SharedPtr<DataBlock> getBlock(int param_index) const;

  SharedPtr<DataBlock> getBlockByName(int name_id) const;

  SharedPtr<DataBlock> getBlock(const std::string &name, int index);

  int getBlockNameId() const;

  uint8_t getParamType(int param_number) {
    if (param_number < 0 || param_number >= (int) this->params.size()) return -1;
    return this->params[param_number]->type;
  }

  std::vector<int> getParamIndexVector() const {
    std::vector<int> ret(this->param_count);
    for (int i = 0; i < (int) this->params.size(); i++) {
      if (this->params[i].get()->type != TYPE_NONE) {
        ret.push_back(i);
      }
    }
    return ret;
  }

  int getNameIdNoAdd(const std::string &name) const {
    return this->nm->GetIdFromName(name, true);
  }

  int getNameId(const std::string &name) const {
    return this->nm->GetIdFromName(name);
  }

  [[nodiscard]] std::string_view getNameFromId(int name_id_) const {
    G_ASSERT(name_id_ != -1);
    return this->nm->getNameFromId(name_id_);
  }

  [[nodiscard]] std::string_view getBlockName() const {
    if (this->name_id == -1) return "root";
    return getNameFromId(this->name_id);
  }

  inline int blockCount() const {
    return this->blocks.size();
  }

  inline int paramCount() const {
    return this->params.size();
  }

  const char *resolveFilename() const;

  void addNewBlock(const SharedPtr<DataBlock> &blk, const char *as_name) {
    if (!blk)
      return ;
    SharedPtr<DataBlock> newBlk = this->getBlock(addBlock(as_name ? as_name : blk->getBlockName().data()));
    G_ASSERT(newBlk);
    this->params.clear();
    for (const auto &param: newBlk->params) {
      this->addParam(param);
    }
    for (auto &block: newBlk->blocks) {
      this->addNewBlock(block, block->getBlockName().data());
    }
  }


  bool loadFromStream(GenReader &crd, const std::shared_ptr<NameMap> &names, ZSTD_DDict_s *zstd_dict);

  bool loadFromBinDump(GenReader &crd, const std::shared_ptr<NameMap> &names);

  bool loadText(std::span<char>& text);
  bool loadText(std::vector<char>& text)
  {
    std::span<char> spn{text};
    return this->loadText(spn);
  }
  ~DataBlock() {
    //std::cout << "DataBlock dtor " << this->getBlockName() << "\n";
    Destruct();
  }

  using isValidParamCallback = bool (*)(SharedPtr<Param>&);

  void cleanupParamsByCB(isValidParamCallback cb);


protected:


  bool saveBinToStream(BitStream &bs);

  void saveToTextFile(const std::string &path) const;

  static bool loadText(std::ifstream *in);


  void setShared(std::shared_ptr<DataBlockShared> &set) {
    this->shared = set;
  }
public:
  void Clear() {
    this->params.clear();
    this->blocks.clear();
    this->nm = std::make_shared<NameMap>();
    this->name_id = -1;
    this->param_count = 0;
    this->block_count = 0;
  }

  void Destruct() {
    this->params.clear();
    this->blocks.clear();
    this->nm.reset();
    this->shared.reset();
  }

  char * addOwnedData(char * data, size_t length) {
    auto offs = this->owned_data.addDataRaw((char *)data, length);
    return (char *)this->owned_data.getDataRawUnsafe(offs);
  }


  DataBlock(int prealloc_param, int prealloc_block);

  DataBlock(int prealloc_param, int prealloc_block, const std::shared_ptr<NameMap> &nm);

  static SharedPtr<DataBlock> makeAliasedBlock(const std::shared_ptr<DataBlockShared> &pool, DataBlock *ptr);

  void dumpBlocks(std::vector<DataBlock *> &blocks_, std::vector<DataBlockInfo> &blkinfo) const;

  void dumpParam() const;

  void printParams(const std::vector<Param> &params_to_print, std::basic_ostream<char> &out = std::cout) const;

  struct compiled_param {
    int name;
    uint8_t type;
    int data;
    int ptr;

    compiled_param() {
      name = -1;
      type = -1;
      data = -1;
      ptr = -1;
    }

    compiled_param(int name, uint8_t type, int data, int ptr) {
      this->name = name;
      this->type = type;
      this->data = data;
      this->ptr = ptr;
    }
  };


  void construct_param(ParamBin &p, Param *into);

  int addParam(const SharedPtr<Param> &param) {
    this->params.push_back(param);
    param->owner = this;
    this->param_count++;
    return (int) this->params.size() - 1;
  }
  int addParam(SharedPtr<Param> &param) {
    G_ASSERT(param->name_id != -1);
    this->params.push_back(param);
    this->param_count++;
    param->owner = this;
    return (int) this->params.size() - 1;
  }

  // assumes that no conflicting type and name_id
  [[nodiscard]] Param *getIndexedParam(int name_id_, int count, uint8_t type) const {
    int i = 0;
    for (const auto &p: params) {
      if (p->name_id == name_id_) {
        if (p->type == type) {
          if (i >= count) {
            return p.get();
          }
          i += 1;
        }
      }
    }
    return nullptr;
  }
protected:
  enum BLKTypes : uint8_t {
    BBF = 0x00,
    FAT = 0x01,
    FAT_ZSTD = 0x02,
    SLIM = 0x03,
    SLIM_ZSTD = 0x04,
    SLIM_ZSTD_DICT = 0x05,
  };

  struct BLKType {
    BLKTypes type;

    explicit BLKType(uint8_t input) {
      type = static_cast<BLKTypes>(input);
    }

    [[nodiscard]] bool is_slim() const {
      return type == SLIM || type == SLIM_ZSTD || type == SLIM_ZSTD_DICT;
    }

    [[nodiscard]] bool is_zstd() const {
      return type == FAT_ZSTD || type == SLIM_ZSTD || type == SLIM_ZSTD_DICT;
    }

    [[nodiscard]] bool needs_dict() const {
      return type == SLIM_ZSTD_DICT;
    }
  };


  std::vector<SharedPtr<Param>> params;

  std::vector<SharedPtr<DataBlock>> blocks;
  StringTableAllocator owned_data; // while this is made to store strings, its also a good way to store arbitrary data that wont be moved
  int name_id;
  int param_count;
  int block_count;
  std::shared_ptr<NameMap> nm;
  std::shared_ptr<DataBlockShared> shared;
  friend DataBlockShared;
  friend VROMFs;
};

class DecompressorObj {
private:
  NameMap nm;

};

struct DataBlockShared {
  uint32_t complexDataSize;
  uint32_t ParamCount;
  uint32_t BlockCount;
  uint32_t BlockOffset;
  char *data;
  std::string srcFilename; // src filename
  void setSrc(std::string src) { srcFilename = std::move(src); }

  const char *getSrc() const { return !srcFilename.empty() ? srcFilename.c_str() : nullptr; }

  DataBlockShared(uint32_t complexDataSize, uint32_t ParamCount, uint32_t BlockCount);

  [[nodiscard]] char *getComplex() const {
    return data; // complex data is first
  }

  [[nodiscard]] DataBlock::Param *getParams() const {
    return (DataBlock::Param *) (data + complexDataSize);
  }

  [[nodiscard]] DataBlock *getBlocks() const {
    return (DataBlock *) (data + BlockOffset);
  }

  [[nodiscard]] uint32_t getBlockCount() const {
    return BlockCount;
  }

  ~DataBlockShared();
};

bool load(DataBlock &blk, const char *fname);

#endif