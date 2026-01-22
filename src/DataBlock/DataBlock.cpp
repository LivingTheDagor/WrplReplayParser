

//#include <chrono>
#include "DataBlock.h"
#include <iostream>
#include <fstream>
#include "reader.h"
#include "FileSystem.h"

bool DataBlock::getStr(int param_number, std::string &out) const {
  if (this->params[param_number]->type != TYPE_STRING) return false;

  out = this->params[param_number]->data.s;
  return true;
}

bool DataBlock::getBool(int param_number, bool &out) const {
  if (this->params[param_number]->type != TYPE_BOOL) return false;
  out = this->params[param_number]->data.b;
  return true;
}

bool DataBlock::getInt(int param_number, int &out) const {
  if (this->params[param_number]->type != TYPE_INT) return false;
  out = this->params[param_number]->data.i;
  return true;
}

bool DataBlock::getReal(int param_number, float &out) const {
  if (this->params[param_number]->type != TYPE_REAL) return false;
  out = this->params[param_number]->data.r;
  return true;
}

bool DataBlock::getPoint2(int param_number, Point2 &out) const {
  if (this->params[param_number]->type != TYPE_POINT2) return false;
  out = this->params[param_number]->data.p2;
  return true;
}

bool DataBlock::getPoint3(int param_number, Point3 &out) const {
  if (this->params[param_number]->type != TYPE_POINT3) return false;
  out = this->params[param_number]->data.p3;
  return true;
}

bool DataBlock::getPoint4(int param_number, Point4 &out) const {
  if (this->params[param_number]->type != TYPE_POINT4) return false;
  out = this->params[param_number]->data.p4;
  return true;
}

bool DataBlock::getIPoint2(int param_number, IPoint2 &out) const {
  if (this->params[param_number]->type != TYPE_IPOINT2) return false;
  out = this->params[param_number]->data.ip2;
  return true;
}

bool DataBlock::getIPoint3(int param_number, IPoint3 &out) const {
  if (this->params[param_number]->type != TYPE_IPOINT3) return false;
  out = this->params[param_number]->data.ip3;
  return true;
}

bool DataBlock::getE3DColor(int param_number, E3DCOLOR &out) const {
  if (this->params[param_number]->type != TYPE_E3DCOLOR) return false;
  out = this->params[param_number]->data.c;
  return true;
}

bool DataBlock::getTMatrix(int param_number, TMatrix &out) const {
  if (this->params[param_number]->type != TYPE_MATRIX) return false;
  out = this->params[param_number]->data.tm;
  return true;
}

bool DataBlock::getStr(const std::string &name, std::string &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_STRING);
  if (obj) {
    out = obj->data.s;
    return true;
  }
  return false;
}

bool DataBlock::getBool(const std::string &name, bool &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_BOOL);
  if (obj) {
    out = obj->data.b;
    return true;
  }
  return false;
}

bool DataBlock::getInt(const std::string &name, int &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_INT);
  if (obj) {
    out = obj->data.i;
    return true;
  }
  return false;
}

bool DataBlock::getReal(const std::string &name, float &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_REAL);
  if (obj) {
    out = obj->data.r;
    return true;
  }
  return false;
}

bool DataBlock::getPoint2(const std::string &name, Point2 &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_POINT2);
  if (obj) {
    out = obj->data.p2;
    return true;
  }
  return false;
}

bool DataBlock::getPoint3(const std::string &name, Point3 &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_POINT3);
  if (obj) {
    out = obj->data.p3;
    return true;
  }
  return false;
}

bool DataBlock::getPoint4(const std::string &name, Point4 &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_POINT4);
  if (obj) {
    out = obj->data.p4;
    return true;
  }
  return false;
}

bool DataBlock::getIPoint2(const std::string &name, IPoint2 &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_IPOINT2);
  if (obj) {
    out = obj->data.ip2;
    return true;
  }
  return false;
}

bool DataBlock::getIPoint3(const std::string &name, IPoint3 &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_IPOINT3);
  if (obj) {
    out = obj->data.ip3;
    return true;
  }
  return false;
}

bool DataBlock::getE3DColor(const std::string &name, E3DCOLOR &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_E3DCOLOR);
  if (obj) {
    out = obj->data.c;
    return true;
  }
  return false;
}

bool DataBlock::getTMatrix(const std::string &name, TMatrix &out, int index) const {
  int name_id_ = getNameIdNoAdd(name);
  Param *obj = getIndexedParam(name_id_, index, TYPE_MATRIX);
  if (obj) {
    out = obj->data.tm;
    return true;
  }
  return false;
}

