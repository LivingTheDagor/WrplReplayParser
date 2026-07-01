#ifndef MYEXTENSION_ENTITYDESCS_H
#define MYEXTENSION_ENTITYDESCS_H
#include "ecs/typesAndLimits.h"
#include <vector>
#include "ecs/entityId.h"
#include "EASTL/bitvector.h"

namespace ecs {
  static inline entity_id_t make_eid(uint32_t index, uint32_t gen) { return (index << ENTITY_GENERATION_BITS) | gen; }
  static constexpr uint32_t RESERVED_EID_RANGE = std::numeric_limits<uint16_t>::max();
  static constexpr uint32_t globalGen =
    1; // my globalGen is never going to change as an EntityManager is destroyed upon session destruction
  struct EntityDesc {
    template_t templ_id = INVALID_TEMPLATE_INDEX; // 16 bit
    archetype_t archetype_id = INVALID_ARCHETYPE; // 16 bit
    // this is all gaijon
    uint32_t generation : ENTITY_GENERATION_BITS = globalGen;
    chunk_index_t chunk_id : 32 - ENTITY_GENERATION_BITS = INVALID_CHUNK_INDEX_T;
  };
  G_STATIC_ASSERT(sizeof(EntityDesc) == 8);
  // a class that represents
  class EntityDescs {
  protected:
    friend EntityManager;
    friend GState;


    std::vector<EntityDesc> entDescs;
    // has a specific entity index already been 'basic destroyed?'
    // placed here as its tied directly with the entDesc capacity
    eastl::bitvector<> basic_destroyed{};

    EntityDescs() {
      entDescs.resize(RESERVED_EID_RANGE + 1); // reserve initial reserved space
      basic_destroyed.resize(RESERVED_EID_RANGE + 1, false);
      EntityDesc desc{1, 1, 1}; // reserves first slot, an invalid entity id is 0
      entDescs[0] = desc;
    }
    inline const EntityDesc *getEntityDesc(EntityId eid) const;
    inline EntityDesc *getEntityDesc(EntityId eid);
    bool getEntityTemplateId(EntityId eid, template_t &tmpl) const;
    bool getEntityArchetypeId(EntityId eid, archetype_t &archetype) const;
    bool getEntityChunkId(EntityId eid, chunk_index_t &chunk) const;

    /// ensures that eid has space to exist
    inline void Allocate(EntityId eid) {
      auto idx = eid.index();
      if (idx >= entDescs.size()) {
        basic_destroyed.resize(idx + 1, false);
        entDescs.resize(idx + 1);
      }
    }

    [[nodiscard]] inline bool doesEntityExistInternal(unsigned idx) const {
      return idx < entDescs.size() && entDescs[idx].templ_id != INVALID_TEMPLATE_INDEX;
    }

    [[nodiscard]] inline bool doesEntityExist(unsigned idx, uint32_t generation) const {
      return idx < entDescs.size() && entDescs[idx].generation == generation &&
             entDescs[idx].templ_id != INVALID_TEMPLATE_INDEX;
    }
    [[nodiscard]] bool doesEntityExist(EntityId e) const { return doesEntityExist(e.index(), e.get_generation()); }


    EntityDesc &operator[](uint32_t i) { return entDescs[i]; }
    const EntityDesc &operator[](uint32_t i) const { return entDescs[i]; }

    EntityDesc &operator[](ecs::EntityId i) { return this->entDescs[i.index()]; }
    const EntityDesc &operator[](ecs::EntityId i) const { return this->entDescs[i.index()]; }


    size_t size() const { return entDescs.size(); }

    uint32_t push_back() {
      auto idx = this->entDescs.size();
      entDescs.push_back({});
      basic_destroyed.push_back(false);
      return (uint32_t) idx;
    }

    EntityId makeEid(uint32_t idx) {
      return EntityId(make_eid(idx, idx < entDescs.size() ? entDescs[idx].generation : globalGen));
    }
  };


} // namespace ecs

#endif // MYEXTENSION_ENTITYDESCS_H
