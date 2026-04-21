add_library(BitStream
        STATIC
        BitStream.cpp
)
add_library(BS::BS ALIAS BitStream)

target_link_libraries(BitStream
        BLK::BLK
        READER::READER
)

set(INCLUDES
        "${BASE_DIR}/src/include"
        "${BASE_DIR}/3rdParty/Dagor_Engine/include"
)


target_include_directories(BitStream
    PUBLIC
        ${INCLUDES}
)
