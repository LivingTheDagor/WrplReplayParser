# Now set up your module
pybind11_add_module(PyReplayParser
        # ecs.cpp
        main.cpp
)
target_link_libraries(PyReplayParser PRIVATE
        ZLIB::ZLIB
        Python::Python
        PyReplay::PyReplay
        PyState::PyState
        PyECS::PyECS
        PyTranslate::PyTranslate
        pybind11::module
)

target_include_directories(PyReplayParser PRIVATE
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/python/include"
)
attach_signal_tracer(PyReplayParser)
if (LDAG_DBGLEVEL_TO_SET EQUAL 0)
    target_compile_options(PyReplayParser PRIVATE -fPIC)
    target_link_options(PyReplayParser PRIVATE
            -static-libgcc
            -static-libstdc++
            # -static
    )
endif ()

install(TARGETS PyReplayParser
        LIBRARY
        DESTINATION "PyReplayParser"
        COMPONENT PyReplayParser
        RUNTIME
        DESTINATION "PyReplayParser"
        COMPONENT PyReplayParser
)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    install(TARGETS signal_tracer
            LIBRARY
            DESTINATION "PyReplayParser"
            COMPONENT PyReplayParser
            RUNTIME
            DESTINATION "PyReplayParser"
            COMPONENT PyReplayParser
            ARCHIVE
            DESTINATION "PyReplayParser"
            COMPONENT PyReplayParser
    )
endif ()

# strips most debug symbols
# dunno a better way to do this
install(CODE [[    set(_installed_file "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/PyReplayParser/$<TARGET_FILE_NAME:PyReplayParser>")
    if(EXISTS "${_installed_file}")
        execute_process(
            COMMAND "strip" --strip-debug "${_installed_file}"
            RESULT_VARIABLE strip_result
        )
        if(NOT strip_result EQUAL 0)
            message(FATAL_ERROR "strip failed for PyReplayParser: ${strip_result} on ${_installed_file} on exe ${CMAKE_STRIP}")
        endif()
    else()
        message(FATAL_ERROR "strip target not found: ${_installed_file}")
    endif()
]]
        COMPONENT PyReplayParser)