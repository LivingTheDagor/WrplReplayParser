cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 23)
project(WrplReplayParser LANGUAGES C CXX)

# ok look I have to do this for whatever reason to make MSVC works, it generates alot of ugly cli warnings but I don't feel like fixing it
if (MSVC)
    # Enable the CMAKE_MSVC_RUNTIME_LIBRARY variable
    cmake_policy(SET CMP0091 NEW)
    # Use static runtime library (/MT for release, /MTd for debug)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library")
endif ()

include(cmake/functions.cmake)
include(cmake/CompilerWarnings.cmake)
set(BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
# set(CMAKE_VERBOSE_MAKEFILE ON)


add_compile_definitions(_TARGET_64BIT)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(_TARGET_PC)
    add_compile_definitions(_TARGET_PC_WIN)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(_TARGET_PC_LINUX)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_definitions(Mac)
endif ()


# my custom ovverides
if (EXISTS "${CMAKE_SOURCE_DIR}/cmake/overrides.cmake")
    include("${CMAKE_SOURCE_DIR}/cmake/overrides.cmake")
endif ()

if (POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif ()
set(pybind11_FINDPYTHON ON CACHE BOOL "" FORCE)
set(PYTHON_VERSION_TO_USE "3.12...3.12.13" CACHE STRING
        "Python version to use (e.g. 3.11, 3.12.13)")
set(PYBIND11_PYTHON_VERSION ${PYTHON_VERSION_TO_USE})

if (PYTHON_VERSION_TO_USE)
    set(Python_VERSION ${PYTHON_VERSION_TO_USE})
    set(PythonInterp_FIND_VERSION ${PYTHON_VERSION_TO_USE})
    set(PythonLibs_FIND_VERSION ${PYTHON_VERSION_TO_USE})
endif ()

option(PARSER_ENABLE_TESTS "include tests in library" OFF) # currently no tests, is a TODO
option(PARSER_ENABLE_PYTHON_BINDINGS "include python bindings in library" OFF)
option(PARSER_ENABLE_INSPECTOR "include replay inspector in library" OFF)
# curl and openSSL can be a PITA to set up, and its only used for downloading replays, so not really required
option(PARSER_ENABLE_INSPECTOR_CURL "include curl in replay inspector" OFF)
option(PARSER_INCLUDE_EXPERIMENTING_DIRS "include /debugging and /experiment which is where I do testing bullshit" OFF)
option(PARSER_MY_FLAGS "include my custom flags" OFF)
option(TRACY_ENABLE "Tracy Enabled" OFF)
# default debug level is 1 for debug builds, 0 for release builds
# it is set to these defaults unless if you set it before
# I named mine to be LDAG instead of DAGOR to add some difference
if (NOT DEFINED LDAG_DBGLEVEL_TO_SET)
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(LDAG_DBGLEVEL_TO_SET 1)
    else ()
        set(LDAG_DBGLEVEL_TO_SET 0)
    endif ()
endif ()
option(LDAG_DBGLEVEL_TO_SET "Set the LDAG_DBGLEVEL to a specific value" LDAG_DBGLEVEL_TO_SET)
message("LDAG_DBGLEVEL=${LDAG_DBGLEVEL_TO_SET}")
add_compile_definitions(LDAG_DBGLEVEL=${LDAG_DBGLEVEL_TO_SET})

# mingw threading lib has a much slower shared_mutex compared to what MSVC uses.
if (NOT DEFINED ECS_USE_WIN_MTX AND ${CMAKE_SYSTEM_NAME} STREQUAL "Windows" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(ECS_USE_WIN_MTX ON)
    add_compile_definitions(ECS_USE_WIN_MTX)
endif ()

set_default_flags()

if (PARSER_MY_FLAGS)
    if(LDAG_DBGLEVEL_TO_SET EQUAL 0)
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
        set(BUILD_SHARED_LIBS OFF)
    endif ()
    set_compiler_flags() # sets the flags I want, idk how to do flags laugh at me
endif ()
add_subdirectory(3rdParty)

if (PARSER_ENABLE_PYTHON_BINDINGS)
    add_subdirectory(python)
endif ()

if (PARSER_ENABLE_INSPECTOR)
    add_subdirectory(replay_inspector)
endif ()

if (PARSER_INCLUDE_EXPERIMENTING_DIRS)
    option(ENABLE_DECOMPRESSION
            "Enables support for compressed debug sections if both libz/libzstd are present"
            FALSE)
    print_stats() # print CMAKE stats about compiler and such

    set_data_folders_loc()
    add_subdirectory(debugging)
    add_subdirectory(ClogDecryptor)
    add_subdirectory(experiment)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/hidden")
        add_subdirectory(hidden)
    endif ()
endif()
add_subdirectory(src)
add_subdirectory(replay)