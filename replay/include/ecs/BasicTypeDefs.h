

#ifndef MYEXTENSION_BASICTYPEDEFS_H
#define MYEXTENSION_BASICTYPEDEFS_H
#include <cstdint>
#include <cstring>
#include "Unit.h"


struct LootModelRes {
  char data[8] = {0};
  bool operator==(const LootModelRes &other) const { return true; }
};

struct TwoPhysicalTracks {
  char data[8] = {0};
  bool operator==(const TwoPhysicalTracks &other) const { return true; }
};

struct BreachOffenderDataList {
  char data[8] = {0};
  bool operator==(const BreachOffenderDataList &other) const { return true; }
};

struct AimComponent {
  char data[8] = {0};
  bool operator==(const AimComponent &other) const { return true; }
};

struct UnitCrewLayout {
  char data[8] = {0};
  bool operator==(const UnitCrewLayout &other) const { return true; }
};
namespace unitDmPartFx {
  struct UnitDmPartFx {
    char data[8] = {0};
    bool operator==(const UnitDmPartFx &other) const { return true; }
  };

} // namespace unitDmPartFx
struct UnitFx {
  char data[8] = {0};
  bool operator==(const UnitFx &other) const { return true; }
};

struct FuelTanks {
  char data[8] = {0};
  bool operator==(const FuelTanks &other) const { return true; }
};
struct HumanActor {
  char data[8] = {0};
  bool operator==(const HumanActor &other) const { return true; }
};
struct GridHolder {
  char data[8] = {0};
  bool operator==(const GridHolder &other) const { return true; }
};
struct GridObjComponent {
  char data[8] = {0};
  bool operator==(const GridObjComponent &other) const { return true; }
};
struct SmokeGridObject {
  char data[8] = {0};
  bool operator==(const SmokeGridObject &other) const { return true; }
};
struct BackgroundModelRes {
  char data[8] = {0};
  bool operator==(const BackgroundModelRes &other) const { return true; }
};
struct FuelLeakEffectMgr {
  char data[8] = {0};
  bool operator==(const FuelLeakEffectMgr &other) const { return true; }
};
struct ShipSinkingFxMgr {
  char data[8] = {0};
  bool operator==(const ShipSinkingFxMgr &other) const { return true; }
};
namespace unitPhysCls {
  struct PhysObjClsNodeActivationMgr {
    char data[8] = {0};
    bool operator==(const PhysObjClsNodeActivationMgr &other) const { return true; }
  };
} // namespace unitPhysCls
struct TrackSound {
  char data[8] = {0};
  bool operator==(const TrackSound &other) const { return true; }
};
struct HudSkinElem {
  char data[8] = {0};
  bool operator==(const HudSkinElem &other) const { return true; }
};
struct GroundEffectManager {
  char data[8] = {0};
  bool operator==(const GroundEffectManager &other) const { return true; }
};
namespace dafg {
  struct NodeHandle {
    char data[8] = {0};
    bool operator==(const NodeHandle &other) const { return true; }
  };
} // namespace dafg

struct LensFlareRenderer {
  char data[8] = {0};
  bool operator==(const LensFlareRenderer &other) const { return true; }
};

struct OutlineContexts {
  char data[8] = {0};
  bool operator==(const OutlineContexts &other) const { return true; }
};

struct DestructedModelRes {
  char data[8] = {0};
  bool operator==(const DestructedModelRes &other) const { return true; }
};

struct WormVisual {
  char data[8] = {0};
  bool operator==(const WormVisual &other) const { return true; }
};


