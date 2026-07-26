
#include "ecs/ComponentTypes.h"
#include "ecs/ComponentTypes/objectType.h"
#include "ecs/ComponentTypes/arrayType.h"
#include "ecs/ComponentTypes/listType.h"
#include "ecs/entityId.h"
#include "math/dag_Point2.h"
#include "math/dag_Point3.h"
#include "math/dag_Point4.h"
#include "math/dag_e3dColor.h"
#include "math/dag_bounds3.h"
#include "math/dag_TMatrix.h"
#include "math/integer/dag_IPoint2.h"
#include "math/integer/dag_IPoint3.h"
#include "math/integer/dag_IPoint4.h"
#include "math/vecmath/dag_vecMath.h"
#include "dag/dag_vector.h"

#ifndef MYEXTENSION_COMPONENTTYPESDEFS_H
#define MYEXTENSION_COMPONENTTYPESDEFS_H

#include "BasicTypeDefs.h"
#include <cctype>

// ECS_DECLARE_POD_TYPE(ecs::EntityId) // actually uint32_t
// ECS_DECLARE_CREATABLE_TYPE(ecs::string)


// declares a type and its list, the type is a pod
#define POD_DEFS(nm, list_name, class_type)  \
  ECS_DECLARE_POD_TYPE(class_type)           \
  namespace nm {                             \
    using list_name = ecs::List<class_type>; \
  }                                          \
  ECS_DECLARE_CREATABLE_TYPE(nm::list_name)

// declares a type and its list, the type is a created
#define CREATEABLE_DEFS(nm, list_name, class_type) \
  ECS_DECLARE_CREATABLE_TYPE(class_type)           \
  namespace nm {                                   \
    using list_name = ecs::List<class_type>;       \
  }                                                \
  ECS_DECLARE_CREATABLE_TYPE(nm::list_name)

POD_DEFS(dm, PartIdList, dm::PartId)
POD_DEFS(props, PropsIdList, props::PropsId)
CREATEABLE_DEFS(ecs, StringList, ecs::string)
POD_DEFS(ecs, EidList, ecs::EntityId)
POD_DEFS(ecs, UInt8List, uint8_t)
POD_DEFS(ecs, UInt16List, uint16_t)
POD_DEFS(ecs, UInt32List, uint32_t)
POD_DEFS(ecs, UInt64List, uint64_t)
POD_DEFS(ecs, FloatList, float)
POD_DEFS(ecs, Point2List, Point2)
POD_DEFS(ecs, Point3List, Point3)
POD_DEFS(ecs, Point4List, Point4)
POD_DEFS(ecs, IPoint2List, IPoint2)
POD_DEFS(ecs, IPoint3List, IPoint3)
POD_DEFS(ecs, IPoint4List, IPoint4)
POD_DEFS(ecs, BoolList, bool)
POD_DEFS(ecs, TMatrixList, TMatrix)
POD_DEFS(ecs, ColorList, E3DCOLOR)
POD_DEFS(ecs, Int8List, int8_t)
POD_DEFS(ecs, Int16List, int16_t)
POD_DEFS(ecs, IntList, int)
POD_DEFS(ecs, Int64List, int64_t)

typedef ecs::UInt8List ProjectilePhysObject;

ECS_DECLARE_CREATABLE_TYPE(dag::Vector<dafg::NodeHandle>)

#undef POP_DEFS
#undef CREATEABLE_DEFS

ECS_DECLARE_CREATABLE_TYPE(ecs::Object)
ECS_DECLARE_CREATABLE_TYPE(ecs::Array)

ECS_DECLARE_POD_TYPE(DPoint3)
ECS_DECLARE_POD_TYPE(vec4f)
ECS_DECLARE_POD_TYPE(bbox3f)
ECS_DECLARE_POD_TYPE(mat44f)
ECS_DECLARE_POD_TYPE(BBox3)

void hello();


