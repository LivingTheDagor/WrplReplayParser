

#ifndef MYEXTENSION_DATABLOCKOWNED_H
#define MYEXTENSION_DATABLOCKOWNED_H

#include "StringTableAllocator.h"

namespace dblk {
/*
 * A DataBlockOwned will own all its params and child blocks, allowing for them to be modified.
 * This is more memory and computationally expensive compared to a DataBlockShared, so only convert a DataBlockShared to owned if you want to modify the blk
 *
 */
  class DataBlockOwned {
    std::vector<DataBlock> blocks;
    std::vector<Param> params;
    std::vector<uint8_t> param_data; // large array of param data
  };
}


#endif //MYEXTENSION_DATABLOCKOWNED_H
