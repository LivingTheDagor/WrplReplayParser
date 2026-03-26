


add_subdirectory(arc)
add_subdirectory(eastl)
# add_subdirectory(openssl-3.x)
# add_subdirectory(mimalloc)
add_subdirectory(hash)
# add_subdirectory(regExp)
add_subdirectory(enet)
add_subdirectory(fmt-12.1.0)
add_subdirectory(Dagor_Engine)
add_subdirectory(hello_imgui)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    # add_subdirectory(libunwind-1.8.3)
    find_library(UNWIND_LIB unwind)
    set(CPPTRACE_UNWIND_WITH_LIBUNWIND ON) # random bullshit go
endif ()
add_subdirectory(cpptrace-1.0.4)
add_subdirectory(signal_tracer)
add_subdirectory(compile-time-regular-expressions-3.10.0)
add_subdirectory(tracy)
add_subdirectory(libdeflate)
add_subdirectory(tinyfiledialogs)

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/opencv")
    set(OPENCV_MODULES_BUILD "core;highgui;imgproc;imgcodecs;videoio" FORCE)
    add_subdirectory(opencv)
endif ()