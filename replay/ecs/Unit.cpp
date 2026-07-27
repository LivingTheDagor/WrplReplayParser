
#include "Unit.h"
#include "idFieldSerializer.h"
#include "ecs/ComponentTypesDefs.h"
#include "ecs/EntityManager.h" // g_ecs_state
#include "FileSystem.h"
#include "math/dag_mathAng.h"
#include "state/ParserState.h"

namespace unit {
  Tank *Unit::AsTank() {
    if (unitType == TankType)
      return (Tank *) this;
    return nullptr;
  }

  Aircraft *Unit::AsAircraft() {
    if (unitType == AircraftType)
      return (Aircraft *) this;
    return nullptr;
  }
  const char *Aircraft::getUnitTypeName() {
    return "Aircraft";
  }

#define RET_FAIL(opt) \
  if (!(opt))         \
  return false

  bool unit::Unit::LoadFromStorage(const FieldSerializerDict &data) {
    BitStream bs{data.data.data(), data.data.size(), false};
    IdFieldSerializer255 IdFieldSerializer{};
    uint32_t end;
    uint16_t count = IdFieldSerializer.readFieldsSizeAndCount(bs, end);
    if (!IdFieldSerializer.readFieldsIndex(bs)) {
      return false;
    }

    for (uint16_t i = 0; i < count; ++i) {
      auto fieldId = IdFieldSerializer.getFieldId(i);
      auto f_size_bits = IdFieldSerializer.getFieldSize(i);
      auto start_offs = bs.GetReadOffset();

      switch (fieldId) {
        case 0x5: {
          RET_FAIL(bs.Read(spawn_position));
          break;
        }
        case 0x6: {
          RET_FAIL(bs.Read(raw_unit_name));
          break;
        }
        case 0x7: {
          RET_FAIL(bs.Read(player_internal_name));
          break;
        }
        case 0x8: {
          RET_FAIL(bs.Read(loadout_name));
          break;
        }
        case 0x9: {
          RET_FAIL(bs.Read(skin_name));
          break;
        }
        case 0xd: {
          RET_FAIL(bs.Read(owner_pid));
          break;
        }
        case 0x33: {
          BitStream t_bs;
          RET_FAIL(bs.Read(t_bs));
          uint8_t sz;
          RET_FAIL(t_bs.Read(sz));
          RET_FAIL(sz <= 6); // it should always be 6 currently, so if anything weird has happened best to know
          storage_weapons.resize(sz);
          for (auto &weapon: storage_weapons) {
            RET_FAIL(t_bs.Read(weapon.launcher));
            RET_FAIL(t_bs.Read(weapon.bullet));
            RET_FAIL(t_bs.Read(weapon.count));
          }
          RET_FAIL(t_bs.Read(sz));
          weapon_mods.resize(sz);
          for (auto &mod: weapon_mods)
            RET_FAIL(t_bs.Read(mod));
          RET_FAIL(t_bs.Read(sz));
          fm_mods.resize(sz);
          for (auto &mod: fm_mods)
            RET_FAIL(t_bs.Read(mod));
          break;
        }
        case 0x34: {
          RET_FAIL(bs.Read(camo_info));
          break;
        }
        case 0x40: {
          RET_FAIL(bs.Read(custom_weapons_blk));
          break;
        }
      }
      if (bs.GetReadOffset() != start_offs && bs.GetReadOffset() != (start_offs + f_size_bits)) {
        LOGE("Error while parsing Storage, parsed invalid size for id {:#x}", fieldId);
      }
      bs.SetReadOffset(start_offs + f_size_bits);
    }
    Load();
    return true;
  }

  struct weap {
    int id;
    const char *name;
  };

  std::array<weap, 18> weapon_id_match = {{{0, "machine gun"},
                                           {1, "cannon"},
                                           {2, "additional gun"},
                                           {0x3, "rockets"},
                                           {0x4, "bombs"},
                                           {0x5, "mines"},
                                           {0x6, "torpedoes"},
                                           {0x7, "atgm"},
                                           {0x8, "aam"},
                                           {0x9, "guided bombs"},
                                           {0xa, "fuel tanks"},
                                           {0xb, "boosters"},
                                           {0xc, "undercarriage"},
                                           {0xd, "airdrops"},
                                           {0xe, "countermeasures"},
                                           {0x12, "targetingPod"},
                                           {0xf, "special gun"},
                                           {0x10, "smoke"}}};


