import inspect
import types
import ast
from typing import TextIO
from write_header import write_header

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
            return v.lookup(namespace[1:], name, stack + f"::{self.name}")
        else:  # we looking up datatype now
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
        print(f"{' ' * index}namespace {self.name}")
        for x in self.contains.values():
            x.print(index + 2)


import re


def parse_cpp_type(type_str):
    # aied this lmao
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
    def __init__(self, name: str, data_reg: DataTypeRegister, mgr: 'DataTypeManager'):
        super().__init__(name)
        self.reg: DataTypeRegister = data_reg
        self.is_pod = data_reg.is_pod
        if data_reg.is_pod and (data_reg.custom_loader is not None or data_reg.custom_writer is not None):
            raise Exception(f"A datatype(type {name}) cant have a custom rw and be pod")
        if not data_reg.is_pod and None in [data_reg.custom_writer, data_reg.custom_loader] and len(data_reg.members) == 0:
            raise Exception(f"A datatype(type {name}) with custom rw must define both reader and write and not be pod (or have components)")
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
            self.vars: list['DataTypeInst'] = [mgr.parse_var_decl(x) for x in data_reg.members]
        else:
            self.is_ext = True
            self.vars: list['DataTypeInst'] = []

    def print(self, index):
        print(f"{' ' * index}datatype {self.name}")
        for x in self.contains.values():
            x.print(index + 2)

    def to_serialize(self) -> list[str]:
        return [
            f"struct {self.name} {'{'}",
            *["  " + x.rstrip(";") + "{};" for x in self.reg.members],
            "};"
        ]


class DataTypeCompiled:
    def __init__(self, full_type_name: str, datatype: DataType, template_args: list['DataTypeCompiled']):
        self.full_type_name = full_type_name
        self.datatype: DataType = datatype
        self.template_args: list[DataTypeCompiled] = template_args
        self.serializer_build = False  # have we build a serializer for this type?

    def __str__(self):
        return self.datatype.name



class DataTypeInst:
    def __init__(self, var_name: str, compiled: DataTypeCompiled, is_ptr: bool = False, obj_count: int = 1):
        self.var_name = var_name
        self.compiled: DataTypeCompiled = compiled
        self.is_ptr = is_ptr
        self.obj_count = obj_count

    def is_array(self):
        return self.obj_count > 1

    def access_var(self, ctx: str):
        '''
        returns a string with accessing this var directly (think if you need to deref an into so you can read into it), so '*' or ''
        :param ctx: the previous vars leading to this. assume the previous var defines accessing this var
        :return:
        '''
        if self.is_ptr:
            return f"*({ctx}{self.var_name})"
        else:
            return f"{ctx}{self.var_name}"

    def get_ref_to_child(self, ctx: str):
        '''
        returns a string with this var accessing some component of it (think containg var or even a method of this var)
        :param ctx: the previous vars leading to this. assume the previous var defines accessing this var
        :return:
        '''
        if self.is_ptr:
            return f"{ctx}{self.var_name}->"
        else:
            return f"{ctx}{self.var_name}."


def default_bs_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      REPL_VER(bs->Read({datatype.access_var(name)}));"


