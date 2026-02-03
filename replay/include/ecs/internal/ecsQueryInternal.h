

#ifndef WTFILEUTILS_ECSQUERYINTERNAL_H
#define WTFILEUTILS_ECSQUERYINTERNAL_H


#include <EASTL/bitvector.h>

namespace ecs {

  static constexpr int MAX_OPTIONAL_PARAMETERS = 128;
  typedef eastl::bitset<MAX_OPTIONAL_PARAMETERS> OptionalMask;

  struct ResolvedQueryDesc // resolved query won't change ever (if it was successfully resolved)
  {
    typedef uint8_t required_components_count_t;
    // one fixed_vector + ranges. is more cache friendly
    // struct Range { uint16_t start = 0, cnt = 0; bool isInside(uint16_t a) const{return a>=start && a - start < cnt;}};
    struct Range
    {
      uint16_t start, cnt;
      [[nodiscard]] bool isInside(uint16_t a) const { return a >= start && a - start < cnt; }
    };
    // pretend this is a std::vector<component_index_t> and your problems go away.
    typedef dag::RelocatableFixedVector<component_index_t, 7, true, MidmemAlloc, uint16_t> components_vec_t;

  private:
    union
    {
      struct
      {
        required_components_count_t requiredComponentsCnt;
        uint8_t rwCnt, roCnt, rqCnt;
      };
      uint64_t ranges = 0;
    };
    OptionalMask optionalMask;
    components_vec_t components; // descriptors of components that this ES needs. can and should be replaced with uint16_t start, count;
  public:
    required_components_count_t requiredComponentsCount() const { return requiredComponentsCnt; }
    void setRequiredComponentsCount(required_components_count_t s) { requiredComponentsCnt = s; }
    OptionalMask &getOptionalMask() { return optionalMask; }
    const OptionalMask &getOptionalMask() const { return optionalMask; }
    const components_vec_t &getComponents() const { return components; }
    components_vec_t &getComponents() { return components; }

    Range getNO() const { return Range{getNoStart(), getNoCnt()}; }
    Range getRQ() const { return Range{uint16_t(rwCnt + roCnt), rqCnt}; }
    Range getRO() const { return Range{rwCnt, roCnt}; }
    Range getRW() const { return Range{0, rwCnt}; }
    uint16_t getNoStart() const { return uint16_t(rwCnt + roCnt + rqCnt); }
    uint8_t getNoCnt() const { return uint8_t(components.size() - getNoStart()); }
    uint8_t getRqCnt() const { return rqCnt; }
    uint8_t getRwCnt() const { return rwCnt; }
    uint8_t getRoCnt() const { return roCnt; }
    uint8_t &getRqCnt() { return rqCnt; }
    uint8_t &getRwCnt() { return rwCnt; }
    uint8_t &getRoCnt() { return roCnt; }
    void reset()
    {
      components.clear();
      optionalMask.reset();
      ranges = 0;
    }
    bool isEmpty() const { return components.empty(); }
    bool operator==(const ResolvedQueryDesc &b)
    {
      if (ranges != b.ranges)
        return false;
      if (optionalMask != b.optionalMask)
        return false;
      return components.size() == b.components.size() &&
             memcmp(components.data(), b.components.data(), components.size() * sizeof(component_index_t)) == 0;
    }
    size_t memUsage() const;
  };

  inline size_t ResolvedQueryDesc::memUsage() const { return sizeof(component_index_t) * components.capacity() + sizeof(optionalMask); }

  // currently, an ArchetypeQuery just holds a list of archetypes that this query matches too
  struct ArchetypeQuery {
    std::vector<archetype_t> indexes;
  };

}

#endif //WTFILEUTILS_ECSQUERYINTERNAL_H
