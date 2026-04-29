#include <utility>

#include "modules/ecs/ECSQueryES.h"
#include "modules/ecs/EntityManager.h"
#include "modules/ecs/ECSTypes.h"
#include "dag_assert.h"
#include "ecs/EntityManager.h"
#include "ecs/query/event.h"
#include "ecs/query/entitySystem.h"

PyECSQueryES py_ecs_query_es;

struct PythonEsSystem {
  py::function cb;
  std::string es_name{};
  // no rw components, rw is impossible in python
  std::vector<std::string> ro_components{};
  std::vector<std::string> rq_components{};
  std::vector<std::string> no_components{};
  std::vector<ecs::ComponentDesc> ro_descs{};
  std::vector<uint32_t> component_sizes{};
  std::vector<ecs::ComponentDesc> rq_descs{};
  std::vector<ecs::ComponentDesc> no_descs{};
  ecs::EventSet events{};
  std::unique_ptr<ecs::EntitySystemDesc> desc;
  PythonEsSystem(py::function cb, std::string name, std::vector<std::string> ro_comps, std::vector<std::string> rq_comps, std::vector<std::string> no_comps, const std::vector<uint32_t> &events)
  : cb(std::move(cb)), es_name(std::move(name)), ro_components(std::move(ro_comps)), rq_components(std::move(rq_comps)), no_components(std::move(no_comps)) {
    for(uint32_t ev : events) {
      this->events.insert(ev);
    }
  }
};

bool has_initialized = false;

std::vector<std::unique_ptr<PythonEsSystem>> registered_systems{};
void registerEsSystem(const py::function& method,
                      const std::string& es_name,
                      const std::vector<std::string>& rq_components,
                      const std::vector<std::string>& no_components,
                      const std::vector<uint32_t>& events) {
  if(has_initialized) {
    EVENT_LOGE("tried to register an ES system after global initialize called");
    return;
  }
  std::vector<std::string> ro_components{};
  py::object inspect = py::module_::import("inspect");
  py::object signature = inspect.attr("signature")(method);
  py::dict params = signature.attr("parameters");
  bool ignored_first = false;
  for (auto item : params) {
    if(ignored_first) {
      std::string name = py::str(item.first);
      ro_components.push_back(name);
    } else {
      ignored_first = true;
    }
  }
  registered_systems.push_back(std::make_unique<PythonEsSystem>(method, es_name, ro_components, rq_components, no_components, events));
}

void on_callback(ecs::GState * state) {
  G_ASSERT(!has_initialized);
  has_initialized = true;
  for(auto & c : registered_systems) {
      c->ro_descs.reserve(c->ro_components.size());
      c->component_sizes.reserve(c->ro_components.size());
      for(auto & s : c->ro_components) {
        HashedConstString hashed = ECS_HASH(s.c_str());
        auto data_comp = state->getDataComponents()->getDataComponent(hashed.hash);
        if(!data_comp) {
          EVENT_LOGE("failed to find RO data component \"{}\" for es system {}", s, c->es_name);
          goto continue_label;
        }
        c->ro_descs.emplace_back(hashed, data_comp->componentHash);
        auto comp = state->getComponentTypes()->getComponentData(data_comp->componentIndex);
        c->component_sizes.emplace_back(comp->size);
      }

      c->rq_descs.reserve(c->rq_components.size());
      for(auto & s : c->rq_components) {
        HashedConstString hashed = ECS_HASH(s.c_str());
        auto data_comp = state->getDataComponents()->getDataComponent(hashed.hash);
        if(!data_comp) {
          EVENT_LOGE("failed to find RQ data component \"{}\" for es system {}", s, c->es_name);
          goto continue_label;
        }
        c->rq_descs.emplace_back(hashed, data_comp->componentHash);
      }

      c->no_descs.reserve(c->no_components.size());
      for(auto & s : c->no_components) {
        HashedConstString hashed = ECS_HASH(s.c_str());
        auto data_comp = state->getDataComponents()->getDataComponent(hashed.hash);
        if(!data_comp) {
          EVENT_LOGE("failed to find NO data component \"{}\" for es system {}", s, c->es_name);
          goto continue_label;
        }
        c->no_descs.emplace_back(hashed, data_comp->componentHash);
      }

      c->desc = std::make_unique<ecs::EntitySystemDesc>(
          c->es_name.c_str(),
          "python/ecs/ECSQueryES.cpp",
          ecs::EntitySystemOps([&c](ecs::EntityManager *mgr, const ecs::Event &evt, const ecs::QueryView &components) {

            auto compBegin = components.begin(), compEnd = components.end();
            G_ASSERT(compBegin != compEnd);
            if (PyGILState_Check()) {
              do {
                auto eid = components.eid_refs[compBegin];
                if (eid != ecs::INVALID_ENTITY_ID) {
                  py::tuple tup(c->ro_descs.size()+1);
                  tup[0] = py::cast(&evt);
                  for(size_t i = 0; i < c->ro_descs.size(); i++) {
                    tup[i+1] = castData(c->ro_descs[i].type, ((uint8_t*)components.componentData[i])+compBegin*c->component_sizes[i]);
                  }
                  c->cb(*tup);
                }
              } while (++compBegin != compEnd);
            } else {
              py::gil_scoped_acquire gil;
              do {
                auto eid = components.eid_refs[compBegin];
                if (eid != ecs::INVALID_ENTITY_ID) {
                  py::tuple tup(c->ro_descs.size()+1);
                  tup[0] = py::cast(&evt); // make event first arg
                  for(size_t i = 0; i < c->ro_descs.size(); i++) {
                    tup[i+1] = castData(c->ro_descs[i].type, ((uint8_t*)components.componentData[i])+compBegin*c->component_sizes[i]); // pass incoming component data
                    // we can do this because our EsSystem is made from the function parameters
                  }
                  g_log_handler->wait_until_empty();
                  g_log_handler->flush_all();
                  c->cb(*tup);
                }
              } while (++compBegin != compEnd);
            }

          }),
          ecs::empty_span(),
          ecs::make_span(c->ro_descs.data(), c->ro_descs.size()),
          ecs::make_span(c->rq_descs.data(), c->rq_descs.size()),
          ecs::make_span(c->no_descs.data(), c->no_descs.size()),
          std::move(c->events),
          0
      );
  continue_label:
    continue;
  }
}

void PyECSQueryES::include(py::module_ &m) {
  DO_INCLUDE()
  py_entity_manager.include(m);
  ecs::after_comps_callbacks->push_back(on_callback);
  auto ecs = m.def_submodule("ecs");
  py::class_<ecs::Event>(ecs, "Event")
      .def_property_readonly("type", &ecs::Event::getType)
      .def_property_readonly("name", &ecs::Event::getName)
      .def_property_readonly("length", &ecs::Event::getLength);
  py::class_<ecs::EventEntityCreated, ecs::Event>(ecs, "EventEntityCreated");
  py::class_<ecs::EventEntityDestroyed, ecs::Event>(ecs, "EventEntityDestroyed");
  ecs.attr("EventEntityCreatedType") = ecs::EventEntityCreated::staticType();
  ecs.attr("EventEntityDestroyedType") = ecs::EventEntityDestroyed::staticType();
  ecs.def("registerEsSystem", registerEsSystem, py::arg("method"), py::arg("esSystemName"), py::arg("RQComponents"), py::arg("NOComponents"), py::arg("EventTypes"));
}
