#include "ecs/EntityManager.h" // component.h included through this
#include "ecs/ComponentTypesDefs.h"
#include "ecs/ComponentRef.h"


namespace ecs {

  bool Component::operator==(const Component &a) const {
    return getComponentRef() == a.getComponentRef();
  }

  bool Component::operator==(const ComponentRef &a) const {
    return getComponentRef() == a;
  }

  Component::Component(void *data, component_type_t type, type_index_t compIndex, uint32_t size) {
    componentTypeSize = size;
    componentTypeIndex = compIndex;
    componentType = type;
    value = data;
  }

  void Component::free() {
    if (DAGOR_LIKELY(componentType != 0)) {
      const auto typeInfo = g_ecs_data->componentTypes.getComponentData(componentTypeIndex);
      if (need_constructor(typeInfo->flags)) {
        ComponentTypeManager *tm = g_ecs_data->componentTypes.getCTM(componentTypeIndex);
        if (getRawData())
          tm->destroy(getRawData());
      }
      ::free(value);
    } else {
      reset();
    }
  }

  void Component::initTypeIndex(component_type_t component_type) {
    componentType = component_type;
    componentTypeIndex = g_ecs_data->componentTypes.findType(componentType);
  }

  void Component::setRaw(component_type_t component_type, const void *raw_data) {
    if (DAGOR_UNLIKELY(component_type == 0)) {
      reset();
      return;
    }
    initTypeIndex(component_type);
    const auto typeInfo = g_ecs_data->componentTypes.getComponentData(componentTypeIndex);
    componentTypeSize = typeInfo->size;

    value = malloc(componentTypeSize);
    if (is_pod(typeInfo->flags))
      memcpy(value, raw_data, componentTypeSize); // POD is like a float or something
    else if (need_constructor(typeInfo->flags)) // we will need copy constructor
    {
      ComponentTypeManager *tm = g_ecs_data->componentTypes.getCTM(componentTypeIndex);

      // proper copying is important!! this will tell us what failed to copy
      if (!tm->copy(getRawData(), raw_data, componentTypeIndex)) {
        EXCEPTION("Copy failed for component index {}", componentTypeIndex);
      }
    }
  }

  Component &Component::operator=(const Component &a) {
    if (DAGOR_UNLIKELY(this == &a))
      return *this;
    free();
    setRaw(a.getUserType(), a.getRawData());
    return *this;
  }

  const ComponentRef Component::getComponentRef() const {
    return ComponentRef(const_cast<void *>(getRawData()), getUserType(), getTypeId(), this->componentTypeSize);
  }

  //void Component::createCopy(void *data, ComponentTypes *types) {
  //  this->getComponentRef().createCopy(data, types);
  //}




  ComponentRef::ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint32_t size) {
    componentTypeSize = size;
    componentTypeIndex = compIndex;
    componentType = type;
    value = data;
  }

  ComponentRef::ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint32_t size,
                             component_index_t cidx) {
    componentTypeSize = size;
    componentTypeIndex = compIndex;
    componentType = type;
    value = data;
    componentId = cidx;
  }

  bool ComponentRef::operator==(const ComponentRef &a) const {
    const component_index_t typeId = getTypeId();
    if (typeId != a.getTypeId())
      return false;
    auto ctm = g_ecs_data->getComponentTypes()->getCTM(typeId);
    if (ctm) {
      return ctm->is_equal(value, a.value);
    }
    return memcmp(value, a.value, componentTypeSize) == 0;
  }

  void ComponentRef::createCopy(void *data, ComponentTypes *types, ecs::EntityManager *mgr, EntityId eid, component_index_t cidx) const {
    if (!types)
      types = g_ecs_data->getComponentTypes();
    const auto typeInfo = types->getComponentData(componentTypeIndex);
    if (this->value) {
      if (is_pod(typeInfo->flags))
        memcpy(data, value, componentTypeSize); // POD is like a float or something
      else if (need_constructor(typeInfo->flags)) // we will need copy constructor
      {
        ComponentTypeManager *ctm = types->getCTM(componentTypeIndex);
        // proper copying is important!! this will tell us what failed to copy
        if (!ctm->copy(data, this->value, componentTypeIndex)) {
          EXCEPTION("Copy failed for component index {}", componentTypeIndex);
        }
      }
    } else // data not initialized, so instead we just initialize the data with our type
    {
      if (is_pod(typeInfo->flags))
        memset(data, 0, componentTypeSize);
      else if (need_constructor(typeInfo->flags)) // we will need copy constructor
      {
        ComponentTypeManager *ctm = types->getCTM(componentTypeIndex);
        G_ASSERT(ctm);
        ctm->create(data, *mgr, eid, cidx);
      }
    }

  }

  void ComponentRef::destructCopy(void *data, ComponentTypes *types) const {
    if (!types)
      types = g_ecs_data->getComponentTypes();
    const auto typeInfo = types->getComponentData(componentTypeIndex);
    // we only need to clean up the data, so we dont do anything with POD data.
    if (need_constructor(typeInfo->flags)) // we dont own the data
    {
      ComponentTypeManager *ctm = types->getCTM(componentTypeIndex);
      G_ASSERT(ctm);
      //LOG("Destructing a %s\n", typeInfo->name.data());
      //std::cout << std::flush;
      ctm->destroy(data);
    }
  }

  void ComponentRef::print(void *ext_data, ComponentTypes *types) const {
    if (!ext_data)
      return;
    if (!types)
      types = g_ecs_data->getComponentTypes();
    auto ctm = types->getCTM(this->componentTypeIndex);
    LOG("{}", ctm->toString(ext_data));
  }

  void ComponentRef::print(ComponentTypes *types) const {
    print(this->value, types);
  }

  std::string ComponentRef::toString(void *ext_data, ComponentTypes *types) const {
    if (!ext_data)
      return "<NULLPTR>";
    if (!types)
      types = g_ecs_data->getComponentTypes();
    auto ctm = types->getCTM(this->componentTypeIndex);
    return ctm->toString(ext_data);
  }

  std::string ComponentRef::toString(ComponentTypes *types) const {
    return this->toString(this->value, types);
  }
}
