add_subdirectory(test)
add_subdirectory(demo)

set(ARCHON_ASSUME_VISUAL_STUDIO_CMAKE_GENERATOR 0)
if (CMAKE_GENERATOR MATCHES "^Visual Studio")
  set(ARCHON_ASSUME_VISUAL_STUDIO_CMAKE_GENERATOR 1)
endif()

configure_file(impl/config.h.in impl/config.h)

add_library(Core
  demangle.cpp
  terminate.cpp
  time.cpp
  newline_codec.cpp
  random.cpp
  locale.cpp
  timer.cpp
  misc_error.cpp
  build_environment.cpp
  platform_info.cpp
  platform_support.cpp
  resource_limits.cpp
  thread.cpp
  source.cpp
  rewindable_source.cpp
  filesystem.cpp
  file.cpp
  file_sink.cpp
  file_source.cpp
  text_file_error.cpp
  impl/prim_text_file_impl.cpp
  terminal.cpp
  base64.cpp
  word_wrap.cpp
)

set_target_properties(Core PROPERTIES OUTPUT_NAME "archon-core")

if(MSVC)
  # platform_info.cpp needs to link against GetFileVersionInfoSizeExW,
  # GetFileVersionInfoExW, and VerQueryValueW.
  target_link_libraries(Core version.lib)
endif()

install(TARGETS Core)
