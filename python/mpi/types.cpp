#include "modules/mpi/types.h"
#include "modules/mpi/codegen_types.h"
#include "modules/DataBlock/DataBlock.h"
#include "modules/ecs/EntityId.h"
#include "mpi/types.h"
#include "modules/bind_readonly_vector.h"
#include "Unit.h"

PyMpiTypes py_mpi_types;

void PyMpiTypes::include(py::module_ &m) {
  DO_INCLUDE()
  py_data_block.include(m);
  py_entity_id.include(m);

  auto mpi = m.def_submodule("mpi");
  py::class_<danet::Uid>(mpi, "Uid")
    .def_readonly("player_id", &danet::Uid::player_id)
    .def_property_readonly("player_name", &danet::Uid::get_player_name)
    .def("as_bytes", [](danet::Uid &self) {
      std::string payload{};
      payload.resize(sizeof(danet::Uid));
      memcpy(&payload[0], &self, sizeof(danet::Uid));
      return py::bytes(payload);
    });
  py::class_<danet::UnitId>(mpi, "UnitId").def_readonly("val", &danet::UnitId::val);
  py::class_<std::unordered_set<ecs::EntityId>>(m, "EntityIdSet")
    .def("__contains__", [](const std::unordered_set<int> &self, int val) { return self.count(val); })
    .def("__len__", [](const std::unordered_set<int> &self) { return self.size(); })
    .def(
      "__iter__", [](const std::unordered_set<int> &self) { return py::make_iterator(self.begin(), self.end()); },
      py::keep_alive<0, 1>());
  py::enum_<danet::AreaFlagsEnum>(mpi, "AreaFlagsEnum")
    .value("air", danet::AreaFlagsEnum::air)
    .value("ground", danet::AreaFlagsEnum::ground)
    .value("mapArea", danet::AreaFlagsEnum::mapArea)
    .value("team1", danet::AreaFlagsEnum::team1)
    .value("team2", danet::AreaFlagsEnum::team2)
    .value("killArea", danet::AreaFlagsEnum::killArea)
    .value("detectionArea", danet::AreaFlagsEnum::detectionArea)
    .value("airMapArea", danet::AreaFlagsEnum::airMapArea);

  py::class_<danet::WeaponMask>(mpi, "WeaponMask")
    .def_property_readonly("num_weapons", &danet::WeaponMask::get_num_weapons)
    .def_property_readonly("weapon_index", &danet::WeaponMask::get_weapon_index)
    .def_property_readonly("mask", [](const danet::WeaponMask &self) -> py::memoryview {
      return py::memoryview::from_memory(self.get_mask_c(), BITS_TO_BYTES(self.get_num_weapons()));
    });

  bind_readonly_vector<std::vector<danet::WeaponMask>>(mpi, "WeaponMaskVector");
  py::class_<danet::WeaponsMask>(mpi, "WeaponsMask").def_readonly("weapons", &danet::WeaponsMask::weapons);

  py::enum_<danet::Country> CountryEnum(mpi, "Country");
  CountryEnum.value("USA", danet::Country::USA)
    .value("GERMANY", danet::Country::GERMANY)
    .value("RUSSIA", danet::Country::RUSSIA)
    .value("BRITAIN", danet::Country::BRITAIN)
    .value("JAPAN", danet::Country::JAPAN)
    .value("CHINA", danet::Country::CHINA)
    .value("FRANCE", danet::Country::FRANCE)
    .value("ITALY", danet::Country::ITALY)
    .value("SWEDEN", danet::Country::SWEDEN)
    .value("ISRAEL", danet::Country::ISRAEL);
  CountryEnum
    .def("long_name",
         [](const danet::Country &self) {
           switch (self) {
             case danet::Country::USA: return "USA";
             case danet::Country::GERMANY: return "GERMANY";
             case danet::Country::RUSSIA: return "RUSSIA";
             case danet::Country::BRITAIN: return "BRITAIN";
             case danet::Country::JAPAN: return "JAPAN";
             case danet::Country::CHINA: return "CHINA";
             case danet::Country::FRANCE: return "FRANCE";
             case danet::Country::ITALY: return "ITALY";
             case danet::Country::SWEDEN: return "SWEDEN";
             case danet::Country::ISRAEL: return "ISRAEL";
             default: return "UNKNOWN";
           }
         })
    .def("short_name", [](const danet::Country &self) {
      switch (self) {
        case danet::Country::USA: return "US";
        case danet::Country::GERMANY: return "GR";
        case danet::Country::RUSSIA: return "RU";
        case danet::Country::BRITAIN: return "BR";
        case danet::Country::JAPAN: return "JP";
        case danet::Country::CHINA: return "CN";
        case danet::Country::FRANCE: return "FR";
        case danet::Country::ITALY: return "IT";
        case danet::Country::SWEDEN: return "SE";
        case danet::Country::ISRAEL: return "IL";
        default: return "UNKNOWN";
      }
    });

  py_codegen_types.include(m);
}
