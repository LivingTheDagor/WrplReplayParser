#pragma once
#include "EASTL/fixed_function.h"
#include "Unit.h"
#include "ecs/componentTypesDefs.h"

typedef eastl::fixed_function<sizeof(void *) * 2, void(const unit::Unit &)> on_unit_cb_t;
typedef eastl::fixed_function<sizeof(void *) * 2, void(const Rocket &)> on_rocket_cb_t;
typedef eastl::fixed_function<sizeof(void *) * 2, void(const Bomb &)> on_bomb_cb_t;
