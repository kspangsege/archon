cmake_minimum_required(VERSION 3.23.0)

project(Archon VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

option(ARCHON_BUILD_CORE_LIBRARY "Build the Archon Core Library" OFF)
option(ARCHON_BUILD_LOG_LIBRARY "Build the Archon Logging Library" OFF)
option(ARCHON_BUILD_CLI_LIBRARY "Build the Archon Commandline Interface Library" OFF)
option(ARCHON_BUILD_CHECK_LIBRARY "Build the Archon Testing Library" OFF)
option(ARCHON_BUILD_MATH_LIBRARY "Build the Archon Math Library" OFF)
option(ARCHON_BUILD_UTIL_LIBRARY "Build the Archon Utilities Library" OFF)
option(ARCHON_BUILD_IMAGE_LIBRARY "Build the Archon Image Library" OFF)
option(ARCHON_BUILD_FONT_LIBRARY "Build the Archon Font Rendering Library" OFF)
option(ARCHON_BUILD_GFX_LIBRARY "Build the Archon Graphics Utilities Library" OFF)
option(ARCHON_BUILD_DISPLAY_LIBRARY "Build the Archon Display Library" OFF)
option(ARCHON_BUILD_RENDER_LIBRARY "Build the Archon Graphics Rendering Library" OFF)

option(ARCHON_BUILD_ALL_LIBS "Build all Archon libraries" OFF)
option(ARCHON_BUILD_DEMO_PROGS "Build the Archon demo applications" OFF)
option(ARCHON_BUILD_TEST_SUITE "Build the Archon test suite" OFF)
option(ARCHON_BUILD_ALL "Build everything" OFF)

option(ARCHON_ASAN "Turn Address Sanitizer on" OFF)
option(ARCHON_TSAN "Turn Thread Sanitizer on" OFF)
option(ARCHON_UBSAN "Turn Undefined Behavior Sanitizer on" OFF)

include("cmake/dependencies.cmake")


# For an unknown reason, when using the Cygwin compiler, `-ansi` (or `-std=c++20`)
# suppresses the automatic setting of `_POSIX_C_SOURCE` when compiling in C++ mode. Judging
# from regular GCC, this is supposed to happen when compiling in C mode, but not when
# compiling in C++ mode.
if(CYGWIN)
  add_compile_options(-D_POSIX_C_SOURCE=200809)
endif()

# Necessary increase of the maximum number of sectinos in object files
if(MSVC)
  add_compile_options(/bigobj)
endif()
if(CYGWIN)
  add_compile_options(-Wa,-mbig-obj)
endif()

# Target Windows 10 when building with MinGW
if(MINGW)
  add_compile_options(-D_WIN32_WINNT=0x0A00)
endif()

# Warning level
if(NOT MSVC)
  add_compile_options(-pedantic -Wall -Wextra -Wundef -Wold-style-cast -Wextra-semi -Wunreachable-code -Wno-redundant-move)
endif()

# `constexpr` evaluation
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fconstexpr-ops-limit=100000000)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  add_compile_options(-fconstexpr-steps=100000000)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/constexpr:steps100000000)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

# Address Sanitizer
if(ARCHON_ASAN)
  if(MSVC)
    add_compile_options(/fsanitize=address /DARCHON_ASAN=1)
  else()
    add_compile_options(-fsanitize=address -DARCHON_ASAN=1)
    add_link_options(-fsanitize=address)
  endif()
endif()

# Thread Sanitizer
if(ARCHON_TSAN)
  if(MSVC)
    message(FATAL_ERROR "Not yet known how to turn Thread Sanitizer on with Visual Studio")
  else()
    add_compile_options(-fsanitize=thread -DARCHON_TSAN=1)
    add_link_options(-fsanitize=thread)
  endif()
endif()

# Undefined Behavior Sanitizer
if(ARCHON_UBSAN)
  if(MSVC)
    message(FATAL_ERROR "Not yet known how to turn Undefined Behavior Sanitizer on with Visual Studio")
  else()
    add_compile_options(-fsanitize=undefined -DARCHON_UBSAN=1)
    add_link_options(-fsanitize=undefined)
  endif()
endif()

include(CTest)

add_subdirectory(src)
