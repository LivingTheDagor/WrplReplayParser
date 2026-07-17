


#include "VROMFs.h"

#include "ioSys/dag_dataBlock.h"
#include <chrono>
#include <array>
#include <fstream> // For ifstream, ofstream, and fstream
#include "unordered_set"
#include <regex>
#include "math/integer/dag_IPoint3.h"
#include "math/integer/dag_IPoint2.h"

int max_player_count_per_team = 8;

std::unordered_set<std::string> parsed = {"gameData/missions/templates/tank_templates/starshell_template.blk",
                                          "gameData/missions/templates/tank_arcade_streaks_template.blk"};
int rank;
IPoint2 defaultRankRange{0, 51};

struct import_data {
  bool importAreas = true;
  bool importUnits = true;
  bool importTriggers = true;
  bool importMissionObjectives = true;
  bool importWayPoints = true;


  import_data() = default;

  import_data(const DataBlock *imp) {
    importAreas = imp->getBool("importAreas", false);
    importUnits = imp->getBool("importUnits", false);
    importTriggers = imp->getBool("importTriggers", false);
    importMissionObjectives = imp->getBool("importMissionObjectives", false);
    importWayPoints = imp->getBool("importWayPoints", false);
  }

  import_data(const DataBlock *imp, import_data other) {
    importAreas = other.importAreas && imp->getBool("importAreas", false);
    importUnits = other.importUnits && imp->getBool("importUnits", false);
    importTriggers = other.importTriggers && imp->getBool("importTriggers", false);
    importMissionObjectives = other.importMissionObjectives && imp->getBool("importMissionObjectives", false);
    importWayPoints = other.importWayPoints && imp->getBool("importWayPoints", false);
  }
};

void appendBlock(DataBlock *to, const DataBlock *from) {
  auto blk = to->addNewBlock(from->getBlockName());
  blk->appendParamsFrom(from);
  for (int i = 0; i < from->blockCount(); i++) {
    auto subBlk = from->getBlock(i);
    auto newBlk = blk->addBlock(subBlk->getBlockName());
    newBlk->setFrom(subBlk);
  }
}

void appendBlockData(DataBlock *to, const DataBlock *from) {
  to->appendParamsFrom(from);
  for (int i = 0; i < from->blockCount(); i++) {
    auto subBlk = from->getBlock(i);
    auto newBlk = to->addBlock(subBlk->getBlockName());
    newBlk->setFrom(subBlk);
  }
}

void unpackTriggers(DataBlock *to, const DataBlock *from) {
  for (int i = 0; i < from->blockCount(); i++) {
    auto subBlk = from->getBlock(i);
    if (subBlk->getBool("isCategory", false)) {
      unpackTriggers(to, subBlk);
    } else {
      auto maybe_resp = subBlk->getBlockByNameEx("actions")->getBlockByNameEx("missionMarkAsRespawnPoint", nullptr);
      if (maybe_resp) {
        auto PayloadBlk = to->addBlock(subBlk->getBlockName());
        PayloadBlk->appendParamsFrom(subBlk);
        for (int b = 0; b < subBlk->blockCount(); b++) {
          auto bl = subBlk->getBlock(b);
          if (bl->getBlockNameView() == "actions") {
            auto actions = PayloadBlk->addBlock("actions");
            for (int act_blk_index = 0; act_blk_index < bl->blockCount(); act_blk_index++) {
              auto act_blk = bl->getBlock(act_blk_index);
              if (act_blk->getBlockNameView() == "missionMarkAsRespawnPoint") {
                if (auto out = act_blk->getStr("loc_name", nullptr)) {
                  if (strcmp(out, "missions/air_spawn") == 0 && false) {
                    act_blk->printBlock(std::cout);
                    continue;
                  }
                }
              }
              appendBlock(actions, act_blk);
            }
          } else {
            appendBlock(PayloadBlk, bl);
          }
        }
      } else {
        appendBlock(to, subBlk);
      }
    }
  }
}


