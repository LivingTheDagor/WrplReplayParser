

#ifndef MYEXTENSION_ARCHETYPES_H
#define MYEXTENSION_ARCHETYPES_H
#include <cmath>
#include "Logger.h"
#include <shared_mutex>
#include "BitVector.h"
#include "ecs/shared_mtx.h"


namespace ecs {

  typedef uint32_t chunk_index_t; // represents the chunk at which a particular entity lives
  // gaijin why
  static constexpr chunk_index_t INVALID_CHUNK_INDEX_T = (1 << (32 - ENTITY_GENERATION_BITS)) - 1;


  typedef uint16_t archetype_component_id; // represents a index into the components for a template
  static constexpr archetype_component_id INVALID_ARCHETYPE_COMPONENT_ID =
    std::numeric_limits<archetype_component_id>::max();

  static constexpr uint32_t MAX_CHUNK_SIZE = 64 * 1024; // 64 kb

  struct MgrArchetypeStorage;
  class GState;
  /// an archetype is the data holder for a specific template basically
  /// has no knowledge about an Entities real makeup
  class Archetype {
  private:
    /// a chunk will store data in a SOA format
    /// the offsets for the SOAs are stored in the Archetype
    struct Chunk {
      uint8_t *__restrict data = nullptr; // points to chunk start
      uint32_t used = 0; // amount of entities in this chunk
      [[nodiscard]] inline uint8_t *getData() const { return data; }
      [[nodiscard]] inline uint8_t *getCompArrayUnsafe(uint32_t ofs,
                                                       uint16_t entity_count) const // pointer to all components of ofs
      {
        return getData() + (ofs * entity_count);
      }
      explicit Chunk(size_t size) : data((uint8_t *) malloc(size)) {}
      Chunk(Chunk &&ref) noexcept {
        data = ref.data;
        ref.data = nullptr;
      }
      Chunk &operator=(Chunk &&ref) noexcept {
        data = ref.data;
        ref.data = nullptr;
        return *this;
      }
      // a chunk should never be copied
      Chunk(Chunk &ref) = delete;
      Chunk &operator=(Chunk const &ref) = delete;
      ~Chunk() noexcept {
        if (data) // the object has been moved
          free(data);
      }
    };
    std::vector<Chunk> chunks; // space is only every allocated, never deallocated
    BitVector avaiableSlots{true}; // lists all the available slots an entity can be created in
    // 'false' means slot isn't avaiable, 'true' means it is
    chunk_index_t last_available_slot = INVALID_CHUNK_INDEX_T; // the most recent avaiable slot turned available.
    uint16_t EntityCount = 0; // how many entities are allocated per Chunk
    uint32_t entity_size = 0;
    friend MgrArchetypeStorage;
    friend GState;

  public:
    void printChunkBoundries(chunk_index_t chunk_id);
    /// allocates a new chunk
    void AllocateChunk() {
      chunks.emplace_back(entity_size * EntityCount);
      ecs::EntityId *ptr = (ecs::EntityId *) (chunks[chunks.size() - 1].getCompArrayUnsafe(0, EntityCount));
      for (int i = 0; i < EntityCount; i++) {
        ptr[i] = ecs::INVALID_ENTITY_ID;
      }
      avaiableSlots.resize(avaiableSlots.size() + EntityCount, true);
    }
    /// Gets the next available chunk id
    /// \return
    [[nodiscard]] chunk_index_t getNextAvailableChunkId() {
      if (last_available_slot == INVALID_CHUNK_INDEX_T) {
        last_available_slot =
          (chunk_index_t)
            avaiableSlots.size(); // availableSlots.size() will point to the first new data of the new chunk;
        AllocateChunk();
      }
      return last_available_slot;
    }

    explicit Archetype(uint32_t entity_size) {
      EntityCount = (uint16_t) (MAX_CHUNK_SIZE / entity_size);
      this->entity_size = entity_size;
    }

    [[nodiscard]] inline void *getCompDataUnsafe(uint32_t comp_ofs, chunk_index_t chunk_id, uint32_t data_size) const {
      ZoneScoped;
      // Calculate which physical chunk contains this entity
      uint32_t chunk_list_index = chunk_id / EntityCount;
      G_ASSERT(chunk_list_index < this->chunks.size());

      // Calculate the index within that chunk
      uint32_t entity_index_in_chunk = chunk_id % EntityCount;
      // G_ASSERT(entity_index_in_chunk < EntityCount); // what was I asserting on, MATH????

      // Get the component array for this component type within the chunk
      // comp_ofs is the byte offset where this component type starts within an entity
      // We need to get the array of all instances of this component in the chunk
      uint8_t *component_array_start = chunks[chunk_list_index].getCompArrayUnsafe(comp_ofs, EntityCount);

      // Jump to the specific component instance for our entity
      return component_array_start + entity_index_in_chunk * data_size;
    }

