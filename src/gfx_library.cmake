add_library(Gfx
  archon/gfx/object_builder.cpp
)

set_target_properties(Gfx PROPERTIES OUTPUT_NAME "archon-gfx")

target_link_libraries(Gfx PUBLIC
  Core
  Util
  Math
)

target_sources(Gfx PUBLIC FILE_SET HEADERS FILES
  archon/gfx/object_builder.hpp
)

install(TARGETS Gfx FILE_SET HEADERS)
