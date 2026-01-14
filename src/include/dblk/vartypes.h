

#ifndef MYEXTENSION_VARTYPES_H
#define MYEXTENSION_VARTYPES_H

#include <cstdint>
#include "limits"
#include "vector"
#include "dag_assert.h"
typedef int name_id;
typedef uint32_t hash_t;
static constexpr name_id INVALID_NAME_ID =  -std::numeric_limits<name_id>::max();

#endif //MYEXTENSION_VARTYPES_H
