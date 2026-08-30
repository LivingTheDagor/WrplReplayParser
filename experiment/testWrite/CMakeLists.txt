add_executable(dagor_rpl_write_test
        TestWritingReplay.cpp
)
target_link_libraries(dagor_rpl_write_test
        LOGGER::LOGGER
        fmt::fmt
        BS::BS
        dagBlk::dagBlk
        dagIoSys::dagIoSys
        FS::FS
        VROMFS::VROMFS
        Replay::Replay
        ZLIB::ZLIB
        MPI_UI_STUBS::MPI_UI_STUBS
        # mimalloc

)

target_include_directories(dagor_rpl_write_test
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

attach_signal_tracer(dagor_rpl_write_test)

# SetGenFlags(dagor_rpl_write_test)