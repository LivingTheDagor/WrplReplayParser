

#ifndef MYEXTENSION_COMPONENTTYPES_H
#define MYEXTENSION_COMPONENTTYPES_H

#include <cstring>
#include <string_view>
#include <vector>
#include "unordered_map"
#include <climits>


#include "ecs/typesAndLimits.h"
#include "ecs/entityId.h"
#include "ecs/ecsHash.h"


#include <sstream>
#include "dag_assert.h"
#include "EASTL/string.h"

namespace ecs {
#define CONCATENATE(x, y) x##y
#define MAKE_UNIQUE(x, y) CONCATENATE(x, y)

#define ECS_MAYBE_VALUE(T)   (std::is_scalar<T>::value || std::is_same<T, ecs::EntityId>::value)
#define ECS_MAYBE_VALUE_T(T) typename std::conditional<ECS_MAYBE_VALUE(T), T, const T &>::type

  class Component;

  template<typename T>
  struct ComponentTypeInfo;
  typedef std::nullptr_t auto_type; // this is to be used with REQUIRE/REQUIRE_NOT only
  template<>
  struct ComponentTypeInfo<auto_type> {
    static constexpr component_type_t type = ECS_HASH("ecs::auto_type").hash;
  };

  struct Tag {
  };
#define ECS_TAG_NAME "ecs::Tag"
  template<>
  struct ComponentTypeInfo<Tag> // tag type traits, so it has 0 size
  {
    static constexpr const char *type_name = ECS_TAG_NAME;
    static constexpr component_type_t type = ECS_HASH(ECS_TAG_NAME).hash;
    static constexpr bool is_pod_class = true;
    static constexpr bool has_creator_class = false;
    static constexpr size_t size = 0;
  };
} // ecs

#include "ecs/ComponentTypeManager.h"
// all types are now inplace
#define ECS_DECLARE_BASE_TYPE(class_type, class_name, has_creator_class_)                                                  \
  namespace ecs                                                                                                           \
  {                                                                                                                       \
  template <>                                                                                                             \
  struct ComponentTypeInfo<class_type>                                                                                    \
  {                                                                                                                       \
    static constexpr const char *type_name = class_name;                                                                  \
    static constexpr component_type_t type = ECS_HASH(class_name).hash;                                                   \
    static constexpr bool is_pod_class = !has_creator_class_;                                                              \
    static constexpr bool has_creator_class = has_creator_class_;                                                        \
    static constexpr size_t size = sizeof(class_type);                                                                    \
  };                                                                                                                      \
  template <>                                                                                                             \
  struct ComponentTypeInfo<const class_type> : ComponentTypeInfo<class_type>                                              \
  {};                                                                                                                     \
  template <>                                                                                                             \
  struct ComponentTypeInfo<class_type &> : ComponentTypeInfo<class_type>                                                  \
  {};                                                                                                                     \
  template <>                                                                                                             \
  struct ComponentTypeInfo<const class_type &> : ComponentTypeInfo<class_type>                                            \
  {};                                                                                                                     \
  }

