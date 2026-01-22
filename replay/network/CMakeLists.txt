add_library(NET
        CNetwork.cpp
        eid.cpp
        Connection.cpp
        Object.cpp
)

target_link_libraries(NET
        ZSTD
        LZ4::LZ4
        dagECS::dagECS
        ENET::ENET
)
add_library(NET::NET ALIAS NET)

set(INCLUDES
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty/arc/lz4"
        # "${BASE_DIR}/3rdParty/arc/zstd-1.4.5"
        # "${BASE_DIR}/3rdParty/arc"
        # "${BASE_DIR}/3rdParty/arc/lz4"
        # "${BASE_DIR}/3rdParty/arc/zlib-ng"
        "${BASE_DIR}/replay/include"
)

target_include_directories(NET
        PUBLIC
        ${INCLUDES}
)


SetGenFlags(NET)