struct FieldSerializerDict {
  std::vector<uint8_t> data{};
  // std::unordered_map<uint16_t, std::vector<unsigned char>> data;
  std::string toString(int indent) const {
    return FormatHexToStream(std::span((char *) data.data(), data.size())).str();
  }
  bool operator==(const FieldSerializerDict &other) const = default;
};

struct BarrageBalloonStorageComponent : FieldSerializerDict {};
struct LightVehicleModelStorageComponent : FieldSerializerDict {};
struct FortificationModelStorageComponent : FieldSerializerDict {};
struct WalkerVehicleStorageComponent : FieldSerializerDict {};
struct HumanStorageComponent : FieldSerializerDict {};
struct InfantryTroopStorageComponent : FieldSerializerDict {};
struct WarShipModelStorageComponent : FieldSerializerDict {};
struct HeavyVehicleModelStorageComponent : FieldSerializerDict {};
struct FlightModelWrapStorageComponent : FieldSerializerDict {};


struct Rocket {
  ObjectRewindState<SpaceTimeEuler, false> positions{};
  uint32_t created_at_ms = 0xFFFFFFFF;
  uint32_t destroyed_at_ms = 0xFFFFFFFF; // when a rocket 'dies / explodes'


  uint32_t uleb_1;
  ecs::EntityId ownerEid;
  ecs::EntityId eid2;
  uint8_t u1_1;
  uint32_t u4_1;
  uint32_t weapon_ref; // maybe some flags?
  Point3 starting_pos;
  Point4 u16_1;
  Point3 u12_2;
  Point3 u12_3;
  uint8_t u1_2;
  uint8_t shell_type;
  float creation_time;
  uint32_t u4_4;
  Point3 u12_4;
  Point3 u12_5;
  uint8_t u1_4;
  float u4_5;
  BitStream some_weap_type_info;
  BitStream bomb_info; // only ever encodes a single 4 byte val
  BitStream maybe_sensor_info; // from what I can tell, this is always encoded
  uint8_t u1_5;
  uint32_t u4_6;
  uint8_t u1_6;
  Point2 u8_1;
  std::string toString(int indent) const {
    std::ostringstream oss{};
    oss << fmt::format("[Rocket, owner: {:#x};"
                       " other_eid: {:#x};"
                       " u1_1: {};"
                       " u4_1: {};"
                       " u4_2: {:#x};"
                       " starting_pos: {};"
                       " u16_1: {};"
                       " u12_2: {};"
                       " u12_3: {};"
                       " u1_2: {};"
                       " shell_type: {};"
                       " u4_4: {};"
                       " u12_4: {};"
                       " u12_5: {};"
                       " u1_4: {};"
                       " u4_5: {};"
                       " u1_5: {};"
                       " u4_6: {};"
                       " u1_6: {};"
                       " u8_1: {};",
                       ownerEid.get_handle(), eid2.get_handle(), u1_1, u4_1, weapon_ref, starting_pos.toString(0),
                       u16_1.toString(0), u12_2.toString(0), u12_3.toString(0), u1_2, shell_type, u4_4,
                       u12_4.toString(0), u12_5.toString(0), u1_4, u4_5, u1_5, u4_6, u1_6, u8_1.toString(0));
    return oss.str();
  }

  bool operator==(const Rocket &other) const = default;
};

struct Payload : Rocket {};
struct Bomb : Rocket {};
struct Jettisoned : Rocket {};
struct Torpedo : Rocket {};

namespace ecs {

  class FieldSerializerDictIO : public ecs::ComponentSerializer {
  public:
    void serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint,
                   ecs::EntityManager *mgr) override;

    bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint,
                     ecs::EntityManager *mgr) override;
  };

  class RocketSerializer : public ecs::ComponentSerializer {
  public:
    void serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint,
                   ecs::EntityManager *mgr) override;

    bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint,
                     ecs::EntityManager *mgr) override;
  };

  class RendInstDescSerializer : public ecs::ComponentSerializer {
    void serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint,
                   ecs::EntityManager *mgr) override;

    bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint,
                     ecs::EntityManager *mgr) override;
  };

} // namespace ecs

