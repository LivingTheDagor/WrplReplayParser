#pragma once

#include "limits"
#include "cstdint"

namespace ecs {

  // #define CHAR_BIT 8
  static constexpr int MAX_POSSIBLE_WORKERS_COUNT = 8; // max number of parallel workers

  typedef uint32_t components_masks_t[2]; // first rw, second ro. should be removed, I assume

  typedef uint32_t component_type_t; // this is hash of name, component
  typedef uint32_t component_t; // this is hash of name, datacomponent

  typedef uint16_t type_index_t; // limit types to 64k, component
  static constexpr type_index_t INVALID_COMPONENT_TYPE_INDEX = std::numeric_limits<type_index_t>::max();

  typedef uint16_t component_index_t; // limit different components 64k, datacomponent
  static constexpr component_index_t INVALID_COMPONENT_INDEX = std::numeric_limits<component_index_t>::max();

  typedef uint16_t template_t; // limit different templates to 64k
  static constexpr template_t INVALID_TEMPLATE_INDEX = std::numeric_limits<template_t>::max();


  typedef uint32_t ecs_query_handle_t;
  static constexpr ecs_query_handle_t INVALID_QUERY_HANDLE_VAL = std::numeric_limits<ecs_query_handle_t>::max();

  static constexpr uint32_t max_alignment_bits = 4; // if we wil be working in AVX, it will have to be 256 bits,
                                                    // i.e. 32. Currently assume it is 16
  static constexpr uint32_t max_alignment = 1 << max_alignment_bits; // if we wil be working in AVX, it will have to be
                                                                     // 256 bits, i.e. 32. Currently assume it is 16

  // all data components to be stored in a very simple way.
  // one chunk (persumably allocated per template) have all of data components in linear order, separated by capacity

  typedef uint8_t chunk_type_t;
  typedef uint16_t id_in_chunk_type_t;
  // #define ECS_MAX_CHUNK_ID_BITS 16
  static constexpr int ECS_MAX_CHUNK_ID_BITS = sizeof(id_in_chunk_type_t) * 8;
  static constexpr int MAX_CHUNKS_COUNT = std::numeric_limits<chunk_type_t>::max();

  typedef uint16_t archetype_t;
  static constexpr archetype_t INVALID_ARCHETYPE = std::numeric_limits<archetype_t>::max();


  typedef uint16_t component_flags_t;

  struct FastGetInfo {
    component_index_t cidx = INVALID_COMPONENT_INDEX;
    [[nodiscard]] component_index_t getCidx() const { return cidx; }
    [[nodiscard]] bool canBeTracked() const { return canTrack; }
    [[nodiscard]] bool isValid() const { return valid; }

  protected:
    bool canTrack = true;
    bool valid = true;
    friend struct LTComponentList;
    friend class EntityManager;
    friend struct DataComponents;
  };

#ifndef DAECS_EXTENSIVE_CHECKS
#ifdef _DEBUG_TAB_
#define DAECS_EXTENSIVE_CHECKS (DAGOR_DBGLEVEL > 0)
#else
#define DAECS_EXTENSIVE_CHECKS 0
#endif
#endif

#if DAGOR_DBGLEVEL > 0
#define DAECS_RELEASE_INLINE inline
#else
#define DAECS_RELEASE_INLINE __forceinline
#endif

}; // namespace ecs
