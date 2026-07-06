#include "ecs/EntityManager.h"
#include "ecs/ComponentTypesDefs.h"
#include "network/CNetwork.h"
#include "network/message.h"
#include "tracy/Tracy.hpp"
#include "state/ParserState.h"

CREATE_HANDLE(handle_ecs, "EntityManager")
CREATE_HANDLE(handle_entity, "EntityHandling")
namespace ecs {
  OnDemandInit<std::vector<after_components_cb>> after_comps_callbacks{};
  OnDemandInit<GState> g_ecs_data{};


  void EntityCreatedAction::forward(EntityManager &mgr) {
    //LOGI("EntityCreatedAction::forward: {}", this->time_ms);
    G_ASSERT(this->last_direction == DIRECTION::Rewind);
    mgr.swap_desc(before, after);
    auto ptr = mgr.getNullable<ecs::EntityId>(after, ECS_HASH("eid"));
    G_ASSERT(ptr);
    *ptr = after;
    mgr.sendEventImmediate(after, EventEntityCreatedBasic{});
    last_direction = DIRECTION::Fastforward;
  }

  void EntityCreatedAction::backward(EntityManager &mgr) {
    //LOGI("EntityCreatedAction::backward: {}", this->time_ms);
    G_ASSERT(this->last_direction == DIRECTION::Fastforward);
    auto ptr = mgr.getNullable<ecs::EntityId>(after, ECS_HASH("eid"));
    G_ASSERT(ptr);
    *ptr = before;
    mgr.sendEventImmediate(after, EventEntityDestroyedBasic{before, false});
    mgr.swap_desc(before, after);
    last_direction = DIRECTION::Rewind;
  }

  void EntityDestroyedAction::forward(EntityManager &mgr) {
    //LOGI("EntityDestroyedAction::forward: {}", this->time_ms);
    G_ASSERT(this->last_direction == DIRECTION::Rewind);
    auto ptr = mgr.getNullable<ecs::EntityId>(after, ECS_HASH("eid"));
    G_ASSERT(ptr);
    *ptr = before;
    mgr.sendEventImmediate(after, EventEntityDestroyedBasic{before, true});
    mgr.swap_desc(before, after);
    last_direction = DIRECTION::Fastforward;
  }

  void EntityDestroyedAction::backward(EntityManager &mgr) {
    //LOGI("EntityDestroyedAction::backward: {}", this->time_ms);
    G_ASSERT(this->last_direction == DIRECTION::Fastforward);
    mgr.swap_desc(before, after);
    auto ptr = mgr.getNullable<ecs::EntityId>(after, ECS_HASH("eid"));
    G_ASSERT(ptr);
    *ptr = after;
    mgr.sendEventImmediate(after, EventEntityCreatedBasic{});
    last_direction = DIRECTION::Rewind;
  }

  ComponentUpdateAction::~ComponentUpdateAction() {
    auto data_comp = g_ecs_data->getDataComponents()->getDataComponent(cidx);
    auto comp = g_ecs_data->getComponentTypes()->getComponentData(data_comp->componentIndex);

    if (is_pod(comp->flags)) {
      free(ptr);
      return;
    } else {
      comp->ctm->destroy(ptr);
      free(ptr);
    }
  }

  // swap is inherently reversible, so the only difference between these is the assert
  void ComponentUpdateAction::forward(EntityManager &mgr) {
    G_ASSERT(this->last_direction == DIRECTION::Rewind);
    auto ref = mgr.getComponentRefCidx(eid, cidx);
    G_ASSERT(!ref.isNull());
    ref.swap(ptr);
    last_direction = DIRECTION::Fastforward;
  }

  void ComponentUpdateAction::backward(EntityManager &mgr) {
    G_ASSERT(this->last_direction == DIRECTION::Fastforward);
    auto ref = mgr.getComponentRefCidx(eid, cidx);
    G_ASSERT(!ref.isNull());
    ref.swap(ptr);
    last_direction = DIRECTION::Rewind;
  }

