add_executable(LoadTextBLKTest
        main.cpp
)

# SET_TARGET_PROPERTIES(VromfsTest PROPERTIES LINK_FLAGS "/PROFILE")
target_link_libraries(LoadTextBLKTest
        # mimalloc-static
        BS::BS
        dagBlk::dagBlk
        dagIoSys::dagIoSys
        FS::FS

)


# target_compile_definitions(VromfsTest PRIVATE MI_MALLOC_OVERRIDE)
# target_include_directories(VromfsTest PRIVATE mimalloc/include)
