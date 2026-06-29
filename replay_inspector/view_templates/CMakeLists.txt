# Your application
add_executable(TemplateViewer viewer.cpp)
target_link_libraries(TemplateViewer PRIVATE
        imgui
        SDL2::SDL2main
        SDL2::SDL2-static
        LOGGER::LOGGER
        fmt::fmt
        BS::BS
        dagBlk::dagBlk
        dagIoSys::dagIoSys
        FS::FS
        VROMFS::VROMFS
        FieldSerializer::FieldSerializer
        NET::NET
        dagECS::dagECS
        INIT::INIT
        Replay::Replay
)

target_include_directories(TemplateViewer
        PUBLIC
        "${BASE_DIR}/replay_inspector/include"
)

# if(WIN32)
# Copy SDL2.dll to the output directory after build
#     add_custom_command(TARGET TemplateViewer POST_BUILD
#             COMMAND ${CMAKE_COMMAND} -E copy_if_different
#             $<TARGET_FILE:SDL2>
#             $<TARGET_FILE_DIR:TemplateViewer>
#     )
# endif ()