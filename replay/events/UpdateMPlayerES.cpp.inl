#include "state/ParserState.h"
#include "mpi/codegen/ReflIncludes.h"
#include "Unit.h"
#include "ecs/query/coreEvents.h"
#include "ecs/ecsCodegen.h"


ECS_AFTER(uid_entity_es)
inline void mplayer_add_entity_es(const ecs::EventEntityCreated &evt, const int &unit__playerId,
                                  const unit::UnitRef &unit__ref, ecs::EntityManager &manager) {
  if (unit__playerId == -1) { // not owned by a player
    return;
  }
  G_ASSERTF(unit__playerId < manager.owned_by->players.size(), "Invalid PLayer index {}", unit__playerId);
  if (unit__ref.unit) {
    manager.owned_by->players[unit__playerId].allOwnedUnits.emplace_back(unit__ref.unit);
  }
}

ECS_AFTER(uid_entity_es)
inline void mplayer_add_entity_es(const ecs::EventEntityCreatedBasic &evt, const ecs::EntityId &eid,
                                  const int &unit__playerId, const unit::UnitRef &unit__ref,
                                  ecs::EntityManager &manager) {
  if (unit__playerId == -1) { // not owned by a player
    return;
  }
  G_ASSERTF(unit__playerId < manager.owned_by->players.size(), "Invalid PLayer index {}", unit__playerId);
  if (unit__ref.unit) {
    manager.owned_by->players[unit__playerId].currentOwnedUnits.emplace(unit__ref.unit);
  }
}

ECS_AFTER(uid_entity_es)
inline void mplayer_add_entity_es(const ecs::EventEntityDestroyedBasic &evt, const ecs::EntityId &eid,
                                  const int &unit__playerId, const unit::UnitRef &unit__ref,
                                  ecs::EntityManager &manager) {

  if (unit__playerId == -1) { // not owned by a player
    return;
  }
  G_ASSERTF(unit__playerId < manager.owned_by->players.size(), "Invalid PLayer index {}", unit__playerId);
  if (unit__ref.unit) {
    manager.owned_by->players[unit__playerId].currentOwnedUnits.erase(unit__ref.unit);
  }
}