/*DLLEXPORT*/ const char *DataBlock::getStr(int i) const {
  if (i < 0 || i >= params.size())
    return NULL;
  if (params[i]->type != TYPE_STRING)
    return NULL;
  return params[i]->data.s.data();
}

/*DLLEXPORT*/ int DataBlock::getInt(int i) const {
  if (i < 0 || i >= params.size())
    return 0;
  if (params[i]->type != TYPE_INT)
    return 0;
  return params[i]->data.i;
}

/*DLLEXPORT*/ bool DataBlock::getBool(int i) const {
  if (i < 0 || i >= params.size())
    return false;
  if (params[i]->type != TYPE_BOOL)
    return false;
  return params[i]->data.b;
}

/*DLLEXPORT*/ real DataBlock::getReal(int i) const {
  if (i < 0 || i >= params.size())
    return 0;
  if (params[i]->type != TYPE_REAL)
    return 0;
  return params[i]->data.r;
}

Point2 DataBlock::getPoint2(int i) const {
  if (i < 0 || i >= params.size())
    return Point2(0.f, 0.f);
  if (params[i]->type != TYPE_POINT2)
    return Point2(0.f, 0.f);
  return params[i]->data.p2;
}

Point3 DataBlock::getPoint3(int i) const {
  if (i < 0 || i >= params.size())
    return Point3(0.f, 0.f, 0.f);
  if (params[i]->type != TYPE_POINT3)
    return Point3(0.f, 0.f, 0.f);
  return params[i]->data.p3;
}

Point4 DataBlock::getPoint4(int i) const {
  if (i < 0 || i >= params.size())
    return Point4(0.f, 0.f, 0.f, 0.f);
  if (params[i]->type != TYPE_POINT4)
    return Point4(0.f, 0.f, 0.f, 0.f);
  return params[i]->data.p4;
}

IPoint2 DataBlock::getIPoint2(int i) const {
  if (i < 0 || i >= params.size())
    return IPoint2(0.f, 0.f);
  if (params[i]->type != TYPE_IPOINT2)
    return IPoint2(0.f, 0.f);
  return params[i]->data.ip2;
}

IPoint3 DataBlock::getIPoint3(int i) const {
  if (i < 0 || i >= params.size())
    return IPoint3(0.f, 0.f, 0.f);
  if (params[i]->type != TYPE_IPOINT3)
    return IPoint3(0.f, 0.f, 0.f);
  return params[i]->data.ip3;
}

/*DLLEXPORT*/ E3DCOLOR DataBlock::getE3DColor(int i) const {
  if (i < 0 || i >= params.size())
    return E3DCOLOR(0, 0, 0, 0);
  if (params[i]->type != TYPE_E3DCOLOR)
    return E3DCOLOR(0, 0, 0, 0);
  return params[i]->data.c;
}


TMatrix DataBlock::getTMatrix(int i) const {
  if (i < 0 || i >= params.size())
    return TMatrix::IDENT;
  if (params[i]->type != TYPE_MATRIX)
    return TMatrix::IDENT;
  return params[i]->data.tm;
}

uint64_t DataBlock::getUInt64(int i) const {
  if (i < 0 || i >= params.size())
    return 0;
  if (params[i]->type != TYPE_UINT64)
    return 0;
  return params[i]->data.u64;
}

DataBlock::DataBlock() {
  this->name_id = -1;
  this->param_count = 0;
  this->block_count = 0;
  this->nm = std::make_shared<NameMap>();
  //std::cout << "DataBlock ctor 1 " << this->getBlockName() << "\n";
}

DataBlock::DataBlock(int name_id, int prealloc_param, int prealloc_block, const std::shared_ptr<NameMap> &nm) {
  this->name_id = name_id;
  this->param_count = 0;
  this->block_count = 0;
  this->params.reserve(prealloc_param);
  this->blocks.reserve(prealloc_block);
  this->nm = nm;
  //std::cout << "DataBlock ctor 2 " << this->getBlockName() << "\n";
}

DataBlock::DataBlock(const std::shared_ptr<NameMap> &nm) {
  this->name_id = -1;
  this->param_count = 0;
  this->block_count = 0;
  this->nm = nm;
  //std::cout << "DataBlock ctor 3 " << this->getBlockName() << "\n";
}

DataBlock::DataBlock(int prealloc_param, int prealloc_block) {
  this->params.reserve(prealloc_param);
  this->blocks.reserve(prealloc_block);
  this->name_id = -1;
  this->param_count = 0;
  this->block_count = 0;
  this->nm = std::make_shared<NameMap>(NameMap());
  //std::cout << "DataBlock ctor 4 " << this->getBlockName() << "\n";
}