  int get_weapon_id(std::string_view weapon_name) {
    for (auto &[id, name]: weapon_id_match) {
      if (strcmp(weapon_name.data(), name) == 0)
        return id;
    }
    if (weapon_name == "agm") {
      return 7;
    } else if (weapon_name == "torpedoes") {
      return 6;
    } else if (strncmp(weapon_name.data(), "gunner", 6) == 0 && weapon_name.length() >= 7) {
      return atoi(weapon_name.data() + 6) + 0x11;
    }
    return -1;
  }

  void blkPrint(DataBlock &blk) {
    blk.printBlock(std::cout);
    std::cout.flush();
  }

  bool getWeaponPresetAndSlot(const DataBlock &weapons, int WeaponSlotNid, int WeaponPresetNid,
                              DataBlock &custom_weapons, int custom_blk_index, DataBlock const **WeaponSlot,
                              DataBlock const **WeaponPreset, int &tier, int &slot) {
    auto curr_custom_weapon = custom_weapons.getBlock(custom_blk_index);

    if (!curr_custom_weapon)
      return false;
    slot = curr_custom_weapon->getInt("slot", -1);
    auto preset = curr_custom_weapon->getStr("preset", nullptr);
    if (slot == -1 || preset == nullptr)
      return false;
    const DataBlock *currentSlot = nullptr;
    const DataBlock *currentPreset = nullptr;
    for (int i = 0; i < weapons.blockCount(); i++) {
      auto curr_block = weapons.getBlock(i);

      if (curr_block->getBlockNameId() == WeaponSlotNid) {
        auto index = curr_block->getInt("index", -1);
        if (index == slot) {
          currentSlot = curr_block;
          break;
        }
      }
    }
    if (!currentSlot)
      return false;
    tier = currentSlot->getInt("tier", -1);
    for (int i = 0; i < currentSlot->blockCount(); i++) {
      auto curr_block = currentSlot->getBlock(i);
      if (curr_block->getBlockNameId() == WeaponPresetNid) {
        auto name = curr_block->getStr("name", "");
        if (strcmp(name, preset) == 0) {
          currentPreset = curr_block;
        }
      }
    }
    if (currentPreset == nullptr)
      return false;
    *WeaponSlot = currentSlot;
    *WeaponPreset = currentPreset;
    return true;
  }

  struct LauncherInfo {
    int order = -1;
    int slot = -1;
    int tier = -1;
    const DataBlock *blk = nullptr;
  };

