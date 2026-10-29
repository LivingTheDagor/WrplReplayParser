

#ifndef MYEXTENSION_EID_H
#define MYEXTENSION_EID_H
#include "ecs/typesAndLimits.h"
#include "ecs/entityId.h"
#include "daNet/bitStream.h"


namespace net {
  bool write_server_eid(ecs::entity_id_t eidVal, danet::BitStream &bs);
  bool read_server_eid(ecs::entity_id_t &eidVal, const danet::BitStream &bs);
  void write_eid(danet::BitStream &bs, ecs::EntityId eid);
  bool read_eid(const danet::BitStream &bs, ecs::EntityId &eid);
} // namespace net
#endif // MYEXTENSION_EID_H
