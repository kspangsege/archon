add_library(Render
  archon/render/opengl.cpp
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

set(ARCHON_RENDER_HAVE_OPENGL 0)
find_package(OpenGL)
if(OPENGL_FOUND)
  set(ARCHON_RENDER_HAVE_OPENGL 1)
  target_link_libraries(Render INTERFACE OpenGL::GL)
endif()

configure_file(archon/render/impl/config.h.in archon/render/impl/config.h)

target_sources(Render PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" "${ARCHON_SOURCE_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/render/impl/config.h"
  archon/render/opengl.hpp
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
