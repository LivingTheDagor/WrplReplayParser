

#ifndef WTFILEUTILS_QUERY_H
#define WTFILEUTILS_QUERY_H
#include "dag_generationRefId.h"
#include <EASTL/fixed_function.h>

namespace dag {
  template <typename T> // emulates dag::ConstSpan
  class ConstSpan : public std::span<const T> {
    using base_type = std::span<const T>;
    using size_type = base_type::size_type;
  public:
    ConstSpan(const T * ptr, size_type size) : base_type(const_cast<T*>(ptr), size) {}
    ConstSpan(std::vector<T>::const_iterator it, size_type size) : base_type(it.base(), size) {}

  };
}
namespace ecs {
  class QueryIdDummy {};

  // while I dont really want to use this im also too lazy to update dagor engine code I copied
  typedef GenerationRefId<8, QueryIdDummy> QueryId;
  //typedef uint32_t QueryId;
  enum
  {
    CDF_OPTIONAL = 1, // component is optional and might be absent
  };

  struct ComponentDesc {
    component_t name = 0;
    component_type_t type = 0;
    uint32_t flags = 0; // bitmask of CDFlags enum
    const char *nameStr = nullptr;

    constexpr ComponentDesc(const HashedConstString n, component_type_t tp, uint32_t f = 0) :
        name(n.hash),
        type(tp),
        flags(f),
        nameStr(n.str) {}
  };

  // The query system  has been HEAVILY optimized so I am still having issue understanding it
  // I still haven't actually figured out how it checks if a specific archetype is compatible with a query

  struct BaseQueryDesc {
    dag::ConstSpan<ComponentDesc> componentsRW; // the components the action of this query will read from / write to (RW; ReadWrite)
    dag::ConstSpan<ComponentDesc> componentsRO; // the components the action of this query wil read from (RO; ReadOnly)
    // current, RW and RO are functionally the same internally, I dont have 'tracked' support
    dag::ConstSpan<ComponentDesc> componentsRQ; // the components this query requires exists (RQ; Required)
    // while it may seem like RQ is not needed when we have RO, RQ can just contain 'sanity check' components that confirms your only acting on the required entity
    // REMEMBER, the query action WILL NOT KNOW what entity / template its action on, it will only be given a list of components!!!
    // furthermore, in Event generation scripts, RW and RO are expected to be actual inputs to the query function, but RQ are not
    dag::ConstSpan<ComponentDesc> componentsNO; // the components that should not exist for a template this query acts on (NO; no fucking idea)

    BaseQueryDesc(dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro, dag::ConstSpan<ComponentDesc> comps_rq,
                  dag::ConstSpan<ComponentDesc> comps_no) :
        componentsRW(comps_rw), componentsRO(comps_ro), componentsRQ(comps_rq), componentsNO(comps_no)
    {}
    static inline bool equal_list(dag::ConstSpan<ComponentDesc> a, dag::ConstSpan<ComponentDesc> b)
    {
      if (a.size() != b.size())
        return false;
      for (uint32_t i = 0, e = (uint32_t)a.size(); i != e; ++i)
        if (
            a[i].name != b[i].name || a[i].flags != b[i].flags ||
            (a[i].type != b[i].type && a[i].type != ComponentTypeInfo<auto_type>::type && b[i].type != ComponentTypeInfo<auto_type>::type))
          return false;
      return true;
    }
    bool operator==(const BaseQueryDesc &b) const
    {
      return equal_list(componentsRW, b.componentsRW) && equal_list(componentsRO, b.componentsRO) &&
             equal_list(componentsRQ, b.componentsRQ) && equal_list(componentsNO, b.componentsNO);
    }
  };

  template <unsigned N>
  constexpr int comp_hash_index_of(const ComponentDesc (&arr)[N], const component_t n, int i = 0)
  {
    return i >= N ? -1 : arr[i].name == n ? i : comp_hash_index_of(arr, n, i + 1);
  }

  template <int N>
  struct ComponentSystemIndex
  { // just functions as a sanity check, you pass in returned value (probably return from comp_hash_index_of)
      // and the static assert ensures that the 'value' is valid
    static constexpr int value = N;
    static_assert(N >= 0, "component not found");
  };

#define ECS_QUERY_COMP_INDEX(carr, name) ecs::ComponentSystemIndex<ecs::comp_hash_index_of(carr, ECS_HASH(name).hash)>::value

#define ECS_QUERY_COMP_RO_INDEX(carr, name) ((uint16_t)(ECS_QUERY_COMP_INDEX(carr, name)))
#define ECS_QUERY_COMP_RW_INDEX(carr, name) ((uint16_t)(ECS_QUERY_COMP_INDEX(carr, name)))
  struct NamedQueryDesc : public BaseQueryDesc {
    const char *name = nullptr; // name of this entity system, must be unique (dunno why it needs to be yet)
    NamedQueryDesc(const char *n, dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro,
                   dag::ConstSpan<ComponentDesc> comps_rq, dag::ConstSpan<ComponentDesc> comps_no) :
        BaseQueryDesc(comps_rw, comps_ro, comps_rq, comps_no), name(n)
    {}
  };

  struct CompileTimeQueryDesc : public NamedQueryDesc
  {
    CompileTimeQueryDesc(const char *n, dag::ConstSpan<ComponentDesc> comps_rw, dag::ConstSpan<ComponentDesc> comps_ro,
                         dag::ConstSpan<ComponentDesc> comps_rq, dag::ConstSpan<ComponentDesc> comps_no) :
        NamedQueryDesc(n, comps_rw, comps_ro, comps_rq, comps_no)//, quant(def_quant)
    {
      next = tail;
      tail = this;
    }
    QueryId getHandle() const { return query; }
    //uint32_t getQuant() const { return quant; }
    //void setQuant(uint16_t q) { quant = q; }

  protected:
    QueryId query;
    //uint32_t quant = 0;
    friend class EntityManager;
    CompileTimeQueryDesc *next = NULL; // slist
    static CompileTimeQueryDesc *tail;
  };


  struct CopyQueryDesc
  {
    std::string name;
    std::vector<ComponentDesc> components;
    uint8_t requiredSetCount = 0, rwCnt = 0, roCnt = 0, rqCnt = 0, noCnt = 0;
    // 3 bytes of padding. add is_eid_query?
    uint16_t rwEnd() const { return rwCnt; }
    uint16_t roEnd() const { return rwEnd() + roCnt; }
    uint16_t rqEnd() const { return roEnd() + rqCnt; }
    uint16_t noEnd() const { return rqEnd() + noCnt; }
    CopyQueryDesc() = default;
    const char *getName() const { return name.c_str(); }
    void clear() {}
    void init(const EntityManager &mgr, const char *name_, const BaseQueryDesc &d);
    const BaseQueryDesc getDesc() const;
  };

  inline const BaseQueryDesc CopyQueryDesc::getDesc() const
  {
    return BaseQueryDesc(dag::ConstSpan<ComponentDesc>(components.begin(), rwCnt),
                         dag::ConstSpan<ComponentDesc>(components.begin() + rwEnd(), roCnt),
                         dag::ConstSpan<ComponentDesc>(components.begin() + roEnd(), rqCnt),
                         dag::ConstSpan<ComponentDesc>(components.begin() + rqEnd(), noCnt));
  }
}



#endif //WTFILEUTILS_QUERY_H
