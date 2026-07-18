add_executable(test_multiple_replays
        test_multiple_replays.cpp
)
target_link_libraries(test_multiple_replays
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
        INIT::INIT
        Replay::Replay
        MPI_UI_STUBS::MPI_UI_STUBS

)

target_include_directories(test_multiple_replays
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

# SetGenFlags(dagor_rpl_test)