

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
  protected:
    friend Component;
    friend InstantiatedTemplate;

    GState *data_state = g_ecs_data.get();
    EntityDescs entDescs;
    Archetypes archetypes;
    BitVector wasInit{false};
    std::unordered_map<InstantiatedTemplate*, archetype_t> initialized_archetypes;
  };



}

#endif //MYEXTENSION_ENTITYMANAGER_H