namespace gpu_objects {
  struct riex_handles {
    char data[520] = {0};
    bool operator==(const riex_handles &other) const { return true; }
  };
} // namespace gpu_objects
struct DagorAssetMgr {
  char data[1] = {0};
  bool operator==(const DagorAssetMgr &other) const { return true; }
};
struct SoundEventGroup {
  char data[488] = {0};
  bool operator==(const SoundEventGroup &other) const { return true; }
};
struct AnimationFilterTags {
  char data[24] = {0};
  bool operator==(const AnimationFilterTags &other) const { return true; }
};
struct FrameFeatures {
  char data[32] = {0};
  bool operator==(const FrameFeatures &other) const { return true; }
};
struct LightningFX {
  char data[132] = {0};
  bool operator==(const LightningFX &other) const { return true; }
};
namespace ai {
  struct AgentDangers {
    char data[24] = {0};
    bool operator==(const AgentDangers &other) const { return true; }
  };
} // namespace ai
namespace pathfinder {
  struct CustomNav {
    char data[288] = {0};
    bool operator==(const CustomNav &other) const { return true; }
  };
} // namespace pathfinder
namespace walkerai {
  struct AgentObstacles {
    char data[16] = {0};
    bool operator==(const AgentObstacles &other) const { return true; }
  };
  struct Target {
    char data[40] = {0};
    bool operator==(const Target &other) const { return true; }
  };
} // namespace walkerai

namespace rendinstfloating {
  struct PhysFloatingModel {
    char data[96] = {0};
    bool operator==(const PhysFloatingModel &other) const { return true; }
  };
} // namespace rendinstfloating
struct ProjectileImpulse {
  char data[24] = {0};
  bool operator==(const ProjectileImpulse &other) const { return true; }
};
struct CollisionObject {
  char data[16] = {0};
  bool operator==(const CollisionObject &other) const { return true; }
};
struct SoundEventsPreload {
  char data[136] = {0};
  bool operator==(const SoundEventsPreload &other) const { return true; }
};
struct SoundVarId {
  char data[8] = {0};
  bool operator==(const SoundVarId &other) const { return true; }
};
struct SoundStream {
  char data[8] = {0};
  bool operator==(const SoundStream &other) const { return true; }
};
struct SoundEvent {
  char data[8] = {0};
  bool operator==(const SoundEvent &other) const { return true; }
};
struct PhysVars {
  char data[112] = {0};
  bool operator==(const PhysVars &other) const { return true; }
};
struct CollisionResource {
  char data[8] = {0}; // ptr
  bool operator==(const CollisionResource &other) const { return true; }
};

// struct FastPhysTag {
//   char data[0]  = {0};
// };
struct AnimatedPhys {
  char data[48] = {0};
  bool operator==(const AnimatedPhys &other) const { return true; }
};
struct EffectorData {
  char data[72] = {0};
  bool operator==(const EffectorData &other) const { return true; }
};
struct HumanAnimcharSound {
  char data[8] = {0};
  bool operator==(const HumanAnimcharSound &other) const { return true; }
};
struct AnimcharSound {
  char data[8] = {0};
  bool operator==(const AnimcharSound &other) const { return true; }
};
struct AnimcharResourceReferenceHolder {
  char data[8] = {0};
  bool operator==(const AnimcharResourceReferenceHolder &other) const { return true; }
};
struct AnimcharNodesMat44 {
  char data[48] = {0};
  bool operator==(const AnimcharNodesMat44 &other) const { return true; }
};
namespace AnimV20 {
  struct AnimcharRendComponent {
    char data[96] = {0};
    bool operator==(const AnimcharRendComponent &other) const { return true; }
  };
} // namespace AnimV20
namespace AnimV20 {
  struct AnimcharBaseComponent {
    char data[416] = {0};
    bool operator==(const AnimcharBaseComponent &other) const { return true; }
  };
} // namespace AnimV20


