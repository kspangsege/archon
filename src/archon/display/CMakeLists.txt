add_subdirectory(demo)

find_package(X11)
find_package(SDL2)

add_subdirectory(probe)

set(ARCHON_DISPLAY_HAVE_XLIB 0)
if (X11_FOUND)
  set(ARCHON_DISPLAY_HAVE_XLIB 1)
endif()

set(ARCHON_DISPLAY_HAVE_SDL 0)
if (SDL2_FOUND)
  set(ARCHON_DISPLAY_HAVE_SDL 1)
endif()

configure_file(impl/config.h.in impl/config.h)

add_library(Display
  event_handler.cpp
  implementation.cpp
  implementation_sdl.cpp
)

set_target_properties(Display PROPERTIES OUTPUT_NAME "archon-display")

target_link_libraries(Display PUBLIC
  Core
  Image
)

if (X11_FOUND)
  target_include_directories(Display PRIVATE ${X11_INCLUDE_DIR})
  target_link_libraries(Display PRIVATE ${X11_LIBRARIES})
endif()

if (SDL2_FOUND)
  target_include_directories(Display PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(Display PRIVATE ${SDL2_LIBRARIES})
endif()

install(TARGETS Display)
