add_library(DECRYPT
        ClogDecryptor.cpp
)


add_library(DECRYPT::DECRYPT ALIAS DECRYPT)

target_link_libraries(DECRYPT
        PUBLIC
        fmt::fmt
        LOGGER::LOGGER
)
set(INCLUDES
        "${BASE_DIR}/ClogDecryptor/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty/fmt-12.1.0/include"
)

target_include_directories(DECRYPT
        PUBLIC
        ${INCLUDES}
)

