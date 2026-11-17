#include "state/internal/share.h"
#include "mpi/codegen/ReflIncludes.h"
#include "Unit.h"
#include "ecs/query/coreEvents.h"
#include "ecs/ecsCodegen.h"
struct SHARE_STRUCT(MPlayerES) {
  static auto &Players(ParserState &state) { return parserInt::ParserShare::get_m_players(state); }
};

struct alias : public SHARE_STRUCT(MPlayerES) {};

ECS_AFTER(uid_entity_es)
inline void mplayer_add_entity_es(const ecs::EventEntityCreated &evt, const int &unit__playerId,
                                  const unit::UnitRef &unit__ref, ecs::EntityManager &manager) {
  if (unit__playerId == -1) { // not owned by a player
    return;
  }
  auto &players = alias::Players(*manager.owned_by);
  G_ASSERTF(unit__playerId < players.size(), "Invalid PLayer index {}", unit__playerId);
  if (unit__ref.unit) {
    players[unit__playerId].allOwnedUnits.emplace_back(unit__ref.unit);
  }
}

ECS_AFTER(uid_entity_es)
inline void mplayer_add_entity_es(const ecs::EventEntityCreatedBasic &evt, const ecs::EntityId &eid,
                                  const int &unit__playerId, const unit::UnitRef &unit__ref,
                                  ecs::EntityManager &manager) {
  if (unit__playerId == -1) { // not owned by a player
    return;
  }
  auto &players = alias::Players(*manager.owned_by);
  G_ASSERTF(unit__playerId < players.size(), "Invalid PLayer index {}", unit__playerId);
  if (unit__ref.unit) {
    players[unit__playerId].currentOwnedUnits.emplace(unit__ref.unit);
  }
}

ECS_AFTER(uid_entity_es)
inline void mplayer_add_entity_es(const ecs::EventEntityDestroyedBasic &evt, const ecs::EntityId &eid,
                                  const int &unit__playerId, const unit::UnitRef &unit__ref,
                                  ecs::EntityManager &manager) {

  if (unit__playerId == -1) { // not owned by a player
    return;
  }
  auto &players = alias::Players(*manager.owned_by);
  G_ASSERTF(unit__playerId < players.size(), "Invalid PLayer index {}", unit__playerId);
  if (unit__ref.unit) {
    players[unit__playerId].currentOwnedUnits.erase(unit__ref.unit);
  }
}
