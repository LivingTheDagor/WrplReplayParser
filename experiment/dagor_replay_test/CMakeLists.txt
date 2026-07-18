add_executable(dagor_rpl_test
        TestLoadingReplay.cpp
)
target_link_libraries(dagor_rpl_test
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
        ZLIB::ZLIB
        ParserState::ParserState
        Translate::Translate
        MPI_UI_STUBS::MPI_UI_STUBS
        # mimalloc

)

target_include_directories(dagor_rpl_test
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

attach_signal_tracer(dagor_rpl_test)

# SetGenFlags(dagor_rpl_test)