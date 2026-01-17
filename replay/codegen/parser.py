import inspect
import sys

import builtin_types
import cpp_types
import objects.MPlayer as MPlayer
from objects.obj_base import ReflectableObject, ReflectionVarMeta
from DataTypes import DataTypeManager

obj_imports = [MPlayer]
type_imports = [builtin_types, cpp_types]
types = {}
mgr = DataTypeManager(type_imports)

sys.exit(0)

for imp in obj_imports:
    for name, obj in inspect.getmembers(imp, inspect.isclass):
        if obj.__module__ == imp.__name__: # only look at objs from file
            if type(obj) == type(ReflectableObject): # only do work on ReflectableObject
                curr_ids = set()
                obj_name = obj.__name__
                for attr_name, attr_val in obj.__dict__.items():
                    if(isinstance(attr_val, ReflectionVarMeta)): # only care about ReflectionVarMeta vals,
                        # python preserves order in dicts, so this parses vars in order that they are defined
                        print(attr_name)
                        if attr_val.var_id in curr_ids:
                            print(f"{obj_name}.{attr_name} repeats id {attr_val.var_id}")
                            assert False
                        curr_ids.add(attr_val.var_id)
