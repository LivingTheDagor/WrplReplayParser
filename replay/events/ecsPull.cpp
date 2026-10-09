
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/query/entitySystem.h>
#include <ecs/component.h>
#include <ecs/componentsMap.h>
#include <ecs/ComponentTypes.h>
#include <ecs/EntityManager.h>
#include "Pull.h"

#define REG_SYS     \
  RS(BaseEntity)    \
  RS(UpdateMPlayer) \
  RS(Rocketry)


#define RS(x) DECL_PULL_VAR(x);
REG_SYS
#undef RS

// this var is required to actually pull static ctors from EntitySystem's objects that otherwise have no other publicly
// visible symbols
volatile size_t ecs_primary_pulls = 0
#define RS(x) + PULL_VAR(x)
  REG_SYS
#undef RS

  ;