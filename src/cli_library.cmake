add_library(Cli INTERFACE)

target_link_libraries(Cli INTERFACE
  Core
  Log
)

target_sources(Cli PUBLIC FILE_SET HEADERS FILES
  archon/cli/impl/call.hpp
  archon/cli/string_holder.hpp
  archon/cli/impl/value_formatter.hpp
  archon/cli/impl/value_parser.hpp
  archon/cli/proc_error.hpp
  archon/cli/error_handler.hpp
  archon/cli/logging_error_handler.hpp
  archon/cli/config.hpp
  archon/cli/spec_error.hpp
  archon/cli/help_spec_error.hpp
  archon/cli/exception.hpp
  archon/cli/option_actions.hpp
  archon/cli/attributes.hpp
  archon/cli/spec_support.hpp
  archon/cli/help_config.hpp
  archon/cli/impl/root_state.hpp
  archon/cli/impl/option_action.hpp
  archon/cli/impl/error_accum.hpp
  archon/cli/processor_fwd.hpp
  archon/cli/impl/option_occurrence.hpp
  archon/cli/impl/option_invocation.hpp
  archon/cli/command_line.hpp
  archon/cli/impl/pattern_symbol.hpp
  archon/cli/impl/pattern_structure.hpp
  archon/cli/impl/pattern_args_parser.hpp
  archon/cli/impl/pattern_func_checker.hpp
  archon/cli/impl/pattern_action.hpp
  archon/cli/impl/spec.hpp
  archon/cli/impl/nfa.hpp
  archon/cli/impl/nfa_builder.hpp
  archon/cli/impl/pattern_matcher.hpp
  archon/cli/spec.hpp
  archon/cli/impl/processor.hpp
  archon/cli/impl/help_formatter.hpp
  archon/cli/impl/spec_parser.hpp
  archon/cli/processor.hpp
  archon/cli/process.hpp
  archon/cli.hpp
)

install(TARGETS Cli FILE_SET HEADERS)

add_subdirectory(archon/cli/test)
#add_subdirectory(archon/cli/demo)
