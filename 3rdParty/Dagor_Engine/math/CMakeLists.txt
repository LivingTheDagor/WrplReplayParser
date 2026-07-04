add_library(dag_math
        STATIC
        math3d.cpp
        perlin.cpp
        mathAng.cpp
        alloc.cpp
)


add_library(DAG_MATH::DAG_MATH ALIAS dag_math)

target_link_libraries(dag_math
        #mimalloc
        EASTL::EASTL
        LOGGER::LOGGER
)

set(INCLUDES

        "${BASE_DIR}/3rdParty/Dagor_Engine/include"
        # "${BASE_DIR}/src/3rdParty/fmt-12.1.0/include"
)

target_include_directories(dag_math
        SYSTEM PUBLIC
        ${INCLUDES}
)

target_compile_options(dag_math PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/w>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:--no-warnings>
)