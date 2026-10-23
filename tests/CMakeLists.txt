add_executable(lux_review_regression lux_review_regression.cpp)
target_link_libraries(lux_review_regression
        LOGGER::LOGGER fmt::fmt BS::BS dagBlk::dagBlk dagIoSys::dagIoSys FS::FS VROMFS::VROMFS
        FieldSerializer::FieldSerializer NET::NET dagECS::dagECS INIT::INIT Replay::Replay
        ZLIB::ZLIB ParserState::ParserState Translate::Translate)
target_include_directories(lux_review_regression PRIVATE "${BASE_DIR}/replay/include" "${BASE_DIR}/src/include")
add_test(NAME lux_review_regression COMMAND lux_review_regression)
set_tests_properties(lux_review_regression PROPERTIES SKIP_RETURN_CODE 77)
