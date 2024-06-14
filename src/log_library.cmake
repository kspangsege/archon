add_library(Log
  archon/log/log_level.cpp
)

set_target_properties(Log PROPERTIES OUTPUT_NAME "archon-log")

target_link_libraries(Log PUBLIC
  Core
)

target_sources(Log PUBLIC FILE_SET HEADERS FILES
  archon/log/log_level.hpp
  archon/log/limit.hpp
  archon/log/prefix.hpp
  archon/log/sink.hpp
  archon/log/channel.hpp
  archon/log/channel_map.hpp
  archon/log/logger.hpp
  archon/log/stream_logger.hpp
  archon/log/timestamp_logger.hpp
  archon/log/prefix_logger.hpp
  archon/log/limit_logger.hpp
  archon/log/channel_logger.hpp
  archon/log/duplicating_logger.hpp
  archon/log/jail_logger.hpp
  archon/log/impl/encoding_logger_impl.hpp
  archon/log/encoding_logger.hpp
  archon/log/log.hpp
  archon/log.hpp
)

install(TARGETS Log FILE_SET HEADERS)

add_subdirectory(archon/log/test)
