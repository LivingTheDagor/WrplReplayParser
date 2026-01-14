add_library(dag_math
        math3d.cpp
        perlin.cpp
        mathAng.cpp
)


add_library(DAG_MATH::DAG_MATH ALIAS dag_math)

target_link_libraries(dag_math
        #mimalloc
        EASTL::EASTL
        LOGGER::LOGGER
)

set(INCLUDES
        "${BASE_DIR}/src/include"
        "${BASE_DIR}/src/3rdParty/fmt-12.1.0/include"
)

target_include_directories(dag_math
        PUBLIC
        ${INCLUDES}
)

