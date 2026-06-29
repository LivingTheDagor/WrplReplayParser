

#ifndef MYEXTENSION_EID_H
#define MYEXTENSION_EID_H
#include "ecs/typesAndLimits.h"
#include "ecs/entityId.h"
#include "danet/BitStream.h"


namespace net {
  bool write_server_eid(ecs::entity_id_t eidVal, BitStream &bs);
  bool read_server_eid(ecs::entity_id_t &eidVal, const BitStream &bs);
  void write_eid(BitStream &bs, ecs::EntityId eid);
  bool read_eid(const BitStream &bs, ecs::EntityId &eid);
} // namespace net
#endif // MYEXTENSION_EID_H