void parse(DataBlock *to, const DataBlock *from, import_data prev) {
  auto file_name = std::string(from->getStr("file", ""));
  G_ASSERT(!file_name.empty()); // no reason why it shouldn't be there
  if (parsed.contains(file_name))
    return; // we have already parsed this file
  std::cout << file_name << "\n";
  parsed.emplace(file_name);
  import_data imp_data{from, prev};
  DataBlock blk{};
  G_ASSERTF(dblk::load(blk, file_name), "failed to import blk({})", file_name);
  int importNid = from->getNameId("imports");
  int importRecordNid = from->getNameId("import_record");
  int importAreasNid = from->getNameId("areas");
  int importUnitsNid = from->getNameId("units");
  int importTriggersNid = from->getNameId("triggers");
  int importMissionObjectivesNid = from->getNameId("mission_objectives");
  int importWayPointsNid = from->getNameId("wayPoints");
  int mission_settingsNid = from->getNameId("mission_settings");
  int variables_Nid = from->getNameId("variables");
  for (int i = 0; i < blk.blockCount(); i++) {
    auto subBlk = blk.getBlock(i);
    auto subBlk_nid = subBlk->getBlockNameId();
    // std::cout << subBlk->getBlockName() << "\n";
    if (subBlk_nid == mission_settingsNid) {
      auto misBlock = subBlk->getBlockByNameEx("mission");
      // misBlock->printBlock(4, std::cout);
      continue;
    }
    if (subBlk_nid == importNid) {

      // subBlk->printBlock(4, std::cout);
      //  do nothing this iter
    } else if (subBlk_nid == importAreasNid) {
      if (imp_data.importAreas) {
        appendBlock(to, subBlk);
      }
    } else if (subBlk_nid == importUnitsNid) {
      if (imp_data.importUnits) {
        auto temp_blk = to->addBlock(subBlk->getBlockName());
        for (int b = 0; b < subBlk->blockCount(); b++) {
          auto obj_blk = subBlk->getBlock(b);
          if (obj_blk->getBlockNameView() == "armada") {
            auto name = obj_blk->getStr("name", nullptr);
            if (name && *name == 't') {
              name += 9;
              auto nid = std::stoi(std::string(name));

              // std::cout << name << " : " << nid << "\n";
              if (nid > max_player_count_per_team)
                continue;
            }
          }
          appendBlock(temp_blk, obj_blk);
        }
        // temp_blk->addBlockInplace(subBlk, false, false); // we dont care about the 'from' blk
      }
    } else if (subBlk_nid == importTriggersNid) {
      if (imp_data.importTriggers) {
        auto temp_blk = to->addBlock(subBlk->getBlockName());
        unpackTriggers(temp_blk, subBlk);
        // temp_blk->addBlockInplace(subBlk, false, false); // we dont care about the 'from' blk
      }
    } else if (subBlk_nid == importMissionObjectivesNid) {
      if (imp_data.importMissionObjectives) {

        auto temp_blk = to->addBlock(subBlk->getBlockName());
        appendBlockData(temp_blk, subBlk);
      }
    } else if (subBlk_nid == importWayPointsNid) {
      if (imp_data.importWayPoints) {
        auto temp_blk = to->addBlock(subBlk->getBlockName());
        appendBlockData(temp_blk, subBlk);
      }
    } else if (subBlk_nid == variables_Nid) {
      auto temp_blk = to->addBlock(subBlk->getBlockName());
      appendBlockData(temp_blk, subBlk);
    } else // if it isnt one of the managed imports, then add it
    {

      auto temp_blk = to->addBlock(subBlk->getBlockName());
      appendBlock(temp_blk, subBlk);
    }
  }
  for (int i = 0; i < blk.blockCount(); i++) {
    auto subBlk = blk.getBlock(i);
    auto subBlk_nid = subBlk->getBlockNameId();
    if (subBlk_nid == importNid) {
      for (int z = 0; z < subBlk->blockCount(); z++) {


        auto ImportRecordBlk = subBlk->getBlock(z);
        IPoint2 range = ImportRecordBlk->getIPoint2("rankRange", defaultRankRange);
        // std::cout << range.toString() << "\n";
        if (rank < range.x || rank > range.y)
          continue;
        G_ASSERT(ImportRecordBlk->getBlockNameId() == importRecordNid);
        parse(to, ImportRecordBlk, imp_data);
      }
    }
  }
}

