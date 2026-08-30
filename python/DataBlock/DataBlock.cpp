#include "modules/DataBlock/DataBlock.h"
#include "modules/Math.h"
#include "modules/BitStream.h"
#include "ioSys/dag_dataBlock.h"
#include "ioSys/dag_memIo.h"
#include "math/dag_Point2.h"
#include "math/dag_Point3.h"
#include "math/dag_Point4.h"
#include "math/integer/dag_IPoint2.h"
#include "math/integer/dag_IPoint3.h"
#include "math/integer/dag_IPoint4.h"
#include "math/dag_TMatrix.h"
#include "math/dag_e3dColor.h"
#include "modules/utf8_err_ignore_string.h"

struct DataBlockRWObj : DataBlockRW {
  DataBlock blk;
  DataBlock *operator->() { return &blk; }
  DataBlock &operator*() { return blk; }
  DataBlock *ptr() { return &blk; }
  const DataBlock *operator->() const { return &blk; }
  const DataBlock &operator*() const { return blk; }
  const DataBlock *ptr() const { return &blk; }
  DataBlockRWObj() : DataBlockRW(&blk) {}
  DataBlockRWObj(const std::string &path) : DataBlockRW(&blk), blk(path.c_str()) {}
  DataBlockRWObj(const char *path) : DataBlockRW(&blk), blk(path) {}
};

using namespace pybind11::literals;

void addObjectImpl(py::dict &dict, py::object obj, const std::string &name) {
  if (dict.contains(name)) {
    py::object v = dict[name.c_str()]; // If operator[] isn't available, use .attr("get")
    if (py::isinstance<py::list>(v)) {
      auto l = v.cast<py::list>();
      l.append(obj);
    } else {
      py::list list;
      list.append(v);
      list.append(obj);
      py::dict new_entry;
      new_entry[py::str(name)] = list;
      dict.attr("update")(new_entry);
    }
  } else {
    py::dict new_entry;
    new_entry[py::str(name)] = obj;
    dict.attr("update")(new_entry);
  }
}

template<typename T>
void addObject(py::dict &dict, T &&t, const std::string &name) {
  py::object obj = py::cast(std::forward<T>(t));
  addObjectImpl(dict, obj, name);
}

void addObject(py::dict &dict, py::str &&t, const std::string &name) { addObjectImpl(dict, std::move(t), name); }

template<>
void addObject(py::dict &dict, py::dict &t, const std::string &name) {
  py::object obj = t;
  addObjectImpl(dict, obj, name);
}

