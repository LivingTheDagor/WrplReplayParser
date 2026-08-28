#include "math/dag_TMatrix.h"
#include "mpi/codegen/ReflIncludes.h"
#include "state/ParserState.h"

CREATE_HANDLE(handle_replication, "Replication")

constexpr uint32_t MAX_MISSION_AREAS = 4096;

void force_link_replication() { std::cout << ""; }

template <typename T>
void grow(std::pmr::vector<T*> &vec, ParserState * state, uint32_t new_size) {
  if (new_size > vec.size()) {
    vec.resize(new_size);
    for (uint32_t i = 0; i < new_size; i++) {
      if (vec[i] == nullptr) {
        vec[i] = state->_new<T>();
      }
    }
  }
}

MissionZone *create_zone(BitStream &bs, uint8_t zone_type, ParserState *state) {
  IdFieldSerializer255 serializer255{};
  BitSize_t end;
  uint32_t count = serializer255.readFieldsSizeAndCount(bs, end);
  G_ASSERT(serializer255.readFieldsIndex(bs));
  auto local_44 = bs.GetReadOffset();
  // LOGE("Creating Zone");
  uint8_t zone_id = 0;
  uint8_t maybe_team_id = 0;
  uint32_t mission_area_id = 0;
  uint16_t some_val_2 = 0;
  for (uint16_t i = 0; i < count; i++) {
    BitSize_t start_size = bs.GetReadOffset();
    switch (serializer255.getFieldId(i)) {
      case 1: {
        bs.Read(zone_id);
        // LOGE("Mission area_id: {}", mission_area_id);
        break;
      }
      case 2: {
        bs.Read(maybe_team_id);
        // LOGE("maybe_team_id: {}", maybe_team_id);
        break;
      }
      case 3: {
        bs.ReadCompressed(mission_area_id);
        // LOGE("some_val_1: {}", some_val);
        break;
      }
      case 4: {
        bs.Read(some_val_2);
        // LOGE("some_val_2: {}", some_val_2);
        break;
      }
      default: EXCEPTION("");
    }
    G_ASSERT(bs.GetReadOffset() - start_size == serializer255.getFieldSize(i));

    // LOGE("{}; data: {}", serializer255.getFieldId(i), FormatHexToStream(data).str());
  }
  // LOGE("mission_area_id: {}; maybe_team_id: {}, some_val: {}; some_val_2: {}", mission_area_id, maybe_team_id,
  // some_val, some_val_2);
  if (zone_id == state->Zones.size()) {
    MissionZone *obj;
    mpi::ObjectID oid = (0x5<<0xb) + (mpi::ObjectID)state->Zones.size();
    switch (zone_type) {
      case 0: {
        obj = state->_new<BombingZone>(state, oid);
        break;
      }
      case 1: {
        obj = state->_new<CaptureZone>(state, oid);
        break;
      }
      case 2: {
        obj = state->_new<RearmZone>(state, oid);
        break;
      }
      case 3: {
        obj = state->_new<ExitZone>(state, oid);
        break;
      }
      case 4: {
        obj = state->_new<PickupZone>(state, oid);
        break;
      }
      default: EXCEPTION("Invalid Zone id: {}", zone_type);
    }
    if (mission_area_id >= MAX_MISSION_AREAS)
      EXCEPTION("Invalid MissionArea id: {}", mission_area_id);
    grow(state->missionAreas1, state, mission_area_id + 1);
    auto area = state->missionAreas1[mission_area_id];
    if (area->curr() == nullptr) {
      LOGE("WARNING: Create dummy MissionArea id: {} for MissionZone id:{} type:{}", mission_area_id, zone_id,
           zone_type);
      auto & ref = state->missionAreas1[mission_area_id];
      *ref->reserveOne() = state->_new<MissionArea>(state, (0x16<<0xb) + (mpi::ObjectID)mission_area_id);
      ref->checkAndPush(state);
      area = state->missionAreas1[mission_area_id];
    }
    obj->area = *area->curr();
    grow(state->Zones, state, zone_id + 1);
    *state->Zones[zone_id]->reserveOne() = obj;
    state->Zones[zone_id]->checkAndPush(state);
  }
  bs.SetReadOffset(end);
  if (zone_id >= state->Zones.size())
    EXCEPTION("Invalid MissionZone id: {}", zone_id);
  return *state->Zones[zone_id]->curr();
}

