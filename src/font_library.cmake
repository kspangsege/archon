include(FindPackageMessage)

# NOTE: Keep this version in sync with the runtime version check in
# font/freetype_implementation.cpp, and with the documented dependency version requirements.
set(_min_version "2.10")
find_package(Freetype ${_min_version} QUIET)
set(_version "")
set(_library "")
if(Freetype_FOUND)
  if(Freetype_VERSION)
    set(_version "${Freetype_VERSION}")
  elseif(FREETYPE_VERSION_STRING)
    set(_version "${FREETYPE_VERSION_STRING}")
  endif()
  if(Freetype_DIR)
    set(_library "${Freetype_DIR}")
  else()
    set(_library "${FREETYPE_LIBRARY}")
  endif()
  string(CONCAT _msg
    "Found FreeType: ${_library} (found suitable version \"${_version}\", minimum required is \"${_min_version}\")"
  )
else()
  string(CONCAT _msg
    "Could NOT find FreeType (minimum required version is \"${_min_version}\")"
  )
endif()
find_package_message(ARCHON_FREETYPE_MSG "${_msg}" "[${_version}][${_min_version}][${_library}]")


add_library(Font
  archon/font/face.cpp
  archon/font/loader.cpp
  archon/font/implementation.cpp
  archon/font/fallback_implementation.cpp
  archon/font/freetype_implementation.cpp
  archon/font/list_implementations.cpp
  archon/font/list_font_faces.cpp
)

set_target_properties(Font PROPERTIES OUTPUT_NAME "archon-font")


target_sources(Font PUBLIC FILE_SET HEADERS FILES
  archon/font/size.hpp
  archon/font/code_point.hpp
  archon/font/face.hpp
  archon/font/loader.hpp
  archon/font/implementation.hpp
  archon/font/fallback_implementation.hpp
  archon/font/freetype_implementation.hpp
  archon/font/list_implementations.hpp
  archon/font/list_font_faces.hpp
  archon/font.hpp
)


set(ARCHON_FONT_HAVE_FREETYPE 0)
if(Freetype_FOUND)
  set(ARCHON_FONT_HAVE_FREETYPE 1)
endif()

configure_file(archon/font/impl/config.h.in archon/font/impl/config.h)

target_sources(Font PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/font/impl/config.h"
)


target_link_libraries(Font
  PUBLIC Core
  PUBLIC Log
  PUBLIC Math
  PRIVATE Util
  PUBLIC Image
)

if(Freetype_FOUND)
  target_link_libraries(Font PRIVATE Freetype::Freetype)
endif()


install(TARGETS Font FILE_SET HEADERS)


add_subdirectory(archon/font/tool)

if(ARCHON_BUILD_DEMO_PROGS)
  add_subdirectory(archon/font/demo)
endif()

if(ARCHON_BUILD_TEST_SUITE)
  add_subdirectory(archon/font/test)
endif()
