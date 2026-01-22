

#ifndef MYEXTENSION_ARCHETYPES_H
#define MYEXTENSION_ARCHETYPES_H
#include <cmath>
#include "Logger.h"



namespace ecs
{

  typedef uint32_t chunk_index_t; // represents the chunk at which a particular entity lives
  // probably can be u16
  static constexpr chunk_index_t INVALID_CHUNK_INDEX_T = std::numeric_limits<chunk_index_t>::max();


  typedef uint16_t archetype_component_id; // represents a index into the components for a template
  static constexpr archetype_component_id INVALID_ARCHETYPE_COMPONENT_ID = std::numeric_limits<archetype_component_id>::max();

  static constexpr uint32_t MAX_CHUNK_SIZE = 64*1024; // 64 kb

  /// an archetype is the data holder for a specific template basically
/// has no knowledge about an Entities real makup
  class Archetype
  {
  private:
    /// a chunk will store data in a SOA format
    /// the offsets for the SOAs are stored in the Archetype
    /// TODO: maybe look into AOS instead? considering we might get whole units instead
    struct Chunk
    {
      uint8_t *__restrict data = nullptr; // points to chunk start

      [[nodiscard]] inline uint8_t * getData() const { return data; }
      [[nodiscard]] inline uint8_t * getCompArrayUnsafe(uint32_t ofs, uint16_t entity_count) const // pointer to all components of ofs
      {
        return getData() + (ofs * entity_count);
      }
      explicit Chunk(size_t size) : data((uint8_t *)malloc(size)) {}
      Chunk(Chunk &&ref) noexcept {data=ref.data; ref.data = nullptr;}
      Chunk &operator=(Chunk &&ref) noexcept  {data=ref.data; ref.data = nullptr; return *this;}
      // a chunk should never be copied
      Chunk(Chunk &ref) = delete;
      Chunk &operator=(Chunk const &ref) = delete;
      ~Chunk() noexcept {
        if(data) // the object has been moved
          free(data);}
    };
    std::vector<Chunk> chunks; // space is only every allocated, never deallocated
    BitVector avaiableSlots{true}; // lists all the available slots an entity can be created in
    // 'false' means slot isn't avaiable, 'true' means it is
    chunk_index_t last_available_slot = INVALID_CHUNK_INDEX_T; // the most recent avaiable slot turned available.

    uint16_t EntityCount = 0; // how many entities are allocated per Chunk
    uint32_t entity_size = 0;
  public:
    void printChunkBoundries(chunk_index_t chunk_id)
    {
      uint32_t chunk_list_index = (chunk_id / EntityCount);
      LOG("start: {}; end: {}\n", fmt::ptr(this->chunks[chunk_list_index].data), fmt::ptr(this->chunks[chunk_list_index].data+entity_size*EntityCount));
    }
    /// allocates a new chunk
    void AllocateChunk()
    {
      chunks.emplace_back(entity_size*EntityCount);
      avaiableSlots.resize(avaiableSlots.size()+EntityCount, true);
    }
    /// Gets the next available chunk id
    /// \return
    [[nodiscard]] chunk_index_t getNextAvailableChunkId()
    {
      if(last_available_slot == INVALID_CHUNK_INDEX_T)
      {
        last_available_slot = (chunk_index_t)avaiableSlots.size(); // availableSlots.size() will point to the first new data of the new chunk;
        AllocateChunk();
      }
      return last_available_slot;
    }

    explicit Archetype(uint32_t entity_size)
    {
      EntityCount = (uint16_t)(MAX_CHUNK_SIZE/entity_size);
      this->entity_size = entity_size;
    }

    [[nodiscard]] inline void* getCompDataUnsafe(uint32_t comp_ofs, chunk_index_t chunk_id, uint32_t data_size) const
    {
      // Calculate which physical chunk contains this entity
      uint32_t chunk_list_index = chunk_id / EntityCount;
      G_ASSERT(chunk_list_index < this->chunks.size());

      // Calculate the index within that chunk
      uint32_t entity_index_in_chunk = chunk_id % EntityCount;
      G_ASSERT(entity_index_in_chunk < EntityCount);

      // Get the component array for this component type within the chunk
      // comp_ofs is the byte offset where this component type starts within an entity
      // We need to get the array of all instances of this component in the chunk
      uint8_t* component_array_start = chunks[chunk_list_index].getCompArrayUnsafe(comp_ofs, EntityCount);

      // Jump to the specific component instance for our entity
      return component_array_start + entity_index_in_chunk * data_size;
    }

