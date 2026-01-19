from DataTypes import *

def datablock_loader(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    ptr_type = "->" if is_ptr else "."
    return f"""
uint32_t size;
REPL_VER(bs->ReadCompressed(size));
if (size > 0)
{'{'}
  std::vector<char> raw;
  raw.resize(size);
  REPL_VER(bs->ReadArray(raw.data(), size));
  BaseReader rdr{'{'}raw.data(), (int)raw.size(), false{'}'};
  REPL_VER({name}{ptr_type}loadFromStream(rdr, nullptr, nullptr));
{'}'}
"""

def datablock_writer(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    return f"EXCEPTION(\"datablock packing is unsupported for {name}\");"


def EntityId_loader(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    if is_ptr:
        return f"REPL_VER(net::read_eid(*bs, *({name})));"
    else:
        return f"REPL_VER(net::read_eid(*bs, {name}));"

def EntityId_writer(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    if is_ptr:
        return f"net::write_eid(*bs, *({name}));"
    else:
        return f"net::write_eid(*bs, {name});"



def entity_id_t_loader(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    if is_ptr:
        return f"REPL_VER(net::read_server_eid(*({name}), *bs));"
    else:
        return f"REPL_VER(net::read_server_eid({name}, *bs));"

def entity_id_t_writer(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    if is_ptr:
        return f"net::write_server_eid(*({name}), *bs);"
    else:
        return f"net::write_server_eid({name}, *bs);"

def vector_loader(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    s_ptr = "*" if is_ptr else ""
    s_ptr_2 = "->" if is_ptr else "."
    return f"""
uint8_t sz;
REPL_VER(bs->Read(sz));
{name}{s_ptr_2}resize(sz);
for(auto & v : data->vals) {'{'}

{'}'}
"""

def vector_loader(mgr: 'DataTypeRegister', datatype: DataTypeInst, name: str, is_ptr: bool):
    if is_ptr:
        return f"net::write_server_eid(*({name}), *bs);"
    else:
        return f"net::write_server_eid({name}, *bs);"