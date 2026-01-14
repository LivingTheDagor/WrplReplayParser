

#ifndef MYEXTENSION_DATABLOCK_H
#define MYEXTENSION_DATABLOCK_H
#include "vartypes.h"
#include "Param.h"

namespace dblk {
  class DataBlockShared;
  class DataBlockOwned;
  class DataBlock {
  private:
    DataBlockShared * shared; // exists to hold the name map and blocks if this is an shared block
    name_id name_id = -1; // name_id is index into DataBlockShared NameMap for this block's name. if -1, then block name is 'root'
    uint16_t param_count = 0;
    uint16_t block_count = 0;
    uint32_t block_ofs = 0; // block_ofs is used with DataBlockShared to indicate where this BLKs blocks start, used with block_count
    uint32_t param_ofs = 0;
    DataBlockOwned * owned; // holds the block and param data for if this block is 'owned', as in it owns its blocks and params
  public:
  };
}
// due to how code I copied from gaijin is setup, DataBlockShared needs DataBlock to be defined
#include "DataBlockOwned.h"
#include "DataBlockShared.h"


#endif //MYEXTENSION_DATABLOCK_H
