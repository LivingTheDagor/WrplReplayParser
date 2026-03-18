

#ifndef WTFILEUTILS_GENERALOBJECT_H
#define WTFILEUTILS_GENERALOBJECT_H
#include "mpi.h"

namespace mpi {

  struct GeneralObject : public IObject {
    ParserState *state;
    GeneralObject(ParserState *state) : IObject(0x5802) { this->state=state;}
    enum MainEnum {
      SevereDamage = 0xf157,
      CriticalDamage = 0xf056,
      Kill = 0xf058,
      Awards = 0xf078,
      Action = 0xf028,
      Replication = 0xd039,
      ReflectionNoDecompress = 0xf0aa,
      Reflection1 = 0xf02d,
      Reflection2 = 0xd136,
      ACTUALLY_NOT_REFLECTION = 0xd137,

    };
    Message *dispatchMpiMessage(MessageID mid) override;
    void applyMpiMessage(const Message *m) override;
    ~GeneralObject() override = default;
  };

  class IBattleMessage: public Message {
  public:
    IBattleMessage(IObject *o, MessageID mid) : Message(o, mid) {}
    //virtual std::string read_basic_message() = 0;
    //virtual std::string read_extended_message() = 0;
  };
  enum WeaponType: uint8_t {
    IsBullet = 1, // I presume, this is the only  case not covered
    IsBomb = 2,
    IsRocket = 3,
    iSTorpedo = 4,
  };

  enum SomeWeaponFlags: uint8_t {
    isArtillery = 1<<0, // if 1, then this is artillery instead of bullet / shell
    isDepthBomb = 1<<1, // if this is true, then it's a mine
    isMine = 1<<2, // if this is true, then it's a mine
    someFlag = 1<<5,
  };
  enum DeathType:int32_t {
    Normal = 0,
    IsCrash = -1,
    isLeave = -2,
  };

  class KillMessage : public IBattleMessage {
  public:
    std::string offender_vehicle; // case 2
    std::string used_weapon; // case 0xa
    std::string destroyed_weapon; // case 0xc
    ecs::EntityId offended_entity; // case 3
    ecs::EntityId offender_entity; // case 4
    // DON'T USE THESE, LIKE AT ALL, THE PID VAL IS FUCKED
    int DeathType; // ok so like I think its supposed to be the offended pid, but it really isnt, case 0xb
    int offender_pid; // case 1
    int VictimPid; // some player index
    WeaponType some_enum;
    uint8_t unitType;
    bool maybe_is_burav_kill; // bit
    uint8_t some_weap_flags; // according to blk
    std::string weird_str_1{};
    std::string weird_str_2{};
    uint32_t weird_val_3{};
    std::string weird_str_4{};

    KillMessage(IObject *o): IBattleMessage(o, GeneralObject::Kill) {}
    bool readPayload(ParserState *state) override;
  };
}

struct BattleMessageHandler {
  std::vector<std::string> simple_messages;
  std::vector<std::string> long_messages;
  ParserState *state;
  explicit BattleMessageHandler(ParserState *s): state(s) {}
  void parse_kill_message(mpi::KillMessage *msg);
};


#endif //WTFILEUTILS_GENERALOBJECT_H
