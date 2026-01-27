from .obj_base import ReflectableObject, ReplicatedObject
from .obj_base import ReflectionVarMeta as Var

class MPlayer(ReflectableObject):
    uid = Var("danet::Uid", 2)
    invitedNickName = Var("std::string", 3)
    nickLocKey = Var("std::string", 4, "TranslatedCoder")
    ClanTag = Var("std::string", 5)
    Title = Var("std::string", 6)
    publicFlags = Var("uint32_t", 7)
    decals = Var("DataBlock", 8)
    team = Var("uint8_t", 9)
    countryId = Var("uint8_t", 10)
    memberId = Var("uint16_t", 11)
    customState = Var("DataBlock", 12)
    score = Var("uint16_t", 13)
    dummyForSupportPlanes = Var("std::array<ecs::EntityId, 20>", 14) # TODO, define custom type?
    dummyForCrewUnitsList = Var("danet::CrewUnitsList", 15)
    disabledByMatchingSlots = Var("uint32_t", 16)
    brokenSlots = Var("uint32_t", 17)
    wasReadySlots = Var("uint32_t", 18)
    spareAircraftInSlots = Var("uint32_t", 19)
    ownedSlots = Var("uint32_t", 20)
    classinessMark = Var("uint8_t", 21)
    timeToRespawn = Var("float", 22)
    timeToRespawnInCoop = Var("float", 23)
    forcedRespawn = Var("bool", 24)
    timeToKick = Var("float", 25)
    guiState = Var("uint8_t", 26)
    spectatedModelIndex = Var("ecs::EntityId", 27)
    dummyForCountUsedSlots = Var("std::vector<uint8_t, uint8_t>", 28)
    dummyForSpawnCosts = Var("std::vector<uint32_t, uint8_t>", 29)
    dummyForSpawnDelayTimes = Var("std::vector<uint16_t, uint8_t>", 30)
    dummyForKillStreaksProgress = Var("danet::dummyForKillStreaksProgress", 31)
    state = Var("uint16_t", 32)
    squadScore = Var("uint32_t", 33)
    ownedUnitRef = Var("ecs::EntityId", 34)
    controlledUnitRef = Var("ecs::EntityId", 35)
    supportUnitRef = Var("ecs::EntityId", 36)
    wreckedPartShipUnitRef = Var("ecs::EntityId", 37)
    dummyForRoundScore = Var("danet::RoundScore", 38)
    dummyForPlayerStat = Var("danet::dummyForPlayerStat", 39)
    dummyForFootballStat = Var("danet::dummyForFootballStat", 40)
    realNick = Var("std::string", 41)
    squadronId = Var("uint32_t", 42)
    forceLockTarget = Var("uint16_t", 43)
    cachedIsAutoSquad = Var("bool", 44)
    nickFrame = Var("std::string", 45)
    missionSupportUnitRef = Var("ecs::EntityId", 46)
    missionSupportUnitEnabled = Var("bool", 47)
    rageTokens = Var("uint16_t", 48)

class TeamData(ReflectableObject):
    score = Var("uint16_t", 2)
    tickets = Var("uint16_t", 3)
    orderCooldownTotal = Var("uint32_t", 4)
    orderCooldownLeft = Var("uint32_t", 5)
    spawnScore = Var("uint32_t", 6)
    roundScore = Var("uint32_t", 7)

class GlobalElo(ReflectableObject):
    teamAvgEloRatings = Var("danet::teamAvgEloRatings", 2)

class GeneralState(ReflectableObject):
    lastSuperArtilleryTime = Var("float", 2)
    dummyForExitZonesSettings = Var("danet::dummyForExitZonesSettings", 0xe)
    battleAreaChangeTime = Var("float", 3)
    battleAreaChangeToId = Var("int", 4)
    forcedMapCellsAir = Var("uint8_t", 9)
    forcedMapCellsGround = Var("uint8_t", 10)
    useCustomSuperArtillery = Var("bool", 5)
    waterWindStrengthClamp = Var("Point2", 0xc, "WeirdFloatSerializer")
    weatherEffectsDummyVar = Var("danet::WeatherEffects", 0xd)
    timeLeft = Var("uint16_t", 6)
    dummyForBombingEvent = Var("bool", 7, "InvalidSerializer") # TODO dummy value
    dummyForUnlimitedControlEvent = Var("bool", 0xb, "InvalidSerializer") # TODO dummy value
    customState = Var("DataBlock", 8)

class BombingZone(ReplicatedObject):
    armyNo = Var("uint8_t", 2)
    flags = Var("uint16_t", 3)
    curZoneIntegrity = Var("float", 0x43)

class CaptureZone(ReplicatedObject):
    armyNo = Var("uint8_t", 2)
    flags = Var("uint16_t", 3)
    mpTimeX100 = Var("uint8_t", 0x43)
    conqTeam = Var("uint8_t", 0x44)
    iconIdx = Var("uint8_t", 0x45)
    dummyVarForCapturers = Var("std::vector<danet::UnitId, uint8_t>", 0x46)
    dummyVarForCapturePart = Var("std::vector<danet::UnitIdStruct, uint8_t>", 0x47)
    dummyVarForNumOfActiveCapturers = Var("std::vector<uint8_t, uint8_t>", 0x48)
class RearmZone(ReplicatedObject):
    armyNo = Var("uint8_t", 2)
    flags = Var("uint16_t", 3)

class ExitZone(ReplicatedObject):
    armyNo = Var("uint8_t", 2)
    flags = Var("uint16_t", 3)

class PickupZone(ReplicatedObject):
    armyNo = Var("uint8_t", 2)
    flags = Var("uint16_t", 3)
    showOnTacticalMap = Var("bool", 0x43)