add_executable(RepackServerToClientReplay
        main.cpp
)
target_link_libraries(RepackServerToClientReplay
        LOGGER::LOGGER
        fmt::fmt
        BS::BS
        dagBlk::dagBlk
        dagIoSys::dagIoSys
        FS::FS
        NET::NET
        Replay::Replay
        # mimalloc

)

target_include_directories(RepackServerToClientReplay
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

attach_signal_tracer(RepackServerToClientReplay)