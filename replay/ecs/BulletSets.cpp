#include "Unit.h"

#include <ioSys/dag_dataBlock.h>

#include <unordered_map>

// What a loadout can put in a gun. The player picks these before the battle, the game
// carries the pick in Unit::storage_weapons, and the ammo counter reports what is left
// of each one; none of that names the round itself, only the block in the gun blk.
//
// Two different things wear that name. A gun loaded one shell at a time - a tank cannon -
// has a block per shell, and the block names it: 122mm_ussr_APHE holds 122mm_br_471b,
// which the game calls BR-471B. A belt fed gun - every machine gun, every autocannon,
// every gun an aircraft carries - has a block per belt, and a belt is a fixed mix of
// round types that is never broken up, so no round in it can stand for the belt. There
// the block name is the belt, and the game names it as a modification.
//
// Telling them apart needs two things, because neither alone holds. The bullets count is
// the rounds a belt holds and is -1 on a cannon, but gunType97 carries belts and still
// says -1. A belt usually holds several bullets and names none of them, but 12mm_usa_
// M2HB_APIT is a belt of one named round. So a gun is belt fed when its count says so or
// when any one of its blocks looks like a belt, and then all of its blocks are belts.
namespace unit {

  namespace {
    /// What the game calls the belt a gun comes with. It has no block in the gun blk, so
    /// nothing there names it, and this one key covers every gun.
    constexpr const char *STOCK_BELT_KEY = "modification/default_bullets";

    /// Belts are named as modifications and the game spells that key three ways, all of
    /// which are in use: over four replays 37 belts resolve through modification/<name>
    /// and 15 through <name>/name. INVALID_TRANSLATE_INDEX when the game names the belt
    /// nowhere, which is 36 of them - no csv of lang.vromfs holds a key for those.
    translate::translate_index_t beltNameIndex(const std::string &name) {
      for (const std::string &key: {"modification/" + name + "/short", "modification/" + name, name + "/name"}) {
        const auto idx = translate::get_locale_index(key);
        if (idx != translate::INVALID_TRANSLATE_INDEX)
          return idx;
      }
      return translate::INVALID_TRANSLATE_INDEX;
    }

    struct SetRounds {
      std::vector<std::string> rounds{};
      bool named = true; ///< every round of the set carries a bulletName of its own
    };

    /// Every round of a set, in blk order, named the way the battle report names it.
    ///
    /// Two keys hold that name and which one is filled depends on the gun. A cannon
    /// spells both: bulletName is the shell (90mm_m82) and bulletType its class
    /// (apcbc_tank), and the report says the former. An aircraft belt spells only
    /// bulletType, and there it is the round itself (he_i, ap_t, sapi) - which is
    /// exactly what the report says for those. So the name is bulletName when there is
    /// one and bulletType otherwise, and both cases match the report.
    SetRounds setRounds(const DataBlock *set, int bullet_nid) {
      SetRounds out;
      for (int i = 0; i < set->blockCount(); ++i) {
        const DataBlock *body = set->getBlock(i);
        if (body->getBlockNameId() != bullet_nid)
          continue;
        const char *name = body->getStr("bulletName", nullptr);
        if (!name || !*name) {
          out.named = false;
          name = body->getStr("bulletType", nullptr);
        }
        if (name && *name)
          out.rounds.emplace_back(name);
      }
      return out;
    }

    const std::vector<BulletSet> &load(const std::string &blk_path) {
      static std::unordered_map<std::string, std::vector<BulletSet>> cache;
      auto it = cache.find(blk_path);
      if (it != cache.end())
        return it->second;
      std::vector<BulletSet> out{};
      DataBlock blk{};
      // Most guns have a blk, but not all of them do, and a plain load on a missing file
      // is fatal rather than false.
      if (dblk::load(blk, blk_path.c_str(), dblk::ReadFlags(dblk::ReadFlag::ROBUST))) {
        const int bullet_nid = blk.getNameId("bullet");
        std::vector<const DataBlock *> sets{};
        bool belt_fed = blk.getInt("bullets", -1) > 0;
        std::vector<SetRounds> rounds{};
        for (int i = 0; i < blk.blockCount(); ++i) {
          const DataBlock *sub = blk.getBlock(i);
          // The gun's own default rounds sit in a bullet block of the top level; a
          // loadout never names those, it names the blocks around them.
          if (sub->getBlockNameId() == bullet_nid || !sub->getBlockByName("bullet"))
            continue;
          sets.push_back(sub);
          rounds.push_back(setRounds(sub, bullet_nid));
          belt_fed |= rounds.back().rounds.size() != 1;
        }
        // The stock load the gun comes with. A loadout line names no block for it -
        // there is none to name - so it answers to the empty name the line carries.
        if (blk.getBlockByName("bullet")) {
          BulletSet &set = out.emplace_back();
          // The stock rounds sit in bullet blocks of the top level, so the whole gun blk
          // is the set here.
          const SetRounds stock = setRounds(&blk, bullet_nid);
          set.rounds = stock.rounds;
          if (!belt_fed && set.rounds.size() == 1 && stock.named) {
            set.shell = set.rounds.front();
            set.name_index = translate::get_locale_index(set.shell);
          } else {
            // The stock belt has no block of its own to be named by, but the game does
            // name it: it is the Default of the belt list, keyed once for every gun.
            set.name_index = translate::get_locale_index(STOCK_BELT_KEY);
          }
        }
        for (size_t i = 0; i < sets.size(); ++i) {
          BulletSet &set = out.emplace_back();
          set.name = blk.getName(sets[i]->getBlockNameId());
          set.rounds = std::move(rounds[i].rounds);
          if (!belt_fed && set.rounds.size() == 1 && rounds[i].named) {
            set.shell = set.rounds.front();
            set.name_index = translate::get_locale_index(set.shell);
          } else {
            set.name_index = beltNameIndex(set.name);
          }
        }
      }
      return cache.emplace(blk_path, std::move(out)).first->second;
    }
  } // namespace

  const std::vector<BulletSet> &Weapon::bulletSets() const { return load(this->blk_path); }

} // namespace unit