  void Aircraft::Load() {
    unit_name = raw_unit_name;
    unit::Unit::Load();
    // blkPrint(this->custom_weapons_blk);
    DataBlock empty_blk{};
    if (this->unit_name == "dummy_plane") // fuck the bitch
      return;
    auto vehicle_blk = fmt::format("gamedata/flightmodels/{}.blk", this->unit_name);
    DataBlock blk{};
    if (!dblk::load(blk, vehicle_blk)) {
      LOGE("failed to load unit blk for unit {}", this->unit_name);
      return;
    }
    DataBlock in_file_weapon_preset{}; // even though we are modifying this block in place, the only thing that may be
                                       // affected is the name map
    DataBlock *weapon_preset{};
    if (this->loadout_name.empty()) {
      weapon_preset = &in_file_weapon_preset; // empty block
    } else if (this->loadout_name.starts_with("custom")) {
      weapon_preset = &this->custom_weapons_blk;
    } else {
      auto presets_blk = blk.getBlockByNameEx("weapon_presets");
      auto preset_nid = blk.getNameId("preset");
      for (int i = 0; i < presets_blk->blockCount(); i++) {
        auto preset_blk = presets_blk->getBlock(i);
        if (preset_blk->getBlockNameId() == preset_nid) {
          std::string name = preset_blk->getStr("name", "");
          if (name == this->loadout_name) {
            dblk::load(in_file_weapon_preset,
                       preset_blk->getStr("blk", "")); // we don't care if it actually loads or not
            weapon_preset = &in_file_weapon_preset;
            break;
          }
        }
      }
    }
    if (!weapon_preset) {
      LOGE("missing weapon preset for unit {}", this->unit_name);
      return;
    }
    DataBlock modules_data{};
    DataBlock default_modules{};
    auto modifications_block = blk.getBlockByNameEx("modifications");
    if (!modifications_block) {
      modifications_block = &default_modules;
      return;
    }
    for (auto &mod: this->weapon_mods) {
      if (auto mod_blk = modifications_block->getBlockByNameEx(mod)) {
        if (auto effects = mod_blk->getBlockByNameEx("effects")) {
          for (int i = 0; i < effects->blockCount(); i++) {
            auto effect = effects->getBlock(i);
            modules_data.addNewBlock(effect);
          }
        }
      }
    }
    int commonWeapons_nid = modules_data.getNameId("commonWeapons");
    int weapons_nid = modules_data.getNameId("Weapon");
    bool found = false;
    for (int i = 0; i < modules_data.blockCount(); i++) {
      auto mod_blk = modules_data.getBlock(i);
      if (mod_blk->getBlockNameId() == commonWeapons_nid) {
        found = true;
        for (int j = 0; j < mod_blk->blockCount(); j++) {
          auto weap_blk = mod_blk->getBlock(j);
          if (weap_blk->getBlockNameId() == weapons_nid)
            weapon_preset->addNewBlock(weap_blk);
        }
      }
    }
    if (!found) {
      weapons_nid = blk.getNameId("Weapon");
      auto mod_blk = blk.getBlockByNameEx("commonWeapons");
      if (!mod_blk)
        mod_blk = &empty_blk;
      for (int j = 0; j < mod_blk->blockCount(); j++) {
        auto weap_blk = mod_blk->getBlock(j);
        if (weap_blk->getBlockNameId() == weapons_nid)
          weapon_preset->addNewBlock(weap_blk);
      }
    }
    std::vector<uint16_t> weapons_count{};
    weapons_count.resize(0xFF, 0);
    if (this->unit_wpcost->getBool("hasWeaponSlots", false)) {
      // hasWeaponSlots:b=true denotes that this unit has custom
      // weapon creation which changes how weapons work
      int WeaponSlotNid = blk.getNameId("WeaponSlot"), WeaponPresetNid = blk.getNameId("WeaponPreset");
      int WeaponNid = blk.getNameId("Weapon"), weaponNid = blk.getNameId("weapon");
      auto weapon_blk = blk.getBlockByNameEx("WeaponSlots");
      if (!weapon_blk) {
        LOGE("unable to find WeaponSlots for unit: {}", this->unit_name);
        return;
      }
      std::vector<LauncherInfo> launchers{}; // thats what they technically are idduno deal with it
      // blkPrint(weapon_preset);
      for (int i = 0; i < weapon_preset->blockCount(); i++) {
        auto curr_preset = weapon_preset->getBlock(i);
        const DataBlock *curr_weapon_slot = nullptr, *curr_weapon_preset = nullptr;
        int tier = -1;
        int slot = -1;
        if (!getWeaponPresetAndSlot(*weapon_blk, WeaponSlotNid, WeaponPresetNid, *weapon_preset, i, &curr_weapon_slot,
                                    &curr_weapon_preset, tier, slot)) {
          LOGE("failed to get preset for unit: {}", this->unit_name);
          return;
        }
        int order = curr_weapon_slot->getInt("order", -1);
        for (int j = 0; j < curr_weapon_preset->blockCount(); j++) {
          auto curr_weap = curr_weapon_preset->getBlock(j);
          if (curr_weap->getBlockNameId() == WeaponNid || curr_weap->getBlockNameId() == weaponNid) {
            launchers.emplace_back(order, slot, tier, curr_weap);
          }
        }
      }
      std::stable_sort(launchers.begin(), launchers.end(),
                       [](const LauncherInfo &f, const LauncherInfo &s) { return f.order < s.order; });
      this->weapons.reserve(launchers.size());
      for (auto &launcher: launchers) {
        auto trigger = launcher.blk->getStr("trigger", nullptr);
        auto blk_str = launcher.blk->getStr("blk", nullptr);
        auto emitter = launcher.blk->getStr("emitter", nullptr);
        if (!trigger || !blk_str || !emitter) {
          LOGE("error while making final weapon vector for unit {}", this->unit_name);
          return;
        }
        DataBlock launcher_blk{};
        if (!dblk::load(launcher_blk, blk_str)) {
          LOGE("error while making final weapon vector for unit {}", this->unit_name);
          return;
        }
        int weapon_id = get_weapon_id(trigger);
        int weapon_count = weapons_count[weapon_id];
        weapons_count[weapon_id]++;
        this->weapons.emplace_back(weapon_id, weapon_count, emitter, blk_str);
      }
    } else {

      int WeaponNid = weapon_preset->getNameId("Weapon"), weaponNid = weapon_preset->getNameId("weapon");
      for (int i = 0; i < weapon_preset->blockCount(); i++) {
        auto curr_preset = weapon_preset->getBlock(i);
        if (curr_preset->getBlockNameId() == WeaponNid || curr_preset->getBlockNameId() == weaponNid) {
          auto trigger = curr_preset->getStr("trigger", nullptr);
          auto blk_str = curr_preset->getStr("blk", nullptr);
          auto emitter = curr_preset->getStr("emitter", nullptr);
          if (!trigger || !blk_str || !emitter) {
            LOGE("error while making final weapon vector for unit {}", this->unit_name);
            return;
          }
          DataBlock launcher_blk{};
          if (!dblk::load(launcher_blk, blk_str)) {
            LOGE("error while making final weapon vector for unit {}", this->unit_name);
            return;
          }
          int weapon_id = get_weapon_id(trigger);
          int weapon_count = weapons_count[weapon_id];
          weapons_count[weapon_id]++;
          this->weapons.emplace_back(weapon_id, weapon_count, emitter, blk_str);
        }
      }
    }
    std::sort(this->weapons.begin(), this->weapons.end(), [](const Weapon &f, const Weapon &s) {
      if (f.weapon_id == s.weapon_id)
        return f.weapon_index < s.weapon_index;
      return f.weapon_id < s.weapon_id;
    });
  }

