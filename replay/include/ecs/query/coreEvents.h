

#ifndef WTFILEUTILS_COREEVENTS_H
#define WTFILEUTILS_COREEVENTS_H

#include "event.h"

namespace ecs {
  // does expensive stuff / allcations. should only be called during inital parse and no more
  ECS_UNICAST_EVENT_TYPE(EventEntityCreated)
  // basic actions when an entity is created. can be called multiple times based on rewind actions
  ECS_UNICAST_EVENT_TYPE(EventEntityCreatedBasic)
  // does more permanent stuff like actual UnitRef.unit destruction
  ECS_UNICAST_EVENT_TYPE(EventEntityDestroyed)
  // basic destruction, only does minimal, like remove uid lookup entry, also only called
  // first arg is the eid the entity was moved to
  // second arg is if this is true destruction or not
  // this is used during rewinding to mark if this was the 'destruction' caused by undoing creation or if its the actual
  // destruction
  ECS_UNICAST_EVENT_TYPE(EventEntityDestroyedBasic, ecs::EntityId, bool)

  // event called during a rewind event, arg is time_ms changing to
  ECS_BROADCAST_EVENT_TYPE(EventRewind, uint32_t)
} // namespace ecs


#endif // WTFILEUTILS_COREEVENTS_H
