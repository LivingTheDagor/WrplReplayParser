

#ifndef MYEXTENSION_ENTITYMANAGER_H
#define MYEXTENSION_ENTITYMANAGER_H


#include "typesAndLimits.h"
#include "ecs/ComponentTypes.h"
#include "ecs/Component.h"
#include "ecs/ComponentRef.h"
#include "ecs/DataComponents.h"
#include "onDemandInit.h"
#include "baseIo.h"
#include "Templates.h"
#include "EntityDescs.h"
#include "ecs/LoadTemplatesBlk.h"
#include "ecs/ComponentTypes.h"
#include "unordered_set"
#include "ecs/query/ecsQuery.h"
#include "ecs/query/event.h"
#include "ecs/query/entitySystem.h"
#include "ecs/query/queryView.h"
#include "ecs/query/eventsDB.h"
#include "ecs/internal/ecsQueryInternal.h"
#include "ecs/internal/ArchetypesQuery.h"
#include "Unit.h"
#include <shared_mutex>

DEFINE_HANDLE(handle_ecs)
#define ECS_LOGI(format_, ...) ELOGI(handle_ecs, format_, __VA_ARGS__)
#define ECS_LOGD1(format_, ...) ELOGD1(handle_ecs, format_, __VA_ARGS__)
#define ECS_LOGD2(format_, ...) ELOGD2(handle_ecs, format_, __VA_ARGS__)
#define ECS_LOGD3(format_, ...) ELOGD3(handle_ecs, format_, __VA_ARGS__)
#define ECS_LOGE(format_, ...) ELOGE(handle_ecs, format_, __VA_ARGS__)

DEFINE_HANDLE(handle_ecs_events)
#define EVENT_LOGI(format_, ...) ELOGI(handle_ecs_events, format_, __VA_ARGS__)
#define EVENT_LOGD1(format_, ...) ELOGD1(handle_ecs_events, format_, __VA_ARGS__)
#define EVENT_LOGD2(format_, ...) ELOGD2(handle_ecs_events, format_, __VA_ARGS__)
#define EVENT_LOGD3(format_, ...) ELOGD3(handle_ecs_events, format_, __VA_ARGS__)
#define EVENT_LOGE(format_, ...) ELOGE(handle_ecs_events, format_, __VA_ARGS__)

DEFINE_HANDLE(handle_ecs_query)
#define QUERY_LOGI(format_, ...) ELOGI(handle_ecs_query, format_, __VA_ARGS__)
#define QUERY_LOGD1(format_, ...) ELOGD1(handle_ecs_query, format_, __VA_ARGS__)
#define QUERY_LOGD2(format_, ...) ELOGD2(handle_ecs_query, format_, __VA_ARGS__)
#define QUERY_LOGD3(format_, ...) ELOGD3(handle_ecs_query, format_, __VA_ARGS__)
#define QUERY_LOGE(format_, ...) ELOGE(handle_ecs_query, format_, __VA_ARGS__)

DEFINE_HANDLE(handle_entity)
#define ENTITY_LOGI(format_, ...) ELOGI(handle_entity, format_, __VA_ARGS__)
#define ENTITY_LOGD1(format_, ...) ELOGD1(handle_entity, format_, __VA_ARGS__)
#define ENTITY_LOGD2(format_, ...) ELOGD2(handle_entity, format_, __VA_ARGS__)
#define ENTITY_LOGD3(format_, ...) ELOGD3(handle_entity, format_, __VA_ARGS__)
#define ENTITY_LOGE(format_, ...) ELOGE(handle_entity, format_, __VA_ARGS__)

class PyGState; // for python bindings
struct ParserState;

namespace ecs {
  class GState;
  static inline entity_id_t make_eid(uint32_t index, uint32_t gen) { return index | (gen << ENTITY_INDEX_BITS); }

  typedef void (*after_components_cb)(GState * state);
  extern OnDemandInit<std::vector<after_components_cb>> after_comps_callbacks;
  class GState {
    ComponentTypes componentTypes{};
    DataComponents dataComponents{};
    TemplateDB templates{};
    Archetypes archetypes{};
  public:
    DataBlock wp_cost{}; // I don't feel like implementing blk caching, and this is the most central object. loaded in init.cpp
    GState() { Init(); }

    void printCurrentMemoryUsage(int indent=0) {
      std::string indent_str{};

      indent_str.resize(indent, ' ');
      ECS_LOGI("{}Global State (GState) Memory Usage", indent_str);
      indent_str.resize(indent+2, ' ');
      auto component_types_size = this->componentTypes.printMemoryUsage(indent+2);
      ECS_LOGI("{}Combined Size: {}", indent_str, component_types_size);
      auto datacomponent_types_size = this->dataComponents.printMemoryUsage(indent+2);
      ECS_LOGI("{}Combined Size: {}", indent_str, datacomponent_types_size);
      auto templates_size = this->templates.printMemoryUsage(indent+2);
      ECS_LOGI("{}Combined Size: {}", indent_str, templates_size);
      ECS_LOGI("{}Overall Size: {}", indent_str, component_types_size+datacomponent_types_size+templates_size);
    }