DataBlock::DataBlock(int prealloc_param, int prealloc_block, const std::shared_ptr<NameMap> &nm) {
  this->params.reserve(prealloc_param);
  this->blocks.reserve(prealloc_block);
  this->name_id = -1;
  this->param_count = 0;
  this->block_count = 0;
  this->nm = nm;
  //std::cout << "DataBlock ctor 5 " << this->getBlockName() << "\n";
}


int DataBlock::addStr(int name_id_, const std::string &value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, std::string_view(this->addOwnedData((char *)value.c_str(), value.size()+1)));
  return addParam(p);
}

int DataBlock::addStr(int name_id_, const std::string_view &value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addBool(int name_id_, bool value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addInt(int name_id_, int value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addReal(int name_id_, float value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addPoint2(int name_id_, const Point2 &value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addPoint3(int name_id_, const Point3 &value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addPoint4(int name_id_, const Point4 &value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addIPoint2(int name_id_, const IPoint2 &value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addIPoint3(int name_id_, const IPoint3 &value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addE3DColor(int name_id_, const E3DCOLOR value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addTMatrix(int name_id_, const TMatrix &value) {
  this->param_count += 1;
  auto p = SharedPtr<Param>::make(name_id_, value);
  return addParam(p);
}

int DataBlock::addBlock(int name_id_) {

  SharedPtr<DataBlock> blk = SharedPtr<DataBlock>::make(this->nm);
  blk->name_id = name_id_;
  this->blocks.emplace_back(blk);
  this->block_count++;
  return this->block_count - 1;
}


int DataBlock::addBlock(SharedPtr<DataBlock> &blk) {
  blk->updateNameMap(this->nm);
  this->blocks.emplace_back(blk);
  this->block_count++;
  return this->block_count - 1;
}

int DataBlock::addBlockUnsafe(SharedPtr<DataBlock> &blk) {
  this->blocks.emplace_back(blk);
  this->block_count++;
  return this->block_count - 1;
}

bool DataBlock::updateNameMap(const std::shared_ptr<NameMap> &other) {
  if((this->nm == other))
    return true; // already same
  if (!(this->nm == other)) {
    for (auto &p: this->params) {

      if (p->type == TYPE_NONE) continue;
      std::string_view name = this->getNameFromId(p->name_id);
      p->name_id = other->GetIdFromName(name);
    }
  }
  for (auto &b: this->blocks) {
    std::string_view name = this->getNameFromId(b->name_id);
    if (!(this->nm == other))
      b->name_id = other->GetIdFromName(name);
    b->updateNameMap(other);
  }
  this->nm = other;
  return true;
}

void printIndent(int level, std::basic_ostream<char> &out) {
  for (int i = 0; i < level; i++) {
    out << " ";
  }
}


void DataBlock::printBlock(int indent, std::basic_ostream<char> &out) const {
  for (auto &p: params) {
    if (!p) continue;
    printIndent(indent * 2, out);
    if(p->name_id == -1)
    {
      G_ASSERT(false);
      out << "UNKOWN_NAME:";
    }
    else
    {
      out << this->getNameFromId(p->name_id) << ":";
    }

    switch (p->type) {

      case TYPE_STRING:
        out << fmt::format("t=\"{}\"\n", p->data.s);
        break;
      case TYPE_BOOL: {
        out << fmt::format("b={}\n", p->data.b ? "yes" : "no");
        break;
      }
      case TYPE_INT: {
        out << fmt::format("i={}\n", p->data.i);
        break;
      }
      case TYPE_REAL: {
        out << fmt::format("r={}\n", p->data.r);
        break;
      }
      case TYPE_POINT2: {
        out << fmt::format("p2={}, {}\n", p->data.p2.x, p->data.p2.y);
        break;
      }
      case TYPE_POINT3: {
        out << fmt::format("p3={}, {}, {}\n", p->data.p3.x, p->data.p3.y, p->data.p3.z);
        break;
      }
      case TYPE_POINT4: {
        out << fmt::format("p4={}, {}, {}, {}\n", p->data.p4.x, p->data.p4.y, p->data.p4.z, p->data.p4.w);
        break;
      }
      case TYPE_IPOINT2: {
        out << fmt::format("ip2={}, {}\n", p->data.ip2.x, p->data.ip2.y);
        break;
      }
      case TYPE_IPOINT3: {
        out << fmt::format("ip3={}, {}, {}\n", p->data.ip3.x, p->data.ip3.y, p->data.ip3.z);
        break;
      }
      case TYPE_E3DCOLOR: {
        out << fmt::format("c={}, {}, {}, {}\n", p->data.c.r, p->data.c.g, p->data.c.b, p->data.c.a);
        break;
      }
      case TYPE_MATRIX: {
        out << fmt::format("m=[[{}, {}, {}] [{}, {}, {}] [{}, {}, {}] [{}, {}, {}]]\n", p->data.tm.m[0][0], p->data.tm[0][1],
                           p->data.tm[0][2], p->data.tm[1][0], p->data.tm[1][1], p->data.tm[1][2], p->data.tm[2][0],
                           p->data.tm[2][1], p->data.tm[2][2], p->data.tm[3][0], p->data.tm[3][1], p->data.tm[3][2]);
        break;
      }
      case TYPE_UINT64: {
        out << fmt::format("i64={}\n", p->data.u64);
        break;
      }
    }
  }
  for (auto &b: this->blocks) {
    if (!b) continue;
    printIndent(indent * 2, out);
    out << b->getBlockName() << "{\n";
    b->printBlock(indent + 1, out);
    printIndent(indent * 2, out);
    out << "}\n";
  }
}

void fwrite(const char *w, size_t size, [[maybe_unused]] size_t count, std::ostream *cb) {
  cb->write(w, (std::streamsize) size);
}

static void writeIndent(std::ostream *cb, int n) {
  if (n <= 0)
    return;
  for (; n >= 8; n -= 8)
    fwrite("        ", 8, 1, cb);
  for (; n >= 2; n -= 2)
    fwrite("  ", 2, 1, cb);
  for (; n >= 1; n--)
    fwrite(" ", 1, 1, cb);
}

static void writeString(std::ostream *cb, const char *s) {
  if (!s || !*s)
    return;
  int l = (int) strlen(s);
  fwrite(s, l, 1, cb);
}

static void writeString(std::ostream *cb, const char *s, int len) {
  if (!s || !*s)
    return;
  fwrite(s, len, 1, cb);
}

static void writeStringValue(std::ostream *cb, const char *s) {
  if (!s)
    s = "";

  const char *quot = "\"";
  int quot_len = 1;
  {
    // choose quotation type to minimize escaping
    bool has_ln = false, has_quot = false, has_tick = false;
    for (const char *p = s; *p; ++p)
    {
      if (*p == '\n' || *p == '\r')
        has_ln = true;
      else if (*p == '\"')
        has_quot = true;
      else if (*p == '\'')
        has_tick = true;
      else
        continue;
      if (has_ln && has_quot && has_tick)
        break;
    }
    if (has_ln && !has_quot)
      quot_len = 4, quot = "\"\"\"\n\"\"\"";
    else if (has_ln && !has_tick)
      quot_len = 4, quot = "'''\n'''";
    else if (has_ln)
      quot_len = 4, quot = "\"\"\"\n\"\"\"";
    else if (has_quot && !has_tick)
      quot = "'";
  }

  cb->write(quot, quot_len);

  for (; *s; ++s)
  {
    char c = *s;
    if (c == '~')
      cb->write("~~", 2);
    else if (c == quot[0] && (quot_len == 1 || s[1] == c))
      cb->write(c == '\"' ? "~\"" : "~\'", 2);
    else if (c == '\r' && quot_len == 1)
      cb->write("~r", 2);
    else if (c == '\n' && quot_len == 1)
      cb->write("~n", 2);
    else if (c == '\t')
      cb->write("~t", 2);
    else
      cb->write(&c, 1);
  }

  cb->write(quot + quot_len - 1, quot_len);
}

/*DLLEXPORT*/ void DataBlock::saveText(std::ostream *cb, int level) const {
  int i;
  for (i = 0; i < (int) params.size(); ++i) {
    Param p = *params[i].get();

    writeIndent(cb, level * 2);
    writeString(cb, this->getNameFromId(p.name_id).data());

    char buf[256];
    switch (p.type) {
      case TYPE_STRING:
        writeString(cb, ":t=");
        writeStringValue(cb, p.data.s.data());
        break;
      case TYPE_BOOL: {
        writeString(cb, ":b=");
        sprintf(buf, "%s", p.data.b ? "yes" : "no");
        writeString(cb, buf);
      }
        break;
      case TYPE_INT: {
        writeString(cb, ":i=");
        sprintf(buf, "%d", p.data.i);
        writeString(cb, buf);
      }
        break;
      case TYPE_REAL: {
        writeString(cb, fmt::format(":r={}", p.data.r).c_str());
      }
        break;
      case TYPE_POINT2: {
        writeString(cb, fmt::format(":p2={}, {}", p.data.p2.x, p.data.p2.y).c_str());
      }
        break;
      case TYPE_POINT3: {
        writeString(cb, fmt::format(":p3={}, {}, {}", p.data.p3.x, p.data.p3.y, p.data.p3.z).c_str());
      }
        break;
      case TYPE_POINT4: {
        writeString(cb, fmt::format(":p4={}, {}, {}, {}\n", p.data.p4.x, p.data.p4.y, p.data.p4.z, p.data.p4.w).c_str());
      }
        break;
      case TYPE_IPOINT2: {
        writeString(cb, ":ip2=");
        sprintf(buf, "%d, %d", p.data.ip2.x, p.data.ip2.y);
        writeString(cb, buf);
      }
        break;
      case TYPE_IPOINT3: {
        writeString(cb, ":ip3=");
        sprintf(buf, "%d, %d, %d", p.data.ip3.x, p.data.ip3.y, p.data.ip3.z);
        writeString(cb, buf);
      }
        break;
      case TYPE_E3DCOLOR: {
        writeString(cb, ":c=");
        sprintf(buf, "%d, %d, %d, %d", p.data.c.r, p.data.c.g, p.data.c.b, p.data.c.a);
        writeString(cb, buf);
      }
        break;
      case TYPE_MATRIX: {
        writeString(cb, fmt::format(":m=[[{}, {}, {}] [{}, {}, {}] [{}, {}, {}] [{}, {}, {}]]\n", p.data.tm.m[0][0], p.data.tm[0][1],
                                    p.data.tm[0][2], p.data.tm[1][0], p.data.tm[1][1], p.data.tm[1][2], p.data.tm[2][0],
                                    p.data.tm[2][1], p.data.tm[2][2], p.data.tm[3][0], p.data.tm[3][1], p.data.tm[3][2]).c_str());
        break;
      }
      case TYPE_UINT64: {

        writeString(cb, ":i64=");
        snprintf(buf, sizeof(buf), "%lld", (long long int) p.data.u64);
        writeString(cb, buf);
      }
        break;

    }
    fwrite("\n", 1, 1, cb);
  }

  if (params.size() && blocks.size()) {
    writeIndent(cb, level * 2);
    fwrite("\n", 1, 1, cb);
  }
  for (i = 0; i < (int) blocks.size(); ++i) {
    DataBlock &b = *blocks[i];

    writeIndent(cb, level * 2);
    writeString(cb, getNameFromId(b.name_id).data());
    fwrite("{\n", 2, 1, cb);

    b.saveText(cb, level + 1);

    writeIndent(cb, level * 2);
    fwrite("}\n", 2, 1, cb);

    if (i != (int) blocks.size() - 1)
      fwrite("\n", 1, 1, cb);
  }
}

#define COMMENT_NAME_PREFIX     "@\xC"
#define COMMENT_NAME_PREFIX_LEN 2

#define COMMENT_PRE_SUFFIX_CPP  "\x1"  // CPP-style comment before commented statement
#define COMMENT_PRE_SUFFIX_C    "\x2"  // C-style comment before commented statement
#define COMMENT_POST_SUFFIX_CPP "\x11" // CPP-style comment after commented statement (same line)
#define COMMENT_POST_SUFFIX_C   "\x12" // C-style comment after commented statement (same line)

#define COMMENT_PRE_CPP  COMMENT_NAME_PREFIX COMMENT_PRE_SUFFIX_CPP
#define COMMENT_PRE_C    COMMENT_NAME_PREFIX COMMENT_PRE_SUFFIX_C
#define COMMENT_POST_CPP COMMENT_NAME_PREFIX COMMENT_POST_SUFFIX_CPP
#define COMMENT_POST_C   COMMENT_NAME_PREFIX COMMENT_POST_SUFFIX_C

#define CHECK_COMMENT_PREFIX(NM) strncmp(NM, COMMENT_NAME_PREFIX, COMMENT_NAME_PREFIX_LEN) == 0

#define IS_COMMENT_PRE_CPP(NM) ((NM)[2] == *COMMENT_PRE_SUFFIX_CPP)
#define IS_COMMENT_PRE_C(NM)   ((NM)[2] == *COMMENT_PRE_SUFFIX_C)
#define IS_COMMENT_POST(NM)    (((NM)[2] == *COMMENT_POST_SUFFIX_CPP) || ((NM)[2] == *COMMENT_POST_SUFFIX_C))
#define IS_COMMENT_CPP(NM)     (((NM)[2] == *COMMENT_POST_SUFFIX_CPP) || ((NM)[2] == *COMMENT_PRE_SUFFIX_CPP))
#define IS_COMMENT_C(NM)       (((NM)[2] == *COMMENT_POST_SUFFIX_C) || ((NM)[2] == *COMMENT_PRE_SUFFIX_C))


static inline bool is_ident_char(char c)
{
  return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || (c >= 'A' && c <= 'Z') || c == '-' || c == '.' || c == '~');
}

static inline const char *resolve_short_type(uint32_t type)
{
  static const char *types[DataBlock::TYPE_COUNT + 1] = {
      "none", "t", "i", "r", "p2", "p3", "p4", "ip2", "ip3", "b", "c", "m", "i64", "err"};
  G_ASSERT(type < DataBlock::TYPE_COUNT+1);
  return types[type];
}


bool DataBlock::writeText(std::ostream *cb, int level) const
{
  const bool print_with_limits = false;
  const char *eol = (level >= 0 && !print_with_limits) ? "\r\n" : "\n";
  const uint32_t eolLen = (level >= 0 && !print_with_limits) ? 2 : 1;


  int skipped_comments = 0;
  bool skip_next_indent = false;
  for (const auto& pp : this->params)
  {
    const auto &p = *pp;
    bool isCIdent = true;
    const char *keyName = this->getNameFromId(p.name_id).data();
    if (!keyName)
      keyName = "@null";
    if (!*keyName)
      isCIdent = false;
    for (const char *c = keyName; *c && isCIdent; ++c)
      isCIdent = is_ident_char(*c);

    if (DAGOR_LIKELY(!false) && p.type == TYPE_STRING && CHECK_COMMENT_PREFIX(keyName))
    {
      skipped_comments++;
      continue;
    }

    if (level > 0)
    {
      if (!skip_next_indent)
        writeIndent(cb, level * 2);
      else
        skip_next_indent = false;
    }
    if (p.type == TYPE_STRING && keyName[0] == '@')
    {
      if (strcmp(keyName, "@include") == 0)
      {
        writeString(cb, "include ", 8);
        writeStringValue(cb, p.data.s.data());
        writeString(cb, eol, eolLen);
        continue;
      }
      else if (DAGOR_UNLIKELY(CHECK_COMMENT_PREFIX(keyName)))
      {
        if (IS_COMMENT_C(keyName))
        {
          cb->write("/*", 2);
          writeString(cb, p.data.s.data());
          cb->write("*/", 2);
          cb->write(eol, eolLen);
        }
        if (IS_COMMENT_CPP(keyName))
        {
          cb->write("//", 2);
          writeString(cb, p.data.s.data());
          cb->write(eol, eolLen);
        }
        continue;
      }
    }
    if (isCIdent)
    {
      writeString(cb, keyName);
    }
    else
    {
      writeStringValue(cb, keyName);
    }
    writeString(cb, ":");
    writeString(cb, resolve_short_type(p.type));
    writeString(cb, "=");
    if (p.type == TYPE_STRING)
    {
      writeStringValue(cb, p.data.s.data()); // getParamString<string_t>(p.v)
    }
    else
    {
      //const char *paramData = &p.data.data_ptr;
      //const int *cdi = (const int *)paramData;
      //const int64_t *cdi64 = (const int64_t *)paramData;
      //const float *cd = (const float *)paramData;
      std::string out;
      switch (p.type) {

        case TYPE_STRING:
          out = fmt::format("\"{}\"\n", p.data.s);
          break;
        case TYPE_BOOL: {
          out = fmt::format("{}\n", p.data.b ? "yes" : "no");
          break;
        }
        case TYPE_INT: {
          out = fmt::format("{}\n", p.data.i);
          break;
        }
        case TYPE_REAL: {
          out = fmt::format("{}\n", p.data.r);
          break;
        }
        case TYPE_POINT2: {
          out = fmt::format("{}, {}\n", p.data.p2.x, p.data.p2.y);
          break;
        }
        case TYPE_POINT3: {
          out = fmt::format("{}, {}, {}\n", p.data.p3.x, p.data.p3.y, p.data.p3.z);
          break;
        }
        case TYPE_POINT4: {
          out = fmt::format("{}, {}, {}, {}\n", p.data.p4.x, p.data.p4.y, p.data.p4.z, p.data.p4.w);
          break;
        }
        case TYPE_IPOINT2: {
          out = fmt::format("{}, {}\n", p.data.ip2.x, p.data.ip2.y);
          break;
        }
        case TYPE_IPOINT3: {
          out = fmt::format("{}, {}, {}\n", p.data.ip3.x, p.data.ip3.y, p.data.ip3.z);
          break;
        }
        case TYPE_E3DCOLOR: {
          out = fmt::format("{}, {}, {}, {}\n", p.data.c.r, p.data.c.g, p.data.c.b, p.data.c.a);
          break;
        }
        case TYPE_MATRIX: {
          out = fmt::format("[[{}, {}, {}] [{}, {}, {}] [{}, {}, {}] [{}, {}, {}]]\n", p.data.tm.m[0][0],
                            p.data.tm[0][1],
                            p.data.tm[0][2], p.data.tm[1][0], p.data.tm[1][1], p.data.tm[1][2], p.data.tm[2][0],
                            p.data.tm[2][1], p.data.tm[2][2], p.data.tm[3][0], p.data.tm[3][1], p.data.tm[3][2]);
          break;
        }
        case TYPE_UINT64: {
          out = fmt::format("{}\n", p.data.u64);
          break;
        }
        default:
          G_ASSERT(0);
      }
      writeString(cb, out.c_str());
    }
    if (level < 0 && paramCount() == 1 && blockCount() == 0)
      cb->write(";", 1);
    else
    {

      cb->write(eol, eolLen);
    }
  }

  bool params_blocks_sep = false;
  for (uint32_t i = 0, e = blockCount(); i < e; ++i)
  {
    const DataBlock &b = *getBlock(i);
    bool isCIdent = true;
    const char *blkName = b.getNameFromId(b.getBlockNameId()).data();
    if (!blkName)
      blkName = "@null";
    if (DAGOR_UNLIKELY(CHECK_COMMENT_PREFIX(blkName)))
    {
      if (DAGOR_LIKELY(!false))
        continue;
      if (level > 0)
      {
        if (!params_blocks_sep && paramCount() > skipped_comments)
        {
          params_blocks_sep = true;
          cb->write(eol, eolLen);
        }
        writeIndent(cb, level * 2);
      }

      if (IS_COMMENT_PRE_C(blkName))
      {
        cb->write("/*", 2);
        writeString(cb, b.getStr(0));
        cb->write("*/", 2);
        cb->write(eol, eolLen);
      }
      else if (IS_COMMENT_PRE_CPP(blkName))
      {
        cb->write("//", 2);
        writeString(cb, b.getStr(0));
        cb->write(eol, eolLen);
      }
      continue;
    }

    if (level >= 0 && !params_blocks_sep && paramCount() > skipped_comments)
    {
      params_blocks_sep = true;
      cb->write(eol, eolLen);
    }
    if (level > 0)
      writeIndent(cb, level * 2);

    if (!*blkName)
      isCIdent = false;
    for (const char *c = blkName; *c && isCIdent; ++c)
      isCIdent = is_ident_char(*c);
    if (isCIdent)
    {
      writeString(cb, blkName);
    }
    else
    {
      writeStringValue(cb, blkName);
    }
    if (b.blockCount()+b.paramCount()==0)
    {
      cb->write("{}", 2);
      cb->write(eol, eolLen);
      continue;
    }

    cb->write("{", 1);
    if (!(level < 0 && b.paramCount() == 1 && b.blockCount() == 0))
    {
      cb->write(eol, eolLen);
    }

    if (
        !b.writeText(cb, level >= 0 ? level + 1 : level))
      return false;
    if (level > 0)
      writeIndent(cb, level * 2);
    cb->write("}", 1);
    cb->write(eol, eolLen);

    if (i != e - 1 && level >= 0)
    {
      cb->write(eol, eolLen);
    }
  }
#undef CHECK_LINES_LIMIT
  return true;
}


int DataBlock::getIdFromName(const std::string &name) {
  return this->nm->getIdFromNameAdd(name);
}

SharedPtr<DataBlock> DataBlock::getBlock(const std::string &name, int index) {
  int nid = this->getIdFromName(name);
  int c = 0;
  for (auto &b: this->blocks) {
    if (!b) continue;
    if (b->name_id == nid) {
      if (c >= index)
        return b;
      c++;
    }
  }
  return nullptr;
}

SharedPtr<DataBlock> DataBlock::getBlock(int block_id) const {
  return this->blocks[block_id];
}

SharedPtr<DataBlock> DataBlock::getBlockByName(int name_id) const {
  for (auto blk: this->blocks) {
    if (blk->name_id == name_id) {
      return blk;
    }
  }
  return nullptr;
}

/*DLLEXPORT*/ const char *DataBlock::getStr(const char *name, const char *ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_STRING)
    return ref;
  return params[i]->data.s.data();
}

/*DLLEXPORT*/ int DataBlock::getInt(const char *name, int ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_INT)
    return ref;
  return params[i]->data.i;
}

/*DLLEXPORT*/ bool DataBlock::getBool(const char *name, bool ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_BOOL)
    return ref;
  return params[i]->data.b;
}

/*DLLEXPORT*/ real DataBlock::getReal(const char *name, real ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_REAL)
    return ref;
  return params[i]->data.r;
}

Point2 DataBlock::getPoint2(const char *name, const Point2 &ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_POINT2)
    return ref;
  return params[i]->data.p2;
}

Point3 DataBlock::getPoint3(const char *name, const Point3 &ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_POINT3)
    return ref;
  return params[i]->data.p3;
}

Point4 DataBlock::getPoint4(const char *name, const Point4 &ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_POINT4)
    return ref;
  return params[i]->data.p4;
}

IPoint2 DataBlock::getIPoint2(const char *name, const IPoint2 &ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_IPOINT2)
    return ref;
  return params[i]->data.ip2;
}

IPoint3 DataBlock::getIPoint3(const char *name, const IPoint3 &ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_IPOINT3)
    return ref;
  return params[i]->data.ip3;
}

