#include "ecs/EntityManager.h"
#include "ecs/archetypes.h"
#include "hash/xxhash.h"


namespace ecs
{


  inline uint32_t Archetypes::getComponentSizeFromOfs(archetype_component_id component_id, uint32_t ofs) const
  {
    return archetypeComponents[component_id + ofs].DATA_SIZE;
  }

  uint32_t calculate_true_size(uint32_t sz)
  {
    switch(sz) {
      case 0:
        return 0;
      case 1:
        return 1;
      case 2:
        return 2;
      case 3:
      case 4:
        return 4;
      default:
        return (uint16_t)(((sz + 3) / 4) * 4); // ensure packing to 4
    }
  }

  archetype_t Archetypes::createArchetype(const component_index_t *__restrict components, uint32_t components_cnt,
                                   DataComponents &dataComponents, ComponentTypes &componentTypes) {
    //TODO: add support for findArchetype, only plan to do it if we 1: plan to use east::tuple_vector or make our own, what it does makes that much easier
    uint32_t entitySize = 0;
    const uint32_t componentsAt = (uint32_t)archetypeComponents.size();
    archetypeComponents.resize(componentsAt + components_cnt);

    for(int i = 0; i < components_cnt; i++)
    {
      //TODO: when tuple vector, update this to be like gaijn, fucking memcpy
      archetypeComponents[componentsAt+i].INDEX = components[i];
    }
    //std::unique_ptr<uint16_t[]> initialComponentDataOffset(new uint16_t[components_cnt]);

    // populates archetypeComponents
    for (archetype_component_id i = 0; i < components_cnt; ++i)
    {
      auto x = components[i];
      const auto typeIndex = dataComponents.getDataComponent(x)->componentIndex;
      const auto type = componentTypes.getComponentData(typeIndex);
      uint32_t true_size = calculate_true_size(type->size); // this ensures larger structs are stored optimally
      if(uint32_t offset = entitySize%4) // if offset is > 0, then we arnt alligned to 4 bytes
      {
        // components smaller than 4 bytes we dont care about alligned storage
        if(true_size >= 4) // components at least 4 bytes in size we want to allign
        {
          entitySize += 4-offset; // 19 % 4 == 3, 4-3 = 1, only need 1 byte to be alligned again
        }
      }
      archetypeComponents[componentsAt+i].DATA_OFFSET = entitySize;
      archetypeComponents[componentsAt+i].DATA_SIZE = true_size;
      entitySize += true_size;
    }
    G_ASSERTF(entitySize >= sizeof(EntityId) && entitySize < std::numeric_limits<uint16_t>::max(), "%d", entitySize); // ensures entity can be properly addressed
    //  SmallTab<CreatableComponent> creatables;
    //  SmallTab<ResourceComponent> withResources;
    //  SmallTab<TrackedPod> trackedPods;
    //  SmallTab<CreatableComponent> trackedCreatables;
    ArchetypeInfo info;
    if (components_cnt > 1) // should always have an entity_id component, which is why >1 and not >0
    {
      component_index_t firstNonEidIndex = components[1], lastIndex = components[components_cnt - 1]; // this assumes components is sorted least to greatest
      // also assumes that components[0] == eid
      uint32_t indicesCount = lastIndex - firstNonEidIndex + 1;
      std::unique_ptr<uint16_t[]> indicesMap(new uint16_t[indicesCount]);
      memset(indicesMap.get(), 0xFF, indicesCount * sizeof(uint16_t));
      for (uint16_t i = 1; i < components_cnt; ++i)
      {
        G_ASSERT(components[i] >= firstNonEidIndex && components[i] <= lastIndex);
        indicesMap[components[i] - firstNonEidIndex] = i;
      }
      info = ArchetypeInfo{firstNonEidIndex, ecs::component_index_t(indicesCount), std::move(indicesMap)};
      for (archetype_component_id i = 1; i < components_cnt; ++i)
      {
        const component_index_t cidx = components[i];
        const auto dataComponent = dataComponents.getDataComponent(cidx);
        const auto typeIndex = dataComponent->componentIndex;
        const auto type = componentTypes.getComponentData(typeIndex);
        /// TODO, set up creatables shit???
        G_ASSERT(getComponentSizeFromOfs(i, componentsAt) == type->size);
      }
    }
    else
      info = ArchetypeInfo{INVALID_COMPONENT_INDEX, 0, nullptr};
    G_ASSERT(components_cnt < 65535 && entitySize <= 65535);
    archetypes.emplace_back(Archetype{entitySize}, componentsAt, std::move(info), components_cnt);
    return (archetype_t)archetypes.size() - 1;
  }

  archetype_component_id Archetypes::getComponentsCount(uint32_t archetype) const {
    G_ASSERT(archetype < this->archetypes.size());
    return this->archetypes[archetype].COMPONENT_COUNT;
  }
}