add_library(PyState State.cpp)

target_link_libraries(PyState
        pybind11::module
        Python::Python
        INIT::INIT
        PyMpi::PyMpi
        PyReplay::PyReplay
)

add_library(PyState::PyState ALIAS PyState)


target_include_directories(PyState PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
)