  void RewindManager::rewindTo(uint32_t time_ms, EntityManager &mgr) {
    const int sz = (int) actions.size();
    if (sz == 0)
      return;

    // curr_index in [0, sz]:
    //   0   = no actions applied
    //   sz  = all actions applied
    //   k   = actions [0, k-1] applied, action k is next to apply forward
    curr_index = std::clamp(curr_index, 0, sz);
    auto curr_time = getTime(curr_index - 1);
    size_t test_size = 0;
    if (curr_time < time_ms) {
      auto iter = std::upper_bound(actions.begin() + curr_index, actions.end(), time_ms,
                                   [](uint32_t val, const ACTION_ARRAY_CONTAINER &data) {
                                     auto action = (RewindAction *) data.data();
                                     return val < action->time_ms;
                                   });
      test_size = std::distance(actions.begin(), iter);
    } else {
      auto iter = std::lower_bound(actions.begin(), actions.begin() + curr_index, time_ms,
                                   [](const ACTION_ARRAY_CONTAINER &data, uint32_t val) {
                                     auto action = (RewindAction *) data.data();
                                     return action->time_ms < val;
                                   });
      test_size = std::distance(actions.begin(), iter);
    }

    // Undo actions whose time is strictly after target: action[curr_index-1] was last applied
    while (curr_index > 0 && getTime(curr_index - 1) > time_ms) {
      --curr_index;
      *mgr.curr_time_ms = getTime(curr_index - 1);
      getAction(curr_index)->backward(mgr);
    }

    // Apply actions whose time is within target
    while (curr_index < sz && getTime(curr_index) <= time_ms) {
      *mgr.curr_time_ms = getTime(curr_index - 1);
      getAction(curr_index)->forward(mgr);
      ++curr_index;
    }
    G_ASSERT(test_size == curr_index);
    mgr.broadcastEventImmediate(EventRewind{time_ms});
  }

  CompileTimeQueryDesc *CompileTimeQueryDesc::tail = nullptr;

  void GState::initCompileTimeQueries() {
    for (CompileTimeQueryDesc *sd = CompileTimeQueryDesc::tail; sd; sd = sd->next)
      sd->query = createUnresolvedQuery(*sd);
  }

  void EntityManager::swap_desc(EntityId e1, EntityId e2) {
    auto &destroyed_desc = entDescs[e1];
    auto &created_desc = entDescs[e2];
    std::swap(destroyed_desc, created_desc);
  }

  EntityManager::EntityManager(ParserState*owned_by) {
    this->owned_by = owned_by;
    this->curr_time_ms = &owned_by->curr_time_ms;
    // componentTypes and dataComponents initalzied in initialize() in /init/initialze.h
    wasInit.resize(10000, false);
  }

  inline const EntityDesc *EntityDescs::getEntityDesc(EntityId eid) const {
    auto idx = eid.index();
    if (idx < entDescs.size()) {
      return &entDescs[idx];
    }
    return nullptr;
  }

  inline EntityDesc *EntityDescs::getEntityDesc(EntityId eid) {
    if (!this->doesEntityExist(eid))
      return nullptr;

    return &entDescs[eid.index()];
  }

  bool EntityDescs::getEntityTemplateId(EntityId eid, template_t &tmpl) const {
    auto desc = this->getEntityDesc(eid);
    if (desc) {
      tmpl = desc->templ_id;
      return true;
    }
    tmpl = INVALID_TEMPLATE_INDEX;
    return false;
  }

  bool EntityDescs::getEntityArchetypeId(EntityId eid, archetype_t &archetype) const {
    auto desc = this->getEntityDesc(eid);
    if (desc) {
      archetype = desc->archetype_id;
      return true;
    }
    archetype = INVALID_ARCHETYPE;
    return false;
  }

