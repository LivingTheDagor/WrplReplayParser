
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <mpi/mpi.h>
#include "math/dag_Point3.h"
#include "state/ParserState.h"
#include "fstream"

struct ParserState;


namespace packet_ids {
  constexpr std::optional<std::string_view> get_name(uint16_t id) {
    switch (id) {
      case 0xB00C: return "InfTroopSync";
      case 0xB00E: return "ReplicatedObjectCreate";
      case 0xB013: return "DoFlakExplosion";
      case 0xB01D: return "SemChangeAircraftRequest";
      case 0xB01E: return "PpChangeAircraftResponse";
      case 0xB02C: return "SetTimeSpeedEx";
      case 0xB062: return "SpectateRequestId";
      case 0xB063: return "ControlsConfirmation";
      case 0xB065: return "FmwControlsPacket";
      case 0xB066: return "FmwAuthorityApprovedState";
      case 0xB068: return "MissionEvent";
      case 0xB06E: return "UnitBulletHitPartCount";
      case 0xB07B: return "MissionProgressDataReceived";
      case 0xB07D: return "MissionProgressTimeDataReceived";
      case 0xB08B: return "VehicleControlsPacket";
      case 0xB08C: return "VehicleAuthorityApprovedState";
      case 0xB091: return "AppUnitVersionMismatch";
      case 0xB092: return "AppUnitVersionMatch";
      case 0xB095: return "GmDesyncStats";
      case 0xB097: return "GmDoEscape";
      case 0xB0A8: return "UnitDesyncData";
      case 0xB0C5: return "UnitRequestWinch";
      case 0xB0CE: return "WalkerControlsPacket";
      case 0xB0D1: return "ResponseChallenge";
      case 0xB0D3: return "ShowUpObjToTeamResponse";
      case 0xB0D4: return "ShowUpObjToTeamRequest";
      case 0xB0DC: return "RocketControlsPacket";
      case 0xB0E5: return "ConfirmAuthorityApprovedState";
      case 0xB0F5: return "UnitHighlight";
      case 0xB107: return "EACClientToServerMsg";
      case 0xB11D: return "CloudsTracesData";
      case 0xB12E: return "TerraformIntegrityCheckDataRequest";
      case 0xB145: return "HumanControlsPacket";
      case 0xD01F: return "PlayFlakExplosionVisual";
      case 0xD020: return "GmSetGunAngles";
      case 0xD039: return "DeferredReplicatedObjectCreate";
      case 0xD043: return "ReplayCameraParams";
      case 0xD046: return "UnitReloadGun";
      case 0xD047: return "FmwRpm";
      case 0xD04A: return "ReplayCockpitParams";
      case 0xD08E: return "PlayExplosionVisual";
      case 0xD0AD: return "UnitDelayedStatusChange";
      case 0xD0FE: return "ProjectileHitReplay";
      case 0xD136: return "DeferredReflectionData";
      case 0xD137: return "UnitDataSnapshot";
      case 0xD146: return "ReplayVrHandsState";
      case 0xF016: return "GmGroundExplosionSmokeEffect";
      case 0xF017: return "GmGroundExplosionFireEffect";
      case 0xF018: return "GmDoInactive";
      case 0xF01A: return "GmDoSingleShotReliable";
      case 0xF01C: return "GmDoStopFire";
      case 0xF026: return "TurretYawImmobile";
      case 0xF027: return "TurretPitchImmobile";
      case 0xF028: return "RunTriggerAction";
      case 0xF02D: return "ReflectionData";
      case 0xF02F: return "UnitRequestRespawn";
      case 0xF032: return "UnitRequestRepair";
      case 0xF033: return "UnitRepair";
      case 0xF037: return "UnitDetected";
      case 0xF038: return "OwnerPlayerViewDirection";
      case 0xF03B: return "HintPlayFromScript";
      case 0xF04B: return "OnUnitDead";
      case 0xF04D: return "UnitDoExplosion";
      case 0xF050: return "UnitBailoutResponse";
      case 0xF053: return "UnitRequestRearm";
      case 0xF054: return "UnitRearm";
      case 0xF055: return "MissionFailOrSuccess";
      case 0xF056: return "TextCriticalHitReport";
      case 0xF058: return "TextKillReport";
      case 0xF05D: return "FadeToDebriefing";
      case 0xF06A: return "DialogMessageAction";
      case 0xF06B: return "ResetGroundModelPositionDeltas";
      case 0xF071: return "UnitReloadGuns";
      case 0xF073: return "GroundModelPositions";
      case 0xF074: return "GroundModelPositionsServerReplay";
      case 0xF078: return "TextStreak";
      case 0xF07A: return "MissionProgressData";
      case 0xF07C: return "MissionProgressTimeData";
      case 0xF08D: return "FmwAuthorityApprovedPartialState";
      case 0xF091: return "GmCutPartResponse";
      case 0xF099: return "PlayLandcrashExplosions";
      case 0xF09A: return "DvmUnreliableHpReflectionData";
      case 0xF09B: return "DvmUnreliableHpReflectionReply";
      case 0xF09C: return "EconUpdateWeapon";
      case 0xF0A0: return "GmDoSingleShotUnreliable";
      case 0xF0A3: return "VehicleAuthorityApprovedPartialState";
      case 0xF0A6: return "UnitArtilleryShootingAtPos";
      case 0xF0AA: return "RedundancyReflectionData";
      case 0xF0AB: return "DvmCutDecor";
      case 0xF0B1: return "UnitSingleShot";
      case 0xF0B3: return "DebugSphere";
      case 0xF0B6: return "SmartCutsceneFinished";
      case 0xF0B8: return "PlayAreaEffects";
      case 0xF0BB: return "UnitRequestDamageModelEvents";
      case 0xF0BC: return "UnitResponseDamageModelEvents";
      case 0xF0BD: return "UnitBulletRearm";
      case 0xF0C2: return "UnitLastEffectiveHit";
      case 0xF0C3: return "MessageStartGame";
      case 0xF0C8: return "UseOrderRequest";
      case 0xF0CA: return "UnitRequestRepairAssist";
      case 0xF0CB: return "DvmDamageDataForReferee";
      case 0xF0D2: return "OrderResult";
      case 0xF0D5: return "UnitToggleGunners";
      case 0xF0D6: return "GmDoStartFireWithDist";
      case 0xF0D8: return "HudMarkMapSquare";
      case 0xF0DB: return "ShellsData";
      case 0xF0DD: return "GmRequestToggleCurWeapon";
      case 0xF0DF: return "CustomWeatherParams";
      case 0xF0E1: return "GmRequestChangeCrew";
      case 0xF0E2: return "GmRequestRepair";
      case 0xF0E3: return "RocketAuthorityApprovedState";
      case 0xF0E6: return "GmApplyExplosionImpulse";
      case 0xF0E9: return "UnitHitEffects";
      case 0xF0EC: return "MissionPlayHint";
      case 0xF0ED: return "UnitPickupItem";
      case 0xF0F0: return "UnitSetTimeFuseDistance";
      case 0xF0F6: return "UnitScoutResult";
      case 0xF0F7: return "InteractiveObjectAuthorityApprovedState";
      case 0xF0F8: return "InteractiveObjectAuthorityApprovedPartialState";
      case 0xF0F9: return "UnitRefuel";
      case 0xF0FD: return "UnitRequestChangeShotFreq";
      case 0xF101: return "UnitRequestRepairWithoutMod";
      case 0xF103: return "ConsoleUnitCommand";
      case 0xF104: return "TextHitReport";
      case 0xF109: return "GmEngineOnOff";
      case 0xF10A: return "UnitChangeNightVision";
      case 0xF10C: return "RecreateTorpedoes";
      case 0xF10F: return "SquadTargetDesignationRequest";
      case 0xF118: return "UnitDamagePartKill";
      case 0xF119: return "GmToggleOptics";
      case 0xF11A: return "ShellsDataServerReplay";
      case 0xF11E: return "UnitRequestSwitchOnSupport";
      case 0xF11F: return "GmRequestToggleStealth";
      case 0xF120: return "UnitFriendlyFire";
      case 0xF121: return "ActiveProtectionSystemTriggering";
      case 0xF122: return "TerraformPatchAlt";
      case 0xF123: return "TerraformData";
      case 0xF124: return "GmToggleTerraform";
      case 0xF125: return "GmDoWeaponLockOnReliable";
      case 0xF126: return "GmDoWeaponLockOnUnreliable";
      case 0xF127: return "UnitRequestExtinguishAssist";
      case 0xF128: return "UnitRequestExtinguishWithoutMod";
      case 0xF129: return "UnitResponseSwitchOnSupport";
      case 0xF12C: return "ECSNetUnitsData";
      case 0xF12D: return "ECSNetUnitsDataServerReplay";
      case 0xF12F: return "TerraformIntegrityCheckDataResponce";
      case 0xF130: return "UnitSmokeScreen";
      case 0xF131: return "UnitChangeCurShortcut";
      case 0xF132: return "ShotFailedApsWorking";
      case 0xF133: return "UnitOnExplosion";
      case 0xF134: return "UnitRequestChangeSupportPlane";
      case 0xF135: return "UnitSupportPlaneAttackCommand";
      case 0xF13B: return "DvmDamageDataForReplay";
      case 0xF13D: return "GmCutAllWreckedParts";
      case 0xF140: return "UnitRequestUnlimitedControl";
      case 0xF142: return "TargetDesignationMark";
      case 0xF143: return "SetBenchmarkMode";
      case 0xF144: return "UnitOnHit";
      case 0xF147: return "UnitDoProximityExplosion";
      case 0xF148: return "ForceMusic";
      case 0xF08F: return "UnitAttachEffectRelative";
      case 0xf0cc: // added after
        return "UnitCameraAngles";
      default: return std::nullopt;
    }
  }
} // namespace packet_ids