    void Init() {
      this->componentTypes.initialize();
      dataComponents.initialize(componentTypes);
      parseTemplates();
      for(auto &c : *after_comps_callbacks) {
        c(this);
      }
      initCompileTimeQueries();
      resetEsOrder();
    }

    [[nodiscard]] const ComponentTypes *getComponentTypes() const {
      return &componentTypes;
    }

    [[nodiscard]] ComponentTypes *getComponentTypes() {
      return &componentTypes;
    }

    [[nodiscard]] const DataComponents *getDataComponents() const {
      return &dataComponents;
    }

    [[nodiscard]] DataComponents *getDataComponents() {
      return &dataComponents;
    }

    TemplateDB *getTemplateDB() {
      return &templates;
    }

    // because of how the persistent state works, an archetype can exist in GState that has actually not been created in the EntityManager
    // because that specific archetype was only needed for a previous replay
    // this is always called within the mutex already, so need for the shared lock
    void ensureArchetypeInStorage(archetype_t arch_index, MgrArchetypeStorage *storage) {
      this->archetypes.createArchetype(arch_index, storage);
    }

    // given a template, it will attempt to ensure the archetype exists for it
    // assumes InstTemplate has already been created
    // will create archetype in GState and in storage if not exists
    archetype_t EnsureArchetype(template_t tid, MgrArchetypeStorage *storage) {
      InstantiatedTemplate *inst = this->templates.getInstTemplate(tid);;


      // Step 2: Check if archetype needs to be created
      archetype_t arch_index = inst->archetype_index;

      if (arch_index == INVALID_ARCHETYPE) {
        std::unique_lock lk(this->archetypes.archetypes_mtx);

        // Double-check after acquiring lock
        arch_index = inst->archetype_index;
        if (arch_index == INVALID_ARCHETYPE) {
          arch_index = this->archetypes.createArchetype(
              inst->component_indexes.data(),
              (uint32_t) inst->component_indexes.size(),
              this->dataComponents,
              this->componentTypes,
              tid
          );

          inst->archetype_index = arch_index;
          updateAllQueries();
        }
        this->archetypes.createArchetype(arch_index, storage);
      } else {
        std::shared_lock lk(this->archetypes.archetypes_mtx);
        this->archetypes.createArchetype(arch_index, storage);
      }

      return arch_index;
    }


    inline component_index_t
    createComponent(const HashedConstString name, type_index_t component_type, ComponentSerializer *io) {
      return this->dataComponents.createComponent(name, component_type, io, this->componentTypes);
    }

    std::string_view getTemplateName(template_t tid) {
      std::shared_lock lk(this->templates.template_mtx);
      auto templ = this->templates.getTemplateNoLock(tid);
      if (templ) return templ->getName();
      return "";
    }

    template_t getTemplateIdByName(std::string_view name) {
      return this->templates.getTemplateIdByName(name);
    }

    void sendEventImmediate(EntityId eid, Event &evt, EntityManager *mgr);

    void broadcastEventImmediate(Event &evt, EntityManager *mgr);

    friend Component;
    friend InstantiatedTemplate;
  protected:
    void notifyESEventHandlers(EntityId eid, const Event &evt, EntityManager *mgr);

    bool makeArchetypesQuery(archetype_t first_archetype, uint32_t index, bool wasFullyResolved);

    void resetEsOrder();

    typedef uint64_t query_components_hash;

    // thorough hash of RW, RO, RQ, and NO of a BaseQueryDesc
    static query_components_hash query_components_hash_calc(const BaseQueryDesc &desc);

    QueryId createUnresolvedQuery(const NamedQueryDesc &desc);

    QueryId createQuery(const NamedQueryDesc &desc);

    bool updatePersistentQuery(archetype_t last_arch_count, QueryId h, uint32_t &index, bool should_re_resolve);

    void updateAllQueriesInternal();

