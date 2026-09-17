//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ecs/query/schemelessEvent.h>
#include <EASTL/optional.h>


class BitStream;
namespace ecs {

  class EntityManager;
  struct SchemelessEvent;
  typedef eastl::optional<SchemelessEvent> MaybeSchemelessEvent;
  void serialize_to(EntityManager &mgr, const SchemelessEvent &, BitStream &bs);
  MaybeSchemelessEvent deserialize_from(EntityManager &mgr, const BitStream &bs);

} // namespace ecs
