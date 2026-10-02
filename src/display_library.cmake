include(FindPackageMessage)


# Need X11 for the following reasons:
# * X11-based display implementation (see archon/display/x11_implementation.cpp)
#
find_package(X11 QUIET)
set(_version "unknown")
set(_library "")
if(X11_FOUND)
  # Manually hunt for the `x11.pc` file in nearby pkgconfig directories
  if (X11_X11_LIB)
    get_filename_component(_dir "${X11_X11_LIB}" DIRECTORY)
    find_file(_pc_file "x11.pc" PATHS
      "${_dir}/pkgconfig"
      "${_dir}/../lib/pkgconfig"
      "${_dir}/../share/pkgconfig"
    )
    if(_pc_file)
      file(STRINGS "${_pc_file}" _line REGEX "^Version: ")
      if(_line MATCHES "Version: ([0-9]+\\.[0-9]+\\.[0-9]+)")
        set(_version "${CMAKE_MATCH_1}")
        set(_library "${X11_X11_LIB}")
      endif()
    endif()
  endif()
endif()
set(ARCHON_GOOD_X11_FOUND 0)
set(_min_version "1.3.0")
if(NOT _version STREQUAL "unknown" AND _version VERSION_GREATER_EQUAL _min_version)
  set(ARCHON_GOOD_X11_FOUND 1)
  string(CONCAT _msg
    "Found Xlib: ${_library} (found suitable version \"${_version}\", minimum required is \"${_min_version}\")"
  )
else()
  string(CONCAT _msg
    "Could NOT find Xlib (minimum required version is \"${_min_version}\")"
  )
endif()
find_package_message(ARCHON_XLIB_MSG "${_msg}" "[${_version}][${_min_version}][${_library}]")


# Need OpenGL for the following reasons:
# * Exposure of OpenGL to applciations through archon/display/opengl.hpp
#
# Need GLX for the following reasons:
# * X11-based display implementation (see archon/display/x11_implementation.cpp)
#
find_package(OpenGL QUIET)
if(OPENGL_FOUND)
  set(_library "${OPENGL_LIBRARIES}")
  string(CONCAT _msg
    "Found OpenGL: ${_library}"
  )
else()
  set(_library "")
  string(CONCAT _msg
    "Could NOT find OpenGL"
  )
endif()
find_package_message(ARCHON_OPENGL_MSG "${_msg}" "[${OPENGL_FOUND}][${_library}]")


# Need SDL for the following reasons:
# * SDL-based display implementation (see archon/display/sdl_implementation.cpp)
#
set(_archon_sdl_min_version "3.2.20")
find_package(SDL3 ${_archon_sdl_min_version} CONFIG QUIET)
if(SDL3_FOUND)
  set(_version "${SDL3_VERSION}")
  get_target_property(_library SDL3::SDL3 IMPORTED_LOCATION)
  if(NOT _library OR _library MATCHES "-NOTFOUND")
    get_target_property(_configs SDL3::SDL3 IMPORTED_CONFIGURATIONS)
    if(_configs)
      list(GET _configs 0 _first_config)
      get_target_property(_library SDL3::SDL3 IMPORTED_LOCATION_${_first_config})
    endif()
  endif()
  if(NOT _library OR _library MATCHES "-NOTFOUND")
    set(_library "unknown")
  endif()
  string(CONCAT _msg
    "Found SDL: ${_library} (found version \"${_version}\")"
  )
else()
  set(_version "")
  set(_library "")
  string(CONCAT _msg
    "Could NOT find SDL (minimum required version is \"${_archon_sdl_min_version}\")"
  )
endif()
find_package_message(ARCHON_SDL_MSG "${_msg}" "[${SDL3_FOUND}][${_version}][${_archon_sdl_min_version}][${_library}]")


# Need GLEW for the following reasons:
# * Exposure of OpenGL to applciations through archon/display/opengl.hpp
#
find_package(GLEW QUIET)
if(GLEW_FOUND)
  set(_version "${GLEW_VERSION}")
  if("${_version}" STREQUAL "" AND DEFINED glew_VERSION)
    set(_version "${glew_VERSION}")
  endif()
  if("${_version}" STREQUAL "")
    set(_version "unknown")
  endif()
  get_target_property(_library GLEW::GLEW IMPORTED_LOCATION)
  if(NOT _library OR _library MATCHES "-NOTFOUND")
    get_target_property(_configs GLEW::GLEW IMPORTED_CONFIGURATIONS)
    if(_configs)
      list(GET _configs 0 _first_config)
      get_target_property(_library GLEW::GLEW IMPORTED_LOCATION_${_first_config})
    endif()
  endif()
  if(NOT _library OR _library MATCHES "-NOTFOUND")
    set(_library "unknown")
  endif()
  string(CONCAT _msg
    "Found GLEW: ${_library} (found version \"${_version}\")"
  )