    bool reserveChunkId(chunk_index_t chunk_id) {

      if (!avaiableSlots.get(chunk_id))
        EXCEPTION("Tried to reserve the chunk id for a already reserved chunk");

      uint32_t chunk_list_index = chunk_id / EntityCount;
      this->chunks[chunk_list_index].used++;
      avaiableSlots.set(chunk_id, false);
      // TODO, improve lookup by checking if a BitVector chunk is all false
      for (chunk_index_t slot = last_available_slot + 1; slot < avaiableSlots.size(); slot++) {
        if (avaiableSlots.get(slot)) {
          last_available_slot = slot;
          return true;
        }
      }
      // no more free slots, must allocate
      last_available_slot =
        (chunk_index_t)
          avaiableSlots.size(); // availableSlots.size() will point to the first new data of the new chunk;
      AllocateChunk();
      return true;
    }

    void releaseChunkId(chunk_index_t chunk_id) {
      avaiableSlots.set(chunk_id, true); // dont care if it was already true or not
      if (chunk_id < last_available_slot)
        last_available_slot = chunk_id;

      uint32_t chunk_list_index = chunk_id / EntityCount;
      this->chunks[chunk_list_index].used--;
    }

    Chunk *getChunkAndOffset(chunk_index_t chunk_id, uint32_t &offset) {
      uint32_t chunk_list_index = chunk_id / EntityCount;
      G_ASSERT(chunk_list_index < this->chunks.size());

      offset = chunk_id % EntityCount;
      return this->chunks.data() + chunk_list_index;
    }

    inline uint16_t getEntityCount() { return EntityCount; }
    // uint16_t entitySize = 0, componentsCnt = 0;
  };

  class SharedMutexWrapper {
  public:
    SharedMutexWrapper() = default;
    SharedMutexWrapper(const SharedMutexWrapper&) = delete;
    SharedMutexWrapper& operator=(const SharedMutexWrapper&) = delete;

    // Basic mutex interface
    void lock() { ZoneScoped; mtx_.lock(); }
    void unlock() { ZoneScoped; mtx_.unlock(); }

    void lock_shared() { ZoneScoped; mtx_.lock_shared(); }
    void unlock_shared() { ZoneScoped; mtx_.unlock_shared(); }

    bool try_lock() { ZoneScoped; return mtx_.try_lock(); }
    bool try_lock_shared() { ZoneScoped; return mtx_.try_lock_shared(); }

    // Barebones RAII unique guard
    class UniqueGuard {
    public:
      explicit UniqueGuard(SharedMutexWrapper& m) : m_(&m), owns_(true) { m_->lock(); }
      ~UniqueGuard() { if (owns_) m_->unlock(); }

      UniqueGuard(const UniqueGuard&) = delete;
      UniqueGuard& operator=(const UniqueGuard&) = delete;

      UniqueGuard(UniqueGuard&& other) noexcept : m_(other.m_), owns_(other.owns_) {
        other.m_ = nullptr;
        other.owns_ = false;
      }

    private:
      SharedMutexWrapper* m_;
      bool owns_;
    };

    // Barebones RAII shared guard
    class SharedGuard {
    public:
      explicit SharedGuard(SharedMutexWrapper& m) : m_(&m), owns_(true) { m_->lock_shared(); }
      ~SharedGuard() { if (owns_) m_->unlock_shared(); }

      SharedGuard(const SharedGuard&) = delete;
      SharedGuard& operator=(const SharedGuard&) = delete;

      SharedGuard(SharedGuard&& other) noexcept : m_(other.m_), owns_(other.owns_) {
        other.m_ = nullptr;
        other.owns_ = false;
      }

    private:
      SharedMutexWrapper* m_;
      bool owns_;
    };

    // Convenience factories
    [[nodiscard]] UniqueGuard unique_guard() { return UniqueGuard(*this); }
    [[nodiscard]] SharedGuard shared_guard() { return SharedGuard(*this); }

  private:
    std::shared_mutex mtx_;
  };

  class MgrArchetypeStorage;

  class Archetypes {
  public:
    archetype_t size() const { return (archetype_t) archetypes.size(); }
    void createArchetype(archetype_t archetype, MgrArchetypeStorage &storage);