  bool EntityDescs::getEntityChunkId(EntityId eid, chunk_index_t &chunk) const {
    auto desc = this->getEntityDesc(eid);
    if (desc) {
      chunk = desc->chunk_id;
      return true;
    }
    chunk = INVALID_CHUNK_INDEX_T;
    return false;
  }

  EntityId EntityManager::createEntity(EntityId eid, template_t templId, ComponentsInitializer &&initializer) {
    this->wasInit.clear();
    validateInitializer(templId, initializer); // ensures the initializer has cIndex populated
    bool isRecreating = this->entDescs[eid].archetype_id != INVALID_ARCHETYPE;
    archetype_t archetype_id = data_state->EnsureArchetype(templId, this->arch_data);
    chunk_index_t chunk_id;
    InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(templId);
    {
      G_ASSERTF(instTempl, "Template {} not initialized", data_state->getTemplateName(templId));
      ENTITY_LOGD2("Creating new entity {} of template '{}' at {}", eid,
                   data_state->templates.getTemplate(templId)->getName(), ((double) *this->curr_time_ms) / 1000);
      auto arch_inst = this->arch_data.getArch(archetype_id);
      auto info = this->arch_data.getStorageArch(archetype_id);
      chunk_id = arch_inst->getNextAvailableChunkId();
      auto t = arch_inst->reserveChunkId(chunk_id);
      G_ASSERT(t);
      auto archInfo = &info->INFO;
      auto ComponentInfo = info->components.data();
      if(isRecreating) {
        auto &old_desc = this->entDescs[eid];
        archetype_t new_arch_id = archetype_id;
        archetype_t old_arch_id = old_desc.archetype_id;
        auto &new_ARCHETYPE = *this->arch_data.getArch(new_arch_id);
        auto &new_info = *this->arch_data.getStorageArch(new_arch_id);
        auto &new_archInfo = *archInfo;
        //auto &new_ComponentInfo = data_state->archetypes.archetypeComponents[new_info.COMPONENT_OFS];

        auto &old_ARCHETYPE = *this->arch_data.getArch(old_arch_id);
        auto &old_info = *this->arch_data.getStorageArch(old_arch_id);
        //auto &old_archInfo = old_info.INFO;
        auto &old_ComponentInfo = old_info.components.front();

        for (auto comp_info = &old_ComponentInfo; comp_info != &old_ComponentInfo + old_info.COMPONENT_COUNT; comp_info++) {
          auto old_data = old_ARCHETYPE.getCompDataUnsafe(comp_info->DATA_OFFSET, old_desc.chunk_id, comp_info->DATA_SIZE);
          auto old_data_comp = data_state->dataComponents.getDataComponent(comp_info->INDEX);
          auto old_comp = data_state->componentTypes.getComponentData(old_data_comp->componentIndex);

          ComponentRef ref{old_data, old_comp->hash, old_data_comp->componentIndex, old_comp->size};
          archetype_component_id id = new_archInfo.getComponentId(comp_info->INDEX);
          // new template has the component
          if (id != INVALID_ARCHETYPE_COMPONENT_ID) {
            auto curr_info = ComponentInfo[id];
            auto new_data = new_ARCHETYPE.getCompDataUnsafe(curr_info.DATA_OFFSET, chunk_id, curr_info.DATA_SIZE);
            ref.move(new_data, old_data);
          }
          // we always destroy the old data.
          ref.destructCopy(old_data);
          // maybe debuglevel here?
          memset(old_data, 0xFF, old_comp->size);

          this->wasInit.set(comp_info->INDEX, true);
        }

        *this->getNullable<ecs::EntityId>(eid, ECS_HASH("eid")) = ecs::INVALID_ENTITY_ID;
        old_ARCHETYPE.releaseChunkId(old_desc.chunk_id);
      }

      // setup entity with initialized data
      for (auto &comp: initializer) {
        archetype_component_id id = archInfo->getComponentId(comp.cIndex);
        G_ASSERT(id != INVALID_ARCHETYPE_COMPONENT_ID); // component exists for us
        auto curr_info = ComponentInfo[id];
        auto data = arch_inst->getCompDataUnsafe(curr_info.DATA_OFFSET, chunk_id, curr_info.DATA_SIZE);
        comp.second.getComponentRef().move(data, comp.second.value); // TODO, how does gaijin do it???
        this->wasInit.set(comp.cIndex, true);
      }

      // now setup any remaining components with default data
      for (auto &comp: instTempl->components) {

        //LOG("instTempl trying id %i\n", comp.comp_type_index);
        if (!this->wasInit.test(comp.comp_type_index, false)) {
          //LOG("succeeded\n");
          archetype_component_id id = archInfo->getComponentId(comp.comp_type_index);

          auto curr_info = ComponentInfo[id];
          G_ASSERT(curr_info.INDEX == comp.comp_type_index);
          auto data = arch_inst->getCompDataUnsafe(curr_info.DATA_OFFSET, chunk_id, curr_info.DATA_SIZE);
          comp.default_component.createCopy(data, this, eid, comp.comp_type_index);
        }
      }
    }

    auto storage_eid = this->allocateOneEid(); // (int16_t)eid.get_generation()

    auto idx = this->rewindManager.createCreationAction(*curr_time_ms, storage_eid, eid);
    eidToEventCreationMap[eid] = idx;
    this->entDescs.Allocate(eid);
    this->entDescs[eid.index()] = {templId, archetype_id, eid.generation(), chunk_id};
    this->wasInit.clear();
    if(!isRecreating) {
      this->sendEventImmediate(eid, ecs::EventEntityCreated{});
      this->sendEventImmediate(eid, ecs::EventEntityCreatedBasic{});
    }
    return eid;
  }

