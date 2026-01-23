
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    add_custom_target(
            CodeGeneration ALL
            COMMAND py ${CMAKE_CURRENT_SOURCE_DIR}/generate.py "${BASE_DIR}/replay/include/mpi/codegen" "${BASE_DIR}/replay/mpi/codegen"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS builtin_types.py cpp_types.py objects/MPlayer.py objects/TeamData.py
            COMMENT "Generating code before build"
    )
else ()
    add_custom_target(
            CodeGeneration ALL
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/generate.py "${BASE_DIR}/replay/include/mpi/codegen" "${BASE_DIR}/replay/mpi/codegen"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS builtin_types.py cpp_types.py objects/MPlayer.py objects/TeamData.py
            COMMENT "Generating code before build"
    )
endif ()