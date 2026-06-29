
// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/query/entitySystem.h>
#include <ecs/Component.h>
#include <ecs/componentsMap.h>
#include <ecs/componentTypes.h>
#include <ecs/EntityManager.h>

#define REG_SYS     \
  RS(BaseEntity)    \
  RS(UpdateMPlayer) \
  RS(Rocketry)


#define RS(x) ECS_DECL_PULL_VAR(x);
REG_SYS
#undef RS

// this var is required to actually pull static ctors from EntitySystem's objects that otherwise have no other publicly
// visible symbols
volatile size_t framework_primary_pulls = 0
#define RS(x) +ECS_PULL_VAR(x)
  REG_SYS
#undef RS

  ;