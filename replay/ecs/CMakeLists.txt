add_library(dagECS
        ComponentTypes.cpp
        ComponentTypesDefs.cpp
        BitStreamDeserializer.cpp
        EntityManager.cpp
        DataComponents.cpp
        DataComponentsDefs.cpp
        baseio.cpp
        Component.cpp
        Templates.cpp
        Archetypes.cpp
        LoadTemplatesBLK.cpp
        EventsDB.cpp
        EntityManagerQuery.cpp
)


target_link_libraries(dagECS
        FieldSerializer:FieldSerializer
        BLK::BLK
        FS::FS
        VROMFS::VROMFS
        BLK::BLK
        # REGEX::REGEX
        XXH3
        NET::NET
        EASTL::EASTL
        LOGGER::LOGGER
)
add_library(dagECS::dagECS ALIAS dagECS)

set(INCLUDES
        "${BASE_DIR}/replay/include"
        "${BASE_DIR}/src/include"
        # "${BASE_DIR}/3rdParty"
)

target_include_directories(dagECS
        PUBLIC
        ${INCLUDES}
)


SetGenFlags(dagECS)