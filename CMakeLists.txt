cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 23)
project(WtFileUtils LANGUAGES C CXX)

include(cmake/functions.cmake)
set(BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_definitions(_TARGET_64BIT)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(_TARGET_PC)
    add_compile_definitions(Windows)
    add_compile_definitions(_TARGET_PC_WIN)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(Linux)
    add_compile_definitions(_TARGET_PC_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_definitions(Mac)
endif ()

add_subdirectory(3rdParty)
add_subdirectory(src)
add_subdirectory(replay)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR) # not a subproject, include tests
    message(STATUS "WtFileUtils ran as base project")
    print_stats() # print CMAKE stats about compiler and such
    set_compiler_flags() # sets the flags I want, idk how to do flags laugh at me
    set_data_folders_loc()
    add_subdirectory(debugging)
    add_subdirectory(ClogDecryptor)
    add_subdirectory(experiment)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/hidden")
        add_subdirectory(hidden)
    endif ()
else()
    message(STATUS "WtFileUtils ran as subproject")
endif()