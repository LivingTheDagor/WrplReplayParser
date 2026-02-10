#include "ecs/EntityManager.h"
#include <EASTL/vector_set.h>

namespace ecs {
  static constexpr int max_query_components_count = 256;
  EntitySystemDesc *EntitySystemDesc::tail = NULL;
  uint32_t EntitySystemDesc::generation = 0;

  inline void CopyQueryDesc::init(const char *name_, const BaseQueryDesc &d) {
    components.clear();
    components.reserve(d.componentsRW.size() + d.componentsRO.size() + d.componentsRQ.size() + d.componentsNO.size());
    G_UNUSED(name_);
    static constexpr uint32_t max_cnt = eastl::numeric_limits<decltype(rwCnt)>::max();
    rwCnt = (uint8_t) eastl::min<uint32_t>((uint32_t) d.componentsRW.size(), max_cnt); //-V1029
    roCnt = (uint8_t) eastl::min<uint32_t>((uint32_t) d.componentsRO.size(), max_cnt); //-V1029
    rqCnt = (uint8_t) eastl::min<uint32_t>((uint32_t) d.componentsRQ.size(), max_cnt); //-V1029
    noCnt = (uint8_t) eastl::min<uint32_t>((uint32_t) d.componentsNO.size(), max_cnt); //-V1029
    components.insert(components.end(), d.componentsRW.begin(), d.componentsRW.begin() + rwCnt);
    components.insert(components.end(), d.componentsRO.begin(), d.componentsRO.begin() + roCnt);
    components.insert(components.end(), d.componentsRQ.begin(), d.componentsRQ.begin() + rqCnt);
    components.insert(components.end(), d.componentsNO.begin(), d.componentsNO.begin() + noCnt);
    components.shrink_to_fit();

    if (rwCnt != d.componentsRW.size() || roCnt != d.componentsRO.size() || rqCnt != d.componentsRQ.size() ||
        noCnt != d.componentsNO.size())
    EXCEPTION("query <{}> has too many components (rw={},ro={},rq={},no={}) max is {}", name_, rwCnt, roCnt, rqCnt,
              noCnt, max_cnt);
    auto getNameStr = [&](const ComponentDesc &cd) { return cd.nameStr; };

    typedef dag::RelocatableFixedVector<component_t, 64, true> fixed_vector_components_t;
    eastl::vector_set<component_t, eastl::less<component_t>, EASTLAllocatorType, fixed_vector_components_t> requiredComponentsSet;
    for (const auto &cd: d.componentsRQ) {
      if (cd.flags & CDF_OPTIONAL)
      EXCEPTION("query <{}> doesn't make sense as component {}({:#x}) is both required and optional", name_,
                getNameStr(cd), cd.name);
      requiredComponentsSet.emplace(cd.name);
    }
    for (const auto &cd: d.componentsRQ) {
      if (cd.flags & CDF_OPTIONAL)
      EXCEPTION("query <{}> doesn't make sense as component {}({:#x}) is both required and optional", name_,
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
      EXCEPTION("query <{}> doesn't make sense as component {}({:#x}) is both required_not and optional", name_,
                getNameStr(cd), cd.name);
      if (requiredComponentsSet.find(cd.name) != requiredComponentsSet.end())
      EXCEPTION("query <{}> doesn't make sense as component {}({:#x}) is both in required and required_not lists",
                name_,
                getNameStr(cd),
                cd.name);
    }
    requiredSetCount = static_cast<uint8_t>(eastl::min<uint32_t>(MAX_OPTIONAL_PARAMETERS,
                                                                 eastl::min<uint32_t>(
                                                                     (uint32_t) requiredComponentsSet.size(),
                                                                     max_cnt)));
    if (requiredComponentsSet.size() >= MAX_OPTIONAL_PARAMETERS)
    EXCEPTION(
        "too many optional data components in query <{}>. To fix, increase bitset size or add first-optional-component-no",
        name_);
    G_ASSERT(components.size() <= USHRT_MAX);
    name = name_;
  }


  QueryId GState::createUnresolvedQuery(const NamedQueryDesc &desc) {
    G_ASSERT_RETURN(desc.componentsRW.size() + desc.componentsRO.size() < max_query_components_count, QueryId());
    query_components_hash descHash = query_components_hash_calc(desc);
    auto it = queryMap.find(descHash);
    uint32_t index;
    if (it != queryMap.end() && isQueryValidGen(it->second) && queriesReferences[it->second.index()]) {
      index = it->second.index();

      G_ASSERTF(desc == queryDescs[index].getDesc(), "query {} hashed aliased with different query {}!", desc.name,
                queryDescs[index].getName());
      queriesReferences[index]++;
      return it->second;
    } else {
      index = addOneQuery();
    }
    queryDescs[index].init(desc.name, desc);

    // ima trust in the process :)
    //for (auto &c : queryDescs[index].components)
    //  c.nameStr = queriesComponentsNames.getName(queriesComponentsNames.addNameId(c.nameStr));

    QueryId ret = QueryId::make(index, queriesGenerations[index]);
    queryMap[descHash] = ret;
    return ret;
  }


  GState::query_components_hash GState::query_components_hash_calc(const BaseQueryDesc &desc) {
    struct {
      uint8_t ro, rw, rq, no;
    } details;
    details.rw = (uint8_t) desc.componentsRW.size();
    details.ro = (uint8_t) desc.componentsRO.size();
    details.rq = (uint8_t) desc.componentsRQ.size();
    details.no = (uint8_t) desc.componentsNO.size();
    query_components_hash componentsHash = mem_hash_fnv1<64>((const char *) &details.ro, sizeof(details));
    auto hashComponents = [&componentsHash](dag::ConstSpan<ComponentDesc> list) {
      for (const auto &c: list) {
        componentsHash = mem_hash_fnv1<64>((const char *) &c.name, sizeof(c.name), componentsHash);
        componentsHash = mem_hash_fnv1<64>((const char *) &c.flags, sizeof(c.flags), componentsHash);
        componentsHash = mem_hash_fnv1<64>((const char *) &c.type, sizeof(c.type), componentsHash);
      }
    };
    hashComponents(desc.componentsRW);
    hashComponents(desc.componentsRO);
    hashComponents(desc.componentsRQ);
    hashComponents(desc.componentsNO);
    return componentsHash;
  }

  uint32_t GState::addOneQuery() {
    // todo: may be keep free list instead?
    auto qi = freeQueriesCount ? eastl::find(queriesReferences.begin(), queriesReferences.end(), 0)
                               : queriesReferences.end();
    if (qi != queriesReferences.end()) {
      freeQueriesCount--;
      *qi = 1;
      uint32_t index = (uint32_t) std::distance(queriesReferences.begin(), qi);
      archetypeQueries[index].reset();
      archetypeEidQueries[index].reset();
      resolvedQueries[index].reset();
      resetQueryStatus(index);
      queryDescs[index].clear();
      return index;
    }
    archetypeEidQueries.emplace_back();
    archetypeQueries.emplace_back();
    queryDescs.emplace_back();
    //DAECS_EXT_ASSERT(archetypeQueries.size() == queryDescs.size());
    resolvedQueries.emplace_back();
    //DAECS_EXT_ASSERT(archetypeQueries.size() == resolvedQueries.size());
    addOneResolvedQueryStatus();

    queriesReferences.push_back(1);
    //DAECS_EXT_ASSERT(archetypeQueries.size() == queriesReferences.size());
    queriesGenerations.push_back(currentQueryGen);
    //DAECS_EXT_ASSERT(archetypeQueries.size() == queriesGenerations.size());
    return (uint32_t) resolvedQueries.size() - 1;
  }

  template<typename T, typename Cnt>
  static T *add_to_fixed_container(T *ft, Cnt &__restrict cnt, const T *__restrict add, size_t size) {
    if (size == 0)
      return ft;
    G_ASSERT(cnt + size <= eastl::numeric_limits<Cnt>::max());
    const size_t nextSize = std::min<size_t>(size_t(cnt) + size, eastl::numeric_limits<Cnt>::max());

    if (!ft) {
      T *next = (T *) realloc(ft, nextSize * sizeof(T));
      if (DAGOR_UNLIKELY(next == nullptr)) {
        next = (T *) malloc(nextSize * sizeof(T)); // so no mem setting
        memcpy(next, ft, cnt * sizeof(T));                  //-V575
        free(ft);
      }
      eastl::swap(ft, next);
    }
    memcpy(ft + cnt, add, size * sizeof(T));
    cnt = (Cnt)nextSize;
    return ft;
  }

  bool GState::makeArchetypesQuery(archetype_t first_archetype, uint32_t index, bool wasFullyResolved) {
    const ResolvedQueryDesc &resDesc = resolvedQueries[index];
    if (resDesc.isEmpty()) {
      //LOGD3("skipping empty query {}", index);
      return false;
    }
    ArchetypesQuery &query = archetypeQueries[index];
    ArchetypesEidQuery &queryEid = archetypeEidQueries[index];
    if (DAGOR_UNLIKELY(!wasFullyResolved)) {
      const uint32_t totalDataComponentsCount = resDesc.getRwCnt() + resDesc.getRoCnt();
      //LOGD2("copying types and sizes to archetype query {} rw = {} ro ={}", index, resDesc.rw.cnt, resDesc.ro.cnt);

      const bool validCsz = totalDataComponentsCount == query.getComponentsCount();
      const bool reResolve = !validCsz || lastQueriesResolvedComponents != dataComponents.size();
      if (!validCsz) // todo : only needed for eid queries. Can be deferred explicitly marked, if eid query is needed at all
      {
        if (totalDataComponentsCount) {
          queryEid.componentsSizesAt = (uint32_t)archComponentsSizeContainers.size();
          archComponentsSizeContainers.resize(totalDataComponentsCount + archComponentsSizeContainers.size());
        }
      }
      if (reResolve) {
        query.data.rwCount = (uint8_t) resDesc.getRW().cnt;
        query.data.roCount = (uint8_t) resDesc.getRO().cnt;
        G_ASSERT(totalDataComponentsCount <= resDesc.getComponents().size());
        G_ASSERT(
            totalDataComponentsCount == 0 ||
            queryEid.componentsSizesAt + totalDataComponentsCount <= archComponentsSizeContainers.size());
        // todo: this is only needed for eid queries
        auto componentCsz = archComponentsSizeContainers.data() + queryEid.componentsSizesAt;
        for (uint32_t compI = 0; compI < totalDataComponentsCount; ++compI, ++componentCsz) {
          if (validCsz && *componentCsz) // if it is already inited, why bother
            continue;
          component_index_t cidx = resDesc.getComponents()[compI];
          DataComponent *dt = dataComponents.getDataComponent(cidx);
          *componentCsz = (uint16_t) componentTypes.getComponentData(dt->componentIndex)->size;
        }
      }
    } else {
    }
    const archetype_t archetypesCount = archetypes.size();
    const uint32_t minRequired = resDesc.requiredComponentsCount();
    const bool updateOneArchetype = first_archetype == archetypesCount - 1;

    if (updateOneArchetype) // saves 5% of framemem_region
    {
      if (archetypes.getComponentsCount(first_archetype) < minRequired) // not enough components to even match
      {
        return archetypeQueries[index].getQueriesCount() != 0;
      }
    }
    // TIME_PROFILE_DEV(makeArchetypesQuery);
    //FramememScopedRegion scope_framemem_lib16;
    // G_ASSERT_RETURN(resDesc.components.size() == 0, false);//not sure if it is needed
    G_ASSERTF_RETURN(resDesc.getRO().start == resDesc.getRW().start + resDesc.getRW().cnt, false, "{} {} {}",
                     resDesc.getRO().start, resDesc.getRW().start, resDesc.getRW().cnt);
    // DAECS_EXT_ASSERT(query.lastArchetypesGeneration != archetypes.generation() || archetypes.generation() == 0);
    const uint32_t totalDataComponentsCount = query.getComponentsCount();
    G_ASSERT(resDesc.getRwCnt() + resDesc.getRoCnt() + resDesc.getRqCnt() <= resDesc.getComponents().size());

    // this is potentially slow, as it depends on amount of archetypes
    // for persistent queries doesn't matter (won't be re-calculated until new archetype added)
    // but we can speed that up, if we will have list/range of archetype for each components, and iterate over smallest list/range from
    // all archetype components there are much less components, than archetypes, todo: make this optimization if ever slow

    // dagECS generation() returns size()
    //G_ASSERT(archetypes.generation() == archetypes.size()); // todo: otherwise we can't incrementally update queries

    std::vector<archetype_t> queries;
    std::vector<ArchetypesQuery::offset_type_t> allComponentsArchOffsets;
    //dag::RelocatableFixedVector<archetype_t, 4, true, framemem_allocator> queries;
    //dag::RelocatableFixedVector<ArchetypesQuery::offset_type_t, 16, true, framemem_allocator, uint32_t, false> allComponentsArchOffsets;

    //LOGD2("adding {} archetypes to query {}", archetypesCount - first_archetype,
    //      &query >= archetypeQueries.begin().base() && &query < archetypeQueries.end().base() ? &query -
    //                                                                                             archetypeQueries.begin().base()
    //                                                                                          : -1);

    // we don't want to add same archetypes to query list
    // chose max between next after last already found (query.queriesEnd()[-1] + 1) and first_archetype to add
    archetype_t firstArchetypeToAdd =
        !updateOneArchetype && query.queriesCount > 0 ? std::max(archetype_t(query.queriesEnd()[-1] + 1),
                                                                 first_archetype) : first_archetype;
    for (archetype_t ai = firstArchetypeToAdd, ae = archetypesCount; ai < ae; ++ai) {
      //const auto &archetype = archetypes.getArchetype(ai);
      const auto componentCount = archetypes.getArchetypeComponentCount(ai);
      if (componentCount < minRequired) // not enough components to even match
        continue;
      const auto &archInfo = archetypes.getArchetypeStorageUnsafe(ai).INFO;
      archetype_component_id tempIds[max_query_components_count];

      if (resDesc.getRqCnt()) {
        for (uint32_t i = resDesc.getRQ().start, ei = i + resDesc.getRqCnt(); i < ei; ++i)
          if (archInfo.getComponentId(resDesc.getComponents()[i]) == INVALID_ARCHETYPE_COMPONENT_ID)
            goto loop_continue; // legal continue outer loop
      }
      if (resDesc.getNoCnt()) {
        for (uint32_t i = resDesc.getNO().start, ei = resDesc.getComponents().size(); i < ei; ++i)
          if (archInfo.getComponentId(resDesc.getComponents()[i]) != INVALID_ARCHETYPE_COMPONENT_ID)
            goto loop_continue; // legal continue outer loop
      }
      for (uint32_t i = 0; i != totalDataComponentsCount; ++i) {
        archetype_component_id id = archInfo.getComponentId(resDesc.getComponents()[i]);
        if (id == INVALID_ARCHETYPE_COMPONENT_ID) {
          if (!resDesc.getOptionalMask()[i]) // required data is not optional
            goto loop_continue;              // legal continue outer loop
        }
        tempIds[i] = id;
      }
      goto loop_normal;
      loop_continue:
      continue;
      loop_normal:;
      // we have found acceptable archetype
      const uint32_t oldOffsets = (uint32_t)allComponentsArchOffsets.size();

      allComponentsArchOffsets.resize(oldOffsets + totalDataComponentsCount);
      const archetype_component_id *__restrict id = tempIds;
      auto archOffsetsOfs = archetypes.getArchetypeComponentOfsUnsafe(ai);
      for (auto oi = allComponentsArchOffsets.data() + oldOffsets, oe = oi + totalDataComponentsCount;
           oi != oe; ++oi, ++id)
        *oi = *id == INVALID_ARCHETYPE_COMPONENT_ID ? ArchetypesQuery::INVALID_OFFSET
                                                    : archetypes.getComponentOfsFromOfs(*id, archOffsetsOfs);
      queries.push_back(ai);
    }

    G_ASSERT(resDesc.getRO().start == resDesc.getRW().start + resDesc.getRW().cnt);
    // G_ASSERT(query.componentsCount == totalDataComponentsCount || !query.componentsType);

    if (!queries.size())
      return query.getQueriesCount() != 0;
    G_ASSERTF_RETURN(totalDataComponentsCount == resDesc.getRO().cnt + resDesc.getRW().cnt, false, "{}",
                     totalDataComponentsCount);

    const size_t oldQueriesCount = query.getQueriesCount(),
        newQueriesCount =
        std::min<size_t>(eastl::numeric_limits<decltype(ArchetypesQuery::queriesCount)>::max(),
                         oldQueriesCount + queries.size());
    G_ASSERT(oldQueriesCount + queries.size() < eastl::numeric_limits<archetype_t>::max());
    size_t oldOfsCount = oldQueriesCount * totalDataComponentsCount, newOfsCount =
        newQueriesCount * totalDataComponentsCount;
    std::ostringstream t{};
    for(auto arch : queries) {
      t << fmt::format(" {}", this->templates.getTemplate(this->archetypes.getParentTemplate(arch))->getName());
    }
    LOG("Query \"{}\" adding archetypes of templates: {}", this->queryDescs[index].getName(), t.str());

    if (query.isInplaceOffsets(newOfsCount)) {
      // we still fit inside inplace array
      memcpy(query.getArchetypeOffsetsPtrInplace() + oldOfsCount, allComponentsArchOffsets.data(),
             sizeof(ArchetypesQuery::offset_type_t) * allComponentsArchOffsets.size());
    } else {
      if (query.isInplaceOffsets(oldOfsCount)) {
        // convert offsets into allocated array, we are not fitting anymore inside inplace array
        int cnt = 0;
        query.allComponentsArchOffsets =
            add_to_fixed_container((ArchetypesQuery::offset_type_t *) nullptr, cnt,
                                   query.getArchetypeOffsetsPtrInplace(), oldOfsCount);
      }

      query.allComponentsArchOffsets = add_to_fixed_container(query.allComponentsArchOffsets, oldOfsCount,
                                                              allComponentsArchOffsets.data(),
                                                              allComponentsArchOffsets.size());
    }

    // first check if new arch size can fit inplace
    if (ArchetypesQuery::isInplaceQueries(newQueriesCount)) {
      // we still fit inside inplace array
      memcpy((void *) (query.queriesInplace() + oldQueriesCount), queries.data(), sizeof(archetype_t) * queries.size());
      query.queriesCount = uint16_t(query.queriesCount + queries.size());
    } else {
      // is current query implace?
      if (query.isInplaceQueries()) {
        // convert queries into allocated array, we are not fitting anymore inside inplace array
        int cnt = 0;
        query.queries = add_to_fixed_container((archetype_t *) nullptr, cnt, query.queriesInplace(), oldQueriesCount);
      }
      query.queries = add_to_fixed_container(query.queries, query.queriesCount, queries.data(), queries.size());
    }
    query.lastArch = queries.back();
    if (oldQueriesCount == 0)
      query.firstArch = *query.queriesBegin();

    G_ASSERT(query.getQueriesCount() < eastl::numeric_limits<archetype_t>::max());

    return query.getQueriesCount() != 0;
  }

  GState::ResolvedStatus
  GState::resolveQuery(uint32_t index, ResolvedStatus currentStatus, ResolvedQueryDesc &resDesc) {
    const CopyQueryDesc &copyDesc = queryDescs[index];
    const char *name = copyDesc.getName();
    const BaseQueryDesc desc = copyDesc.getDesc();
    G_UNUSED(name);
    G_ASSERT(
        desc.componentsRQ.size() + desc.componentsNO.size() + desc.componentsRW.size() + desc.componentsRO.size() > 0);
    uint8_t ret = RESOLVED_MASK;
    int componentsIterated = 0;

    enum ReqType {
      DATA,
      REQUIRED,
      REQUIRED_NOT
    };
    auto makeRange = [&](dag::ConstSpan<ComponentDesc> components, ReqType req) -> bool {
      auto &dest = resDesc.getComponents();
      for (auto cdI = components.begin(), cdE = components.end(); cdI != cdE; ++cdI, ++componentsIterated) {
        G_ASSERTF(componentsIterated < dest.size(), "{}: {} < {}", (uint8_t) req, componentsIterated, dest.size());
        if (dest[componentsIterated] != INVALID_COMPONENT_INDEX) {
          G_ASSERT(bool(cdI->flags & CDF_OPTIONAL) == resDesc.getOptionalMask()[componentsIterated]);
          continue;
        }
        auto &cd = *cdI;
        component_index_t id = this->dataComponents.getIndex(cd.name);
        LOGD3("query <{}>, component {:#x}, type {:#x}", name, cd.name, cd.type);

        if (id == INVALID_COMPONENT_INDEX) {
          if ((req == REQUIRED_NOT) ||
              (cd.flags & CDF_OPTIONAL)) // optional component can be even not registered at all
          {
            ret &= ~FULLY_RESOLVED;
            continue;
          }
          ret = NOT_RESOLVED;
          return false;
        }
        // verify type
        if (cd.type != ComponentTypeInfo<auto_type>::type && (req == DATA)) //-V560
        {
          DataComponent *comp = this->dataComponents.getDataComponent(id);
          if (comp->componentHash !=
              cd.type) // cd.type == auto_type is special case, basically it is 'auto' (generic). It is legit
            // at least in require/require_not
          {
            auto dat = this;
            EXCEPTION("component<{}> type mismatch in query <{}>, type is {}({:#x}), required in query <{}>({:#x})",
                      dat->dataComponents.getName(id), name, dat->componentTypes.getName(comp->componentIndex),
                      comp->componentHash,
                      dat->componentTypes.getName(cd.type), cd.type);
            resDesc.reset();
            // wrong query can not become correct one ever. no need to check it, just assume it is empty query
            ret = RESOLVED_MASK;
            return false;
          }
        }
        dest[componentsIterated] = id;
      }
      return true;
    };
    const bool wasResolved = isResolved(currentStatus);
    if (!wasResolved) {
      const uint32_t dataComponentsCount = (uint32_t) desc.componentsRW.size() + (uint32_t) desc.componentsRO.size();
      const uint32_t totalComponents =
          (uint32_t) desc.componentsRQ.size() + (uint32_t) desc.componentsNO.size() + dataComponentsCount;
      G_ASSERT(resDesc.getComponents().size() == 0 || resDesc.getComponents().size() == totalComponents);
      resDesc.getComponents().reserve(totalComponents);
      resDesc.getComponents().assign((uint16_t) totalComponents, INVALID_COMPONENT_INDEX);
      resDesc.getOptionalMask().reset();
      for (uint32_t i = 0, e = dataComponentsCount; i < e; ++i)
        if (copyDesc.components[i].flags & CDF_OPTIONAL)
          resDesc.getOptionalMask().set(i);

      const uint32_t checkComponents = totalComponents;

      // reduce finds in later stages
      for (uint32_t i = 0, e = checkComponents; i != e; ++i) {
        auto &cd = copyDesc.components[i];
        auto cidx = this->dataComponents.getIndex(cd.name);
        if (cidx != INVALID_COMPONENT_INDEX) {
          DataComponent *comp = this->dataComponents.getDataComponent(cidx);

          if (comp->componentHash != cd.type) {
            if (i < dataComponentsCount || cd.type != ComponentTypeInfo<auto_type>::type) {
              EXCEPTION("component<{}> type mismatch in query <{}>, type is {}({:#x}), required in query <{}>({:#x})",
                        this->dataComponents.getName(cidx), name, this->componentTypes.getName(comp->componentIndex),
                        comp->componentHash, this->componentTypes.getName(cd.type), cd.type);
              resDesc.reset();
              // wrong query can not become correct one ever. no need to check it, just assume it is empty query
              return RESOLVED_MASK;
            }
          }
        }
        resDesc.getComponents()[i] = cidx;
      }

      for (uint32_t i = checkComponents, e = totalComponents; i < e; ++i)
        resDesc.getComponents()[i] = this->dataComponents.getIndex(copyDesc.components[i].name);
      resDesc.getRwCnt() = uint8_t(desc.componentsRW.size());
      resDesc.getRoCnt() = uint8_t(desc.componentsRO.size());
      resDesc.getRqCnt() = uint8_t(desc.componentsRQ.size());
      resDesc.setRequiredComponentsCount(copyDesc.requiredSetCount);
    } else {
    }
    // Note: order of range fill is important
    if (!makeRange(dag::ConstSpan<ComponentDesc>(copyDesc.components.data(), resDesc.getRwCnt() + resDesc.getRoCnt()),
                   DATA))
      return (ResolvedStatus) ret;
    G_ASSERTF(resDesc.getRwCnt() + resDesc.getRoCnt() == componentsIterated,
              "we rely on optionalmask to be parallel with data components");
    if (wasResolved) // if query was partially resolved, it for sure has resolved required components
      componentsIterated += (int) desc.componentsRQ.size();
    else {
      if (!makeRange(desc.componentsRQ, REQUIRED))
        return (ResolvedStatus) ret;
    }
    if (!makeRange(desc.componentsNO, REQUIRED_NOT))
      return (ResolvedStatus) ret;
    LOGD3("resolved query <{}>, resolve={}", name, (uint8_t) currentStatus);
    return (ResolvedStatus) ret;
  }

  bool GState::resolvePersistentQueryInternal(uint32_t index) {
    const ResolvedStatus ret = resolveQuery(index, getQueryStatus(index), resolvedQueries[index]);
    orQueryStatus(index, ret);
    LOGD3("set resolved query <{}> to {}", queryDescs[index].getName(), isResolved(index));
    return ret != NOT_RESOLVED;
  }

  QueryId GState::createQuery(const NamedQueryDesc &desc) {
    QueryId h = createUnresolvedQuery(desc);
    if (!h)
      return h;
    // return h;
    uint32_t index = h.index();
    if (queriesReferences[index] == 1 && resolvePersistentQueryInternal(index)) {} // recently added
    makeArchetypesQuery(0, index, false);
    return h;
  }

  bool GState::makeArchetypesQueryInternal(archetype_t last_arch_count, uint32_t index, bool should_re_resolve) {
    const ResolvedStatus status = getQueryStatus(index);

    if (DAGOR_UNLIKELY(!isFullyResolved(status))) {
      LOGD3("updating = {} {}, as not fully resolved", queryDescs[index].getName(), index);
      if (should_re_resolve) {
        if (!resolvePersistentQueryInternal(index)) {
          EXCEPTION("update failed = {}", queryDescs[index].getName());
          return false;
        }
      } else if (!isResolved(status)) // not resolved and can't be resolved now. no need to check anything
        return false;
    }
    return makeArchetypesQuery(last_arch_count, index, isFullyResolved(status));
  }

  __forceinline bool
  GState::updatePersistentQueryInternal(archetype_t last_arch_count, uint32_t index, bool should_re_resolve) {
    //LOGD3("updatePersistent = {}, index = {}", queryDescs[index].name, index);
    //LOGD2("update = {}, current resolved= {}", queryDescs[index].name, (uint8_t) getQueryStatus(index));
    const bool ret = makeArchetypesQueryInternal(last_arch_count, index, should_re_resolve);

    return ret;
  }

  inline bool GState::updatePersistentQuery(archetype_t last_arch_count, QueryId h, uint32_t &index, bool should_re_resolve) {
    G_ASSERT(isQueryValid(h));
    index = h.index();
    return updatePersistentQueryInternal(last_arch_count, index, should_re_resolve);
  }

  void GState::registerEsEvent(es_index_type j) {
    const EntitySystemDesc *es = esList[j];
    if(!es->ops.onEvent) // Should never happen, but yea
      return;
    for (auto evt : es->evSet) {
      //const auto evtId = eventsDb.findEvent(evt);
      esEvents[evt].push_back(j);
    }
  }

  void GState::updateAllQueriesInternal() {
    G_ASSERT(allQueriesUpdatedToArch < this->archetypes.size());
    // we never add datacomponents after init, so this is not needed
    const bool shouldResolveQueries = lastQueriesResolvedComponents != dataComponents.size();
    for (int index = 0, e = (int) queriesReferences.size(); index < e; ++index) {
      if (queriesReferences[index] && updatePersistentQueryInternal(allQueriesUpdatedToArch, index, shouldResolveQueries)) {
        auto it = queryToEsMap.find(QueryId::make(index, queriesGenerations[index]));
        if (it != queryToEsMap.end()) {
          for (auto esIndex: it->second)
            registerEsEvent(esIndex); // we dont have updates cause that's literally impossible
          queryToEsMap.erase(it);
        }
      }
    }
    lastQueriesResolvedComponents = (uint32_t) dataComponents.size();
    uint32_t currentArchetypeCount = archetypes.size();
    //for (int ai = allQueriesUpdatedToArch, e = currentArchetypeCount; ai < e; ++ai) // only used for optimzied Create, recreate, destroy, which I dont do
    //  updateArchetypeESLists(ai);
    allQueriesUpdatedToArch = (archetype_t) currentArchetypeCount;
  }

  void GState::registerEsEvents() {
    G_ASSERT(ECS_HASH("name").hash == ecs_hash("name") && ECS_HASH("name").hash == ECS_HASH_SLOW("name").hash);
    //esEvents.clear();
    //esOnChangeEvents.clear();
    for (es_index_type j = 0, ej = (es_index_type)esList.size(); j < ej; ++j) {
      QueryId h = esListQueries[j];
      if (!h || archetypeQueries[h.index()].getQueriesCount())
        registerEsEvent(j);
      else
        queryToEsMap[h].push_back((es_index_type) j);
    }
  }

  void GState::resetEsOrder() {
    {
      // fake framemen goes here hahaha
      std::vector<EntitySystemDesc *> esFullList;

      // fuck da framemem all my homies hate the frame mem
      for (EntitySystemDesc *psd = EntitySystemDesc::tail; psd; psd = psd->next)
        esFullList.push_back(psd);

      // randomize list to avoid depending on native ES registration order
      // which might be different on different platforms, depend on hot-reload, etc...
      std::sort(esFullList.begin(), esFullList.end(),
                [](auto a, auto b) { return ECS_HASH_SLOW(a->name).hash < ECS_HASH_SLOW(b->name).hash; });

      // check if they are correct event handlers
      for (auto sd: esFullList) {
        if ((sd->ops.onEvent == nullptr) !=
            (sd->evSet.empty())) //  && (sd->getCompSet() == NULL || *sd->getCompSet() == 0)
        {
          EXCEPTION("entity system <{}> has {} events signed for but has {}event handler", sd->name, sd->evSet.size(),
                    sd->ops.onEvent ? "" : "no ");
          if (sd->evSet.empty())
            sd->ops.onEvent = nullptr;
          else
            sd->evSet.clear();
        }
      }
      this->esList.reserve(esFullList.size());
      for (int i = 0; i < esFullList.size(); i++) {
        this->esList.push_back(esFullList[i]);
      }
      for (auto &eq: esListQueries)
        if (eq != QueryId() && isQueryValid(eq))
          G_ASSERT(
              false); // dont feel like doing allthat, shouldnt ever happen, I dont think ill ever plan to destroy a query
      //destroyQuery(eq);
      esListQueries.resize(esList.size());
      for (int j = 0, ej = (int) esList.size(); j < ej; ++j)
        esListQueries[j] = esList[j]->emptyES ? QueryId() : createUnresolvedQuery(*esList[j]);
      registerEsEvents();
    }
    updateAllQueries();
    //lastEsGen = EntitySystemDesc::generation;
  }

  void GState::sendEventImmediate(EntityId eid, Event &evt, EntityManager *mgr) {
    G_ASSERTF(evt.getFlags() & EVCAST_UNICAST, "Tried to send entity {:#x} event {} that is not unicast", eid.get_handle(), evt.getName());
    return notifyESEventHandlers(eid, evt, mgr);
  }

  void GState::broadcastEventImmediate(Event &evt, EntityManager *mgr) {
    G_ASSERTF(evt.getFlags() & EVCAST_BROADCAST, "Tried to send event {} that is not broadcast", evt.getName());
    auto esListIt = esEvents.find(evt.getType());
    if (esListIt != esEvents.end()) {
      for (es_index_type esListNo : esListIt->second)
      {
        const EntitySystemDesc &es = *esList[esListNo];
        performQueryEmptyAllowed(esListQueries[esListNo], es.ops.onEvent, evt, mgr);
      }
    }
  }

  static constexpr int MAX_ONE_EID_QUERY_COMPONENTS = 96;
  inline void GState::performQueryES(QueryId h, EventFuncType fun, const Event &__restrict evt, EntityManager *mgr) {
    uint32_t index = h.index();
    auto &__restrict archDesc = archetypeQueries[index];
    if (!archDesc.getQueriesCount())
      return;

    QueryView qv{mgr};
    QueryView::ComponentsData componentData[MAX_ONE_EID_QUERY_COMPONENTS];
    qv.componentData = componentData;
    auto archBegin = archDesc.queriesBegin();
    auto archEnd = archDesc.queriesEnd();
    for(int i = 0;archBegin != archEnd; i++, archBegin++) {
      auto curr_arch = mgr->arch_data.getArch(*archBegin);
      auto EntityCount = curr_arch->getEntityCount();
      qv.index_start = 0;
      qv.index_end = EntityCount;
      if(DAGOR_UNLIKELY(archDesc.getComponentsCount()==0)) { // has no data we need to read in
        for(auto & chunk : curr_arch->chunks) {
          qv.eid_refs = (ecs::EntityId*)(chunk.getCompArrayUnsafe(0, EntityCount)); // first array at 0th is always eid
          qv.num_of_entities = chunk.used;
          fun(evt, qv);
        }
      } else {
        for(auto & chunk : curr_arch->chunks) {
          qv.eid_refs = (ecs::EntityId*)(chunk.getCompArrayUnsafe(0, EntityCount)); // first array at 0th is always eid
          qv.num_of_entities = chunk.used;
          auto totalComponentsCount = archDesc.getComponentsCount();
          auto *__restrict componentData = const_cast<QueryView::ComponentsData *>(qv.componentData);
          auto *__restrict archetypeOffsets = archDesc.getArchetypeOffsetsPtr() + totalComponentsCount * i;
          for(uint32_t compIndex = 0; compIndex != totalComponentsCount; compIndex++, componentData++, archetypeOffsets++) {
            auto archOfs = *archetypeOffsets;
            if (DAGOR_LIKELY(archOfs != ArchetypesQuery::INVALID_OFFSET)) {
              *componentData = chunk.getCompArrayUnsafe(archOfs, EntityCount);
            } else {
              *componentData = nullptr;
            }
          }
          fun(evt, qv);
        }
      }
    }
  }

  inline void GState::performQueryEmptyAllowed(QueryId h, EventFuncType fun, const Event &evt, EntityManager *mgr)
  {
    if (h)
      performQueryES(h, fun, evt, mgr);
    else
      fun(evt, QueryView(mgr));
  }

  inline void GState::callESEvent(es_index_type esIndex, const Event &evt, QueryView &qv) {
    const EntitySystemDesc &es = *esList[esIndex];
    es.ops.onEvent(evt, qv);
  }

  void GState::notifyESEventHandlers(EntityId eid, const Event &evt, EntityManager *mgr) {
    auto eventType = evt.getType();
    auto esListIt = esEvents.find(eventType); // this is extremely slow, and should not be needed. We can register all events with flat
    // arrays, not search.
    if (esListIt == esEvents.end())
      return; // nvm it can happen :(
      //G_ASSERT(false); // shouldnt ever happen so lets make it a fail condition
    auto &list = esListIt->second;
    if(list.empty()) // entirely possible for us to have events that have no valid archetypes
      return;
    QueryView qv{mgr};
    QueryView::ComponentsData componentData[MAX_ONE_EID_QUERY_COMPONENTS];
    qv.componentData = componentData;
    const uint32_t idx = eid.index();
    for(es_index_type esIndex : list) {
      QueryId queryId = esListQueries[esIndex];
      //G_ASSERTF(queryId, "Empty queries are not allowed");
      // invalid queryId signifies empty query, so just send it cause no data to serialize
      if(!queryId || fillEidQueryView(eid, mgr->entDescs[idx], queryId, qv, mgr->arch_data))
        callESEvent(esIndex, evt, qv);
    }
  }

  bool GState::fillEidQueryView(ecs::EntityId eid, EntityDesc entDesc, QueryId h, QueryView &__restrict qv,
                                MgrArchetypeStorage &storage) {
    //DAECS_EXT_ASSERT(isQueryValid(h));
    const uint32_t qIndex = h.index();
    auto &__restrict archDesc = archetypeQueries[qIndex];
    const auto lastArch = archDesc.lastArch;
    auto archetype = entDesc.archetype_id;
    if (DAGOR_LIKELY(archetype > lastArch))
      return false;
    const auto queriesCount = archDesc.getQueriesCount();
    const auto firstArch = archDesc.firstArch;

    size_t itId = archetype - firstArch;
    const auto archEidDesc = archetypeEidQueries[qIndex];
    // all this basically does a check to see:
    // 1: if this query has the archetype of the entity
    // 2: where that archetype is inside this query so we can do proper lookup
    if (DAGOR_UNLIKELY(itId > 0 && lastArch + 1 != queriesCount + firstArch)) // non sequential
    {
      if (DAGOR_LIKELY(archDesc.isInplaceQueries(queriesCount)))
      {
        // linear search inside inplace queries. that is likely be to quiet often case, and this data is already inside cache line
        itId = std::find(archDesc.queriesInplace(), archDesc.queriesInplace() + queriesCount, archetype) - archDesc.queriesInplace();
      }
      else
      {
        // binary search otherwise
        if (lastArch == archetype) // no cache miss comparison
          itId = queriesCount - 1;
        else
          itId = eastl::binary_search_i(archDesc.queries, archDesc.queries + queriesCount, archetype) - archDesc.queries;
      }
    }
    // likely that the archetype of our entity isnt applied to by this query
    if (DAGOR_LIKELY(uint32_t(itId) >= queriesCount))
      return false;
    qv.index_start = 0; // we are going to point to specific entity data, so only iterate over this specific entities data
    qv.index_end = 1;
    qv.roRW = archDesc.roRW;
    qv.id = h;
    const uint32_t totalComponentsCount = archDesc.getComponentsCount();
    G_ASSERT(archDesc.getComponentsCount() < MAX_ONE_EID_QUERY_COMPONENTS); // shouldnt happen cause this is the value gaijin uses so its THEIR fault
    uint32_t idInChunk;
    auto chunk = storage.getArch(archetype)->getChunkAndOffset(entDesc.chunk_id, idInChunk);
    auto EntityCount = storage.getArch(archetype)->getEntityCount();
    // get inlined bitch
    auto *__restrict componentData = const_cast<QueryView::ComponentsData *>(qv.componentData);
    auto *__restrict archetypeOffsets = archDesc.getArchetypeOffsetsPtr() + totalComponentsCount * itId;
    auto *__restrict componentsSize = archComponentsSizeContainers.data() + archEidDesc.componentsSizesAt;
    for(uint32_t compIndex = 0; compIndex != totalComponentsCount; compIndex++, componentData++, archetypeOffsets++) {
      auto archOfs = *archetypeOffsets;
      if (DAGOR_LIKELY(archOfs != ArchetypesQuery::INVALID_OFFSET)) {
        *componentData = chunk->getCompArrayUnsafe(archOfs, EntityCount) + idInChunk * uint32_t(componentsSize[compIndex]);
      } else {
        *componentData = nullptr;
      }
    }
    qv.eid_refs = (ecs::EntityId*)(chunk->getCompArrayUnsafe(0, EntityCount) + idInChunk * sizeof(ecs::EntityId));
    return true;
  }
}