  bool EntityManager::validateInitializer(template_t templId, ComponentsInitializer &comp_init) {
    // more stuff will be done here eventually
    for (auto &initIt: comp_init) {
      if (initIt.cIndex == INVALID_COMPONENT_INDEX) {
        component_index_t initializerIndex = data_state->dataComponents.getIndex(initIt.name);
        G_ASSERTF(initializerIndex != INVALID_COMPONENT_INDEX, "Invalid initializer index hash:{:#x}\n", initIt.name);
        G_ASSERT(data_state->dataComponents.getDataComponent(initializerIndex)->componentHash ==
                 initIt.second.getUserType());
        if (DAGOR_UNLIKELY(initializerIndex == INVALID_COMPONENT_INDEX))
        EXCEPTION("Invalid component of name {:#x}", initIt.name);
        initIt.cIndex = initializerIndex;
      }
    }
    return true;
  }

  template_t EntityManager::buildTemplateIdByName(const char *templ_name) {
    return this->data_state->templates.buildTemplateIdByName(templ_name);
  }

  void EntityManager::instantiateTemplate(template_t t) {
    this->data_state->templates.instantiateTemplate(t);
  }

  bool EntityManager::destroyEntity(EntityId eid, bool is_dtor, bool force_destroy) {
    if (!this->doesEntityExist(eid))
      return false;

    auto desc = this->entDescs.getEntityDesc(eid);

    if (!force_destroy && !is_dtor && !eidsReservationMode && MoveServerDestroyedEntities && eid.index() <=
        RESERVED_EID_RANGE) {
      G_ASSERT(!this->entDescs.basic_destroyed.test(eid.index(), false));
      //auto new_eid = this->allocateOneEid();
      auto creation_action = (EntityCreatedAction*)this->rewindManager.getState(eidToEventCreationMap[eid]).data.data();
      auto new_eid = creation_action->before;
      G_ASSERT(new_eid);
      eidToEventCreationMap.erase(eid);
      sendEventImmediate(eid, EventEntityDestroyedBasic{new_eid, true});
      *this->getNullable<ecs::EntityId>(eid, ECS_HASH("eid")) = new_eid;
      ENTITY_LOGD2("Moving eid: {} of template {} to {}", eid, this->data_state->getTemplateName(desc->templ_id),
                   new_eid);
      swap_desc(eid, new_eid);
      //add_sub_template(new_eid, "dagor_destroyed_t");
      this->entDescs.basic_destroyed.set(new_eid.index(), true);
      this->rewindManager.createDestroyAction(*curr_time_ms, new_eid, eid);
      return true;
    }

    if(!this->entDescs.basic_destroyed.test(eid.index(), false))
      sendEventImmediate(eid, EventEntityDestroyedBasic{ecs::INVALID_ENTITY_ID, true});

    sendEventImmediate(eid, EventEntityDestroyed{});
    if (is_dtor) {
      ENTITY_LOGD3("Destroying entity {} of template {}", eid,
            data_state->templates.getTemplate(desc->templ_id)->getName());
    } else {
      ENTITY_LOGD2("Destroying entity {} of template {}", eid,
                   data_state->templates.getTemplate(desc->templ_id)->getName());
    }

    //const InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(desc->templ_id);
    archetype_t archetype_id = desc->archetype_id;
    auto ARCHETYPE = this->arch_data.getArch(archetype_id);
    auto info = this->arch_data.getStorageArch(archetype_id);
    auto ComponentInfo = info->components.data();
    //auto archInfo = &info->INFO;
    //if (eid.handle == 0x4008a3) {
    //  LOG("WOMP");
    //  g_log_handler.wait_until_empty();
    //  g_log_handler.flush_all();
    //  this->debugPrintEntity(desc->eid);
    //}
    //this->debugPrintEntity(desc->eid);
    //LOG("archetype datacomponents:");
    //for(auto comp_info = ComponentInfo; comp_info != ComponentInfo+info->COMPONENT_COUNT; comp_info++)
    //{
    //  LOG("%i ", comp_info->INDEX);
    //}
    //LOG("\n");
    for (auto comp_info = ComponentInfo; comp_info != ComponentInfo + info->COMPONENT_COUNT; comp_info++) {
      auto data = ARCHETYPE->getCompDataUnsafe(comp_info->DATA_OFFSET, desc->chunk_id, comp_info->DATA_SIZE);
      auto dataComp = data_state->dataComponents.getDataComponent(comp_info->INDEX);
      auto comp = data_state->componentTypes.getComponentData(dataComp->componentIndex);
      //LOG("Destroying component {}({})(compid: {}) of entity {:#x} of template '{}' at address {} in chunk {}",
      //    dataComp->getName(),
      //    comp->name,
      //    comp_info->INDEX,
      //    desc->eid.handle,
      //    g_ecs_data->templates.getTemplate(desc->templ_id)->name.data(), fmt::ptr(data), desc->chunk_id);
      //LOG("\nRaw Data: 0x");
      //auto charPtr = (const char *)data;
      //for(int i = 0; i < comp->size; i++)
      //{
      //  LOG("%02X", charPtr[i]);
      //}
      //LOG("\n");
      //std::cout.flush();
      ComponentRef ref{data, comp->hash, dataComp->componentIndex, comp->size};
      //if(eid.handle == 0x4008a3 && strcmp(dataComp->getName().data(), "skeleton_attach__remapParentSlots") == 0) {
      //  ref.print(nullptr);
      //  g_log_handler.wait_until_empty();
      //  g_log_handler.flush_all();
      //  LOG("");
      //}

      ref.destructCopy(data);
      if (dataComp->hash == ECS_HASH("eid").hash)
        *(ecs::EntityId *) data = INVALID_ENTITY_ID; // needed for query system
    }
    /*for(const auto &comp : instTempl->components)
    {
      archetype_component_id id = archInfo->getComponentId(comp.comp_type_index);

      auto data =info->ARCHETYPE.getCompDataUnsafe(comp_info->DATA_OFFSET, desc->chunk_id, comp_info->DATA_SIZE);
      auto curr_info = ComponentInfo[id];
      auto data =info->ARCHETYPE.getCompDataUnsafe(curr_info.DATA_OFFSET, desc->chunk_id, curr_info.DATA_SIZE);

      //comp.default_component.print(data, &componentTypes);
      //LOG("\n");
      //if( componentTypes.getName(comp.default_component.getTypeId()) == "ecs::string")
      //  std::cout << "found it 2\n";
      comp.default_component.destructCopy(data, &this->componentTypes); // destructs the component, doesnt free
    }*/
    ARCHETYPE->releaseChunkId(desc->chunk_id);
    desc->templ_id = INVALID_TEMPLATE_INDEX;
    desc->archetype_id = INVALID_ARCHETYPE;
    desc->chunk_id = INVALID_CHUNK_INDEX_T;
    desc->generation++;
    if(!is_dtor) {
      auto findices = (eid.index() <= RESERVED_EID_RANGE) ? ((eid.index() < nextReservedIndex) ? &freeIndicesReserved : nullptr)
                                                                  : &freeIndices;
      if(findices)
        findices->push_back((entity_id_t)eid);
    }

    return true;
  }

