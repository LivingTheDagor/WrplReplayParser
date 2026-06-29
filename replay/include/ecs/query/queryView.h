

#ifndef WTFILEUTILS_QUERYVIEW_H
#define WTFILEUTILS_QUERYVIEW_H

namespace ecs {

  class QueryIdDummy {};

  // while I dont really want to use this im also too lazy to update dagor engine code I copied
  typedef GenerationRefId<8, QueryIdDummy> QueryId;

  // a QueryView holds a reference to the RW and RO components a specific query requests
  // it can reference all the needed components of entities in a single chunk
  class QueryView {
  public:
    typedef void *__restrict ComponentsData;
    const ComponentsData *__restrict componentData =
      nullptr; // chunk data // references the arrays based on the component layout found in the ecs::ComponentDesc[]
               // this event uses
    ecs::EntityId *eid_refs; // we use eid to determine if an entity exists in a slot or not.
    // im assuming dagor engine actually defragments an archetype before running a query
    uint32_t index_start; // where in the chunk entities start
    uint32_t index_end; // where in the chunk entities end
    EntityManager *mgr;
    QueryId id;
    uint32_t num_of_entities = 1; // number of entities this points to, for sendEvent, this will always be 1, for
                                  // BroadcastEvent, this is set to entity count in a chunk
    struct RWData {
      uint8_t roCount, rwCount;
    };
    union {
      RWData data;
      uint16_t roRW = 0;
    };
    // msvc eat it
#ifdef _MSC_VER
    [[nodiscard]] ComponentsData getComponentUntypedData(uint16_t compId) const { return componentData[compId]; }
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
    [[nodiscard]] ComponentsData getComponentUntypedData(uint16_t compId) const { return componentData[compId]; }
#pragma GCC diagnostic pop
#endif


    template<class T>
    const T *getComponentRawRO(uint16_t compId) const {
      T *ret = (T *) getComponentUntypedData(compId);
      return ret;
    }

    template<class T>
    T *getComponentRawRW(uint16_t compId) const {
      T *ret = (T *) getComponentUntypedData(compId);
      return ret;
    }

    template<class T, typename U = T>
    [[nodiscard]] auto &getComponentRW(uint16_t compId, uint32_t idInChunk) const {
      auto p = getComponentUntypedData(compId);
      G_ASSERT(p);
      return ((T *) p)[idInChunk];
      // return ref<T, U>(p, idInChunk);
    }
    template<class T, typename U = T>
    [[nodiscard]] auto &getComponentRO(uint16_t compId, uint32_t idInChunk) const {
      auto p = getComponentUntypedData(compId);
      G_ASSERT(p);
      return ((const T *) p)[idInChunk];
      // return ref<T, U, /*bConst*/ true>(p, idInChunk);
    }

    friend GState;
    QueryView(EntityManager *mgr) { this->mgr = mgr; }
    QueryView(EntityManager &mgr) { this->mgr = &mgr; }
    [[nodiscard]] uint32_t begin() const;
    [[nodiscard]] uint32_t end() const;
  };
  inline uint32_t QueryView::begin() const { return index_start; }
  inline uint32_t QueryView::end() const { return index_end; }
} // namespace ecs

#endif // WTFILEUTILS_QUERYVIEW_H
