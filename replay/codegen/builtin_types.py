from DataTypes import DataTypeRegister
import custom_rw as rw
from replay.codegen.DataTypes import default_bs_loader, default_bs_writer
from replay.codegen.custom_rw import *


class bool_reg(DataTypeRegister):
    name = "bool"
    is_pod = True


class uint8_t_reg(DataTypeRegister):
    name = "uint8_t"
    is_pod = True


class uint16_t_reg(DataTypeRegister):
    name = "uint16_t"
    is_pod = True


class uint32_t_reg(DataTypeRegister):
    name = "uint32_t"
    is_pod = True


class uint64_t_reg(DataTypeRegister):
    name = "uint64_t"
    is_pod = True


class std_string_reg(DataTypeRegister):
    name = "std::string"
    # ok while std::string is most CERTAINLY not a pod type,
    # in this context the read overload makes it appear as so to my system
    is_pod = True
    # the bs->Read(std::string) overload does all we need already


class dblk_reg(DataTypeRegister):
    name = "DataBlock"
    custom_loader = datablock_loader
    custom_writer = datablock_writer

class EntityId_reg(DataTypeRegister):
    name = "ecs::EntityId"
    custom_loader = EntityId_loader
    custom_writer = EntityId_writer

class entity_id_t_reg(DataTypeRegister):
    name = "ecs::entity_id_t"
    custom_loader = entity_id_t_loader
    custom_writer = entity_id_t_writer

class vector_reg(DataTypeRegister):
    name = "std::vector"
    template_type_args = [DataTypeType]


class array_reg(DataTypeRegister):
    name = "std::array"
    template_type_args = [DataTypeType, int]