namespace mpi {
#define MAX_CALL_DEPTH (16)
  static IMessageListener *message_listeners = nullptr;
  object_dispatcher obj_dispatcher = nullptr;
  static thread_local int curr_call_depth = 0;

  IObject *dispatch_object(ObjectID oid, ObjectExtUID ext, ParserState *state) {
    return static_cast<IObject *>((oid != INVALID_OBJECT_ID && obj_dispatcher) ? obj_dispatcher(oid, ext, state)
                                                                               : nullptr);
  }

  Message *dispatch(const BitStream &bs, ParserState *state, bool copy_payload) {
    ZoneScoped;
    ObjectID oid;
    ObjectExtUID extUid;
    MessageID mid;
    if (obj_dispatcher && read_object_ext_uid(bs, oid, extUid) && bs.Read(mid)) {
      // LOG("MPI Dispatch: oid: {:#x}; extUid: {:#x}; mid: {:#x};", oid, extUid, mid);
      // mpi_data[oid][mid] += 1; used for packet
      IObject *o = dispatch_object(oid, extUid, state);
      if (o) {
        Message *m = (mid != INVALID_MESSAGE_ID) ? o->dispatchMpiMessage(mid) : nullptr;
        if (m) {
          m->payload.~BitStream();
          G_ASSERT(!(bs.GetReadOffset() & 7)); // bit offset
          uint32_t bytesReaded = BITS_TO_BYTES(bs.GetReadOffset());
          new (&m->payload)
            BitStream(bs.GetData() + bytesReaded, bs.GetNumberOfBytesUsed() - bytesReaded, copy_payload);
          if (m->readPayload(state))
            return m;
          else if (m->delete_message)
            state->_delete(m);
        }
      } else {
        // LOG("object doesen't exist");
      }
    }
    return nullptr;
  }

