#include "ecs/EntityManager.h"

#include <EASTL/vector_set.h>

namespace ecs {
  static constexpr int max_query_components_count = 256;

  inline void CopyQueryDesc::init(const EntityManager &mgr, const char *name_, const BaseQueryDesc &d) {
    G_UNUSED(mgr);
    components.clear();
    components.reserve(d.componentsRW.size() + d.componentsRO.size() + d.componentsRQ.size() + d.componentsNO.size());
    G_UNUSED(name_);
    static constexpr uint32_t max_cnt = eastl::numeric_limits<decltype(rwCnt)>::max();
    rwCnt = (uint8_t) eastl::min<uint32_t>((uint32_t)d.componentsRW.size(), max_cnt); //-V1029
    roCnt = (uint8_t) eastl::min<uint32_t>((uint32_t)d.componentsRO.size(), max_cnt); //-V1029
    rqCnt = (uint8_t) eastl::min<uint32_t>((uint32_t)d.componentsRQ.size(), max_cnt); //-V1029
    noCnt = (uint8_t) eastl::min<uint32_t>((uint32_t)d.componentsNO.size(), max_cnt); //-V1029
    components.insert(components.end(), d.componentsRW.begin(), d.componentsRW.begin() + rwCnt);
    components.insert(components.end(), d.componentsRO.begin(), d.componentsRO.begin() + roCnt);
    components.insert(components.end(), d.componentsRQ.begin(), d.componentsRQ.begin() + rqCnt);
    components.insert(components.end(), d.componentsNO.begin(), d.componentsNO.begin() + noCnt);
    components.shrink_to_fit();

    if (rwCnt != d.componentsRW.size() || roCnt != d.componentsRO.size() || rqCnt != d.componentsRQ.size() ||
        noCnt != d.componentsNO.size())
      EXCEPTION("query <%s> has too many components (rw=%d,ro=%d,rq=%d,no=%d) max is %d", name_, rwCnt, roCnt, rqCnt,
              noCnt, max_cnt);
    auto getNameStr = [&](const ComponentDesc &cd) { return cd.nameStr; };

    typedef dag::RelocatableFixedVector<component_t, 64, true> fixed_vector_components_t;
    eastl::vector_set<component_t, eastl::less<component_t>, EASTLAllocatorType, fixed_vector_components_t> requiredComponentsSet;
    for (const auto &cd: d.componentsRQ) {
      if (cd.flags & CDF_OPTIONAL)
      EXCEPTION("query <%s> doesn't make sense as component %s(0x%X) is both required and optional", name_,
                getNameStr(cd), cd.name);
      requiredComponentsSet.emplace(cd.name);
    }
    for (const auto &cd: d.componentsRQ) {
      if (cd.flags & CDF_OPTIONAL)
      EXCEPTION("query <%s> doesn't make sense as component %s(0x%X) is both required and optional", name_,
                getNameStr(cd), cd.name);
      requiredComponentsSet.emplace(cd.name);
    }
    for (const auto &cd: d.componentsRO)
      if (!(cd.flags & CDF_OPTIONAL))
        requiredComponentsSet.emplace(cd.name);
    for (const auto &cd: d.componentsRW)
      if (!(cd.flags & CDF_OPTIONAL))
        requiredComponentsSet.emplace(cd.name);
    for (const auto &cd: d.componentsNO) {
      if (cd.flags & CDF_OPTIONAL)
      EXCEPTION("query <%s> doesn't make sense as component %s(0x%X) is both required_not and optional", name_,
                getNameStr(cd), cd.name);
      if (requiredComponentsSet.find(cd.name) != requiredComponentsSet.end())
      EXCEPTION("query <%s> doesn't make sense as component %s(0x%X) is both in required and required_not lists", name_,
                getNameStr(cd),
                cd.name);
    }
    requiredSetCount = static_cast<uint8_t>(eastl::min<uint32_t>(MAX_OPTIONAL_PARAMETERS,
                                                                 eastl::min<uint32_t>(
                                                                     (uint32_t) requiredComponentsSet.size(),
                                                                     max_cnt)));
    if (requiredComponentsSet.size() >= MAX_OPTIONAL_PARAMETERS)
    EXCEPTION(
        "too many optional data components in query <%s>. To fix, increase bitset size or add first-optional-component-no",
        name_);
    G_ASSERT(components.size() <= USHRT_MAX);
    name = name_;
  }


