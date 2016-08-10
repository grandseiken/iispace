set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
add_subdirectory(protobuf/cmake EXCLUDE_FROM_ALL)

set(BUILD_SHARED_LIBS FALSE CACHE BOOL "" FORCE)
set(SFML_USE_STATIC_STD_LIBS TRUE CACHE BOOL "" FORCE)
add_subdirectory(SFML EXCLUDE_FROM_ALL)
target_include_directories(sfml-system INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/SFML/include")

set(OIS_SOURCES
  OIS/src/OISEffect.cpp
  OIS/src/OISException.cpp
  OIS/src/OISForceFeedback.cpp
  OIS/src/OISInputManager.cpp
  OIS/src/OISJoyStick.cpp
  OIS/src/OISKeyboard.cpp
  OIS/src/OISObject.cpp)

if(MSVC)
  set(OIS_SOURCES ${OIS_SOURCES}
    OIS/src/win32/Win32ForceFeedback.cpp
    OIS/src/win32/Win32InputManager.cpp
    OIS/src/win32/Win32JoyStick.cpp
    OIS/src/win32/Win32KeyBoard.cpp
    OIS/src/win32/Win32Mouse.cpp)
else()
  set(OIS_SOURCES ${OIS_SOURCES}
    OIS/src/linux/EventHelpers.cpp
    OIS/src/linux/LinuxForceFeedback.cpp
    OIS/src/linux/LinuxInputManager.cpp
    OIS/src/linux/LinuxJoyStickEvents.cpp
    OIS/src/linux/LinuxKeyboard.cpp
    OIS/src/linux/LinuxMouse.cpp)
endif()

add_library(ois STATIC ${OIS_SOURCES})
target_include_directories(ois SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/OIS/includes")
target_include_directories(ois PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/OIS/src")
