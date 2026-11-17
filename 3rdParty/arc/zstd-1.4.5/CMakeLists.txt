

add_library(ZSTD STATIC
        common/debug.c
        common/entropy_common.c
        common/error_private.c
        common/fse_decompress.c
        common/pool.c
        common/threading.c
        common/xxhash.c
        common/zstd_common.c
        compress/fse_compress.c
        compress/hist.c
        compress/huf_compress.c
        compress/zstd_compress.c
        compress/zstd_compress_sequences.c
        compress/zstd_compress_superblock.c
        compress/zstd_compress_literals.c
        compress/zstd_double_fast.c
        compress/zstd_fast.c
        compress/zstd_lazy.c
        compress/zstd_ldm.c
        compress/zstd_opt.c
        decompress/huf_decompress.c
        decompress/zstd_ddict.c
        decompress/zstd_decompress.c
        decompress/zstd_decompress_block.c
        dictBuilder/cover.c
        dictBuilder/divsufsort.c
        dictBuilder/fastcover.c
        dictBuilder/zdict.c
)

target_include_directories(ZSTD
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/common>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/compress>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/decompress>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/dictBuilder>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/legacy>
        $<INSTALL_INTERFACE:include>
)

add_library(ZSTD::ZSTD ALIAS ZSTD)


include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ZSTD
        EXPORT zstdTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS ZSTD
        EXPORT libdwarfTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
# install headers
install(FILES zstd.h zdict.h zstd_errors.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY common/ compress/ decompress/ dictBuilder/ legacy/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")

install(EXPORT zstdTargets
        FILE zstdTargets.cmake
        NAMESPACE ZSTD::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zstd
)

configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/zstdConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/zstdConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zstd
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/zstdConfigVersion.cmake
        VERSION 1.4.5
        COMPATIBILITY SameMajorVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/zstdConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/zstdConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/zstd
)

target_compile_options(ZSTD PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/w>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:--no-warnings>
)
