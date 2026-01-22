#ifndef MYEXTENSION_ENTITYCOMPONENTS_H
#define MYEXTENSION_ENTITYCOMPONENTS_H
#include "ecs/component.h"

namespace ecs
{

  /// a ComponentRef functions as a Component that doesnt hold ownership
  class ComponentRef
  {
  public:

    ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint32_t size);
    ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint32_t size, component_index_t cidx);

    /// makes this a reference to a particular component
    /// has no knowledge of component lifetime, so the pointer can become undefined
    /// doesnt store a reference to a component, only to a components data
    /// considnering all components store data as pointers, a ref will exist as long as the base component was not destroyed, even if it was moved or copied
    explicit ComponentRef(Component &c) : ComponentRef(c.value, c.componentType, c.componentTypeIndex, c.componentTypeSize) {}

    // WE DONT OWN DATA
    ComponentRef() = default;
    ComponentRef(ComponentRef &&ref) = default;
    ComponentRef(ComponentRef &ref) = default;
    ComponentRef &operator=(ComponentRef &&ref) = default;
    ComponentRef &operator=(ComponentRef const &ref) = default;
    ~ComponentRef() = default;

    void createCopy(void *data, ComponentTypes *types, ecs::EntityManager *mgr, EntityId eid=INVALID_ENTITY_ID, component_index_t cidx=INVALID_COMPONENT_INDEX) const;
    void destructCopy(void *data, ComponentTypes *types) const;


    bool operator==(const ComponentRef &a) const;

    bool operator!=(const ComponentRef &a) const { return !(*this == a); }

    [[nodiscard]] component_type_t getUserType() const { return componentType; }

    [[nodiscard]] type_index_t getTypeId() const { return componentTypeIndex; }

    component_index_t getComponentId() const { G_ASSERT(this->componentId != INVALID_COMPONENT_INDEX); return this->componentId; }
    const void *getRawData() const { return this->value; }

    std::string toString(void *ext_data, ComponentTypes *types) const;
    std::string toString(ComponentTypes *types) const;

    void print(void *ext_data, ComponentTypes *types) const; // prints external data
    void print(ComponentTypes *types) const; // prints internal data

    inline uint32_t getSize() const { return this->componentTypeSize;}
    template<typename T>
    T *getTypedData() const {
      G_STATIC_ASSERT(this->componentType == ComponentTypeInfo<T>::type);
      return (T*)value;
    }

    template<typename T>
    T *getTypedData() {
      G_STATIC_ASSERT(this->componentType == ComponentTypeInfo<T>::type);
      return (T*)value;
    }

    template<typename T>
    [[nodiscard]] bool is() const {
      return componentType == ComponentTypeInfo<T>::type;
    }

    template<typename T>
    const T *getNullable() const {
      return is<T>() ? (const T *) getTypedData<T>() : nullptr;
    }

    template<typename T>
    T *getNullable() {
      return is<T>() ? (T *) getTypedData<T>() : nullptr;
    }

    bool isNull() const { return componentType == 0; }

    void reset()
    {
      value = nullptr;
      componentType = 0;
      componentTypeSize = 0;
      componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;
    }
  private:
    void *value = nullptr;
    component_type_t componentType = 0;
    type_index_t componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;
    uint32_t componentTypeSize = 0;
    component_index_t componentId = INVALID_COMPONENT_INDEX;
  };



}

#endif //MYEXTENSION_ENTITYCOMPONENTS_H
