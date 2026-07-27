

#ifndef MYEXTENSION_OBJECTDISPATCHER_H
#define MYEXTENSION_OBJECTDISPATCHER_H

#include "codegen/ReflIncludes.h"
#include "types.h"

DEFINE_HANDLE(handle_object_dispatcher)
#define DISPATCHER_LOGI(format_, ...)  ELOGI(handle_object_dispatcher, format_, __VA_ARGS__)
#define DISPATCHER_LOGD1(format_, ...) ELOGD1(handle_object_dispatcher, format_, __VA_ARGS__)
#define DISPATCHER_LOGD2(format_, ...) ELOGD2(handle_object_dispatcher, format_, __VA_ARGS__)
#define DISPATCHER_LOGD3(format_, ...) ELOGD3(handle_object_dispatcher, format_, __VA_ARGS__)
#define DISPATCHER_LOGE(format_, ...)  ELOGE(handle_object_dispatcher, format_, __VA_ARGS__)

struct ParserState;
namespace mpi {
  enum QueuePacketTypes {
    MPI = 0x10000000,
    REFL = 0x20000000,
  };

  class MpiQueueObject : public danet::ReflectableObject {
    struct QueueData {
      ObjectID oid;
      ObjectExtUID extUid;
      BitStream bs;
    };

    std::unordered_map<ecs::EntityId, std::vector<QueueData>> dispatched_objects;

    Message *dispatchMpiMessage(MessageID mid) override;

    void applyMpiMessage(const Message *m) override;

    bool deserialize(BitStream &bs, int data_size, ParserState *state) override;

    friend ParserState;

    MpiQueueObject(ParserState *state) : danet::ReflectableObject(state) {}
  public:
    DECL_REFLECTION(MpiQueueObject, danet::ReflectableObject)

    ~MpiQueueObject() override = default;

    void set_oid_ext_uid(ObjectID oid, ObjectExtUID extUid) {
      this->mpiObjectUID = oid;
      this->mpiObjectExtUID = extUid;
    }

    // put here purely to make friending easier
    static IObject *UnitRef_Dispatch(ObjectID oid, ObjectExtUID extUid, ParserState *state, bool do_queue);
  };


  IObject *ObjectDispatcher(ObjectID oid, ObjectExtUID extUid, ParserState *state);

} // namespace mpi


#endif // MYEXTENSION_OBJECTDISPATCHER_H