struct GpuObjectRiResourcePreload {
  char data[1] = {0};
  bool operator==(const GpuObjectRiResourcePreload &other) const { return true; }
};
struct GlobalVisibleCoversMapMAX {
  char data[32] = {0};
  bool operator==(const GlobalVisibleCoversMapMAX &other) const { return true; }
};
struct GlobalVisibleCoversMap4 {
  char data[32] = {0};
  bool operator==(const GlobalVisibleCoversMap4 &other) const { return true; }
};
struct HumanVisibleCoversMap {
  char data[24] = {0};
  bool operator==(const HumanVisibleCoversMap &other) const { return true; }
};
struct CoversComponent {
  char data[464] = {0};
  bool operator==(const CoversComponent &other) const { return true; }
};

namespace rendinst {
  struct RendInstDesc {
    uint32_t v1;
    uint32_t v2;
    uint32_t v3;
    char data[20] = {0};

    bool operator==(const RendInstDesc &rhs) const { return std::memcmp(this, &rhs, sizeof(RendInstDesc)) == 0; }
  };

  typedef uint64_t riex_handle_t;

  // 16.32u for ri_idx/instance
  typedef uint64_t riex_handle_t;
  inline constexpr riex_handle_t RIEX_HANDLE_NULL = ~riex_handle_t(0);
  inline constexpr uint32_t ri_instance_type_shift = 32, ri_instance_inst_mask = ~0u;

  constexpr inline uint32_t riex_max_type() { return (1u << (48 - ri_instance_type_shift)) - 1; }
  constexpr inline uint32_t riex_max_inst() { return ri_instance_inst_mask; }

  inline uint32_t handle_to_ri_type(riex_handle_t h) { return (uint32_t) (h >> ri_instance_type_shift); }
  inline uint32_t handle_to_ri_inst(riex_handle_t h) { return (uint32_t) (h & ri_instance_inst_mask); }
  inline riex_handle_t make_handle(uint32_t ri_type, uint32_t ri_inst) {
    return (uint64_t(ri_type) << ri_instance_type_shift) | ri_inst;
  }
} // namespace rendinst
struct RiExtraComponent {
  char data[8] = {0};
  bool operator==(const RiExtraComponent &other) const { return true; }
};

struct HumanAnimCtx {
  char data[340] = {0};
  bool operator==(const HumanAnimCtx &other) const { return true; }
};
struct PlaneActor {
  char data[16] = {0};
  bool operator==(const PlaneActor &other) const { return true; }
};
struct OffenderData {
  char data[176] = {0};
  bool operator==(const OffenderData &other) const { return true; }
};
struct ResizableDecals {
  char data[184] = {0};
  bool operator==(const ResizableDecals &other) const { return true; }
};
struct UniqueBufHolder {
  char data[24] = {0};
  bool operator==(const UniqueBufHolder &other) const { return true; }
};
struct EnviEmitterId {
  char data[4] = {0};
  bool operator==(const EnviEmitterId &other) const { return true; }
};
struct EffectRef {
  char data[8] = {0};
  bool operator==(const EffectRef &other) const { return true; }
};
struct CameraFxEntity {
  char data[4] = {0};
  bool operator==(const CameraFxEntity &other) const { return true; }
};
struct AreaFxEntity {
  char data[4] = {0};
  bool operator==(const AreaFxEntity &other) const { return true; }
};
struct SpotLightEntity {
  char data[4] = {0};
  bool operator==(const SpotLightEntity &other) const { return true; }
};
struct OmniLightEntity {
  char data[4] = {0};
  bool operator==(const OmniLightEntity &other) const { return true; }
};
struct TheEffect {
  char data[16] = {0};
  bool operator==(const TheEffect &other) const { return true; }
};
struct RiExtraGen {
  char data[1] = {0};
  bool operator==(const RiExtraGen &other) const { return true; }
};
struct MountedGun {
  char data[168] = {0};
  bool operator==(const MountedGun &other) const { return true; }
};

