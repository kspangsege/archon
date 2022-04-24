add_subdirectory(test)
add_subdirectory(tools)
add_subdirectory(demo)

find_package(Freetype)

set(ARCHON_FONT_HAVE_FREETYPE 0)
if (FREETYPE_FOUND)
  set(ARCHON_FONT_HAVE_FREETYPE 1)
endif()

configure_file(impl/config.h.in impl/config.h)

add_library(Font
  face.cpp
  loader.cpp
  loader_fallback.cpp
  loader_freetype.cpp
)

set_target_properties(Font PROPERTIES OUTPUT_NAME "archon-font")

target_link_libraries(Font PUBLIC
  Core
  Image
)

if (FREETYPE_FOUND)
  target_link_libraries(Font PRIVATE Freetype::Freetype)
endif()

install(TARGETS Font)
