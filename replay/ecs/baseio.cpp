
#include "utils.h"
#include "ecs/DataComponents.h"
#include "ecs/EntityManager.h"
#include "ecs/baseIo.h"

namespace ecs
{

  void serialize_entity_component_ref_typeless(const void *comp_data, component_index_t cidx, component_type_t type_name,
                                               type_index_t type_id, SerializerCb &serializer, ecs::EntityManager *mgr)
  {
    auto componentTypes = g_ecs_data->getComponentTypes();
    ComponentSerializer *typeIO = nullptr;
    if (cidx != ecs::INVALID_COMPONENT_INDEX)
      typeIO = g_ecs_data->getDataComponents()->getTypeIo(cidx);
    auto componentTypeInfo = *componentTypes->getComponentData(type_id);

    const bool isPod = is_pod(componentTypeInfo.flags);
    if (!typeIO && has_io(componentTypeInfo.flags))
      typeIO = componentTypes->getComponentData(type_id)->serializer;
    G_ASSERT(typeIO || isPod);
    if (!typeIO && isPod) // pod data can be just readed as-is
    {
      serializer.write(comp_data, componentTypeInfo.size * 8, type_name);
    }
    else if (typeIO)
    {
      typeIO->serialize(serializer, comp_data, componentTypeInfo.size, type_name, mgr);
    }
  }


  void serialize_entity_component_ref_typeless(const ComponentRef &comp, SerializerCb &serializer, ecs::EntityManager *mgr)
  {
    G_ASSERT(comp.getComponentId() != INVALID_COMPONENT_INDEX);
    serialize_entity_component_ref_typeless(comp.getRawData(), comp.getComponentId(), comp.getUserType(), comp.getTypeId(), serializer,
                                            mgr);
  }

  void serialize_child_component(const Component &comp, SerializerCb &serializer, ecs::EntityManager *mgr)
  {
    component_type_t userType = comp.getUserType();
    serializer.write(&userType, sizeof(userType) * CHAR_BIT, 0);
    serialize_entity_component_ref_typeless(comp.getRawData(), ecs::INVALID_COMPONENT_INDEX, comp.getUserType(), comp.getTypeId(),
                                            serializer, mgr);
  }
  MaybeComponent deserialize_init_component_typeless(component_type_t userType, ecs::component_index_t cidx,
                                                     const DeserializerCb &deserializer, ecs::EntityManager *mgr)
  {
    if (userType == 0)
      return {}; // Assume that null type is not an error
    auto componentTypes = g_ecs_data->getComponentTypes();
    ComponentSerializer *typeIO = nullptr;
    type_index_t typeId;
    if (cidx != ecs::INVALID_COMPONENT_INDEX)
    {
      auto components = g_ecs_data->getDataComponents();
      auto dataComp = components->getDataComponent(cidx);
      typeId = dataComp->componentIndex;
      G_ASSERT(dataComp->componentHash == userType);
      if (dataComp->flags & DataComponent::HAS_SERIALIZER)
      {
        typeIO = components->getTypeIo(cidx);
        G_ASSERT(typeIO != nullptr);
      }
    }
    else
    {
      typeId = componentTypes->findType(userType); // hash lookup
      if (DAGOR_UNLIKELY(typeId == INVALID_COMPONENT_TYPE_INDEX))
      {
        EXCEPTION("Attempt to deserialize component of invalid/unknown type 0x%X", userType);
        return {}; // We can't read unknown type of unknown size
      }
    }
    auto componentTypeInfo = *componentTypes->getComponentData(typeId);
    if (has_io(componentTypeInfo.flags) && !typeIO) // datacomponent io takes priority
      typeIO = g_ecs_data->getComponentTypes()->getComponentData(typeId)->serializer;
    //ecsdebug::track_ecs_component_by_index(cidx, ecsdebug::TRACK_WRITE, "deserialize_init");
    const bool isPod = is_pod(componentTypeInfo.flags);
    // always 'boxed', always allocating data
    //const bool allocatedMem = ChildComponent::is_child_comp_boxed_by_size(componentTypeInfo.size);
    //alignas(void *) char compData[8]; // actually, should be sizeof(ChildComponent::Value), but it is protected
    void *tempData;
    tempData = malloc(componentTypeInfo.size);
    if (typeIO)
    {
      //const bool isBoxed = (componentTypeInfo.flags & COMPONENT_TYPE_BOXED) != 0;
      ComponentTypeManager *ctm = NULL;
      if (need_constructor(componentTypeInfo.flags))
      {
        // yes, this const cast is ugly. It yet has to be here, there is no other way (besides explicit method in entity manager of
        // course).
        ctm = const_cast<ComponentTypes *>(componentTypes)->getCTM(typeId);
        G_ASSERTF(ctm, "type manager for type {:#x} ({}) missing", userType, typeId);
      }
      if (ctm)
        ctm->create(tempData, *mgr, INVALID_ENTITY_ID, cidx);
      else if (!isPod)
        memset(tempData, 0, componentTypeInfo.size);
      if (typeIO->deserialize(deserializer, tempData, componentTypeInfo.size, userType, mgr))
        return Component(tempData,userType, typeId, componentTypeInfo.size);
      else {return {};}
    }
    else if (isPod) // pod data can be just readed as-is
    {
      if (deserializer.read(tempData, componentTypeInfo.size * CHAR_BIT, userType))
        return Component(tempData, userType, typeId, componentTypeInfo.size);
    }
    else
      EXCEPTION("Attempt to deserialize type 0x%X %d<%s>, which has no typeIO and not pod", userType, typeId,
             componentTypes->getName(typeId).data());
    if (tempData)
      free(tempData);
    return {};
  }
  MaybeComponent deserialize_child_component(const DeserializerCb &deserializer, ecs::EntityManager *mgr)
  {
    component_type_t userType = 0;
    if (deserializer.read(&userType, sizeof(userType) * CHAR_BIT, 0))
      return deserialize_init_component_typeless(userType, ecs::INVALID_COMPONENT_INDEX, deserializer, mgr);
    else
      return MaybeComponent();
  }

  bool deserialize_component_typeless(ComponentRef &comp, const DeserializerCb &deserializer, ecs::EntityManager &mgr)
  {
    auto componentTypes = g_ecs_data->getComponentTypes();
    ecs::component_index_t cidx = comp.getComponentId();
    type_index_t typeId = comp.getTypeId();
    component_type_t userType = comp.getUserType();
    ComponentSerializer *typeIO = nullptr;
    if (cidx != ecs::INVALID_COMPONENT_INDEX)
      typeIO = g_ecs_data->getDataComponents()->getTypeIo(cidx);
    auto componentTypeInfo = componentTypes->getComponentData(typeId);
    if (!typeIO && has_io(componentTypeInfo->flags))
      typeIO = componentTypeInfo->serializer;
    //mgr.trackComponentIndex(comp.getComponentId(), EntityManager::TrackComponentOp::WRITE, "deserialize");
    if (typeIO)
    {
      //const bool isBoxed = (componentTypeInfo.flags & COMPONENT_TYPE_BOXED) != 0;
      return typeIO->deserialize(deserializer, (void *)comp.getRawData(), componentTypeInfo->size,
                                 userType, &mgr);
    }
    else if (is_pod(componentTypeInfo->flags))
      return deserializer.read((void *)comp.getRawData(), componentTypeInfo->size * CHAR_BIT, userType);
    else
    {
      LOGE("Attempt to deserialize type {:#X} {}<{}>, which has no typeIO and not pod", userType, typeId,
             componentTypes->getComponentData(typeId)->name.data());
      return false;
    }
  }
}


