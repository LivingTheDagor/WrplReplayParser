#include "modules/DataBlock/DataBlock.h"
#include "modules/BitStream.h"
#include "DataBlock.h"
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
void addObject(py::dict &dict, T &t, const std::string &name) {
  py::object obj = py::cast(t);
  addObjectImpl(dict, obj, name);
}

template <>
void addObject(py::dict &dict, py::dict &t, const std::string &name) {
  py::object obj = t;
  addObjectImpl(dict, obj, name);
}

void BuildDict(py::dict &dict, DataBlock &blk) {
  for(int i = 0; i < blk.paramCount(); i++) {
    auto p = blk.getParam(i).get();
    switch(p->type) {
      case DataBlock::TYPE_STRING: {
        addObject(dict, p->data.s, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_INT: {
        addObject(dict, p->data.i, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_REAL: {
        addObject(dict, p->data.r, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_POINT2: {
        addObject(dict, p->data.p2, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_POINT3: {
        addObject(dict, p->data.p3, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_POINT4: {
        addObject(dict, p->data.p4, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_IPOINT2: {
        addObject(dict, p->data.ip2, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_IPOINT3: {
        addObject(dict, p->data.ip3, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_IPOINT4: {
        addObject(dict, p->data.ip4, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_BOOL: {
        addObject(dict, p->data.b, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_E3DCOLOR: {
        addObject(dict, p->data.c, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_MATRIX: {
        addObject(dict, p->data.tm, blk.getParamName(i));
        break;
      }
      case DataBlock::TYPE_UINT64: {
        addObject(dict, p->data.u64, blk.getParamName(i));
        break;
      }
    }
  }
  for(int i = 0; i < blk.blockCount(); i++) {
    py::dict t_dict;
    DataBlock * b = blk.getBlock(i).get();
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
        std::ostringstream oss{};
        self.printBlock(0, oss);
        return oss.str();
      })
      .def("toDict", [](DataBlock &self){
        py::dict to_dict;
        BuildDict(to_dict, self);
        return to_dict;
      })
      .def("fromBytes", [](DataBlock &blk, py::bytes &bytes) -> bool {
        auto spn = bytes_to_span(bytes);
        BaseReader rdr(const_cast<char*>(spn.data()), (int)spn.size(), false);
        return blk.loadFromStream(rdr, nullptr, nullptr);
      });
}

PyDataBlock py_data_block{};
