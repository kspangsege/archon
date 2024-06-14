cmake_minimum_required(VERSION 3.23.0)

project(Archon VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

# For an unknown reason, when using the Cygwin compiler, `-ansi` (or `-std=c++20`)
# suppresses the automatic setting of `_POSIX_C_SOURCE` when compiling in C++ mode. Judging
# from regular GCC, this is supposed to happen when compiling in C mode, but not when
# compiling in C++ mode.
if(CYGWIN)
  add_compile_options(-D_POSIX_C_SOURCE=200809)
endif()

# Necessary to increase number of sectinos in object file when using Cygwin (`/bigobj` with
# MSVC).
if(CYGWIN)
  add_compile_options(-Wa,-mbig-obj)
  add_link_options(-Wa,-mbig-obj)
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
option(ARCHON_ASAN "Turn Address Sanitizer on" OFF)
if(ARCHON_ASAN)
  if(MSVC)
    add_compile_options(/fsanitize=address /DARCHON_ASAN=1)
  else()
    add_compile_options(-fsanitize=address -DARCHON_ASAN=1)
    add_link_options(-fsanitize=address)
  endif()
endif()

# Thread Sanitizer
option(ARCHON_TSAN "Turn Thread Sanitizer on" OFF)
if(ARCHON_TSAN)
  if(MSVC)
    message(FATAL_ERROR "Not yet known how to turn Thread Sanitizer on with Visual Studio")
  else()
    add_compile_options(-fsanitize=thread -DARCHON_TSAN=1)
    add_link_options(-fsanitize=thread)
  endif()
endif()

# Undefined Behavior Sanitizer
option(ARCHON_UBSAN "Turn Undefined Behavior Sanitizer on" OFF)
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
