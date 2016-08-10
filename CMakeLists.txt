# Required packages for building on Linux:
# libz-dev libxcb-image0-dev libxcb1-dev libjpeg-dev libudev-dev libglew-dev libx11-dev
# libopenal-dev libflac-dev libvorbis-dev libfreetype6-dev
cmake_minimum_required(VERSION 3.0)
project(iispace)

if(MSVC)
  foreach(flag
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
    endif()
  endforeach()
endif()

add_subdirectory(dependencies)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
endif()

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
add_custom_command(
  OUTPUT "${GENERATED_DIR}/iispace.pb.cc" "${GENERATED_DIR}/iispace.pb.h"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_DIR}"
  COMMAND protoc --proto_path=src "--cpp_out=${GENERATED_DIR}" "src/iispace.proto"
  DEPENDS protoc "src/iispace.proto" VERBATIM)

set(IISPACE_SOURCES
  src/boss_chaser.cpp
  src/boss.cpp
  src/boss_deathray.cpp
  src/boss_ghost.cpp
  src/boss_shield.cpp
  src/boss_square.cpp
  src/boss_super.cpp
  src/boss_tractor.cpp
  src/enemy.cpp
  src/fix32.cpp
  src/lib.cpp
  src/main.cpp
  src/modal.cpp
  src/overmind.cpp
  src/player.cpp
  src/replay.cpp
  src/save.cpp
  src/shape.cpp
  src/ship.cpp
  src/stars.cpp
  src/z0_game.cpp
  src/z.cpp
  "${GENERATED_DIR}/iispace.pb.cc")

add_executable(iispace ${IISPACE_SOURCES})
target_include_directories(iispace PRIVATE ${GENERATED_DIR})
target_link_libraries(iispace PRIVATE
  libprotobuf ois sfml-audio sfml-graphics sfml-window sfml-system)
set_property(TARGET iispace PROPERTY CXX_STANDARD 11)
if(NOT MSVC)
  target_compile_definitions(iispace PRIVATE PLATFORM_LINUX)
endif()

add_executable(score ${IISPACE_SOURCES})
target_include_directories(score PRIVATE ${GENERATED_DIR})
target_link_libraries(score PRIVATE libprotobuf)
set_property(TARGET score PROPERTY CXX_STANDARD 11)
target_compile_definitions(score PRIVATE PLATFORM_SCORE)

set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package")
add_custom_target(iispace_package ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PACKAGE_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "${PACKAGE_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:iispace> "${PACKAGE_DIR}"
  DEPENDS iispace VERBATIM)

add_custom_target(iispace_test
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMAND score "tests/seiken_1p__crikey_641530.wrp"
  DEPENDS score VERBATIM)
