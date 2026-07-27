#include "math/dag_TMatrix.h"
#include "state/ParserState.h"

#define MESSAGE_SWITCH_HEADER                                                                                      \
  uint32_t fields = this->readFieldsSizeAndFlag();                                                                 \
  if (fields == 0) /* no data was serialized in a message that expects data*/                                      \
    return false;                                                                                                  \
  /* sizes are stored based on # of fields, they are not stored based on a field index, so we need a separate var  \
   * counting iterations*/                                                                                         \
  for (uint8_t curr_field_index = 0; fields != 0; curr_field_index++) {                                            \
    uint8_t curr_field = 0;                                                                                        \
    while (((fields >> curr_field) & 1) == 0)                                                                      \
      curr_field++; /* this just iterates over all the fields to find the next one*/                               \
    fields &= ~(1 << (curr_field & 0x1f)); /*this is done to remove previous (and maybe current????) field index*/ \
    /* dont ask me how it works, its probably compiler black magic that created this, or Gaijin did*/              \
    BitSize_t start_index = this->payload.GetReadOffset();                                                         \
    switch (curr_field) {                                                                                          \
      {


#define MESSAGE_SWITCH_FOOTER                                   \
  default: {                                                    \
    EXCEPTION("Unknown id found in message serializer switch"); \
    break;                                                      \
  }                                                             \
    }                                                           \
    }                                                           \
    this->checkFieldSize(curr_field_index, this->payload.GetReadOffset() - start_index);
#define RET_FAIL(op) \
  if (!op)           \
  return false


// if only offended uid exists, then they killed themselves
namespace mpi {
  bool KillMessage::readPayload(ParserState *state) {
    IBattleMessage::readPayload(state);
    uint16_t killer_uid, victim_uid;
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      switch (field_id) {
        case 1: {
          RET_FAIL(bs->Read(this->offender_pid));
          break;
        }
        case 2: {
          RET_FAIL(bs->Read(this->offender_vehicle));
          break;
        }
        case 3: {
          RET_FAIL(bs->Read(victim_uid));
          this->offended_unit = state->getUnitObj(victim_uid);
          break;
        }
        case 4: {
          RET_FAIL(bs->Read(killer_uid));
          this->offender_unit = state->getUnitObj(killer_uid);
          break;
        }
        case 5: {
          RET_FAIL(bs->Read(this->VictimPid));
          break;
        }
        case 6: {
          RET_FAIL(bs->Read(this->some_enum));
          // LOG("Some enum val: {}", (uint8_t)this->some_enum);
          break;
        }
        case 7: {
          RET_FAIL(bs->Read(this->some_weap_flags));
          // LOG("Some flags: {}", (uint8_t)this->some_weap_flags);
          break;
        }
        case 8: {
          RET_FAIL(bs->Read(this->maybe_is_burav_kill));
          break;
        }
        case 9: {
          RET_FAIL(bs->Read(this->unitType));
          // LOG("unknwon_7: {}", (uint8_t)this->unitType);
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
          MPI_SERIALIZER_DEFAULT
      }
      return true;
    });
    if (!valid)
      return false;
    if (SINK_LOG_ALLOWED(handle_object_dispatcher, LOGLEVEL::DEBUG_L2)) {
      DISPATCHER_LOGD2("KillerPid: {}; KillerUid: {}; KillerVehicle: {}; KillerWeapon: {}", this->offender_pid,
                       killer_uid, this->offender_vehicle, this->used_weapon);
      DISPATCHER_LOGD2("VictimPid: {}; VictimUid: {}; DeathType: {}", this->VictimPid, victim_uid, this->DeathType);
      DISPATCHER_LOGD2("some_enum: {}; some_weap_flags: {}; unitType: {}", (uint8_t) this->some_enum,
                       this->some_weap_flags, this->unitType);
      DISPATCHER_LOGD2("destroyed_weapon: {}", this->destroyed_weapon);
      DISPATCHER_LOGD2("newStr1: {}; newStr2: {}; newVal3: {}; newStr4: {}", this->weird_str_1, this->weird_str_2,
                       this->weird_val_3, this->weird_str_4);
      // ecs::string *killer_className, *killer_missionName, *victim_className, *victim_missionName;

      /*if(this->offender_entity) {
        killer_className = state->g_entity_mgr.getNullable<ecs::string>(this->offender_entity,
      ECS_HASH("unit__className")); killer_missionName =
      state->g_entity_mgr.getNullable<ecs::string>(this->offender_entity, ECS_HASH("unit__missionName"));
        DISPATCHER_LOGD2("killer_className: {}; killer_missionName: {}", *killer_className, *killer_missionName);
      }
      if(this->offended_entity) {
        victim_className = state->g_entity_mgr.getNullable<ecs::string>(this->offended_entity,
      ECS_HASH("unit__className")); victim_missionName =
      state->g_entity_mgr.getNullable<ecs::string>(this->offended_entity, ECS_HASH("unit__missionName"));
        DISPATCHER_LOGD2("victim_className: {}; victim_missionName: {}", *victim_className, *victim_missionName);
      }*/
    }

    if (offended_unit && !offended_unit->positions.history().empty()) {
      offended_unit->killed_position = offended_unit->positions.history().back().data.location;
      offended_unit_position = offended_unit->killed_position;
    }
    if (offender_unit && !offender_unit->positions.history().empty()) {
      offender_unit_position = offender_unit->positions.history().back().data.location;
    }
    return true;
  }

  bool CriticalDamageMessage::readPayload(ParserState *state) {
    IBattleMessage::readPayload(state);
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      switch (field_id) {
        case 1: {
          uint16_t uid;
          RET_FAIL(bs->Read(uid));
          this->offended_unit = state->getUnitObj(uid);
          break;
        }
        case 2: {
          RET_FAIL(bs->Read(this->player_pid));
          break;
        }
        case 3: {
          RET_FAIL(bs->Read(this->vehicle));
          break;
        }
        case 4: {
          uint16_t uid;
          RET_FAIL(bs->Read(uid));
          this->offender_unit = state->getUnitObj(uid);
          break;
        }
        case 5: {
          RET_FAIL(bs->Read(this->is_fire));
          break;
        }
        case 6: {
          RET_FAIL(bs->Read(this->unitType));
          break;
        }
          MPI_SERIALIZER_DEFAULT
      }
      return true;
    });
    return valid;
  }

  bool SevereDamageMessage::readPayload(ParserState *state) {
    IBattleMessage::readPayload(state);
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      switch (field_id) {
        case 1: {
          uint16_t uid;
          RET_FAIL(bs->Read(uid));
          this->offended_unit = state->getUnitObj(uid);
          break;
        }
        case 2: {
          RET_FAIL(bs->Read(this->player_pid));
          break;
        }
        case 3: {
          RET_FAIL(bs->Read(this->vehicle));
          break;
        }
        case 4: {
          uint16_t uid;
          RET_FAIL(bs->Read(uid));
          this->offender_unit = state->getUnitObj(uid);
          break;
        }
        case 5: {
          RET_FAIL(bs->Read(this->unitType));
          break;
        }
          MPI_SERIALIZER_DEFAULT
      }
      return true;
    });
    return valid;
  }
  bool AwardMessage::readPayload(ParserState *state) {
    IBattleMessage::readPayload(state);
    bool valid = parse([&](const BitStream *bs, uint32_t field_id) {
      switch (field_id) {
        case 1: {
          bs->Read(this->player_pid);
          break;
        }
        case 2: {
          bs->Read(this->award);
          break;
        }
        case 3: {
          bs->Read(this->stage);
          break;
        }
        case 4: {
          bs->Read(this->wp);
          break;
        }
        case 5: {
          bs->Read(this->exp);
          break;
        }
          MPI_SERIALIZER_DEFAULT
      }
      return true;
    });
    return valid;
  }


  bool IBattleMessage::readPayload(ParserState *state) {
    this->time_ms = state->curr_time_ms;
    return true;
  }
}
