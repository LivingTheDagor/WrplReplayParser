import inspect
import io
from io import StringIO

import builtin_types
import cpp_types
import objects.MPlayer as MPlayer
import objects.TeamData as TeamData
from objects.obj_base import ReflectableObject, ReflectionVarMeta
from DataTypes import DataTypeManager
from write_header import write_header

def generate(header_codegen_path: str, cpp_codegen_path: str):
    obj_imports = [MPlayer, TeamData]
    type_imports = [builtin_types, cpp_types]
    mgr = DataTypeManager(type_imports)
    refl_include_paths: list[str] = []
    for imp in obj_imports:
        for name, obj in inspect.getmembers(imp, inspect.isclass):
            if obj.__module__ == imp.__name__: # only look at objs from file
                if type(obj) == type(ReflectableObject): # only do work on ReflectableObject
                    payload: StringIO = io.StringIO()
                    curr_ids = set()
                    obj_name = obj.__name__
                    payload.write(f"class {name} : public danet::ReflectableObject")
                    payload.write(" {\npublic:\n")
                    first: str = ""
                    prev: tuple[str, ReflectionVarMeta] | None = None
                    for attr_name, attr_val in obj.__dict__.items():
                        if(isinstance(attr_val, ReflectionVarMeta)): # only care about ReflectionVarMeta vals,
                            # python preserves order in dicts, so this parses vars in order that they are defined
                            if attr_val.var_id in curr_ids:
                                raise Exception(f"{obj_name}.{attr_name} repeats id {attr_val.var_id}")
                            curr_ids.add(attr_val.var_id)
                            attr_val.requestCoder(mgr)
                            if prev is None:
                                first = attr_name
                            else:
                                payload.write(f"  danet::ReflectionVar<{prev[1].data_type}> {prev[0]}" "{" f"\"{prev[0]}\", &{attr_name}, {prev[1].var_id}" "};\n")
                            prev = (attr_name, attr_val)
                    payload.write(f"  danet::ReflectionVar<{prev[1].data_type}> {prev[0]}" "{" f"\"{prev[0]}\", nullptr, {prev[1].var_id}" "};\n")
                    payload.write(f"  {name}()" " {\n")
                    payload.write(f"    varList.head = &{first};\n")
                    payload.write(f"    varList.tail = &{prev[0]};\n")
                    payload.write("  }\n")
                    payload.write("};\n\n")
                    payload.write(f"ECS_DECLARE_CREATABLE_TYPE({name});\n")
                    # we have passed previous checks, lets serialize
                    with open(f"{header_codegen_path}/ReflectableObjects/{obj_name}.h", "w") as f:
                        refl_include_paths.append(f"mpi/codegen/ReflectableObjects/{obj_name}.h")
                        write_header(f)
                        f.write("#pragma once\n\n")
                        f.write(payload.getvalue())


    mgr.compile(header_codegen_path, cpp_codegen_path) # probably wont fail
    with open(f"{header_codegen_path}/ReflIncludes.h", "w") as f1:
        write_header(f1)
        f1.write("#pragma once\n")
        f1.write("#include \"mpi/serializers.h\"\n")
        for header in refl_include_paths:
            f1.write(f"#include \"{header}\"\n")

if __name__ == "__main__":
    header_codegen_path = r"D:\ReplayParser\replay\include\mpi\codegen"
    cpp_codegen_path = r"D:\ReplayParser\replay\mpi\codegen"
    generate(header_codegen_path, cpp_codegen_path)
    print("Codegen Completed")