add_executable(create_rpl_packets
        code.cpp
)

target_link_libraries(create_rpl_packets
        BS::BS
        BLK::BLK
        READER::READER
        FS::FS
        VROMFS::VROMFS
        FieldSerializer:FieldSerializer
        NET::NET
        dagECS::dagECS
        INIT::INIT
)

target_include_directories(create_rpl_packets
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