  QueryId EntityManager::createUnresolvedQuery(const NamedQueryDesc &desc)
  {
    G_ASSERT_RETURN(desc.componentsRW.size() + desc.componentsRO.size() < max_query_components_count, QueryId());
    query_components_hash descHash = query_components_hash_calc(desc);
    auto it = queryMap.find(descHash);
    uint32_t index;
    if (it != queryMap.end() && isQueryValidGen(it->second) && queriesReferences[it->second.index()])
    {
      index = it->second.index();

      G_ASSERTF(desc == queryDescs[index].getDesc(), "query %s hashed aliased with different query %s!", desc.name,
      queryDescs[index].getName());
      queriesReferences[index]++;
      return it->second;
    }
    else
    {
      index = addOneQuery();
    }
    queryDescs[index].init(*this, desc.name, desc);

    // ima trust in the process :)
    //for (auto &c : queryDescs[index].components)
    //  c.nameStr = queriesComponentsNames.getName(queriesComponentsNames.addNameId(c.nameStr));

    QueryId ret = QueryId::make(index, queriesGenerations[index]);
    queryMap[descHash] = ret;
    // if (resolvePersistentQueryInternal(index))//recently added
    //   makeArchetypesQuery(0, index, false);
    return ret;
  }


  EntityManager::query_components_hash EntityManager::query_components_hash_calc(const BaseQueryDesc &desc)
  {
    struct
    {
      uint8_t ro, rw, rq, no;
    } details;
    details.rw = (uint8_t)desc.componentsRW.size();
    details.ro = (uint8_t)desc.componentsRO.size();
    details.rq = (uint8_t)desc.componentsRQ.size();
    details.no = (uint8_t)desc.componentsNO.size();
    query_components_hash componentsHash = mem_hash_fnv1<64>((const char *)&details.ro, sizeof(details));
    auto hashComponents = [&componentsHash](dag::ConstSpan<ComponentDesc> list) {
      for (const auto &c : list)
      {
        componentsHash = mem_hash_fnv1<64>((const char *)&c.name, sizeof(c.name), componentsHash);
        componentsHash = mem_hash_fnv1<64>((const char *)&c.flags, sizeof(c.flags), componentsHash);
        componentsHash = mem_hash_fnv1<64>((const char *)&c.type, sizeof(c.type), componentsHash);
      }
    };
    hashComponents(desc.componentsRW);
    hashComponents(desc.componentsRO);
    hashComponents(desc.componentsRQ);
    hashComponents(desc.componentsNO);
    return componentsHash;
  }

  uint32_t EntityManager::addOneQuery()
  {
    // todo: may be keep free list instead?
    auto qi = freeQueriesCount ? eastl::find(queriesReferences.begin(), queriesReferences.end(), 0) : queriesReferences.end();
    if (qi != queriesReferences.end())
    {
      freeQueriesCount--;
      *qi = 1;
      uint32_t index = (uint32_t)std::distance(queriesReferences.begin(), qi);
      //archetypeQueries[index].reset();
      //archetypeEidQueries[index].reset();
      resolvedQueries[index].reset();
      resetQueryStatus(index);
      queryDescs[index].clear();
      return index;
    }
    //archetypeEidQueries.emplace_back();
    //archetypeQueries.emplace_back();
    queryDescs.emplace_back();
    //DAECS_EXT_ASSERT(archetypeQueries.size() == queryDescs.size());
    resolvedQueries.emplace_back();
    //DAECS_EXT_ASSERT(archetypeQueries.size() == resolvedQueries.size());
    addOneResolvedQueryStatus();

    queriesReferences.push_back(1);
    //DAECS_EXT_ASSERT(archetypeQueries.size() == queriesReferences.size());
    queriesGenerations.push_back(currentQueryGen);
    //DAECS_EXT_ASSERT(archetypeQueries.size() == queriesGenerations.size());
    return (uint32_t)resolvedQueries.size() - 1;
  }
}
