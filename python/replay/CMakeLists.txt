add_library(PyReplay replay.cpp)

target_link_libraries(PyReplay
        pybind11::module
        Python::Python
        Replay::Replay
        PyDataBlock::PyDataBlock
        PyBitStream::PyBitStream
)

add_library(PyReplay::PyReplay ALIAS PyReplay)

target_include_directories(PyReplay PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
)