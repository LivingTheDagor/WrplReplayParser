

#ifndef MYEXTENSION_BASICTYPEDEFS_H
#define MYEXTENSION_BASICTYPEDEFS_H
#include <cstdint>
#include <cstring>

struct LootModelRes {
  char data[8]  = {0};
};

struct TwoPhysicalTracks {
  char data[8]  = {0};
};

struct BreachOffenderDataList {
  char data[8]  = {0};
};

struct AimComponent {
  char data[8]  = {0};
};

struct UnitCrewLayout {
  char data[8]  = {0};
};
namespace unitDmPartFx {
  struct UnitDmPartFx {
    char data[8]  = {0};
  };

}
struct UnitFx {
  char data[8]  = {0};
};

struct FuelTanks {
  char data[8]  = {0};
};
struct HumanActor {
  char data[8]  = {0};
};
struct GridHolder {
  char data[8]  = {0};
};
struct GridObjComponent {
  char data[8]  = {0};
};
struct SmokeGridObject {
  char data[8]  = {0};
};
struct BackgroundModelRes {
  char data[8]  = {0};
};
struct FuelLeakEffectMgr {
  char data[8]  = {0};
};
struct ShipSinkingFxMgr {
  char data[8]  = {0};
};
namespace unitPhysCls {
  struct PhysObjClsNodeActivationMgr {
    char data[8]  = {0};
  };
}
struct TrackSound
{
  char data[8] = {0};
};
struct HudSkinElem
{
  char data[8] = {0};
};
struct GroundEffectManager
{
  char data[8] = {0};
};
namespace dafg
{
  struct NodeHandle
  {
    char data[8] = {0};
    bool operator==(const NodeHandle& other) const {
      return std::memcmp(data, other.data, sizeof(data)) == 0;
    }
  };
}

struct LensFlareRenderer {
  char data[8] = {0};
};

struct OutlineContexts {
  char data[8] = {0};
};

struct DestructedModelRes {
  char data[8] = {0};
};

struct WormVisual {
  char data[8] = {0};
};


namespace gpu_objects {
  struct riex_handles {
    char data[520]  = {0};
  };
}
struct DagorAssetMgr {
  char data[1]  = {0};
};
struct SoundEventGroup {
  char data[488]  = {0};
};
struct AnimationFilterTags {
  char data[24]  = {0};
};
struct FrameFeatures {
  char data[32]  = {0};
};
struct LightningFX {
  char data[132]  = {0};
};
namespace ai {
  struct AgentDangers {
    char data[24]  = {0};
  };
}
namespace pathfinder {
  struct CustomNav {
    char data[288]  = {0};
  };
}
namespace walkerai {
  struct AgentObstacles {
    char data[16]  = {0};
  };
  struct Target {
    char data[40]  = {0};
  };
}

namespace rendinstfloating {
  struct PhysFloatingModel {
    char data[96]  = {0};
  };
}
struct ProjectileImpulse {
  char data[24]  = {0};
};
struct CollisionObject {
  char data[16]  = {0};
};
struct SoundEventsPreload {
  char data[136]  = {0};
};
struct SoundVarId {
  char data[8]  = {0};
};
struct SoundStream {
  char data[8]  = {0};
};
struct SoundEvent {
  char data[8]  = {0};
};
struct PhysVars {
  char data[112]  = {0};
};
struct CollisionResource {
  char data[8]  = {0}; // ptr
};

//struct FastPhysTag {
//  char data[0]  = {0};
//};
struct AnimatedPhys {
  char data[48]  = {0};
};
struct EffectorData {
  char data[72]  = {0};
};
struct HumanAnimcharSound {
  char data[8]  = {0};
};
struct AnimcharSound {
  char data[8]  = {0};
};
struct AnimcharResourceReferenceHolder {
  char data[8]  = {0};
};
struct AnimcharNodesMat44 {
  char data[48]  = {0};
};
namespace AnimV20 {
  struct AnimcharRendComponent {
    char data[96]  = {0};
  };
}
namespace AnimV20 {
  struct AnimcharBaseComponent {
    char data[416]  = {0};
  };
}


struct GpuObjectRiResourcePreload {
  char data[1]  = {0};
};
struct GlobalVisibleCoversMapMAX {
  char data[32]  = {0};
};
struct GlobalVisibleCoversMap4 {
  char data[32]  = {0};
};
struct HumanVisibleCoversMap {
  char data[24]  = {0};
};
struct CoversComponent {
  char data[464]  = {0};
};

namespace rendinst {
  struct RendInstDesc {
    uint32_t v1;
    uint32_t v2;
    uint32_t v3;
    char data[20]  = {0};
  };

  typedef uint64_t riex_handle_t;

  // 16.32u for ri_idx/instance
  typedef uint64_t riex_handle_t;
  inline constexpr riex_handle_t RIEX_HANDLE_NULL = ~riex_handle_t(0);
  inline constexpr uint32_t ri_instance_type_shift = 32, ri_instance_inst_mask = ~0u;

  constexpr inline uint32_t riex_max_type() { return (1u << (48 - ri_instance_type_shift)) - 1; }
  constexpr inline uint32_t riex_max_inst() { return ri_instance_inst_mask; }

