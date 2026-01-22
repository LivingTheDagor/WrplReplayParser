

if (NOT FUNCTIONS_INCLUDED)
    set(FUNCTIONS_INCLUDED TRUE)
    function(print_stats)
        message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
        message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
        message(STATUS "C++ Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
        message(STATUS "C Compiler ID: ${CMAKE_C_COMPILER_ID}")
        message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
        message(STATUS "System Name: ${CMAKE_SYSTEM_NAME}")
        message(STATUS "System Processor: ${CMAKE_SYSTEM_PROCESSOR}")
        message(STATUS "Generator Platform: ${CMAKE_GENERATOR_PLATFORM}")
        message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
    endfunction()

    function(set_compiler_flags)
        # set(BUILD_SHARED_LIBS OFF)
        # add_definitions(-D_GLIBCXX_USE_CXX11_ABI=1)

        # add_compile_definitions(_TARGET_SIMD_SSE=2)
        # add_compile_definitions(WITH_SHOW_INCLUDES=ON)

        if("${MSVC_WARNINGS}" STREQUAL "")
            set(MSVC_WARNINGS
                    /W4 # Baseline reasonable warnings
                    /w14242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
                    /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
                    /w14263 # 'function': member function does not override any base class virtual member function
                    /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not
                    # be destructed correctly
                    /w14287 # 'operator': unsigned/negative constant mismatch
                    /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
                    # the for-loop scope
                    /w14296 # 'operator': expression is always 'boolean_value'
                    /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
                    /w14545 # expression before comma evaluates to a function which is missing an argument list
                    /w14546 # function call before comma missing argument list
                    /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
                    /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
                    /w14555 # expression has no effect; expected expression with side- effect
                    /w14619 # pragma warning: there is no warning number 'number'
                    /w14640 # Enable warning on thread un-safe static member initialization
                    /w14826 # Conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior.
                    /w14905 # wide string literal cast to 'LPSTR'
                    /w14906 # string literal cast to 'LPWSTR'
                    /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
                    /permissive- # standards conformance mode for MSVC compiler.
            )
        endif()

        if("${CLANG_WARNINGS}" STREQUAL "")
            set(CLANG_WARNINGS
                    -Wall
                    -Wextra # reasonable and standard
                    # -Wshadow # warn the user if a variable declaration shadows one from a parent context
                    -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps
                    # catch hard to track down memory errors
                    # -Wold-style-cast # warn for c-style casts
                    -Wcast-align # warn for potential performance problem casts
                    -Wunused # warn on anything being unused
                    -Woverloaded-virtual # warn if you overload (not override) a virtual function
                    -Wpedantic # warn if non-standard C++ is used
                    -Wconversion # warn on type conversions that may lose data
                    # -Wsign-conversion # warn on sign conversions # causes alot of issues in 3rd party libs
                    -Wnull-dereference # warn if a null dereference is detected
                    -Wdouble-promotion # warn if float is implicit promoted to double
                    -Wformat=2 # warn on security issues around functions that format output (ie printf)
                    -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
            )
        endif()

        if("${GCC_WARNINGS}" STREQUAL "")
            set(GCC_WARNINGS
                    ${CLANG_WARNINGS}
                    -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
                    -Wduplicated-cond # warn if if / else chain has duplicated conditions
                    -Wduplicated-branches # warn if if / else branches have duplicated code
                    -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
                    # -Wuseless-cast # warn if you perform a cast to the same type # fuck you my cast is not usless
                    -Wsuggest-override # warn if an overridden member function is not marked 'override' or 'final'
            )
        endif()

        if("${CUDA_WARNINGS}" STREQUAL "")
            set(CUDA_WARNINGS
                    -Wall
                    -Wextra
                    -Wunused
                    -Wconversion
                    -Wshadow
                    # TODO add more Cuda warnings
            )
        endif()
        set(GEN_NO_NON_MSVC
                -Wno-multichar
                -Wno-attributes
                -Wno-ignored-attributes
                -Wno-unused-function
                -Wno-unused-variable
                # -Wno-sign-compare
        )

        # THIS IS BAD BUT I DUNNO HOW TO DO IT AND DIDNT LOOK IT UP (sorta???)
        # if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        #     add_compile_options(${MSVC_WARNINGS})
        # elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        #     add_compile_options(${CLANG_WARNINGS} ${GEN_NO_NON_MSVC})
        # elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            #  -Wno-reorder
        #     add_compile_options(${GCC_WARNINGS} ${GEN_NO_NON_MSVC})
            # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-multichar -Wno-attributes -Wno-ignored-attributes" PARENT_SCOPE)
        # endif ()

        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            if(CMAKE_BUILD_TYPE STREQUAL "Release")
                set(BUILD_SHARED_LIBS OFF)
                # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static" PARENT_SCOPE)
                # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static" PARENT_SCOPE)
                if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi" PARENT_SCOPE)
                    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /debug" PARENT_SCOPE)
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /wd4068" PARENT_SCOPE)
                    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" PARENT_SCOPE)
                    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" PARENT_SCOPE)
                elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static" PARENT_SCOPE)
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3" PARENT_SCOPE)
                elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static" PARENT_SCOPE)
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3" PARENT_SCOPE)
                endif ()
            elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
                if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                    add_compile_options(/showIncludes)
                    # add_compile_options(/fsanitize=address)
                    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address " PARENT_SCOPE)
                    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}" PARENT_SCOPE)
                    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug" PARENT_SCOPE)
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /showIncludes /Y- /MTd" PARENT_SCOPE)
                    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /debug" PARENT_SCOPE)
                elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0" PARENT_SCOPE)
                elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                    add_compile_options(-msse4.2)
                    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -msse4.2 -march=native" PARENT_SCOPE)
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2 -march=native -O0" PARENT_SCOPE)
                endif ()
            else()
                if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                    add_compile_options(/showIncludes)
                    # add_compile_options(/fsanitize=address)
                    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address /MTd" PARENT_SCOPE)
                endif ()
            endif ()
        elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
            if(CMAKE_BUILD_TYPE STREQUAL "release")
                if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3" PARENT_SCOPE)
                elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3" PARENT_SCOPE)
                endif ()
            elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
                if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0" PARENT_SCOPE)
                elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIC -fsanitize=address" PARENT_SCOPE)
                    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O0 -fsanitize=address -fno-omit-frame-pointer -g" PARENT_SCOPE)
                endif ()
            endif ()
        endif ()
    endfunction()


    function(set_data_folders_loc)
        message("set config files dir to ${CMAKE_CURRENT_SOURCE_DIR}/config_dir")
        add_compile_definitions(CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/config_dir")

        message("set base dir to ${CMAKE_CURRENT_SOURCE_DIR}")
        add_compile_definitions(BASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")


        message("set warthunder dir to D:/SteamLibrary/steamapps/common/War Thunder")
        add_compile_definitions(WARTHUNDER_DIR="D:/SteamLibrary/steamapps/common/War Thunder")
    endfunction()
endif ()
