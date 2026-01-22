add_library(EASTL STATIC
        source/allocator_eastl.cpp
        source/assert.cpp
        source/atomic.cpp
        source/fixed_pool.cpp
        source/hashtable.cpp
        source/intrusive_list.cpp
        source/numeric_limits.cpp
        source/red_black_tree.cpp
        source/string.cpp
        source/thread_support.cpp
        mem.cpp
        snprintf.cpp
)

target_include_directories(EASTL
        SYSTEM PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
)



add_library(EASTL::EASTL ALIAS EASTL)