  ecs::EntityManager::~EntityManager() {
    ZoneScoped;
    ECS_LOGD3("starting EntityManager Destruction");
    for (int i = 1; i < this->entDescs.entDescs.size(); i++) {
      if (!this->entDescs.doesEntityExistInternal(i))
        continue;
      auto &desc = this->entDescs[i];
      destroyEntity(EntityId(make_eid(i, desc.generation)), true); //
    }
    ECS_LOGD3("finished EntityManager Destruction");
    //g_log_handler.wait_until_empty();
    //g_log_handler.flush_all();
  }

  void EntityManager::debugPrintEntities() {
    for (entity_id_t i = 0; i < this->entDescs.entDescs.size(); i++) {

      if (this->entDescs.doesEntityExistInternal(i)) {
        EntityId eid{make_eid(i, this->entDescs[i].generation)};
        this->debugPrintEntity(eid);
      }
    }
  }

  void EntityManager::debugPrintEntity(EntityId eid) {
    if (this->doesEntityExist(eid)) {
      auto desc = this->entDescs.getEntityDesc(eid);
      //const InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(desc->templ_id);

      archetype_t archetype_id = desc->archetype_id;
      auto ARCHETYPE = this->arch_data.getArch(archetype_id);
      auto info = this->arch_data.getStorageArch(archetype_id);
      auto ComponentInfo = info->components.data();
      // auto archInfo = &info->INFO;
      LOG("DebugPrint of Entity {} of template '{}' of archetype_id {}", eid,
          this->data_state->templates.getTemplate(desc->templ_id)->getName(), archetype_id);
      for (auto comp_info = ComponentInfo; comp_info != ComponentInfo + info->COMPONENT_COUNT; comp_info++) {
        //     ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint16_t size);
        auto data = ARCHETYPE->getCompDataUnsafe(comp_info->DATA_OFFSET, desc->chunk_id, comp_info->DATA_SIZE);
        auto dataComp = data_state->dataComponents.getDataComponent(comp_info->INDEX);
        //if(strcmp(dataComp->getName().data(), "skeleton_attach__remapParentSlots") == 0 && eid.handle == 0x4008a3)
        //  LOG("WOMP");
        auto comp = data_state->componentTypes.getComponentData(dataComp->componentIndex);
        ComponentRef ref{data, comp->hash, dataComp->componentIndex, comp->size};
        LOG("  ArchData: idx: {}; data_off: {}; chunk_id: {}; data_size: {}; ptr: {}", comp_info->INDEX,
            comp_info->DATA_OFFSET, (uint32_t) desc->chunk_id, comp_info->DATA_SIZE, fmt::ptr(data));
        LOG("  component {}({}) data: {}", dataComp->getName().data(), comp->name.data(), ref.toString());
      }
      LOG("");
    }
  }

