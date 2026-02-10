add_subdirectory(view_templates)
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/repl")
    add_subdirectory(repl)
endif ()