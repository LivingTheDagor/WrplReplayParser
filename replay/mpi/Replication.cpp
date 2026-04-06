#include "mpi/codegen/ReflIncludes.h"
#include "state/ParserState.h"

void force_link_replication() {
  std::cout << "";
}


danet::ReplicatedObject* create_zone(BitStream &bs, uint8_t zone_id, ParserState * state) {
  IdFieldSerializer255 serializer255{};
  BitSize_t end;
  uint32_t count = serializer255.readFieldsSizeAndCount(bs, end);
  G_ASSERT(serializer255.readFieldsIndex(bs));
  auto local_44 = bs.GetReadOffset();
  //LOGE("Creating Zone");
  uint8_t mission_area_id;
  uint8_t maybe_team_id;
  uint32_t some_val;
  uint16_t some_val_2;
  for(uint16_t i = 0; i < count; i++) {
    BitSize_t start_size = bs.GetReadOffset();
    switch (serializer255.getFieldId(i)) {
      case 1: {
        bs.Read(mission_area_id);
        //LOGE("Mission area_id: {}", mission_area_id);
        break;
      }
      case 2: {
        bs.Read(maybe_team_id);
        //LOGE("maybe_team_id: {}", maybe_team_id);
        break;
      }
      case 3: {
        bs.ReadCompressed(some_val);
        //LOGE("some_val_1: {}", some_val);
        break;
      }
      case 4: {
        bs.Read(some_val_2);
        //LOGE("some_val_2: {}", some_val_2);
        break;
      }
      default:
        EXCEPTION("");
    }
    G_ASSERT(bs.GetReadOffset()-start_size == serializer255.getFieldSize(i));

    //LOGE("{}; data: {}", serializer255.getFieldId(i), FormatHexToStream(data).str());
  }
  //LOGE("mission_area_id: {}; maybe_team_id: {}, some_val: {}; some_val_2: {}", mission_area_id, maybe_team_id, some_val, some_val_2);
  if(mission_area_id == state->Zones.size()) {
    BaseZone * obj;
    switch(zone_id) {
      case 0: {obj = new BombingZone(); break;}
      case 1: {obj = new CaptureZone(); break;}
      case 2: {obj = new RearmZone(); break;}
      case 3: {obj = new ExitZone(); break;}
      case 4: {obj = new PickupZone(); break;}
      default:
        EXCEPTION("Invalid Zone id: {}", zone_id);
    }
    state->Zones.push_back(obj);
  }
  bs.SetReadOffset(end);
  return state->Zones[mission_area_id];
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

IMPLEMENT_REPLICATION(Squadron)  // 20

IMPLEMENT_REPLICATION(UnitWinch) // 21

IMPLEMENT_REPLICATION(Waypoint) // 22

IMPLEMENT_REPLICATION(Wing) // 23

IMPLEMENT_REPLICATION(BaseZone) // 24, but should not be used eveeeeeeer

danet::ReplicatedObject * BaseZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  EXCEPTION("this should never be created");
}

danet::ReplicatedObject * Airfield::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated Airfield");
  return nullptr;
}

danet::ReplicatedObject * BombingZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  //LOGE("Parsing Replicated BombingZone");
  auto zone = create_zone(bs, 0, state);
  IdFieldSerializer255 serializer255{};
  BitSize_t end;
  auto count = serializer255.readFieldsSizeAndCount(bs, end);
  G_ASSERT(serializer255.readFieldsIndex(bs));
  for(uint16_t i = 0; i < count; i++) {
    std::vector<char> data{};
    data.resize(BITS_TO_BYTES(serializer255.getFieldSize(i)));
    bs.ReadBits((uint8_t*)data.data(), serializer255.getFieldSize(i));
    //LOGE("Index: {}; size: {}; data: {}", serializer255.getFieldId(i), serializer255.getFieldSize(i), FormatHexToStream(data).str());
  }
  return zone;
}

danet::ReplicatedObject * CaptureZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  //LOGE("Parsing Replicated CaptureZone");
  auto zone = create_zone(bs, 1, state);
  IdFieldSerializer255 serializer255{};
  BitSize_t end;
  auto count = serializer255.readFieldsSizeAndCount(bs, end);
  G_ASSERT(serializer255.readFieldsIndex(bs));
  for(uint16_t i = 0; i < count; i++) {
    std::vector<char> data{};
    data.resize(BITS_TO_BYTES(serializer255.getFieldSize(i)));
    bs.ReadBits((uint8_t*)data.data(), serializer255.getFieldSize(i));
    //LOGE("Index: {}; size: {}; data: {}", serializer255.getFieldId(i), serializer255.getFieldSize(i), FormatHexToStream(data).str());
  }
  return zone;
}

danet::ReplicatedObject * DMSquad::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated DMSquad");
  return nullptr;
}

danet::ReplicatedObject * ExitZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOGE("Parsing Replicated ExitZone");
  return create_zone(bs, 3, state);
}

danet::ReplicatedObject * FlightModelWrap::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated FlightModelWrap");
  return nullptr;
}

danet::ReplicatedObject * IGroundModel::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated IGroundModel");
  return nullptr;
}

danet::ReplicatedObject * InfantryTroop::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated InfantryTroop");
  return nullptr;
}

danet::ReplicatedObject * InteractiveObject::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated InteractiveObject");
  return nullptr;
}

danet::ReplicatedObject * InteractiveObjectProxy::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated InteractiveObjectProxy");
  return nullptr;
}

danet::ReplicatedObject * MissionArea::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated MissionArea");
  return nullptr;
}

danet::ReplicatedObject * MissionDrawing::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated MissionDrawing");
  return nullptr;
}

danet::ReplicatedObject * MissionObjective::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated MissionObjective");
  return nullptr;
}

danet::ReplicatedObject * MissionZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated MissionZone");

  return nullptr;
}

danet::ReplicatedObject * ObjectsGroup::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated ObjectsGroup");
  return nullptr;
}

danet::ReplicatedObject * OrderPlayerProgress::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated OrderPlayerProgress");
  return nullptr;
}

danet::ReplicatedObject * PickupZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOGE("Parsing Replicated PickupZone");
  return create_zone(bs, 4, state);
}

danet::ReplicatedObject * RaceMode::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated RaceMode");
  return nullptr;
}

danet::ReplicatedObject * RearmZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOGE("Parsing Replicated RearmZone");
  return create_zone(bs, 2, state);
}

danet::ReplicatedObject * RespawnBase::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated RespawnBase");
  return nullptr;
}

danet::ReplicatedObject * Squadron::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated Squadron");
  return nullptr;
}

danet::ReplicatedObject * UnitWinch::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated UnitWinch");
  return nullptr;
}

danet::ReplicatedObject * Waypoint::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated Waypoint");
  return nullptr;
}

danet::ReplicatedObject * Wing::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated Wing");
  return nullptr;
}


void Airfield::serializeReplicaCreationData(BitStream &bs) const {}
void BaseZone::serializeReplicaCreationData(BitStream &bs) const {}
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
void MissionZone::serializeReplicaCreationData(BitStream &bs) const {}
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