  void sendto(Message *m, SystemID receiver) {
    ZoneScoped;
    if (++curr_call_depth >= MAX_CALL_DEPTH) {
      G_ASSERTF(0, "{} call depth is {}! Infinite recursion?!", __FUNCTION__, (int) curr_call_depth);
      // LOG("{} call depth is {}! Infinite recursion?!", __FUNCTION__, (int) curr_call_depth);
      --curr_call_depth;
      return;
    }

    if (!m)
      ;
    else if (!m->obj)
      LOG("{} ignore attempt to send message {} ({:#x}) without recepient object\n", __FUNCTION__, fmt::ptr(m), m->id);
    else
      for (IMessageListener *l = message_listeners; l; l = l->next) // in reality, only one IMessageListener
        if (m->isApplicable(l))
          l->receiveMpiMessage(m, receiver);
    --curr_call_depth;
  }

  void register_listener(IMessageListener *l) {
    l->next = nullptr;
    IMessageListener *prev = nullptr;
    for (IMessageListener *ll = message_listeners; ll; prev = ll, ll = ll->next)
      ;
    (prev ? prev->next : message_listeners) = l;
  }

  void unregister_listener(IMessageListener *l) {
    IMessageListener *prev = nullptr;
    for (IMessageListener *ll = message_listeners; ll; prev = ll, ll = ll->next)
      if (l == ll) {
        if (prev)
          prev->next = l->next;
        if (l == message_listeners)
          message_listeners = l->next;
        break;
      }
  }

  void register_object_dispatcher(object_dispatcher od) { obj_dispatcher = od; }
}; // namespace mpi