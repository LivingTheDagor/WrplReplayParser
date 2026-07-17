import os.path
import sys

import reflection.parser
from codegen.reflection import custom_rw
from hashCheck import HashChecker

from reflection import builtin_types
from  reflection import cpp_types
import reflection.objects.ReflectableObjects as objects
from reflection.parser import generate_reflectables, generate_bindings
import inspect
import pathlib
from ecs.gen_es import generate

ROOT_PATH = '/'.join(sys.argv[0].split("\\")[:-2])


def check_hash(hash_name: str, file_paths: list[str]) -> bool:
    with HashChecker("keyfile.json") as checker:
        # file_paths = [*[inspect.getsourcefile(module) for module in obj_imports], *[inspect.getsourcefile(module) for module in type_imports]]
        hash_matches = checker.compute_hash(hash_name, file_paths)
        if hash_matches:
            print(f"Hash check passed for {hash_name}, skipping code generation")
            return True
        else:
            print(f"Hash Missmatch for {hash_name}, running code generation")
    return False


def codegen_reflection(force_gen: bool = False):
    obj_imports = [objects]
    type_imports = [builtin_types, cpp_types]

    file_paths = [*[inspect.getsourcefile(module) for module in obj_imports], *[inspect.getsourcefile(module) for module in type_imports]]
    file_paths.append(inspect.getsourcefile(custom_rw))
    if check_hash("ReflectionObjBindings", file_paths) and False:
        return


    root_path = ROOT_PATH + "/"
    obj_codegen_header = root_path + r"replay/include/mpi/codegen"
    obj_codegen_cpp = root_path + r"replay/mpi/codegen"

    binding_codegen_header = root_path + r"python/include/modules/mpi"
    binding_codegen_cpp = root_path + r"python/mpi"
    generate_reflectables(obj_codegen_header, obj_codegen_cpp, force_gen)

    generate_bindings(binding_codegen_header, binding_codegen_cpp, force_gen)


# codegen for DagECS. this basically calls into the dagECS codegen.
def codegen_ecs():
    SEARCH_DIRS = [f"{ROOT_PATH}/replay", f"{ROOT_PATH}/replay_inspector"]
    parsed_objects = set()
    for d in SEARCH_DIRS:
        for p in pathlib.Path(d).rglob("*.cpp.inl"):
            p: pathlib.Path
            file_name = p.name[:-len(".cpp.inl")]
            if file_name in parsed_objects:
                raise Exception(f"cannot have ECS codegen files with the same name ({file_name})")
            parsed_objects.add(file_name)
            if not check_hash(file_name, [str(p)]): # or True
                inp_file_path = str(p).replace("\\", "/")
                out_file_path = str(p).replace(".cpp.inl", ".gen.es.cpp").replace("\\", "/")
                rel_path = p.name.replace("\\", "/")
                generate(inp_file_path, out_file_path, rel_path)
                print(f"generated {inp_file_path}")






if __name__ == "__main__":
    assert os.path.exists(r"D:\develop\devtools\LLVM-18.1.8") # blame gaijin they wanted it
    os.environ['DAGOR_CLANG_DIR'] = r"D:\develop\devtools\LLVM-18.1.8"
    codegen_reflection()
    codegen_ecs()