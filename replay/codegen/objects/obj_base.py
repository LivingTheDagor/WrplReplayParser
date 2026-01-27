from abc import ABC, abstractmethod
from io import StringIO


class NullIndexList:
    def __init__(self, ls: list[tuple[str, 'ReflectionVarMeta']]):
        self.ls: list[tuple[str, 'ReflectionVarMeta']] = ls

    def __getitem__(self, item):
        if item >= len(self.ls):
            return None
        return self.ls[item]

class ReflectableObject(ABC):
    def __init__(self):
        pass


class ReplicatedObject(ReflectableObject):
    def __init__(self):
        super().__init__()


# we get var name from defined class, so we only need the var_id and optionally the special encoder this var may need, default is often enough though
class ReflectionVarMeta(ABC):
    def __init__(self, data_type: str, var_id: int, EncoderName=""):
        self.data_type = data_type
        self.var_id = var_id
        self.EncoderName = EncoderName
        self.has_custom_coder = EncoderName != ""

    def requestCoder(self, mgr: 'DataTypeManager'):
        if self.EncoderName == "":
            self.EncoderName = mgr.request_reflectionVar_serializer(self.data_type)

class InstReflectable:
    def __init__(self, obj: ReflectableObject, oss:StringIO, mgr: 'DataTypeManager'):
        self.obj = obj
        self.oss = oss
        self.vars: NullIndexList
        self.mgr = mgr
        self.first_var: str = ""
        self.last_var: str = ""
        self.obj_name = self.obj.__name__

    def _iter_vars(self):
        for attr_name, attr_val in self.obj.__dict__.items():
            if(isinstance(attr_val, ReflectionVarMeta)): # only care about ReflectionVarMeta vals,
                yield (attr_name, attr_val)

    def write_header(self):
        self.oss.write(f"class {self.obj_name} : public danet::ReflectableObject")
        self.oss.write(" {\npublic:\n"f"  DECL_REFLECTION({self.obj_name}, danet::ReflectableObject)\n")

    def verify_params(self):
        curr_ids = set()
        payload: list = []
        for varname, var in self._iter_vars():
            if var.var_id in curr_ids:
                raise Exception(f"{self.obj_name}.{varname} repeats id {var.var_id}")
            curr_ids.add(var.var_id)
            var.requestCoder(self.mgr)
            payload.append((varname, var))
        self.vars = NullIndexList(payload)

    def serialize_params(self):
        self.first_var = self.vars[0][0]
        self.last_var = self.vars.ls[-1][0]
        for i in range(len(self.vars.ls)):
            curr_var = self.vars[i]
            next_var = self.vars[i+1]
            next_var_ref = "nullptr" if next_var is None else f"&{next_var[0]}"
            if curr_var[1].has_custom_coder:
                self.oss.write(f"  danet::ReflectionVar<{self.mgr.refractor_raw_name(curr_var[1].data_type)}> {curr_var[0]}" "{" f"\"{curr_var[0]}\", {next_var_ref}, {curr_var[1].var_id}, danet::{curr_var[1].EncoderName}" "};\n")
            else:
                self.oss.write(f"  danet::ReflectionVar<{self.mgr.refractor_raw_name(curr_var[1].data_type)}> {curr_var[0]}" "{" f"\"{curr_var[0]}\", {next_var_ref}, {curr_var[1].var_id}" "};\n")

    def write_ctor(self):
        self.oss.write(f"  {self.obj_name}() : ReflectableObject() " " {\n")
        self.oss.write(f"    varList.head = &{self.first_var};\n")
        self.oss.write(f"    varList.tail = &{self.last_var};\n")
        self.oss.write("  }\n")
    def write_footer(self):
        self.oss.write("};\n\n")
        self.oss.write(f"ECS_DECLARE_CREATABLE_TYPE({self.obj_name});\n")

class InstReplicated(InstReflectable):
    def __init__(self, obj: ReflectableObject, oss: StringIO, mgr: 'DataTypeManager'):
        super().__init__(obj, oss, mgr)


    def write_header(self):
        self.oss.write(f"class {self.obj_name} : public danet::ReplicatedObject")
        self.oss.write(" {\npublic:\n"f"  DECL_REPLICATION({self.obj_name}, danet::ReplicatedObject)\n")

    def write_ctor(self):
        self.oss.write(f"  {self.obj_name}() : ReplicatedObject() " " {\n")
        self.oss.write(f"    varList.head = &{self.first_var};\n")
        self.oss.write(f"    varList.tail = &{self.last_var};\n")
        self.oss.write("  }\n")

    def write_footer(self):
        self.oss.write("};\n\n")
