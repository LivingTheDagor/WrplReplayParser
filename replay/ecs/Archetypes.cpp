#include "ecs/EntityManager.h"
#include "ecs/archetypes.h"
#include "hash/xxhash.h"


namespace ecs {


  void Archetypes::createArchetype(archetype_t archetype, MgrArchetypeStorage &storage) {
    storage.constructArch(archetype, this->archetypes[archetype]);
  }
  bool Archetypes::archetypeExists(archetype_t archetype, MgrArchetypeStorage &storage) {
    return storage.hasArchetype(archetype);
  }

  archetype_t Archetypes::createArchetype(const component_index_t *__restrict components, uint32_t components_cnt,
                                          DataComponents &dataComponents, ComponentTypes &componentTypes,
                                          template_t parent_template) {
    // TODO: add support for findArchetype, only plan to do it if we 1: plan to use east::tuple_vector or make our own,
    // what it does makes that much easier
    uint32_t entitySize = 0;
    //const uint32_t componentsAt = (uint32_t) archetypeComponents.size();
    //archetypeComponents.resize(componentsAt + components_cnt);
    std::vector<ArchetypeComponentStorage> newComponents(components_cnt);

    for (int i = 0; i < components_cnt; i++) {
      // TODO: when tuple vector, update this to be like gaijn, fucking memcpy
      newComponents[i].INDEX = components[i];
    }
    // std::unique_ptr<uint16_t[]> initialComponentDataOffset(new uint16_t[components_cnt]);

    // populates archetypeComponents
    for (archetype_component_id i = 0; i < components_cnt; ++i) {
      auto x = components[i];
      const auto typeIndex = dataComponents.getDataComponent(x)->componentIndex;
      const auto type = componentTypes.getComponentData(typeIndex);
      // uint32_t true_size = calculate_true_size(type->size); // this ensures larger structs are stored optimally
      uint32_t true_size =
        type->size; // The builtin compiler padding does the exact same as my padding lmao (im stupid)

      // LOG("Calculated true size; original size: {}; new size: {}", type->size, true_size);
      // if(type->size != true_size)
      //   LOG("DIFFEREEEEENT");
      if (uint32_t offset = entitySize % 4) // if offset is > 0, then we arnt alligned to 4 bytes
      {
        // components smaller than 4 bytes we dont care about alligned storage
        if (true_size >= 4) // components at least 4 bytes in size we want to allign
        {
          entitySize += 4 - offset; // 19 % 4 == 3, 4-3 = 1, only need 1 byte to be alligned again
        }
      }
      newComponents[i].DATA_OFFSET = (uint16_t) entitySize;
      newComponents[i].DATA_SIZE = true_size;
      entitySize += true_size;
    }
    G_ASSERTF(entitySize >= sizeof(EntityId) && entitySize < std::numeric_limits<uint16_t>::max(), "%d",
              entitySize); // ensures entity can be properly addressed
    //  SmallTab<CreatableComponent> creatables;
    //  SmallTab<ResourceComponent> withResources;
    //  SmallTab<TrackedPod> trackedPods;
    //  SmallTab<CreatableComponent> trackedCreatables;
    ArchetypeInfo info;
    if (components_cnt > 1) // should always have an entity_id component, which is why >1 and not >0
    {
      component_index_t firstNonEidIndex = components[1],
                        lastIndex =
                          components[components_cnt - 1]; // this assumes components is sorted least to greatest
      // also assumes that components[0] == eid
      uint32_t indicesCount = lastIndex - firstNonEidIndex + 1;
      std::unique_ptr<uint16_t[]> indicesMap(new uint16_t[indicesCount]);
      memset(indicesMap.get(), 0xFF, indicesCount * sizeof(uint16_t));
      for (uint16_t i = 1; i < components_cnt; ++i) {
        G_ASSERT(components[i] >= firstNonEidIndex && components[i] <= lastIndex);
        indicesMap[components[i] - firstNonEidIndex] = i;
      }
      info = ArchetypeInfo{firstNonEidIndex, ecs::component_index_t(indicesCount), std::move(indicesMap)};
      for (archetype_component_id i = 1; i < components_cnt; ++i) {
        const component_index_t cidx = components[i];
        const auto dataComponent = dataComponents.getDataComponent(cidx);
        const auto typeIndex = dataComponent->componentIndex;
        const auto type = componentTypes.getComponentData(typeIndex);
        /// TODO, set up creatables shit???
        G_ASSERT(newComponents[i].DATA_SIZE == type->size);
      }
    } else
      info = ArchetypeInfo{INVALID_COMPONENT_INDEX, 0, nullptr};
    G_ASSERT(components_cnt < 65535 && entitySize <= 65535);
    archetypes.emplace_back(new ArchetypeStorage{
      entitySize,
      std::move(info),
      (archetype_component_id)components_cnt,
      parent_template,
      std::move(newComponents)}); // Archetype{entitySize}
    return (archetype_t) archetypes.size() - 1;
  }

