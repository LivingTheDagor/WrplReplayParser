add_library(READER
        Readers.cpp
        ZlibRead.cpp
)

target_link_libraries(READER
        ZSTD::ZSTD
        Zlib::Zlib
        fmt::fmt
        LOGGER::LOGGER
)
add_library(READER::READER ALIAS READER)

set(INCLUDES
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty/arc/zstd-1.4.5"
        # "${BASE_DIR}/3rdParty/arc"
        # "${BASE_DIR}/3rdParty/arc/zlib-ng"
)

target_include_directories(READER
        PUBLIC
        ${INCLUDES}
)

