add_library(PyBitStream BitStream.cpp)

target_link_libraries(PyBitStream;
        pybind11::module
        Python::Python
        DAG_MATH::DAG_MATH # gives you access to blk path
        LZ4::LZ4
        LOGGER::LOGGER
        dagBlk::dagBlk
)

add_library(PyBitStream::PyBitStream ALIAS PyBitStream)

target_include_directories(PyBitStream PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
        "${BASE_DIR}/src/include"
)