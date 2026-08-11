#include "modules/ecs/GState.h"
#include "ecs/EntityManager.h"
#include "modules/ecs/ECSTypes.h"

PyGState py_global_state;

void PyGState::include(py::module_ &m) {
  DO_INCLUDE()
  auto ecs = m.def_submodule("ecs");
  py::class_<ecs::ComponentInfo>(ecs, "ComponentInfo")
      .def_readonly("size", &ecs::ComponentInfo::size)
      .def_readonly("hash", &ecs::ComponentInfo::hash)
      .def_property_readonly("name", [](ecs::ComponentInfo &info){
        std::string t_str(info.name);
        return t_str;
      });
  py::class_<ecs::DataComponent>(ecs, "DataComponent")
      .def_readonly("hash", &ecs::DataComponent::hash)
      .def_readonly("componentIndex", &ecs::DataComponent::componentIndex)
      .def_readonly("componentHash", &ecs::DataComponent::componentHash)
      .def_property_readonly("name", [](ecs::DataComponent &comp){
        std::string payload{comp.getName()};
        return payload;
      });
  // there shouldn't be a case where you are trying to look up a component
  bind_readonly_vector_no_contain<std::vector<ecs::ComponentInfo>>(ecs, "ComponentInfoVector");
  bind_readonly_vector_no_contain<std::vector<ecs::DataComponent>>(ecs, "DataComponentVector");
  py::class_<ecs::ComponentTypes>(ecs, "ComponentTypes")
      .def_readonly("components", &ecs::ComponentTypes::types)
      .def("getComponent", [](ecs::ComponentTypes &self, ecs::component_type_t &hash) -> std::optional<ecs::ComponentInfo*> {
        auto comp = self.getComponentData(hash);
        if(!comp) {
          return std::nullopt;
        }
        return comp;
      }, py::arg("hash"), py::return_value_policy::reference_internal);

  py::class_<ecs::DataComponents>(ecs, "DataComponents")
      .def_readonly("dataComponents", &ecs::DataComponents::components)
      .def("getDataComponent", [](ecs::DataComponents &self, ecs::component_t &hash) -> std::optional<ecs::DataComponent*> {
        auto comp = self.getDataComponent(hash);
        if(!comp) {
          return std::nullopt;
        }
        return comp;
      }, py::arg("hash"), py::return_value_policy::reference_internal);
  py::class_<ecs::GState>(ecs, "GState")
      .def_readonly("componentTypes", &ecs::GState::componentTypes)
      .def_readonly("dataComponents", &ecs::GState::dataComponents);
  ecs.def("g_ecs_state", [](){
    return ecs::g_ecs_data.get();
  }, py::return_value_policy::reference);
}