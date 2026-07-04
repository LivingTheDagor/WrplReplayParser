include(FetchContent)


option(USE_SYSTEM_TBB "Prefer system/package-manager TBB via find_package" ON)

if (USE_SYSTEM_TBB)
    find_package(TBB CONFIG QUIET)
endif ()

if (NOT TBB_FOUND)
    message(STATUS "TBB not found via find_package; fetching oneTBB from source...")
    option(TBB_TEST "Enable testing" OFF)
    FetchContent_Declare(
            onetbb
            SOURCE_DIR ${BASE_DIR}/3rdParty/oneTBBv202300
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(onetbb)
endif ()

add_library(TBB INTERFACE)
add_library(TBB::TBB ALIAS TBB)

target_link_libraries(TBB
        INTERFACE
)
target_link_libraries(TBB INTERFACE TBB::tbb TBB::tbbmalloc)