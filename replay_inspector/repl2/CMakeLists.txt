
add_executable(ReplayInspector
        viewer.cpp
)

if (PARSER_ENABLE_INSPECTOR_CURL)


    target_compile_definitions(ReplayInspector PUBLIC PARSER_ENABLE_INSPECTOR_CURL=1)

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
        set(_old_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

        find_package(OpenSSL REQUIRED)
        find_library(LZMA_STATIC NAMES lzma liblzma REQUIRED)

        set(CMAKE_FIND_LIBRARY_SUFFIXES ${_old_suffixes})
    endif ()

    # set(ZLIB_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}")
    # set(ZLIB_LIBRARY ZLIB::ZLIB)
    set(CURL_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(CURL_NO_CURL_CMAKE ON)
    set(BUILD_STATIC_CURL ON CACHE BOOL "" FORCE)
    set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
    set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_MISC_DOCS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)

    # Optional: trim features you may not need
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
    set(CURL_DISABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
    set(USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
    set(CURL_BROTLI OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
            curl
            GIT_REPOSITORY https://github.com/curl/curl.git
            GIT_TAG curl-8_19_0
    )

    FetchContent_MakeAvailable(curl)

    target_link_libraries(ReplayInspector PUBLIC CURL::libcurl)
endif ()

target_link_libraries(ReplayInspector PUBLIC
        SDL2::SDL2main
        SDL2::SDL2-static
        INIT::INIT
        imgui
        ZLIB::ZLIB
        LOGGER::LOGGER
        fmt::fmt
        BS::BS
        dagBlk::dagBlk
        dagIoSys::dagIoSys
        FS::FS
        VROMFS::VROMFS
        FieldSerializer::FieldSerializer
        NET::NET
        dagECS::dagECS
        Replay::Replay
        NET_UTILS::NET_UTILS
        ctre::ctre
        TFD::TFD
        ParserState::ParserState
        InspectorECS::InspectorECS
)

target_include_directories(ReplayInspector
        PUBLIC
        "${BASE_DIR}/replay_inspector/include"
        "${BASE_DIR}/3rdParty/arc/zlib-ng"
)

# get_target_property(target_includes ReplayInspector INTERFACE_INCLUDE_DIRECTORIES)
# mes;sage(STATUS "Interface include dirs: ${target_includes}")

# get_target_property(target_includes ReplayInspector INCLUDE_DIRECTORIES)
# message(STATUS "Include dirs: ${target_includes}")
# if(WIN32)
# Copy SDL2.dll to the output directory after build
#     add_custom_command(TARGET ReplayInspector POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_FILE:SDL2>
#             $<TARGET_FILE_DIR:ReplayInspector>
#     )
# endif ()