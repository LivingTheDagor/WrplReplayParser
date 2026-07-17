add_library(MPI STATIC
        mpi.cpp
        reflection.cpp
        ObjectDispatcher.cpp
        serializers.cpp
        codegen/serializers.cpp
        Replication.cpp
        BattleMessages.cpp
        PositionSync.cpp
)


target_link_libraries(MPI
        FieldSerializer::FieldSerializer
        FS::FS
        VROMFS::VROMFS
        dagBlk::dagBlk
        # REGEX::REGEX
        XXH3
        NET::NET
        ZSTD::ZSTD
        DAG_MATH::DAG_MATH
        NET_UTILS::NET_UTILS
        DELTA::DELTA
        Boost::describe
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

add_library(MPI_UI_STUBS STATIC
    codegen/mpiUiStubs.cpp
)

target_link_libraries(MPI_UI_STUBS
        MPI::MPI
        dagBlk::dagBlk
        DAG_MATH::DAG_MATH
)

target_include_directories(MPI_UI_STUBS
        PUBLIC
        ${INCLUDES}
)

add_library(MPI_UI_STUBS::MPI_UI_STUBS ALIAS MPI_UI_STUBS)