  private:
    EventsDB eventsDb;
    // don't fucking ask me to fully explain why this is like this I don't fucking know
    // the reason all these comments say SOA is probably because you could also just make one big struct per query
    // so instead they made multiple vectors for each subsection of query
    //I am going to try to offload as much of this as I can to g_data_mgr to prevent a need to reparse every single state creation
    std::vector<CopyQueryDesc> queryDescs;   // SoA, not empty ONLY if resolvedQueries is not resolved fully
    // a list of references to a specific QueryId, basically overcomplicated std::shared_ptr managed on the EntityManager level
    std::vector<uint16_t> queriesReferences; // SoA, reference count of ecs_query_handles
    std::vector<uint8_t> queriesGenerations; // SoA, generation in ecs_query_handles. Sanity check.
    uint32_t freeQueriesCount = 0; // keeps count of available query slots within queriesReferences
    std::unordered_map<query_components_hash, QueryId> queryMap; // used with query hashing to see if a query already exists
    std::vector<ResolvedQueryDesc> resolvedQueries;
    archetype_t allQueriesUpdatedToArch = 0;
    uint32_t lastQueriesResolvedComponents = 0;
    typedef uint32_t status_word_type_t;
    static constexpr status_word_type_t status_words_shift = get_const_log2(sizeof(status_word_type_t) * 4),
        status_words_mask = (1 << status_words_shift) - 1;
    // this stores ResolvedStatus with two bits in a vector, so does some funny bit packing
    std::vector<status_word_type_t> resolvedQueryStatus; // SoA, two-bit vector
    // SoA for QueryId
    uint8_t currentQueryGen = 1;

    typedef uint16_t es_index_type;
    std::vector<const EntitySystemDesc *> esList;
    std::vector<QueryId> esListQueries; // parallel to esList, index in ecs_query_handle
    struct QueryHasher {
      size_t operator()(const QueryId &h) const { return wyhash64(uint32_t(h), 1); }
    };

    // maps all Events to what query they use, basically reverse of esList, es_index_type indexes into esListQueries and esList
    std::unordered_map<QueryId, std::vector<es_index_type>, QueryHasher> queryToEsMap;
    std::vector<ArchetypesQuery> archetypeQueries; // SoA, we need to update ArchetypesQuery from ResolvedQueryDesc again, if we add new
    // archetype
    std::vector<ArchetypesEidQuery> archetypeEidQueries; // SoA, we need to update ArchetypesQuery from ResolvedQueryDesc again, if we add
    // new archetype
    std::vector<uint16_t> archComponentsSizeContainers;
    std::unordered_map<event_type_t, std::vector<es_index_type>> esEvents; // maps all the events to what EntitySystems use it
    bool updateAllQueries() // lock is done during only access that needs mutex
    {
      if (DAGOR_LIKELY(allQueriesUpdatedToArch == this->archetypes.size()))
        return false;
      updateAllQueriesInternal();
      return true;
    }

    bool resolvePersistentQueryInternal(uint32_t index);

    bool makeArchetypesQueryInternal(archetype_t last_arch_count, uint32_t index, bool should_re_resolve);

    bool updatePersistentQueryInternal(archetype_t last_arch_count, uint32_t index, bool should_re_resolve);

    void initCompileTimeQueries();

    enum ResolvedStatus : uint8_t {
      NOT_RESOLVED = 0,
      FULLY_RESOLVED = 1, // im assuming fully resolved is has created ArchetypeQuery
      RESOLVED = 2, // im assuming resolved is has created ResolvedQueryDesc
      RESOLVED_MASK = 3
    };

    uint32_t addOneQuery();

    static bool isFullyResolved(ResolvedStatus s) { return s & FULLY_RESOLVED; }

    static bool isResolved(ResolvedStatus s) { return s != NOT_RESOLVED; }

    ResolvedStatus getQueryStatus(uint32_t idx) const {
      const status_word_type_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
      return (ResolvedStatus) ((resolvedQueryStatus[wordIdx] >> wordShift) & RESOLVED_MASK);
    }

    inline bool isResolved(uint32_t index) const { return isResolved(getQueryStatus(index)); }

    void orQueryStatus(uint32_t idx, ResolvedStatus status) {
      const uint32_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
      resolvedQueryStatus[wordIdx] |= (status << wordShift);
    }

    void resetQueryStatus(uint32_t idx) {
      const status_word_type_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
      resolvedQueryStatus[wordIdx] &= ~(status_word_type_t(RESOLVED_MASK) << wordShift);
    }

    void addOneResolvedQueryStatus() {
      uint32_t sz = (uint32_t) (resolvedQueries.size() + status_words_mask) >> status_words_shift;
      if (sz != resolvedQueryStatus.size())
        resolvedQueryStatus.push_back(status_word_type_t(0));
    }

    ResolvedStatus resolveQuery(uint32_t index, ResolvedStatus currentStatus, ResolvedQueryDesc &resDesc);

    bool isQueryValidGen(QueryId id) const {
      if (!id)
        return false;
      auto idx = id.index();
      return idx < queriesGenerations.size() && queriesGenerations[idx] == id.generation();
    }

