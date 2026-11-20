add_executable(test_replay
        test_replay_loading.cpp
)
target_link_libraries(test_replay
        LOGGER::LOGGER
        Catch2::Catch2WithMain
        EASTL::EASTL
        Replay::Replay
)

target_include_directories(test_replay
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
)

add_test(NAME test_replay COMMAND test_replay)

attach_signal_tracer(test_replay)