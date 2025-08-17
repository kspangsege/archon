add_library(Render
  archon/render/virt_trackball.cpp
  archon/render/impl/key_bindings.cpp
  archon/render/engine.cpp
  archon/render/noinst/engine_impl.cpp
)

set_target_properties(Render PROPERTIES OUTPUT_NAME "archon-render")

target_link_libraries(Render PUBLIC
  Core
  Log
  Math
  Util
  Display
)

target_sources(Render PUBLIC FILE_SET HEADERS FILES
  archon/render/impl/finite_sequence_memory.hpp
  archon/render/impl/finite_curve_memory.hpp
  archon/render/virt_trackball.hpp
  archon/render/key_binding_support.hpp
  archon/render/impl/key_bindings.hpp
  archon/render/engine.hpp
  archon/render.hpp
)

install(TARGETS Render FILE_SET HEADERS)

add_subdirectory(archon/render/probe)
add_subdirectory(archon/render/demo)
