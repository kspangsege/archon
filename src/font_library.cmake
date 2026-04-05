include(FindPackageMessage)

add_library(Font
  archon/font/face.cpp
  archon/font/loader.cpp
  archon/font/loader_fallback.cpp
  archon/font/loader_freetype.cpp
)

set_target_properties(Font PROPERTIES OUTPUT_NAME "archon-font")

target_link_libraries(Font PUBLIC
  Core
  Util
  Image
)

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

set(ARCHON_FONT_HAVE_FREETYPE 0)
if(Freetype_FOUND)
  set(ARCHON_FONT_HAVE_FREETYPE 1)
  target_link_libraries(Font PRIVATE Freetype::Freetype)
endif()

configure_file(archon/font/impl/config.h.in archon/font/impl/config.h)

target_sources(Font PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" "${ARCHON_SOURCE_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/font/impl/config.h"
  archon/font/size.hpp
  archon/font/code_point.hpp
  archon/font/face.hpp
  archon/font/loader.hpp
  archon/font/loader_fallback.hpp
  archon/font/loader_freetype.hpp
  archon/font.hpp
)

install(TARGETS Font FILE_SET HEADERS)

add_subdirectory(archon/font/test)
add_subdirectory(archon/font/tool)
add_subdirectory(archon/font/demo)