    bool archetypeExists(archetype_t archetype, MgrArchetypeStorage &storage);

    archetype_t createArchetype(const component_index_t *__restrict components, uint32_t components_cnt,
                                DataComponents &dataComponents, ComponentTypes &componentTypes,
                                template_t parent_template);
    [[nodiscard]] archetype_component_id getComponentsCount(uint32_t archetype) const;

    [[nodiscard]] component_index_t getComponentUnsafe(uint32_t archetype, archetype_component_id id) const;

    inline uint32_t getArchetypeComponentCount(archetype_t arch) {
      G_ASSERT(arch < this->archetypes.size());
      return this->archetypes[arch]->COMPONENT_COUNT;
    }

  protected:
    dagor::shared_mutex archetypes_mtx{};
    friend GState;
    friend EntityManager; // mgr directly access components for performance, maybe //TODO?
    friend MgrArchetypeStorage;
    struct ArchetypeInfo {
      component_index_t firstNonEidIndex, count;
      std::unique_ptr<archetype_component_id[]> componentIndexToArchetypeOffset; // todo: make it soa as well
      [[nodiscard]] inline archetype_component_id getComponentId(component_index_t cidx) const;
    };
    /// represents component data in relation to an archetype. it is indexed by archetype_component_id;
    struct ArchetypeComponentStorage {
      component_index_t INDEX;
      uint16_t DATA_OFFSET; // offset into entity where you can find the component data
      uint32_t DATA_SIZE; // size of a particular data, assumes a components cant be larger than 65535 bytes
    };

    struct ArchetypeStorage {
      uint32_t ENTITY_SIZE;
      ArchetypeInfo INFO; // used to convert component_index_t (datacomponent) to archetype_component_id
      archetype_component_id COMPONENT_COUNT;
      template_t TEMPLATE;
      std::vector<ArchetypeComponentStorage> components;
    };

    ArchetypeStorage *getArchetypeStorageUnsafe(archetype_t arch) { return this->archetypes[arch]; }
    inline template_t getParentTemplate(archetype_t arch) const;

    /// archetype storage
    /// extra fields holds needed metadata about archetype
    std::vector<ArchetypeStorage*> archetypes;

    ~Archetypes();
  };
  inline archetype_component_id Archetypes::ArchetypeInfo::getComponentId(component_index_t cidx) const {
    if (cidx == 0) // eid
      return 0;
    uint32_t at = (uint32_t) ((int) cidx - (int) firstNonEidIndex);
    if (at >= count)
      return INVALID_ARCHETYPE_COMPONENT_ID;
    return componentIndexToArchetypeOffset[at];
  }

  template_t Archetypes::getParentTemplate(archetype_t arch) const { return archetypes[arch]->TEMPLATE; }

  // holds actual data for a EntityManager instance
  struct MgrArchetypeStorage {
    std::vector<Archetype *> data{};
    std::vector<Archetypes::ArchetypeStorage*> archetypeStorages;
    void constructArch(archetype_t index, Archetypes::ArchetypeStorage* storage) {
      if (data.size() <= index) {
        data.resize(index + 1);
        archetypeStorages.resize(index + 1);
      }
      if (!data[index]) {
        data[index] = new Archetype(storage->ENTITY_SIZE);
        archetypeStorages[index] = storage;
      } else {
        G_ASSERT(data[index]->entity_size ==
                 storage->ENTITY_SIZE); // sanity check, should always match as we construct from Archetypes
      }
    }

    bool hasArchetype(archetype_t index) const { return index < data.size() && data[index] != nullptr; }
    // all construction of a specific archetype happens earlier, so no checks here
    Archetype *getArch(archetype_t archetype) const;

    Archetypes::ArchetypeStorage * getStorageArch(archetype_t archetype) const;

    component_index_t getComponentUnsafe(archetype_t archetype, archetype_component_id id) const;

    /// returns a pointer to some component from a specific archetype
    ///
    /// @param archetype the archetype to look up
    /// @param cid the index of your component into the archetype components
    /// @param chunkId the chunk inside the archetype that holds your component
    /// @return your pointer or null
    void * getComponentDataIdUnsafe(archetype_t archetype, archetype_component_id cid, chunk_index_t chunkId) const;

    void *getComponentDataUnsafe(archetype_t archetype, component_index_t cidx, chunk_index_t chunkId) const;

    ~MgrArchetypeStorage();
  };

} // namespace ecs


#endif // MYEXTENSION_ARCHETYPES_H
