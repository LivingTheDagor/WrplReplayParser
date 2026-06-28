add_library(Translate STATIC
        translate.cpp
)

target_link_libraries(Translate
        DAG_MATH::DAG_MATH # brings in dagor engine includes
)
add_library(Translate::Translate ALIAS Translate)

target_include_directories(Translate
        PUBLIC
        "${BASE_DIR}/replay/include"
)

SetGenFlags(Translate)