#include "modules/replay/replay.h"
#include "modules/codegenObjects.h"
#include "modules/State.h"
#include "modules/ecs/EntityManager.h"
#include "modules/ecs/GState.h"
#include "modules/ecs/ECSQueryES.h"

PYBIND11_MODULE(PyReplayParser, m) {
  py_replay.include(m);
  py_entity_manager.include(m);
  py_replay_state.include(m);
  py_global_state.include(m);
  py_ecs_query_es.include(m);
}