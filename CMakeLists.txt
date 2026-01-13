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
  
  add_executable(nesemu
      src/main.cpp
      src/cpu.cpp
      src/ppu.cpp
      src/apu.cpp
      src/memory.cpp
      src/cartridge.cpp
      src/video.cpp
      src/audio.cpp
  )
  
  target_include_directories(nesemu PRIVATE src)
  target_link_libraries(nesemu PRIVATE SDL2::SDL2)
  
  # Copy SDL2 DLL to output directory
  add_custom_command(TARGET nesemu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL2::SDL2>
    $<TARGET_FILE_DIR:nesemu>
  )
else()
  add_executable(nesemu
      src/main.cpp
      src/cpu.cpp
      src/ppu.cpp
      src/apu.cpp
      src/memory.cpp
      src/cartridge.cpp
      src/video.cpp
      src/audio.cpp
  )
  
  target_include_directories(nesemu PRIVATE src)
  target_link_libraries(nesemu PRIVATE SDL2::SDL2 SDL2::SDL2main)
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

# Enable testing
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
    src/cartridge.cpp
    tests/ppu_test.cpp
)

target_include_directories(ppu_test PRIVATE src)
target_link_libraries(ppu_test PRIVATE gtest gtest_main)

add_test(NAME PPUTests COMMAND ppu_test)

# Cartridge Unit Tests
add_executable(cartridge_test
    src/cartridge.cpp
    tests/cartridge_test.cpp
)

target_include_directories(cartridge_test PRIVATE src)
target_link_libraries(cartridge_test PRIVATE gtest gtest_main)

add_test(NAME CartridgeTests COMMAND cartridge_test)
