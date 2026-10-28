#include "modules/ecs/ECSTypes.h"
#include "ecs/EntityManager.h" // so all types are defined
#include "pybind11/stl_bind.h"
#include "ecs/componentTypesDefs.h"


// on mingw the array definitions adds to many section to the .cpp.obj file the compiler generations, so I had to move
// them to their own file
void PyECSTypes::include_array(py::module_ &m) {
  auto ecs = m.def_submodule("ecs");
  bind_readonly_vector<ecs::StringList>(ecs, "StringList");
  REGISTER_TYPE(ecs::StringList);
  bind_readonly_vector<ecs::EidList>(ecs, "EntityIdList");
  REGISTER_TYPE(ecs::EidList);
  bind_readonly_vector<ecs::UInt8List>(ecs, "UInt8List");
  REGISTER_TYPE(ecs::UInt8List);
  bind_readonly_vector<ecs::UInt16List>(ecs, "UInt16List");
  REGISTER_TYPE(ecs::UInt16List);
  bind_readonly_vector<ecs::UInt32List>(ecs, "UInt32List");
  REGISTER_TYPE(ecs::UInt32List);
  bind_readonly_vector<ecs::UInt64List>(ecs, "UInt64List");
  REGISTER_TYPE(ecs::UInt64List);
  bind_readonly_vector<ecs::FloatList>(ecs, "FloatList");
  REGISTER_TYPE(ecs::FloatList);
  bind_readonly_vector<ecs::Point2List>(ecs, "Point2List");
  REGISTER_TYPE(ecs::Point2List);
  bind_readonly_vector<ecs::Point3List>(ecs, "Point3List");
  REGISTER_TYPE(ecs::Point3List);
  bind_readonly_vector<ecs::Point4List>(ecs, "Point4List");
  REGISTER_TYPE(ecs::Point4List);
  bind_readonly_vector<ecs::IPoint2List>(ecs, "IPoint2List");
  REGISTER_TYPE(ecs::IPoint2List);
  bind_readonly_vector<ecs::IPoint3List>(ecs, "IPoint3List");
  REGISTER_TYPE(ecs::IPoint3List);
  bind_readonly_vector<ecs::IPoint4List>(ecs, "IPoint4List");
  REGISTER_TYPE(ecs::IPoint4List);
  bind_readonly_vector<ecs::BoolList>(ecs, "BoolList");
  REGISTER_TYPE(ecs::BoolList);
  bind_readonly_vector<ecs::TMatrixList>(ecs, "TMatrixList");
  REGISTER_TYPE(ecs::TMatrixList);
  bind_readonly_vector<ecs::ColorList>(ecs, "ColorList");
  REGISTER_TYPE(ecs::ColorList);
  bind_readonly_vector<ecs::Int8List>(ecs, "Int8List");
  REGISTER_TYPE(ecs::Int8List);
  bind_readonly_vector<ecs::Int16List>(ecs, "Int16List");
  REGISTER_TYPE(ecs::Int16List);
  bind_readonly_vector<ecs::IntList>(ecs, "IntList");
  REGISTER_TYPE(ecs::IntList);
  bind_readonly_vector<ecs::Int64List>(ecs, "Int64List");
  REGISTER_TYPE(ecs::Int64List);

}

