find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)

# Build the module
Python3_add_library(WtFileUtils MODULE main.cpp)

# Include BitStream header-only path
target_include_directories(WtFileUtils
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

# Strip "lib" prefix from the resulting file (important for Python modules)
set_target_properties(WtFileUtils PROPERTIES
        PREFIX ""
)

set_target_properties(WtFileUtils PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Ensure position-independent code is generated (needed for shared libs)
set_target_properties(WtFileUtils PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

# On Windows, make sure the output file ends with `.pyd`
if (WIN32)
    set_target_properties(WtFileUtils PROPERTIES SUFFIX ".pyd")
endif ()
# target_link_options(WtFileUtils PRIVATE
#         -static-libgcc
#         -static-libstdc++
#         -static
# )
