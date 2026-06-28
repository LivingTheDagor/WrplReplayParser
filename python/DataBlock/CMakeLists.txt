add_library(PyDataBlock DataBlock.cpp)

target_link_libraries(PyDataBlock;
        pybind11::module
        Python::Python
        dagBlk::dagBlk
)

add_library(PyDataBlock::PyDataBlock ALIAS PyDataBlock)

target_include_directories(PyDataBlock PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
        "${BASE_DIR}/src/include"
)