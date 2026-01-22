set(sources
        callbacks.c
        compress.c
        host.c
        list.c
        packet.c
        peer.c
        protocol.c
        enet_log.c
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(sources
            ${sources}
            win32.c
    )

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(sources
            ${sources}
            unix.c
    )
endif ()
add_library(ENET STATIC ${sources})
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(ENET PRIVATE
            ws2_32
            winmm
    )
endif ()

target_include_directories(ENET
        SYSTEM PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
)

add_library(ENET::ENET ALIAS ENET)

set_target_properties(ENET PROPERTIES
        COMPILE_OPTIONS --no-warnings
)