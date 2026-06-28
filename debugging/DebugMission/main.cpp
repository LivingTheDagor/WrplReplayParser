#include "VROMFs.h"

#include "DataBlock.h"
#include <chrono>
#include <array>
#include "reader.h"
#include <fstream> // For ifstream, ofstream, and fstream
#include "unordered_set"
#include <regex>


int max_player_count_per_team = 8;


std::unordered_set<std::string> parsed = {"gameData/missions/templates/tank_templates/starshell_template.blk", "gameData/missions/templates/tank_arcade_streaks_template.blk"};
int rank;
IPoint2 defaultRankRange{0, 51};

struct import_data {
  bool importAreas = true;
  bool importUnits = true;
  bool importTriggers = true;
  bool importMissionObjectives = true;
  bool importWayPoints = true;


  import_data() = default;

  import_data(SharedPtr<DataBlock> &imp) {
    importAreas = imp->getBool("importAreas", false);
    importUnits = imp->getBool("importUnits", false);
    importTriggers = imp->getBool("importTriggers", false);
    importMissionObjectives = imp->getBool("importMissionObjectives", false);
    importWayPoints = imp->getBool("importWayPoints", false);
  }

  import_data(SharedPtr<DataBlock> &imp, import_data other) {
    importAreas = other.importAreas && imp->getBool("importAreas", false);
    importUnits = other.importUnits && imp->getBool("importUnits", false);
    importTriggers = other.importTriggers && imp->getBool("importTriggers", false);
    importMissionObjectives = other.importMissionObjectives && imp->getBool("importMissionObjectives", false);
    importWayPoints = other.importWayPoints && imp->getBool("importWayPoints", false);
  }
};

void unpackTriggers(SharedPtr<DataBlock> &to, SharedPtr<DataBlock> &from)
{
  for (int i = 0; i < from->blockCount(); i++) {
    auto subBlk = from->getBlock(i);
    if(subBlk->getBool("isCategory", false))
    {
      unpackTriggers(to, subBlk);
    }
    else
    {
      to->addBlock(subBlk);
    }
  }
}


void parse(SharedPtr<DataBlock> &to, SharedPtr<DataBlock> &from, import_data prev) {
  auto file_name = std::string(from->getStr("file", ""));
  G_ASSERT(!file_name.empty()); // no reason why it shouldn't be there
  if (parsed.contains(file_name))
    return; // we have already parsed this file
  std::cout << file_name << "\n";
  parsed.emplace(file_name);
  import_data imp_data{from, prev};
  SharedPtr<DataBlock> blk = SharedPtr<DataBlock>::make();
  EXCEPTION_IF_FALSE(load(*blk.get(), file_name.c_str()), "failed to import blk(%s)", file_name.c_str());
  int importNid = from->getNameId("imports");
  int importRecordNid = from->getNameId("import_record");
  int importAreasNid = from->getNameId("areas");
  int importUnitsNid = from->getNameId("units");
  int importTriggersNid = from->getNameId("triggers");
  int importMissionObjectivesNid = from->getNameId("mission_objectives");
  int importWayPointsNid = from->getNameId("wayPoints");
  int mission_settingsNid = from->getNameId("mission_settings");
  int variables_Nid = from->getNameId("variables");
  for (int i = 0; i < blk->blockCount(); i++) {
    auto subBlk = blk->getBlock(i);
    auto subBlk_nid = subBlk->getBlockNameId();
    //std::cout << subBlk->getBlockName() << "\n";
    if (subBlk_nid == mission_settingsNid)
    {
      auto misBlock = subBlk->getBlock("mission", 0);
      //misBlock->printBlock(4, std::cout);
      continue;
    }
    if (subBlk_nid == importNid) {

      //subBlk->printBlock(4, std::cout);
      // do nothing this iter
    } else if (subBlk_nid == importAreasNid) {
      if (imp_data.importAreas) {
        auto temp_blk = to->getAddBlock(subBlk->getBlockNameId());
        temp_blk->addBlockInplace(subBlk, false, false); // we dont care about the 'from' blk
      }
    } else if (subBlk_nid == importUnitsNid) {
      if (imp_data.importUnits) {
        auto temp_blk = to->getAddBlock(subBlk->getBlockNameId());
        for(int b = 0; b < subBlk->blockCount(); b++)
        {
          auto obj_blk = subBlk->getBlock(b);
          if(obj_blk->getBlockName() == "armada")
          {
            auto name = obj_blk->getStr("name", nullptr);
            if(name && *name=='t')
            {
              name += 9;
              auto nid = std::stoi(std::string(name));

              //std::cout << name << " : " << nid << "\n";
              if(nid > max_player_count_per_team)
                continue;
            }
          }
          temp_blk->addBlock(obj_blk);

        }
        //temp_blk->addBlockInplace(subBlk, false, false); // we dont care about the 'from' blk
      }
    } else if (subBlk_nid == importTriggersNid) {
      if (imp_data.importTriggers) {
        auto temp_blk = to->getAddBlock(subBlk->getBlockNameId());
        unpackTriggers(temp_blk, subBlk);
        //temp_blk->addBlockInplace(subBlk, false, false); // we dont care about the 'from' blk
      }
    } else if (subBlk_nid == importMissionObjectivesNid) {
      if (imp_data.importMissionObjectives) {

        auto temp_blk = to->getAddBlock(subBlk->getBlockNameId());
        temp_blk->addBlockInplace(subBlk, false, false); // we dont care about the 'from' blk
      }
    } else if (subBlk_nid == importWayPointsNid) {
      if (imp_data.importWayPoints) {

        auto temp_blk = to->getAddBlock(subBlk->getBlockNameId());
        temp_blk->addBlockInplace(subBlk, false, false); // we dont care about the 'from' blk
      }
    } else if (subBlk_nid == variables_Nid) {

      auto temp_blk = to->getAddBlock(subBlk->getBlockNameId());
      temp_blk->addBlockInplace(subBlk, false, true); // we dont care about the 'from' blk
    } else // if it isnt one of the managed imports, then add it
    {
      auto temp_blk = to->getAddBlock(subBlk->getBlockNameId());
      temp_blk->addBlockInplace(subBlk, false, false); // we dont care about the 'from' blk
    }
  }
  for (int i = 0; i < blk->blockCount(); i++) {
    auto subBlk = blk->getBlock(i);
    auto subBlk_nid = subBlk->getBlockNameId();
    if (subBlk_nid == importNid) {
      for (int z = 0; z < subBlk->blockCount(); z++) {


        auto ImportRecordBlk = subBlk->getBlock(z);
        IPoint2 range = ImportRecordBlk->getIPoint2("rankRange", defaultRankRange);
        //std::cout << range.toString() << "\n";
        if(rank < range.x || rank > range.y)
          continue;
        G_ASSERT(ImportRecordBlk->getBlockNameId() == importRecordNid);
        parse(to, ImportRecordBlk, imp_data);
      }
    }
  }
}

