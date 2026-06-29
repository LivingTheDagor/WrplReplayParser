#include "network/Connection.h"
#include "network/message.h"
#ifndef MYEXTENSION_OBJECT_H
#define MYEXTENSION_OBJECT_H

namespace net {
  class Object {
  public:
    explicit Object(ecs::EntityId eid_);
    static Object *getByEid(ecs::EntityId, ecs::EntityManager *);
    bool deserializeComps(const BitStream &bs, Connection *conn);
    bool operator==(const Object &other) const = default;

  protected:
    ecs::EntityId eid;
  };


} // namespace net
ECS_DECLARE_BASE_TYPE(net::Object, "net::Object", true);
#endif // MYEXTENSION_OBJECT_H
