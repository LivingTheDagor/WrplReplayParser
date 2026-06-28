
# Find the specified Python version, or use any found by default
if(PYTHON_VERSION)
    find_package(Python ${PYTHON_VERSION} REQUIRED COMPONENTS Interpreter Development)
else()
    find_package(Python 3.12...3.12.13 REQUIRED COMPONENTS Interpreter Development)
endif()

# Try to find pybind11 via find_package first
find_package(pybind11 QUIET)

# If not found, fetch pybind11 automatically
if(NOT pybind11_FOUND)
    message(STATUS "pybind11 not found, fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG        v2.11.1  # You can update the version/tag as needed
    )
    FetchContent_MakeAvailable(pybind11)
endif()


add_subdirectory(ecs)
add_subdirectory(math)
add_subdirectory(mpi)
add_subdirectory(DataBlock)
add_subdirectory(replay)
add_subdirectory(BitStream)
add_subdirectory(state)
add_subdirectory(main)
add_subdirectory(translate)