namespace ballistics {
  struct ProjectileBallistics {
    char data[80] = {0};
    bool operator==(const ProjectileBallistics &other) const { return true; }
  };
  struct ProjectileProps {
    char data[64] = {0};
    bool operator==(const ProjectileProps &other) const { return true; }
  };
} // namespace ballistics
namespace daweap {
  struct Gun {
    char data[536] = {0};
    bool operator==(const Gun &other) const { return true; }
  };
  struct LaunchDesc {
    char data[84] = {0};
    bool operator==(const LaunchDesc &other) const { return true; }
  };
  struct GunLaunchEvents {
    char data[24] = {0};
    bool operator==(const GunLaunchEvents &other) const { return true; }
  };
  struct GunDeviation {
    char data[124] = {0};
    bool operator==(const GunDeviation &other) const { return true; }
  };

  struct ShellResourceLoader {
    char data[8];
    bool operator==(const ShellResourceLoader &other) const { return true; }
  };

} // namespace daweap
struct GrenadeThrower {
  char data[16] = {0};
  bool operator==(const GrenadeThrower &other) const { return true; }
};
struct EntityActions {
  char data[32] = {0};
  bool operator==(const EntityActions &other) const { return true; }
};
struct SimpleObjectModelResList {
  char data[24] = {0};
  bool operator==(const SimpleObjectModelResList &other) const { return true; }
};
struct SimpleObjectModelRes {
  char data[8] = {0};
  bool operator==(const SimpleObjectModelRes &other) const { return true; }
};
struct Camera {
  char data[256] = {0};
  bool operator==(const Camera &other) const { return true; }
};
struct SmokeFx {
  char data[32] = {0};
  bool operator==(const SmokeFx &other) const { return true; }
};
namespace shells {
  struct ShellRef {
    char data[16] = {0};
    bool operator==(const ShellRef &other) const { return true; }
  };
} // namespace shells
struct Bullet {
  char data[776] = {0};
  bool operator==(const Bullet &other) const { return true; }
};
struct VehiclePhysActor {
  char data[16] = {0};
  bool operator==(const VehiclePhysActor &other) const { return true; }
};
struct UnitByEid {
  char data[16] = {0};
  bool operator==(const UnitByEid &other) const { return true; }
};


namespace dm {
  namespace splash {
    struct Properties {
      char data[28] = {0};
      bool operator==(const Properties &other) const { return true; }
    };
  } // namespace splash
  struct ExplosiveProperties {
    char data[24] = {0};
    bool operator==(const ExplosiveProperties &other) const { return true; }
  };
  struct FireDamageState {
    char data[24] = {0};
    bool operator==(const FireDamageState &other) const { return true; }
  };
  struct FireDamageComponent {
    char data[40] = {0};
    bool operator==(const FireDamageComponent &other) const { return true; }
  };
  struct DamagePartProps {
    char data[576] = {0};
    bool operator==(const DamagePartProps &other) const { return true; }
  };
  struct SplashWave {
    char data[8] = {0};
    bool operator==(const SplashWave &other) const { return true; }
  };
} // namespace dm
struct AmmoStowageMassToSplashList {
  char data[24] = {0};
  bool operator==(const AmmoStowageMassToSplashList &other) const { return true; }
};
struct AmmoStowageSlotCollAndGeomNodesList {
  char data[24] = {0};
  bool operator==(const AmmoStowageSlotCollAndGeomNodesList &other) const { return true; }
};

struct GameObjects {
  char data[40] = {0};
  bool operator==(const GameObjects &other) const { return true; }
};

namespace dm {
  struct PartId //: public MemcmpEquality<PartId>
  {
    uint8_t data = 0;

    bool operator==(const PartId &other) const { return std::memcmp(this, &other, sizeof(PartId)) == 0; }
    [[nodiscard]] std::string toString() const { return fmt::format("{}", data); }
  };

} // namespace dm

namespace props {
  struct PropsId //: public MemcmpEquality<PropsId>
  {
    uint32_t data = 0;

    bool operator==(const PropsId &other) const { return std::memcmp(this, &other, sizeof(PropsId)) == 0; }
  };
} // namespace props

