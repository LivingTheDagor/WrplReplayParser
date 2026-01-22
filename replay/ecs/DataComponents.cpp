
#include "ecs/DataComponents.h"
#include "utils.h"
namespace ecs
{
  CompileComponentRegister *CompileComponentRegister::tail = nullptr;
  std::string_view DataComponent::getName() const{
    //G_ASSERT(this->name_index>0);
    return this->owner->getName(this);
  }

  component_index_t DataComponents::createComponent(HashedConstString name_, type_index_t component_type,
                                                              ComponentSerializer *io, ComponentTypes &types) {
    const char *nameStr = name_.str;
    hash_str_t nameHash = name_.hash;
    auto existingId = this->getIndex(nameHash);

    // case were component already exists
    if (DAGOR_UNLIKELY(existingId != INVALID_COMPONENT_INDEX)) {
      auto existing_component = getDataComponent(existingId);
      if (nameStr != nullptr && existing_component->name_index != 0 &&
          strcmp(nameStr, names.getDataRawUnsafe(existing_component->name_index)) != 0) //-V522
      {
        EXCEPTION("component <{}> with same hash ={:#x} as <{}> is already registered, hash collision.",
                  getName(existingId),
                  nameHash, nameStr);
        return INVALID_COMPONENT_INDEX;
      }
      if (existing_component->componentIndex != component_type) {
        if (component_type != INVALID_COMPONENT_TYPE_INDEX) {
          EXCEPTION("component <{}>({:#x}) with type <{}>({}) is already registered with different type <{}>({})!",
                    nameStr, nameHash,
                    types.getName(component_type), component_type,
                    types.getName(this->getDataComponent(existingId)->componentIndex),
                    this->getDataComponent(existingId)->componentIndex);
        } else
          component_type = existing_component->componentIndex;
      }
      if (existing_component->name_index == 0 && nameStr != nullptr)
        existing_component->name_index = names.addDataRaw(nameStr,
                                                          strlen(nameStr) + 1); // replace nullptr name with
      return existingId;
    }

    const char *usedName = nameStr;

    if (component_type == INVALID_COMPONENT_TYPE_INDEX) {
      return INVALID_COMPONENT_INDEX;
    }

    component_type_t component_type_name = types.getComponentData(component_type)->hash;
    G_ASSERT_RETURN(components.size() < INVALID_COMPONENT_INDEX - 1, INVALID_COMPONENT_INDEX);
    componentIndex[nameHash] = (component_index_t)components.size();
    const uint32_t nameAddr = usedName ? names.addDataRaw(usedName, strlen(usedName) + 1) : 0;
    components.emplace_back(nameHash,component_type, component_type_name, io, this, nameAddr,
                            io ? DataComponent::HAS_SERIALIZER : 0);
    return (component_index_t)components.size() - 1;
    LOG("create {} ecs component <{}> hash<{:#x}> of component_type {}<{}|{:#x}>", components.size() - 1,
                usedName,
                nameHash, component_type, types.getName(component_type).data(), component_type_name);
    G_UNUSED(types);
    return (component_index_t)components.size() - 1;
  }

  void DataComponents::initialize(ComponentTypes &types) {
    //clear();
    LOG("ecs: initialize DataComponents Types");
    this->names.addDataRaw("", 1); // ensures the zeroth index is not a valid name / is emtpy name
    createComponent(ECS_HASH("eid"), types.findType(ECS_HASH("ecs::EntityId").hash), nullptr, types);
    // initialize eid and tag first, for debugging purposes?
    for (CompileComponentRegister *start = CompileComponentRegister::tail; start; start = start->next) {
      //std::cout << start->name.str << " : " << start->type_name << "\n";
      createComponent(start->name, types.findType(start->type), start->io, types);
    }
  }

}

