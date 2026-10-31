add_executable(test_bitstream
        test_bitstream.cpp
        test_dagor_bitstream.cpp
)
target_link_libraries(test_bitstream
        LOGGER::LOGGER
        Catch2::Catch2WithMain
        EASTL::EASTL
)

target_include_directories(test_bitstream
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
)

add_test(NAME test_bitstream COMMAND test_bitstream)

attach_signal_tracer(test_bitstream)