    bool reserveChunkId(chunk_index_t chunk_id)
    {
      if(!avaiableSlots.get(chunk_id))
        EXCEPTION("Tried to reserve the chunk id for a already reserved chunk");
      avaiableSlots.set(chunk_id, false);
      //TODO, improve lookup by checking if a BitVector chunk is all false
      for(chunk_index_t slot = last_available_slot+1; slot < avaiableSlots.size(); slot++)
      {
        if(avaiableSlots.get(slot))
        {
          last_available_slot = slot;
          return true;
        }
      }
      // no more free slots, must allocate
      last_available_slot = (chunk_index_t)avaiableSlots.size(); // availableSlots.size() will point to the first new data of the new chunk;
      AllocateChunk();
      return true;
    }

    void releaseChunkId(chunk_index_t chunk_id)
    {
      avaiableSlots.set(chunk_id, true); // dont care if it was already true or not
      if(chunk_id < last_available_slot)
        last_available_slot = chunk_id;
    }
    //uint16_t entitySize = 0, componentsCnt = 0;
  };

  class Archetypes
  {
  public:



    inline void * getComponentDataUnsafe(archetype_component_id archetype, component_index_t cidx, chunk_index_t chunkId) const
    {
      auto arch_data = &archetypes[archetype];
      archetype_component_id cid = arch_data->INFO.getComponentId(cidx);
      if (cid == INVALID_ARCHETYPE_COMPONENT_ID)
        return nullptr;
      auto storage = &archetypeComponents[cid+arch_data->COMPONENT_OFS];
      return arch_data->ARCHETYPE.getCompDataUnsafe(storage->DATA_OFFSET, chunkId, storage->DATA_SIZE);
    }
    inline void * getComponentDataIdUnsafe(archetype_t archetype, archetype_component_id cid, chunk_index_t chunkId) const
    {
      auto arch_data = &archetypes[archetype];
      auto storage = &archetypeComponents[cid+arch_data->COMPONENT_OFS];
      return arch_data->ARCHETYPE.getCompDataUnsafe(storage->DATA_OFFSET, chunkId, storage->DATA_SIZE);
    }

    [[nodiscard]] inline uint32_t getComponentSizeFromOfs(archetype_component_id component_id, uint32_t ofs) const;
    archetype_t createArchetype(const component_index_t *__restrict components, uint32_t components_cnt,
                         DataComponents &dataComponents, ComponentTypes &componentTypes);
    [[nodiscard]] archetype_component_id getComponentsCount(uint32_t archetype) const;

    [[nodiscard]] component_index_t getComponentUnsafe(uint32_t archetype, archetype_component_id id) const;
    [[nodiscard]] inline uint32_t getArchetypeComponentOfsUnsafe(uint32_t archetype) const
    {
      return archetypes[archetype].COMPONENT_OFS;
    }
  protected:
    friend EntityManager;
    struct ArchetypeInfo
    {
      component_index_t firstNonEidIndex, count;
      std::unique_ptr<archetype_component_id[]> componentIndexToArchetypeOffset; // todo: make it soa as well
      [[nodiscard]] inline archetype_component_id getComponentId(component_index_t cidx) const;
    };

    struct ArchetypeStorage
    {
      Archetype ARCHETYPE;
      uint32_t COMPONENT_OFS; // offset into archetypeComponents where this particular archetype exists
      ArchetypeInfo INFO; // used to convert component_index_t (datacomponent) to archetype_component_id
      archetype_component_id COMPONENT_COUNT;

    };

    /// represents component data in relation to an archetype. it is indexed by archetype_component_id;
    struct ArchetypeComponentStorage
    {
      component_index_t INDEX;
      uint32_t DATA_OFFSET; // offset into entity where you can find the component data
      uint32_t DATA_SIZE; // size of a particular data, assumes a components cant be larger than 65535 bytes
    };

    /// archetype storage
    /// extra fields holds needed metadata about archetype
    std::vector<ArchetypeStorage> archetypes;
    /// global table (for the archetypes) that holds all the components info for all archetypes
    /// each time an archetype is added, this table is expanded by the number of components that archetype has
    std::vector<ArchetypeComponentStorage> archetypeComponents;
  };
  inline archetype_component_id Archetypes::ArchetypeInfo::getComponentId(component_index_t cidx) const
  {
    if (cidx == 0) // eid
      return 0;
    uint32_t at = (uint32_t)((int)cidx - (int)firstNonEidIndex);
    if (at >= count)
      return INVALID_ARCHETYPE_COMPONENT_ID;
    return componentIndexToArchetypeOffset[at];
  }
  inline component_index_t Archetypes::getComponentUnsafe(uint32_t archetype, archetype_component_id id) const
  {
    G_ASSERT(archetype < archetypes.size());
    uint32_t at = id + getArchetypeComponentOfsUnsafe(archetype);
    G_ASSERT(at < archetypeComponents.size());
    return archetypeComponents[at].INDEX;
  }
}


#endif //MYEXTENSION_ARCHETYPES_H
