add_library(PyMath Math.cpp)

target_link_libraries(PyMath
        pybind11::module
        Python::Python
        DAG_MATH::DAG_MATH
)

add_library(PyMath::PyMath ALIAS PyMath)

target_include_directories(PyMath PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
)