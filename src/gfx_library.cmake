add_library(Gfx
  archon/gfx/math.cpp
  archon/gfx/object_builder.cpp
  archon/gfx/build_object.cpp
)

set_target_properties(Gfx PROPERTIES OUTPUT_NAME "archon-gfx")


target_sources(Gfx PUBLIC FILE_SET HEADERS FILES
  archon/gfx/math.hpp
  archon/gfx/object_builder.hpp
  archon/gfx/build_object.hpp
)


target_link_libraries(Gfx
  PUBLIC Core
  PUBLIC Math
  PUBLIC Util
)


install(TARGETS Gfx FILE_SET HEADERS)
