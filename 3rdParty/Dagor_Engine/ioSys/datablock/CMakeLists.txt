add_library(dagBlk
        STATIC
        blk_errors.cpp
        blk_core.cpp
        blk_parser.cpp
        blk_serialize.cpp
        blk_readBBF3.cpp
        roDataBlk.cpp
)


add_library(dagBlk::dagBlk ALIAS dagBlk)

target_link_libraries(dagBlk
        EASTL::EASTL
        LOGGER::LOGGER
        XXH3::XXH3
        ZSTD::ZSTD
        VROMFS::VROMFS
        dagIoSys::dagIoSys
)

set(INCLUDES

        "${BASE_DIR}/3rdParty/Dagor_Engine/include"
        # "${BASE_DIR}/src/3rdParty/fmt-12.1.0/include"
)

target_include_directories(dagBlk
        SYSTEM PUBLIC
        ${INCLUDES}
)

target_compile_options(dagBlk PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/w>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:--no-warnings>
)