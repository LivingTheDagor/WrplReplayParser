#include "DataBlock.h"

DataBlockShared::DataBlockShared(uint32_t complexDataSize,uint32_t ParamCount, uint32_t BlockCount)
{
    this->complexDataSize = complexDataSize;
    this->ParamCount = ParamCount;
    this->BlockCount = BlockCount;
    BlockOffset = (uint32_t)(complexDataSize+ParamCount*sizeof(DataBlock::Param));
    data = (char *)malloc(complexDataSize+ParamCount*sizeof(DataBlock::Param)+BlockCount*sizeof(DataBlock));
}

DataBlockShared::~DataBlockShared()
{
    //std::cout << "DataBlockShared dtor\n";
    if (data)
    {
        //DataBlock* blocks_ = getBlocks();
        //for (uint32_t i = 1; i < BlockCount; ++i)
        //{
        //    blocks_[i].~DataBlock();
        //}
        free(data);
    }
}


