cmake_minimum_required(VERSION 3.14.7)

project(Archon VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

if(MSVC)
  add_compile_options(/Zc:lambda /constexpr:steps10000000)
else()
  add_compile_options(-pedantic -Wall -Wextra -Wundef -Wold-style-cast -Wextra-semi -Wunreachable-code -Wno-redundant-move)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

# Address Sanitizer
option(ARCHON_ASAN "Turn Address Sanitizer on" OFF)
if(ARCHON_ASAN)
  if(MSVC)
    message(FATAL_ERROR "Not yet known how to turn Address Sanitizer on with Visual Studio")
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
