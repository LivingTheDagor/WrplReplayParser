

#ifndef WTFILEUTILS_REPLICATED_DEFINE_H
#define WTFILEUTILS_REPLICATED_DEFINE_H

void force_link_replication();

// id 0
class Airfield : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(Airfield, danet::ReplicatedObject)
};

// id 1
class BombingZone : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(BombingZone, danet::ReplicatedObject)
};

// id 2
class CaptureZone : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(CaptureZone, danet::ReplicatedObject)
};

// id 3
class DMSquad : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(DMSquad, danet::ReplicatedObject)
};

// id 4
class ExitZone : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(ExitZone, danet::ReplicatedObject)
};

// id 5
class FlightModelWrap : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(FlightModelWrap, danet::ReplicatedObject)
};

// id 6
class IGroundModel : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(IGroundModel, danet::ReplicatedObject)
};

// id 7
class InfantryTroop : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(InfantryTroop, danet::ReplicatedObject)
};

// id 8
class InteractiveObject : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(InteractiveObject, danet::ReplicatedObject)
};

// id 9
class InteractiveObjectProxy : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(InteractiveObjectProxy, danet::ReplicatedObject)
};

// id 10
class MissionArea : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(MissionArea, danet::ReplicatedObject)
};

// id 11
class MissionDrawing : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(MissionDrawing, danet::ReplicatedObject)
};

// id 12
class MissionObjective : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(MissionObjective, danet::ReplicatedObject)
};

// id 13
class MissionZone : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(MissionZone, danet::ReplicatedObject)
};

// id 14
class ObjectsGroup : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(ObjectsGroup, danet::ReplicatedObject)
};

// id 15
class OrderPlayerProgress : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(OrderPlayerProgress, danet::ReplicatedObject)
};

// id 16
class PickupZone : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(PickupZone, danet::ReplicatedObject)
};

// id 17
class RaceMode : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(RaceMode, danet::ReplicatedObject)
};

// id 18
class RearmZone : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(RearmZone, danet::ReplicatedObject)
};

// id 19
class RespawnBase : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(RespawnBase, danet::ReplicatedObject)
};

// id 20
class Squadron : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(Squadron, danet::ReplicatedObject)
};

// id 21
class UnitWinch : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(UnitWinch, danet::ReplicatedObject)
};

// id 22
class Waypoint : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(Waypoint, danet::ReplicatedObject)
};

// id 23
class Wing : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(Wing, danet::ReplicatedObject)
};




#endif //WTFILEUTILS_REPLICATED_DEFINE_H
