add_executable(test_net_unpack
        testNet.cpp
)

target_link_libraries(test_net_unpack
        BS::BS
        BLK::BLK
        READER::READER
        FS::FS
        VROMFS::VROMFS
        FieldSerializer:FieldSerializer
        NET::NET
        dagECS::dagECS
        INIT::INIT
        MPI::MPI
        NET_UTILS::NET_UTILS
)

target_include_directories(test_net_unpack
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

