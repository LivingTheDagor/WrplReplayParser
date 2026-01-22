add_library(FS
        FSObject.cpp
        Directory.cpp
        File.cpp
        FileManager.cpp
)


target_link_libraries(FS
        ZSTD::ZSTD
        Zlib::Zlib
        DAG_MATH::DAG_MATH
        BLK::BLK
)

add_library(FS::FS ALIAS FS)

set(INCLUDES
        "${BASE_DIR}/src/include"
)

target_include_directories(FS
        PUBLIC
        ${INCLUDES}
)


SetGenFlags(FS)