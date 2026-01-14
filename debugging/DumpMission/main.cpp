


#include "VROMFs.h"

#include "DataBlock.h"
#include <chrono>
#include <array>
#include "reader.h"
#include <fstream> // For ifstream, ofstream, and fstream
#include "unordered_set"
#include <regex>

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

void unpackTriggers(SharedPtr<DataBlock> &to, SharedPtr<DataBlock> &from) {
  for (int i = 0; i < from->blockCount(); i++) {
    auto subBlk = from->getBlock(i);
    if (subBlk->getBool("isCategory", false)) {
      unpackTriggers(to, subBlk);
    } else {
      auto maybe_resp = subBlk->getBlock("actions", 0)->getBlock("missionMarkAsRespawnPoint", 0);
      if (maybe_resp) {
        auto PayloadBlk = to->getBlock(to->addBlock(subBlk->getBlockName().data()));
        for (int p = 0; p < subBlk->paramCount(); p++)
          PayloadBlk->addParam(subBlk->getParam(p));
        for (int b = 0; b < subBlk->blockCount(); b++) {
          auto bl = subBlk->getBlock(b);
          if (bl->getBlockName() == "actions") {
            auto actions = PayloadBlk->getAddBlock("actions");
            for (int act_blk_index = 0; act_blk_index < bl->blockCount(); act_blk_index++) {
              auto act_blk = bl->getBlock(act_blk_index);
              if (act_blk->getBlockName() == "missionMarkAsRespawnPoint") {
                if (auto out = act_blk->getStr("loc_name", nullptr)) {
                  if (strcmp(out, "missions/air_spawn") == 0 && false)
                  {
                    std::cout << "REMOVED\n";
                    act_blk->printBlock(0, std::cout);
                    continue;
                  }
                }
              }
              actions->addBlock(act_blk);
            }
          } else {
            PayloadBlk->addBlock(bl);
          }
        }
      } else {
        to->addBlock(subBlk);
      }
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
    if (subBlk_nid == mission_settingsNid) {
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
        for (int b = 0; b < subBlk->blockCount(); b++) {
          auto obj_blk = subBlk->getBlock(b);
          if (obj_blk->getBlockName() == "armada") {
            auto name = obj_blk->getStr("name", nullptr);
            if (name && *name == 't') {
              name += 9;
              auto nid = std::stoi(std::string(name));

              //std::cout << name << " : " << nid << "\n";
              if (nid > max_player_count_per_team)
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
        if (rank < range.x || rank > range.y)
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

SharedPtr<DataBlock> CreateFlatBlk(std::string &miss_blk,
                   int in_rank,
                   bool addCustomBlock,
                   bool addCustomScore,
                   bool addCustomTriggers,
                   bool addCustomVars) {
  ::rank = in_rank;
  SharedPtr<DataBlock> blk = SharedPtr<DataBlock>::make();
  EXCEPTION_IF_FALSE(load(*blk.get(), miss_blk.c_str()), "failed to load mission blk");
  SharedPtr<DataBlock> outBlk = SharedPtr<DataBlock>::make(blk->getNameMap());
  FlattenMissionBlk(outBlk, blk);
  auto mission_settings_blk = outBlk->getBlock("mission_settings", 0);
  auto mission_blk = mission_settings_blk->getBlock("mission", 0);
  if (addCustomBlock || addCustomScore || addCustomVars) {
    auto mission2_blk = mission_blk->getBlock("mission", 0);
    fs::path base_path = BASE_DIR;
    fs::path SettingsBlk = base_path / R"(debugging\DumpMission\CustomSettings.blk)";
    fs::path ScoreBlk = base_path / R"(debugging\DumpMission\CustomSpawnScore.blk)";
    fs::path TriggersBlk = base_path / R"(debugging\DumpMission\customTriggers.blk)";
    fs::path VarsBlk = base_path / R"(debugging\DumpMission\CustomVars.blk)";
    if (addCustomBlock) {
      std::ifstream file{SettingsBlk};
      EXCEPTION_IF_FALSE(file.is_open(), "failed to open file");
      SharedPtr<DataBlock> _blk = SharedPtr<DataBlock>::make(outBlk->getNameMap());
      std::vector<char> characters(
          (std::istreambuf_iterator<char>(file)),
          std::istreambuf_iterator<char>()
      );
      EXCEPTION_IF_FALSE(_blk->loadText(characters), "Failed to parse CustomBlk");
      mission_blk->addBlockInplace(_blk, false, true);
    }
    if (addCustomScore) {
      mission_blk->addBool("useSpawnScore", true);
      std::ifstream file{ScoreBlk};
      EXCEPTION_IF_FALSE(file.is_open(), "failed to open file");
      SharedPtr<DataBlock> _blk = SharedPtr<DataBlock>::make(outBlk->getNameMap());
      std::vector<char> characters(
          (std::istreambuf_iterator<char>(file)),
          std::istreambuf_iterator<char>()
      );
      EXCEPTION_IF_FALSE(_blk->loadText(characters), "Failed to parse CustomScoreBlk");
      mission2_blk->addBlockInplace(_blk, false, true);
    }
    if (addCustomTriggers) {
      std::ifstream file{TriggersBlk};
      EXCEPTION_IF_FALSE(file.is_open(), "failed to open file");
      SharedPtr<DataBlock> _blk = SharedPtr<DataBlock>::make(outBlk->getNameMap());
      std::vector<char> characters(
          (std::istreambuf_iterator<char>(file)),
          std::istreambuf_iterator<char>()
      );
      EXCEPTION_IF_FALSE(_blk->loadText(characters), "Failed to parse CustomScoreBlk");
      auto triggers_blk = outBlk->getBlock("triggers", 0);
      for (int b = 0; b < _blk->blockCount(); b++) {
        auto blk_ = _blk->getBlock(b);
        triggers_blk->addBlock(blk_);
      }
    }
    if (addCustomVars) {
      std::ifstream file{VarsBlk};
      EXCEPTION_IF_FALSE(file.is_open(), "failed to open file");
      SharedPtr<DataBlock> _blk = SharedPtr<DataBlock>::make(outBlk->getNameMap());
      std::vector<char> characters(
          (std::istreambuf_iterator<char>(file)),
          std::istreambuf_iterator<char>()
      );
      EXCEPTION_IF_FALSE(_blk->loadText(characters), "Failed to parse CustomVarsBlk");
      auto triggers_blk = outBlk->getBlock("variables", 0);

      for (int b = 0; b < _blk->paramCount(); b++) {
        auto blk_ = _blk->getParam(b);
        triggers_blk->addParam(blk_);
      }
    }
  }

  DataBlock::isValidParamCallback cbp = [](SharedPtr<DataBlock::Param> &p) {
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
  };

  // used to remove unneeded player declarations
  outBlk->cleanupParamsByCB(cbp);
  return outBlk;
}

int main() {
  //parsed.emplace("gameData/missions/templates/tank_arcade_streaks_template.blk");
  //parsed.emplace("gameData/missions/templates/ww_exit_zones.blk");
  //parsed.emplace("gameData/missions/templates/ww_rearm_zones.blk");
  bool addCustomBlock = true;
  bool addCustomScore = false;
  bool addCustomTriggers = true;
  bool addCustomVars = false;
  bool disableAirSsawn = false;
  bool disableBomberSpawn = false;
  std::string vromfs_mission_path = "gamedata/missions/cta/tanks/port_novorossiysk/port_novorossiysk_aslt_dom.blk";
  vromfs_mission_path = "gamedata/missions/cta/humans/granitograd_test_infdom.blk";
  //std::string vromfs_mission_path = "gamedata/missions/cta/tanks/mozdok/mozdok_dom.blk";
  // AN ERROR HAS OCCURED
#ifdef Linux
  std::string dump_path = R"(/mnt/d/GoogleDriveWtMission/dumpTest2.blk)";
  std::string p1 = R"(/mnt/d/SteamLibrary/steamapps/common/War Thunder/cache/binary.2.49.0/mis.vromfs.bin)";
#endif
#ifdef Windows
  std::string dump_path = R"(D:/GoogleDriveWtMission/dumpLow.blk)";
  dump_path = "D:/GoogleDriveWtMission/dumpInf.blk";
  //std::string p1 = R"(D:\SteamLibrary\steamapps\common\War Thunder\mis.vromfs.bin)";
  std::string p1 = R"(D:\WarThunderDev\cache\binary.2.53.0\mis.vromfs.bin)";
  std::string p2 = "D:\\WarThunderDev\\cache\\binary.2.52.0\\game-dev.vromfs.bin";
#endif
  //EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p2), "Ah shit");
  EXCEPTION_IF_FALSE(file_mgr.loadVromfs(p1), "Ah shit");
  SharedPtr<DataBlock> outBlk = CreateFlatBlk(vromfs_mission_path, 10, true, false, true, false);
  EXCEPTION_IF_FALSE(outBlk, "failed to create flat blk");
  //outBlk->getBlock("triggers", 0)->getBlock("aslt_check_capture", 0)->getBlock("actions", 0)->getBlock("triggerEnable", 0)->addStr("target", "on_capture_respawn");
  //outBlk->getBlock("triggers", 0)->getBlock("aslt_spawn_captured", 0)->getBlock("actions", 0)->getBlock("triggerEnable", 0)->addStr("target", "on_capture_respawn");
  //outBlk->getBlock("variables", 0)->addBool("force_plane_airfield_spawn", true);

  std::ostringstream ss;
  std::ofstream out{dump_path};
  auto cb = &out;
  EXCEPTION_IF_FALSE(out, "failed to open file for write({})", dump_path.c_str());
  outBlk->printBlock(0, ss);
  auto s = ss.str();
  cb->write(s.c_str(), s.size());
}
/*
     slotMultiSpawn:b=yes
    gt_use_replay:b=yes
    dedicatedReplay:b=yes
    groundUnitsAllowed:b=yes
    allowUseArtillerySupport:b=no
    useHumanBots:b=yes
    hudUseModernMarkers:b=yes
    isTankGunnerCameraFromSightForced:b=yes
    customHUDMessagesOffset:r=5
    mis_file:t="gamedata/missions/cta/tanks/krymsk/krymsk_bttl.blk"
    showAIKilllog:b=yes
    userMission:b=no
    hellohello:b=no
    level:t="levels/avg_port_novorossiysk.bin"
    type:t="domination"
    environment:t="Day"
    weather:t="clear"
    locName:t="missions/_Aslt;port_novorossiysk/name"
    locDesc:t="port_novorossiysk/desc;missions/_Aslt/desc"
    scoreLimit:i=14000
    timeLimit:i=20
    deathPenaltyMul:r=0
    postfix:t="_Dom"
    useAlternativeMapCoord:b=yes
    allowedKillStreaks:b=yes
    randomSpawnTeams:b=no
    remapAiTankModels:b=yes
    battleAreaColorPreset:t="battleArea"
    allowEmptyTeams:b=no
    useSpawnScore:b=yes
    showTacticalMapCellSize:b=yes
    timeLimitWarn:i=120
    country_axis:t="germany"
    country_allies:t="japan"
    restoreType:t="attempts"
    optionalTakeOff:b=no
    allowWinch:b=no
 */