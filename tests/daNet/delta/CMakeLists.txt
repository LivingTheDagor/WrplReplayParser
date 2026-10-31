add_executable(test_rle
        test_rle.cpp
)
target_link_libraries(test_rle
        LOGGER::LOGGER
        Catch2::Catch2WithMain
        EASTL::EASTL
        DELTA::DELTA
)

target_include_directories(test_rle
        PUBLIC
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
)

add_test(NAME test_rle COMMAND test_rle)

attach_signal_tracer(test_rle)