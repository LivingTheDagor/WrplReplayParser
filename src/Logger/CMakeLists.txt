
add_library(LOGGER STATIC logger.cpp)

set(INCLUDES
        "${BASE_DIR}/src/include"
        "${BASE_DIR}/3rdParty/Dagor_Engine/include"
)

target_include_directories(LOGGER
        PUBLIC
        ${INCLUDES}
)

target_link_libraries(LOGGER PUBLIC
        fmt::fmt
        # cpptrace::cpptrace
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(LOGGER PUBLIC /Zc:preprocessor) # some of the logging preprossor logic under MSVC needs this
endif ()
add_library(LOGGER::LOGGER ALIAS LOGGER)


SetGenFlags(LOGGER)