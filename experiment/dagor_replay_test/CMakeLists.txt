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
        ParserState::ParserState
        Translate::Translate
        # mimalloc

)

target_include_directories(dagor_rpl_test
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

attach_signal_tracer(dagor_rpl_test)
if (EMSCRIPTEN)
    target_link_options(dagor_rpl_test PRIVATE
            "SHELL:-sEXIT_RUNTIME=1"
            # "SHELL:-sASSERTIONS=2"
            # "SHELL:-sSAFE_HEAP=1"
            # "SHELL:-gsource-map"
            "SHELL:-sFORCE_FILESYSTEM=1"
            "SHELL:-sNODERAWFS=1"
            "SHELL:-sSTACK_SIZE=8388608"
            "SHELL:-sALLOW_MEMORY_GROWTH=1"
            "SHELL:-sINITIAL_MEMORY=268435456"
    )
endif ()
# SetGenFlags(dagor_rpl_test)