/*DLLEXPORT*/ E3DCOLOR DataBlock::getE3dcolor(const char *name, const E3DCOLOR &ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_E3DCOLOR)
    return ref;
  return params[i]->data.c;
}


TMatrix DataBlock::getTm(const char *name, const TMatrix &ref) const {
  int i = findParam(name);
  if (i < 0)
    return ref;
  if (params[i]->type != TYPE_MATRIX)
    return ref;
  return params[i]->data.tm;
}

bool DataBlock::getBoolByNameId(int paramNameId, bool def) const {
  for (const auto &param: this->params) {
    if (param->name_id == paramNameId)
      return param->data.b;
  }
  return def;
}

int DataBlock::getBlockNameId() const {
  return name_id;
}

const char *DataBlock::resolveFilename() const {
  if (shared) {
    const char *s = shared->getSrc();
    return (s && *s) ? s : "unknown";
  }
  return nullptr;
}

int DataBlock::getParamType(int param_number) const {
  auto p = this->params[param_number];
  if (p) {
    return p->type;
  }
  return 0;
}

int DataBlock::getParamNameId(int param_number) const {
  auto p = this->params[param_number];
  if (p) {
    return p->name_id;
  }
  return 0;
}

void DataBlock::addBlockUpdate(SharedPtr<DataBlock> &blk) {
  for (const auto &param : blk->params)
  {
    SharedPtr<Param> new_param;
    auto nid = this->getNameId(std::string(blk->getNameFromId(param->name_id)));
    new_param->data = param->data;
    new_param->type = param->type;
    new_param->name_id = nid;
    this->addParam(new_param);
  }
  for (auto &blks : blk->blocks)
  {
    this->addBlockUnsafe(blks); // TODO
  }
}

