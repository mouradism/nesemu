# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
project(nesemu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Try to find SDL2, use FetchContent if not found
find_package(SDL2 CONFIG QUIET)

if(NOT SDL2_FOUND)
  message(STATUS "SDL2 not found, downloading via FetchContent...")
  include(FetchContent)
  
  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.28.5
    GIT_SHALLOW TRUE
  )
  
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)
  
  FetchContent_MakeAvailable(SDL2)
endif()

# Find OpenGL early
find_package(OpenGL REQUIRED)

# Fetch ImGui AFTER SDL2 is available
include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.89.9
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(imgui)

# Create ImGui library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC SDL2::SDL2 OpenGL::GL)

# Main executable
add_executable(nesemu
    src/main.cpp
    src/cpu.cpp
    src/ppu.cpp
    src/apu.cpp
    src/memory.cpp
    src/cartridge.cpp
    src/mapper.cpp
    src/video.cpp
    src/audio.cpp
    src/imgui_manager.cpp
)

target_include_directories(nesemu PRIVATE src)
target_link_libraries(nesemu PRIVATE SDL2::SDL2 imgui OpenGL::GL)

if(NOT SDL2_FOUND)
  # Copy SDL2 DLL to output directory
  add_custom_command(TARGET nesemu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE_DIR:nesemu>
  )
endif()

# GTest configuration
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
  GIT_SHALLOW TRUE
)

# For Windows, enable all features
set(gtest_force_shared_crt ON CACHE BOOL "Use shared C runtime" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# APU Unit Tests
add_executable(apu_test
    src/apu.cpp
    tests/apu_test.cpp
)

target_include_directories(apu_test PRIVATE src)
target_link_libraries(apu_test PRIVATE gtest gtest_main)

add_test(NAME APUTests COMMAND apu_test)

# PPU Unit Tests
add_executable(ppu_test
    src/ppu.cpp
    src/memory.cpp
    src/apu.cpp
    src/cartridge.cpp
    src/mapper.cpp
    tests/ppu_test.cpp
)

target_include_directories(ppu_test PRIVATE src)
target_link_libraries(ppu_test PRIVATE gtest gtest_main)

add_test(NAME PPUTests COMMAND ppu_test)

# Cartridge Unit Tests
add_executable(cartridge_test
    src/cartridge.cpp
    src/mapper.cpp
    tests/cartridge_test.cpp
)

target_include_directories(cartridge_test PRIVATE src)
target_link_libraries(cartridge_test PRIVATE gtest gtest_main)

add_test(NAME CartridgeTests COMMAND cartridge_test)

# Mapper Unit Tests
add_executable(mapper_test
    src/mapper.cpp
    tests/mapper_test.cpp
)

target_include_directories(mapper_test PRIVATE src)
target_link_libraries(mapper_test PRIVATE gtest gtest_main)

add_test(NAME MapperTests COMMAND mapper_test)

# Memory Unit Tests
add_executable(memory_test
    src/memory.cpp
    src/ppu.cpp
    src/apu.cpp
    src/cartridge.cpp
    src/mapper.cpp
    tests/memory_test.cpp
)

target_include_directories(memory_test PRIVATE src)
target_link_libraries(memory_test PRIVATE gtest gtest_main)

add_test(NAME MemoryTests COMMAND memory_test)

# Controller Unit Tests
add_executable(controller_test
    tests/controller_test.cpp
)

target_include_directories(controller_test PRIVATE src)
target_link_libraries(controller_test PRIVATE gtest gtest_main)

add_test(NAME ControllerTests COMMAND controller_test)

# CPU Unit Tests
add_executable(cpu_test
    src/cpu.cpp
    src/memory.cpp
    src/ppu.cpp
    src/apu.cpp
    src/cartridge.cpp
    src/mapper.cpp
    tests/cpu_test.cpp
)

target_include_directories(cpu_test PRIVATE src)
target_link_libraries(cpu_test PRIVATE gtest gtest_main)

add_test(NAME CPUTests COMMAND cpu_test)
