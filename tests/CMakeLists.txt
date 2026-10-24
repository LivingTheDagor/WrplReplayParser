add_executable(packet_regression_tests packet_regression_tests.cpp)
target_link_libraries(packet_regression_tests
        LOGGER::LOGGER fmt::fmt BS::BS dagBlk::dagBlk dagIoSys::dagIoSys FS::FS VROMFS::VROMFS
        FieldSerializer::FieldSerializer NET::NET dagECS::dagECS INIT::INIT Replay::Replay
        ZLIB::ZLIB ParserState::ParserState Translate::Translate)
target_include_directories(packet_regression_tests PRIVATE "${BASE_DIR}/replay/include" "${BASE_DIR}/src/include")
add_test(NAME packet_regression_tests COMMAND packet_regression_tests)
set_tests_properties(packet_regression_tests PROPERTIES SKIP_RETURN_CODE 77)

add_executable(bitstream_tests bitstream_tests.cpp)
target_link_libraries(bitstream_tests LOGGER::LOGGER fmt::fmt BS::BS FieldSerializer::FieldSerializer)
target_include_directories(bitstream_tests PRIVATE "${BASE_DIR}/replay/include" "${BASE_DIR}/src/include")
add_test(NAME bitstream_tests COMMAND bitstream_tests)

add_executable(logger_tests logger_tests.cpp)
target_link_libraries(logger_tests LOGGER::LOGGER fmt::fmt)
target_include_directories(logger_tests PRIVATE "${BASE_DIR}/src/include")
add_test(NAME logger_tests COMMAND logger_tests)
