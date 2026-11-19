include(FetchContent)

FetchContent_Declare(
        sol2
        GIT_REPOSITORY https://github.com/ThePhD/sol2.git
        GIT_TAG v3.5.0
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(sol2)

add_library(SOL3::SOL3 ALIAS sol2)
target_link_libraries(sol2 INTERFACE LUA::LUA)
