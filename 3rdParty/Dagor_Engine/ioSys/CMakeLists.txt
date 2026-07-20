
add_subdirectory(datablock)

add_library(dagIoSys
        STATIC

        baseIo.cpp
        fileIo.cpp
        memIo.cpp
        zlibIo.cpp
        zlibIoFatal.cpp
        zstdIo.cpp
        zstdIoFatal.cpp
        chainedMemIo.cpp
)


target_link_libraries(dagIoSys
        PRIVATE
        ZLIB::ZLIB
        ZSTD::ZSTD
        PUBLIC
        EASTL::EASTL
        LOGGER::LOGGER
        FS::FS
)
add_library(dagIoSys::dagIoSys ALIAS dagIoSys)

target_include_directories(dagIoSys
        PUBLIC
        "${BASE_DIR}/3rdParty/Dagor_Engine/include"
        "${BASE_DIR}/src/include"
)


SetGenFlags(dagIoSys)