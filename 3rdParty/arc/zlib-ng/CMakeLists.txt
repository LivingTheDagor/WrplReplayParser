FetchContent_Declare(
        ZLIB
        SOURCE_DIR ${BASE_DIR}/3rdParty/arc/zlib-ng-2.1.2
        OVERRIDE_FIND_PACKAGE
)

FetchContent_GetProperties(ZLIB)
if (NOT zlib_POPULATED)
    set(ZLIB_ALIASES ON)
    # zlib-ng specific options
    set(ZLIB_SYMBOL_PREFIX "dag_" CACHE STRING "" FORCE)
    set(ZLIB_COMPAT ON CACHE BOOL "" FORCE)  # makes it a drop-in zlib replacement
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(ZLIB_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(ZLIBNG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ZLIB_INSTALL ON CACHE BOOL "" FORCE)  # avoids the export issues we hit earlier
    FetchContent_MakeAvailable(ZLIB)
endif ()

if (NOT TARGET ZLIB::ZLIB)
    if (BUILD_SHARED_LIBS)
        add_library(ZLIB::ZLIB ALIAS zlib)
    else ()
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    endif ()
endif ()