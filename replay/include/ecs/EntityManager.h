

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

namespace ecs {
  static inline entity_id_t make_eid(uint32_t index, uint32_t gen) { return index | (gen << ENTITY_INDEX_BITS); }

  class GState {
    ComponentTypes componentTypes{};
    DataComponents dataComponents{};
    TemplateDB templates{};

  public:
    GState() {Init();}

    void Init() {
      this->componentTypes.initialize();
      dataComponents.initialize(componentTypes);

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


    inline component_index_t
    createComponent(const HashedConstString name, type_index_t component_type, ComponentSerializer *io) {
      return this->dataComponents.createComponent(name, component_type, io, this->componentTypes);
    }

    friend EntityManager;
    friend Component;
    friend InstantiatedTemplate;
  };

  extern OnDemandInit<GState> g_ecs_data;


  class EntityManager {

  public:
    EntityManager();
    ~EntityManager();
    inline bool doesEntityExist(EntityId e) const { return entDescs.doesEntityExist(e); }

    template_t getEntityTemplateId(EntityId &eid) {
      template_t t;
      entDescs.getEntityTemplateId(eid, t);
      return t;
    }



    void* getNullable(EntityId eid, component_index_t index, archetype_t &archetype) const;
    void* getNullableUnsafe(EntityId eid, component_index_t index, archetype_t &archetype) const;
    template <class T>
    T* getNullable(EntityId eid, HashedConstString component) {
      if(!this->entDescs.doesEntityExist(eid))
        return nullptr;
      auto cidx = this->data_state->dataComponents.getIndex(component.hash);
      if(cidx == INVALID_COMPONENT_INDEX)
        return nullptr;
      //auto comp = this->data_state->dataComponents.getDataComponent(cidx);
      archetype_t arch = INVALID_ARCHETYPE;
      return (T*)this->getNullableUnsafe(eid, cidx, arch);
    }

    template_t buildTemplateIdByName(const char *templ_name);
    void instantiateTemplate(template_t t);
    /// validates that an initializer
    inline bool validateInitializer(template_t templId, ComponentsInitializer &comp_init);
    EntityId createEntity(EntityId eid, template_t templId, ComponentsInitializer &&initializer);
    bool destroyEntity(EntityId eid);
    ComponentRef getComponentRef(EntityId eid, archetype_component_id cid) const;   // cid is 0.. till getNumComponents
    ComponentRef getComponentRefCidx(EntityId eid, component_index_t cidx) const;

    inline bool getEntityArchetype(EntityId eid, int &idx, archetype_t &archetype) const;
    int getNumComponents(EntityId eid) const;

    std::string_view getEntityTemplateName(EntityId &eid) {
      template_t t = getEntityTemplateId(eid);
      if (t == INVALID_TEMPLATE_INDEX)
        return "";
      Template *templ = this->data_state->templates[t];
      return templ->name;
    }

    void debugPrintEntity(EntityId eid);

    void debugPrintEntities();

    void collectEntitiesWithComponent(std::vector<EntityId> &out, HashedConstString component);

    void collectEntitiesOfTemplate(std::vector<EntityId> &out, template_t template_id);

    void collectEntitiesOfTemplate(std::vector<EntityId> &out, std::string_view templ_name) {return collectEntitiesOfTemplate(out, this->data_state->templates.getTemplateIdByName(templ_name));}

    archetype_t createArchetype(InstantiatedTemplate *inst)
    {
      auto it = this->initialized_archetypes.find(inst);
      if(it == this->initialized_archetypes.end())
      {
        archetype_t arch = this->archetypes.createArchetype(inst->component_indexes.data(), (uint32_t)inst->component_indexes.size(), data_state->dataComponents, data_state->componentTypes);;
        this->initialized_archetypes.emplace(inst, arch);
        return arch;
      }
      return it->second;
    }

    EntityId getNext() {return this->entDescs.GetNextEid();}

    typedef uint64_t query_components_hash;
    // thorough hash of RW, RO, RQ, and NO of a BaseQueryDesc
    static query_components_hash query_components_hash_calc(const BaseQueryDesc &desc);
    QueryId createUnresolvedQuery(const NamedQueryDesc &desc);
    QueryId createQuery(const NamedQueryDesc &desc);
    bool resolvePersistentQueryInternal(uint32_t index);

  protected:

    enum ResolvedStatus : uint8_t
    {
      NOT_RESOLVED = 0,
      FULLY_RESOLVED = 1,
      RESOLVED = 2,
      RESOLVED_MASK = 3
    };

    uint32_t addOneQuery();
    bool isQueryValidGen(QueryId id) const
    {
      if (!id)
        return false;
      auto idx = id.index();
      return idx < queriesGenerations.size() && queriesGenerations[idx] == id.generation();
    }

    friend Component;
    friend InstantiatedTemplate;

    GState *data_state = g_ecs_data.get();
    EntityDescs entDescs;
    Archetypes archetypes;
    BitVector wasInit{false}; // used during entity creation
    std::unordered_map<InstantiatedTemplate*, archetype_t> initialized_archetypes; // because mgr no longer owns templates, we need do this
    std::unordered_map<event_type_t, std::vector<QueryId>> event_to_query; // maps an event to what queries use it
    EventsDB db; // TODO: move to g_ecs_data
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
    typedef uint32_t status_word_type_t;
    static constexpr status_word_type_t status_words_shift = get_const_log2(sizeof(status_word_type_t) * 4),
        status_words_mask = (1 << status_words_shift) - 1;
    // this stores ResolvedStatus with two bits in a vector, so does some funny bit packing
    std::vector<status_word_type_t> resolvedQueryStatus; // SoA, two-bit vector
    // SoA for QueryId
    uint8_t currentQueryGen = 0;

    static bool isResolved(ResolvedStatus s) { return s != NOT_RESOLVED; }
    ResolvedStatus getQueryStatus(uint32_t idx) const
    {
      const status_word_type_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
      return (ResolvedStatus)((resolvedQueryStatus[wordIdx] >> wordShift) & RESOLVED_MASK);
    }
    inline bool isResolved(uint32_t index) const { return isResolved(getQueryStatus(index)); }
    void orQueryStatus(uint32_t idx, ResolvedStatus status)
    {
      const uint32_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
      resolvedQueryStatus[wordIdx] |= (status << wordShift);
    }
    void resetQueryStatus(uint32_t idx)
    {
      const status_word_type_t wordIdx = idx >> status_words_shift, wordShift = (idx & status_words_mask) << 1;
      resolvedQueryStatus[wordIdx] &= ~(status_word_type_t(RESOLVED_MASK) << wordShift);
    }
    void addOneResolvedQueryStatus()
    {
      uint32_t sz = (uint32_t)(resolvedQueries.size() + status_words_mask) >> status_words_shift;
      if (sz != resolvedQueryStatus.size())
        resolvedQueryStatus.push_back(status_word_type_t(0));
    }
    ResolvedStatus resolveQuery(uint32_t index, ResolvedStatus currentStatus, ResolvedQueryDesc &resDesc);
  };



}

#endif //MYEXTENSION_ENTITYMANAGER_H
