

#ifndef WTFILEUTILS_REPLICATED_DEFINE_H
#define WTFILEUTILS_REPLICATED_DEFINE_H
#include "ecs/query/event.h"


DEFINE_HANDLE(handle_replication)
#define REPLICATION_LOGI(format_, ...)  ELOGI(handle_replication, format_, __VA_ARGS__)
#define REPLICATION_LOGD1(format_, ...) ELOGD1(handle_replication, format_, __VA_ARGS__)
#define REPLICATION_LOGD2(format_, ...) ELOGD2(handle_replication, format_, __VA_ARGS__)
#define REPLICATION_LOGD3(format_, ...) ELOGD3(handle_replication, format_, __VA_ARGS__)
#define REPLICATION_LOGE(format_, ...)  ELOGE(handle_replication, format_, __VA_ARGS__)

void force_link_replication();

// id 0
class Airfield : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(Airfield, danet::ReplicatedObject)
};

// id 3
class DMSquad : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(DMSquad, danet::ReplicatedObject)
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

// id 17
class RaceMode : public danet::ReplicatedObject {
public:
  DECL_REPLICATION(RaceMode, danet::ReplicatedObject)
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


#endif // WTFILEUTILS_REPLICATED_DEFINE_H
