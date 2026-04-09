

#ifndef WTFILEUTILS_TYPES_H
#define WTFILEUTILS_TYPES_H
#include "string"
#include "cstdint"
#include "dag_assert.h"
#include "vector"
#include "array"
#include "ecs/entityId.h"
#include "math/dag_Point3.h"
namespace danet {
  struct UnitId { // so that Uid and uint16_t can have different encoders
    uint16_t val{};
  };
}

#include "codegen/types.h" // generated types

namespace danet {
#pragma pack(push, 1) // FUCK OFF COMPILER THIS IS 90 BYTES not 96 CAUSE GAIJIN SAID SO
  struct Uid {
    uint64_t player_id{};
    char name[82]{};

    std::string_view get_player_name() {
      return std::string_view(name, std::min(strlen(name), sizeof(name)));
    }
  };
#pragma pack(pop)

  struct SpaceTime {
    uint32_t time_ms=0;
    Point3 location{};
  };
}
G_STATIC_ASSERT(sizeof(danet::Uid) == 90);


#endif //WTFILEUTILS_TYPES_H
