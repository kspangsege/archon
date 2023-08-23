add_library(Util
  archon/util/css_color.cpp
)

set_target_properties(Util PROPERTIES OUTPUT_NAME "archon-util")

target_link_libraries(Util PUBLIC
  Core
)

target_sources(Util PUBLIC FILE_SET HEADERS FILES
  archon/util/unit_frac.hpp
  archon/util/kdtree.hpp
  archon/util/color_space.hpp
  archon/util/color.hpp
  archon/util/colors.hpp
  archon/util/css_color.hpp
  archon/util/as_css_color.hpp
  archon/util/perlin_noise.hpp
  archon/util/rectangle_packer.hpp
)

install(TARGETS Util FILE_SET HEADERS)

add_subdirectory(archon/util/test)