    bool isQueryValid(QueryId id) const {
      bool ret = isQueryValidGen(id);
          G_FAST_ASSERT(!ret || queriesReferences[id.index()]);
      return ret;
    }

    void registerEsEvents();

    void registerEsEvent(es_index_type j);

    bool fillEidQueryView(ecs::EntityId eid, EntityDesc entDesc, QueryId h, QueryView &__restrict qv,
                          MgrArchetypeStorage &storage);

    void callESEvent(es_index_type esIndex, const Event &evt, QueryView &qv, EntityManager *mgr);

    void performQueryEmptyAllowed(QueryId h, EventFuncType fun, const Event &evt, EntityManager *mgr);

    void performQueryES(QueryId h, EventFuncType fun, const Event &__restrict evt, EntityManager *mgr);

    friend EntityManager;
    friend PyGState;
  };

  extern OnDemandInit<GState> g_ecs_data;

  class EntityManager {

  public:

    ParserState * owned_by=nullptr;
    uint32_t * curr_time_ms;
    // for ease of access
    std::array<ecs::EntityId, 2048> uid_lookup{};
    std::array<unit::UnitRef*, 2048> uid_unit_ref_lookup{};

    explicit EntityManager(ParserState*owned_by);

    ~EntityManager();

    inline bool doesEntityExist(EntityId e) const { return entDescs.doesEntityExist(e); }

    template_t getEntityTemplateId(EntityId &eid) {
      template_t t;
      entDescs.getEntityTemplateId(eid, t);
      return t;
    }


    void *getNullable(EntityId eid, component_index_t index, archetype_t &archetype) const;

    void *getNullableUnsafe(EntityId eid, component_index_t index, archetype_t &archetype) const;

    bool entityHasComponent(EntityId eid, component_index_t index) const;

    void *getNullable(EntityId eid, HashedConstString component) {
      if (!this->entDescs.doesEntityExist(eid))
        return nullptr;
      auto cidx = this->data_state->dataComponents.getIndex(component.hash);
      if (cidx == INVALID_COMPONENT_INDEX)
        return nullptr;
      //auto comp = this->data_state->dataComponents.getDataComponent(cidx);
      archetype_t arch = INVALID_ARCHETYPE;
      return this->getNullableUnsafe(eid, cidx, arch);
    }

    template<class T>
    inline T *getNullable(EntityId eid, HashedConstString component) {
      return (T*)this->getNullable(eid, component);
    }

    template_t buildTemplateIdByName(const char *templ_name);

    void instantiateTemplate(template_t t);

    /// validates that an initializer
    inline bool validateInitializer(template_t templId, ComponentsInitializer &comp_init);

    EntityId createEntity(EntityId eid, template_t templId, ComponentsInitializer &&initializer);

    bool destroyEntity(EntityId eid, bool is_dtor=false);

    ComponentRef getComponentRef(EntityId eid, archetype_component_id cid) const;   // cid is 0.. till getNumComponents
    ComponentRef getComponentRefCidx(EntityId eid, component_index_t cidx) const;

    inline bool getEntityArchetype(EntityId eid, int &idx, archetype_t &archetype) const;

    int getNumComponents(EntityId eid) const;

    void collectComponentInfo(EntityId eid, std::vector<std::pair<ComponentInfo*, DataComponent*>> &comps);

    std::string_view getEntityTemplateName(EntityId &eid) {
      template_t t = getEntityTemplateId(eid);
      if (t == INVALID_TEMPLATE_INDEX)
        return "";
      return this->data_state->getTemplateName(t);
    }

    void debugPrintEntity(EntityId eid);

    void debugPrintEntities();

    void collectEntitiesWithComponent(std::vector<EntityId> &out, HashedConstString component);

    void collectEntitiesOfTemplate(std::vector<EntityId> &out, template_t template_id);

    void collectEntitiesOfTemplate(std::vector<EntityId> &out,
                                   std::string_view templ_name) { return collectEntitiesOfTemplate(out,
                                                                                                   this->data_state->getTemplateIdByName(
                                                                                                       templ_name));
    }

    EntityId getNext() { return this->entDescs.GetNextEid(); }

    void sendEventImmediate(EntityId eid, Event &evt);

    void broadcastEventImmediate(Event &evt);

    void sendEventImmediate(EntityId eid, Event &&evt);

    void broadcastEventImmediate(Event &&evt);

    ecs::EntityId getUnit(uint16_t uid);

  protected:

    friend Component;
    friend InstantiatedTemplate;
    friend GState;

    GState *data_state = g_ecs_data.get();
    EntityDescs entDescs;
    BitVector wasInit{false}; // used during entity creation
    MgrArchetypeStorage arch_data; // EntityManager now only owns raw entity storage

  };


}

#endif //MYEXTENSION_ENTITYMANAGER_H