  inline uint32_t handle_to_ri_type(riex_handle_t h) { return (uint32_t)(h >> ri_instance_type_shift); }
  inline uint32_t handle_to_ri_inst(riex_handle_t h) { return (uint32_t)(h & ri_instance_inst_mask); }
  inline riex_handle_t make_handle(uint32_t ri_type, uint32_t ri_inst)
  {
    return (uint64_t(ri_type) << ri_instance_type_shift) | ri_inst;
  }
}
struct RiExtraComponent {
  char data[8]  = {0};
};

struct HumanAnimCtx {
  char data[340]  = {0};
};
struct PlaneActor {
  char data[16]  = {0};
};
struct OffenderData {
  char data[176]  = {0};
};
struct ResizableDecals {
  char data[184]  = {0};
};
struct UniqueBufHolder {
  char data[24]  = {0};
};
struct EnviEmitterId {
  char data[4]  = {0};
};
struct EffectRef {
  char data[8] = {0};
};
struct CameraFxEntity {
  char data[4] = {0};
};
struct AreaFxEntity {
  char data[4] = {0};
};
struct SpotLightEntity {
  char data[4] = {0};
};
struct OmniLightEntity {
  char data[4] = {0};
};
struct TheEffect {
  char data[16] = {0};
};
struct RiExtraGen {
  char data[1] = {0};
};
struct MountedGun {
  char data[168] = {0};
};

namespace ballistics {
  struct ProjectileBallistics {
    char data[80] = {0};
  };
  struct ProjectileProps {
    char data[64] = {0};
  };
}
namespace daweap {
  struct Gun {
    char data[536] = {0};
  };
  struct LaunchDesc {
    char data[84] = {0};
  };
  struct GunLaunchEvents {
    char data[24] = {0};
  };
  struct GunDeviation {
    char data[124] = {0};
  };

  struct ShellResourceLoader {
    char data[8];
  };

}
struct GrenadeThrower {
  char data[16] = {0};
};
struct EntityActions {
  char data[32] = {0};
};
struct SimpleObjectModelResList {
  char data[24] = {0};
};
struct SimpleObjectModelRes {
  char data[8] = {0};
};
struct Camera {
  char data[256] = {0};
};
struct SmokeFx {
  char data[32] = {0};
};
namespace shells {
  struct ShellRef {
    char data[16] = {0};
  };
}
struct Bullet {
  char data[776] = {0};
};
struct VehiclePhysActor {
  char data[16] = {0};
};
struct UnitByEid {
  char data[16] = {0};
};
namespace unit {
  struct UnitRef {
    char data[16] = {0};
  };
}

namespace dm {
  namespace splash {
    struct Properties {
      char data[28] = {0};
    };
  }
  struct ExplosiveProperties {
    char data[24] = {0};
  };
  struct FireDamageState {
    char data[24] = {0};
  };
  struct FireDamageComponent {
    char data[40] = {0};
  };
  struct DamagePartProps {
    char data[576] = {0};
  };
}
struct AmmoStowageMassToSplashList {
  char data[24] = {0};
};
struct AmmoStowageSlotCollAndGeomNodesList {
  char data[24] = {0};
};

struct GameObjects {
  char data[40] = {0};
};

namespace dm {
  struct PartId //: public MemcmpEquality<PartId>
  {
    uint8_t data = 0;

    bool operator==(const PartId &other) const {
      return std::memcmp(this, &other, sizeof(PartId)) == 0;
    }
    std::string toString() const {
      return fmt::format("{}", data);
    }
  };

}

namespace props {
  struct PropsId //: public MemcmpEquality<PropsId>
  {
    uint32_t data = 0;

    bool operator==(const PropsId &other) const {
      return std::memcmp(this, &other, sizeof(PropsId)) == 0;
    }
  };
}

namespace freeAreaCheck {
  struct CheckTracesMgr {
    char data[8]; // boxed
  };
}

struct TargetSignatureDetectorContainer {
  char data[8];
};

struct GuidanceLockPtr {
  char data[8];
};

struct GuidancePtr {
  char data[8];
};

struct FastPhysTag : public ecs::Tag {};

struct AnimIrqToEventComponent {
  char data[8];
};

// for now, we only need a dummy sharedComponent
namespace ecs {
  template <typename T>
  struct SharedComponent {
    char data[8];
  };
  struct TemplatesListToInstantiate {
    char data[8];
  };
}

struct BehaviourTree {
  char data[8];
};

struct SharedPrecomputedWeaponPositions {
  char data[8];
};

struct CapsuleApproximation {
  char data[8];
};

struct PhysRagdoll {
  char data[8];
};

struct PhysBody {
  char data[8];
};

struct LightingAnimchar {
  char data[8];
};


struct HeatSourceId {
  char data[8];
};

struct Footstep {
  char data[8];
};

struct FootstepFx {
  char data[8];
};

struct MotionMatchingController {
  char data[8];
};

struct AnimationDataBase {
  char data[8];
};

struct CapsulesAOHolder {
  char data[8];
};

#endif //MYEXTENSION_BASICTYPEDEFS_H
