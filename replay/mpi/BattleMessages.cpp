
#include "mpi/GeneralObject.h"
#include "state/ParserState.h"

#define MESSAGE_SWITCH_HEADER \
uint32_t fields = this->readFieldsSizeAndFlag(); \
if(fields == 0) /* no data was serialized in a message that expects data*/ \
  return false; \
/* sizes are stored based on # of fields, they are not stored based on a field index, so we need a separate var counting iterations*/ \
for(uint8_t curr_field_index = 0; fields != 0; curr_field_index++) { \
  uint8_t curr_field = 0; \
  while (((fields >> curr_field) & 1) == 0) curr_field++; /* this just iterates over all the fields to find the next one*/ \
  fields &= ~(1 << (curr_field & 0x1f)); /*this is done to remove previous (and maybe current????) field index*/ \
  /* dont ask me how it works, its probably compiler black magic that created this, or Gaijin did*/                                   \
  BitSize_t start_index = this->payload.GetReadOffset();                            \
  switch(curr_field) {


#define MESSAGE_SWITCH_FOOTER \
        default: { \
          EXCEPTION("Unknown id found in message serializer switch"); \
          break; \
          } \
        } \
      }                       \
      this->checkFieldSize(curr_field_index, this->payload.GetReadOffset()-start_index);
#define RET_FAIL(op) if(!op) return false

// if only offended uid exists, then they killed themselves
namespace mpi {
  bool KillMessage::readPayload(ParserState *state) {
    auto bs = &this->payload;
    uint16_t killer_uid, victim_uid;
    MESSAGE_SWITCH_HEADER
        {
          case 1: {
            RET_FAIL(bs->Read(this->offender_pid));
            break;
          }
          case 2: {
            RET_FAIL(bs->Read(this->offender_vehicle));
            break;
          }
          case 3: {
            RET_FAIL(bs->Read(killer_uid));
            this->offender_entity = state->g_entity_mgr.getUnit(killer_uid);
            break;
          }
          case 4: {
            RET_FAIL(bs->Read(victim_uid));
            this->offended_entity = state->g_entity_mgr.getUnit(victim_uid);
            break;
          }
          case 5: {
            RET_FAIL(bs->Read(this->VictimPid));
            break;
          }
          case 6: {
            RET_FAIL(bs->Read(this->some_enum));
            //LOG("Some enum val: {}", (uint8_t)this->some_enum);
            break;
          }
          case 7: {
            RET_FAIL(bs->Read(this->some_weap_flags));
            //LOG("Some flags: {}", (uint8_t)this->some_weap_flags);
            break;
          }
          case 8: {
            RET_FAIL(bs->Read(this->maybe_is_burav_kill));
            break;
          }
          case 9: {
            RET_FAIL(bs->Read(this->unitType));
            //LOG("unknwon_7: {}", (uint8_t)this->unitType);
            break;
          }
          case 0xa: {
            RET_FAIL(bs->Read(this->used_weapon));
            break;
          }
          case 0xb: {
            RET_FAIL(bs->Read(this->DeathType));
            break;
          }
          case 0xc: {
            RET_FAIL(bs->Read(this->destroyed_weapon));
            break;
          }
          case 0xd: {

            RET_FAIL(bs->Read(this->weird_str_1));
            break;
          }
          case 0xe: {
            RET_FAIL(bs->Read(this->weird_str_2));
            break;
          }
          case 0xf: {
            RET_FAIL(bs->Read(this->weird_val_3));
            break;
          }
          case 0x10: {
            RET_FAIL(bs->Read(this->weird_str_4));
            break;
          }
      MESSAGE_SWITCH_FOOTER
    }
    LOG("KillerPid: {}; KillerUid: {}; KillerVehicle: {}; KillerWeapon: {}", this->offender_pid, killer_uid, this->offender_vehicle, this->used_weapon);
    LOG("VictimPid: {}; VictimUid: {}; DeathType: {}", this->VictimPid, victim_uid, this->DeathType);
    LOG("some_enum: {}; some_weap_flags: {}; unitType: {}", (uint8_t)this->some_enum, this->some_weap_flags, this->unitType);
    LOG("destroyed_weapon: {}", this->destroyed_weapon);
    LOG("newStr1: {}; newStr2: {}; newVal3: {}; newStr4: {}", this->weird_str_1, this->weird_str_2, this->weird_val_3, this->weird_str_4);
    ecs::string *killer_className, *killer_missionName, *victim_className, *victim_missionName;
    if(this->offender_entity) {
      killer_className = state->g_entity_mgr.getNullable<ecs::string>(this->offender_entity, ECS_HASH("unit__className"));
      killer_missionName = state->g_entity_mgr.getNullable<ecs::string>(this->offender_entity, ECS_HASH("unit__missionName"));
      LOG("killer_className: {}; killer_missionName: {}", *killer_className, *killer_missionName);
    }
    if(this->offended_entity) {
      victim_className = state->g_entity_mgr.getNullable<ecs::string>(this->offended_entity, ECS_HASH("unit__className"));
      victim_missionName = state->g_entity_mgr.getNullable<ecs::string>(this->offended_entity, ECS_HASH("unit__missionName"));
      LOG("victim_className: {}; victim_missionName: {}", *victim_className, *victim_missionName);
    }

    return true;
  }
}