def default_bs_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      bs->Write({datatype.access_var(name)});"


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
        self.datatypes.add(namespace, name, DataType(name, reg, self))
        # print(f"registered type {reg.name}")

    def __init__(self, modules: list[types.ModuleType]):
        self.datatypes = NameSpace("global")
        self.inst_datatypes: dict[str, DataTypeCompiled] = {}
        for imp in modules:
            for obj in iterate_classes_in_source_order(imp):
                name = obj.__name__
                if obj.__module__ == imp.__name__ and type(obj) is type(DataTypeRegister):
                    obj: DataTypeRegister
                    if obj.name == "":
                        raise Exception(f"DataTypeRegister class of name {name} has no name set")
                    self.register_type(obj)
        # self.datatypes.print(0)

        self.serializer_strs: list[str] = []
        self.EncoderChoosers: list[str] = []

    def recurse_types(self, data_type: DataTypeInst, ctx: str, out_list: list[tuple[str, DataTypeInst]]):
        if data_type.compiled.datatype.is_pod or data_type.compiled.datatype.has_rw_def:
            out_list.append((ctx, data_type))
            return
        new_ctx = data_type.get_ref_to_child(ctx)
        for var in data_type.compiled.datatype.vars:
            self.recurse_types(var, new_ctx, out_list)

    def get_ReflectionVar_writer(self, data_type: DataTypeInst, indent_lvl=0) -> str:
        all_to_be_serialized = []
        self.recurse_types(data_type, "", all_to_be_serialized)
        payload: list[str] = []
        for ctx, var in all_to_be_serialized:
            writer = default_bs_writer
            if var.compiled.datatype.has_rw_def:
                writer = var.compiled.datatype.Writer
            payload.append(writer(self, var, ctx))
        return f'\n{" "*indent_lvl}'.join(payload)

    def get_ReflectionVar_loader(self, data_type: DataTypeInst, indent_lvl=0) -> str:
        all_to_be_serialized: list[tuple[str, DataTypeInst]] = []
        self.recurse_types(data_type, "", all_to_be_serialized)
        payload: list[str] = []
        for ctx, var in all_to_be_serialized:
            loader = default_bs_loader
            if var.compiled.datatype.has_rw_def:
                loader = var.compiled.datatype.Reader
            payload.append(loader(self, var, ctx))
        return f'\n{" "*indent_lvl}'.join(payload)

    def get_serializer_name(self, data_type: DataTypeCompiled):
        if len(data_type.template_args) > 0:
            name = '_'.join([str(x) for x in data_type.template_args]) + data_type.datatype.name + "Coder"
        else:
            name = data_type.datatype.name + "Coder"
        return name

    def request_reflectionVar_serializer(self, data_type_str: str) -> str:
        '''
        requests the mgr for the serializer function name, if the serializer is requested but not written yet,
         it also generates the appropriate code (both function and DefaultEncoderChooser
        :param data_type_str: name of datatype
        :return: serializer function name
        '''
        data_type = self.get_compiled_type(data_type_str)
        name = self.get_serializer_name(data_type)
        if data_type.serializer_build:
            return name

        serializer = self.get_ReflectionVar_serializer(data_type)
        self.serializer_strs.append(serializer)
        payload = f"""
  template <>
  struct DefaultEncoderChooser<{data_type.full_type_name}> {'{'}
    static constexpr reflection_var_encoder coder = {name};
  {'}'};"""
        self.EncoderChoosers.append(payload)
        data_type.serializer_build = True
        return name


    def get_ReflectionVar_serializer(self, data_type: DataTypeCompiled) -> str:
        name = self.get_serializer_name(data_type)
        datatype = data_type.full_type_name
        inst = DataTypeInst("data", data_type, True, 1)
        return f"""
  int {name}(DANET_ENCODER_SIGNATURE) {'{'}
    auto data = meta->getValue<{datatype}>();
    if (op == DANET_REFLECTION_OP_ENCODE) {'{'}
{self.get_ReflectionVar_writer(inst)}
      return true;
    {'}'}
    else if (op == DANET_REFLECTION_OP_DECODE) {'{'}
{self.get_ReflectionVar_loader(inst)}
      return true;
    {'}'}
    return false;
  {'}'}
"""


    def parse_var_decl(self, decl_str: str) -> DataTypeInst:
        out = decl_str.split(" ")
        var_type, var_name = "", ""
        if len(out) > 3:
            Exception("var decl cannot have more than 2 spaces in it")
        if len(out) == 3:
            if out[1] != "*":
                Exception("you cannot have spaces inside a var declaration")
            var_type = out[0] + out[1]
            var_name = out[2]
        else:
            var_type = out[0]
            var_name = out[1]
        if var_type[-1] == "*":
            is_ptr = True
            var_type = var_type[:-1]
        else:
            is_ptr = False

        if '[' in var_name:
            var_count = re.findall(r"\[(.*?)]", var_name)
            parsed_name = var_name[:var_name.find("[")]
        else:
            var_count = 1
            parsed_name = var_name.rstrip(";")
        return self.parse_type(var_type, parsed_name, is_ptr, var_count)

    def parse_type(self, type_str: str, name: str, is_ptr: bool, count: int) -> DataTypeInst:
        type_comp = self.get_compiled_type(type_str)
        return DataTypeInst(name, type_comp, is_ptr, count)
    def get_compiled_type(self, type_inst: str) -> DataTypeCompiled:
        """
        
        :param type_inst:
        :return:
        """
        type_inst = type_inst.replace(" ", "").rstrip("\n\r\t").lstrip("\n\r\t")
        get = self.inst_datatypes.get(type_inst, None)
        if get is None:  # doesnt exist, lets make it
            namespace, type_name, template_args = parse_cpp_type(type_inst)
            base_type = self.datatypes.lookup(namespace, type_name)
            assert isinstance(base_type, DataType)  # TODO, make actual exception
            parsed_template_args = []
            if template_args:
                assert len(template_args) == len(base_type.reg.template_type_args)
                for i in range(len(template_args)):
                    type_ = base_type.reg.template_type_args[i]
                    if type_ is DataTypeType:
                        parsed_template_args.append(self.get_compiled_type(template_args[i]))
                    elif type_ is int:
                        parsed_template_args.append(int(template_args[i]))
                    else:
                        raise Exception(f"Unknown Datatype template arg: {str(type_)}")
                # parsed_template_args = [self.get_compiled_type(x) for x in template_args]
            inst = DataTypeCompiled(type_inst, base_type, template_args=parsed_template_args)
            self.inst_datatypes[type_inst] = inst
            get = inst
        return get

    def compile(self, codegen_header_path: str, codegen_cpp_path: str):
        def does_nm_contain_serialized_types(nm: NameSpace):
            if isinstance(nm, DataType):
                if len(nm.vars) > 0:
                    return True
            for nms in nm.contains.values():
                payload = does_nm_contain_serialized_types(nms)
                if payload:
                    return True
            return False

        def write_namespace(nm_: NameSpace, indent: int, io: TextIO):
            def write_indent(addon: int = 0):
                io.write(' '*(indent+addon))
            for nm in nm_.contains.values():
                if does_nm_contain_serialized_types(nm):
                    if isinstance(nm, DataType):
                        for line in nm.to_serialize():
                            write_indent()
                            io.write(f"{line}\n")
                    else:
                        write_indent()
                        io.write(f"namespace {nm.name} {'{'}\n")
                        write_namespace(nm, indent+2, io)
                        io.write('}\n')


        with open(f"{codegen_header_path}/types.h", "w") as f:
            write_header(f)
            f.write("#pragma once\n")
            write_namespace(self.datatypes, 0, f)


        with open(f"{codegen_header_path}/serializers.h", "w") as f:
            write_header(f)
            f.write("#pragma once\n")
            f.write("""#define REPL_VER(x) \\\n  if (!(x))         \\\n    return false\n\n""")
            f.write("namespace danet {\n")
            for serializer in self.inst_datatypes.values():
                if serializer.serializer_build:
                    name = self.get_serializer_name(serializer)
                    f.write(f"  int {name}(DANET_ENCODER_SIGNATURE);\n\n")

            for chooser in self.EncoderChoosers:
                f.write(chooser + "\n")
            f.write("}")

            with open(f"{codegen_cpp_path}/serializers.cpp", "w") as f:
                write_header(f)
                f.write("#include \"mpi/serializers.h\"\n")
                f.write("#include \"ecs/entityId.h\"\n")
                f.write("#include \"network/eid.h\"\n")
                f.write("namespace danet {\n")
                for serializer in self.serializer_strs:
                    f.write(serializer)
                f.write("}")
