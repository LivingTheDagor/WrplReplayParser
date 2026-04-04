#include "ecs/EntityManager.h"
#include "modules/ecs/EntityManager.h"
#include "modules/ecs/EntityId.h"
#include "modules/ecs/Entity.h"

PyEntityManager py_entity_manager;
void PyEntityManager::include(py::module_ &m) {
  DO_INCLUDE()
  py_entity.include(m);
  auto ecs = m.def_submodule("ecs");
  py::class_<ecs::EntityManager>(ecs, "EntityManager")
      .def("getEntityTemplateName", &ecs::EntityManager::getEntityTemplateName, py::arg("eid"))
      .def("getUnit", &ecs::EntityManager::getUnit, py::arg("uid"))
      .def("getEntity", [](ecs::EntityManager &mgr, ecs::EntityId &eid) -> py::object {
        if(mgr.doesEntityExist(eid)) {
          auto templ = mgr.getEntityTemplateId(eid);
          auto t = ecs::g_ecs_data->getTemplateDB()->getTemplate(templ);
          return py::cast(std::make_unique<Entity>(eid, t, &mgr));
        }
        return py::none{};
      }, py::arg("eid"));
}