ECS_DECLARE_CREATABLE_TYPE(Rocket) // these need to be creatable as we have vectors in the struct
ECS_DECLARE_CREATABLE_TYPE(Payload)
ECS_DECLARE_CREATABLE_TYPE(Bomb)
ECS_DECLARE_CREATABLE_TYPE(Jettisoned)
ECS_DECLARE_CREATABLE_TYPE(Torpedo)


ECS_DECLARE_CREATABLE_TYPE(BarrageBalloonStorageComponent)
ECS_DECLARE_CREATABLE_TYPE(LightVehicleModelStorageComponent)
ECS_DECLARE_CREATABLE_TYPE(FortificationModelStorageComponent)
ECS_DECLARE_CREATABLE_TYPE(WalkerVehicleStorageComponent)
ECS_DECLARE_CREATABLE_TYPE(HumanStorageComponent)
ECS_DECLARE_CREATABLE_TYPE(InfantryTroopStorageComponent)
ECS_DECLARE_CREATABLE_TYPE(WarShipModelStorageComponent)
ECS_DECLARE_CREATABLE_TYPE(HeavyVehicleModelStorageComponent)
ECS_DECLARE_CREATABLE_TYPE(FlightModelWrapStorageComponent)


ECS_DECLARE_CREATABLE_TYPE(LootModelRes)
ECS_DECLARE_CREATABLE_TYPE(AimComponent)
ECS_DECLARE_CREATABLE_TYPE(UnitCrewLayout)
ECS_DECLARE_CREATABLE_TYPE(unitDmPartFx::UnitDmPartFx)
ECS_DECLARE_CREATABLE_TYPE(UnitFx)
ECS_DECLARE_CREATABLE_TYPE(FuelTanks)
ECS_DECLARE_CREATABLE_TYPE(HumanActor)
ECS_DECLARE_CREATABLE_TYPE(GridHolder)
ECS_DECLARE_CREATABLE_TYPE(GridObjComponent)
ECS_DECLARE_CREATABLE_TYPE(SmokeGridObject)
ECS_DECLARE_CREATABLE_TYPE(BackgroundModelRes)
ECS_DECLARE_CREATABLE_TYPE(FuelLeakEffectMgr)
ECS_DECLARE_CREATABLE_TYPE(ShipSinkingFxMgr)
ECS_DECLARE_CREATABLE_TYPE(unitPhysCls::PhysObjClsNodeActivationMgr)
ECS_DECLARE_CREATABLE_TYPE(TrackSound)
ECS_DECLARE_CREATABLE_TYPE(HudSkinElem)
ECS_DECLARE_CREATABLE_TYPE(GroundEffectManager)
ECS_DECLARE_CREATABLE_TYPE(dafg::NodeHandle)
ECS_DECLARE_CREATABLE_TYPE(LensFlareRenderer)
ECS_DECLARE_CREATABLE_TYPE(OutlineContexts)
ECS_DECLARE_CREATABLE_TYPE(DestructedModelRes)
ECS_DECLARE_CREATABLE_TYPE(WormVisual)

