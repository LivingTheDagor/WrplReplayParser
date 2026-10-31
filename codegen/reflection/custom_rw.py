from .DataTypes import *


def EntityId_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      REPL_VER(net::read_eid(*bs, {datatype.access_var(name)}));"


def EntityId_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      net::write_eid(*bs, {datatype.access_var(name)});"


def compressed_int_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"""      REPL_VER(bs->ReadCompressed({datatype.access_var(name)}));"""


def compressed_int_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"""      bs->WriteCompressed({datatype.access_var(name)});"""


def vector_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    template_type = datatype.compiled.template_args[0]
    size_type = datatype.compiled.template_args[1]
    size_name = datatype.compiled.template_args[1].datatype.reg.serialize_name(size_type)
    varname = f"sz{mgr.counter_inc()}"
    return f"""      {size_name} {varname};
{mgr.get_ReflectionVar_loader(DataTypeInst(varname, size_type, False, 1))}
      {datatype.get_ref_to_child(name)}resize({varname});
      for(auto & v : {datatype.access_var(name)}) {{
  {mgr.get_ReflectionVar_loader(DataTypeInst("v", template_type, False, 1), indent_lvl=2)}
      }}"""


def vector_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    template_type = datatype.compiled.template_args[0]
    size_type = datatype.compiled.template_args[1]
    size_name = datatype.compiled.template_args[1].datatype.reg.serialize_name(size_type)
    print(size_type)
    varname = f"sz{mgr.counter_inc()}"
    return f"""      auto {varname} = ({size_name}){datatype.get_ref_to_child(name)}size();
{mgr.get_ReflectionVar_writer(DataTypeInst(varname, size_type, False, 0))}
      for(auto & v : {datatype.access_var(name)}) {{
  {mgr.get_ReflectionVar_writer(DataTypeInst("v", template_type, False, 1), indent_lvl=2)}
      }}"""


def array_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    template_type = datatype.compiled.template_args[0]
    return f"""      for(auto & v : {datatype.access_var(name)}) {{
  {mgr.get_ReflectionVar_loader(DataTypeInst("v", template_type, False, 1), indent_lvl=2)}
      }}"""


def array_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    template_type = datatype.compiled.template_args[0]
    return f"""      for(auto & v : {datatype.access_var(name)}) {{
  {mgr.get_ReflectionVar_writer(DataTypeInst("v", template_type, False, 1), indent_lvl=2)}
      }}"""


def dummyForExitZonesSettings_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"""      int sz;
      REPL_VER(bs->ReadCompressed(sz));
      if(sz > 0) {{
        {datatype.get_ref_to_child(name)}vals.resize(sz);
        for(auto &v : ({datatype.access_var(name)}).vals) {{
          REPL_VER(bs->ReadCompressed(v.v1));
          REPL_VER(bs->ReadCompressed(v.v2));
        }}
      }}"""


def dummyForExitZonesSettings_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"""
      bs->WriteCompressed((int){datatype.get_ref_to_child(name)}vals.size());
      for(auto &v : ({datatype.access_var(name)}).vals) {{
        bs->WriteCompressed(v.v1);
        bs->WriteCompressed(v.v2);
      }}"""


def UnitId_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      REPL_VER(bs->ReadBits((uint8_t*)&{datatype.access_var(name)}, 0xb));"


def UnitId_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      bs->WriteBits((uint8_t*)&{datatype.access_var(name)}, 0xb);"
