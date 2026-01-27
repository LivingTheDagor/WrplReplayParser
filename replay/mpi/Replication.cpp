#include "mpi/codegen/ReflIncludes.h"

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

danet::ReplicatedObject * Airfield::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * BombingZone::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * CaptureZone::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * DMSquad::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * ExitZone::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * FlightModelWrap::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * IGroundModel::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * InfantryTroop::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * InteractiveObject::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * InteractiveObjectProxy::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * MissionArea::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * MissionDrawing::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * MissionObjective::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * MissionZone::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * ObjectsGroup::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * OrderPlayerProgress::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * PickupZone::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * RaceMode::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * RearmZone::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * RespawnBase::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * Squadron::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * UnitWinch::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * Waypoint::createReplicatedObject(BitStream&) { return nullptr; }
danet::ReplicatedObject * Wing::createReplicatedObject(BitStream&) { return nullptr; }