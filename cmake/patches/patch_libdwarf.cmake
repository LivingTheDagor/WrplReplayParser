file(READ "src/lib/libdwarf/CMakeLists.txt" content)
message("PATCHING LIBDWARF")

if (content MATCHES "TARGETS[ \t]+dwarf[ \t]+zlibstatic zlib")
    message(STATUS "zlib already present on TARGETS dwarf line; skipping patch.")
else ()
    if (content MATCHES "TARGETS[ \t]+dwarf")
        string(REPLACE
                "  TARGETS dwarf"
                "  TARGETS dwarf zlibstatic zlib"
                patched_content
                "${content}"
        )
        file(WRITE "src/lib/libdwarf/CMakeLists.txt" "${patched_content}")
        message(STATUS "Added zlib to TARGETS dwarf.")
    else ()
        message(WARNING "Did not find expected 'TARGETS dwarf' line; no changes made.")
    endif ()
endif ()