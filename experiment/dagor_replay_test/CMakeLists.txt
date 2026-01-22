add_executable(dagor_rpl_test
        TestLoadingReplay.cpp
        Replay.cpp
)
target_link_libraries(dagor_rpl_test
        LOGGER::LOGGER
        fmt::fmt
        BS::BS
        BLK::BLK
        READER::READER
        FS::FS
        VROMFS::VROMFS
        FieldSerializer:FieldSerializer
        NET::NET
        dagECS::dagECS
        INIT::INIT
        Replay::Replay

)

target_include_directories(dagor_rpl_test
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

# SetGenFlags(dagor_rpl_test)