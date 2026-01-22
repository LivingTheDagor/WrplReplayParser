#include "ecs/EntityManager.h"
#include "ecs/ComponentTypesDefs.h"
namespace ecs {
  OnDemandInit<GState> g_ecs_data{};

  EntityManager::EntityManager() {
    // componentTypes and dataComponents initalzied in initialize() in /init/initialze.h
    wasInit.resize(10000, false); // just in case, current max datacomponents was like 950; edit: its now like 7000 cause of infantry and shit
  }

  inline const EntityDesc * EntityDescs::getEntityDesc(EntityId eid) const{
    auto idx = eid.index();
    if (idx < entDescs.size())
    {
      return &entDescs[idx];
    }
    return nullptr;
  }
  inline EntityDesc * EntityDescs::getEntityDesc(EntityId eid) {
    if(!this->doesEntityExist(eid))
      return nullptr;

    return &entDescs[eid.index()];
  }

  bool EntityDescs::getEntityTemplateId(EntityId eid, template_t &tmpl) const{
    auto desc = this->getEntityDesc(eid);
    if(desc)
    {
      tmpl = desc->templ_id;
      return true;
    }
    tmpl = INVALID_TEMPLATE_INDEX;
    return false;
  }

  bool EntityDescs::getEntityArchetypeId(EntityId eid, archetype_t &archetype) const {
    auto desc = this->getEntityDesc(eid);
    if(desc)
    {
      archetype = desc->archetype_id;
      return true;
    }
    archetype = INVALID_ARCHETYPE;
    return false;
  }

  bool EntityDescs::getEntityChunkId(EntityId eid, chunk_index_t &chunk) const {
    auto desc = this->getEntityDesc(eid);
    if(desc)
    {
      chunk = desc->chunk_id;
      return true;
    }
    chunk = INVALID_CHUNK_INDEX_T;
    return false;
  }