  archetype_component_id Archetypes::getComponentsCount(uint32_t archetype) const {
    G_ASSERT(archetype < this->archetypes.size());
    return this->archetypes[archetype]->COMPONENT_COUNT;
  }
  Archetypes::~Archetypes() {
    for (auto & arch : archetypes) {
      delete arch;
      arch = nullptr;
    }
  }
  Archetype *MgrArchetypeStorage::getArch(archetype_t archetype) const {
    if (this->data.size() > archetype) {
      return this->data[archetype];
    }
    return nullptr;
  }
  Archetypes::ArchetypeStorage *MgrArchetypeStorage::getStorageArch(archetype_t archetype) const {
    if (this->archetypeStorages.size() > archetype) {
      return this->archetypeStorages[archetype];
    }
    return nullptr;
  }
  component_index_t MgrArchetypeStorage::getComponentUnsafe(archetype_t archetype, archetype_component_id id) const {
    if (!this->hasArchetype(archetype))
      return INVALID_COMPONENT_INDEX;
    auto arch_storage = this->archetypeStorages[archetype];
    return arch_storage->components[id].INDEX;
  }
  void *MgrArchetypeStorage::getComponentDataIdUnsafe(archetype_t archetype, archetype_component_id cid,
                                                      chunk_index_t chunkId) const {
    if (!this->hasArchetype(archetype))
      return nullptr;
    auto arch = this->data[archetype];
    auto arch_storage = this->archetypeStorages[archetype];
    auto storage = &arch_storage->components[cid];
    return arch->getCompDataUnsafe(storage->DATA_OFFSET, chunkId, storage->DATA_SIZE);

  }
  void *MgrArchetypeStorage::getComponentDataUnsafe(archetype_t archetype,
                                                    component_index_t cidx, chunk_index_t chunkId) const {
    ZoneScoped;
    if (!this->hasArchetype(archetype))
      return nullptr;
    auto arch = this->data[archetype];
    auto arch_storage = this->archetypeStorages[archetype];
    archetype_component_id cid = arch_storage->INFO.getComponentId(cidx);
    if (cid == INVALID_ARCHETYPE_COMPONENT_ID)
      return nullptr;
    auto storage = &arch_storage->components[cid];
    return arch->getCompDataUnsafe(storage->DATA_OFFSET, chunkId, storage->DATA_SIZE);
  }
  MgrArchetypeStorage::~MgrArchetypeStorage() {
    for (auto ptr: data) {
      delete ptr;
    }
  }

  void Archetype::printChunkBoundries(chunk_index_t chunk_id) {
    uint32_t chunk_list_index = (chunk_id / EntityCount);
    LOG("start: {}; end: {}\n", fmt::ptr(this->chunks[chunk_list_index].data),
        fmt::ptr(this->chunks[chunk_list_index].data + entity_size * EntityCount));
  }
}