ECS_DECLARE_POD_TYPE(gpu_objects::riex_handles)
ECS_DECLARE_CREATABLE_TYPE(DagorAssetMgr)
ECS_DECLARE_CREATABLE_TYPE(SoundEventGroup)
ECS_DECLARE_CREATABLE_TYPE(AnimationFilterTags)
ECS_DECLARE_CREATABLE_TYPE(FrameFeatures)
ECS_DECLARE_CREATABLE_TYPE(ai::AgentDangers)
ECS_DECLARE_CREATABLE_TYPE(LightningFX)
ECS_DECLARE_CREATABLE_TYPE(pathfinder::CustomNav)
ECS_DECLARE_CREATABLE_TYPE(walkerai::AgentObstacles)
ECS_DECLARE_CREATABLE_TYPE(walkerai::Target)
ECS_DECLARE_CREATABLE_TYPE(rendinstfloating::PhysFloatingModel)
ECS_DECLARE_CREATABLE_TYPE(ProjectileImpulse)
ECS_DECLARE_CREATABLE_TYPE(CollisionObject)
ECS_DECLARE_CREATABLE_TYPE(SoundEventsPreload)
ECS_DECLARE_CREATABLE_TYPE(SoundVarId)
ECS_DECLARE_CREATABLE_TYPE(SoundStream)
ECS_DECLARE_CREATABLE_TYPE(SoundEvent)
ECS_DECLARE_CREATABLE_TYPE(PhysVars)
ECS_DECLARE_POD_TYPE(FastPhysTag)
ECS_DECLARE_CREATABLE_TYPE(AnimatedPhys)
ECS_DECLARE_CREATABLE_TYPE(EffectorData)
ECS_DECLARE_CREATABLE_TYPE(HumanAnimcharSound)
ECS_DECLARE_CREATABLE_TYPE(AnimcharSound)
ECS_DECLARE_CREATABLE_TYPE(AnimcharResourceReferenceHolder)
ECS_DECLARE_CREATABLE_TYPE(AnimcharNodesMat44)
ECS_DECLARE_CREATABLE_TYPE(AnimV20::AnimcharRendComponent)
ECS_DECLARE_CREATABLE_TYPE(AnimV20::AnimcharBaseComponent)
ECS_DECLARE_POD_TYPE(dm::FireDamageComponent)
ECS_DECLARE_POD_TYPE(dm::FireDamageState)

ECS_DECLARE_CREATABLE_TYPE(GpuObjectRiResourcePreload)
ECS_DECLARE_CREATABLE_TYPE(GlobalVisibleCoversMapMAX)
ECS_DECLARE_CREATABLE_TYPE(GlobalVisibleCoversMap4)
ECS_DECLARE_CREATABLE_TYPE(HumanVisibleCoversMap)
ECS_DECLARE_CREATABLE_TYPE(CoversComponent)

