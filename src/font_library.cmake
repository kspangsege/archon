add_library(Font
  archon/font/face.cpp
  archon/font/loader.cpp
  archon/font/loader_fallback.cpp
  archon/font/loader_freetype.cpp
)

set_target_properties(Font PROPERTIES OUTPUT_NAME "archon-font")

target_link_libraries(Font PUBLIC
  Core
  Image
)

find_package(Freetype)

set(ARCHON_FONT_HAVE_FREETYPE 0)
if (FREETYPE_FOUND)
  set(ARCHON_FONT_HAVE_FREETYPE 1)
  target_link_libraries(Font PRIVATE Freetype::Freetype)
endif()

configure_file(archon/font/impl/config.h.in archon/font/impl/config.h)

target_sources(Font PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" "${ARCHON_SOURCE_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/font/impl/config.h"
  archon/font/size.hpp
  archon/font/code_point.hpp
  archon/font/face.hpp
  archon/font/loader.hpp
  archon/font/loader_fallback.hpp
  archon/font/loader_freetype.hpp
  archon/font.hpp
)

install(TARGETS Font FILE_SET HEADERS)

add_subdirectory(archon/font/test)
add_subdirectory(archon/font/tools)
add_subdirectory(archon/font/demo)
