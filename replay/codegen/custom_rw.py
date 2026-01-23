from DataTypes import *

def datablock_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"""
      uint32_t size;
      REPL_VER(bs->ReadCompressed(size));
      if (size > 0)
      {'{'}
        std::vector<char> raw;
        raw.resize(size);
        REPL_VER(bs->ReadArray(raw.data(), size));
        BaseReader rdr{'{'}raw.data(), (int)raw.size(), false{'}'};
        REPL_VER({datatype.get_ref_to_child(name)}loadFromStream(rdr, nullptr, nullptr));
      {'}'}"""

def datablock_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      EXCEPTION(\"DataBlock packing is unsupported for {name}\");"


def EntityId_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      REPL_VER(net::read_eid(*bs, {datatype.access_var(name)}));"


def EntityId_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      net::write_eid(*bs, {datatype.access_var(name)});"


def entity_id_t_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      REPL_VER(net::read_server_eid({datatype.access_var(name)}, *bs));"

def entity_id_t_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    return f"      net::write_server_eid({datatype.access_var(name)}, *bs);"

def vector_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    template_type = datatype.compiled.template_args[0]
    return f"""      uint8_t sz;
      REPL_VER(bs->Read(sz));
      {datatype.get_ref_to_child(name)}resize(sz);
      for(auto & v : {datatype.access_var(name)}) {'{'}
  {mgr.get_ReflectionVar_loader(DataTypeInst("v", template_type, False, 1), indent_lvl=2)}
      {'}'}"""

def vector_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    template_type = datatype.compiled.template_args[0]
    return f"""      bs->Write((uint8_t){datatype.get_ref_to_child(name)}size());
      for(auto & v : {datatype.access_var(name)}) {'{'}
  {mgr.get_ReflectionVar_writer(DataTypeInst("v", template_type, False, 1), indent_lvl=2)}
      {'}'}"""

def array_loader(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    template_type = datatype.compiled.template_args[0]
    return f"""      for(auto & v : {datatype.access_var(name)}) {'{'}
  {mgr.get_ReflectionVar_loader(DataTypeInst("v", template_type, False, 1), indent_lvl=2)}
      {'}'}"""

def array_writer(mgr: 'DataTypeManager', datatype: DataTypeInst, name: str):
    template_type = datatype.compiled.template_args[0]
    return f"""      for(auto & v : {datatype.access_var(name)}) {'{'}
  {mgr.get_ReflectionVar_writer(DataTypeInst("v", template_type, False, 1), indent_lvl=2)}
      {'}'}"""
