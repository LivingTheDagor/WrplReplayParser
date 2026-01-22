add_library(BitStream
        BitStream.cpp
)
add_library(BS::BS ALIAS BitStream)

target_link_libraries(BitStream)

set(INCLUDES
        "${BASE_DIR}/src/include"
        "${BASE_DIR}/3rdParty/Dagor_Engine/include/danet"
)


target_include_directories(BitStream
    PUBLIC
        ${INCLUDES}
)