void FlattenMissionBlk(DataBlock *to, const DataBlock *from) {
  int importNid = from->getNameId("imports");
  int importRecordNid = from->getNameId("import_record");
  // to and from have the same name map currently.
  for (int i = 0; i < from->blockCount(); i++) {
    auto block = from->getBlock(i);
    if (block->getBlockNameId() == importNid) {
      for (int z = 0; z < block->blockCount(); z++) {
        auto ImportRecordBlk = block->getBlock(z);
        G_ASSERT(ImportRecordBlk->getBlockNameId() == importRecordNid);

        // ImportRecordBlk->printBlock(4, std::cout);
        parse(to, ImportRecordBlk, {});
      }
    } else { // anything that isnt an import gets added to final blk
      appendBlock(to, block);
    }
  }
  to->appendParamsFrom(from);
}

DataBlock* CreateFlatBlk(std::string &miss_blk, int in_rank, bool addCustomBlock, bool addCustomScore,
                                   bool addCustomTriggers, bool addCustomVars) {
  ::rank = in_rank;
  DataBlock blk{};
  EXCEPTION_IF_FALSE(dblk::load(blk, miss_blk.c_str()), "failed to load mission blk");
  auto outBlk = new DataBlock();
  FlattenMissionBlk(outBlk, &blk);
  auto mission_settings_blk = outBlk->addBlock("mission_settings");
  auto mission_blk = mission_settings_blk->addBlock("mission");
  if (addCustomBlock || addCustomScore || addCustomVars) {
    auto mission2_blk = mission_blk->addBlock("mission");
    fs::path base_path = BASE_DIR;
    fs::path SettingsBlk = base_path / R"(debugging\DumpMission\CustomSettings.blk)";
    fs::path ScoreBlk = base_path / R"(debugging\DumpMission\CustomSpawnScore.blk)";
    fs::path TriggersBlk = base_path / R"(debugging\DumpMission\customTriggers.blk)";
    fs::path VarsBlk = base_path / R"(debugging\DumpMission\CustomVars.blk)";
    if (addCustomBlock) {
      DataBlock temp{};
      EXCEPTION_IF_FALSE(dblk::load(temp, SettingsBlk.string()), "Failed to parse CustomBlk");
      appendBlock(mission2_blk, &temp);
    }
    if (addCustomScore) {
      mission_blk->addBool("useSpawnScore", true);

      DataBlock temp{};
      EXCEPTION_IF_FALSE(dblk::load(temp, ScoreBlk.string()), "Failed to parse CustomScoreBlk");
      appendBlock(mission2_blk, &temp);
    }
    if (addCustomTriggers) {
      DataBlock temp{};
      EXCEPTION_IF_FALSE(dblk::load(temp, TriggersBlk.string()), "Failed to parse TriggersBlk");
      appendBlock(mission2_blk, &temp);
      auto triggers_blk = outBlk->addBlock("triggers");
      for (int b = 0; b < temp.blockCount(); b++) {
        auto blk_ = temp.getBlock(b);
        auto trigger = triggers_blk->addNewBlock(blk_->getBlockName());
        trigger->setFrom(blk_);
      }
    }
    /*if (addCustomVars) {
      std::ifstream file{VarsBlk};
      EXCEPTION_IF_FALSE(file.is_open(), "failed to open file");
      SharedPtr<DataBlock> _blk = SharedPtr<DataBlock>::make(outBlk->getNameMap());
      std::vector<char> characters((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      EXCEPTION_IF_FALSE(_blk->loadText(characters), "Failed to parse CustomVarsBlk");
      auto triggers_blk = outBlk->addBlock("variables");

      for (int b = 0; b < _blk->paramCount(); b++) {
        auto blk_ = _blk->getParam(b);
        triggers_blk->addParam(blk_);
      }
    }*/
  }

  /*DataBlock::isValidParamCallback cbp = [](SharedPtr<DataBlock::Param> &p) {
    const std::regex pattern(R"(^(t1|t2)_player\d{2}$)");
    if (p->type == DataBlock::TYPE_STRING) {
      auto name = p->data.s;
      if (std::regex_match(std::string(name), pattern)) {
        auto ptr = name.data() + 9;
        auto val = std::stoi(ptr);
        if (val > max_player_count_per_team)
          return true;
      }
    }
    return false;
  };*/

  // used to remove unneeded player declarations
  //outBlk->cleanupParamsByCB(cbp);
  return outBlk;
}

