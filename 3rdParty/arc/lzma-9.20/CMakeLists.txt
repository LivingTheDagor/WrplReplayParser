

add_library(LZMA STATIC
        LzmaDec.c
        LzmaEnc.c
        LzFind.c
)

target_include_directories(LZMA
        SYSTEM PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
)


target_compile_definitions(LZMA PRIVATE _7ZIP_ST)

add_library(LZMA::LZMA ALIAS LZMA)

target_compile_options(LZMA PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/w>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:--no-warnings>
)