void BuildDict(py::dict &dict, const DataBlock &blk) {
  for (int i = 0; i < blk.paramCount(); i++) {
    switch (blk.getParamType(i)) {
      case DataBlock::TYPE_STRING: {
        auto py_str = str_to_py_str(blk.getStr(i));
        addObject(dict, std::move(py_str), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_INT: {
        addObject(dict, blk.getInt(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_REAL: {
        addObject(dict, blk.getReal(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_POINT2: {
        addObject(dict, blk.getPoint2(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_POINT3: {
        addObject(dict, blk.getPoint3(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_POINT4: {
        addObject(dict, blk.getPoint4(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_IPOINT2: {
        addObject(dict, blk.getIPoint2(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_IPOINT3: {
        addObject(dict, blk.getIPoint3(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_IPOINT4: {
        addObject(dict, blk.getIPoint4(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_BOOL: {
        addObject(dict, blk.getBool(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_E3DCOLOR: {
        addObject(dict, blk.getE3dcolor(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_MATRIX: {
        addObject(dict, blk.getTm(i), blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_INT64: {
        addObject(dict, blk.getInt64(i), blk.getParamName(i));
        break;
      }
    }
  }
  for (int i = 0; i < blk.blockCount(); i++) {
    py::dict t_dict;
    const DataBlock *b = blk.getBlock(i);
    BuildDict(t_dict, *b);
    std::string t_str{b->getBlockName()};
    addObject(dict, t_dict, t_str);
  }
}

inline std::span<const char> bytes_to_span(const py::bytes &py_bytes) {
  // Extract data pointer and size from py::bytes
  char *data;
  Py_ssize_t size;
  // This will not copy, just gets access to internals
  PYBIND11_BYTES_AS_STRING_AND_SIZE(py_bytes.ptr(), &data, &size);

  // Convert to std::span<std::byte>
  return {reinterpret_cast<const char *>(data), static_cast<size_t>(size)};
}

struct ROBlkBlockIterator {
  const DataBlock *block;
  int index = 0;
  uint32_t blockCount;

  ROBlkBlockIterator &iter() { return *this; }

  const DataBlockRO next() {
    if (index < blockCount) {
      return DataBlockRO(block->getBlock(index++));
    }
    throw py::stop_iteration();
  }
};

struct RWBlkBlockIterator {
  DataBlock *block;
  int index = 0;
  uint32_t blockCount;

  RWBlkBlockIterator &iter() { return *this; }

  const DataBlockRW next() {
    if (index < blockCount) {
      return DataBlockRW(block->getBlock(index++));
    }
    throw py::stop_iteration();
  }
};


void PyDataBlock::include(py::module_ &m) {
  DO_INCLUDE()
  py_math.include(m);

  auto ro_iter = py::class_<ROBlkBlockIterator>(m, "ROBlkBlockIterator");
  auto rw_iter = py::class_<RWBlkBlockIterator>(m, "RWBlkBlockIterator");

  ro_iter.def("__iter__", &ROBlkBlockIterator::iter);
  rw_iter.def("__iter__", &RWBlkBlockIterator::iter);

  py::enum_<DataBlock::ParamType>(m, "DataBlockParamType")
    .value("TYPE_NONE", DataBlock::TYPE_NONE)
    .value("TYPE_STRING", DataBlock::TYPE_STRING)
    .value("TYPE_INT", DataBlock::TYPE_INT)
    .value("TYPE_REAL", DataBlock::TYPE_REAL)
    .value("TYPE_POINT2", DataBlock::TYPE_POINT2)
    .value("TYPE_POINT3", DataBlock::TYPE_POINT3)
    .value("TYPE_POINT4", DataBlock::TYPE_POINT4)
    .value("TYPE_IPOINT2", DataBlock::TYPE_IPOINT2)
    .value("TYPE_IPOINT3", DataBlock::TYPE_IPOINT3)
    .value("TYPE_BOOL", DataBlock::TYPE_BOOL)
    .value("TYPE_E3DCOLOR", DataBlock::TYPE_E3DCOLOR)
    .value("TYPE_MATRIX", DataBlock::TYPE_MATRIX)
    .value("TYPE_INT64", DataBlock::TYPE_INT64)
    .value("TYPE_IPOINT4", DataBlock::TYPE_IPOINT4)
    .value("TYPE_COUNT", DataBlock::TYPE_COUNT);

  const auto wrap_ro = [](const DataBlock *blk) -> py::object {
    if (!blk)
      return py::none();
    return py::cast(DataBlockRO(blk));
  };

  const auto wrap_rw = [](DataBlock *blk) -> py::object {
    if (!blk)
      return py::none();
    return py::cast(DataBlockRW(blk));
  };

  auto dbro = py::class_<DataBlockRO>(m, "DataBlockRO");
  dbro
    .def(
      "toString",
      [](const DataBlockRO &self) {
        DynamicMemGeneralSaveCB cwr(0, 4 << 20);
        self->saveToTextStream(cwr);
        return str_to_py_str({(char *) cwr.data(), (size_t) cwr.size()});
      },
      "Serialize this block to BLK text.")
    .def(
      "toBin",
      [](const DataBlockRO &self, bool compact) {
        DynamicMemGeneralSaveCB cwr(0, 4 << 20);
        if (compact) {
          DataBlock temp{};
          temp.setFrom(self.ptr());
          temp.saveToStream(cwr);
        } else
          self->saveToStream(cwr);
        auto view = std::string_view{reinterpret_cast<const char *>(cwr.data()), static_cast<size_t>(cwr.size())};
        return py::bytes(view);
      },
      "Serialize this block to BLK binary.", py::arg("compact") = false)
    .def(
      "toDict",
      [](DataBlockRO &self) {
        py::dict to_dict;
        BuildDict(to_dict, *self);
        return to_dict;
      },
      "Convert this block and children to a Python dictionary.")
    .def(
      "saveToTextFile",
      [](const DataBlockRO &self, const std::string &filename) { return self->saveToTextFile(filename.c_str()); },
      "Save this block to a text BLK file.")
    .def(
      "saveToTextFileCompact",
      [](const DataBlockRO &self, const std::string &filename) {
        return self->saveToTextFileCompact(filename.c_str());
      },
      "Save this block to a compact text BLK file.")
    .def(
      "__eq__", [](const DataBlockRO &self, const DataBlockRO &rhs) { return *self == *rhs; },
      "Compare two blocks for equality.")
    .def(
      "__ne__", [](const DataBlockRO &self, const DataBlockRO &rhs) { return *self != *rhs; },
      "Compare two blocks for inequality.")
    .def(
      "isValid", [](const DataBlockRO &self) { return self->isValid(); }, "Return whether block data is valid.")
    .def(
      "resolveFilename",
      [](const DataBlockRO &self, bool file_only) { return str_to_py_str(self->resolveFilename(file_only)); },
      "Resolve the source filename associated with this block.", py::arg("file_only") = false)
    .def(
      "isEmpty", [](const DataBlockRO &self) { return self->isEmpty(); },
      "Return whether this block has no params or child blocks.")
    .def(
      "topMost", [](const DataBlockRO &self) { return self->topMost(); },
      "Return whether this is the root owner block.")
    .def(
      "memUsed", [](const DataBlockRO &self) { return self->memUsed(); },
      "Return memory used by this block tree in bytes.")
    .def(
      "getNameId", [](const DataBlockRO &self) { return self->getNameId(); }, "Get this block name id.")
    .def(
      "getNameId", [](const DataBlockRO &self, const std::string &name) { return self->getNameId(name.c_str()); },
      "Look up a name id in this block name map.")
    .def(
      "getName", [](const DataBlockRO &self, int name_id) { return str_to_py_str(self->getName(name_id)); },
      "Get name string by id.")
    .def(
      "hasNoNameId", [](const DataBlockRO &self) { return self->hasNoNameId(); },
      "Return whether this block has no name id.")
    .def(
      "getBlockNameId", [](const DataBlockRO &self) { return self->getBlockNameId(); }, "Get this block's name id.")
    .def(
      "getBlockName",
      [](const DataBlockRO &self) {
        auto block_name = self->getBlockName();
        if (block_name == nullptr) {
          block_name = "root";
        }
        return str_to_py_str(block_name);
      },
      "Get this block's name.")
    .def(
      "getBlockNameView",
      [](const DataBlockRO &self) {
        auto block_name = self->getBlockNameView();
        if (block_name.empty()) {
          block_name = "root";
        }
        return str_to_py_str(block_name);
      },
      "Get this block's name.")
    .def(
      "getBlockCount", [](const DataBlockRO &self) { return self->blockCount(); }, "Get number of child blocks.")
    .def(
      "blockCount", [](const DataBlockRO &self) { return self->blockCount(); }, "Get number of child blocks.")
    .def(
      "blockCountById", [](const DataBlockRO &self, int name_id) { return self->blockCountById(name_id); },
      "Count child blocks by name id.")
    .def(
      "blockCountByName",
      [](const DataBlockRO &self, const std::string &name) { return self->blockCountByName(name.c_str()); },
      "Count child blocks by name.")
    .def(
      "getBlock", [wrap_ro](const DataBlockRO &self, uint32_t i) { return wrap_ro(self->getBlock(i)); },
      "Get child block by index.", py::keep_alive<0, 1>())
    .def(
      "getBlockByNameId",
      [wrap_ro](const DataBlockRO &self, int name_id, int start_after, bool expect_single) {
        return wrap_ro(self->getBlockByName(name_id, start_after, expect_single));
      },
      "Get child block by name id.", py::arg("name_id"), py::arg("start_after") = -1, py::arg("expect_single") = false,
      py::keep_alive<0, 1>())
    .def(
      "getBlockByName",
      [wrap_ro](const DataBlockRO &self, const std::string &name, int start_after, bool expect_single) {
        return wrap_ro(self->getBlockByName(self->getNameId(name.c_str()), start_after, expect_single));
      },
      "Get child block by name.", py::arg("name"), py::arg("start_after") = -1, py::arg("expect_single") = false,
      py::keep_alive<0, 1>())
    .def(
      "getBlockByName",
      [wrap_ro](const DataBlockRO &self, const std::string &name) {
        return wrap_ro(self->getBlockByName(name.c_str()));
      },
      "Get child block by name and require a single match.", py::keep_alive<0, 1>())
    .def(
      "getBlockByNameEx",
      [wrap_ro](const DataBlockRO &self, const std::string &name) {
        return wrap_ro(self->getBlockByNameEx(name.c_str()));
      },
      "Get child block by name or an empty block fallback.", py::keep_alive<0, 1>())
    .def(
      "findBlock",
      [](const DataBlockRO &self, int name_id, int start_after) { return self->findBlock(name_id, start_after); },
      "Find child block index by name id.", py::arg("name_id"), py::arg("start_after") = -1)
    .def(
      "findBlock",
      [](const DataBlockRO &self, const std::string &name, int start_after) {
        return self->findBlock(name.c_str(), start_after);
      },
      "Find child block index by name.", py::arg("name"), py::arg("start_after") = -1)
    .def(
      "findBlockByName",
      [](const DataBlockRO &self, const std::string &name, int start_after) {
        return self->findBlock(name.c_str(), start_after);
      },
      "Find child block index by name.", py::arg("name"), py::arg("start_after") = -1)
    .def(
      "findBlockRev",
      [](const DataBlockRO &self, int name_id, int start_before) { return self->findBlockRev(name_id, start_before); },
      "Find child block index by name id in reverse.")
    .def(
      "blockExists", [](const DataBlockRO &self, const std::string &name) { return self->blockExists(name.c_str()); },
      "Return whether a child block exists.")
    .def(
      "getParamCount", [](const DataBlockRO &self) { return self->paramCount(); }, "Get number of parameters.")
    .def(
      "paramCount", [](const DataBlockRO &self) { return self->paramCount(); }, "Get number of parameters.")
    .def(
      "paramCountById", [](const DataBlockRO &self, int name_id) { return self->paramCountById(name_id); },
      "Count parameters by name id.")
    .def(
      "paramCountByName",
      [](const DataBlockRO &self, const std::string &name) { return self->paramCountByName(name.c_str()); },
      "Count parameters by name.")
    .def(
      "getParamType", [](const DataBlockRO &self, uint32_t i) { return self->getParamType(i); },
      "Get parameter type by index.")
    .def(
      "getParamNameId", [](const DataBlockRO &self, uint32_t i) { return self->getParamNameId(i); },
      "Get parameter name id by index.")
    .def(
      "getParamName", [](const DataBlockRO &self, uint32_t i) { return str_to_py_str(self->getParamName(i)); },
      "Get parameter name by index.")
    .def(
      "findParam", [](const DataBlockRO &self, int name_id) { return self->findParam(name_id); },
      "Find parameter index by name id.")
    .def(
      "findParam", [](const DataBlockRO &self, int name_id, int after) { return self->findParam(name_id, after); },
      "Find parameter index by name id after a position.")
    .def(
      "findParamByName",
      [](const DataBlockRO &self, const std::string &name, int after) { return self->findParam(name.c_str(), after); },
      "Find parameter index by name.", py::arg("name"), py::arg("after") = -1)
    .def(
      "findParamRev",
      [](const DataBlockRO &self, int name_id, int start_before) { return self->findParamRev(name_id, start_before); },
      "Find parameter index by name id in reverse.")
    .def(
      "paramExistsById",
      [](const DataBlockRO &self, int name_id, int after) { return self->paramExists(name_id, after); },
      "Return whether a parameter exists by name id.", py::arg("name_id"), py::arg("after") = -1)
    .def(
      "paramExists", [](const DataBlockRO &self, int name_id, int after) { return self->paramExists(name_id, after); },
      "Return whether a parameter exists by name id.", py::arg("name_id"), py::arg("after") = -1)
    .def(
      "paramExists",
      [](const DataBlockRO &self, const std::string &name, int after) {
        return self->paramExists(name.c_str(), after);
      },
      "Return whether a parameter exists by name.", py::arg("name"), py::arg("after") = -1)
    .def(
      "getStr", [](const DataBlockRO &self, int param_idx) { return str_to_py_str(self->getStr(param_idx)); },
      "Get string parameter by index.")
    .def("getStr", [](const DataBlockRO &self, const std::string &name,
                      const std::string &def) { return str_to_py_str(self->getStr(name.c_str(), def.c_str())); })
    .def("getStr",
         [](const DataBlockRO &self, const std::string &name) { return str_to_py_str(self->getStr(name.c_str())); })
    .def(
      "getStrByNameId",
      [](const DataBlockRO &self, int name_id, const std::string &def) {
        return str_to_py_str(self->getStrByNameId(name_id, def.c_str()));
      },
      "Get string parameter by name id with default.")
    .def(
      "getInt", [](const DataBlockRO &self, int param_idx) { return self->getInt(param_idx); },
      "Get int parameter by index.")
    .def("getInt",
         [](const DataBlockRO &self, const std::string &name, int def) { return self->getInt(name.c_str(), def); })
    .def("getInt", [](const DataBlockRO &self, const std::string &name) { return self->getInt(name.c_str()); })
    .def(
      "getIntByNameId",
      [](const DataBlockRO &self, int name_id, int def) { return self->getIntByNameId(name_id, def); },
      "Get int parameter by name id with default.")
    .def(
      "getE3dcolor", [](const DataBlockRO &self, int param_idx) { return self->getE3dcolor(param_idx); },
      "Get E3DCOLOR parameter by index.")
    .def("getE3dcolor", [](const DataBlockRO &self, const std::string &name,
                           E3DCOLOR def) { return self->getE3dcolor(name.c_str(), def); })
    .def("getE3dcolor",
         [](const DataBlockRO &self, const std::string &name) { return self->getE3dcolor(name.c_str()); })
    .def(
      "getE3dcolorByNameId",
      [](const DataBlockRO &self, int name_id, E3DCOLOR def) { return self->getE3dcolorByNameId(name_id, def); },
      "Get E3DCOLOR parameter by name id with default.")
    .def(
      "getInt64", [](const DataBlockRO &self, int param_idx) { return self->getInt64(param_idx); },
      "Get int64 parameter by index.")
    .def("getInt64", [](const DataBlockRO &self, const std::string &name,
                        int64_t def) { return self->getInt64(name.c_str(), def); })
    .def("getInt64", [](const DataBlockRO &self, const std::string &name) { return self->getInt64(name.c_str()); })
    .def(
      "getInt64ByNameId",
      [](const DataBlockRO &self, int name_id, int64_t def) { return self->getInt64ByNameId(name_id, def); },
      "Get int64 parameter by name id with default.")
    .def(
      "getReal", [](const DataBlockRO &self, int param_idx) { return self->getReal(param_idx); },
      "Get real parameter by index.")
    .def("getReal",
         [](const DataBlockRO &self, const std::string &name, float def) { return self->getReal(name.c_str(), def); })
    .def("getReal", [](const DataBlockRO &self, const std::string &name) { return self->getReal(name.c_str()); })
    .def(
      "getRealByNameId",
      [](const DataBlockRO &self, int name_id, float def) { return self->getRealByNameId(name_id, def); },
      "Get real parameter by name id with default.")
    .def(
      "getBool", [](const DataBlockRO &self, int param_idx) { return self->getBool(param_idx); },
      "Get bool parameter by index.")
    .def("getBool",
         [](const DataBlockRO &self, const std::string &name, bool def) { return self->getBool(name.c_str(), def); })
    .def("getBool", [](const DataBlockRO &self, const std::string &name) { return self->getBool(name.c_str()); })
    .def(
      "getBoolByNameId",
      [](const DataBlockRO &self, int name_id, bool def) { return self->getBoolByNameId(name_id, def); },
      "Get bool parameter by name id with default.")
    .def(
      "getPoint2", [](const DataBlockRO &self, int param_idx) { return self->getPoint2(param_idx); },
      "Get Point2 parameter by index.")
    .def("getPoint2", [](const DataBlockRO &self, const std::string &name,
                         const Point2 &def) { return self->getPoint2(name.c_str(), def); })
    .def("getPoint2", [](const DataBlockRO &self, const std::string &name) { return self->getPoint2(name.c_str()); })
    .def(
      "getPoint2ByNameId",
      [](const DataBlockRO &self, int name_id, const Point2 &def) { return self->getPoint2ByNameId(name_id, def); },
      "Get Point2 parameter by name id with default.")
    .def(
      "getPoint3", [](const DataBlockRO &self, int param_idx) { return self->getPoint3(param_idx); },
      "Get Point3 parameter by index.")
    .def("getPoint3", [](const DataBlockRO &self, const std::string &name,
                         const Point3 &def) { return self->getPoint3(name.c_str(), def); })
    .def("getPoint3", [](const DataBlockRO &self, const std::string &name) { return self->getPoint3(name.c_str()); })
    .def(
      "getPoint3ByNameId",
      [](const DataBlockRO &self, int name_id, const Point3 &def) { return self->getPoint3ByNameId(name_id, def); },
      "Get Point3 parameter by name id with default.")
    .def(
      "getPoint4", [](const DataBlockRO &self, int param_idx) { return self->getPoint4(param_idx); },
      "Get Point4 parameter by index.")
    .def("getPoint4", [](const DataBlockRO &self, const std::string &name,
                         const Point4 &def) { return self->getPoint4(name.c_str(), def); })
    .def("getPoint4", [](const DataBlockRO &self, const std::string &name) { return self->getPoint4(name.c_str()); })
    .def(
      "getPoint4ByNameId",
      [](const DataBlockRO &self, int name_id, const Point4 &def) { return self->getPoint4ByNameId(name_id, def); },
      "Get Point4 parameter by name id with default.")
    .def(
      "getIPoint2", [](const DataBlockRO &self, int param_idx) { return self->getIPoint2(param_idx); },
      "Get IPoint2 parameter by index.")
    .def("getIPoint2", [](const DataBlockRO &self, const std::string &name,
                          const IPoint2 &def) { return self->getIPoint2(name.c_str(), def); })
    .def("getIPoint2", [](const DataBlockRO &self, const std::string &name) { return self->getIPoint2(name.c_str()); })
    .def(
      "getIPoint2ByNameId",
      [](const DataBlockRO &self, int name_id, const IPoint2 &def) { return self->getIPoint2ByNameId(name_id, def); },
      "Get IPoint2 parameter by name id with default.")
    .def(
      "getIPoint3", [](const DataBlockRO &self, int param_idx) { return self->getIPoint3(param_idx); },
      "Get IPoint3 parameter by index.")
    .def("getIPoint3", [](const DataBlockRO &self, const std::string &name,
                          const IPoint3 &def) { return self->getIPoint3(name.c_str(), def); })
    .def("getIPoint3", [](const DataBlockRO &self, const std::string &name) { return self->getIPoint3(name.c_str()); })
    .def(
      "getIPoint3ByNameId",
      [](const DataBlockRO &self, int name_id, const IPoint3 &def) { return self->getIPoint3ByNameId(name_id, def); },
      "Get IPoint3 parameter by name id with default.")
    .def(
      "getIPoint4", [](const DataBlockRO &self, int param_idx) { return self->getIPoint4(param_idx); },
      "Get IPoint4 parameter by index.")
    .def("getIPoint4", [](const DataBlockRO &self, const std::string &name,
                          const IPoint4 &def) { return self->getIPoint4(name.c_str(), def); })
    .def("getIPoint4", [](const DataBlockRO &self, const std::string &name) { return self->getIPoint4(name.c_str()); })
    .def(
      "getIPoint4ByNameId",
      [](const DataBlockRO &self, int name_id, const IPoint4 &def) { return self->getIPoint4ByNameId(name_id, def); },
      "Get IPoint4 parameter by name id with default.")
    .def(
      "getTm", [](const DataBlockRO &self, int param_idx) { return self->getTm(param_idx); },
      "Get TMatrix parameter by index.")
    .def("getTm", [](const DataBlockRO &self, const std::string &name,
                     const TMatrix &def) { return self->getTm(name.c_str(), def); })
    .def("getTm", [](const DataBlockRO &self, const std::string &name) { return self->getTm(name.c_str()); })
    .def(
      "getTmByNameId",
      [](const DataBlockRO &self, int name_id, const TMatrix &def) { return self->getTmByNameId(name_id, def); },
      "Get TMatrix parameter by name id with default.")
    .def("iterBlocks", [](const DataBlockRO &self) {
      ROBlkBlockIterator iter{self.ptr(), 0, self->blockCount()};
      return iter;
    });

  auto dbrw = py::class_<DataBlockRW, DataBlockRO>(m, "DataBlockRW");
  dbrw
    .def(
      "setSharedNameMapAndClearData",
      [](DataBlockRW &self, DataBlockRW &other) { self->setSharedNameMapAndClearData(other.rw_blk); },
      "Share name map with another block and clear current data.")
    .def(
      "clearData", [](DataBlockRW &self) { self->clearData(); }, "Clear all parameters and child blocks.")
    .def(
      "reset", [](DataBlockRW &self) { self->reset(); }, "Reset block data and names.")
    .def(
      "resetAndReleaseRoNameMap", [](DataBlockRW &self) { self->resetAndReleaseRoNameMap(); },
      "Reset and release read-only name map.")
    .def(
      "compact", [](DataBlockRW &self) { self->compact(); }, "Compact block storage to reduce memory usage.")
    .def(
      "shrink", [](DataBlockRW &self) { self->shrink(); }, "Shrink internal storage.")
    .def(
      "load", [](DataBlockRW &self, const std::string &filename) { return self->load(filename.c_str()); },
      "Load block data from file.")
    .def(
      "loadText",
      [](DataBlockRW &self, const std::string &text, const std::string &fname) {
        return self->loadText(text.c_str(), (int) text.size(), fname.empty() ? nullptr : fname.c_str());
      },
      "Load block data from BLK text.", py::arg("text"), py::arg("fname") = "")
    .def(
      "addNameId", [](DataBlockRW &self, const std::string &name) { return self->addNameId(name.c_str()); },
      "Add a name to the name map.")
    .def(
      "changeBlockName", [](DataBlockRW &self, const std::string &name) { self->changeBlockName(name.c_str()); },
      "Rename this block.")
    .def(
      "getBlock", [wrap_rw](DataBlockRW &self, uint32_t i) { return wrap_rw(self->getBlock(i)); },
      "Get writable child block by index.", py::keep_alive<0, 1>())
    .def(
      "getBlockByNameId",
      [wrap_rw](DataBlockRW &self, int name_id, int start_after, bool expect_single) {
        return wrap_rw(self->getBlockByName(name_id, start_after, expect_single));
      },
      "Get writable child block by name id.", py::arg("name_id"), py::arg("start_after") = -1,
      py::arg("expect_single") = false, py::keep_alive<0, 1>())
    .def(
      "getBlockByName",
      [wrap_rw](DataBlockRW &self, const std::string &name, int start_after, bool expect_single) {
        return wrap_rw(self->getBlockByName(self->getNameId(name.c_str()), start_after, expect_single));
      },
      "Get writable child block by name.", py::arg("name"), py::arg("start_after") = -1,
      py::arg("expect_single") = false, py::keep_alive<0, 1>())
    .def(
      "getBlockByName",
      [wrap_rw](DataBlockRW &self, const std::string &name) { return wrap_rw(self->getBlockByName(name.c_str())); },
      "Get writable child block by name and require a single match.", py::keep_alive<0, 1>())
    .def(
      "addBlock",
      [wrap_rw](DataBlockRW &self, const std::string &name) { return wrap_rw(self->addBlock(name.c_str())); },
      "Get or add child block by name.", py::keep_alive<0, 1>())
    .def(
      "addNewBlock",
      [wrap_rw](DataBlockRW &self, const std::string &name) { return wrap_rw(self->addNewBlock(name.c_str())); },
      "Add a new child block by name.", py::keep_alive<0, 1>())
    .def(
      "addNewBlock",
      [wrap_rw](DataBlockRW &self, const DataBlockRO &copy_from, const std::string &as_name) {
        return wrap_rw(self->addNewBlock(copy_from.ro_blk, as_name.empty() ? nullptr : as_name.c_str()));
      },
      "Copy a block subtree into a new child block.", py::arg("copy_from"), py::arg("as_name") = "",
      py::keep_alive<0, 1>())
    .def(
      "setParamsFrom", [](DataBlockRW &self, const DataBlockRO &copy_from) { self->setParamsFrom(copy_from.ro_blk); },
      "Replace parameters from another block.")
    .def(
      "appendParamsFrom",
      [](DataBlockRW &self, const DataBlockRO &copy_from) { self->appendParamsFrom(copy_from.ro_blk); },
      "Append parameters from another block.")
    .def(
      "removeBlock", [](DataBlockRW &self, const std::string &name) { return self->removeBlock(name.c_str()); },
      "Remove child blocks by name.", py::arg("name"))
    .def(
      "removeBlock", [](DataBlockRW &self, uint32_t index) { return self->removeBlock(index); },
      "Remove child block by index.", py::arg("index"))
    .def(
      "swapBlocks", [](DataBlockRW &self, uint32_t i1, uint32_t i2) { return self->swapBlocks(i1, i2); },
      "Swap two child blocks by index.")
    .def(
      "setBlock",
      [wrap_rw](DataBlockRW &self, const DataBlockRO &blk, const std::string &as_name) {
        return wrap_rw(self->setBlock(blk.ro_blk, as_name.empty() ? nullptr : as_name.c_str()));
      },
      "Replace named child block with a copied subtree.", py::arg("blk"), py::arg("as_name") = "",
      py::keep_alive<0, 1>())
    .def(
      "setFrom", [](DataBlockRW &self, const DataBlockRO &from) { self->setFrom(from.ro_blk); },
      "Replace this block from another block.")
    .def(
      "setFrom",
      [](DataBlockRW &self, const DataBlockRO &from, const std::string &fname) {
        self->setFrom(from.ro_blk, fname.c_str());
      },
      "Replace this block from another block and assign source filename.")
    .def(
      "changeParamName",
      [](DataBlockRW &self, uint32_t i, const std::string &name) { self->changeParamName(i, name.c_str()); },
      "Rename a parameter by index.")
    .def(
      "setStr",
      [](DataBlockRW &self, int param_idx, const std::string &value) { return self->setStr(param_idx, value.c_str()); },
      "Set string parameter by index.")
    .def(
      "setStrByNameId",
      [](DataBlockRW &self, int name_id, const std::string &value) {
        return self->setStrByNameId(name_id, value.c_str());
      },
      "Set string parameter by name id.")
    .def("setStr", [](DataBlockRW &self, const std::string &name,
                      const std::string &value) { return self->setStr(name.c_str(), value.c_str()); })
    .def("addStr", [](DataBlockRW &self, const std::string &name,
                      const std::string &value) { return self->addStr(name.c_str(), value.c_str()); })
    .def(
      "addNewStrByNameId",
      [](DataBlockRW &self, int name_id, const std::string &value) {
        return self->addNewStrByNameId(name_id, value.c_str());
      },
      "Add new string parameter by name id.")
    .def(
      "setInt", [](DataBlockRW &self, int param_idx, int value) { return self->setInt(param_idx, value); },
      "Set int parameter by index.")
    .def("setIntByNameId",
         [](DataBlockRW &self, int name_id, int value) { return self->setIntByNameId(name_id, value); })
    .def("setInt",
         [](DataBlockRW &self, const std::string &name, int value) { return self->setInt(name.c_str(), value); })
    .def("addInt",
         [](DataBlockRW &self, const std::string &name, int value) { return self->addInt(name.c_str(), value); })
    .def(
      "addNewIntByNameId",
      [](DataBlockRW &self, int name_id, int value) { return self->addNewIntByNameId(name_id, value); },
      "Add new int parameter by name id.")
    .def(
      "setE3dcolor",
      [](DataBlockRW &self, int param_idx, E3DCOLOR value) { return self->setE3dcolor(param_idx, value); },
      "Set E3DCOLOR parameter by index.")
    .def("setE3dcolorByNameId",
         [](DataBlockRW &self, int name_id, E3DCOLOR value) { return self->setE3dcolorByNameId(name_id, value); })
    .def("setE3dcolor", [](DataBlockRW &self, const std::string &name,
                           E3DCOLOR value) { return self->setE3dcolor(name.c_str(), value); })
    .def("addE3dcolor", [](DataBlockRW &self, const std::string &name,
                           E3DCOLOR value) { return self->addE3dcolor(name.c_str(), value); })
    .def(
      "addNewE3dcolorByNameId",
      [](DataBlockRW &self, int name_id, E3DCOLOR value) { return self->addNewE3dcolorByNameId(name_id, value); },
      "Add new E3DCOLOR parameter by name id.")
    .def(
      "setInt64", [](DataBlockRW &self, int param_idx, int64_t value) { return self->setInt64(param_idx, value); },
      "Set int64 parameter by index.")
    .def("setInt64ByNameId",
         [](DataBlockRW &self, int name_id, int64_t value) { return self->setInt64ByNameId(name_id, value); })
    .def("setInt64",
         [](DataBlockRW &self, const std::string &name, int64_t value) { return self->setInt64(name.c_str(), value); })
    .def("addInt64",
         [](DataBlockRW &self, const std::string &name, int64_t value) { return self->addInt64(name.c_str(), value); })
    .def(
      "addNewInt64ByNameId",
      [](DataBlockRW &self, int name_id, int64_t value) { return self->addNewInt64ByNameId(name_id, value); },
      "Add new int64 parameter by name id.")
    .def(
      "setReal", [](DataBlockRW &self, int param_idx, float value) { return self->setReal(param_idx, value); },
      "Set real parameter by index.")
    .def("setRealByNameId",
         [](DataBlockRW &self, int name_id, float value) { return self->setRealByNameId(name_id, value); })
    .def("setReal",
         [](DataBlockRW &self, const std::string &name, float value) { return self->setReal(name.c_str(), value); })
    .def("addReal",
         [](DataBlockRW &self, const std::string &name, float value) { return self->addReal(name.c_str(), value); })
    .def(
      "addNewRealByNameId",
      [](DataBlockRW &self, int name_id, float value) { return self->addNewRealByNameId(name_id, value); },
      "Add new real parameter by name id.")
    .def(
      "setBool", [](DataBlockRW &self, int param_idx, bool value) { return self->setBool(param_idx, value); },
      "Set bool parameter by index.")
    .def("setBoolByNameId",
         [](DataBlockRW &self, int name_id, bool value) { return self->setBoolByNameId(name_id, value); })
    .def("setBool",
         [](DataBlockRW &self, const std::string &name, bool value) { return self->setBool(name.c_str(), value); })
    .def("addBool",
         [](DataBlockRW &self, const std::string &name, bool value) { return self->addBool(name.c_str(), value); })
    .def(
      "addNewBoolByNameId",
      [](DataBlockRW &self, int name_id, bool value) { return self->addNewBoolByNameId(name_id, value); },
      "Add new bool parameter by name id.")
    .def(
      "setPoint2",
      [](DataBlockRW &self, int param_idx, const Point2 &value) { return self->setPoint2(param_idx, value); },
      "Set Point2 parameter by index.")
    .def("setPoint2ByNameId",
         [](DataBlockRW &self, int name_id, const Point2 &value) { return self->setPoint2ByNameId(name_id, value); })
    .def("setPoint2", [](DataBlockRW &self, const std::string &name,
                         const Point2 &value) { return self->setPoint2(name.c_str(), value); })
    .def("addPoint2", [](DataBlockRW &self, const std::string &name,
                         const Point2 &value) { return self->addPoint2(name.c_str(), value); })
    .def(
      "addNewPoint2ByNameId",
      [](DataBlockRW &self, int name_id, const Point2 &value) { return self->addNewPoint2ByNameId(name_id, value); },
      "Add new Point2 parameter by name id.")
    .def(
      "setPoint3",
      [](DataBlockRW &self, int param_idx, const Point3 &value) { return self->setPoint3(param_idx, value); },
      "Set Point3 parameter by index.")
    .def("setPoint3ByNameId",
         [](DataBlockRW &self, int name_id, const Point3 &value) { return self->setPoint3ByNameId(name_id, value); })
    .def("setPoint3", [](DataBlockRW &self, const std::string &name,
                         const Point3 &value) { return self->setPoint3(name.c_str(), value); })
    .def("addPoint3", [](DataBlockRW &self, const std::string &name,
                         const Point3 &value) { return self->addPoint3(name.c_str(), value); })
    .def(
      "addNewPoint3ByNameId",
      [](DataBlockRW &self, int name_id, const Point3 &value) { return self->addNewPoint3ByNameId(name_id, value); },
      "Add new Point3 parameter by name id.")
    .def(
      "setPoint4",
      [](DataBlockRW &self, int param_idx, const Point4 &value) { return self->setPoint4(param_idx, value); },
      "Set Point4 parameter by index.")
    .def("setPoint4ByNameId",
         [](DataBlockRW &self, int name_id, const Point4 &value) { return self->setPoint4ByNameId(name_id, value); })
    .def("setPoint4", [](DataBlockRW &self, const std::string &name,
                         const Point4 &value) { return self->setPoint4(name.c_str(), value); })
    .def("addPoint4", [](DataBlockRW &self, const std::string &name,
                         const Point4 &value) { return self->addPoint4(name.c_str(), value); })
    .def(
      "addNewPoint4ByNameId",
      [](DataBlockRW &self, int name_id, const Point4 &value) { return self->addNewPoint4ByNameId(name_id, value); },
      "Add new Point4 parameter by name id.")
    .def(
      "setIPoint2",
      [](DataBlockRW &self, int param_idx, const IPoint2 &value) { return self->setIPoint2(param_idx, value); },
      "Set IPoint2 parameter by index.")
    .def("setIPoint2ByNameId",
         [](DataBlockRW &self, int name_id, const IPoint2 &value) { return self->setIPoint2ByNameId(name_id, value); })
    .def("setIPoint2", [](DataBlockRW &self, const std::string &name,
                          const IPoint2 &value) { return self->setIPoint2(name.c_str(), value); })
    .def("addIPoint2", [](DataBlockRW &self, const std::string &name,
                          const IPoint2 &value) { return self->addIPoint2(name.c_str(), value); })
    .def(
      "addNewIPoint2ByNameId",
      [](DataBlockRW &self, int name_id, const IPoint2 &value) { return self->addNewIPoint2ByNameId(name_id, value); },
      "Add new IPoint2 parameter by name id.")
    .def(
      "setIPoint3",
      [](DataBlockRW &self, int param_idx, const IPoint3 &value) { return self->setIPoint3(param_idx, value); },
      "Set IPoint3 parameter by index.")
    .def("setIPoint3ByNameId",
         [](DataBlockRW &self, int name_id, const IPoint3 &value) { return self->setIPoint3ByNameId(name_id, value); })
    .def("setIPoint3", [](DataBlockRW &self, const std::string &name,
                          const IPoint3 &value) { return self->setIPoint3(name.c_str(), value); })
    .def("addIPoint3", [](DataBlockRW &self, const std::string &name,
                          const IPoint3 &value) { return self->addIPoint3(name.c_str(), value); })
    .def(
      "addNewIPoint3ByNameId",
      [](DataBlockRW &self, int name_id, const IPoint3 &value) { return self->addNewIPoint3ByNameId(name_id, value); },
      "Add new IPoint3 parameter by name id.")
    .def(
      "setIPoint4",
      [](DataBlockRW &self, int param_idx, const IPoint4 &value) { return self->setIPoint4(param_idx, value); },
      "Set IPoint4 parameter by index.")
    .def("setIPoint4ByNameId",
         [](DataBlockRW &self, int name_id, const IPoint4 &value) { return self->setIPoint4ByNameId(name_id, value); })
    .def("setIPoint4", [](DataBlockRW &self, const std::string &name,
                          const IPoint4 &value) { return self->setIPoint4(name.c_str(), value); })
    .def("addIPoint4", [](DataBlockRW &self, const std::string &name,
                          const IPoint4 &value) { return self->addIPoint4(name.c_str(), value); })
    .def(
      "addNewIPoint4ByNameId",
      [](DataBlockRW &self, int name_id, const IPoint4 &value) { return self->addNewIPoint4ByNameId(name_id, value); },
      "Add new IPoint4 parameter by name id.")
    .def(
      "setTm", [](DataBlockRW &self, int param_idx, const TMatrix &value) { return self->setTm(param_idx, value); },
      "Set TMatrix parameter by index.")
    .def("setTmByNameId",
         [](DataBlockRW &self, int name_id, const TMatrix &value) { return self->setTmByNameId(name_id, value); })
    .def("setTm", [](DataBlockRW &self, const std::string &name,
                     const TMatrix &value) { return self->setTm(name.c_str(), value); })
    .def("addTm", [](DataBlockRW &self, const std::string &name,
                     const TMatrix &value) { return self->addTm(name.c_str(), value); })
    .def(
      "addNewTmByNameId",
      [](DataBlockRW &self, int name_id, const TMatrix &value) { return self->addNewTmByNameId(name_id, value); },
      "Add new TMatrix parameter by name id.")
    .def(
      "removeParam", [](DataBlockRW &self, const std::string &name) { return self->removeParam(name.c_str()); },
      "Remove parameters by name.", py::arg("name"))
    .def(
      "removeParam", [](DataBlockRW &self, uint32_t index) { return self->removeParam(index); },
      "Remove parameter by index.", py::arg("index"))
    .def(
      "swapParams", [](DataBlockRW &self, uint32_t i1, uint32_t i2) { return self->swapParams(i1, i2); },
      "Swap two parameters by index.")
    .def(
      "fromBytes",
      [](DataBlockRW &blk, py::bytes &bytes) -> bool {
        auto spn = bytes_to_span(bytes);
        InPlaceMemLoadCB rdr(const_cast<char *>(spn.data()), (int) spn.size());
        return blk->loadFromStream(rdr, nullptr);
      },
      "Load block data from some bytes. Can be a text or bin blk")
    .def("iterBlocks", [](const DataBlockRW &self) {
      RWBlkBlockIterator iter{self.ptr(), 0, self->blockCount()};
      return iter;
    });

  ro_iter.def("__next__", &ROBlkBlockIterator::next);
  rw_iter.def("__next__", &RWBlkBlockIterator::next);


  py::class_<DataBlockRWObj, DataBlockRW>(m, "DataBlock")
    .def(py::init<>())
    .def(py::init<std::string>(), "loads the Datablock from a file path.");
}

PyDataBlock py_data_block{};