  Weapon *Aircraft::getWeapon(uint32_t ref) {
    if (this->weapons.empty())
      return nullptr;
    uint32_t id = (ref >> 0x10) & 0xFFFF;
    uint32_t index = ref & 0xFFFF;
    for (auto &w: this->weapons) {
      if (w.weapon_id == id && w.weapon_index == index) {
        return &w;
      }
    }
    return nullptr;
  }
  const char *Tank::getUnitTypeName() {
    return "Tank";
  }
  void Tank::Load() {

    unit_name = raw_unit_name.c_str() + 11;
    Unit::Load();
  }


  void unit::Unit::Load() {
    this->unit_wpcost = ecs::g_ecs_data->wp_cost.getBlockByNameEx(this->unit_name);
    this->unit_tags = ecs::g_ecs_data->unit_tags.getBlockByNameEx(this->unit_name);
    name_index_shop = translate::get_locale_index(fmt::format("{}_shop", this->unit_name));
    name_index_0 = translate::get_locale_index(fmt::format("{}_0", this->unit_name));
    name_index_1 = translate::get_locale_index(fmt::format("{}_1", this->unit_name));
    name_index_2 = translate::get_locale_index(fmt::format("{}_2", this->unit_name));
  }

} // namespace unit


mpi::Message *GMReflectable::dispatchMpiMessage(mpi::MessageID mid) {return BaseExtReflectable::dispatchMpiMessage(mid);}
void GMReflectable::applyMpiMessage(const mpi::Message *m) {BaseExtReflectable::applyMpiMessage(m);}

mpi::Message *FMWReflectable::dispatchMpiMessage(mpi::MessageID mid) {return BaseExtReflectable::dispatchMpiMessage(mid);}
void FMWReflectable::applyMpiMessage(const mpi::Message *m) {BaseExtReflectable::applyMpiMessage(m);}

mpi::Message *BaseExtReflectable::dispatchMpiMessage(mpi::MessageID mid) {
  ZoneScoped;
  switch (mid) {
    case MPI_PACKETS::UnitCamera: {
      return state->_new<mpi::CameraStateMessage>(this);
    }
    default:
      break;
  }
  return nullptr;
}
void BaseExtReflectable::applyMpiMessage(const mpi::Message *m) {
  ZoneScoped;
  switch (m->id) {
    case MPI_PACKETS::UnitCamera: {
      auto camera_m = (mpi::CameraStateMessage *) m;

      Point3 camera_euler;
      quat_to_euler(camera_m->camera_circle_quat, camera_euler.y, camera_euler.x, camera_euler.z);
      Point2 gun_pointer = dir_to_sph_ang(camera_m->gun_circle_norm_vector);
      gun_pointer.x -= PI/2;
      camera_euler.y -= PI/2;
      *camera_data.reserveOne() = {camera_euler, gun_pointer };
      camera_data.checkAndPush(state);
      break;
    }
    default:
      break;
  }
}
