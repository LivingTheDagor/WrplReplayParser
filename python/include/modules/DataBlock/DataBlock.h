

#pragma once

#include "Module.h"

class PyDataBlock : protected Module {
public:
  PyDataBlock() : Module() {}
  void include(py::module_ &m);
};

extern PyDataBlock py_data_block;
class DataBlock;
struct DataBlockRO {
  const DataBlock *ro_blk = nullptr;

  const DataBlock *operator->() const { return ro_blk; }
  const DataBlock &operator*() const { return *ro_blk; }
  const DataBlock *ptr() const { return ro_blk; }
  DataBlockRO() = default;
  DataBlockRO(const DataBlock *ro_blk_) : ro_blk(ro_blk_) {}
};

struct DataBlockRW : DataBlockRO {
  DataBlock *rw_blk = nullptr;
  DataBlock *operator->() const { return rw_blk; }
  DataBlock &operator*() const { return *rw_blk; }
  DataBlock *ptr() const { return rw_blk; }
  DataBlockRW() = default;
  DataBlockRW(DataBlock *blk) : DataBlockRO(blk), rw_blk(blk) {}
};
