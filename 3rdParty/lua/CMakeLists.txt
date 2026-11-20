# https://www.seanbutler.net/2024/08/26/cmake-lua-cpp-project

set(LUA_VERSION "lua-5.3.5")

include(ExternalProject)

find_program(LUA_MAKE_PROGRAM NAMES mingw32-make make REQUIRED)

if (WIN32)
    set(LUA_BUILD_TARGET mingw)
else ()
    set(LUA_BUILD_TARGET generic)
endif ()

ExternalProject_Add(lua_project
        PREFIX ${CMAKE_BINARY_DIR}/${LUA_VERSION}
        URL https://www.lua.org/ftp/${LUA_VERSION}.tar.gz
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${LUA_MAKE_PROGRAM} ${LUA_BUILD_TARGET}
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE true
        BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/${LUA_VERSION}/src/lua_project/src/liblua.a
        DOWNLOAD_EXTRACT_TIMESTAMP false
)

# Specify where to find Lua includes and libraries after building

ExternalProject_Get_Property(lua_project BINARY_DIR)
set(LUA_INCLUDE_DIR ${BINARY_DIR}/src)
set(LUA_LIBRARIES ${BINARY_DIR}/src/liblua.a)


file(MAKE_DIRECTORY ${LUA_INCLUDE_DIR})  # must exist at configure time

add_library(LUA::LUA STATIC IMPORTED GLOBAL)
set_target_properties(LUA::LUA PROPERTIES
        IMPORTED_LOCATION ${LUA_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${LUA_INCLUDE_DIR}
)
add_dependencies(LUA::LUA lua_project)