IMPLEMENT_REPLICATION(Airfield) // 0

IMPLEMENT_REPLICATION(BombingZone) // 1

IMPLEMENT_REPLICATION(CaptureZone) // 2

IMPLEMENT_REPLICATION(DMSquad) // 3

IMPLEMENT_REPLICATION(ExitZone) // 4

IMPLEMENT_REPLICATION(FlightModelWrap) // 5

IMPLEMENT_REPLICATION(IGroundModel) // 6

IMPLEMENT_REPLICATION(InfantryTroop) // 7

IMPLEMENT_REPLICATION(InteractiveObject) // 8

IMPLEMENT_REPLICATION(InteractiveObjectProxy) // 9

IMPLEMENT_REPLICATION(MissionArea) // 10

IMPLEMENT_REPLICATION(MissionDrawing) // 11

IMPLEMENT_REPLICATION(MissionObjective) // 12

IMPLEMENT_REPLICATION(MissionZone) // 13

IMPLEMENT_REPLICATION(ObjectsGroup) // 14

IMPLEMENT_REPLICATION(OrderPlayerProgress) // 15

IMPLEMENT_REPLICATION(PickupZone) // 16

IMPLEMENT_REPLICATION(RaceMode) // 17

IMPLEMENT_REPLICATION(RearmZone) // 18

IMPLEMENT_REPLICATION(RespawnBase) // 19

IMPLEMENT_REPLICATION(Squadron) // 20

IMPLEMENT_REPLICATION(UnitWinch) // 21

IMPLEMENT_REPLICATION(Waypoint) // 22

IMPLEMENT_REPLICATION(Wing) // 23

danet::ReplicatedObject *Airfield::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated Airfield");
  return nullptr;
}

danet::ReplicatedObject *BombingZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  // LOGE("Parsing Replicated BombingZone");
  auto zone = create_zone(bs, 0, state);
  IdFieldSerializer255 serializer255{};
  BitSize_t end;
  auto count = serializer255.readFieldsSizeAndCount(bs, end);
  G_ASSERT(serializer255.readFieldsIndex(bs));
  for (uint16_t i = 0; i < count; i++) {
    std::vector<char> data{};
    data.resize(BITS_TO_BYTES(serializer255.getFieldSize(i)));
    bs.ReadBits((uint8_t *) data.data(), serializer255.getFieldSize(i));
    // LOGE("Index: {}; size: {}; data: {}", serializer255.getFieldId(i), serializer255.getFieldSize(i),
    // FormatHexToStream(data).str());
  }
  return zone;
}

danet::ReplicatedObject *CaptureZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  // LOGE("Parsing Replicated CaptureZone");
  auto zone = create_zone(bs, 1, state);
  IdFieldSerializer255 serializer255{};
  BitSize_t end;
  auto count = serializer255.readFieldsSizeAndCount(bs, end);
  G_ASSERT(serializer255.readFieldsIndex(bs));
  for (uint16_t i = 0; i < count; i++) {
    std::vector<char> data{};
    data.resize(BITS_TO_BYTES(serializer255.getFieldSize(i)));
    bs.ReadBits((uint8_t *) data.data(), serializer255.getFieldSize(i));
    // LOGE("Index: {}; size: {}; data: {}", serializer255.getFieldId(i), serializer255.getFieldSize(i),
    // FormatHexToStream(data).str());
  }
  return zone;
}

danet::ReplicatedObject *DMSquad::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated DMSquad");
  return nullptr;
}

