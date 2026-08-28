#include "modules/mpi/types.h"
#include "modules/mpi/codegen_types.h"
#include "modules/DataBlock/DataBlock.h"
#include "modules/ecs/EntityId.h"
#include "mpi/types.h"
#include "modules/bind_readonly_vector.h"
#include "modules/bind_rewind_state.h"
#include "Unit.h"
#include "modules/utf8_err_ignore_string.h"

PyMpiTypes py_mpi_types;

void PyMpiTypes::include(py::module_ &m) {
  DO_INCLUDE()
  py_data_block.include(m);
  py_entity_id.include(m);

  auto mpi = m.def_submodule("mpi");
  py::class_<danet::Uid>(mpi, "Uid")
    .def_readonly("player_id", &danet::Uid::account_id)
    .def_property_readonly("player_name", [](const danet::Uid &self) { return str_to_py_str(self.get_player_name()); })
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

  py::class_<danet::CameraData>(mpi, "CameraData")
    .def_readonly("camera_euler", &danet::CameraData::camera_euler)
    .def_readonly("gun_pointer", &danet::CameraData::gun_pointer);


  py::class_<ObjectRewindState<danet::CameraData, false, false, false>::TimeState>(mpi, "CameradataTS")
  .def_readonly("time_ms", &ObjectRewindState<danet::CameraData, false, false, false>::TimeState::time_ms)
  .def_readonly("value", &ObjectRewindState<danet::CameraData, false, false, false>::TimeState::data);

  bind_readonly_vector<dag::Vector<ObjectRewindState<danet::CameraData, false, false, false>::TimeState>>(mpi, "CameraDataTSList");


  bind_rewind_state<ObjectRewindState<danet::CameraData, false, false, false>>(mpi, "CameraDataHistory")
    .def("channels", [](py::object self_py) {
      auto &self = self_py.cast<ObjectRewindState<danet::CameraData, false, false, false> &>();
      auto &v = self.history();
      py::dict d;
      if (v.empty())
        return d;
      const auto *b = v.data();
      const size_t n = v.size();
      d["time_ms"] = channel_view(b, &b->time_ms, n, self_py);
      d["cam_yaw"] = channel_view(b, &b->data.camera_euler.y, n, self_py);
      d["cam_pitch"] = channel_view(b, &b->data.camera_euler.z, n, self_py);
      d["cam_roll"] = channel_view(b, &b->data.camera_euler.x, n, self_py);
      d["aim_yaw"] = channel_view(b, &b->data.gun_pointer.x, n, self_py);
      d["aim_pitch"] = channel_view(b, &b->data.gun_pointer.y, n, self_py);
      return d;
    },
         "All samples as read-only zero-copy numpy views over the history buffer.\n"
         "Empty dict when the history is empty, otherwise every array has the same length.\n"
         "Keys: time_ms (uint32, ms); then float32 radians:\n"
         "cam_yaw, cam_pitch, cam_roll - where the camera looks;\n"
         "aim_yaw, aim_pitch - the aiming reticle, where the weapons are told to converge.\n"
         "The reticle has no roll, and both groups share this one timeline: a single sample\n"
         "carries the camera and the reticle at once. cam_roll comes through asin and is\n"
         "therefore bounded to +-pi/2, the rest span +-pi.");


  py_codegen_types.include(m);
}
