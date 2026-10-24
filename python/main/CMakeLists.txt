# Now set up your module
pybind11_add_module(PyReplayParser
        # ecs.cpp
        main.cpp
)
target_link_libraries(PyReplayParser PRIVATE
        ZLIB::ZLIB
        Python::Module
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

# A pip build puts the module at the top of the wheel, so that "import PyReplayParser" finds it.
if (SKBUILD)
    set(PY_INSTALL_DIR ".")
else ()
    set(PY_INSTALL_DIR "PyReplayParser")
endif ()

install(TARGETS PyReplayParser
        LIBRARY
        DESTINATION "${PY_INSTALL_DIR}"
        COMPONENT PyReplayParser
        RUNTIME
        DESTINATION "${PY_INSTALL_DIR}"
        COMPONENT PyReplayParser
)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    install(TARGETS signal_tracer
            LIBRARY
            DESTINATION "${PY_INSTALL_DIR}"
            COMPONENT PyReplayParser
            RUNTIME
            DESTINATION "${PY_INSTALL_DIR}"
            COMPONENT PyReplayParser
            ARCHIVE
            DESTINATION "${PY_INSTALL_DIR}"
            COMPONENT PyReplayParser
    )
endif ()

# strips most debug symbols
# dunno a better way to do this
install(CODE "set(PY_INSTALL_DIR \"${PY_INSTALL_DIR}\")" COMPONENT PyReplayParser)
install(CODE [[    set(_installed_file "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${PY_INSTALL_DIR}/$<TARGET_FILE_NAME:PyReplayParser>")
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