else()
  set(_version "")
  set(_library "")
  string(CONCAT _msg
    "Could NOT find GLEW"
  )
endif()
find_package_message(ARCHON_GLEW_MSG "${_msg}" "[${GLEW_FOUND}][${_version}][${_library}]")


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


target_sources(Display PRIVATE
  archon/display/display_namespace.hpp
  archon/display/noinst/timestamp_unwrapper.hpp
  archon/display/noinst/mult_pixel_format.hpp
  archon/display/noinst/palette_map.hpp
  archon/display/noinst/impl_util.hpp
  archon/display/noinst/x11/support.hpp
)


target_sources(Display PUBLIC FILE_SET HEADERS FILES
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


set(ARCHON_DISPLAY_HAVE_X11 0)
set(ARCHON_DISPLAY_HAVE_X11_XKB 0)
set(ARCHON_DISPLAY_HAVE_X11_XINPUT2 0)
set(ARCHON_DISPLAY_HAVE_X11_XFIXES 0)
set(ARCHON_DISPLAY_HAVE_X11_XDBE 0)
set(ARCHON_DISPLAY_HAVE_X11_XRENDER 0)
set(ARCHON_DISPLAY_HAVE_X11_XRANDR 0)
set(ARCHON_DISPLAY_HAVE_X11_GLX 0)
if(ARCHON_GOOD_X11_FOUND)
  set(ARCHON_DISPLAY_HAVE_X11 1)
  if(X11_Xkb_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_XKB 1)
  endif()
  if(X11_Xi_FOUND)
    # Unfortunately, FindX11.cmake does not directly offer detection of Xinput2, so manual
    # detection is necessary
    find_path(ARCHON_XINPUT2_INCLUDE_PATH X11/extensions/XInput2.h ${X11_Xi_INCLUDE_PATH})
    if(ARCHON_XINPUT2_INCLUDE_PATH)
      set(ARCHON_DISPLAY_HAVE_X11_XINPUT2 1)
    endif()
  endif()
  if(X11_Xfixes_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_XFIXES 1)
  endif()
  if(X11_Xext_FOUND)
    # Unfortunately, Xdbe is not directly covered by FindX11.cmake until CMake 3.29.
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/25591.
    find_path(ARCHON_XDBE_INCLUDE_PATH X11/extensions/Xdbe.h ${X11_Xext_INCLUDE_PATH})
    if(ARCHON_XDBE_INCLUDE_PATH)
      set(ARCHON_DISPLAY_HAVE_X11_XDBE 1)
    endif()
  endif()
  if(X11_Xrender_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_XRENDER 1)
  endif()
  if(X11_Xrandr_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_XRANDR 1)
  endif()
  if(OpenGL_GLX_FOUND AND GLEW_FOUND)
    set(ARCHON_DISPLAY_HAVE_X11_GLX 1)
  endif()
endif()

set(ARCHON_DISPLAY_HAVE_SDL 0)
if(SDL3_FOUND)
  set(ARCHON_DISPLAY_HAVE_SDL 1)
endif()

set(ARCHON_DISPLAY_HAVE_OPENGL 0)
if(OPENGL_FOUND AND GLEW_FOUND)
  set(ARCHON_DISPLAY_HAVE_OPENGL 1)
endif()

configure_file(archon/display/impl/config.h.in archon/display/impl/config.h)

target_sources(Display PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/display/impl/config.h"
)


target_link_libraries(Display
  PUBLIC Core
  PUBLIC Log
  PUBLIC Math
  PUBLIC Util
  PUBLIC Image
)

if(ARCHON_GOOD_X11_FOUND)
  target_link_libraries(Display PRIVATE X11::X11)
  if(X11_Xi_FOUND)
    target_link_libraries(Display PRIVATE X11::Xi)
  endif()
  if(X11_Xfixes_FOUND)
    target_link_libraries(Display PRIVATE X11::Xfixes)
  endif()
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

if(SDL3_FOUND)
  target_link_libraries(Display PRIVATE SDL3::SDL3)
endif()

if(OPENGL_FOUND AND GLEW_FOUND)
  target_link_libraries(Display PUBLIC OpenGL::GL GLEW::GLEW)
endif()


install(TARGETS Display FILE_SET HEADERS)


add_subdirectory(archon/display/tool)

if(ARCHON_INCLUDE_DEMO_PROGS)
  add_subdirectory(archon/display/probe)
  add_subdirectory(archon/display/demo)
endif()

if(ARCHON_INCLUDE_TEST_SUITE)
  add_subdirectory(archon/display/test)
endif()
