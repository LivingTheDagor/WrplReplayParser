add_executable(TestNewBlk
        main.cpp
)

target_link_libraries(TestNewBlk
        XXH3::XXH3
        EASTL::EASTL
)


set(INCLUDES
        "${BASE_DIR}/src/include"
        "${BASE_DIR}/3rdParty/eastl/include"
)

target_include_directories(TestNewBlk
        PUBLIC
        ${INCLUDES}
)