  EntityId EntityManager::createEntity(EntityId eid, template_t templId, ComponentsInitializer &&initializer) {
    this->wasInit.clear();
    validateInitializer(templId, initializer); // ensures the initializer has cIndex populated
    InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(templId);
    G_ASSERTF(instTempl, "Template {} not initialized", data_state->templates.getTemplate(templId)->name);
    LOG("Creating new entity {:#x} of template '{}'", eid.handle, data_state->templates.getTemplate(templId)->name.c_str());
    if(eid.handle == 0x4008a3) {
      //LOG("{} instTemplate components:", g_ecs_data->templates.getTemplate(instTempl->parent)->name);
      //for(auto & c : instTempl->component_indexes) {
      //  LOG("    {}", g_ecs_data->dataComponents.getName(c));
      //}
      //g_log_handler.wait_until_empty();
      //g_log_handler.flush_all();
    }
    archetype_t archetype_id = this->createArchetype(instTempl);
    auto info = &this->archetypes.archetypes[archetype_id];
    auto chunk_id = info->ARCHETYPE.getNextAvailableChunkId();
    G_ASSERT(info->ARCHETYPE.reserveChunkId(chunk_id));
    auto archInfo = &info->INFO;
    auto ComponentInfo = &this->archetypes.archetypeComponents[info->COMPONENT_OFS];
    //LOG("InstantiatedTemplate datacomponents:");
    //for(const auto &comp : instTempl->components)
    // {
    //  LOG("%i ", comp.comp_type_index);
    //}
    //LOG("\n");
    // setup entity with initialized data

    for(auto &comp : initializer)
    {
      //LOG("ComponentInit id %i\n", comp.cIndex);
      archetype_component_id id = archInfo->getComponentId(comp.cIndex);
      //if(id == INVALID_ARCHETYPE_COMPONENT_ID)
      //  continue; // its PROBABLY the case that this component exists in template parts we currently dont allow the tags for.
                  // like render or sound.
      //LOG("%s(%s) data:", dataComponents.getName(comp.cIndex).data(), componentTypes.getName(comp.second.getTypeId()).data());
      //comp.second.getComponentRef().print(&componentTypes);
      G_ASSERT(id != INVALID_ARCHETYPE_COMPONENT_ID); // component exists for us
      auto curr_info = ComponentInfo[id];
      auto data =info->ARCHETYPE.getCompDataUnsafe(curr_info.DATA_OFFSET, chunk_id, curr_info.DATA_SIZE);
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
      memcpy(data, comp.second.getRawData(), comp.second.getSize());
      //LOG("\nRaw Data after copy: 0x");
      //charPtr = (const char *)data;
      //for(int i = 0; i < comp.second.getSize(); i++)
      //{
      //  LOG("%02X", charPtr[i]);
      //}
      //LOG("\n");
      //std::cout.flush();
      free(comp.second.getRawData()); // we copied the data, so now we free the old shit
      comp.second.reset(); // defaults without destructing the data.
      this->wasInit.set(comp.cIndex, true);
      //LOG("set %i\n", comp.cIndex);
    }
    //LOG("\n");

    // now setup any remaining components with default data
    for(auto & comp : instTempl->components)
    {

      //LOG("instTempl trying id %i\n", comp.comp_type_index);
      if(!this->wasInit.test(comp.comp_type_index, false))
      {
        //LOG("succeeded\n");
        archetype_component_id id = archInfo->getComponentId(comp.comp_type_index);

        // we dont have to test here as the archtype was derived from these components
        auto curr_info = ComponentInfo[id];
        G_ASSERT(curr_info.INDEX == comp.comp_type_index);
        auto data =info->ARCHETYPE.getCompDataUnsafe(curr_info.DATA_OFFSET, chunk_id, curr_info.DATA_SIZE);
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

    this->entDescs.Allocate(eid);
    this->entDescs[eid.index()] = {eid, templId, archetype_id, chunk_id};
    this->wasInit.clear();
    return eid;
  }

  bool EntityManager::validateInitializer(template_t templId, ComponentsInitializer &comp_init) {
    // more stuff will be done here eventually
    for (auto &initIt : comp_init) {
      if (initIt.cIndex == INVALID_COMPONENT_INDEX) {
        component_index_t initializerIndex = data_state->dataComponents.getIndex(initIt.name);
        G_ASSERTF(initializerIndex != INVALID_COMPONENT_INDEX, "Invalid initializer index hash:{:#x}\n", initIt.name);
        G_ASSERT(data_state->dataComponents.getDataComponent(initializerIndex)->componentHash == initIt.second.getUserType());
        if(DAGOR_UNLIKELY(initializerIndex == INVALID_COMPONENT_INDEX))
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

  bool EntityManager::destroyEntity(EntityId eid) {
    if (!this->doesEntityExist(eid))
		return false;
    auto desc = this->entDescs.getEntityDesc(eid);
    LOG("Destroying entity {:#x} of template {}", eid.handle, data_state->templates.getTemplate(desc->templ_id)->name.c_str());
    //const InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(desc->templ_id);
    archetype_t archetype_id = desc->archetype_id;
    auto info = &this->archetypes.archetypes[archetype_id];
    auto ComponentInfo = &this->archetypes.archetypeComponents[info->COMPONENT_OFS];
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
    for(auto comp_info = ComponentInfo; comp_info != ComponentInfo+info->COMPONENT_COUNT; comp_info++)
    {
      auto data =info->ARCHETYPE.getCompDataUnsafe(comp_info->DATA_OFFSET, desc->chunk_id, comp_info->DATA_SIZE);
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
    info->ARCHETYPE.releaseChunkId(desc->chunk_id);
    desc->templ_id = INVALID_TEMPLATE_INDEX;
    desc->archetype_id = INVALID_ARCHETYPE;
    desc->chunk_id = INVALID_CHUNK_INDEX_T;
    return true;
  }

  ecs::EntityManager::~EntityManager() {
    LOGD("starting EntityManager Destruction\n");
    for(int i = 1; i < this->entDescs.entDescs.size(); i++)
    {
      if(!this->entDescs.doesEntityExist(i))
        continue;
      destroyEntity(EntityId(i)); //
    }
    LOGD("finished EntityManager Destruction");
    g_log_handler.wait_until_empty();
    g_log_handler.flush_all();
  }

  void EntityManager::debugPrintEntities() {
    for(entity_id_t i = 0; i < this->entDescs.entDescs.size(); i++)
    {
      EntityId eid{i};
      if(this->doesEntityExist(eid))
      {
        this->debugPrintEntity(eid);
      }
    }
  }

  void EntityManager::debugPrintEntity(EntityId eid) {
    if(this->doesEntityExist(eid))
    {
      auto desc= this->entDescs.getEntityDesc(eid);
      eid = desc->eid;
      //const InstantiatedTemplate *instTempl = data_state->templates.getInstTemplate(desc->templ_id);

      archetype_t archetype_id = desc->archetype_id;
      auto info = &this->archetypes.archetypes[archetype_id];
      auto ComponentInfo = &this->archetypes.archetypeComponents[info->COMPONENT_OFS];
      //auto archInfo = &info->INFO;
      LOG("DebugPrint of Entity {:#x} of template '{}' of archetype_id {}", eid.handle, this->data_state->templates.getTemplate(desc->templ_id)->name.c_str(), archetype_id);
      for(auto comp_info = ComponentInfo; comp_info != ComponentInfo+info->COMPONENT_COUNT; comp_info++)
      {
        //     ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint16_t size);
        auto data =info->ARCHETYPE.getCompDataUnsafe(comp_info->DATA_OFFSET, desc->chunk_id, comp_info->DATA_SIZE);
        auto dataComp = data_state->dataComponents.getDataComponent(comp_info->INDEX);
        //if(strcmp(dataComp->getName().data(), "skeleton_attach__remapParentSlots") == 0 && eid.handle == 0x4008a3)
        //  LOG("WOMP");
        auto comp = data_state->componentTypes.getComponentData(dataComp->componentIndex);
        ComponentRef ref{data, comp->hash, dataComp->componentIndex, comp->size};
        LOG("  ArchData: idx: {}; data_off: {}; chunk_id: {}; data_size: {}; ptr: {}", comp_info->INDEX, comp_info->DATA_OFFSET, desc->chunk_id, comp_info->DATA_SIZE, fmt::ptr(data));
        LOG("  component {}({}) data: {}", dataComp->getName().data(), comp->name.data(), ref.toString(0));
      }
      LOG("");
    }
  }

  void * EntityManager::getNullable(EntityId eid, component_index_t index, archetype_t &archetype) const {
    if(!this->entDescs.doesEntityExist(eid))
      return nullptr;
    auto desc = this->entDescs[eid.index()];
    archetype = desc.archetype_id; // should always be valid
    G_ASSERT(archetype != INVALID_ARCHETYPE);
    G_ASSERT(archetypes.archetypes[archetype].INFO.getComponentId(index) != INVALID_COMPONENT_INDEX);
    return archetypes.getComponentDataUnsafe(archetype, index, desc.chunk_id);
  }

  void * EntityManager::getNullableUnsafe(EntityId eid, component_index_t index, archetype_t &archetype) const {
    G_ASSERT(this->entDescs.doesEntityExist(eid)); // sanity check in dev only
    auto desc = this->entDescs[eid.index()];
    archetype = desc.archetype_id; // should always be valid
    G_ASSERT(archetype != INVALID_ARCHETYPE);
    G_ASSERT(archetypes.archetypes[archetype].INFO.getComponentId(index) != INVALID_COMPONENT_INDEX);
    return archetypes.getComponentDataUnsafe(archetype, index, desc.chunk_id);
  }

  __forceinline bool EntityManager::getEntityArchetype(EntityId eid, int &idx, archetype_t &archetype) const
  {
    const bool ret = entDescs.getEntityArchetypeId(eid, archetype);
    if (ret)
    {
      //DAECS_VALIDATE_ARCHETYPE(archetype);
    }
    return ret;
  }

  int EntityManager::getNumComponents(EntityId eid) const
  {
    int idx;
    archetype_t archetype = INVALID_ARCHETYPE;
    if (!getEntityArchetype(eid,idx, archetype))
      return -1;
    return archetypes.getComponentsCount(archetype) - 1; // first is eid
  }

  ComponentRef EntityManager::getComponentRef(EntityId eid, archetype_component_id cid) const {

    auto desc = this->entDescs.getEntityDesc(eid);
    if(!desc)
      return {};
    auto data = this->archetypes.getComponentDataIdUnsafe(desc->archetype_id, cid, desc->chunk_id);
    if(!data)
      return {};
    auto cidx = this->archetypes.getComponentUnsafe(desc->archetype_id, cid);
    //ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint16_t size);
    auto datacomp_data = data_state->dataComponents.getDataComponent(cidx);
    if(!datacomp_data)
      return {};

    return {data, datacomp_data->componentHash, datacomp_data->componentIndex, data_state->componentTypes.types[datacomp_data->componentIndex].size, cidx};
  }

  ComponentRef EntityManager::getComponentRefCidx(EntityId eid, component_index_t cidx) const {

    auto desc = this->entDescs.getEntityDesc(eid);
    if(!desc)
      return {};
    auto data = this->archetypes.getComponentDataUnsafe(desc->archetype_id, cidx, desc->chunk_id);
    if(!data)
      return {};
    //auto cidx = this->archetypes.getComponentUnsafe(desc->archetype_id, cid);
    //ComponentRef(void *data, component_type_t type, type_index_t compIndex, uint16_t size);
    auto datacomp_data = data_state->dataComponents.getDataComponent(cidx);
    if(!datacomp_data)
      return {};

    return {data, datacomp_data->componentHash, datacomp_data->componentIndex, data_state->componentTypes.types[datacomp_data->componentIndex].size, cidx};
  }

  void EntityManager::collectEntitiesWithComponent(std::vector<EntityId> &out, HashedConstString component) {
    component_index_t initializerIndex = data_state->dataComponents.getIndex(component.hash);
    G_ASSERTF(initializerIndex != INVALID_COMPONENT_INDEX, "Unable to find component {}<{:#x}>\n", component.str, component.hash);
    G_ASSERT(false);
    for(int i = 1; i < this->entDescs.entDescs.size(); i++)
    {

    }
  }

  void EntityManager::collectEntitiesOfTemplate(std::vector<EntityId> &out, template_t template_id) {
    for(int i = 1; i < this->entDescs.entDescs.size(); i++)
    {
      template_t tid;
      G_ASSERT(this->entDescs.getEntityTemplateId(ecs::EntityId(i), tid));
      if(tid == template_id)
      {
        out.push_back(this->entDescs[i].eid);
      }
    }
  }
}
