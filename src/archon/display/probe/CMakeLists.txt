add_executable(ProbeX11
  probe_x11.cpp
)

set_target_properties(ProbeX11 PROPERTIES
  OUTPUT_NAME "archon-probe-x11"
)

target_link_libraries(ProbeX11 PUBLIC
  Core
  Log
  Util
  Image
)

if (X11_FOUND)
  target_link_libraries(ProbeX11 PRIVATE X11::X11)
  if (X11_Xrender_FOUND)
    target_link_libraries(ProbeX11 PRIVATE X11::Xrender)
  endif()
  if (X11_Xrandr_FOUND)
    target_link_libraries(ProbeX11 PRIVATE X11::Xrandr)
  endif()
endif()

add_executable(ProbeSDL
  probe_sdl.cpp
)

set_target_properties(ProbeSDL PROPERTIES
  OUTPUT_NAME "archon-probe-sdl"
)

target_link_libraries(ProbeSDL PUBLIC
  Core
  Log
)

if (SDL2_FOUND)
  target_include_directories(ProbeSDL PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(ProbeSDL PRIVATE ${SDL2_LIBRARIES})
endif()
