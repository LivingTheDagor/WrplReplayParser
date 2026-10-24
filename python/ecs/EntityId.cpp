#include <math/dag_Point2.h>
#include "modules/ecs/EntityId.h"
#include "ecs/entityId.h"
#include "daNet/bitStream.h"
#include "network/eid.h"

PyEntityId py_entity_id;

void PyEntityId::include(py::module_ &m) {
  DO_INCLUDE()
  auto ecs = m.def_submodule("ecs");
  py::class_<ecs::EntityId>(ecs, "EntityId")
      .def(py::init<uint32_t>(), py::arg("handle"))
      .def("get_handle", &ecs::EntityId::get_handle)
      .def("index", &ecs::EntityId::index)
      .def("get_generation", &ecs::EntityId::get_generation)
      .def("__bool__", &ecs::EntityId::operator bool)
      .def("__str__", [](ecs::EntityId &eid) -> std::string {
        return fmt::format("{:#08x}", eid.get_handle());
      });

  ecs.def("read_eid", [](BitStream &bs) -> ecs::EntityId{
    ecs::EntityId eid;
    if(net::read_eid(bs, eid)) {
      return eid;
    }
    return ecs::INVALID_ENTITY_ID;
  });
}