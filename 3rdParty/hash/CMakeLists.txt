add_library(XXH3 STATIC
        # sha1.cpp
        md5.c
        xxhash.c
        crc32.c
        crc32c.c
        rijndael-alg-fst.c
        rijndael-api-fst.c
        # xxh_x86dispatch_auto.c
)
target_link_libraries(XXH3
        # OPENSSL::OPENSSL
)

target_include_directories(XXH3
        SYSTEM PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
        "${BASE_DIR}/3rdParty"
)


add_library(XXH3::XXH3 ALIAS XXH3)

target_compile_options(XXH3 PRIVATE -msse4.2)
