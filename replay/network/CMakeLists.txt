add_library(NET
        STATIC
        CNetwork.cpp
        eid.cpp
        Connection.cpp
        Object.cpp
        msgSinkES.gen.es.cpp
        netEventES.gen.es.cpp
        genericEventsES.gen.es.cpp
        ecsPull.cpp
        msgDispatch.cpp
        message.cpp
        schemelessEventSerialize.cpp
        # netUntargetedMessaging.cpp
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

# Print full compile command (when using Ninja generator)
get_target_property(dummy NET TYPE) # Forces evaluation