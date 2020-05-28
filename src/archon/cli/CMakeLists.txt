add_subdirectory(test)
add_subdirectory(demo)

add_library(Cli INTERFACE)

target_link_libraries(Cli INTERFACE
  Core
  Log
)

install(TARGETS Cli)
