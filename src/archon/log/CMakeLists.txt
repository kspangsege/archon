add_library(Log
  log_level.cpp
)

set_target_properties(Log PROPERTIES OUTPUT_NAME "archon-log")

target_link_libraries(Log PUBLIC
  Core
)

install(TARGETS Log)