danet::ReplicatedObject *ExitZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated ExitZone");
  return create_zone(bs, 3, state);
}

danet::ReplicatedObject *FlightModelWrap::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated FlightModelWrap");
  return nullptr;
}

danet::ReplicatedObject *IGroundModel::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated IGroundModel");
  return nullptr;
}

danet::ReplicatedObject *InfantryTroop::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated InfantryTroop");
  return nullptr;
}

danet::ReplicatedObject *InteractiveObject::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated InteractiveObject");
  return nullptr;
}

danet::ReplicatedObject *InteractiveObjectProxy::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated InteractiveObjectProxy");
  return nullptr;
}

inline bool isSet(uint16_t flag, danet::AreaFlagsEnum flags) {
  return (static_cast<uint16_t>(flag) & static_cast<uint16_t>(flags)) != 0;
}

danet::ReplicatedObject *MissionArea::createReplicatedObject(BitStream &bs, ParserState *state) {
  IdFieldSerializer255 serializer255{};
  BitSize_t end;
  auto count = serializer255.readFieldsSizeAndCount(bs, end);
  G_ASSERT(serializer255.readFieldsIndex(bs));
  std::string v1;
  uint32_t index = 0;
  uint8_t v3 = 0;
  mpi::ObjectID index_2 = 0;
  danet::AreaFlagsEnum areaFlags{};
  TMatrix tm = TMatrix::IDENT;
  uint32_t v7 = 0; // not always

  for (uint16_t i = 0; i < count; i++) {
    BitSize_t start_size = bs.GetReadOffset();
    switch (serializer255.getFieldId(i)) {
      case 1: {
        bs.Read(v1);
        break;
      }
      case 2: {
        bs.ReadCompressed(index);
        break;
      }
      case 3: {
        bs.Read(v3);
        break;
      }
      case 4: {
        bs.Read(areaFlags);
        break;
      }
      case 5: {
        bs.Read(tm);
        break;
      }
      case 6: {
        bs.Read(index_2);
        break;
      }
      case 7: {
        bs.Read(v7);
        break;
      }
      case 8: {
        std::vector<Point3> temp{};
        uint32_t size;
        bs.ReadCompressed(size);
        temp.resize(size);
        for (auto &v: temp) {
          bs.Read(v);
        }
        break;
      }
      default: EXCEPTION("");
    }
    G_ASSERT(bs.GetReadOffset() - start_size == serializer255.getFieldSize(i));

    // LOGE("{}; data: {}", serializer255.getFieldId(i), FormatHexToStream(data).str());
  }

  if (index >= MAX_MISSION_AREAS)
    EXCEPTION("Invalid MissionArea index: {}", index);
  auto x = state->_new<MissionArea>(state, index_2);
  *x->areaFlags.reserveOne() = areaFlags;
  x->areaFlags.checkAndPush(state);
  x->tm = tm;
  x->markVarWithFlag(&x->areaFlags, RVF_CALL_HANDLER_FORCE, true);
  REPLICATION_LOGD3("MissionArea({}) v1: {}; v2: {}; v3: {}; v5: {}; v6: {}; v7: {}",
                    ((double) state->curr_time_ms) / 1000.0, v1, index, v3, x->tm.toString(0), index_2, v7);
  auto idx = index_2 & 0x7FF;
  grow(state->missionAreas2, state, idx + 1);
  grow(state->missionAreas1, state, index + 1);
  *state->missionAreas1[index]->reserveOne() = x;
  state->missionAreas1[index]->checkAndPush(state);

  *state->missionAreas2[idx]->reserveOne() = x;
  state->missionAreas2[idx]->checkAndPush(state);
  REPLICATION_LOGD2("Parsing Replicated MissionArea");
  return nullptr;
}

