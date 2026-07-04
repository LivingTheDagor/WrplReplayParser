add_library(delta
        buffer.cpp
        deltaCompression.cpp
        history.cpp
        rle.cpp
)


add_library(DELTA::DELTA ALIAS delta)

target_link_libraries(delta
        LOGGER::LOGGER
        BS::BS
)

set(INCLUDES

        "${BASE_DIR}/3rdParty/Dagor_Engine/include"
        # "${BASE_DIR}/src/3rdParty/fmt-12.1.0/include"
)

target_include_directories(delta
        SYSTEM PUBLIC
        ${INCLUDES}
)

target_compile_options(delta PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/w>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:--no-warnings>
)