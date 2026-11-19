
# Reduce unnecessary Daslang builds.
set(DAS_TESTS_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_AOT_EXAMPLES_DISABLED ON CACHE BOOL "" FORCE)

add_subdirectory(
        ${BASE_DIR}/3rdParty/daScript
        ${CMAKE_BINARY_DIR}/daslang-build
)


add_executable(RepackReplay
        main.cpp
)
target_link_libraries(RepackReplay
        LOGGER::LOGGER
        fmt::fmt
        BS::BS
        dagBlk::dagBlk
        dagIoSys::dagIoSys
        FS::FS
        Replay::Replay
        libDaScript
        # mimalloc

)

target_include_directories(RepackReplay
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        "${BASE_DIR}/3rdParty/daScript/include"
        "${CMAKE_BINARY_DIR}/daslang-build/include"
        # "${BASE_DIR}/3rdParty"
)

attach_signal_tracer(RepackReplay)