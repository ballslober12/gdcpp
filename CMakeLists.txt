cmake_minimum_required(VERSION 3.16)
project(gd_sdl2 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS src/*.cpp)
add_executable(gd ${SRC})
target_include_directories(gd PRIVATE src src/engine)

# SDL2 config packages export slightly different target names across platforms.
foreach(t SDL2::SDL2 SDL2_image::SDL2_image SDL2_mixer::SDL2_mixer SDL2_ttf::SDL2_ttf)
  if(TARGET ${t})
    target_link_libraries(gd PRIVATE ${t})
  endif()
endforeach()
if(NOT TARGET SDL2::SDL2)
  target_include_directories(gd PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(gd PRIVATE ${SDL2_LIBRARIES})
endif()
target_link_libraries(gd PRIVATE ZLIB::ZLIB)
if(NOT WIN32)
  target_link_libraries(gd PRIVATE m)
endif()

# Make the assets folder available next to the executable.
add_custom_command(TARGET gd POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:gd>/assets)