ECS_DECLARE_CREATABLE_TYPE(rendinst::RendInstDesc)
ECS_DECLARE_CREATABLE_TYPE(RiExtraComponent)
ECS_DECLARE_CREATABLE_TYPE(HumanAnimCtx)
ECS_DECLARE_CREATABLE_TYPE(PlaneActor)
ECS_DECLARE_CREATABLE_TYPE(OffenderData)
ECS_DECLARE_CREATABLE_TYPE(TwoPhysicalTracks)
ECS_DECLARE_CREATABLE_TYPE(BreachOffenderDataList)
ECS_DECLARE_CREATABLE_TYPE(ResizableDecals)
ECS_DECLARE_CREATABLE_TYPE(UniqueBufHolder)
ECS_DECLARE_CREATABLE_TYPE(EnviEmitterId)
ECS_DECLARE_CREATABLE_TYPE(EffectRef)
ECS_DECLARE_CREATABLE_TYPE(CameraFxEntity)
ECS_DECLARE_CREATABLE_TYPE(AreaFxEntity)
ECS_DECLARE_CREATABLE_TYPE(SpotLightEntity)
ECS_DECLARE_CREATABLE_TYPE(OmniLightEntity)
ECS_DECLARE_CREATABLE_TYPE(TheEffect)
ECS_DECLARE_CREATABLE_TYPE(RiExtraGen)
ECS_DECLARE_CREATABLE_TYPE(MountedGun)
ECS_DECLARE_CREATABLE_TYPE(daweap::Gun)
ECS_DECLARE_CREATABLE_TYPE(daweap::ShellResourceLoader)
ECS_DECLARE_POD_TYPE(ballistics::ProjectileBallistics)
ECS_DECLARE_POD_TYPE(ballistics::ProjectileProps)
ECS_DECLARE_CREATABLE_TYPE(daweap::LaunchDesc)
ECS_DECLARE_CREATABLE_TYPE(daweap::GunLaunchEvents)
ECS_DECLARE_CREATABLE_TYPE(daweap::GunDeviation)
ECS_DECLARE_CREATABLE_TYPE(GrenadeThrower)
ECS_DECLARE_CREATABLE_TYPE(EntityActions)
ECS_DECLARE_CREATABLE_TYPE(SimpleObjectModelResList)
ECS_DECLARE_CREATABLE_TYPE(SimpleObjectModelRes)
ECS_DECLARE_CREATABLE_TYPE(Camera)
ECS_DECLARE_CREATABLE_TYPE(SmokeFx)
ECS_DECLARE_CREATABLE_TYPE(shells::ShellRef)
// ECS_DECLARE_CREATABLE_TYPE(ProjectilePhysObject) // the deserialzer for this is VERY similar to that for
// ecs::Uint8List
ECS_DECLARE_CREATABLE_TYPE(Bullet)
ECS_DECLARE_CREATABLE_TYPE(VehiclePhysActor)
ECS_DECLARE_CREATABLE_TYPE(UnitByEid)
ECS_DECLARE_CREATABLE_TYPE(unit::UnitRef)
ECS_DECLARE_CREATABLE_TYPE(dm::ExplosiveProperties)
ECS_DECLARE_CREATABLE_TYPE(dm::splash::Properties)
ECS_DECLARE_CREATABLE_TYPE(AmmoStowageMassToSplashList)
ECS_DECLARE_CREATABLE_TYPE(AmmoStowageSlotCollAndGeomNodesList)
ECS_DECLARE_CREATABLE_TYPE(dm::DamagePartProps)
ECS_DECLARE_CREATABLE_TYPE(GameObjects)
ECS_DECLARE_CREATABLE_TYPE(CollisionResource)
ECS_DECLARE_CREATABLE_TYPE(freeAreaCheck::CheckTracesMgr)
ECS_DECLARE_CREATABLE_TYPE(TargetSignatureDetectorContainer)
ECS_DECLARE_CREATABLE_TYPE(GuidanceLockPtr)
ECS_DECLARE_CREATABLE_TYPE(GuidancePtr)
ECS_DECLARE_CREATABLE_TYPE(AnimIrqToEventComponent)
ECS_DECLARE_CREATABLE_TYPE(ecs::SharedComponent<SharedPrecomputedWeaponPositions>)
ECS_DECLARE_CREATABLE_TYPE(ecs::SharedComponent<::ecs::Array>)
ECS_DECLARE_CREATABLE_TYPE(ecs::SharedComponent<::ecs::Object>)
ECS_DECLARE_CREATABLE_TYPE(ecs::SharedComponent<CapsuleApproximation>)
ECS_DECLARE_CREATABLE_TYPE(PhysRagdoll)
ECS_DECLARE_CREATABLE_TYPE(PhysBody)
ECS_DECLARE_CREATABLE_TYPE(LightingAnimchar)
ECS_DECLARE_CREATABLE_TYPE(HeatSourceId)
ECS_DECLARE_CREATABLE_TYPE(Footstep)
ECS_DECLARE_CREATABLE_TYPE(FootstepFx)
ECS_DECLARE_CREATABLE_TYPE(MotionMatchingController)
ECS_DECLARE_CREATABLE_TYPE(AnimationDataBase)
ECS_DECLARE_CREATABLE_TYPE(CapsulesAOHolder)
ECS_DECLARE_CREATABLE_TYPE(ecs::TemplatesListToInstantiate)
ECS_DECLARE_CREATABLE_TYPE(BehaviourTree)
ECS_DECLARE_CREATABLE_TYPE(BufferedHudData)
ECS_DECLARE_CREATABLE_TYPE(InvalidType)
ECS_DECLARE_CREATABLE_TYPE(LaserDecalManager)
ECS_DECLARE_CREATABLE_TYPE(dm::SplashWave)
ECS_DECLARE_CREATABLE_TYPE(UniqueBufWithShaderVar)
ECS_DECLARE_CREATABLE_TYPE(SoundOcclusionBlob)
ECS_DECLARE_CREATABLE_TYPE(aimmem::AimingMemPoints)

#include "ecs/ComponentPrintingImplementations.h"

#endif // MYEXTENSION_COMPONENTTYPESDEFS_H
