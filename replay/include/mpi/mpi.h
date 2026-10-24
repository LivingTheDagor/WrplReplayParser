//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "ecs/EntityManager.h"
#include "daNet/bitStream.h"
#include <idFieldSerializer.h>
#include "utils.h"
#include "dag_assert.h"
#include "unordered_map"
#include "unordered_set"
#include "type_name.h"


struct ParserState;

namespace mpi // message passing interface
{
  // extern std::unordered_map<int, std::unordered_map<int, int>> mpi_data;
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
    virtual bool receiveMpiMessage(const Message *msg, SystemID receiver) = 0; // process message
  };
  class IObject // base class for objects that handle messages
  {
  protected:
    ParserState *state;
    ObjectID mpiObjectUID;

    void setUID(ObjectID uid) { mpiObjectUID = uid; }
    friend ParserState;

  public:
    ObjectExtUID mpiObjectExtUID = INVALID_OBJECT_EXT_UID;

    IObject(const IObject &) = default;

    IObject &operator=(const IObject &) = default;

    virtual ~IObject() = default;

    explicit IObject(ParserState *state, ObjectID uid = INVALID_OBJECT_ID) : state(state), mpiObjectUID(uid) {
      G_ASSERT(this->state);
    }

    [[nodiscard]] ObjectID getUID() const { return mpiObjectUID; }

    virtual Message *dispatchMpiMessage(MessageID mid) = 0; // construct message instance by message id
    virtual bool applyMpiMessage(const Message *m) = 0; // execute message, false if it could not be decoded
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

  inline void write_object_ext_uid(BitStream &bs, const IObject *obj) {
    return write_object_ext_uid(bs, obj->getUID(), obj->mpiObjectExtUID);
  }

// Not used. It does not compile ("uin8_t"), and its loop does not end.
#define SWITCH_MESSAGE_INDEXES(fields)      \
  while (fields != 0) {                     \
    uin8_t curr_index = 0;                  \
    while ((fields >> curr_index) & 1 == 0) \
      curr_index = 0                        \
  }

#define MPI_SERIALIZER_DEFAULT                                  \
  default: {                                                    \
    EXCEPTION("Unknown id found in message serializer switch"); \
    break;                                                      \
  }

  class Message // base class for all messages
  {
  public:
    mpi::IObject *obj; // recipient of this message, can't be NULL
    uint32_t extUID = ~0u;
    MessageID id; // identification of this message
    mutable bool delete_message = true; // do we delete message after dispatch + sendto? only to be used if the message
                                        // now has another object managing its lifetime
    BitStream payload; // serialized parameters for this message

    void setFieldSize(BitSize_t sz) { idFieldSerializer.setFieldSize(sz); }

  protected:
    void writeFieldsSize() { idFieldSerializer.writeFieldsSize(payload); }

    void skipReadingField(uint8_t index) const { idFieldSerializer.skipReadingField(index, payload); }

    uint32_t readFieldsSizeAndFlag() { return idFieldSerializer.readFieldsSizeAndFlag(payload); }

    void checkFieldSize(uint8_t index, BitSize_t size) { idFieldSerializer.checkFieldSize(index, size); }

    inline bool parse(const eastl::function<bool(const BitStream *, uint32_t)> &cb, bool checked = false);
    uint32_t fieldSize(uint8_t index) const { return idFieldSerializer.getFieldSize(index); }

  private:
    IdFieldSerializer32 idFieldSerializer;

  public:
    Message(IObject *o, MessageID mid) : obj(o), id(mid), payload(), idFieldSerializer() {}

    virtual ~Message() = default;

    Message(const Message &rhs) :
      obj(rhs.obj), id(rhs.id), payload(rhs.payload), idFieldSerializer(rhs.idFieldSerializer) {}

    Message &operator=(const Message &rhs) {
      if (this == &rhs)
        return *this;
      obj = rhs.obj;
      id = rhs.id;
      payload.~BitStream();
      new (&payload) BitStream(rhs.payload.GetData(), rhs.payload.GetNumberOfBytesUsed(), true);
      idFieldSerializer = rhs.idFieldSerializer;
      return *this;
    }

    void destroy() { this->~Message(); }

    void serialize(BitStream &bs) const // full serialize of this message
    {
      write_object_ext_uid(bs, obj->getUID(), obj->mpiObjectExtUID);
      bs.Write(id);
      bs.Write((const char *) payload.GetData(), payload.GetNumberOfBytesUsed());
    }

    bool apply() const { return obj->applyMpiMessage(this); } // actually execute this message

    virtual const char *getDebugMpiName() const { return ""; }

    virtual bool isApplicable(const IMessageListener *) const {
      return true;
    }; // is this message relevant for this listener?
    virtual bool isNeedReception() const { return true; }; // do we need receive this messags?
    virtual bool isNeedTransmission() const { return true; } // do we need send this message to remote system(s)?


    virtual int getChannelId() const { return 0; }

    virtual bool isNeedProcessing() const { return true; } // do we need apply this message?

    // deserialize from/serialize parameters to 'payload' bitStream (if exist any)
    virtual void writePayload() {}

    virtual bool readPayload(ParserState *state) { return true; }
  };

  Message *dispatch(const BitStream &bs, ParserState *state,
                    bool copy_payload = false); // assemble message by BitStream. Note : message should be
  // destroyed after use (i.e. msg->destroy())
  bool sendto(Message *m, SystemID receiver); // send message, also apply if need to; false if a handler failed
  inline bool send(Message *m) { return sendto(m, INVALID_SYSTEM_ID); }
  void register_listener(IMessageListener *l); // register listener for message handling
  void unregister_listener(IMessageListener *l); // invert operation

  void register_object_dispatcher(object_dispatcher od); // register function for dispatch objects
  IObject *dispatch_object(mpi::ObjectID oid, ObjectExtUID ext_uid, ParserState *state);


  // Calls cb once for each field in the IdFieldSerializer32 mask, with the field id. checked true: a failed field
  // or a field that reads a different size than its table entry makes parse() return false. checked false: a
  // failed field is logged, and the parse continues.
  inline bool Message::parse(const eastl::function<bool(const BitStream *, uint32_t)> &cb, bool checked) {
    ZoneScoped;
    uint32_t fields = 0;
    if (checked) {
      if (!idFieldSerializer.readFieldsSizeAndFlagChecked(payload, fields))
        return false;
    } else {
      fields = this->readFieldsSizeAndFlag();
    }
    if (fields == 0) /* no data was serialized in a message that expects data*/
      return false;
    /* sizes are stored based on # of fields, they are not stored based on a field index, so we need a separate var
     * counting iterations*/
    for (uint8_t curr_field_index = 0; fields != 0; curr_field_index++) {
      uint8_t curr_field = 0;
      while (((fields >> curr_field) & 1) == 0)
        curr_field++; /* this just iterates over all the fields to find the next one*/
      fields &= ~(1u << (curr_field & 0x1f)); /* remove the current field */
      /* dont ask me how it works, its probably compiler black magic that created this, or Gaijin did*/
      BitSize_t start_index = this->payload.GetReadOffset();
      bool ret = cb(&this->payload, curr_field);
      if (!ret) {
        LOGE("{} failed to parse field {} (index {}) of mid {:#x}", util::type_name_of_obj(this), curr_field,
             curr_field_index, this->id);
        if (checked)
          return false;
      }
      if (checked && this->payload.GetReadOffset() - start_index != fieldSize(curr_field_index))
        return false;
      this->checkFieldSize(curr_field_index, this->payload.GetReadOffset() - start_index);
    }
    return true;
  }
}; // namespace mpi
