add_library(NET_UTILS
        netUtils.cpp
)


add_library(NET_UTILS::NET_UTILS ALIAS NET_UTILS)

target_link_libraries(NET_UTILS
        #mimalloc
        LOGGER::LOGGER
        DAG_MATH::DAG_MATH
)

set(INCLUDES

        "${BASE_DIR}/3rdParty/Dagor_Engine/include"
        # "${BASE_DIR}/src/3rdParty/fmt-12.1.0/include"
)

target_include_directories(NET_UTILS
        SYSTEM PUBLIC
        ${INCLUDES}
)

set_target_properties(NET_UTILS PROPERTIES
        COMPILE_OPTIONS --no-warnings
)