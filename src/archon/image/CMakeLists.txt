add_subdirectory(test)
add_subdirectory(demo)

find_package(PNG 1.5.4)

set(ARCHON_IMAGE_HAVE_PNG 0)
if (PNG_FOUND)
  set(ARCHON_IMAGE_HAVE_PNG 1)
endif()

configure_file(impl/config.h.in impl/config.h)

add_library(Image
  color_space.cpp
  buffer_format.cpp
  writable_image.cpp
  palettes.cpp
  reader.cpp
  writer.cpp
  file_format_png.cpp
  file_format_registry.cpp
  error.cpp
  load.cpp
  save.cpp
)

set_target_properties(Image PROPERTIES OUTPUT_NAME "archon-image")

target_link_libraries(Image PUBLIC
  Core
)

if (PNG_FOUND)
  target_link_libraries(Image PRIVATE PNG::PNG)
endif()

install(TARGETS Image)
