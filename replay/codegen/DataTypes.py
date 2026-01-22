import sys
from abc import ABC, abstractmethod
import types
import inspect

# a special type using in template args to represent that this template input is a type
# think as in std::vector<T>, where T is an arbitrary type
# not used with the second template arg of std::Array<T, N>, where you would probably use an [int] type instead
# used for arg validation and loading
class DataTypeType:
    pass

class DataTypeRegister:
    name: str = ""  # the name of this datatype plus namespace, must be a set.
    members: list[str] = []
    # the members of this datatype if it a struct, if a datatype has no members, it is also assumed to be externally defined

    is_pod = False  # if a datatype is POD, then it will be optimized to be directly read into and out of, even if it has members
    # if one of the members has a custom loader/writer, error state

    # custom_load is the custom read implementation that this datatype needs to use with a bitstream
    custom_loader = None

    # custom_writer is the custom write implementation that this datatype needs to use to write to a bitstream
    custom_writer = None

    # data datatype cannot define a custom_rw and be is_pod at the same time
    # the datatypes expected to be passed in a struct instantiation
    # most of the time, this will only be type 'DataTypeType' which represents a type is passed instead of some value
    # read DataTypeType comments for more info
    # leaving this to default params would be the same as no template args
    template_type_args = []


def parse_raw_datatype_name(name: str) -> tuple[list[str], str]:
    """
    Parses raw datatype name without any template handling
    :param name:
    :return:
    """
    *namespace, name_ = name.split("::")
    return namespace, name_


class NameSpace:
    def __init__(self, name: str):
        self.name = name
        self.contains: dict[str, 'NameSpace'] = dict()

    def lookup(self, namespace: list[str], name: str, stack: str = "") -> 'NameSpace':
        if len(namespace) > 0:
            to_lookup = namespace[0]
            v = self.contains.get(to_lookup, None)
            if not v:
                raise Exception(f"unable to find namespace {to_lookup} in stack {stack}")
            return v.lookup(namespace[1:], name, stack+f"::{self.name}")
        else: # we looking up datatype now
            v = self.contains.get(name, None)
            if not v:
                raise Exception(f"unable to find datatype {name} in stack {stack}")
            return v

    def add(self, namespace: list[str], name: str, val: 'NameSpace'):
        if len(namespace) > 0:
            to_lookup = namespace[0]
            v = self.contains.get(to_lookup, None)
            if not v:
                v = NameSpace(to_lookup)
                self.contains[to_lookup] = v
            v.add(namespace[1:], name, val)
        else:
            to_lookup = name
            v = self.contains.get(to_lookup, None)
            if v:
                print(f"warning, datatype {name} already exists, overwriting")
            self.contains[name] = val

    def print(self, index):
        print(f"{' '*index}namespace {self.name}")
        for x in self.contains.values():
            x.print(index+2)

import re
def parse_cpp_type(type_str):
    #aied this lmao
    # Match the namespace and type name
    namespace_type_match = re.match(r'((?:\w+::)*)?(\w+)', type_str)
    if not namespace_type_match:
        raise ValueError(f"Invalid C++ type string: {type_str}")

    full_namespace = namespace_type_match.group(1) or ""
    type_name = namespace_type_match.group(2)
    namespace = full_namespace.split("::")[:-1] if full_namespace else []

    # Match the template arguments if they exist
    template_args = []
    template_match = re.search(r'<(.+)>', type_str)
    if template_match:
        raw_template_args = template_match.group(1)
        template_args = [arg.strip() for arg in raw_template_args.split(',')]
    return namespace, type_name, template_args


