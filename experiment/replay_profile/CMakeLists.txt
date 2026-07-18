
# add_definitions(-DTRACY_ENABLE)
add_executable(ProfileReplayParsing
        ProfileReplay.cpp
)

target_link_libraries(ProfileReplayParsing
        PUBLIC
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

target_include_directories(ProfileReplayParsing
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        "${BASE_DIR}/3rdParty/tracy/public"
        # "${BASE_DIR}/3rdParty"
)


attach_signal_tracer(ProfileReplayParsing)

# target_compile_definitions(ProfileReplayParsing PUBLIC TRACY_ENABLE)
# SetGenFlags(dagor_rpl_test)