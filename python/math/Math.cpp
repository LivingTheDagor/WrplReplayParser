#include "modules/Math.h"
#include "math/dag_Point2.h"
#include "math/dag_Point3.h"
#include "math/dag_Point4.h"
#include "math/integer/dag_IPoint2.h"
#include "math/integer/dag_IPoint3.h"
#include "math/integer/dag_IPoint4.h"
#include "math/dag_TMatrix.h"
#include "math/dag_e3dColor.h"
#include <pybind11/operators.h>

PyMath py_math;

void PyMath::include(py::module_ &m) {
  DO_INCLUDE()
  auto math = m.def_submodule("math");

  py::class_<Point2>(math, "Point2")
      .def(py::init<>())
      .def(py::init<real, real>(),
           py::arg("x"), py::arg("y"))
      .def_readonly("x", &Point2::x)
      .def_readonly("y", &Point2::y)
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self) // dot product
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("__neg__", [](const Point2& p) { return -p; })
      .def("__pos__", [](const Point2& p) { return +p; })
      .def("length", &Point2::length)
      .def("lengthSq", &Point2::lengthSq)
      .def("to_tuple", [](Point2 &p3){
        return py::make_tuple(p3.x, p3.y);
      })
      .def("__getitem__", [](Point2 &p2, int index){
        if(0 > index || index >= 2) {
          throw py::index_error();
        }
        return p2[index];
      });
  py::class_<Point3>(math, "Point3")
      .def(py::init<>())
      .def(py::init<real, real, real>(),
           py::arg("x"), py::arg("y"), py::arg("z"))
      .def_readonly("x", &Point3::x)
      .def_readonly("y", &Point3::y)
      .def_readonly("z", &Point3::z)
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self) // dot product
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("__neg__", [](const Point3& p) { return -p; })
      .def("__pos__", [](const Point3& p) { return +p; })
      .def("length", &Point3::length)
      .def("lengthSq", &Point3::lengthSq)
      .def("to_tuple", [](Point3 &p3){
        return py::make_tuple(p3.x, p3.y, p3.z);
      })
      .def("__getitem__", [](Point3 &p3, int index){
        if(0 > index || index >= 3) {
          throw py::index_error();
        }
        return p3[index];
      });
  py::class_<Point4>(math, "Point4")
      .def(py::init<>())
      .def(py::init<real, real, real, real>(),
           py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
      .def_readonly("x", &Point4::x)
      .def_readonly("y", &Point4::y)
      .def_readonly("z", &Point4::z)
      .def_readonly("w", &Point4::w)
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self) // dot product
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("__neg__", [](const Point4& p) { return -p; })
      .def("__pos__", [](const Point4& p) { return +p; })
      .def("length", &Point4::length)
      .def("lengthSq", &Point4::lengthSq)
      .def("to_tuple", [](Point4 &p4){
        return py::make_tuple(p4.x, p4.y, p4.z, p4.w);
      })
      .def("__getitem__", [](Point4 &p4, int index){
        if(0 > index || index >= 4) {
          throw py::index_error();
        }
        return p4[index];
      });
  py::class_<IPoint2>(math, "IPoint2")
      .def(py::init<>())
      .def(py::init<int, int>(),
           py::arg("x"), py::arg("y"))
      .def_readonly("x", &IPoint2::x)
      .def_readonly("y", &IPoint2::y)
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self) // dot product
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("__neg__", [](const IPoint2& p) { return -p; })
      .def("__pos__", [](const IPoint2& p) { return +p; })
      .def("length", &IPoint2::length)
      .def("lengthSq", &IPoint2::lengthSq)
      .def("to_tuple", [](IPoint2 &p){
        return py::make_tuple(p.x, p.y);
      })
      .def("__getitem__", [](IPoint2 &p2, int index){
        if(0 > index || index >= 2) {
          throw py::index_error();
        }
        return p2[index];
      });
  py::class_<IPoint3>(math, "IPoint3")
      .def(py::init<>())
      .def(py::init<int, int, int>(),
           py::arg("x"), py::arg("y"), py::arg("z"))
      .def_readonly("x", &IPoint3::x)
      .def_readonly("y", &IPoint3::y)
      .def_readonly("z", &IPoint3::z)
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self) // dot product
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("__neg__", [](const IPoint3& p) { return -p; })
      .def("__pos__", [](const IPoint3& p) { return +p; })
      .def("length", &IPoint3::length)
      .def("lengthSq", &IPoint3::lengthSq)
      .def("to_tuple", [](IPoint3 &p3){
        return py::make_tuple(p3.x, p3.y, p3.z);
      })
      .def("__getitem__", [](IPoint3 &p3, int index){
        if(0 > index || index >= 3) {
          throw py::index_error();
        }
        return p3[index];
      });
  py::class_<IPoint4>(math, "IPoint4")
      .def(py::init<>())
      .def(py::init<int, int, int, int>(),
           py::arg("x"), py::arg("y"), py::arg("z"), py::arg("w"))
      .def_readonly("x", &IPoint4::x)
      .def_readonly("y", &IPoint4::y)
      .def_readonly("z", &IPoint4::z)
      .def_readonly("w", &IPoint4::w)
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self) // dot product
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("__neg__", [](const IPoint4& p) { return -p; })
      .def("__pos__", [](const IPoint4& p) { return +p; })
      .def("length", &IPoint4::length)
      .def("lengthSq", &IPoint4::lengthSq)
      .def("to_tuple", [](IPoint4 &p4){
        return py::make_tuple(p4.x, p4.y, p4.z, p4.w);
      })
      .def("__getitem__", [](IPoint4 &p4, int index){
        if(0 > index || index >= 4) {
          throw py::index_error();
        }
        return p4[index];
      });
  py::class_<TMatrix>(math, "TMatrix")
      .def(py::init<>())
      .def(py::init<real>())
      .def_readonly("col", &TMatrix::col)
      .def_readonly("array", &TMatrix::array)
      .def(py::self + py::self)
      .def(py::self - py::self)
      .def(py::self * py::self) // dot product
      .def(py::self == py::self)
      .def(py::self != py::self)
      .def("to_tuple", [](TMatrix &m){
        return py::make_tuple(m.col[0], m.col[1], m.col[2], m.col[3]);
      })
      .def("__getitem__", [](TMatrix &m, int index){
        if(0 > index || index >= 4) {
          throw py::index_error();
        }
        return m.col[index];
      });
  py::class_<E3DCOLOR>(math, "E3DCOLOR")
      .def(py::init<>())
      .def(py::init<uint8_t, uint8_t, uint8_t, uint8_t>(), py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a")=255)
      .def_readonly("r", &E3DCOLOR::r)
      .def_readonly("g", &E3DCOLOR::g)
      .def_readonly("b", &E3DCOLOR::b)
      .def_readonly("a", &E3DCOLOR::a);
}