SharedPtr<DataBlock::Param> DataBlock::getParam(int param_number) {
  auto p = this->params[param_number];
  return p;
}

void DataBlock::addBlockInplace(SharedPtr<DataBlock> &blk, bool check, bool addParams) {
  blk->updateNameMap(this->nm);
  if(addParams)
  {
    for(auto &param : blk->params)
    {
      SharedPtr<Param> old;
      if (check && (old = this->getParamByNameId(param->name_id), old) && &old->data.data_ptr == &param->data.data_ptr)
        continue;
      this->addParam(param);
    }
  }
  for(auto &block : blk->blocks)
  {
    //auto new_blk = this->getAddBlock(block->getBlockNameId());
    this->addBlockUnsafe(block);
  }
}

SharedPtr<DataBlock> DataBlock::getAddBlock(int name_id) {
  auto blk = this->getBlockByName(name_id);
  if(blk)
    return blk;
  return this->getBlock(this->addBlock(name_id));
}

SharedPtr<DataBlock::Param> DataBlock::getParamByNameId(int name_id) const {
  for (auto & param : this->params)
  {
    if(param->name_id == name_id)
      return param;
  }
  return nullptr;
}

int DataBlock::getIdFromNameAdd(const std::string &name) {
  return this->nm->getIdFromNameAdd(name);
}

void DataBlock::cleanupParamsByCB(DataBlock::isValidParamCallback cb) {
  this->params.erase(std::remove_if(this->params.begin(), this->params.end(), cb), this->params.end());
  for(auto &blk : this->blocks)
    blk->cleanupParamsByCB(cb);
}

bool load(DataBlock &blk, const char *fname) {
  auto file = file_mgr.getFile(fname, true);
  if (file) {
    //LOG("Loading BLK at path: %s", fname);
    return file->loadBlk(blk);
  }
  return false;
}