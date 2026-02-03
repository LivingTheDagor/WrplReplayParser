
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

#ifndef MYEXTENSION_COMPONENTTYPESDEFS_H
#define MYEXTENSION_COMPONENTTYPESDEFS_H

#include "BasicTypeDefs.h"
#include <cctype>

// ECS_DECLARE_POD_TYPE(ecs::EntityId) // actually uint32_t
// ECS_DECLARE_CREATABLE_TYPE(ecs::string)


// declares a type and its list, the type is a pod
#define POD_DEFS(nm, list_name, class_type) \
ECS_DECLARE_POD_TYPE(class_type)        \
namespace nm {                         \
using list_name = ecs::List<class_type>;\
}                                       \
ECS_DECLARE_CREATABLE_TYPE(nm::list_name)

// declares a type and its list, the type is a created
#define CREATEABLE_DEFS(nm, list_name, class_type) \
ECS_DECLARE_CREATABLE_TYPE(class_type)         \
namespace nm {                                \
using list_name = ecs::List<class_type>;       \
}                                              \
ECS_DECLARE_CREATABLE_TYPE(nm::list_name)

POD_DEFS(dm, PartIdList, dm::PartId)
POD_DEFS(props, PropsIdList, props::PropsId)
CREATEABLE_DEFS(ecs, StringList, ecs::string)
ECS_DECLARE_POD_TYPE(ecs::EntityId)
namespace ecs { using EidList = ecs::List<ecs::EntityId>; }
ECS_DECLARE_BASE_TYPE(ecs::EidList, "ecs::EidList", true);
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

namespace dag {
  template<typename T>
  struct Vector : public std::vector<T> {
  };
}

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
  std::unordered_map<uint16_t, std::vector<unsigned char>> data;

  //FieldSerializerDict() {
  //  data = new std::unordered_map<uint16_t, std::vector<unsigned char>>();
  //}

  //~FieldSerializerDict() {
  //for (auto &s: *data) {
  //  if (s.second.data()) {
  //    free(s.second.data());
  //  }
  //}
  //  delete data;
  //}
  std::string toString() const {
    std::ostringstream oss;
    oss << fmt::format("({})", data.size());
    oss << "{";
    for (const auto &pair: data) {
      oss << fmt::format("{}: b'", pair.first);
      FormatBytesToStream(oss, std::span<char>((char *) pair.second.data(), pair.second.size()));
      oss << "', ";
    }
    oss << "}";
    return oss.str();
  }
};

struct BarrageBalloonStorageComponent : FieldSerializerDict {
};
struct LightVehicleModelStorageComponent : FieldSerializerDict {
};
struct FortificationModelStorageComponent : FieldSerializerDict {
};
struct WalkerVehicleStorageComponent : FieldSerializerDict {
};
struct HumanStorageComponent : FieldSerializerDict {
};
struct InfantryTroopStorageComponent : FieldSerializerDict {
};
struct WarShipModelStorageComponent : FieldSerializerDict {
};
struct HeavyVehicleModelStorageComponent : FieldSerializerDict {
};
struct FlightModelWrapStorageComponent : FieldSerializerDict {
};


struct Rocket {
  uint32_t uleb_1;
  ecs::EntityId eid;
  ecs::EntityId eid2;
  uint8_t u1_1;
  uint32_t u4_1;
  uint32_t u4_2;
  char u12_1[12];
  char u16_1[16];
  char u12_2[12];
  char u12_3[12];
  uint8_t u1_2;
  uint8_t u1_3;
  uint32_t u4_3;
  uint32_t u4_4;
  char u12_4[12];
  char u12_5[12];
  uint8_t u1_4;
  uint32_t u4_5;
  std::vector<char> v1;
  std::vector<char> v2;
  uint8_t u1_5;
  uint32_t u4_6;
  uint8_t u1_6;
  uint64_t u8_1;
};

struct Payload : Rocket {
};
struct Bomb : Rocket {
};
struct Jettisoned : Rocket {
};
struct Torpedo : Rocket {
};

namespace ecs {

  class FieldSerializerDictIO : public ecs::ComponentSerializer {
  public:
    void
    serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) override;

    bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint,
                     ecs::EntityManager *mgr) override;
  };

  class RocketSerializer : public ecs::ComponentSerializer {
  public:
    void
    serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) override;

    bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint,
                     ecs::EntityManager *mgr) override;
  };

  class RendInstDescSerializer : public ecs::ComponentSerializer {
    void
    serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) override;

    bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint,
                     ecs::EntityManager *mgr) override;
  };
}

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
//ECS_DECLARE_CREATABLE_TYPE(ProjectilePhysObject) // the deserialzer for this is VERY similar to that for ecs::Uint8List
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

#include "ecs/ComponentPrintingImplementations.h"

#endif //MYEXTENSION_COMPONENTTYPESDEFS_H
