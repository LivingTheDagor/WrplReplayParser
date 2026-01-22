

#ifndef MYEXTENSION_DATACOMPONENTS_H
#define MYEXTENSION_DATACOMPONENTS_H

#include "ecs/ComponentTypes.h"
#include "StringTableAllocator.h"

namespace ecs {
  struct DataComponents;
#define ECS_AUTO_REGISTER_COMPONENT_BASE(type_name_, name, serializer)                                                         \
  static ecs::CompileComponentRegister MAKE_UNIQUE(ccm_rec_, __LINE__)(ECS_HASH(name), ecs::ComponentTypeInfo<type_name_>::type_name, \
    ecs::ComponentTypeInfo<type_name_>::type, serializer);

#define ECS_AUTO_REGISTER_COMPONENT(type_name, name, serializer) \
  ECS_AUTO_REGISTER_COMPONENT_BASE(type_name, name, serializer)

  struct CompileComponentRegister {
    CompileComponentRegister(const HashedConstString name_, const char *type_name_, component_type_t type_,
                             ComponentSerializer *io_) :
        name(name_), type_name(type_name_), io(io_), type(type_) {
      next = tail;
      tail = this;
    }

  protected:
    CompileComponentRegister(const CompileComponentRegister &) = delete;

    CompileComponentRegister &operator=(const CompileComponentRegister &) = delete;

    HashedConstString name = {nullptr, 0};
    const char *type_name = 0;
    ComponentSerializer *io = nullptr;
    CompileComponentRegister *next = NULL; // slist
    static CompileComponentRegister *tail;
    component_type_t type = 0;
    friend struct DataComponents;
  };
  // we dont care about dependency bullshit, we trust in gaijin to have done validation themselves
  struct DataComponent {
    enum
    {
      DONT_REPLICATE = 0x01,
      IS_COPY = 0x02,
      HAS_SERIALIZER = 0x04,
      TYPE_HAS_CONSTRUCTOR = 0x08
    };
    //enum Flags : uint8_t {
    //  Replicated = 1 // if this is 'replicated', it is data sent over the network
    //};
    component_t hash;
    type_index_t componentIndex; // index of component it represents
    component_type_t componentHash;
    ComponentSerializer *serializer;

    component_flags_t flags;
    std::string_view getName() const;
    DataComponent(component_t hash, type_index_t x1, component_type_t x2, ComponentSerializer * io, DataComponents * own, uint32_t name_addr, component_flags_t flags)
    {
      this->hash = hash;
      componentIndex = x1;
      componentHash = x2,
          serializer = io;
      owner = own;
      name_index = name_addr;
      this->flags = flags;
    }
  protected:
    friend DataComponents;
    DataComponents * owner;
    uint32_t name_index;

  };

  struct DataComponents {
    inline component_index_t getIndex(component_t hash) const {
      auto x = componentIndex.find(hash);
      if (x != componentIndex.end()) {
        return x->second;
      }
      return INVALID_COMPONENT_INDEX;
    }

    inline const DataComponent *getDataComponent(component_index_t index) const {
      if (index == INVALID_COMPONENT_INDEX || index >= components.size())
        return nullptr;
      return &components[index];
    }
    inline DataComponent *getDataComponent(component_index_t index) {
      if (index == INVALID_COMPONENT_INDEX || index >= components.size())
        return nullptr;
      return &components[index];
    }

    inline const DataComponent *getDataComponent(component_t hash) const { return getDataComponent(getIndex(hash)); }
    inline DataComponent *getDataComponent(component_t hash) { return getDataComponent(getIndex(hash)); }


    inline ComponentSerializer *getTypeIo(component_index_t index) {
      auto dc = getDataComponent(index);
      if (dc)
        return dc->serializer;
      return nullptr;
    }
    inline ComponentSerializer *getTypeIo(component_index_t index) const {
      auto dc = getDataComponent(index);
      if (dc)
        return dc->serializer;
      return nullptr;
    }

    inline ComponentSerializer *getTypeIo(component_t hash) { return getTypeIo(getIndex(hash)); }

    inline std::string_view getName(component_index_t index) const{
      auto dc = getDataComponent(index);
      if (dc)
      {
        //G_ASSERT(dc->name_index>0);
        return {this->names.getDataRawUnsafe(dc->name_index)};
      }
      return "";
    }

    inline std::string_view getName(const DataComponent *cmp) {
      //G_ASSERT(cmp->name_index>0);
      return this->names.getDataRawUnsafe(cmp->name_index);
    }

    inline std::string_view getName(component_t hash) const { return getName(getIndex(hash)); }

    component_index_t createComponent(HashedConstString name_, type_index_t component_type, ComponentSerializer *io, ComponentTypes &types);

    void initialize(ComponentTypes &types);

    inline size_t size() {return components.size();}

  protected:
    std::vector<DataComponent> components;
    std::unordered_map<component_t, component_index_t> componentIndex; // map of name hash to index in components
    StringTableAllocator names{13, 13};
    friend EntityManager;
    friend DataComponent;

  };
} // ecs
#endif //MYEXTENSION_DATACOMPONENTS_H
