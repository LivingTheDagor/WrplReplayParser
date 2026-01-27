

#ifndef WTFILEUTILS_TYPES_H
#define WTFILEUTILS_TYPES_H

namespace danet {
  typedef int zigZagInt;

  template <typename T>
  class zigZagVector : public std::vector<T> {};
  struct UnitId {
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
      return std::string_view(name, sizeof(name));
    }
  };
#pragma pack(pop)

}
G_STATIC_ASSERT(sizeof(danet::Uid) == 90);


#endif //WTFILEUTILS_TYPES_H
