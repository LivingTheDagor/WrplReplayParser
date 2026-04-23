add_library(pyECS
        EntityId.cpp
        Entity.cpp
        EntityManager.cpp
        ECSTypes.cpp
        ECSArrayTypes.cpp
        GState.cpp
        ECSQueryES.cpp
)

target_link_libraries(pyECS
        pybind11::module
        Python::Python
        dagECS::dagECS
        PyMath::PyMath
)

add_library(PyECS::PyECS ALIAS pyECS)


# Set include dirs for Python if needed
target_include_directories(pyECS PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
)

target_compile_options(pyECS PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)