  void *EntityManager::getNullable(EntityId eid, component_index_t index, archetype_t &archetype) const {
    ZoneScoped;
    if (!this->entDescs.doesEntityExist(eid))
      return nullptr;
    auto desc = this->entDescs[eid.index()];
    archetype = desc.archetype_id; // should always be valid
    G_ASSERT(archetype != INVALID_ARCHETYPE);
    G_ASSERT(this->arch_data.archetypeStorages[archetype]->INFO.getComponentId(index) != INVALID_COMPONENT_INDEX);
    return this->arch_data.getComponentDataUnsafe(archetype, index, desc.chunk_id);
  }

  void *EntityManager::getNullableUnsafe(EntityId eid, component_index_t index, archetype_t &archetype) const {
    ZoneScoped;
    G_ASSERT(this->entDescs.doesEntityExist(eid)); // sanity check in dev only
    auto desc = this->entDescs[eid.index()];
    archetype = desc.archetype_id; // should always be valid
    G_ASSERTF(archetype != INVALID_ARCHETYPE, "Entity {} is invalid", eid);
    if (this->arch_data.archetypeStorages[archetype]->INFO.getComponentId(index) == INVALID_COMPONENT_INDEX)
      return nullptr;

    return this->arch_data.getComponentDataUnsafe(archetype, index, desc.chunk_id);
  }

