configure_file(config.h.in config.h)

add_library(Base
            demangle.cpp
            terminate.cpp
            base64.cpp
            platform_support.cpp
            platform_info.cpp
            resource_limits.cpp
            locale.cpp
            time.cpp
            random.cpp
            terminal.cpp
            file.cpp
            logger.cpp
            timer.cpp
            thread.cpp
            build_environment.cpp)

set_target_properties(Base PROPERTIES
                      OUTPUT_NAME "archon-base")

if(MSVC)
  # platform_info.cpp needs to link against GetFileVersionInfoSizeExW,
  # GetFileVersionInfoExW, and VerQueryValueW.
  target_link_libraries(Base version.lib)
endif()