// this is a type that needs a constructor
#define ECS_DECLARE_CREATABLE_TYPE(class_type)                                                  \
  ECS_DECLARE_BASE_TYPE(class_type, #class_type, true);

// pod has no creator class. IE: the type doesnt need a constructor, like a int or float or TMatrix
#define ECS_DECLARE_POD_TYPE(class_type)                                                        \
  ECS_DECLARE_BASE_TYPE(class_type, #class_type, false);


#define ECS_REGISTER_TYPE_BASE(klass, klass_name, io, creator, destroyer, flags_)                                                   \
  static ecs::CompileComponentTypeRegister MAKE_UNIQUE(ecs_type_rec_, __COUNTER__)(klass_name, ecs::ComponentTypeInfo<klass>::type, \
    ecs::ComponentTypeInfo<klass>::size, io, creator, destroyer,                                                                    \
    ecs::ComponentTypeFlags(                                                                                                             \
    (ecs::ComponentTypeInfo<klass>::has_creator_class ? ecs::COMPONENT_TYPE_NON_TRIVIAL_CREATE : 0) |                                    \
    (ecs::ComponentTypeInfo<klass>::is_pod_class ? ecs::COMPONENT_TYPE_IS_POD : 0) |                                                     \
    (io ? ecs::COMPONENT_TYPE_HAS_IO : 0)))


#define ECS_REGISTER_MANAGED_TYPE(klass, io, klass_manager) \
  G_STATIC_ASSERT(ecs::ComponentTypeInfo<klass>::has_creator_class);                                                          \
  ECS_REGISTER_TYPE_BASE(klass, ecs::ComponentTypeInfo<klass>::type_name, io, (&ecs::CTMFactory<klass_manager>::create), \
    (&ecs::CTMFactory<klass_manager>::destroy),                                                                          \
    ((ecs::HasRequestResources<klass_manager::component_type>::value) ? ecs::COMPONENT_TYPE_NEED_RESOURCES : 0))


#define ECS_REGISTER_CTM_TYPE(kl, io) ECS_REGISTER_MANAGED_TYPE(kl, io, ecs::InplaceCreator<kl>)

// while these are PODS, they still 'have' a ctm that simply makes printing easier
#define ECS_REGISTER_POD_TYPE(klass, io) \
  G_STATIC_ASSERT(ecs::ComponentTypeInfo<klass>::is_pod_class);                                      \
  ECS_REGISTER_TYPE_BASE(klass, #klass, io,                                                          \
  &ecs::CTMFactory<ecs::ReducedCreator<klass>>::create,                                                 \
  &ecs::CTMFactory<ecs::ReducedCreator<klass>>::destroy, 0)

namespace ecs {

  typedef ComponentTypeManager *pComponentTypeManager;

  typedef pComponentTypeManager (*create_ctm_t)();

  typedef void (*destroy_ctm_t)(pComponentTypeManager);


  class SerializerCb {
  public:
    virtual void write(const void *, size_t sz_in_bits, component_type_t hint) = 0;
  };

  class DeserializerCb {
  public:
    virtual bool read(void *, size_t sz_in_bits, component_type_t hint) const = 0;
  };

  inline void write_compressed(SerializerCb &cb, uint32_t v) {
    uint8_t data[sizeof(v) + 1];
    int i = 0;
    for (; i < (int)sizeof(data); ++i) {
      data[i] = uint8_t(v) | (v >= (1 << 7) ? (1 << 7) : 0);
      v >>= 7;
      if (!v) {
        ++i;
        break;
      }
    }
    cb.write(data, (size_t)i * CHAR_BIT, 0);
  }

  inline bool read_compressed(const DeserializerCb &cb, uint32_t &v) {
    v = 0;
    for (int i = 0; i < (int)(sizeof(v) + 1); ++i) {
      uint8_t byte = 0;
      if (!cb.read(&byte, CHAR_BIT, 0))
        return false;
      v |= uint32_t(byte & ~(1 << 7)) << (i * 7);
      if ((byte & (1 << 7)) == 0)
        break;
    }
    return true;
  }

// default serializer is just call as-is.
// if we want we can register different serializer, with quantization and stuff

  class ComponentSerializer {
  public:
    virtual void serialize(SerializerCb &cb, const void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) {
      cb.write(data, sz *
                     CHAR_BIT,
               hint);
    }

    virtual bool deserialize(const DeserializerCb &cb, void *data, size_t sz, component_type_t hint, ecs::EntityManager *mgr) {
      return cb.read(data, sz * CHAR_BIT, hint);
    }
  };


  enum ComponentTypeFlags : uint16_t {
    //COMPONENT_TYPE_TRIVIAL = 0, // basically it is pod
    COMPONENT_TYPE_NON_TRIVIAL_CREATE = 1,
    //COMPONENT_TYPE_NON_TRIVIAL_MOVE = 2,
    // NO BOXED
    //COMPONENT_TYPE_CREATE_ON_TEMPL_INSTANTIATE = 8,
    //COMPONENT_TYPE_NEED_RESOURCES = 16,
    COMPONENT_TYPE_HAS_IO = 32, // that is for optimization, to skip getting io, when io isn't available.
    //COMPONENT_TYPE_TRIVIAL_MASK = COMPONENT_TYPE_HAS_IO - 1,
    COMPONENT_TYPE_IS_POD = 64, // that is just for verification
    COMPONENT_TYPE_REPLICATION = 128,

    COMPONENT_TYPE_LAST_PLUS_1,

  };

  struct CompileComponentTypeRegister {
    CompileComponentTypeRegister(const char *name_, uint32_t name_hash_, uint32_t size_, ComponentSerializer *io_,
                                 create_ctm_t ctm_,
                                 destroy_ctm_t dtm_,
                                 ComponentTypeFlags flags_) :
        name(name_), ctm(ctm_), dtm(dtm_), io(io_), name_hash(name_hash_), size(size_), flags(flags_) {
      next = tail;
      tail = this;
    }

  protected:
    const char *name = nullptr;                   // name of this entity system, must be unique
    CompileComponentTypeRegister *next = nullptr; // slist
    create_ctm_t ctm = nullptr;
    destroy_ctm_t dtm = nullptr;
    ComponentSerializer *io = nullptr;
    uint32_t name_hash;
    uint32_t size;
    ComponentTypeFlags flags;
    static CompileComponentTypeRegister *tail;
    friend struct ComponentTypes;
  };


  inline bool is_pod(ComponentTypeFlags flags) { return (flags & COMPONENT_TYPE_IS_POD) != 0; }

  inline bool has_io(ComponentTypeFlags flags) { return (flags & COMPONENT_TYPE_HAS_IO) != 0; }

  inline bool need_constructor(ComponentTypeFlags flags) {
    return (flags & COMPONENT_TYPE_NON_TRIVIAL_CREATE) != 0;
  }


  struct ComponentInfo {
  public:
    ComponentSerializer *serializer;
    uint32_t size;
    //ComponentTypeFlags flags;
    component_type_t hash;
    std::string_view name; // this name should only be set through preinit, so doesnt need to be a std::string
    ComponentTypeManager *ctm; // the ctm  manages creation and destruction of a component, should only be used if it needs a construction (ie: an int doesnt need one, but a std::string might)
    create_ctm_t create; // used to create an instance of the ctm. the ctm is created on demand, if ctm is null, then this is called
    destroy_ctm_t destroy; // destroys the ctm.
    ComponentTypeFlags flags;
  };

  struct ComponentTypes {
    std::vector<ComponentInfo> types;
    std::unordered_map<component_type_t, type_index_t> typesIndex; // hash to index.


    type_index_t findOr(component_type_t, type_index_t) const;

    type_index_t findType(component_type_t) const;
    void initialize();

  public:
    uint32_t getTypeCount() const { return (uint32_t) types.size(); }


    ~ComponentTypes();

    inline ComponentTypeManager *getCTM(type_index_t index) {
      G_ASSERTF_RETURN(!(this->types.size() <= index || index < 0), nullptr, "Invalid %i index for getCTM", index);
      auto &data = this->types[index];
      if (data.ctm) {
        return data.ctm;
      }
      data.ctm = data.create();
      return data.ctm;
    }

    inline ComponentTypeManager *getCTM(component_type_t hash) {
      auto idx = typesIndex[hash];
      if (idx) {
        return getCTM(idx);
      }
      return nullptr;
    }

    inline std::string_view getName(type_index_t index) {
      G_ASSERTF_RETURN(!(this->types.size() <= index || index < 0), "", "Invalid %i index for getName", index);
      return this->types[index].name;

    }

    inline std::string_view getName(component_t hash) {
      auto idx = typesIndex[hash];
      if (idx) {
        return getName(idx);
      }
      return "#NOT_FOUND#";
    }

    inline const ComponentInfo *getComponentData(type_index_t index) const {
      G_ASSERTF_RETURN(!(this->types.size() <= index || index < 0), nullptr, "Invalid index %i for getComponentData", index);
      return &this->types[index];
    }
    inline ComponentInfo *getComponentData(type_index_t index) {
      G_ASSERTF_RETURN(!(this->types.size() <= index || index < 0), nullptr, "Invalid index %i for getComponentData", index);
      return &this->types[index];
    }

    inline ComponentInfo *getComponentData(component_type_t hash) {
      auto idx = typesIndex[hash];
      if (idx) {
        return getComponentData(idx);
      }
      return nullptr;
    }

    inline const ComponentInfo *getComponentData(component_type_t hash) const {
      auto idx = typesIndex.find(hash);
      if (idx != typesIndex.end()) {
        return getComponentData(idx->second);
      }
      return nullptr;
    }


    uint32_t getComponentsCount() const { return (uint32_t) types.size(); }

    // registers a new component
    type_index_t registerType(const char *name, component_type_t type, uint32_t data_size, ComponentSerializer *io,
                              create_ctm_t ctm, destroy_ctm_t dtm, ComponentTypeFlags flag);


  };

  inline type_index_t ComponentTypes::findOr(component_type_t val, type_index_t ifNotFound) const {
    auto v = typesIndex.find(val);
    if (v == typesIndex.end()) {
      return ifNotFound;
    }
    return v->second;
  }

  inline type_index_t ComponentTypes::findType(component_type_t tp) const {
    return findOr(tp, INVALID_COMPONENT_TYPE_INDEX);
  }

} // ecs





#endif //MYEXTENSION_COMPONENTTYPES_H