  bool EntityManager::entityHasComponent(EntityId eid, component_index_t index) const {
    G_ASSERT(this->entDescs.doesEntityExist(eid)); // sanity check in dev only
    auto desc = this->entDescs[eid.index()];
    archetype_t archetype = desc.archetype_id; // should always be valid
    G_ASSERT(archetype != INVALID_ARCHETYPE);
    //G_ASSERT(data_state->archetypes.archetypes[archetype].INFO.getComponentId(index) != INVALID_COMPONENT_INDEX);
    if (this->arch_data.archetypeStorages[archetype]->INFO.getComponentId(index) == INVALID_COMPONENT_INDEX)
      return false;
    return true;
  }

  __forceinline bool EntityManager::getEntityArchetype(EntityId eid, int &idx, archetype_t &archetype) const {
    const bool ret = entDescs.getEntityArchetypeId(eid, archetype);
    if (ret) {
      //DAECS_VALIDATE_ARCHETYPE(archetype);
    }
    return ret;
  }

  int EntityManager::getNumComponents(EntityId eid) const {
    int idx;
    archetype_t archetype = INVALID_ARCHETYPE;
    if (!getEntityArchetype(eid, idx, archetype))
      return -1;
    return this->arch_data.archetypeStorages[archetype]->COMPONENT_COUNT-1;
  }

  ComponentRef EntityManager::getComponentRef(EntityId eid, archetype_component_id cid) const {

    auto desc = this->entDescs.getEntityDesc(eid);
    if (!desc)
      return {};
    auto data = this->arch_data.getComponentDataIdUnsafe(desc->archetype_id, cid,
                                                                desc->chunk_id);
    if (!data)
      return {};
    auto cidx = this->arch_data.getComponentUnsafe(desc->archetype_id, cid);
    //ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint16_t size);
    auto datacomp_data = data_state->dataComponents.getDataComponent(cidx);
    if (!datacomp_data)
      return {};

    return {data, datacomp_data->componentHash, datacomp_data->componentIndex,
            data_state->componentTypes.types[datacomp_data->componentIndex].size, cidx};
  }

