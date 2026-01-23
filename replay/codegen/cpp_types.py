from DataTypes import DataTypeRegister


class Uid_reg(DataTypeRegister):
    name = "danet::Uid"
    is_pod = True


class RoundScore_reg(DataTypeRegister):
    name = "danet::RoundScore"
    members = [
        "uint32_t combined;",
        "std::vector<uint32_t> scores;"
    ]

class dummyForFootballStat_reg(DataTypeRegister):
    name = "danet::dummyForFootballStat"
    members = [
        "uint16_t v1;",
        "uint16_t v2;",
        "uint16_t v3;"
    ]

class Crew_reg(DataTypeRegister):
    name = "danet::Crew"
    members = [
        "ecs::entity_id_t e1;",
        "ecs::entity_id_t e2;",
        "uint8_t v1;",
        "uint8_t v2;",
    ]

class CrewUnitsList_reg(DataTypeRegister):
    name = "danet::CrewUnitsList"
    members = [
        "std::vector<danet::Crew> crew;"
    ]

class dummyForPlayerStat_reg(DataTypeRegister):
    name = "danet::dummyForPlayerStat"
    members = [
        "uint16_t v1;",
        "uint16_t v2;",
        "uint16_t v3;",
        "uint16_t v4;",
        "uint16_t v5;",
        "uint16_t v6;",
        "uint16_t v7;",
        "uint16_t v8;",
        "uint16_t v9;",
        "uint16_t v10;",
        "uint16_t v11;",
        "uint16_t v12;",
        "uint32_t v13;",
        "uint32_t v14;",
        "uint16_t v15;",
        "uint16_t v16;",
        "uint16_t v17;",
        "uint16_t v18;",
        "uint32_t v19;",
        "uint16_t v20;",
        "uint16_t v21;",
        "uint16_t v22;",
        "uint16_t v23;",
        "uint16_t v24;"
    ]

class streak_reg(DataTypeRegister):
    name = "danet::streak"
    members = [
        "uint8_t v1;",
        "bool v2;",
        "bool v3;",
    ]

class dummyForKillStreaksProgress_reg(DataTypeRegister):
    name = "danet::dummyForKillStreaksProgress"
    members = [
        "std::vector<danet::streak> vals;"
    ]