void FlattenMissionBlk(SharedPtr<DataBlock> &to, SharedPtr<DataBlock> &from) {
  int importNid = from->getNameId("imports");
  int importRecordNid = from->getNameId("import_record");
  // to and from have the same name map currently.
  for (int i = 0; i < from->blockCount(); i++) {
    auto block = from->getBlock(i);
    if (block->getBlockNameId() == importNid) {
      for (int z = 0; z < block->blockCount(); z++) {
        auto ImportRecordBlk = block->getBlock(z);
        G_ASSERT(ImportRecordBlk->getBlockNameId() == importRecordNid);

        //ImportRecordBlk->printBlock(4, std::cout);
        parse(to, ImportRecordBlk, {});
      }
    } else { // anything that isnt an import gets added to final blk
      auto blk = to->getAddBlock(block->getBlockNameId());
      blk->addBlockInplace(block, false, true); // we dont care about the 'from' blk
    }
  }
  for (int i = 0; i < from->paramCount(); i++) {
    auto other_P = from->getParam(i);
    to->addParam(other_P);
  }
}

int main() {
  std::string vromfs_mission_path = "gamedata\\missions\\cta\\tanks\\normandy\\normandy_dom.blk";
  auto mis2Path = "gamedata/missions/cta/tanks/port_novorossiysk/airfields/template_port_novorossiysk_airfields_low_ranks.blk";
#ifdef _TARGET_PC_LINUX
  std::string dump_path = R"(/mnt/d/GoogleDriveWtMission/dumpTest2.blk)";
  std::string p1 = R"(/mnt/d/SteamLibrary/steamapps/common/War Thunder/cache/binary.2.49.0/mis.vromfs.bin)";
#endif
#ifdef _TARGET_PC_WIN
  std::string dump_path = R"(D:/GoogleDriveWtMission/dump_t.blk)";
  std::string p1 = R"(D:\SteamLibrary\steamapps\common\War Thunder\cache\binary.2.53.0\mis.vromfs.bin)";
#endif
  EXCEPTION_IF_FALSE(file_mgr.mountVromfs(p1), "Ah shit");

  SharedPtr<DataBlock> blk = SharedPtr<DataBlock>::make();
  //SharedPtr<DataBlock> tblk = SharedPtr<DataBlock>::make();
  EXCEPTION_IF_FALSE(load(*blk.get(), vromfs_mission_path.c_str()), "failed to load mission blk");
  //EXCEPTION_IF_FALSE(load(*tblk.get(), mis2Path), "failed to load mission blk");
  //tblk->printBlock(0, std::cout);
  SharedPtr<DataBlock> outBlk = SharedPtr<DataBlock>::make(blk->getNameMap());
  FlattenMissionBlk(outBlk, blk);
  std::ofstream out{dump_path};
  //auto triggers = outBlk->getBlock("triggers", 0);
  //for(int b = 0; b < triggers->blockCount(); b++) {
  //  auto trigger = triggers->getBlock(b);
  //  auto actions = trigger->getBlock("actions", 0);
  //  std::cout << "TRIGGER NAME: " << trigger->getBlockName() << "\n";
  //  for(int b1 = 0; b1 < actions->blockCount(); b1++) {
  //    auto action = actions->getBlock(b1);
  //    actions->printBlock(0, std::cout);
  //  }
  //}
  outBlk->printBlock(0, out);
}