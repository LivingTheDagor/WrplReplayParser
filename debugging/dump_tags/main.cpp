#include "VROMFs.h"
#include <unordered_set>

std::unordered_set<std::string> all_tags;

int main() {
  std::string p1 = R"(D:\SteamLibrary\steamapps\common\War Thunder\cache\binary.2.49.0\char.vromfs.bin)";

  EXCEPTION_IF_FALSE(file_mgr.mountVromfs(p1), "Ah shit");

  SharedPtr<DataBlock> blk = SharedPtr<DataBlock>::make();
  EXCEPTION_IF_FALSE(load(*blk.get(), "config/unittags.blk"), "failed to load blk");
  for(int i = 0; i < blk->blockCount(); i++)
  {
    auto unit_blk = blk->getBlock(i);
    auto tags_blk = unit_blk->getBlock("tags", 0);
    if(tags_blk)
    {
      //tags_blk->printBlock(0, std::cout);
      for(int p = 0; p < tags_blk->paramCount(); p++)
      {
        auto tag_t = tags_blk->getParamName(p);
        if(tag_t)
        {
          std::string tag = tag_t;
          if(all_tags.find(tag) == all_tags.end())
            all_tags.emplace(tag);
        }
      }
    }
  }
  for(auto &tag : all_tags)
  {
    std::cout << tag << "\n";
  }
}