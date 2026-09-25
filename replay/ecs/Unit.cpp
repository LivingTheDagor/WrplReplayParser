#include "Unit.h"
#include "idFieldSerializer.h"
#include "ecs/ComponentTypesDefs.h"
#include "ecs/EntityManager.h" // g_ecs_state
#include "FileSystem.h"
#include "math/dag_mathAng.h"
#include "res/grpManager.h"

#include <algorithm>
#include <unordered_map>
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
  struct weap {
    int id;
    const char *name;
  };

  void blkPrint(const DataBlock *blk) {
    blk->printBlock(std::cout);
    std::cout.flush();
  }

  unit::TurretTree::TurretTree(GeomNodeTree *tree) : tree(tree) {
    nodes.reserve(tree->nodeCount() + 1);
    size_t last_size = nodes.capacity();
    for (GeomNodeTree::Index16 i(0), ie(tree->nodeCount()); i != ie; ++i) {
      auto name = tree->getNodeName(i);
      if (name == nullptr) {
        name = "root"; // name stored in static mem, so should always exist
      }

      nodes.emplace_back(i, tree->getParentNodeIdx(i));
      G_ASSERT(nodes.capacity() == last_size);
      name_to_idx[name] = &nodes.back();
    }
  }
  TurretNode *TurretTree::getNode(std::string_view name) {
    auto iter = name_to_idx.find(name);
    if (iter != name_to_idx.end()) {
      return iter->second;
    }
    return nullptr;
  }
  TurretNode *TurretTree::getUsefulNode(std::string_view name) {
    auto node = getNode(name);
    if (!node)
      return nullptr;
    auto iter_node = node;
    while (iter_node) {
      iter_node->is_useful_node = true;
      if (iter_node->parent_index) {
        iter_node = &nodes[iter_node->parent_index.index()];
      } else {
        break;
      }
    }
    return node;
  }

  void unit::TurretTree::doAbsUpdate() {
    ZoneScopedN("TurretTree::doAbsUpdate");
    if (nodes.empty())
      return;

    if (!useful_built) {
      useful_built = true;
      for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i].is_useful_node)
          useful_order.push_back((uint16_t) i);
      }
    }

    // Nodes are stored in tree index order, so parents always precede children — one forward pass is sufficient.
    for (uint16_t node_idx: useful_order) {
      auto &node = nodes[node_idx];
      GeomNodeTree::Index16 parent_idx = node.parent_index;

      TurretNode *parent = nullptr;
      Point3 parent_abs = {0.f, 0.f, 0.f};
      if (parent_idx) { // non-root
        parent = &nodes[parent_idx.index()];
        parent_abs = parent->turret_abs;
      }

      node.turret_abs = parent_abs + node.turret_rel;
      if (node.turret_rel == Point3{0.f, 0.f, 0.f}) {
        continue;
      }
      // if (!is_head_node) {
      //   node.turret_abs = parent_abs + node.turret_rel;
      // } else {
      //  only accumulate Y rotation, ignore X and Z
      //}

      // Wrap each axis into [-PI, PI]
      {
        ZoneScopedN("TurretTree::normalizing");
        node.turret_abs.x = norm_s_ang(node.turret_abs.x);
        node.turret_abs.y = norm_s_ang(node.turret_abs.y);
        node.turret_abs.z = norm_s_ang(node.turret_abs.z);
      }
    }
  }

  const char *Aircraft::getUnitTypeName() { return "Aircraft"; }

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

  constexpr int GUNNER_WEAPON_ID_BASE = 0x13;

  std::string get_weapon_class(int weapon_id) {
    // 0x12 is targetingPod; the game's ground-weapon range starts after it.
    if (weapon_id >= GUNNER_WEAPON_ID_BASE)
      return fmt::format("gunner{}", weapon_id - GUNNER_WEAPON_ID_BASE);
    for (auto &[id, name]: weapon_id_match) {
      if (id == weapon_id)
        return name;
    }
    return {};
  }

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
      return atoi(weapon_name.data() + 6) + GUNNER_WEAPON_ID_BASE;
    }
    return -1;
  }

