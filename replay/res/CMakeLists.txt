add_library(resMgr STATIC
        grpLoader.cpp
)


target_link_libraries(resMgr
        dagBlk::dagBlk
        DAG_MATH::DAG_MATH
        dagIoSys::dagIoSys
        FS::FS
        VROMFS::VROMFS
        fmt::fmt
        LOGGER::LOGGER
)
add_library(resMgr::resMgr ALIAS resMgr)

set(INCLUDES
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
)

target_include_directories(resMgr
        PUBLIC
        ${INCLUDES}
)


SetGenFlags(resMgr)