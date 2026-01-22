add_library(Replay
        ReplayReader.cpp
)

target_link_libraries(Replay
        ZSTD
        LZ4::LZ4
        dagECS::dagECS
)
add_library(Replay::Replay ALIAS Replay)

set(INCLUDES
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty/arc/lz4"
        # "${BASE_DIR}/3rdParty/arc/zstd-1.4.5"
        # "${BASE_DIR}/3rdParty/arc"
        # "${BASE_DIR}/3rdParty/arc/lz4"
        # "${BASE_DIR}/3rdParty/arc/zlib-ng"
        "${BASE_DIR}/replay/include"
)

target_include_directories(Replay
        PUBLIC
        ${INCLUDES}
)


SetGenFlags(Replay)