#define RET_FAIL(opt) G_ASSERT_RETURN(opt, false)

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
  std::string getGoodSkelName(const std::string &str) {
    std::string payload;
    if (str.ends_with("_a"))
      payload = fmt::format("{}_skeleton", str.substr(0, str.length() - 2));
    else
      payload = fmt::format("{}_skeleton", str);
    return payload;
  }

  void TurretDesc::push_curr_state(ParserState &state) {
    ZoneScopedN("TurretDesc::push_curr_state");
    TurretData data{};
    data.rel = head->turret_rel;
    data.abs = head->turret_abs;
    data.gun_rel = gun->turret_rel;
    data.gun_abs = gun->turret_abs;
    *turret_state.reserveOne() = data;
    turret_state.checkAndPush(&state);
  }

  std::string parse_weapon_container(const DataBlock *blk) {
    auto blk_val = blk->getStr("blk", nullptr);
    if (!blk_val)
      return {};
    // container:b=true sits inside the file being pointed at, not in the block doing
    // the pointing, so the flag has to be read after loading it. Checking the outer
    // block only unwraps a container that is already nested inside another one, and
    // leaves the first level as the container itself: the Pantsir's TKB-1055 stayed
    // 170mm_tkb_1055_container, which has no name of its own and no turret.
    //
    // Every weapon of every unit comes through here, so the resolved path is cached:
    // without it the same blk is loaded once per weapon, and twice for a pilon slot,
    // which is read once for the dedup and again by the Weapon ctor.
    static std::unordered_map<std::string, std::string> resolved;
    const auto hit = resolved.find(blk_val);
    if (hit != resolved.end())
      return hit->second;

    std::string out = blk_val;
    // A container pointing at itself would recurse forever, and nothing in the game
    // files rules that out.
    for (int depth = 0; depth < 8; ++depth) {
      DataBlock inner{};
      if (!dblk::load(inner, out, dblk::ReadFlags(dblk::ReadFlag::ROBUST)) ||
          !inner.getBool("container", false))
        break;
      auto next = inner.getStr("blk", nullptr);
      if (!next || out == next)
        break;
      out = next;
    }
    resolved.emplace(blk_val, out);
    return out;
  }

  Weapon::Weapon(const DataBlock *blk, Unit *unit, std::vector<uint16_t> &weapons_count) {
    auto trigger = blk->getStr("trigger", nullptr);
    auto blk_str = blk->getStr("blk", nullptr);
    auto _emitter = blk->getStr("emitter", nullptr);
    if (!trigger || !blk_str || !_emitter) {
      LOGE("error while making final weapon vector for unit {}", unit->unit_name);
      return;
    }
    this->weapon_id = get_weapon_id(trigger);
    if (this->weapon_id < 0 || (size_t) this->weapon_id >= weapons_count.size()) {
      LOGE("unknown weapon trigger {} for unit {}", trigger, unit->unit_name);
      this->weapon_id = -1;
      return;
    }
    this->weapon_index = weapons_count[weapon_id];
    this->emitter = _emitter;
    this->blk_path = parse_weapon_container(blk);
    fs::path blk_fs_path = this->blk_path;
    this->weapon_name = blk_fs_path.filename().string();
    if (this->weapon_name.ends_with(".blk")) {
      this->weapon_name.erase(this->weapon_name.length() - 4);
    }
    this->name_index = translate::get_locale_index(fmt::format("weapons/{}", this->weapon_name));
    this->name_index_short = translate::get_locale_index(fmt::format("weapons/{}/short", this->weapon_name));
    if (this->weapon_name != "dummy_weapon" && strcmp("boosters", trigger) != 0)
      G_ASSERT(this->name_index != translate::INVALID_TRANSLATE_INDEX);

    weapons_count[weapon_id]++;
    if (unit->hasTree()) {
      this->loadTurretData(blk, unit->turret_tree.get());
    }
  }

  void Weapon::loadTurretData(const DataBlock *weap_blk, TurretTree *tree) {
    auto turret_blk = weap_blk->getBlockByName("turret");
    if (!turret_blk)
      return;

    auto head_str = turret_blk->getStr("head", nullptr);
    auto gun_str = turret_blk->getStr("gun", nullptr);
    if (!head_str || !gun_str)
      return;
    auto head_node = tree->getUsefulNode(head_str);
    auto gun_node = tree->getUsefulNode(gun_str);
    if (!head_node || !gun_node)
      return;
    turret_desc = std::make_unique<TurretDesc>(head_node, gun_node);
  }

  void Unit::loadWeaponData(const std::string &path) {
    ZoneScopedN("Unit::loadWeaponData");
    if (this->unit_name == "dummy_plane") // fuck the bitch
      return;
    DataBlock blk{};
    if (!dblk::load(blk, path)) {
      LOGE("failed to load unit blk for unit {}", this->unit_name);
      return;
    }
    auto model_name = blk.getStr("model", "");
    auto skel_name = getGoodSkelName(model_name);
    has_tree = g_grp_manager.getTree(skel_name, geom_tree);
    if (has_tree) {
      this->turret_tree = std::make_unique<unit::TurretTree>(&geom_tree);
    }
    // The damage model is a skeleton of its own, next to the visual one in the same
    // pack. Its _dm nodes are what the hit packets number, but the two counts agree on
    // a minority of vehicles - 10 of the 42 measured on one battle - so the list goes
    // out whole and the caller decides what to count.
    // The tree is read for its node names and dropped: keeping it would hold the node
    // matrices of every damaged vehicle for the whole parse, and nothing reads them.
    GeomNodeTree damage_tree{};
    if (g_grp_manager.getTree(fmt::format("{}_dm_skeleton", model_name), damage_tree)) {
      damage_parts.reserve(damage_tree.nodeCount());
      for (GeomNodeTree::Index16 i(0), ie(damage_tree.nodeCount()); i != ie; ++i) {
        // The super root of every skeleton has no name. Four names in the whole game
        // carry a stray high byte - composite_armor_turret_03_dm1û of the cn_vt_4b
        // among them - which is not valid UTF-8 and would throw on the way into python.
        // Dropping the byte keeps the name readable; dropping the name would lose a
        // part the hit packets do number.
        const char *name = damage_tree.getNodeName(i);
        std::string out{};
        for (const char ch: std::string_view(name ? name : ""))
          if (ch >= 0x20 && ch < 0x7f)
            out.push_back(ch);
        damage_parts.emplace_back(std::move(out));
      }
    }
    // if (this->AsTank())
    //   G_ASSERT(geom_tree.nodeCount() > 0);
    DataBlock in_file_weapon_preset{};
    // even though we are modifying this block in place, the only thing that may be
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
        this->weapons.emplace_back(launcher.blk, this, weapons_count);
      }
    } else {
      int WeaponNid = weapon_preset->getNameId("Weapon"), weaponNid = weapon_preset->getNameId("weapon");
      for (int i = 0; i < weapon_preset->blockCount(); i++) {
        auto curr_preset = weapon_preset->getBlock(i);
        if (curr_preset->getBlockNameId() == WeaponNid || curr_preset->getBlockNameId() == weaponNid) {
          this->weapons.emplace_back(curr_preset, this, weapons_count);
        }
      }
    }
    // WeaponPilons is a third place a weapon can hide, next to commonWeapons and the
    // preset. Only one vehicle in the game uses it, the Pantsir SM-SV: its launcher
    // is not a weapon of the hull but a slot per missile type, and the three types
    // are modifications the crew mounts. Without this pass the vehicle has cannons
    // and a smoke launcher and nothing to fire its missiles from, and every missile
    // it launches fails to resolve its launcher.
    if (auto pilons = blk.getBlockByName("WeaponPilons")) {
      const int WeaponSlotNid = pilons->getNameId("WeaponSlot");
      const int WeaponPresetNid = pilons->getNameId("WeaponPreset");
      const int WeaponNid = pilons->getNameId("Weapon"), weaponNid = pilons->getNameId("weapon");
      for (int i = 0; i < pilons->blockCount(); i++) {
        auto slot = pilons->getBlock(i);
        if (slot->getBlockNameId() != WeaponSlotNid)
          continue;
        for (int j = 0; j < slot->blockCount(); j++) {
          auto preset = slot->getBlock(j);
          if (preset->getBlockNameId() != WeaponPresetNid)
            continue;
          for (int k = 0; k < preset->blockCount(); k++) {
            auto weap = preset->getBlock(k);
            if (weap->getBlockNameId() != WeaponNid && weap->getBlockNameId() != weaponNid)
              continue;
            // One slot per munition, not per launcher: the Pantsir's three slots all
            // resolve to the same launcher blk and the same emitter, and differ only
            // in which missile the crew mounted. Keeping all three would put three
            // identical barrels on the vehicle, so slots are taken once per launcher.
            const std::string path = parse_weapon_container(weap);
            bool seen = false;
            for (auto &w: this->weapons)
              seen |= w.from_pilon && w.blk_path == path;
            if (seen)
              continue;
            this->weapons.emplace_back(weap, this, weapons_count);
            // The ctor can bail out and still leave the object in the vector. Marking
            // such a weapon as a pilon one would make it the fallback of
            // getWeaponFromRef for every unresolved ref of this vehicle, handing out a
            // weapon with no blk, no name and no turret.
            if (this->weapons.back().weapon_id < 0)
              this->weapons.pop_back();
            else
              this->weapons.back().from_pilon = true;
          }
        }
      }
    }
    std::sort(this->weapons.begin(), this->weapons.end(), [](const Weapon &f, const Weapon &s) {
      if (f.weapon_id == s.weapon_id)
        return f.weapon_index < s.weapon_index;
      return f.weapon_id < s.weapon_id;
    });
  }
  void Unit::loadTurretData(Weapon &weapon, const DataBlock *weap_blk) {
    if (!this->has_tree)
      return;
    weapon.loadTurretData(weap_blk, this->turret_tree.get());
  }

  bool unit::Unit::LoadFromStorage(const FieldSerializerDict &data) {
    BitStream bs{data.data.data(), data.data.size(), false};
    IdFieldSerializer255 IdFieldSerializer{};
    uint32_t end;
    uint16_t count = IdFieldSerializer.readFieldsSizeAndCount(bs, end);
    if (!IdFieldSerializer.readFieldsIndex(bs)) {
      return false;
    }
    // std::ostringstream ret = FormatHexToStream(data.data);
    // LOGI("{}", ret.str());
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
          RET_FAIL(sz <= 6);
          // it should always be 6 currently, so if anything weird has happened best to know
          storage_weapons.resize(sz);
          for (auto &weapon: storage_weapons) {
            RET_FAIL(t_bs.Read(weapon.launcher));
            RET_FAIL(t_bs.Read(weapon.bullet));
            RET_FAIL(t_bs.Read(weapon.count));
            RET_FAIL(t_bs.Read(weapon.unk));
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


  void Aircraft::Load() {
    unit_name = raw_unit_name;
    unit::Unit::Load();
    auto vehicle_blk = fmt::format("gamedata/flightmodels/{}.blk", this->unit_name);
    loadWeaponData(vehicle_blk);
  }

  Weapon *Unit::getWeaponFromRef(uint32_t ref) {
    if (this->weapons.empty())
      return nullptr;
    uint32_t id = (ref >> 0x10) & 0xFFFF;
    uint32_t index = ref & 0xFFFF;
    for (auto &w: this->weapons) {
      if (w.weapon_id == id && w.weapon_index == index) {
        return &w;
      }
    }
    // A pilon slot does not keep the id its blk declares: on the Pantsir SM-SV all
    // three slots say trigger gunner1, id 18, while the server refers to them as 22
    // and 23. The id it does use is not derivable from the game files, so the exact
    // slot cannot be picked. It does not have to be: every slot of that block points
    // at the same launcher blk, the container of the third redirecting to it, so the
    // launcher, its emitter and its turret are the same whichever slot fired. What
    // differs is the munition, and that is named by the battle report, not here.
    // Only for the ids the server actually uses for such a slot: those are gunner
    // mounts, 0x13 and up. A miss on a lower id is a miss on a hull weapon and gets
    // no answer rather than a wrong one.
    if (id >= GUNNER_WEAPON_ID_BASE) {
      for (auto &w: this->weapons) {
        if (w.from_pilon)
          return &w;
      }
    }
    return nullptr;
  }

  const char *Tank::getUnitTypeName() { return "Tank"; }

  void Tank::Load() {
    std::string_view raw{raw_unit_name};
    if (raw.starts_with("tracked_vehicles/")) {
      unit_name = raw.substr(17);
    } else if (raw.size() >= 11) {
      unit_name = raw.substr(11);
    } else {
      unit_name = raw_unit_name;
    }
    Unit::Load();
    auto vehicle_blk = fmt::format("gamedata/units/{}.blk", this->raw_unit_name);
    loadWeaponData(vehicle_blk);
  }


  void unit::Unit::Load() {
    this->unit_wpcost = ecs::g_ecs_data->wp_cost.getBlockByNameEx(this->unit_name);
    this->unit_tags = ecs::g_ecs_data->unit_tags.getBlockByNameEx(this->unit_name);
    name_index_shop = translate::get_locale_index(fmt::format("{}_shop", this->unit_name));
    name_index_0 = translate::get_locale_index(fmt::format("{}_0", this->unit_name));
    name_index_1 = translate::get_locale_index(fmt::format("{}_1", this->unit_name));
    name_index_2 = translate::get_locale_index(fmt::format("{}_2", this->unit_name));
  }
  Weapon *Unit::getWeapon(uint16_t idx) {
    if (idx >= this->weapons.size())
      return nullptr;
    return &this->weapons[idx];
  }
  void Unit::calculateTurretData() {
    ZoneScopedN("Unit::calculateTurretData");
    if (!this->has_tree || !this->positions.hasData())
      return;

    Point3 currEuler = this->positions.curr()->euler;
    float roll = currEuler.x;
    float yaw = currEuler.y;
    float pitch = currEuler.z;

    yaw = norm_s_ang(yaw);
    pitch = norm_s_ang(pitch);
    roll = norm_s_ang(roll);
    this->turret_tree->nodes[0].turret_rel = Point3(yaw, pitch, roll);
    this->turret_tree->doAbsUpdate();
  }
} // namespace unit


mpi::Message *GMReflectable::dispatchMpiMessage(mpi::MessageID mid) {
  return BaseExtReflectable::dispatchMpiMessage(mid);
}

void GMReflectable::applyMpiMessage(const mpi::Message *m) { BaseExtReflectable::applyMpiMessage(m); }

mpi::Message *FMWReflectable::dispatchMpiMessage(mpi::MessageID mid) {
  return BaseExtReflectable::dispatchMpiMessage(mid);
}

void FMWReflectable::applyMpiMessage(const mpi::Message *m) { BaseExtReflectable::applyMpiMessage(m); }

mpi::Message *BaseExtReflectable::dispatchMpiMessage(mpi::MessageID mid) {
  ZoneScoped;
  switch (mid) {
    case MPI_PACKETS::UnitCamera: {
      return state->_new<mpi::CameraStateMessage>(this);
    }
    case MPI_PACKETS::UnitHitEffects: {
      return state->_new<mpi::UnitHitEffectsMessage>(this);
    }
    case MPI_PACKETS::UnitHitAnalysis: {
      return state->_new<mpi::UnitHitAnalysisMessage>(this);
    }
    case MPI_PACKETS::UnitOnEffectiveHit:
    case MPI_PACKETS::UnitOnEffectiveCritHit: {
      return state->_new<mpi::UnitOnEffectiveHitMessage>(this, mid);
    }
    case MPI_PACKETS::UnitOnHit: {
      return state->_new<mpi::UnitOnHitMessage>(this);
    }
    case MPI_PACKETS::UnitOnExplosion: {
      return state->_new<mpi::UnitOnExplosionMessage>(this);
    }
    case MPI_PACKETS::UnitLastEffectiveHit: {
      return state->_new<mpi::UnitLastEffectiveHitMessage>(this);
    }
    case MPI_PACKETS::UnitBulletRearm: {
      return state->_new<mpi::UnitBulletRearmMessage>(this);
    }
    case MPI_PACKETS::UnitSingleShot:
    case MPI_PACKETS::GmDoStartFire:
    case MPI_PACKETS::GmDoStopFire: {
      return state->_new<mpi::UnitShotMessage>(this, mid);
    }
    default: break;
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
      camera_euler.z = -camera_euler.z;
      Point2 gun_pointer = dir_to_sph_ang(camera_m->gun_circle_norm_vector);
      gun_pointer.x -= PI / 2;
      camera_euler.y = norm_s_ang(camera_euler.y - PI / 2);
      *camera_data.reserveOne() = {camera_euler, gun_pointer};
      camera_data.checkAndPush(state);
      break;
    }
    // Hit packets are stored as they came. Joining them by projectile id is left
    // to the consumer; only the offender lookup has to happen here, while the
    // uid still maps to the unit that owns it.
    case MPI_PACKETS::UnitHitEffects: {
      auto &rec = state->HitEffects.emplace_back(((const mpi::UnitHitEffectsMessage *) m)->hit);
      rec.offended_unit = owner_unit;
      break;
    }
    case MPI_PACKETS::UnitHitAnalysis: {
      auto &rec = state->HitAnalyses.emplace_back(((const mpi::UnitHitAnalysisMessage *) m)->analysis);
      rec.offended_unit = owner_unit;
      break;
    }
    case MPI_PACKETS::UnitOnEffectiveHit:
    case MPI_PACKETS::UnitOnEffectiveCritHit: {
      auto &rec = state->HitDamages.emplace_back(((const mpi::UnitOnEffectiveHitMessage *) m)->damage);
      rec.offended_unit = owner_unit;
      break;
    }
    case MPI_PACKETS::UnitOnHit: {
      auto &rec = state->HitDirections.emplace_back(((const mpi::UnitOnHitMessage *) m)->direction);
      rec.offended_unit = owner_unit;
      break;
    }
    case MPI_PACKETS::UnitOnExplosion: {
      auto &rec = state->HitExplosions.emplace_back(((const mpi::UnitOnExplosionMessage *) m)->explosion);
      rec.offended_unit = owner_unit;
      break;
    }
    case MPI_PACKETS::UnitLastEffectiveHit: {
      auto &rec = state->HitOutcomes.emplace_back(((const mpi::UnitLastEffectiveHitMessage *) m)->outcome);
      rec.offended_unit = owner_unit;
      break;
    }
    case MPI_PACKETS::UnitBulletRearm: {
      auto &rec = state->AmmoEvents.emplace_back(((const mpi::UnitBulletRearmMessage *) m)->ammo);
      rec.unit = owner_unit;
      break;
    }
    case MPI_PACKETS::UnitSingleShot:
    case MPI_PACKETS::GmDoStartFire:
    case MPI_PACKETS::GmDoStopFire: {
      auto &rec = state->ShotEvents.emplace_back(((const mpi::UnitShotMessage *) m)->shot);
      rec.unit = owner_unit;
      break;
    }
    default: break;
  }
}
