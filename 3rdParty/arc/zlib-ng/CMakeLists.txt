
set(Sources
        "adler32.c"
        "chunkset.c"
        "compare256.c"
        "compress.c"
        "cpu_features.c"
        "crc32.c"
        "crc32_comb.c"
        "crc32_fold.c"
        "deflate.c"
        "deflate_fast.c"
        "deflate_huff.c"
        "deflate_medium.c"
        "deflate_quick.c"
        "deflate_rle.c"
        "deflate_slow.c"
        "deflate_stored.c"
        "functable.c"
        "gzlib.c"
        "gzread.c"
        "gzwrite.c"
        "infback.c"
        "inffast.c"
        "inflate.c"
        "inftrees.c"
        "insert_string.c"
        "insert_string_roll.c"
        "slide_hash.c"
        "trees.c"
        "uncompr.c"
        "zutil.c"
)

set(Allx86Sources
        "arch/x86/adler32_ssse3.c"
        "arch/x86/chunkset_sse2.c"
        "arch/x86/compare256_sse2.c"
        "arch/x86/crc32_fold_pclmulqdq.c"
        "arch/x86/crc32_fold_vpclmulqdq.c"
        "arch/x86/insert_string_sse42.c"
        "arch/x86/slide_hash_sse2.c"
        "arch/x86/x86_features.c"
)

set(AllAVX2Sources
        "arch/x86/slide_hash_avx2.c"
        "arch/x86/chunkset_avx.c"
        "arch/x86/compare256_avx2.c"
        "arch/x86/adler32_avx2.c"
)

set(AllNEONSources
        "arch/arm/adler32_neon.c"
        "arch/arm/arm_features.c"
        "arch/arm/chunkset_neon.c"
        "arch/arm/slide_hash_neon.c"
)

set(AllACLESources
        "arch/arm/crc32_acle.c"
        "arch/arm/insert_string_acle.c"
)


if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(Sources
            ${Sources}
            ${Allx86Sources}
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(Sources
            ${Sources}
            ${Allx86Sources}
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(Sources
            ${Sources}
            ${AllNEONSources}
    )
endif ()

set(INCLUDES
        "${CMAKE_CURRENT_LIST_DIR}"
)

add_library(Zlib
        ${Sources}
)

add_library(Zlib::Zlib ALIAS Zlib)

target_include_directories(Zlib
        SYSTEM PUBLIC
        ${INCLUDES}
)

target_compile_definitions(Zlib PUBLIC DZLIB_COMPAT=1)

set_target_properties(Zlib PROPERTIES
        COMPILE_OPTIONS --no-warnings
)
