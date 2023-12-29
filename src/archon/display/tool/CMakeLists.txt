add_executable(SetupStdColormap
  setup_std_colormap.cpp
)

set_target_properties(SetupStdColormap PROPERTIES
  OUTPUT_NAME "archon-setup-std-colormap"
)

target_link_libraries(SetupStdColormap PUBLIC
  Cli
  Display
)

if(X11_FOUND)
  target_link_libraries(SetupStdColormap PRIVATE X11::X11)
  if(X11_Xext_FOUND)
    target_link_libraries(SetupStdColormap PRIVATE X11::Xext)
  endif()
  if (X11_Xrender_FOUND)
    target_link_libraries(SetupStdColormap PRIVATE X11::Xrender)
  endif()
  if (X11_Xrandr_FOUND)
    target_link_libraries(SetupStdColormap PRIVATE X11::Xrandr)
  endif()
  if(OpenGL_GLX_FOUND)
    target_link_libraries(SetupStdColormap PRIVATE OpenGL::GL)
  endif()
endif()
