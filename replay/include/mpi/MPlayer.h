

#ifndef MYEXTENSION_MPLAYER_H
#define MYEXTENSION_MPLAYER_H

#include "mpi/serializers.h"
#include "ecs/EntityManager.h"
#include "ecs/ComponentTypesDefs.h"
#include "ecs/ComponentTypes.h"
#include "ecs/ComponentPrintingImplementations.h"



class MPlayer : public danet::ReflectableObject {
public:

  mpi::Message *dispatchMpiMessage(mpi::MessageID /*mid*/) override;

  void hello();
  MPlayer() {
    varList.head = &uid;
    varList.tail = &rageTokens;
  }

  danet::ReflectionVar<danet::Uid> uid{"uid", &invitedNickName, 2};
  danet::ReflectionVar<std::string> invitedNickName{"invitedNickName", &nickLocKey, 3};
  danet::ReflectionVar<std::string> nickLocKey{"nickLocKey", &ClanTag, 4, danet::TranslatedCoder};
  danet::ReflectionVar<std::string> ClanTag{"ClanTag", &Title, 5};
  danet::ReflectionVar<std::string> Title{"Title", &publicFlags, 6};
  danet::ReflectionVar<uint32_t> publicFlags{"publicFlags", &decals, 7};
  danet::ReflectionVar<DataBlock> decals{"decals", &team, 8};
  danet::ReflectionVar<uint8_t> team{"team", &countryId, 9};
  danet::ReflectionVar<uint8_t> countryId{"countryId", &memberId, 10};
  danet::ReflectionVar<uint16_t> memberId{"memberId", &customState, 11};
  danet::ReflectionVar<DataBlock> customState{"customState", &score, 12};
  danet::ReflectionVar<uint16_t> score{"score", &dummyForSupportPlanes, 13};
  danet::ReflectionVar<std::array<ecs::EntityId, 20>> dummyForSupportPlanes{"dummyForSupportPlanes", &dummyForCrewUnitsList, 14};
  danet::ReflectionVar<danet::CrewUnitsList> dummyForCrewUnitsList{"dummyForCrewUnitsList", &disabledByMatchingSlots, 15};
  danet::ReflectionVar<uint32_t> disabledByMatchingSlots{"disabledByMatchingSlots", &brokenSlots, 16};
  danet::ReflectionVar<uint32_t> brokenSlots{"brokenSlots", &wasReadySlots, 17};
  danet::ReflectionVar<uint32_t> wasReadySlots{"wasReadySlots", &spareAircraftInSlots, 18};
  danet::ReflectionVar<uint32_t> spareAircraftInSlots{"spareAircraftInSlots", &ownedSlots, 19};
  danet::ReflectionVar<uint32_t> ownedSlots{"ownedSlots", &classinessMark, 20};
  danet::ReflectionVar<uint8_t> classinessMark{"classinessMark", &timeToRespawn, 21};
  danet::ReflectionVar<uint32_t> timeToRespawn{"timeToRespawn", &timeToRespawnInCoop, 22};
  danet::ReflectionVar<uint32_t> timeToRespawnInCoop{"timeToRespawnInCoop", &forcedRespawn, 23};
  danet::ReflectionVar<bool> forcedRespawn{"forcedRespawn", &timeToKick, 24};
  danet::ReflectionVar<uint32_t> timeToKick{"timeToKick", &guiState, 25};
  danet::ReflectionVar<uint8_t> guiState{"guiState", &spectatedModelIndex, 26};
  danet::ReflectionVar<ecs::EntityId> spectatedModelIndex{"spectatedModel", &dummyForCountUsedSlots, 27};
  danet::ReflectionVar<std::vector<uint8_t>> dummyForCountUsedSlots{"dummyForCountUsedSlots", &dummyForSpawnCosts, 28};
  danet::ReflectionVar<std::vector<uint32_t>> dummyForSpawnCosts{"dummyForSpawnCosts", &dummyForSpawnDelayTimes, 29};
  danet::ReflectionVar<std::vector<uint16_t>> dummyForSpawnDelayTimes{"dummyForSpawnDelayTimes", &dummyForKillStreaksProgress, 30};
  danet::ReflectionVar<danet::dummyForKillStreaksProgress> dummyForKillStreaksProgress{"dummyForKillStreaksProgress", &state, 31};
  danet::ReflectionVar<uint16_t> state{"state", &squadScore, 32};
  danet::ReflectionVar<uint32_t> squadScore{"squadScore", &ownedUnitRef, 33};
  danet::ReflectionVar<ecs::EntityId> ownedUnitRef{"ownedUnitRef", &controlledUnitRef, 34};
  danet::ReflectionVar<ecs::EntityId> controlledUnitRef{"controlledUnitRef", &supportUnitRef, 35};
  danet::ReflectionVar<ecs::EntityId> supportUnitRef{"supportUnitRef", &wreckedPartShipUnitRef, 36};
  danet::ReflectionVar<ecs::EntityId> wreckedPartShipUnitRef{"wreckedPartShipUnitRef", &dummyForRoundScore, 37};
  danet::ReflectionVar<danet::RoundScore> dummyForRoundScore{"dummyForRoundScore", &dummyForPlayerStat, 38};
  danet::ReflectionVar<danet::dummyForPlayerStat> dummyForPlayerStat{"dummyForPlayerStat", &dummyForFootballStat, 39};
  danet::ReflectionVar<danet::dummyForFootballStat> dummyForFootballStat{"dummyForFootballStat", &realNick, 40};
  danet::ReflectionVar<std::string> realNick{"realNick", &squadronId, 41};
  danet::ReflectionVar<uint32_t> squadronId{"squadronId", &forceLockTarget, 42};
  danet::ReflectionVar<uint16_t> forceLockTarget{"forceLockTarget", &cachedIsAutoSquad, 43};
  danet::ReflectionVar<bool> cachedIsAutoSquad{"cachedIsAutoSquad", &nickFrame, 44};
  danet::ReflectionVar<std::string> nickFrame{"nickFrame", &missionSupportUnitRef, 45};
  danet::ReflectionVar<ecs::EntityId> missionSupportUnitRef{"missionSupportUnitRef", &missionSupportUnitEnabled, 46};
  danet::ReflectionVar<bool> missionSupportUnitEnabled{"missionSupportUnitEnabled", &rageTokens, 47};
  danet::ReflectionVar<uint16_t> rageTokens{"rageTokens", nullptr, 48};
};


ECS_DECLARE_CREATABLE_TYPE(MPlayer);

#endif //MYEXTENSION_MPLAYER_H
