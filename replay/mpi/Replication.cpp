#include "mpi/codegen/ReflIncludes.h"
#include "state/ParserState.h"

void force_link_replication() {
  std::cout << "";
}

IMPLEMENT_REPLICATION(Airfield)

IMPLEMENT_REPLICATION(BombingZone)

IMPLEMENT_REPLICATION(CaptureZone)

IMPLEMENT_REPLICATION(DMSquad)

IMPLEMENT_REPLICATION(ExitZone)

IMPLEMENT_REPLICATION(FlightModelWrap)

IMPLEMENT_REPLICATION(IGroundModel)

IMPLEMENT_REPLICATION(InfantryTroop)

IMPLEMENT_REPLICATION(InteractiveObject)

IMPLEMENT_REPLICATION(InteractiveObjectProxy)

IMPLEMENT_REPLICATION(MissionArea)

IMPLEMENT_REPLICATION(MissionDrawing)

IMPLEMENT_REPLICATION(MissionObjective)

IMPLEMENT_REPLICATION(MissionZone)

IMPLEMENT_REPLICATION(ObjectsGroup)

IMPLEMENT_REPLICATION(OrderPlayerProgress)

IMPLEMENT_REPLICATION(PickupZone)

IMPLEMENT_REPLICATION(RaceMode)

IMPLEMENT_REPLICATION(RearmZone)

IMPLEMENT_REPLICATION(RespawnBase)

IMPLEMENT_REPLICATION(Squadron)

IMPLEMENT_REPLICATION(UnitWinch)

IMPLEMENT_REPLICATION(Waypoint)

IMPLEMENT_REPLICATION(Wing)
danet::ReplicatedObject * Airfield::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated Airfield");
  return nullptr;
}

danet::ReplicatedObject * BombingZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated BombingZone");
  auto obj = new BombingZone();
  state->Zones.push_back(obj);
  return obj;
}

danet::ReplicatedObject * CaptureZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated CaptureZone");
  auto obj = new CaptureZone();
  state->Zones.push_back(obj);
  return nullptr;
}

danet::ReplicatedObject * DMSquad::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated DMSquad");
  return nullptr;
}

danet::ReplicatedObject * ExitZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated ExitZone");
  auto obj = new ExitZone();
  state->Zones.push_back(obj);
  return nullptr;
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
  LOG("Parsing Replicated PickupZone");
  auto obj = new PickupZone();
  state->Zones.push_back(obj);
  return nullptr;
}

danet::ReplicatedObject * RaceMode::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated RaceMode");
  return nullptr;
}

danet::ReplicatedObject * RearmZone::createReplicatedObject(BitStream &bs, ParserState *state) {
  LOG("Parsing Replicated RearmZone");
  auto obj = new RearmZone();
  state->Zones.push_back(obj);
  return nullptr;
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