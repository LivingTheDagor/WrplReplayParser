add_library(PyMpi
        codegen_objects.cpp
        mpi.cpp
        types.cpp
        codegen_types.cpp
        codegen_types_0.cpp
        codegen_types_1.cpp
        codegen_types_2.cpp
        codegen_types_3.cpp
        codegen_types_4.cpp
        codegen_types_5.cpp
        codegen_types_6.cpp
        codegen_types_7.cpp
        codegen_types_8.cpp
        battle_messages.cpp
        unit.cpp
)

target_link_libraries(PyMpi
        pybind11::module
        Python::Python
        MPI::MPI
        PyECS::PyECS
)

add_library(PyMpi::PyMpi ALIAS PyMpi)

target_include_directories(PyMpi PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
)