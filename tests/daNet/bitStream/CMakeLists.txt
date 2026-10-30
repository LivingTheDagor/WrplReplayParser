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


add_executable(test_dagor_bitstream
        test_dagor_bitstream.cpp
)
target_link_libraries(test_dagor_bitstream
        LOGGER::LOGGER
        Catch2::Catch2WithMain
        EASTL::EASTL
)

add_test(NAME test_dagor_bitstream COMMAND test_dagor_bitstream)

attach_signal_tracer(test_dagor_bitstream)
# SetGenFlags(dagor_rpl_test)