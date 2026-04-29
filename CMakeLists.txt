cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 23)
project(WtFileUtils LANGUAGES C CXX)

if(MSVC)
    # Enable the CMAKE_MSVC_RUNTIME_LIBRARY variable
    cmake_policy(SET CMP0091 NEW)
    # Use static runtime library (/MT for release, /MTd for debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library")
endif()

include(cmake/functions.cmake)
include(cmake/CompilerWarnings.cmake)
set(BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
# set(CMAKE_VERBOSE_MAKEFILE ON)

# python is required for code generation
find_package(Python3 COMPONENTS Interpreter REQUIRED)


set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_definitions(_TARGET_64BIT)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(_TARGET_PC)
    # add_compile_definitions(Windows) // conflicts with imgui
    add_compile_definitions(_TARGET_PC_WIN)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(Linux)
    add_compile_definitions(_TARGET_PC_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_definitions(Mac)
endif ()

set(PYTHON_VERSION "" CACHE STRING "Python version to use (e.g. 3.11 or 3.8 etc.)")



if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR) # not a subproject, include tests
    option(ENABLE_DECOMPRESSION
            "Enables support for compressed debug sections if both libz/libzstd are present"
            FALSE)
    set(BUILD_SHARED_LIBS OFF)
    message(STATUS "WtFileUtils ran as base project")
    print_stats() # print CMAKE stats about compiler and such
    set_compiler_flags() # sets the flags I want, idk how to do flags laugh at me
    if(MINGW) # TODO: make this a flag for my IDE instead?
        # CMAKE_LINKER_TYPE(MOLD)
        set(CMAKE_C_USING_LINKER_LLD "-fuse-ld=mold")
    endif()
    set_data_folders_loc()
    add_subdirectory(debugging)
    add_subdirectory(ClogDecryptor)
    add_subdirectory(experiment)
    add_subdirectory(python)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/hidden")
        add_subdirectory(hidden)
    endif ()
else()
    message(STATUS "WtFileUtils ran as subproject")
endif()
add_subdirectory(3rdParty)
add_subdirectory(src)
add_subdirectory(replay)
add_subdirectory(replay_inspector)
