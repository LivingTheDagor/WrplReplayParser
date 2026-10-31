add_library(DOCTEST STATIC
        doctest.cpp
)


add_library(DOCTEST::DOCTEST ALIAS DOCTEST)


target_include_directories(DOCTEST
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/doctest
)


SetGenFlags(DOCTEST)
