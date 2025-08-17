# Need X11 for the following reasons:
# * X11-based display implementation (see archon/display/x11_implementation.cpp)
find_package(X11)

# Need GLX for the following reasons:
# * X11-based display implementation (see archon/display/x11_implementation.cpp)
find_package(OpenGL)

# Need SDL for the following reasons:
# * SDL-based display implementation (see archon/display/sdl_implementation.cpp)
find_package(SDL2 2.0.22)

set(ARCHON_DISPLAY_HAVE_X11 0)
set(ARCHON_DISPLAY_HAVE_X11_XDBE 0)
set(ARCHON_DISPLAY_HAVE_X11_XKB 0)
set(ARCHON_DISPLAY_HAVE_X11_XRENDER 0)
set(ARCHON_DISPLAY_HAVE_X11_XRANDR 0)
set(ARCHON_DISPLAY_HAVE_X11_GLX 0)
if(X11_FOUND)
  set(ARCHON_DISPLAY_HAVE_X11 1)
  if(X11_Xext_FOUND)
    # Unfortunately, Xdbe is not directly covered by FindX11.cmake
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/25591.
    find_path(ARCHON_Xdbe_INCLUDE_PATH X11/extensions/Xdbe.h ${X11_Xext_INCLUDE_PATH})
    if(ARCHON_Xdbe_INCLUDE_PATH)
      set(ARCHON_DISPLAY_HAVE_X11_XDBE 1)
    endif()
  endif()
  if(X11_Xkb_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_XKB 1)
  endif()
  if(X11_Xrender_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_XRENDER 1)
  endif()
  if(X11_Xrandr_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_XRANDR 1)
  endif()
  if(OpenGL_GLX_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_GLX 1)
  endif()
endif()

set(ARCHON_DISPLAY_HAVE_SDL 0)
if(SDL2_FOUND)
  set(ARCHON_DISPLAY_HAVE_SDL 1)
endif()

set(ARCHON_DISPLAY_HAVE_OPENGL 0)
if(OPENGL_FOUND)
  set(ARCHON_DISPLAY_HAVE_OPENGL 1)
endif()

add_subdirectory(archon/display/probe)

add_library(Display
  archon/display/event_handler.cpp
  archon/display/viewport.cpp
  archon/display/noinst/edid.cpp
  archon/display/connection.cpp
  archon/display/implementation.cpp
  archon/display/x11_implementation.cpp
  archon/display/sdl_implementation.cpp
  archon/display/list_implementations.cpp
  archon/display/noinst/palette_map.cpp
  archon/display/noinst/x11/support.cpp
  archon/display/opengl.cpp
)

set_target_properties(Display PROPERTIES OUTPUT_NAME "archon-display")

target_link_libraries(Display PUBLIC
  Core
  Log
  Math
  Util
  Image
)

if(X11_FOUND)
  target_link_libraries(Display PRIVATE X11::X11)
  if(X11_Xext_FOUND)
    target_link_libraries(Display PRIVATE X11::Xext)
  endif()
  if(X11_Xrender_FOUND)
    target_link_libraries(Display PRIVATE X11::Xrender)
  endif()
  if(X11_Xrandr_FOUND)
    target_link_libraries(Display PRIVATE X11::Xrandr)
  endif()
  if(OpenGL_GLX_FOUND)
    target_link_libraries(Display PRIVATE OpenGL::GL)
  endif()
endif()

if(SDL2_FOUND)
  target_include_directories(Display PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(Display PRIVATE ${SDL2_LIBRARIES})
endif()

if(OPENGL_FOUND)
  target_link_libraries(Display PUBLIC OpenGL::GL)
endif()

configure_file(archon/display/impl/config.h.in archon/display/impl/config.h)

target_sources(Display PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" "${ARCHON_SOURCE_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/display/impl/config.h"
  archon/display/implementation_fwd.hpp
  archon/display/geometry.hpp
  archon/display/key.hpp
  archon/display/key_code.hpp
  archon/display/mouse_button.hpp
  archon/display/event.hpp
  archon/display/event_handler.hpp
  archon/display/resolution.hpp
  archon/display/viewport.hpp
  archon/display/guarantees.hpp
  archon/display/x11_fullscreen_monitors.hpp
  archon/display/x11_connection_config.hpp
  archon/display/sdl_connection_config.hpp
  archon/display/texture.hpp
  archon/display/window.hpp
  archon/display/connection.hpp
  archon/display/implementation.hpp
  archon/display/x11_implementation.hpp
  archon/display/sdl_implementation.hpp
  archon/display/as_key_name.hpp
  archon/display/list_implementations.hpp
  archon/display/opengl.hpp
  archon/display.hpp
)

install(TARGETS Display FILE_SET HEADERS)

add_subdirectory(archon/display/test)
add_subdirectory(archon/display/tool)
add_subdirectory(archon/display/demo)