danet::ReplicatedObject *MissionDrawing::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated MissionDrawing");
  IdFieldSerializer255 serializer255{};
  BitSize_t end;
  auto count = serializer255.readFieldsSizeAndCount(bs, end);
  G_ASSERT(serializer255.readFieldsIndex(bs));
  for (uint16_t i = 0; i < count; i++) {
    std::vector<char> data{};
    data.resize(BITS_TO_BYTES(serializer255.getFieldSize(i)));
    bs.ReadBits((uint8_t *) data.data(), serializer255.getFieldSize(i));
    // LOGE("Index: {}; size: {}; data: {}", serializer255.getFieldId(i), serializer255.getFieldSize(i),
    // FormatHexToStream(data).str());
  }
  return nullptr;
}

danet::ReplicatedObject *MissionObjective::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated MissionObjective");
  return nullptr;
}

danet::ReplicatedObject *MissionZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  EXCEPTION("MissionZone should not be created");
  return nullptr;
}

danet::ReplicatedObject *ObjectsGroup::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated ObjectsGroup");
  return nullptr;
}

danet::ReplicatedObject *OrderPlayerProgress::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated OrderPlayerProgress");
  return nullptr;
}

danet::ReplicatedObject *PickupZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated PickupZone");
  return create_zone(bs, 4, state);
}

danet::ReplicatedObject *RaceMode::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated RaceMode");
  return nullptr;
}

danet::ReplicatedObject *RearmZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated RearmZone");
  return create_zone(bs, 2, state);
}

danet::ReplicatedObject *RespawnBase::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated RespawnBase");
  return nullptr;
}

danet::ReplicatedObject *Squadron::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated Squadron");
  return nullptr;
}

danet::ReplicatedObject *UnitWinch::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated UnitWinch");
  return nullptr;
}

danet::ReplicatedObject *Waypoint::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated Waypoint");
  return nullptr;
}

danet::ReplicatedObject *Wing::createReplicatedObject(BitStream &bs, ParserState *state) {
  REPLICATION_LOGD2("Parsing Replicated Wing");
  return nullptr;
}


void Airfield::serializeReplicaCreationData(BitStream &bs) const {}
void MissionZone::serializeReplicaCreationData(BitStream &bs) const {}
void BombingZone::serializeReplicaCreationData(BitStream &bs) const {}
void CaptureZone::serializeReplicaCreationData(BitStream &bs) const {}
void DMSquad::serializeReplicaCreationData(BitStream &bs) const {}
void ExitZone::serializeReplicaCreationData(BitStream &bs) const {}
void FlightModelWrap::serializeReplicaCreationData(BitStream &bs) const {}
void IGroundModel::serializeReplicaCreationData(BitStream &bs) const {}
void InfantryTroop::serializeReplicaCreationData(BitStream &bs) const {}
void InteractiveObject::serializeReplicaCreationData(BitStream &bs) const {}
void InteractiveObjectProxy::serializeReplicaCreationData(BitStream &bs) const {}
void MissionArea::serializeReplicaCreationData(BitStream &bs) const {}
void MissionDrawing::serializeReplicaCreationData(BitStream &bs) const {}
void MissionObjective::serializeReplicaCreationData(BitStream &bs) const {}
void ObjectsGroup::serializeReplicaCreationData(BitStream &bs) const {}
void OrderPlayerProgress::serializeReplicaCreationData(BitStream &bs) const {}
void PickupZone::serializeReplicaCreationData(BitStream &bs) const {}
void RaceMode::serializeReplicaCreationData(BitStream &bs) const {}
void RearmZone::serializeReplicaCreationData(BitStream &bs) const {}
void RespawnBase::serializeReplicaCreationData(BitStream &bs) const {}
void Squadron::serializeReplicaCreationData(BitStream &bs) const {}
void UnitWinch::serializeReplicaCreationData(BitStream &bs) const {}
void Waypoint::serializeReplicaCreationData(BitStream &bs) const {}
void Wing::serializeReplicaCreationData(BitStream &bs) const {}