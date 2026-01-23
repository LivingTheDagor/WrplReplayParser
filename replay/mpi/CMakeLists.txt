add_library(MPI OBJECT
        mpi.cpp
        reflection.cpp
        ObjectDispatcher.cpp
        serializers.cpp
        codegen/serializers.cpp
)


target_link_libraries(MPI
        FieldSerializer:FieldSerializer
        BLK::BLK
        FS::FS
        VROMFS::VROMFS
        BLK::BLK
        # REGEX::REGEX
        XXH3
        NET::NET
        ZSTD::ZSTD
        DAG_MATH::DAG_MATH
)
add_library(MPI::MPI ALIAS MPI)

set(INCLUDES
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
)

target_include_directories(MPI
        PUBLIC
        ${INCLUDES}
)


SetGenFlags(MPI)
add_dependencies(MPI CodeGeneration)