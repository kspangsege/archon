add_library(Image
  archon/image/color_space.cpp
  archon/image/buffer_format.cpp
  archon/image/writable_image.cpp
  archon/image/palettes.cpp
  archon/image/reader.cpp
  archon/image/writer.cpp
  archon/image/file_format_png.cpp
  archon/image/file_format_registry.cpp
  archon/image/error.cpp
  archon/image/load.cpp
  archon/image/save.cpp
)

set_target_properties(Image PROPERTIES OUTPUT_NAME "archon-image")

target_link_libraries(Image PUBLIC
  Core
)

find_package(PNG 1.5.4)

set(ARCHON_IMAGE_HAVE_PNG 0)
if(PNG_FOUND)
  set(ARCHON_IMAGE_HAVE_PNG 1)
  target_link_libraries(Image PRIVATE PNG::PNG)
endif()

configure_file(archon/image/impl/config.h.in archon/image/impl/config.h)

target_sources(Image PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" "${ARCHON_SOURCE_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/image/impl/config.h"
  archon/image/geom.hpp
  archon/image/iter.hpp
  archon/image/tray.hpp
  archon/image/impl/bit_medium.hpp
  archon/image/bit_medium.hpp
  archon/image/impl/comp_types.hpp
  archon/image/comp_types.hpp
  archon/image/gamma.hpp
  archon/image/comp_repr.hpp
  archon/image/blend.hpp
  archon/image/color_space.hpp
  archon/image/pixel_convert.hpp
  archon/image/pixel_repr.hpp
  archon/image/pixel.hpp
  archon/image/block.hpp
  archon/image/standard_channel_spec.hpp
  archon/image/custom_channel_spec.hpp
  archon/image/bit_field.hpp
  archon/image/channel_packing.hpp
  archon/image/buffer_format.hpp
  archon/image/image.hpp
  archon/image/computed_image.hpp
  archon/image/palette_image.hpp
  archon/image/writable_image.hpp
  archon/image/tray_image.hpp
  archon/image/indexed_tray_image.hpp
  archon/image/integer_pixel_format.hpp
  archon/image/packed_pixel_format.hpp
  archon/image/subword_pixel_format.hpp
  archon/image/indexed_pixel_format.hpp
  archon/image/buffered_image.hpp
  archon/image/palettes.hpp
  archon/image/impl/workspace.hpp
  archon/image/reader.hpp
  archon/image/writer.hpp
  archon/image/progress_tracker.hpp
  archon/image/provider.hpp
  archon/image/file_format.hpp
  archon/image/file_format_registry.hpp
  archon/image/file_format_png.hpp
  archon/image/load_config.hpp
  archon/image/save_config.hpp
  archon/image/input.hpp
  archon/image/output.hpp
  archon/image/error.hpp
  archon/image/load.hpp
  archon/image/save.hpp
  archon/image.hpp
)

install(TARGETS Image FILE_SET HEADERS)

add_subdirectory(archon/image/test)
add_subdirectory(archon/image/demo)
