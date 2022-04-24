add_subdirectory(test)

add_library(Util
  css_color.cpp
)

set_target_properties(Util PROPERTIES OUTPUT_NAME "archon-util")

target_link_libraries(Util PUBLIC
  Core
)

install(TARGETS Util)
