// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <network/schemelessEventSerialize.h>
#include <ecs/query/schemelessEvent.h>
#include <network/serialize.h>
#include <daNet/bitStream.h>
#include <ecs/componentTypes.h>
#include <ecs/entityManager.h>
#include "network/Object.h"

namespace ecs {

  void serialize_to(EntityManager &mgr, const SchemelessEvent &evt, BitStream &bs) {
    bs.Write(evt.getType());
    net::BitstreamSerializer cb(mgr, bs);
    ecs::serialize_entity_component_ref_typeless(&evt.getData(), ecs::ComponentTypeInfo<ecs::Object>::type, cb, &mgr);
  }

  MaybeSchemelessEvent deserialize_from(EntityManager &mgr, const BitStream &bs) {
    ecs::event_type_t evtt = 0;
    if (!bs.Read(evtt))
      return MaybeSchemelessEvent{};
    net::BitstreamDeserializer cb(mgr, bs);
    constexpr ecs::component_type_t objType = ecs::ComponentTypeInfo<ecs::Object>::type;
    if (ecs::MaybeComponent mbcomp =
          deserialize_init_component_typeless(objType, ecs::INVALID_COMPONENT_INDEX, cb, &mgr))
      if (mbcomp->getUserType() == objType)
        return SchemelessEvent(evtt, eastl::move(mbcomp->getRW<ecs::Object>()));
    return MaybeSchemelessEvent{};
  }

} // namespace ecs
