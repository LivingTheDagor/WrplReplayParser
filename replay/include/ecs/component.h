

#ifndef MYEXTENSION_COMPONENT_H
#define MYEXTENSION_COMPONENT_H

#include "ecs/typesAndLimits.h"
#include <optional>
#include "utils.h"
#include "ComponentTypes.h"

namespace ecs {
  constexpr int MAX_STRING_LENGTH = 32768;
  class ComponentRef;
  class Component {
  public:
    // TODO
    Component() = default;

    Component(std::nullptr_t) : Component() {}

    Component(void *data, component_type_t type, type_index_t compIndex, uint32_t size);

    Component(Component &&a) noexcept;
    Component(const Component &a) : Component() { *this = a; }
    const ComponentRef getComponentRef() const; // this is const reference! you should write to it!
    // creates a copy of the internal data in the provided ptr

    template <typename T>
    Component(const T &t)
    {
      (*this) = t;
    }
    template <typename T, typename = std::enable_if_t<std::is_rvalue_reference<T &&>::value, void>>
    Component(T &&t) noexcept
    {
      (*this) = std::move(t);
    }



    bool operator==(const Component &a) const;
    bool operator!=(const Component &a) const { return !(*this == a); }
    bool operator==(const ComponentRef &a) const;

    Component &operator=(Component &&a) noexcept; // move constructor

    Component &operator=(const Component &a); // copy constructor

    Component &operator=(std::nullptr_t) {
      G_ASSERT(isNull());
      return *this;
    }

    template<typename T>
    T *getTypedData() const {
      G_ASSERT(this->componentType == ComponentTypeInfo<T>::type);
      return (T*)value;
    }

    template<typename T>
    T *getTypedData() {
      G_ASSERT(this->componentType == ComponentTypeInfo<T>::type);
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

    const char *getOr(const char *def) const;

    //Component &operator=(const char *v);

    template <typename T>
    T &getRW()
    {
      G_ASSERTF(is<T>() && !isNull(), "{:#x} != {:#x}", +ComponentTypeInfo<T>::type, componentType);
      return *(T *)getTypedData<T>();
    }

    template <typename T>
    Component &operator=(const T &v)
    {
      G_ASSERT_RETURN(is<T>() || isNull(), *this);
      if (isNull())
      {
        initTypeIndex(ComponentTypeInfo<T>::type);
        componentTypeSize = ComponentTypeInfo<T>::size;

        // value.data = (void*)(new typename std::remove_reference<T>::type(v));
        // do it same way, as free in BoxedCreator, as data can be MOVED to it. may be make allocator part of type to remove it?
        value = malloc(sizeof(T));
        //if (!std::is_scalar<T>::value)
        //  memset(value, 0, sizeof(T));
        new (value) typename std::remove_reference<T>::type(v);


      }
      else
        getRW<typename std::remove_reference<T>::type>() = v;
      return *this;
    }

    //inline void createCopy(void *data, ComponentTypes *types);


    // dunno what this is doing this is something gaijin made and I copied
    template<class T, typename U = ECS_MAYBE_VALUE_T(T)>
    U getOr(const T &def) const {
      static_assert(!std::is_same<T, ecs::string>::value,
                    "Generic \"getOr\" is not strings, use non-generic method for that");
      if (isNull())
        return def;
      G_ASSERT_RETURN(is<T>(), def);
      return *(const T *) getTypedData<T>();
    }

    [[nodiscard]] component_type_t getUserType() const { return componentType; }

    [[nodiscard]] type_index_t getTypeId() const { return componentTypeIndex; }
    //[[nodiscard]] component_index_t getComponentId() const { return componentId; }

    inline const void *getRawData() const { return value; }

    inline void *getRawData() { return value; }

    inline uint32_t getSize() { return componentTypeSize;}

    ~Component() {
      free();
    }

    void reset()
    {
      value = nullptr;
      componentType = 0;
      componentTypeSize = 0;
      componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;
    }

  protected:
    friend ComponentRef;
    friend EntityManager;
    void setRaw(component_type_t component_type, const void *raw_data);
    void initTypeIndex(component_type_t component_type);
    void free();


    void *value = nullptr; // everything is stored as a ptr, fuck the inplace shit
    component_type_t componentType = 0;
    type_index_t componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;
    uint32_t componentTypeSize = 0;
  };

//bool Component::operator==(const EntityComponentRef &a) const { return getEntityComponentRef() == a; }
  inline Component &Component::operator=(Component &&a) noexcept {
    if (DAGOR_UNLIKELY(this == &a))
      return *this;
    free();
    G_STATIC_ASSERT(offsetof(Component, value) == 0);
    //G_STATIC_ASSERT(
    //    sizeof(Value) >= sizeof(uintptr_t)); // boxed components should return rawData to something boxed, i.e. buffer
    memcpy(this, &a, sizeof(Component));
    a.reset();
    return *this;
  }

  inline Component::Component(Component &&a) noexcept
  {
    memcpy(this, &a, sizeof(*this));
    a.reset();
  }


  typedef std::optional<Component> MaybeComponent;
} // ecs
#endif //MYEXTENSION_COMPONENT_H
