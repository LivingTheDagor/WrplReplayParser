from .obj_base import ReflectableObject, ReplicatedObject
from .obj_base import ReflectionVarMeta as Var
from .obj_base import SimpleVar as SVar

class MPlayer(ReflectableObject):
    public = [
        SVar("std::unordered_set<unit::Unit*>", "currentOwnedUnits"),
        SVar("std::vector<unit::Unit*>", "allOwnedUnits")
    ]
    uid = Var("danet::Uid", 2)
    invitedNickName = Var("std::string", 3)
    nickLocKey = Var("std::string", 4, "TranslatedCoder")
    ClanTag = Var("std::string", 5)
    Title = Var("std::string", 6)
    publicFlags = Var("uint32_t", 7)
    decals = Var("DataBlock", 8)
    team = Var("uint8_t", 9)
    countryId = Var("danet::Country", 10)
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
    numFreeSpareUsed = Var("uint32_t", 50)

class TeamData(ReflectableObject):
    score = Var("uint16_t", 2)
    tickets = Var("uint16_t", 3)
    orderCooldownTotal = Var("uint32_t", 4)
    orderCooldownLeft = Var("uint32_t", 5)
    spawnScore = Var("uint32_t", 6)
    roundScore = Var("float", 7)

class GlobalElo(ReflectableObject):
    teamAvgEloRatings = Var("Point3", 2)

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
    dummyForMapTimers = Var("Point2", 15)
    totalDomTeam = Var("uint8_t", 16)
    totalDomTime = Var("uint16_t", 17)
    totalDomMult = Var("uint8_t", 18)



class MissionArea(ReplicatedObject):
    public = [
        SVar("TMatrix", "tm"),
    ]
    areaFlags = Var("danet::AreaFlagsEnum", 0x2)


class MissionZone(ReplicatedObject):
    public = [
        SVar("MissionArea*", "area"),
    ]
    armyNo = Var("uint8_t", 2)
    flags = Var("uint16_t", 3)

class BombingZone(MissionZone):
    curZoneIntegrity = Var("float", 0x43)

class CaptureZone(MissionZone):
    mpTimeX100 = Var("int8_t", 0x43)
    conqTeam = Var("uint8_t", 0x44)
    iconIdx = Var("uint8_t", 0x45)
    dummyVarForCapturers = Var("std::vector<danet::UnitId, uint8_t>", 0x46)
    dummyVarForCapturePart = Var("std::vector<danet::UnitIdStruct, uint8_t>", 0x47)
    dummyVarForNumOfActiveCapturers = Var("std::vector<uint8_t, uint8_t>", 0x48)
class RearmZone(MissionZone):
    pass

class ExitZone(MissionZone):
    pass

class PickupZone(MissionZone):
    showOnTacticalMap = Var("bool", 0x43)


class BaseExtReflectable(ReflectableObject):
    isAlternativeShotFreq = Var("bool", 33)
    # name:brokenTurretDriveJammedTime;addr:0x281a670;id:54;flags:0;size:64
    brokenTurretDriveSpeed = Var("float", 55)
    brokenTurretDriveMult = Var("float", 56)
    isBreechDamaged = Var("bool", 57)
    hasModuleEffectsToRepair = Var("bool", 58)
    moduleEffectsRepairAtTime = Var("float", 59)
    curNightVisionMode = Var("int", 40)
    # name:dummyVarForCrewLayout;addr:0x1016c60;id:2;flags:32;size:32
    dummyVarForMissionAddText = Var("std::array<std::string, 2>", 5)
    supportPlanesCount = Var("uint32_t", 49)
    supportPlaneCatapultsFuseMask = Var("uint8_t", 50)
    lastBuildingTime = Var("float", 62)
    visualReloadProgress = Var("uint8_t", 6)
    briefMalfunctionState = Var("uint16_t", 7)
    bombDelayExplosion = Var("float", 8)
    delayWithFlightTime = Var("bool", 9)
    rocketFuseDist = Var("float", 10)
    torpedoDiveDepth = Var("float", 44)
    dummyForDeathInfo = Var("danet::dummyForDeathInfo", 11)
    # name:lastContactFrom;addr:0x281a670;id:12;flags:0;size:96
    # name:lastContactTo;addr:0x281a670;id:13;flags:0;size:96
    lastContactTime = Var("float", 31)
    lastContactTimeRel = Var("float", 14)
    allowBailout = Var("bool", 43)
    repairAssistee = Var("uint16_t", 15)  # actually a uid
    repairAssistant = Var("uint16_t", 16)
    extinguishAssistee = Var("uint16_t", 45)
    extinguishAssistant = Var("uint16_t", 46)
    invulnerable = Var("bool", 17)
    enableGunners = Var("uint8_t", 18)
    enableCiwsGunners = Var("uint8_t", 64)
    vulnerableByUnitUId = Var("uint32_t", 19)
    dummyForUnitFlags = Var("uint32_t", 20)
    lowRateUnitFlags = Var("uint32_t", 21)
    unitState = Var("uint16_t", 22)
    farthestExitZoneId = Var("uint32_t", 60)
    unitArmyNo = Var("uint8_t", 23)
    stealthRadiusSq = Var("float", 24)
    stealthArmyMask = Var("uint8_t", 25)
    fpvRocketCameraControlEnabled = Var("bool", 39)
    scoutStartTime = Var("float", 26)
    scoutCooldown = Var("float", 27)
    prepareRepairAssisteeTime = Var("float", 28)
    prepareRepairAssistantTime = Var("float", 35)
    prepareExtinguishAssistantTime = Var("float", 47)
    nextUseArtilleryTime = Var("float", 29)
    prepareRepairCooldownsTime = Var("float", 37)
    killer = Var("danet::KillerStruct", 30)
    isNeedRepairHelp = Var("bool", 34)
    isNeedExtinguishHelp = Var("bool", 48)
    repairCooldowns = Var("bool", 38)
    maskingFactor = Var("float", 32)
    fuseModeIndex = Var("uint32_t", 61)
    timeToNextSmokeScreen = Var("float", 51)
    smokeScreenCount = Var("uint8_t", 52)
    smokeScreenActived = Var("bool", 53)
    crewStatMult = Var("float", 63)


class FMWReflectable(BaseExtReflectable):  # id 0
    pass


class GMReflectable(BaseExtReflectable):  # id 1
    pass


# class INFReflectable(ReflectableObject): # id 2
#     pass

# class FM_FXReflectable(ReflectableObject): # id 12
#     pass

# class UFXReflectable(ReflectableObject): # id 13
#     pass


class DVMReflectable(ReflectableObject):
    # name:dummyVarForEnergyShieldHits;addr:0xd587c0;id:5;flags:34;size:32
    dummyVarForDamagedStateReflection = Var("std::vector<danet::DamagedState, uint8_t>", 3,
                                            "dummyVarForDamagedStateReflectionCoder")  # name:dummyVarForDamagedStateReflection;addr:0xd49190;id:3;flags:16384;size:32'
    # name:dummyVarForDecorMask;addr:0xd501c0;id:4;flags:32;size:32


class FM_DVMReflectable(DVMReflectable):  # id 16
    pass


class GM_DVMReflectable(DVMReflectable):  # id 17
    pass


# class CUDReflectable(ReflectableObject): # id 18
#     pass

class UnitWeaponsMask(ReflectableObject):  # id 25
    dummyVarForAmmoPartsMask = Var("danet::WeaponsMask", 0x2, "WeaponsCoder")
