add_library(FieldSerializer
        idFieldSerializer.cpp
)

target_link_libraries(FieldSerializer
        PUBLIC
        fmt::fmt
        LOGGER::LOGGER
        BS::BS
)

add_library(FieldSerializer:FieldSerializer ALIAS FieldSerializer)

set(INCLUDES
        "${BASE_DIR}/src/include"
        "${BASE_DIR}/replay/include"
)

target_include_directories(FieldSerializer
        PUBLIC
        ${INCLUDES}
)


SetGenFlags(FieldSerializer)