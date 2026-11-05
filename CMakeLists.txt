cmake_minimum_required(VERSION 3.21)
project(Win11To10 VERSION 0.2.0 LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "NoyaPLANET 11to10 currently builds on Windows only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Existing command-line engine: called on demand by the GUI when the user
# explicitly confirms a reversible visual-effects change.
add_executable(Win11To10 src/Win11To10.cpp)
target_compile_definitions(Win11To10 PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
target_link_libraries(Win11To10 PRIVATE user32 advapi32)

# Native Win32 graphical dashboard and standalone Start menu preview.
# No browser, no .NET runtime and no service or Explorer modifications.
add_library(StartMenuModel STATIC src/StartMenuModel.cpp)
target_include_directories(StartMenuModel PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_executable(NoyaPLANET11to10 WIN32 src/Gui.cpp src/TaskbarPreview.cpp)
target_compile_definitions(NoyaPLANET11to10 PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
target_link_libraries(NoyaPLANET11to10 PRIVATE StartMenuModel user32 gdi32 shell32 advapi32)

if(MSVC)
  foreach(target Win11To10 NoyaPLANET11to10 StartMenuModel)
    target_compile_options(${target} PRIVATE /W4 /utf-8 /permissive-)
  endforeach()
endif()

enable_testing()
add_test(NAME BackupAndProfile COMMAND Win11To10 self-test)
add_test(NAME GuiStartupModel COMMAND NoyaPLANET11to10 --self-test)

add_executable(StartMenuModelTests tests/StartMenuModelTests.cpp)
target_link_libraries(StartMenuModelTests PRIVATE StartMenuModel)
if(MSVC)
  target_compile_options(StartMenuModelTests PRIVATE /W4 /utf-8 /permissive-)
endif()
add_test(NAME StartMenuSearch COMMAND StartMenuModelTests)
