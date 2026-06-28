

#ifndef WTFILEUTILS_GENERALOBJECT_H
#define WTFILEUTILS_GENERALOBJECT_H
#include "mpi.h"

namespace mpi {

  struct GeneralObject : public IObject {
    ParserState *state;
    GeneralObject(ParserState *state) : IObject(0x5802) { this->state=state;}

    enum MainEnum: uint16_t {
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
      Tank1 = 0xf073,
      Tank2 = 0xf074,
      Rocket1 = 0xf11a,
      Rocket2 = 0xf0db,
    };
    Message *dispatchMpiMessage(MessageID mid) override;
    void applyMpiMessage(const Message *m) override;
    ~GeneralObject() override = default;
  };

  class BSMessage : public Message {
  public:
      BSMessage(IObject *o, MessageID mid) : Message(o, mid) {
      }

      BitStream data{};
      bool readPayload(ParserState *state) override { return this->payload.Read(data); };
      void writePayload() override { this->payload.Write(data); };
  };

  class TankMessage: public Message {
  public:
    BitStream data{};
    TankMessage(IObject *o, MessageID mid) : Message(o, mid) {}
    bool readPayload(ParserState *state) override;
    void writePayload() override;
  };

  class IBattleMessage: public Message {
  protected:
    bool readPayload(ParserState *state) override;

  public:
    uint32_t time_ms{};
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
    bool readPayload(ParserState *state) override;
  public:
    KillMessage(IObject *o): IBattleMessage(o, GeneralObject::Kill) {}
    std::string offender_vehicle; // case 2
    std::string used_weapon; // case 0xa
    std::string destroyed_weapon; // case 0xc
    unit::Unit* offended_unit; // case 3
    Point3 offended_unit_position{};
    unit::Unit* offender_unit; // case 4
    Point3 offender_unit_position{};
    int DeathType; //case 0xb
    int offender_pid; // case 1
    int VictimPid; // only filled when a weapon is destroyed
    WeaponType some_enum;
    uint8_t unitType;
    bool maybe_is_burav_kill; // bit
    uint8_t some_weap_flags; // according to blk
    std::string weird_str_1{};
    std::string weird_str_2{};
    uint32_t weird_val_3{};
    std::string weird_str_4{};

  };

  class CriticalDamageMessage : public IBattleMessage {
    bool readPayload(ParserState *state) override;
  public:
    CriticalDamageMessage(IObject *o) : IBattleMessage(o, GeneralObject::CriticalDamage) {}

    unit::Unit* offended_unit;
    int player_pid;
    std::string vehicle;
    unit::Unit* offender_unit;
    uint8_t is_fire{}; // when doesn't equal 0, still an u8 for whatever reason
    uint8_t unitType; // enum value

  };

  class SevereDamageMessage : public IBattleMessage {
    bool readPayload(ParserState *state) override;
  public:
    SevereDamageMessage(IObject *o) : IBattleMessage(o, GeneralObject::SevereDamage) {}
    unit::Unit* offended_unit;
    int player_pid;
    std::string vehicle;
    unit::Unit* offender_unit;
    uint8_t unitType; // enum value
  };

  class AwardMessage : public IBattleMessage {
    bool readPayload(ParserState *state) override;
  public:
    AwardMessage(IObject *o) : IBattleMessage(o, GeneralObject::Awards) {}

    int player_pid;
    std::string award{};
    uint32_t stage;
    uint32_t wp;
    uint32_t exp;
  };
}


#endif //WTFILEUTILS_GENERALOBJECT_H
