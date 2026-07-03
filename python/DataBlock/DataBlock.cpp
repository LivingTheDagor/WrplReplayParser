#include "modules/DataBlock/DataBlock.h"
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

template <typename T>
void addObject(py::dict &dict, T &&t, const std::string &name) {
  py::object obj = py::cast(std::forward<T>(t));
  addObjectImpl(dict, obj, name);
}

void addObject(py::dict &dict, py::str &&t, const std::string &name) { addObjectImpl(dict, std::move(t), name); }

template <>
void addObject(py::dict &dict, py::dict &t, const std::string &name) {
  py::object obj = t;
  addObjectImpl(dict, obj, name);
}

void BuildDict(py::dict &dict, DataBlock &blk) {
  for(int i = 0; i < blk.paramCount(); i++) {
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
  for(int i = 0; i < blk.blockCount(); i++) {
    py::dict t_dict;
    DataBlock *b = blk.getBlock(i);
    BuildDict(t_dict, *b);
    std::string t_str{b->getBlockName()};
    addObject(dict, t_dict, t_str);
  }
}

inline std::span<const char> bytes_to_span(const py::bytes& py_bytes) {
  // Extract data pointer and size from py::bytes
  char* data;
  Py_ssize_t size;
  // This will not copy, just gets access to internals
  PYBIND11_BYTES_AS_STRING_AND_SIZE(py_bytes.ptr(), &data, &size);

  // Convert to std::span<std::byte>
  return {reinterpret_cast<const char*>(data), static_cast<size_t>(size)};
}

void PyDataBlock::include(py::module_ &m) {
  DO_INCLUDE()

  py::class_<DataBlock>(m, "DataBlock")
      .def(py::init<>())
      .def("toString", [](const DataBlock& self){
        // this->length * 8 is just some value
        DynamicMemGeneralSaveCB cwr(0, 4 << 20); // TOOD for this and vromf printer, maybe add std::ostringstream cb?
        self.saveToTextStream(cwr);
           auto py_str = str_to_py_str({(char *) cwr.data(), (size_t) cwr.size()});
           return py_str;
         })
      .def("toDict", [](DataBlock &self){
        py::dict to_dict;
        BuildDict(to_dict, self);
        return to_dict;
      })
      .def("fromBytes", [](DataBlock &blk, py::bytes &bytes) -> bool {
        auto spn = bytes_to_span(bytes);
        InPlaceMemLoadCB rdr(const_cast<char *>(spn.data()), (int) spn.size());
        return blk.loadFromStream(rdr, nullptr);
      });
}

PyDataBlock py_data_block{};
