cmake_minimum_required(VERSION 3.16)
project(CastleEscape VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EMSCRIPTEN)
  # The Emscripten SDL2 port supplies the library; expose it under the same
  # target name the native build gets from find_package(SDL2).
  add_library(SDL2::SDL2 INTERFACE IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    INTERFACE_COMPILE_OPTIONS "SHELL:-sUSE_SDL=2"
    INTERFACE_LINK_OPTIONS "SHELL:-sUSE_SDL=2")
  # Emscripten has no separate libm; keep FindLua from appending a
  # LUA_MATH_LIBRARY-NOTFOUND entry that would fail find_package.
  set(LUA_MATH_LIBRARY "" CACHE FILEPATH "not needed under Emscripten" FORCE)
else()
  find_package(SDL2 REQUIRED)
endif()
find_package(Lua 5.4 REQUIRED)

# Embed the platform-independent game script; no working-directory dependency.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS scripts/game.lua)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/scripts/game.lua" GAME_SCRIPT)
configure_file(src/game_script.hpp.in generated/game_script.hpp @ONLY)

add_library(game_logic src/game.cpp)
target_include_directories(game_logic PUBLIC src PRIVATE "${LUA_INCLUDE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(game_logic PRIVATE ${LUA_LIBRARIES})

add_library(game_render src/render.cpp)
target_include_directories(game_render PUBLIC src)
target_link_libraries(game_render PUBLIC game_logic)

add_library(game_app src/app.cpp)
target_include_directories(game_app PUBLIC src)
target_link_libraries(game_app PUBLIC game_logic game_render)

add_library(platform_sdl src/platform_sdl.cpp)
target_include_directories(platform_sdl PUBLIC src)
target_link_libraries(platform_sdl PUBLIC SDL2::SDL2)

if(EMSCRIPTEN)
  add_executable(castle_escape_web src/main_web.cpp)
  target_link_libraries(castle_escape_web PRIVATE game_app platform_sdl)
  target_link_options(castle_escape_web PRIVATE
    -sUSE_SDL=2
    -sALLOW_MEMORY_GROWTH=1
    -sEXIT_RUNTIME=0
    -sINITIAL_MEMORY=67108864
    -sSTACK_SIZE=1048576
    --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html)
  set_property(TARGET castle_escape_web APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
  set_target_properties(castle_escape_web PROPERTIES
    SUFFIX ".html"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist/web"
    OUTPUT_NAME "castle_escape")
  add_custom_command(TARGET castle_escape_web POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/dist/web/castle_escape.html"
      "${CMAKE_CURRENT_SOURCE_DIR}/dist/web/index.html")
else()
  add_executable(castle_escape src/main.cpp)
  target_link_libraries(castle_escape PRIVATE game_app platform_sdl)
endif()

include(CTest)
if(BUILD_TESTING AND NOT EMSCRIPTEN)
  add_executable(game_logic_test tests/game_logic_test.cpp)
  target_link_libraries(game_logic_test PRIVATE game_logic)
  add_test(NAME logic_and_movement_properties COMMAND game_logic_test)
  add_executable(guards_rooms_test tests/guards_rooms_test.cpp)
  target_link_libraries(guards_rooms_test PRIVATE game_logic)
  add_test(NAME guards_rooms_and_occlusion COMMAND guards_rooms_test)
  add_executable(combat_alarm_test tests/combat_alarm_test.cpp)
  target_link_libraries(combat_alarm_test PRIVATE game_logic)
  add_test(NAME combat_ammo_and_alarm COMMAND combat_alarm_test)
  add_executable(objectives_test tests/objectives_test.cpp)
  target_link_libraries(objectives_test PRIVATE game_logic)
  add_test(NAME objectives COMMAND objectives_test)
  add_executable(application_test tests/application_test.cpp)
  target_link_libraries(application_test PRIVATE game_app)
  add_test(NAME application_loop COMMAND application_test)
  add_test(NAME sdl_smoke COMMAND castle_escape --smoke-test)
  set_tests_properties(sdl_smoke PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 15)
endif()