int main() {
  // parsed.emplace("gameData/missions/templates/tank_arcade_streaks_template.blk");
  // parsed.emplace("gameData/missions/templates/ww_exit_zones.blk");
  // parsed.emplace("gameData/missions/templates/ww_rearm_zones.blk");
  bool addCustomBlock = true;
  bool addCustomScore = false;
  bool addCustomTriggers = true;
  bool addCustomVars = false;
  bool disableAirSsawn = false;
  bool disableBomberSpawn = false;
  std::string vromfs_mission_path = "gamedata/missions/cta/tanks/port_novorossiysk/port_novorossiysk_aslt_dom.blk";
  //vromfs_mission_path = "gamedata/missions/cta/tanks/tunisia/tunisia_dom.blk";
  // std::string vromfs_mission_path = "gamedata/missions/cta/tanks/mozdok/mozdok_dom.blk";
  //  AN ERROR HAS OCCURED
#ifdef _TARGET_PC_LINUX
  std::string dump_path = R"(/mnt/d/GoogleDriveWtMission/dumpTest2.blk)";
  std::string p1 = R"(/mnt/d/SteamLibrary/steamapps/common/War Thunder/mis.vromfs.bin)";
#else
  std::string dump_path = R"(D:\ReplayParser\debugging\DumpMission\output.blk)";
  dump_path = "D:/GoogleDriveWtMission/dumpTesting.blk";
  // std::string p1 = R"(D:\SteamLibrary\steamapps\common\War Thunder\mis.vromfs.bin)";
  std::string p1 = R"(D:\SteamLibrary\steamapps\common\War Thunder\mis.vromfs.bin)";
#endif
  // EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p2), "Ah shit");
  EXCEPTION_IF_FALSE(file_mgr.mountVromfs(p1), "Ah shit");
  DataBlock* outBlk = CreateFlatBlk(vromfs_mission_path, 10, false, false, false, false);
  EXCEPTION_IF_FALSE(outBlk, "failed to create flat blk");
  // outBlk->getBlock("triggers", 0)->getBlock("aslt_check_capture", 0)->getBlock("actions",
  // 0)->getBlock("triggerEnable", 0)->addStr("target", "on_capture_respawn"); outBlk->getBlock("triggers",
  // 0)->getBlock("aslt_spawn_captured", 0)->getBlock("actions", 0)->getBlock("triggerEnable", 0)->addStr("target",
  // "on_capture_respawn"); outBlk->getBlock("variables", 0)->addBool("force_plane_airfield_spawn", true);

  std::ostringstream ss;
  std::ofstream out{dump_path};
  auto cb = &out;
  EXCEPTION_IF_FALSE(out, "failed to open file for write({})", dump_path.c_str());
  outBlk->printBlock(ss);
  auto s = ss.str();
  LOGI("good: {}", out.good());
  cb->write(s.c_str(), s.size());
  cb->flush();
  delete outBlk;
}