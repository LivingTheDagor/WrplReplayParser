#include "network/Object.h"

namespace net {
  net::Object *Object::getByEid(ecs::EntityId eid_, ecs::EntityManager *mgr) {
    return mgr->getNullable<net::Object>(eid_, ECS_HASH("replication"));
  }


#define REPL_BSREAD(x) \
  if (!bs.Read(x))     \
    return failRet;

  bool Object::deserializeComps(const BitStream &bs, Connection *conn) {
    // G_ASSERTF(isReplica(), "%d", (ecs::entity_id_t)eid);

    bool failRet = false;
    do {
      bool exist = false;
      REPL_BSREAD(exist);
      if (!exist)
        break;

      if (!conn->deserializeComponentReplication(eid, bs))
        return false;
    } while (1);
    return true;
  }

  Object::Object(ecs::EntityId eid_) { this->eid = eid_; }

  ECS_REGISTER_CTM_TYPE(net::Object, nullptr);
  ECS_AUTO_REGISTER_COMPONENT_BASE(net::Object, "replication", nullptr)
} // namespace net

namespace ecs {
  ECS_REGISTER_EVENT(EventNetMessage);
}
