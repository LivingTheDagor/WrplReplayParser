#include "modules/ecs/ECSTypes.h"
#include "modules/ecs/EntityId.h"
#include "modules/Math.h"
#include "ecs/typesAndLimits.h"
#include "ecs/EntityManager.h" // so all types are defined
#include "ecs/ComponentTypesDefs.h"
#include "pybind11/stl_bind.h"

std::unordered_map<ecs::component_type_t, cast_cb> cast_to_py_types;
PyECSTypes py_ecs_types;

struct InvalidData {
  std::string_view type_name;
  std::span<uint8_t> data;

  InvalidData(std::string_view name, std::span<uint8_t> d)
      : type_name(name), data(d) {}
};

struct objectIter {
  size_t index = 0;
  ecs::Object *obj;
  py::object py_ref;

  objectIter(const py::object &obj) : obj(obj.cast<ecs::Object *>()), py_ref(obj) {}

  py::object next() {
    while (true) {
      if (index < obj->size()) {

        auto &curr_ref = obj->container[index];
        index++;
        py::object t_obj = castComponent(&curr_ref.second);
        if (!t_obj.is_none()) {
          return py::make_tuple(&curr_ref.first, t_obj);
        }
      } else {
        throw py::stop_iteration();
      }
    }
  }
};

void PyECSTypes::include(py::module_ &m) {
  DO_INCLUDE()
  py_entity_id.include(m);
  py_math.include(m);
  auto dm = m.def_submodule("dm");
  auto ecs = m.def_submodule("ecs");
  auto unit = m.def_submodule("unit");
  py::class_<InvalidData>(ecs, "InvalidData")
      .def_readonly("componentName", &InvalidData::type_name)
      .def_property_readonly("data", [](const InvalidData &self){
        return py::bytes(reinterpret_cast<const char*>(self.data.data()), self.data.size());
      });

  py::class_<objectIter>(ecs, "ObjectIter")
      .def("__next__", &objectIter::next);

  py::class_<ecs::Object>(ecs, "Object")
      .def("size", &ecs::Object::size)
      .def("empty", &ecs::Object::empty)
      .def("__iter__", [](py::object &obj) {
        return std::make_unique<objectIter>(obj);
      })
      .def("__getitem__", [](ecs::Object &obj, const std::string &key) -> py::object {
        auto val = obj.find(key.c_str());
        if (val != obj.end()) {
          return castComponent(&val->second);
        }
        return py::none{};
      });

  py::class_<SpaceTime>(m, "SpaceTime")
      .def_readonly("time_ms", &SpaceTime::time_ms)
      .def_readonly("location", &SpaceTime::location)
      .def("__str__", [](SpaceTime &st) {
        return fmt::format("SpaceTime({}, [{}, {}, {}])", st.time_ms, st.location.x, st.location.y, st.location.z);
      });

  bind_readonly_vector<std::vector<SpaceTime>>(m, "SpaceTimeList");

  py::enum_<UnitType>(unit, "UnitType")
      .value("TankType", UnitType::TankType)
      .value("AircraftType", UnitType::AircraftType);

  py::class_<unit::Unit> un(unit, "Unit");

  py::class_<unit::Aircraft, unit::Unit>(unit, "Aircraft")
      .def_readonly("positions", & unit::Aircraft::positions);

  py::class_<unit::Tank, unit::Unit> t(unit, "Tank");

  un
      .def_readonly("unitType", &unit::Unit::unitType)
      .def_readonly("uid", &unit::Unit::uid)
      .def("AsAircraft", &unit::Unit::AsAircraft)
      .def("AsTank", &unit::Unit::AsTank);

  py::class_<unit::UnitRef>(unit, "UnitRef")
      .def_readonly("unit", &unit::UnitRef::unit);

  py::class_<FieldSerializerDict>(ecs, "FieldSerializerDict")
      .def_property_readonly("data", [](FieldSerializerDict &dict) -> py::bytes {
        std::string data{};
        data.resize(dict.data.size());
        memcpy(data.data(), dict.data.data(), dict.data.size());
        return {data};
      });

  py::class_<BarrageBalloonStorageComponent, FieldSerializerDict>(ecs, "BarrageBalloonStorageComponent");
  py::class_<LightVehicleModelStorageComponent, FieldSerializerDict>(ecs, "LightVehicleModelStorageComponent");
  py::class_<FortificationModelStorageComponent, FieldSerializerDict>(ecs, "FortificationModelStorageComponent");
  py::class_<WalkerVehicleStorageComponent, FieldSerializerDict>(ecs, "WalkerVehicleStorageComponent");
  py::class_<HumanStorageComponent, FieldSerializerDict>(ecs, "HumanStorageComponent");
  py::class_<InfantryTroopStorageComponent, FieldSerializerDict>(ecs, "InfantryTroopStorageComponent");
  py::class_<WarShipModelStorageComponent, FieldSerializerDict>(ecs, "WarShipModelStorageComponent");
  py::class_<HeavyVehicleModelStorageComponent, FieldSerializerDict>(ecs, "HeavyVehicleModelStorageComponent");
  py::class_<FlightModelWrapStorageComponent, FieldSerializerDict>(ecs, "FlightModelWrapStorageComponent");

  REGISTER_TYPE(BarrageBalloonStorageComponent);
  REGISTER_TYPE(LightVehicleModelStorageComponent);
  REGISTER_TYPE(FortificationModelStorageComponent);
  REGISTER_TYPE(WalkerVehicleStorageComponent);
  REGISTER_TYPE(HumanStorageComponent);
  REGISTER_TYPE(InfantryTroopStorageComponent);
  REGISTER_TYPE(WarShipModelStorageComponent);
  REGISTER_TYPE(HeavyVehicleModelStorageComponent);
  REGISTER_TYPE(FlightModelWrapStorageComponent);
  REGISTER_TYPE(unit::UnitRef);
  REGISTER_TYPE(ecs::Object);
  REGISTER_TYPE(ecs::EntityId);
  REGISTER_TYPE(ecs::string);
  REGISTER_TYPE(uint8_t);
  REGISTER_TYPE(uint16_t);
  REGISTER_TYPE(uint32_t);
  REGISTER_TYPE(uint64_t);
  REGISTER_TYPE(int8_t);
  REGISTER_TYPE(int16_t);
  REGISTER_TYPE(int);
  REGISTER_TYPE(int64_t);
  this->include_array(m);
}

py::object castData(ecs::component_type_t hash, const void *data) {
  if (!data) {
    return py::none{};
  }
  auto lookup = cast_to_py_types.find(hash);
  if (lookup != cast_to_py_types.end()) {
    return lookup->second(data);
  }
  auto name = ecs::g_ecs_data->getComponentTypes()->getName(hash);
  auto sz = ecs::g_ecs_data->getComponentTypes()->getComponentData(hash)->size;
  return py::cast(std::make_unique<InvalidData>(name, std::span<uint8_t>((uint8_t *) data, sz)));
}

py::object castComponent(const ecs::Component *comp) {
  auto type = comp->getUserType();
  auto lookup = cast_to_py_types.find(type);
  if (lookup != cast_to_py_types.end()) {
    return lookup->second(comp->getRawData());
  }
  auto name = ecs::g_ecs_data->getComponentTypes()->getName(comp->getTypeId());
  return py::cast(std::make_unique<InvalidData>(name, std::span<uint8_t>((uint8_t *) comp->getRawData(), comp->getSize())));
}

