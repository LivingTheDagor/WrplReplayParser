//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "ecs/EntityManager.h"
#include <idFieldSerializer.h>
#include "BitStream.h"
#include "utils.h"
#include "dag_assert.h"
#include "unordered_map"
#include "unordered_set"

struct ParserState;

namespace mpi // message passing interface
{
  //extern std::unordered_map<int, std::unordered_map<int, int>> mpi_data;
  class IObject;

  class Message;

  typedef uint16_t ObjectID;
  typedef uint32_t ObjectExtUID;
  typedef uint16_t MessageID;
  typedef uint8_t SystemID;

  typedef IObject *(*object_dispatcher)(ObjectID, ObjectExtUID, ParserState *);
  extern object_dispatcher obj_dispatcher;

#define INVALID_OBJECT_ID  ObjectID(-1)
#define INVALID_MESSAGE_ID MessageID(-1)
#define INVALID_SYSTEM_ID  SystemID(-1)

  constexpr ObjectExtUID INVALID_OBJECT_EXT_UID = ObjectExtUID(~0u);

  struct IMessageListener // subsystem that handles messages (typically - local, multiplayer, replay...)
  {
    IMessageListener *next = nullptr;
    virtual void receiveMpiMessage(const Message *msg, SystemID receiver) = 0; // process message
  };
  class IObject // base class for objects that handle messages
  {
  protected:
    ObjectID mpiObjectUID;

  public:
    ObjectExtUID mpiObjectExtUID = INVALID_OBJECT_EXT_UID;

    IObject(const IObject &) = default;

    IObject &operator=(const IObject &) = default;

    virtual ~IObject() = default;

    explicit IObject(ObjectID uid = INVALID_OBJECT_ID) : mpiObjectUID(uid) {}

    [[nodiscard]] ObjectID getUID() const { return mpiObjectUID; }

    virtual Message *dispatchMpiMessage(MessageID mid) = 0; // construct message instance by message id
    virtual void applyMpiMessage(const Message *m) = 0;     // execute message
  };

#define MPI_HEADER_SIZE (sizeof(mpi::ObjectID) + sizeof(mpi::MessageID) + sizeof(mpi::SystemID))

  inline void write_object_ext_uid(BitStream &bs, ObjectID oid, ObjectExtUID ext) {
    // Because MPI lib is used only in WT-based games, first 11 bits are reserved for
    // index. In case mpiObjectExtUID is set, we can ignore the index, so we set
    // all 11 bits to 1
    constexpr auto EXT_MASK = ObjectID(0x7FFu);
    if (oid == mpi::INVALID_OBJECT_ID) {
      bs.Write(oid);
      return;
    }
    if (ext == INVALID_OBJECT_EXT_UID) {
      bs.Write(oid);
    } else {
      oid |= EXT_MASK;
      bs.Write(oid);
      bs.WriteCompressed(ext);
    }
  }

  inline bool read_object_ext_uid(const BitStream &bs, ObjectID &oid, ObjectExtUID &ext) {
    // see write_object_ext_uid
    constexpr auto EXT_MASK = ObjectID(0x7FFu);
    ext = INVALID_OBJECT_EXT_UID;
    if (!bs.Read(oid)) {
      oid = INVALID_OBJECT_ID;
      return false;
    }
    if ((oid & EXT_MASK) == EXT_MASK && oid != INVALID_OBJECT_ID) {
      return bs.ReadCompressed(ext);
    }
    return true;
  }

  class Message // base class for all messages
  {
  public:
    mpi::IObject *obj; // recipient of this message, can't be NULL
    uint32_t extUID = ~0u;
    MessageID id;             // identification of this message
    BitStream payload; // serialized parameters for this message

    void setFieldSize(BitSize_t sz) { idFieldSerializer.setFieldSize(sz); }

#if DAGOR_DBGLEVEL > 0
    void checkFieldSize(int index, BitSize_t sz) const { return idFieldSerializer.checkFieldSize(index, sz); }
#endif
  protected:
    void writeFieldsSize() { idFieldSerializer.writeFieldsSize(payload); }

    void skipReadingField(uint8_t index) const { idFieldSerializer.skipReadingField(index, payload); }

    uint32_t readFieldsSizeAndFlag() { return idFieldSerializer.readFieldsSizeAndFlag(payload); }

  private:
    IdFieldSerializer32 idFieldSerializer;

  public:
    Message(IObject *o, MessageID mid) :
        obj(o), id(mid), payload(), idFieldSerializer() {}

    virtual ~Message() = default;

    Message(const Message &rhs) :
        obj(rhs.obj),
        id(rhs.id),
        payload(rhs.payload),
        idFieldSerializer(rhs.idFieldSerializer) {}

    Message &operator=(const Message &rhs) {
      if (this == &rhs)
        return *this;
      obj = rhs.obj;
      id = rhs.id;
      payload.~BitStream();
      new(&payload) BitStream(rhs.payload.GetData(), rhs.payload.GetNumberOfBytesUsed(), true);
      idFieldSerializer = rhs.idFieldSerializer;
      return *this;
    }

    void destroy() {
      this->~Message();
    }

    void serialize(BitStream &bs) const // full serialize of this message
    {
      write_object_ext_uid(bs, obj->getUID(), obj->mpiObjectExtUID);
      bs.Write(id);
      bs.Write((const char *) payload.GetData(), payload.GetNumberOfBytesUsed());
    }

    void apply() const { obj->applyMpiMessage(this); } // actually execute this message

    virtual const char *getDebugMpiName() const { return ""; }

    virtual bool isApplicable(const IMessageListener *) const { return true; }; // is this message relevant for this listener?
    virtual bool isNeedReception() const { return true; };                      // do we need receive this messags?
    virtual bool
    isNeedTransmission() const { return true; }                    // do we need send this message to remote system(s)?


    virtual int getChannelId() const { return 0; }

    virtual bool isNeedProcessing() const { return true; } // do we need apply this message?

    // deserialize from/serialize parameters to 'payload' bitStream (if exist any)
    virtual void writePayload() {}

    virtual bool readPayload() { return true; }
  };

  Message *
  dispatch(const BitStream &bs, ParserState *state, bool copy_payload = false); // assemble message by BitStream. Note : message should be
  // destroyed after use (i.e. msg->destroy())
  void sendto(Message *m, SystemID receiver);                               // send message, also apply if need to
  inline void send(Message *m) { sendto(m, INVALID_SYSTEM_ID); }
  void register_listener(IMessageListener *l);           // register listener for message handling
  void unregister_listener(IMessageListener *l);         // invert operation

  void register_object_dispatcher(object_dispatcher od); // register function for dispatch objects
  IObject *dispatch_object(mpi::ObjectID oid, ObjectExtUID ext_uid, ParserState *state);

}; // namespace mpi

