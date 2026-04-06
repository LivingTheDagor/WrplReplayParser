import inspect
import io
from io import StringIO

import builtin_types
import cpp_types
import objects.ReflectableObjects as objects
from objects.obj_base import ReflectableObject, ReplicatedObject, InstReflectable, InstReplicated
from DataTypes import DataTypeManager, iterate_classes_in_source_order
from write_header import write_header
from hashCheck import HashChecker

def check_hash(hash_name: str, obj_imports, type_imports) -> bool:
    with HashChecker("keyfile.json") as checker:
        file_paths = [*[inspect.getsourcefile(module) for module in obj_imports], *[inspect.getsourcefile(module) for module in type_imports]]
        hash_matches = checker.compute_hash(hash_name, file_paths)
        if hash_matches:
            print("Hash check passed for ReflectionGenerate, skipping code generation")
            return True
        else:
            print("Hash Missmatch for ReflectionGenerate, running code generation")
    return False

def generate(header_codegen_path: str, cpp_codegen_path: str, force_gen: bool = False):

    obj_imports = [objects]
    type_imports = [builtin_types, cpp_types]
    if not force_gen:
        if check_hash("ReflectionGenerate", obj_imports, type_imports):
            return
    mgr = DataTypeManager(type_imports)
    refl_include_paths: list[str] = []
    for imp in obj_imports:
        for obj in iterate_classes_in_source_order(imp):
            name = obj.__name__
            if obj.__module__ == imp.__name__: # only look at objs from file
                if type(obj) == type(ReflectableObject): # only do work on ReflectableObject / things that inherit it
                    payload: StringIO = io.StringIO()
                    if isinstance(obj(), ReplicatedObject):
                        x = InstReplicated(obj, payload, mgr)
                        print(f"serializing ReplicatedObject {name}")
                    else:
                        print(f"serializing ReflectableObject {name}")
                        x = InstReflectable(obj, payload, mgr)
                    x.write_header()
                    x.verify_params()
                    x.serialize_params()
                    x.write_ctor()
                    x.write_footer()
                    obj_name = x.obj_name
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
        f1.write("#include \"mpi/Replication.h\"\n")

def generate_bindings(header_codegen_path: str, cpp_codegen_path: str, force_gen: bool = False):
    obj_imports = [objects]
    type_imports = [builtin_types, cpp_types]
    if not force_gen:
        if check_hash("PythonBindingsGenerate", obj_imports, type_imports):
            return
    mgr = DataTypeManager(type_imports)
    with open(f"{cpp_codegen_path}/codegen_objects.cpp", "w") as f:
        write_header(f)
        f.write("#include \"modules/mpi/codegen_objects.h\"\n")
        f.write("#include \"modules/mpi/types.h\"\n")
        f.write("#include \"mpi/codegen/ReflIncludes.h\"\n")
        f.write("#include \"modules/mpi/mpi.h\"\n")
        f.write("PyCodegenObjects py_codegen_objects;\n")
        f.write("void PyCodegenObjects::include(py::module_ &m) {\n")
        f.write("  DO_INCLUDE()\n")
        f.write("  py_mpi_types.include(m);\n")
        f.write("  py_mpi.include(m);\n")
        f.write("  auto mpi = m.def_submodule(\"mpi\");\n")
        for imp in obj_imports:
            for obj in iterate_classes_in_source_order(imp):
                name = obj.__name__
                if obj.__module__ == imp.__name__: # only look at objs from file
                    if type(obj) == type(ReflectableObject): # only do work on ReflectableObject / things that inherit it
                        # payload: StringIO = io.StringIO()
                        if isinstance(obj(), ReplicatedObject):
                            x = InstReplicated(obj, f, mgr)
                            print(f"serializing ReplicatedObject {name}")
                        else:
                            print(f"serializing ReflectableObject {name}")
                            x = InstReflectable(obj, f, mgr)
                        x.verify_params_no_coder()
                        x.write_header_bindings()
                        x.serialize_bindings()
        f.write("}")


    mgr.compile_bindings(header_codegen_path, cpp_codegen_path)






if __name__ == "__main__":
    header_codegen_path = r"D:\ReplayParser\replay\include\mpi\codegen"
    cpp_codegen_path = r"D:\ReplayParser\replay\mpi\codegen"
    generate(header_codegen_path, cpp_codegen_path, force_gen=True)

    header_codegen_path = r"D:\ReplayParser\python\include\modules\mpi"
    cpp_codegen_path = r"D:\ReplayParser\python\mpi"

    generate_bindings(header_codegen_path, cpp_codegen_path, True)
    print("Codegen Completed")