from .DataTypes import DataTypeRegister
from .custom_rw import *

class bool_reg(DataTypeRegister):
    name = "bool"
    is_pod = True

class char_reg(DataTypeRegister):
    name = "char"
    is_pod = True

class uint8_t_reg(DataTypeRegister):
    name = "uint8_t"
    is_pod = True

class int8_t_reg(DataTypeRegister):
    name = "int8_t"
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

class int_reg(DataTypeRegister):
    name = "int"
    is_pod = True

class float_reg(DataTypeRegister):
    name = "float"
    is_pod = True

class std_string_reg(DataTypeRegister):
    name = "std::string"
    # ok while std::string is most CERTAINLY not a pod type,
    # in this context the read overload makes it appear as so to my system
    is_pod = True
    # the bs->Read(std::string) overload does all we need already


class dblk_reg(DataTypeRegister):
    name = "DataBlock"
    is_pod = True

class EntityId_reg(DataTypeRegister):
    name = "ecs::EntityId"
    custom_loader = EntityId_loader
    custom_writer = EntityId_writer


class vector_reg(DataTypeRegister):
    name = "std::vector"
    # second DataTypeType is the type of the value holding our size, most often u8, but can be u32
    template_type_args = [DataTypeType, DataTypeType]
    custom_loader = vector_loader
    custom_writer = vector_writer

    @staticmethod
    def serialize_name(datatype: 'DataTypeCompiled'):
        base = datatype.datatype.reg.name
        return f"{base}<{str(datatype.template_args[0])}>" # the second arg is only for code generation



class zigZagInt_reg(DataTypeRegister):
    name = "danet::zigZagInt"
    custom_loader = zigZagInt_loader
    custom_writer = zigZagInt_writer
    @staticmethod
    def serialize_name(datatype: 'DataTypeCompiled'):
        return "int"

    @staticmethod
    def get_base_name(datatype: 'DataTypeCompiled'):
        return "int"


class zigZagVector_reg(DataTypeRegister):
    name = "danet::zigZagVector"
    template_type_args = [DataTypeType]
    custom_loader = zigZagVector_loader
    custom_writer = zigZagVector_writer
    @staticmethod
    def serialize_name(datatype: 'DataTypeCompiled'):
        base = "std::vector"
        return f"{base}<{str(datatype.template_args[0])}>"

    @staticmethod
    def get_base_name(datatype: 'DataTypeCompiled'):
        return "std::vector"

class array_reg(DataTypeRegister):
    name = "std::array"
    template_type_args = [DataTypeType, int]
    custom_loader = array_loader
    custom_writer = array_writer
