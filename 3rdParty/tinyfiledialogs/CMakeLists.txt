add_library(TFD STATIC tinyfiledialogs.c)


target_include_directories(TFD
        SYSTEM PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
)

add_library(TFD::TFD ALIAS TFD)

target_compile_options(TFD PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/w>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:--no-warnings>
)