  ComponentRef EntityManager::getComponentRefCidx(EntityId eid, component_index_t cidx) const {

    auto desc = this->entDescs.getEntityDesc(eid);
    if (!desc)
      return {};
    void *data;
    {
      data = this->arch_data.getComponentDataUnsafe(desc->archetype_id, cidx,
                                                           desc->chunk_id);
    }
    if (!data)
      return {};
    //auto cidx = this->archetypes.getComponentUnsafe(desc->archetype_id, cid);
    //ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint16_t size);
    auto datacomp_data = data_state->dataComponents.getDataComponent(cidx);
    if (!datacomp_data)
      return {};

    return {data, datacomp_data->componentHash, datacomp_data->componentIndex,
            data_state->componentTypes.types[datacomp_data->componentIndex].size, cidx};
  }

  void EntityManager::sendEventImmediate(EntityId eid, Event &evt) {
    ZoneScoped;
    if (!this->entDescs.doesEntityExist(eid))
    EXCEPTION("tried to send a query to an entity that doesnt exist");
    this->data_state->sendEventImmediate(eid, evt, *this);
  }

  void EntityManager::broadcastEventImmediate(Event &evt) {
    ZoneScoped;
    this->data_state->broadcastEventImmediate(evt, *this);
  }

  void EntityManager::sendEventImmediate(EntityId eid, Event &&evt) {
    return sendEventImmediate(eid, evt);
  }

  void EntityManager::broadcastEventImmediate(Event &&evt) {
    return broadcastEventImmediate(evt);
  }

  void
  EntityManager::collectComponentInfo(EntityId eid, std::vector<std::pair<ComponentInfo *, DataComponent *>> &comps) {
    int num_components = this->getNumComponents(eid);
    if (num_components == -1) {
      comps.resize(0);
      return;
    }
    template_t t = this->getEntityTemplateId(eid);
    G_ASSERT(t!=INVALID_TEMPLATE_INDEX);
    std::shared_lock arch_lock(this->data_state->templates.template_mtx);
    auto inst = this->data_state->templates.getInstTemplate(t);
    for(auto &comp : inst->components) {
      auto d = this->data_state->dataComponents.getDataComponent(comp.comp_type_index); // datacomponent
      auto c = this->data_state->componentTypes.getComponentData(d->componentIndex); // component
      comps.emplace_back(c, d);
    }
  }

  EntityId EntityManager::allocateOneEid(int16_t generation) {

    bool reserved = eidsReservationMode;
    auto &freed_deque = reserved ? freeIndicesReserved : freeIndices;
    unsigned idx;
    if(!freed_deque.empty()) {
      alloc_idx:
      idx = freed_deque.front();
      G_ASSERT(idx < entDescs.size());
      freed_deque.pop_front();
    } {
      if(!reserved) {
        idx = entDescs.push_back();
      }
      else if(DAGOR_LIKELY(nextReservedIndex <= RESERVED_EID_RANGE))
        idx = nextReservedIndex++;
      else {
        goto alloc_idx;
      }
    }
    G_ASSERT(generation <= 255);
    auto gen = generation >= 0 ? (uint8_t) generation : entDescs[idx].generation;
    auto eid = EntityId(make_eid(idx, gen));
    this->entDescs.Allocate(eid);
    return eid;
  }

  void EntityManager::add_sub_template(ecs::EntityId eid, const string &sub_template) {
    G_ASSERT(this->entDescs.doesEntityExist(eid));
    template_t sub_templ = g_ecs_data->getTemplateIdByName(sub_template);
    G_ASSERT(sub_templ);

    auto &desc = this->entDescs[eid];
    auto templ = g_ecs_data->getTemplateDB()->getTemplate(desc.templ_id);
    // entity already has this sub template
    if(std::find(templ->getParents().begin(), templ->getParents().end(), sub_templ) != templ->getParents().end())
      return;
    // entity doesn't have it, lets get our new template and recreate entity
    std::string combined_template = fmt::format("{}+{}", templ->getName(), sub_template);

    auto new_templ = g_ecs_data->getTemplateDB()->buildTemplateIdByName(combined_template.c_str());
    g_ecs_data->getTemplateDB()->instantiateTemplate(new_templ);
    this->createEntity(eid, new_templ, {});
  }
}

