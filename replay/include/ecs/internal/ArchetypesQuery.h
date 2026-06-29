#pragma once


namespace ecs {
  /// am ArchetypesQuery stores all the archetypes that a specific query will ever access
  /// it has space for 9 archetypes inplace, and then you allocate
  /// an ArchetypeQuery also stores the offset that each component this query uses in a double array
  /// mostly copied
  struct alignas(16) ArchetypesQuery {
    [[nodiscard]] uint32_t getComponentsCount() const { return data.rwCount + data.roCount; }
    [[nodiscard]] inline bool hasComponents() const {
      G_STATIC_ASSERT(offsetof(ArchetypesQuery, data.rwCount) == offsetof(ArchetypesQuery, data.roCount) + 1);
      G_STATIC_ASSERT(sizeof(data.rwCount) == 1 && sizeof(data.roCount) == 1 && sizeof(roRW) == 2);
      G_STATIC_ASSERT(offsetof(ArchetypesQuery, queries) % sizeof(void *) == 0);
      return roRW != 0;
    }
    struct rwData {
      uint8_t roCount, rwCount;
    };
    union {
      rwData data;
      uint16_t roRW;
    };
    archetype_t queriesCount = 0; // number of archetypes

    static constexpr int max_inline_archs_count = 9;

    archetype_t firstArch = 0, secondArch = 0;
    union {
      archetype_t *queries = nullptr; // if queriesCount > max_inline
      archetype_t arches[4]; // other inplace archetypes, starting from fourth, if queriesCount < max_inline
    };
    archetype_t lastArches0 = 0, lastArches1 = 0, lastArch = 0; //+3 more archetypes
    // we dont care about tracked, this 'ECS' is for a replay parser so all that logic is just being handled secondhand
    // (maybe maybe maybe), im just currently hoping I dont have to deal with that rn
    //   uint16_t trackedChangesCount = 0;

    typedef uint16_t offset_type_t;
    static constexpr int INVALID_OFFSET = std::numeric_limits<offset_type_t>::max();
    union {
      offset_type_t *allComponentsArchOffsets =
        nullptr; // if allocated externally, i.e. total amount of offsets is bigger than 4
      offset_type_t offsets[16]; // if allocated inline, i.e. maximum 8 offsets (2 archetype with 4 components, 4 with 2
                                 // or 8 archetypes with 1 component max)
    };
    uint32_t getQueriesCount() const { return queriesCount; }

    static inline bool isInplaceQueries(size_t cnt) { return cnt <= max_inline_archs_count; }
    [[nodiscard]] inline bool isInplaceQueries() const { return isInplaceQueries(queriesCount); }

    std::span<const archetype_t> getQuerySpan() {
      if (isInplaceQueries()) {
        return {&this->firstArch, this->queriesCount};
      }
      return {this->queries, this->queriesCount};
    }

    // I dont know why it does it like this and I dont feel like reading right now
    static constexpr int inplace_offsets_count_bits = 4; // 1<<bits max, and 0 is impossible, so we check bits for cnt-1
    static inline bool isInplaceOffsets(size_t cnt) { return ((cnt - 1) >> inplace_offsets_count_bits) == 0; }
    inline bool isInplaceOffsets() const { return isInplaceOffsets(getAllComponentsArchOffsetsCount()); }

    [[nodiscard]] uint32_t getAllComponentsArchOffsetsCount() const { return getQueriesCount() * getComponentsCount(); }
    [[nodiscard]] const offset_type_t *getArchetypeOffsetsPtrInplace() const { return offsets; }
    offset_type_t *getArchetypeOffsetsPtrInplace() { return offsets; }

    [[nodiscard]] inline const archetype_t *queriesInplace() const { return &firstArch; }
    [[nodiscard]] inline const archetype_t *queriesBegin() const {
      return isInplaceQueries() ? queriesInplace() : queries;
    }
    [[nodiscard]] const archetype_t *queriesEnd() const { return queriesBegin() + queriesCount; }
    [[nodiscard]] const offset_type_t *getArchetypeOffsetsPtr() const {
      return isInplaceOffsets() ? getArchetypeOffsetsPtrInplace() : allComponentsArchOffsets;
    }

    [[nodiscard]] std::span<const offset_type_t> getArchetypeOffsetsForQuery(uint32_t offset) const {
      uint32_t count = getComponentsCount();
      G_ASSERT(offset < this->getQueriesCount());
      return {getArchetypeOffsetsPtr() + offset * count, count};
    }
    void reset() {
      // allComponentsId.resize(0);
      if (!isInplaceQueries())
        free(queries);
      queries = nullptr;
      if (!isInplaceOffsets())
        free(allComponentsArchOffsets);
      allComponentsArchOffsets = nullptr;
      roRW = 0;
      queriesCount = 0;
      firstArch = lastArch = 0;
    }
    ~ArchetypesQuery() { reset(); }
    ArchetypesQuery() = default;
    ArchetypesQuery(ArchetypesQuery &&a) noexcept :
      roRW(a.roRW),
      queriesCount(a.queriesCount),
      firstArch(a.firstArch),
      secondArch(a.secondArch),
      queries(a.queries),
      lastArches0(a.lastArches0),
      lastArches1(a.lastArches1),
      lastArch(a.lastArch),
      allComponentsArchOffsets(a.allComponentsArchOffsets) {
      a.queries = nullptr;
      a.queriesCount = 0;
      a.allComponentsArchOffsets = nullptr;
    }
  };

  struct ArchetypesEidQuery {
    // actually, this pointer can be duplicated for some queries. Many
    // queries can have exact same archetypes fitting, so we'd better
    // store just non-owning pointer/offset here
    uint32_t componentsSizesAt = ~0u;
    void reset() { componentsSizesAt = ~0u; }
  };
}