class DataType(NameSpace):
    def __init__(self, name: str, data_reg: DataTypeRegister):
        super().__init__(name)
        self.reg: DataTypeRegister = data_reg
        self.is_pod = data_reg.is_pod
        if data_reg.is_pod and (data_reg.custom_loader is not None or data_reg.custom_writer is not None):
            raise Exception(f"A datatype(type {name}) cant have a custom rw and be pod")
        if not data_reg.is_pod and None in [data_reg.custom_writer, data_reg.custom_loader]:
            raise Exception(f"A datatype(type {name}) with custom rw must define both reader and write and not be pod")
        if None not in [data_reg.custom_writer, data_reg.custom_loader]:
            self.has_rw_def = True
            self.Writer = data_reg.custom_writer
            self.Reader = data_reg.custom_loader
        else:
            self.has_rw_def = False
            self.Writer = None
            self.Reader = None

        if len(data_reg.template_type_args) > 0 and not self.has_rw_def:
            raise Exception(f"A datatype(type {name}) that has template args must define custom rw")
        if len(data_reg.members) > 0:
            self.is_ext = False
            self.vars: list['DataTypeInst'] = []
        else:
            self.is_ext = True
            self.vars: list['DataTypeInst'] = []

    def print(self, index):
        print(f"{' '*index}datatype {self.name}")
        for x in self.contains.values():
            x.print(index+2)

class DataTypeInst:
    def __init__(self, var_name: str, datatype: DataType, is_ptr: bool, obj_count: int = 1, template_args=None):
        if template_args is None:
            template_args = []
        self.var_name = var_name
        self.datatype = datatype
        self.is_ptr = is_ptr
        self.obj_count = obj_count
        self.template_args: list[DataTypeInst] = template_args
        self.build_serializer = False

    def is_array(self):
        return self.obj_count > 1

    def set_build(self, do_set: bool):
        if (do_set is False):
            return
        self.build_serializer = do_set

    # use datatype for extra info (like template data), the name is actually the name of the object var being written to
def default_bs_loader(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    if is_ptr:
        return f"REPL_VER(bs->Read(*{name}));"
    else:
        return f"REPL_VER(bs->Read({name}));"

def default_bs_writer(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    if is_ptr:
        return f"bs->Write(*{name});"
    else:
        return f"bs->Write({name});"

import ast

def iterate_classes_in_source_order(module):
    # Get the source file of the module
    source_file = inspect.getsourcefile(module)
    if not source_file:
        raise ValueError("Source file could not be found for the module")

    # Parse the module using AST to get the classes in source order
    with open(source_file, "r") as f:
        source_code = f.read()
    tree = ast.parse(source_code)
    class_names = [node.name for node in ast.walk(tree) if isinstance(node, ast.ClassDef)]

    # Use inspect to get the class objects and map them to their names
    classes = {name: obj for name, obj in inspect.getmembers(module, inspect.isclass)}

    # Iterate over the class objects in the order defined in the source file
    for class_name in class_names:
        if class_name in classes:
            yield classes[class_name]

class DataTypeManager:
    def register_type(self, reg: DataTypeRegister):
        namespace, name = parse_raw_datatype_name(reg.name)
        self.datatypes.add(namespace, name, DataType(name, reg))
        print(f"registered type {reg.name}")

    def __init__(self, modules: list[types.ModuleType]):
        self.datatypes = NameSpace("global")
        self.inst_datatypes: dict[str, DataTypeInst] = {}
        for imp in modules:
            for obj in iterate_classes_in_source_order(imp):
                name = obj.__name__
                if obj.__module__ == imp.__name__ and type(obj) is type(DataTypeRegister):
                    obj: DataTypeRegister
                    if obj.name == "":
                        raise Exception(f"DataTypeRegister class of name {name} has no name set")
                    self.register_type(obj)
        self.datatypes.print(0)

    def get_type_loader(self, data_type: DataTypeInst) -> str:
        pass

    def get_type_inst(self, type_inst: str, __do_inst=True) -> DataTypeInst:
        """
        
        :param type_inst:
        :return:
        """
        type_inst = type_inst.replace(" ", "").rstrip("\n\r\t").lstrip("\n\r\t")
        get = self.inst_datatypes.get(type_inst, None)
        if get is None: # doesnt exist, lets make it
            namespace, type_name, template_args = parse_cpp_type(type_inst)
            base_type = self.datatypes.lookup(namespace, type_name)
            assert isinstance(base_type, DataType) # TODO, make actual exception
            parsed_template_args = []
            if template_args:
                assert len(template_args) == len(base_type.reg.template_type_args)
                for i in range(len(template_args)):
                    type_ = base_type.vars
                parsed_template_args = [self.get_type_inst(x, False) for x in template_args]

        return get


#  83.87  IRC numeric event 744 origin chat-lw1.warthunder.com
#  81.76  IRC got IRC event NOTICE_CHAN by command NOTICE