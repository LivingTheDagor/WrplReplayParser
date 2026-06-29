#ifndef MYEXTENSION_COMPONENTSMAP_H
#define MYEXTENSION_COMPONENTSMAP_H

#include "ecs/typesAndLimits.h"
#include "ecs/Component.h"
#include "ecs/DataComponents.h"
#include "dag_assert.h"
#include <map>
namespace ecs {

  using BaseComponentsMap = std::map<component_t, Component>;

  class ComponentsMap : protected BaseComponentsMap {
    friend class EntityManager;

  public:
    typedef BaseComponentsMap base_type;
    using base_type::empty;
    size_t size() const { return base_type::size(); }
    base_type::iterator begin() { return base_type::begin(); }
    base_type::iterator end() { return base_type::end(); }
    base_type::const_iterator begin() const { return base_type::begin(); }
    base_type::const_iterator end() const { return base_type::end(); }
    void swap(ComponentsMap &other) { base_type::swap(other); }

    base_type::iterator find_as(const component_t c) { return base_type::find(c); }
    [[nodiscard]] base_type::const_iterator find_as(const component_t c) const { return base_type::find(c); }
    base_type::iterator find_as(const HashedConstString &s) { return find_as(s.hash); }
    [[nodiscard]] base_type::const_iterator find_as(const HashedConstString &s) const { return find_as(s.hash); }

    bool operator==(const ComponentsMap &a) const {
      return static_cast<const base_type &>(*this) == static_cast<const base_type &>(a);
    }

    template<class IteratorType>
    static hash_str_t getHash(const IteratorType &it) {
      return it->first;
    }

    template<class IteratorType>
    static component_index_t getComponentIndex(const DataComponents &dataComponents, const IteratorType &it) {
      return dataComponents.getIndex(getHash(it));
    }

    Component &operator[](const component_t name_hash) { return base_type::operator[](name_hash); }
    Component &operator[](const HashedConstString &name) { return base_type::operator[](name.hash); }

    [[nodiscard]] const Component *find(const component_t name) const {
      auto it = base_type::find(name);
      if (it != base_type::end())
        return &it->second;
      return nullptr;
    }
    [[nodiscard]] const Component *find(const HashedConstString &name) const { return find(name.hash); }

    // For const operator[], throw or assert if not found (std::map behavior is to create if not found, but for const,
    // you must check)
    const Component &operator[](const HashedConstString &name) const {
      auto it = base_type::find(name.hash);
      if (it != base_type::end())
        return it->second;
      throw std::out_of_range("Component not found");
    }

    // Warning: SLOW operation, hashes string at runtime
    base_type::iterator find_as(const char *s) { return base_type::find(ECS_HASH_SLOW(s).hash); }
    Component &operator[](const char *name) { return this->operator[](ECS_HASH_SLOW(name)); }
    const Component &operator[](const char *name) const { return this->operator[](ECS_HASH_SLOW(name)); }
    // using base_type::shrink_to_fit; // Not available in std::map, can omit or handle differently
  };

  struct InitializerNode {
    Component second;
    component_t name;
    component_index_t cIndex; // initialized once during validation
  };

  typedef std::vector<InitializerNode> BaseComponentsInitializer;

  class ComponentsInitializer : protected BaseComponentsInitializer {
  public:
    typedef BaseComponentsInitializer base_type;
    using base_type::empty;
    size_t size() const { return base_type::size(); }
    base_type::iterator begin() { return base_type::begin(); }
    base_type::iterator end() { return base_type::end(); }
    base_type::const_iterator begin() const { return base_type::begin(); }
    base_type::const_iterator end() const { return base_type::end(); }

    Component &operator[](const HashedConstString name); // always create new value
    // Component& operator[](const char* name) { return this->operator[](ECS_HASH_SLOW(name)); }
    template<class IteratorType>
    static hash_str_t getHash(const IteratorType &it) {
      return it->name;
    }

    template<class IteratorType>
    static component_index_t getComponentIndex(const DataComponents &dataComponents, const IteratorType &it) {
      return dataComponents.getIndex(it->name);
    }

    template<class ComponentType>
    bool insert(const component_t name, const FastGetInfo &lt, ComponentType &&val, const char *nameStr);
    void reserve(size_t sz) { base_type::reserve(sz); }

    // explicit
    // todo: should be explicit, but we keep it implicit during transition
    // explicit ComponentsInitializer(ComponentsMap &&map);
    // Rule-of-5
    ComponentsInitializer() = default;
    ComponentsInitializer(size_t sz) { reserve(sz); }
    ComponentsInitializer(const ComponentsInitializer &) = default;
    ComponentsInitializer(ComponentsInitializer &&) = default;
    ComponentsInitializer &operator=(const ComponentsInitializer &) = default;
    ComponentsInitializer &operator=(ComponentsInitializer &&) = default;
    ~ComponentsInitializer() = default;

    friend class EntityManager;
  };

  /*inline const Component *ComponentsMap::find(const component_t name) const
  {
    auto it = find_as(name);
    if (it == base_type::end())
      return NULL;
    return &it->second;
  }

  inline const Component &ComponentsMap::operator[](const HashedConstString name) const
  {
    auto it = find_as(name);
    if (it == base_type::end())
      return Component::get_null();
    return it->second;
  }*/

  template<class ComponentType>
  inline bool ComponentsInitializer::insert(const component_t name, const FastGetInfo &lt, ComponentType &&val,
                                            const char *nameStr) {
    if (DAGOR_UNLIKELY(!lt.isValid()))
      return false;
    G_UNUSED(nameStr);
    base_type::emplace_back(InitializerNode{Component(std::forward<ComponentType>(val)), name, lt.getCidx()});
    return true;
  }

  inline Component &ComponentsInitializer::operator[](const HashedConstString name) {
    // LOG("ComponentsInitializer %s<0x%X>\n", name.str, name.hash);
    auto val = name.hash;
    base_type::emplace_back(InitializerNode{Component(), val, INVALID_COMPONENT_INDEX});
    return base_type::back().second;
  }

  // inline ComponentsInitializer::ComponentsInitializer(ComponentsMap &&map)
  //{
  //   for (auto it = map.begin(); it != map.end(); ++it)
  //   {
  //     auto val = it->first;
  //     base_type::emplace_back(InitializerNode{std::move(it->second), val, INVALID_COMPONENT_INDEX});
  //   }
  // };
} // namespace ecs

#endif // MYEXTENSION_COMPONENTSMAP_H
