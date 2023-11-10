add_executable(ProbeOpenGL
  probe_opengl.cpp
)

set_target_properties(ProbeOpenGL PROPERTIES
  OUTPUT_NAME "archon-probe-opengl"
)

target_link_libraries(ProbeOpenGL PUBLIC
  Display
  Render
)