namespace freeAreaCheck {
  struct CheckTracesMgr {
    char data[8]; // boxed
    bool operator==(const CheckTracesMgr &other) const { return true; }
  };
} // namespace freeAreaCheck

struct TargetSignatureDetectorContainer {
  char data[8];
  bool operator==(const TargetSignatureDetectorContainer &other) const { return true; }
};

struct GuidanceLockPtr {
  char data[8];
  bool operator==(const GuidanceLockPtr &other) const { return true; }
};

struct GuidancePtr {
  char data[8];
  bool operator==(const GuidancePtr &other) const { return true; }
};

struct FastPhysTag : public ecs::Tag {};

struct AnimIrqToEventComponent {
  char data[8];
  bool operator==(const AnimIrqToEventComponent &other) const { return true; }
};

// for now, we only need a dummy sharedComponent
namespace ecs {
  template<typename T>
  struct SharedComponent {
    char data[8];
    bool operator==(const SharedComponent &other) const { return true; }
  };
  struct TemplatesListToInstantiate {
    char data[8];
    bool operator==(const TemplatesListToInstantiate &other) const { return true; }
  };
} // namespace ecs

struct BehaviourTree {
  char data[8];
  bool operator==(const BehaviourTree &other) const { return true; }
};

struct SharedPrecomputedWeaponPositions {
  char data[8];
  bool operator==(const SharedPrecomputedWeaponPositions &other) const { return true; }
};

struct CapsuleApproximation {
  char data[8];
  bool operator==(const CapsuleApproximation &other) const { return true; }
};

struct PhysRagdoll {
  char data[8];
  bool operator==(const PhysRagdoll &other) const { return true; }
};

struct PhysBody {
  char data[8];
  bool operator==(const PhysBody &other) const { return true; }
};

struct LightingAnimchar {
  char data[8];
  bool operator==(const LightingAnimchar &other) const { return true; }
};


struct HeatSourceId {
  char data[8];
  bool operator==(const HeatSourceId &other) const { return true; }
};

struct Footstep {
  char data[8];
  bool operator==(const Footstep &other) const { return true; }
};

struct FootstepFx {
  char data[8];
  bool operator==(const FootstepFx &other) const { return true; }
};

struct MotionMatchingController {
  char data[8];
  bool operator==(const MotionMatchingController &other) const { return true; }
};

struct AnimationDataBase {
  char data[8];
  bool operator==(const AnimationDataBase &other) const { return true; }
};

struct CapsulesAOHolder {
  char data[8];
  bool operator==(const CapsulesAOHolder &other) const { return true; }
};

struct BufferedHudData {
  std::vector<uint8_t> data{};
  [[nodiscard]] std::string toString(int indent) const {
    std::ostringstream oss{};
    FormatHexToStream(oss, std::span((char *) data.data(), data.size()));
    return fmt::format("{}", oss.str());
  }
  bool operator==(const BufferedHudData &other) const { return data == other.data; }
};

struct InvalidType {
  char data[8];
  bool operator==(const InvalidType &other) const { return true; }
};

struct LaserDecalManager {
  char data[8];
  bool operator==(const LaserDecalManager &other) const { return true; }
};

struct UniqueBufWithShaderVar {
  char data[8];
  bool operator==(const UniqueBufWithShaderVar &other) const { return true; }
};

struct SoundOcclusionBlob {
  char data[8];
  bool operator==(const SoundOcclusionBlob &other) const { return true; }
};

namespace aimmem {
  struct AimingMemPoints {
    char data[8];
    bool operator==(const AimingMemPoints &other) const { return true; }
  };
} // namespace aimmem
namespace resource_slot {
  struct NodeHandleWithSlotsAccess {
    char data[8];
    bool operator==(const NodeHandleWithSlotsAccess &other) const { return true; }
  };
} // namespace resource_slot
#endif // MYEXTENSION_BASICTYPEDEFS_H
