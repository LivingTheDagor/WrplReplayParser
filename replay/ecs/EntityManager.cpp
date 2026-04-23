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

  CompileTimeQueryDesc *CompileTimeQueryDesc::tail = nullptr;

  void GState::initCompileTimeQueries() {
    for (CompileTimeQueryDesc *sd = CompileTimeQueryDesc::tail; sd; sd = sd->next)
      sd->query = createUnresolvedQuery(*sd);
  }

  EntityManager::EntityManager(ParserState*owned_by) {
    this->owned_by = owned_by;
    this->curr_time_ms = &owned_by->curr_time_ms;
    // componentTypes and dataComponents initalzied in initialize() in /init/initialze.h
    for (auto &eid: this->uid_lookup) {
      eid = ecs::INVALID_ENTITY_ID;
    }
    wasInit.resize(10000,
                   false); // just in case, current max datacomponents was like 950; edit: its now like 7000 cause of infantry and shit
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
    archetype_t archetype_id = data_state->EnsureArchetype(templId, &this->arch_data);
    chunk_index_t chunk_id;
    InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(templId);
    {
      auto comp_types = &g_ecs_data->componentTypes;
      std::shared_lock arch_lock(this->data_state->archetypes.archetypes_mtx);
      std::shared_lock templ_lock(this->data_state->templates.template_mtx);
      G_ASSERTF(instTempl, "Template {} not initialized", data_state->getTemplateName(templId));
      ENTITY_LOGD2("Creating new entity {:#x} of template '{}'", eid.handle,
            data_state->templates.getTemplate(templId)->getName());
      auto arches = &this->data_state->archetypes;

      auto arch_inst = this->arch_data.getArch(archetype_id);
      auto info = &arches->archetypes[archetype_id];
      chunk_id = arch_inst->getNextAvailableChunkId();
      auto t = arch_inst->reserveChunkId(chunk_id);
      G_ASSERT(t);
      auto archInfo = &info->INFO;
      auto ComponentInfo = &arches->archetypeComponents[info->COMPONENT_OFS];
      // setup entity with initialized data

      for (auto &comp: initializer) {
        //LOG("ComponentInit id %i\n", comp.cIndex);
        archetype_component_id id = archInfo->getComponentId(comp.cIndex);
        //LOG("%s(%s) data:", dataComponents.getName(comp.cIndex).data(), componentTypes.getName(comp.second.getTypeId()).data());
        //comp.second.getComponentRef().print(&componentTypes);
        G_ASSERT(id != INVALID_ARCHETYPE_COMPONENT_ID); // component exists for us
        auto curr_info = ComponentInfo[id];
        auto data = arch_inst->getCompDataUnsafe(curr_info.DATA_OFFSET, chunk_id, curr_info.DATA_SIZE);
        //LOG("Initializing component %s(%s) at address %p of size %i in chunk %i\n",
        //    dataComponents.getName(comp.cIndex).data(),
        //    componentTypes.getName(comp.second.componentTypeIndex).data(),
        //    data, comp.second.getSize(), chunk_id);
        //comp.second.getComponentRef().print(&componentTypes);
        //LOG("\n");
        //info->ARCHETYPE.printChunkBoundries(chunk_id);
        //LOG("\nRaw Data before copy: 0x");
        //auto charPtr = (const char *)data;
        //for(int i = 0; i < comp.second.getSize(); i++)
        //{
        //  LOG("%02X", charPtr[i]);
        //}
        //LOG("\n");
        //std::cout.flush();
        comp.second.getComponentRef().createCopy(data, comp_types, this, eid, comp.cIndex);
        //memcpy(data, comp.second.getRawData(), comp.second.getSize());
        //LOG("\nRaw Data after copy: 0x");
        //charPtr = (const char *)data;
        //for(int i = 0; i < comp.second.getSize(); i++)
        //{
        //  LOG("%02X", charPtr[i]);
        //}
        //LOG("\n");
        //std::cout.flush();
        //LOGE("Freeing Pointer: {}", fmt::ptr(comp.second.getRawData()));
        //free(comp.second.getRawData()); // we copied the data, so now we free the old shit
        //comp.second.reset(); // defaults without destructing the data.
        this->wasInit.set(comp.cIndex, true);
        //LOG("set %i\n", comp.cIndex);
      }
      //LOG("\n");

      // now setup any remaining components with default data
      for (auto &comp: instTempl->components) {

        //LOG("instTempl trying id %i\n", comp.comp_type_index);
        if (!this->wasInit.test(comp.comp_type_index, false)) {
          //LOG("succeeded\n");
          archetype_component_id id = archInfo->getComponentId(comp.comp_type_index);

          // we dont have to test here as the archtype was derived from these components
          auto curr_info = ComponentInfo[id];
          G_ASSERT(curr_info.INDEX == comp.comp_type_index);
          auto data = arch_inst->getCompDataUnsafe(curr_info.DATA_OFFSET, chunk_id, curr_info.DATA_SIZE);
          //LOG("creating component %s(%s) at address %p in chunk %i\n",
          //    dataComponents.getName(comp.comp_type_index).data(),
          //    componentTypes.getName(comp.default_component.getTypeId()).data(),
          //    data, chunk_id);

          //comp.default_component.print(&componentTypes);
          //LOG("\nRaw Data before copy; 0x");
          //auto charPtr = (const char *)data;
          //for(int i = 0; i < comp.default_component.getSize(); i++)
          //{
          //  LOG("%02X", charPtr[i]);
          //}
          //LOG("\n");
          //std::cout.flush();
          comp.default_component.createCopy(data, &data_state->componentTypes, this, eid, comp.comp_type_index);
          //LOG("\nRaw Data after copy: 0x");
          //charPtr = (const char *)data;
          //for(int i = 0; i < comp.default_component.getSize(); i++)
          //{
          //  LOG("%02X", charPtr[i]);
          //}
          //LOG("\n");
          //std::cout.flush();
        }
      }
    }

    this->entDescs.Allocate(eid);
    this->entDescs[eid.index()] = {eid, templId, archetype_id, chunk_id};
    this->wasInit.clear();
    this->sendEventImmediate(eid, ecs::EventEntityCreated{});
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

  bool EntityManager::destroyEntity(EntityId eid, bool is_dtor) {
    sendEventImmediate(eid, EventEntityDestroyed{});
    if (!this->doesEntityExist(eid))
      return false;
    auto desc = this->entDescs.getEntityDesc(eid);
    if(is_dtor) {
      ENTITY_LOGD3("Destroying entity {:#x} of template {}", eid.handle,
            data_state->templates.getTemplate(desc->templ_id)->getName());
    } else {
      ENTITY_LOGD2("Destroying entity {:#x} of template {}", eid.handle,
                   data_state->templates.getTemplate(desc->templ_id)->getName());
    }
    //const InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(desc->templ_id);
    archetype_t archetype_id = desc->archetype_id;
    auto ARCHETYPE = this->arch_data.getArch(archetype_id);
    auto info = &data_state->archetypes.archetypes[archetype_id];
    auto ComponentInfo = &data_state->archetypes.archetypeComponents[info->COMPONENT_OFS];
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

      ref.destructCopy(data, &data_state->componentTypes);
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
    return true;
  }

  ecs::EntityManager::~EntityManager() {
    ZoneScoped;
    ECS_LOGD3("starting EntityManager Destruction");
    for (int i = 1; i < this->entDescs.entDescs.size(); i++) {
      if (!this->entDescs.doesEntityExist(i))
        continue;
      destroyEntity(EntityId(i), true); //
    }
    ECS_LOGD3("finished EntityManager Destruction");
    //g_log_handler.wait_until_empty();
    //g_log_handler.flush_all();
  }

  void EntityManager::debugPrintEntities() {
    for (entity_id_t i = 0; i < this->entDescs.entDescs.size(); i++) {
      EntityId eid{i};
      if (this->doesEntityExist(eid)) {
        this->debugPrintEntity(eid);
      }
    }
  }

  void EntityManager::debugPrintEntity(EntityId eid) {
    if (this->doesEntityExist(eid)) {
      auto desc = this->entDescs.getEntityDesc(eid);
      eid = desc->eid;
      //const InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(desc->templ_id);

      archetype_t archetype_id = desc->archetype_id;
      auto ARCHETYPE = this->arch_data.getArch(archetype_id);
      auto info = &data_state->archetypes.archetypes[archetype_id];
      auto ComponentInfo = &data_state->archetypes.archetypeComponents[info->COMPONENT_OFS];
      //auto archInfo = &info->INFO;
      LOG("DebugPrint of Entity {:#x} of template '{}' of archetype_id {}", eid.handle,
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
            comp_info->DATA_OFFSET, desc->chunk_id, comp_info->DATA_SIZE, fmt::ptr(data));
        LOG("  component {}({}) data: {}", dataComp->getName().data(), comp->name.data(), ref.toString(nullptr));
      }
      LOG("");
    }
  }

  void *EntityManager::getNullable(EntityId eid, component_index_t index, archetype_t &archetype) const {
    if (!this->entDescs.doesEntityExist(eid))
      return nullptr;
    auto desc = this->entDescs[eid.index()];
    archetype = desc.archetype_id; // should always be valid
    G_ASSERT(archetype != INVALID_ARCHETYPE);
    std::shared_lock lk(this->data_state->archetypes.archetypes_mtx);
    G_ASSERT(data_state->archetypes.archetypes[archetype].INFO.getComponentId(index) != INVALID_COMPONENT_INDEX);
    return data_state->archetypes.getComponentDataUnsafe(this->arch_data, archetype, index, desc.chunk_id);
  }

  void *EntityManager::getNullableUnsafe(EntityId eid, component_index_t index, archetype_t &archetype) const {
    G_ASSERT(this->entDescs.doesEntityExist(eid)); // sanity check in dev only
    auto desc = this->entDescs[eid.index()];
    archetype = desc.archetype_id; // should always be valid
    G_ASSERT(archetype != INVALID_ARCHETYPE);
    //G_ASSERT(data_state->archetypes.archetypes[archetype].INFO.getComponentId(index) != INVALID_COMPONENT_INDEX);
    std::shared_lock lk(this->data_state->archetypes.archetypes_mtx);
    if (data_state->archetypes.archetypes[archetype].INFO.getComponentId(index) == INVALID_COMPONENT_INDEX)
      return nullptr;
    return data_state->archetypes.getComponentDataUnsafe(this->arch_data, archetype, index, desc.chunk_id);
  }

  bool EntityManager::entityHasComponent(EntityId eid, component_index_t index) const {
    G_ASSERT(this->entDescs.doesEntityExist(eid)); // sanity check in dev only
    auto desc = this->entDescs[eid.index()];
    archetype_t archetype = desc.archetype_id; // should always be valid
    G_ASSERT(archetype != INVALID_ARCHETYPE);
    //G_ASSERT(data_state->archetypes.archetypes[archetype].INFO.getComponentId(index) != INVALID_COMPONENT_INDEX);
    std::shared_lock lk(this->data_state->archetypes.archetypes_mtx);
    if (data_state->archetypes.archetypes[archetype].INFO.getComponentId(index) == INVALID_COMPONENT_INDEX)
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
    std::shared_lock lk(this->data_state->archetypes.archetypes_mtx);
    return data_state->archetypes.getComponentsCount(archetype) - 1; // first is eid
  }

  ComponentRef EntityManager::getComponentRef(EntityId eid, archetype_component_id cid) const {

    auto desc = this->entDescs.getEntityDesc(eid);
    if (!desc)
      return {};

    std::shared_lock lk(this->data_state->archetypes.archetypes_mtx);
    auto data = data_state->archetypes.getComponentDataIdUnsafe(this->arch_data, desc->archetype_id, cid,
                                                                desc->chunk_id);
    if (!data)
      return {};
    auto cidx = data_state->archetypes.getComponentUnsafe(desc->archetype_id, cid);
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
      std::shared_lock lk(this->data_state->archetypes.archetypes_mtx);
      data = data_state->archetypes.getComponentDataUnsafe(this->arch_data, desc->archetype_id, cidx,
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

  void EntityManager::collectEntitiesOfTemplate(std::vector<EntityId> &out, template_t template_id) {
    for (int i = 1; i < this->entDescs.entDescs.size(); i++) {
      template_t tid;
      G_ASSERT(this->entDescs.getEntityTemplateId(ecs::EntityId(i), tid));
      if (tid == template_id) {
        out.push_back(this->entDescs[i].eid);
      }
    }
  }

  void EntityManager::sendEventImmediate(EntityId eid, Event &evt) {
    if (!this->entDescs.doesEntityExist(eid))
    EXCEPTION("tried to send a query to an entity that doesnt exist");
    this->data_state->sendEventImmediate(eid, evt, this);
  }

  void EntityManager::broadcastEventImmediate(Event &evt) {
    this->data_state->broadcastEventImmediate(evt, this);
  }

  void EntityManager::sendEventImmediate(EntityId eid, Event &&evt) {
    return sendEventImmediate(eid, evt);
  }

  void EntityManager::broadcastEventImmediate(Event &&evt) {
    return broadcastEventImmediate(evt);
  }

  ecs::EntityId EntityManager::getUnit(uint16_t uid) {
    uid &= 0x7FF;
    if (uid == 0x7FF) {
      return INVALID_ENTITY_ID;
    }
    return this->uid_lookup[uid];
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
    auto inst = this->data_state->templates.inst_templates[t];
    for(auto &comp : inst->components) {
      auto d = this->data_state->dataComponents.getDataComponent(comp.comp_type_index); // datacomponent
      auto c = this->data_state->componentTypes.getComponentData(d->componentIndex); // component
      comps.emplace_back(c, d);
    }
  }

  // uid handling
  static void
  uid_handler_add_delete_entities(EntityManager *mgr, const ecs::Event &__restrict evt,
                                  const ecs::QueryView &__restrict components) {
    auto *eid = (ecs::EntityId *) components.componentData[0];
    auto *uid = (int *) components.componentData[1];
    auto *unit__ref = (unit::UnitRef *)components.componentData[2];
    if (evt.is<ecs::EventEntityCreated>()) {
      if (*uid == -1) {
        LOGE("Entity {:#x} has no uid, normal?", eid->get_handle());
        return;
      }
      G_ASSERT(mgr->uid_lookup[*uid] == ecs::INVALID_ENTITY_ID);
      mgr->uid_lookup[*uid] = *eid;
      mgr->uid_unit_ref_lookup[*uid] = unit__ref;
      //LOG("set uid {} to eid {}", *uid, eid->get_handle());
    } else if (evt.is<ecs::EventEntityDestroyed>()) {
      G_ASSERT(mgr->uid_lookup[*uid] != ecs::INVALID_ENTITY_ID);
      mgr->uid_lookup[*uid] = ecs::INVALID_ENTITY_ID;
      mgr->uid_unit_ref_lookup[*uid] = nullptr;
      //LOG("removing entity at uid {}", *uid);
    }
  }

  static constexpr ecs::ComponentDesc uid_lookup_add_remove_event[] =
      {
          {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
          {ECS_HASH("uid"), ecs::ComponentTypeInfo<int>()},
          {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()}
      };

  static ecs::EntitySystemDesc uid_handler_add_delete_entities_desc
      (
          "msg_sink_es",
          "prog/gameLibs/daECS/net/msgSinkES.cpp.inl",
          ecs::EntitySystemOps(uid_handler_add_delete_entities),
          empty_span(),
          make_span(uid_lookup_add_remove_event, 3)/*ro*/,
          empty_span(),
          empty_span(),
          ecs::EventSetBuilder<ecs::EventEntityCreated, ecs::EventEntityDestroyed>::build(),
          0
      );

  // handling adding new entities to a player's owned units list
  static constexpr ecs::ComponentDesc mplayer_add_comps[] =
      {
          {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
          {ECS_HASH("unit__playerId"),    ecs::ComponentTypeInfo<int>()},
          {ECS_HASH("playerUnit"), ecs::ComponentTypeInfo<ecs::Tag>()}
      };

  static void
  mplayer_add_entity(EntityManager *mgr, const ecs::Event &__restrict evt,
                        const ecs::QueryView &__restrict components) {
    auto *eid = (ecs::EntityId *) components.componentData[0];
    auto *unit__playerId = (int *) components.componentData[1];
    if (evt.is<ecs::EventEntityCreated>()) {
      if(*unit__playerId == -1) { // not owned by a player
        return;
      }
      G_ASSERT(*unit__playerId < mgr->owned_by->players.size());
      mgr->owned_by->players[*unit__playerId].ownedUnits.emplace(*eid);
    } else if(evt.is<ecs::EventEntityDestroyed>()) {
      if(*unit__playerId == -1) { // not owned by a player
        return;
      }
      G_ASSERT(*unit__playerId < mgr->owned_by->players.size());
      mgr->owned_by->players[*unit__playerId].ownedUnits.erase(*eid);
    }
  }

  static ecs::EntitySystemDesc mplayer_add_entity_es
      (
          "mplayer_add_entity_es",
          "womp womp",
          ecs::EntitySystemOps(mplayer_add_entity),
          empty_span(),
          make_span(mplayer_add_comps, 2),/*ro*/
          make_span(mplayer_add_comps+2, 1),/*rq*/
          empty_span(),
          ecs::EventSetBuilder<ecs::EventEntityCreated, ecs::EventEntityDestroyed>::build(),
          0
      );

  static constexpr ecs::ComponentDesc unit_aircraft_create_comps[] =
      {
          {ECS_HASH("unit_storage__aircraft"), ecs::ComponentTypeInfo<FlightModelWrapStorageComponent>()},
          {ECS_HASH("uid"),    ecs::ComponentTypeInfo<int>()},
          {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()}
      };

  static void
  unit_aircraft_create(EntityManager *mgr, const ecs::Event &__restrict evt,
                     const ecs::QueryView &__restrict components) {
    auto *unit_storage__aircraft = (FlightModelWrapStorageComponent *) components.componentData[1];
    auto *uid = (int *) components.componentData[2];
    auto *unit__ref = (unit::UnitRef*)components.componentData[0];
    if (evt.is<ecs::EventEntityCreated>()) {
      G_ASSERT(unit__ref->unit == nullptr);
      unit__ref->unit = new unit::Aircraft(static_cast<uint16_t>(*uid));
      unit::LoadFromStorage(unit__ref->unit, unit_storage__aircraft);
    }
  }

  static ecs::EntitySystemDesc unit_aircraft_create_es
      (
          "unit_aircraft_create_es",
          "womp womp",
          ecs::EntitySystemOps(unit_aircraft_create),
          make_span(unit_aircraft_create_comps+2, 1),/*rw*/
          make_span(unit_aircraft_create_comps, 2),/*ro*/
          empty_span(),
          empty_span(),
          ecs::EventSetBuilder<ecs::EventEntityCreated>::build(),
          0
      );

  static constexpr ecs::ComponentDesc unit_tank_create_comps[] =
      {
          {ECS_HASH("unit_storage__tank"), ecs::ComponentTypeInfo<HeavyVehicleModelStorageComponent>()},
          {ECS_HASH("uid"),    ecs::ComponentTypeInfo<int>()},
          {ECS_HASH("unit__ref"), ecs::ComponentTypeInfo<unit::UnitRef>()}
      };

  static void
  unit_tank_create(EntityManager *mgr, const ecs::Event &__restrict evt,
                       const ecs::QueryView &__restrict components) {
    auto *unit_storage_tank = (HeavyVehicleModelStorageComponent *) components.componentData[1];
    auto *uid = (int *) components.componentData[2];
    auto *unit__ref = (unit::UnitRef*)components.componentData[0];
    if (evt.is<ecs::EventEntityCreated>()) {
      G_ASSERT(unit__ref->unit == nullptr);
      unit__ref->unit = new unit::Tank(static_cast<uint16_t>(*uid));
      unit::LoadFromStorage(unit__ref->unit, unit_storage_tank);
    }
  }

  static ecs::EntitySystemDesc unit_tank_create_es
      (
          "unit_tank_create_es",
          "womp womp",
          ecs::EntitySystemOps(unit_tank_create),
          make_span(unit_tank_create_comps+2, 1),/*rw*/
          make_span(unit_tank_create_comps, 2),/*ro*/
          empty_span(),
          empty_span(),
          ecs::EventSetBuilder<ecs::EventEntityCreated>::build(),
          0
      );
}

