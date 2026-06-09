
# All Archon libraries
set(ARCHON_LIBRARIES Core Log Cli Check Math Util Image Font Gfx Display Render)

# The libraries that libraries and their tools depend on
set(ARCHON_LIB_DEPS_Core    "")
set(ARCHON_LIB_DEPS_Log     Core)
set(ARCHON_LIB_DEPS_Cli     Core Log)
set(ARCHON_LIB_DEPS_Check   Core Log Cli)
set(ARCHON_LIB_DEPS_Math    Core)
set(ARCHON_LIB_DEPS_Util    Core Math)
set(ARCHON_LIB_DEPS_Image   Core Log Util)
set(ARCHON_LIB_DEPS_Font    Core Log Cli Math Util Image)
set(ARCHON_LIB_DEPS_Gfx     Core Math Util)
set(ARCHON_LIB_DEPS_Display Core Log Cli Math Util Image)
set(ARCHON_LIB_DEPS_Render  Core Log Math Util Image Gfx Display)

# The libraries that library-specific probing and demo programs depend on
set(ARCHON_DEMO_DEPS_Core    Log Cli)
set(ARCHON_DEMO_DEPS_Log     "")
set(ARCHON_DEMO_DEPS_Cli     Core)
set(ARCHON_DEMO_DEPS_Check   "")
set(ARCHON_DEMO_DEPS_Math    "")
set(ARCHON_DEMO_DEPS_Util    "")
set(ARCHON_DEMO_DEPS_Image   Core Log Cli Math Util)
set(ARCHON_DEMO_DEPS_Font    Core Log Cli Math Util Image)
set(ARCHON_DEMO_DEPS_Gfx     "")
set(ARCHON_DEMO_DEPS_Display Core Log Cli Math Util Image)
set(ARCHON_DEMO_DEPS_Render  Core Log Cli Math Util Image Gfx Display)

# The libraries that library-specific test suites depend on
set(ARCHON_TEST_DEPS_Core    Log Check)
set(ARCHON_TEST_DEPS_Log     Core Check)
set(ARCHON_TEST_DEPS_Cli     Log Check)
set(ARCHON_TEST_DEPS_Check   Core)
set(ARCHON_TEST_DEPS_Math    Core Check)
set(ARCHON_TEST_DEPS_Util    Core Check Math)
set(ARCHON_TEST_DEPS_Image   Core Check Util)
set(ARCHON_TEST_DEPS_Font    Core Check Math Util Image)
set(ARCHON_TEST_DEPS_Gfx     "")
set(ARCHON_TEST_DEPS_Display Core Check)
set(ARCHON_TEST_DEPS_Render  "")

# The libraries that the test suite executor depend on
set(ARCHON_TEST_DEPS Core Check)


# Compute effective set of components to be built

set(ARCHON_INCLUDE_LIBRARIES "")
set(ARCHON_INCLUDE_DEMO_PROGS "")
set(ARCHON_INCLUDE_TEST_SUITE "")

if(ARCHON_BUILD_DEMO_PROGS OR ARCHON_BUILD_ALL)
  set(ARCHON_INCLUDE_DEMO_PROGS ON)
endif()
if(ARCHON_BUILD_TEST_SUITE OR ARCHON_BUILD_ALL)
  set(ARCHON_INCLUDE_TEST_SUITE ON)
endif()

macro(archon_include_library _lib)
  if(NOT "${_lib}" IN_LIST ARCHON_INCLUDE_LIBRARIES)
    list(APPEND ARCHON_INCLUDE_LIBRARIES "${_lib}")
    foreach(_dep IN LISTS ARCHON_LIB_DEPS_${_lib})
      archon_include_library("${_dep}")
    endforeach()
    if(ARCHON_INCLUDE_DEMO_PROGS)
      foreach(_dep IN LISTS ARCHON_DEMO_DEPS_${_lib})
        archon_include_library("${_dep}")
      endforeach()
    endif()
    if(ARCHON_INCLUDE_TEST_SUITE)
      foreach(_dep IN LISTS ARCHON_TEST_DEPS_${_lib})
        archon_include_library("${_dep}")
      endforeach()
    endif()
  endif()
endmacro()

foreach(_lib IN LISTS ARCHON_LIBRARIES)
  string(TOUPPER "${_lib}" _upper_lib)
  if(ARCHON_BUILD_${_upper_lib}_LIBRARY)
    archon_include_library("${_lib}")
  endif()
endforeach()

if(ARCHON_BUILD_ALL_LIBS OR ARCHON_BUILD_ALL)
  foreach(_lib IN LISTS ARCHON_LIBRARIES)
    archon_include_library("${_lib}")
  endforeach()
endif()

if(ARCHON_BUILD_TEST_SUITE)
  foreach(_dep IN LISTS ARCHON_TEST_DEPS)
    archon_include_library("${_dep}")
  endforeach()
endif()

foreach(_lib IN LISTS ARCHON_LIBRARIES)
  set("ARCHON_INCLUDE_LIBRARY_${_lib}" "")
  if("${_lib}" IN_LIST ARCHON_INCLUDE_LIBRARIES)
    set("ARCHON_INCLUDE_LIBRARY_${_lib}" ON)
  endif()
endforeach()


# If the Font library is included, PNG is a mandatory dependency

set(ARCHON_REQUIRE_PNG "")
if("Font" IN_LIST ARCHON_INCLUDE_LIBRARIES)
  set(ARCHON_REQUIRE_PNG "ON")
endif()
