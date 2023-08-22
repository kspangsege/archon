add_library(Core
  archon/core/demangle.cpp
  archon/core/terminate.cpp
  archon/core/time.cpp
  archon/core/newline_codec.cpp
  archon/core/random.cpp
  archon/core/locale.cpp
  archon/core/timer.cpp
  archon/core/misc_error.cpp
  archon/core/build_environment.cpp
  archon/core/platform_info.cpp
  archon/core/platform_support.cpp
  archon/core/resource_limits.cpp
  archon/core/thread.cpp
  archon/core/source.cpp
  archon/core/rewindable_source.cpp
  archon/core/filesystem.cpp
  archon/core/file.cpp
  archon/core/file_sink.cpp
  archon/core/file_source.cpp
  archon/core/text_file_error.cpp
  archon/core/impl/prim_text_file_impl.cpp
  archon/core/terminal.cpp
  archon/core/base64.cpp
  archon/core/word_wrap.cpp
)

set_target_properties(Core PROPERTIES OUTPUT_NAME "archon-core")

if(MSVC)
  # platform_info.cpp needs to link against GetFileVersionInfoSizeExW,
  # GetFileVersionInfoExW, and VerQueryValueW.
  target_link_libraries(Core version.lib)
endif()

file(RELATIVE_PATH "ARCHON_SOURCE_FROM_BUILD_PATH" "${ARCHON_BUILD_ROOT}" "${ARCHON_SOURCE_ROOT}")

set(ARCHON_ASSUME_VISUAL_STUDIO_CMAKE_GENERATOR 0)
if (CMAKE_GENERATOR MATCHES "^Visual Studio")
  set(ARCHON_ASSUME_VISUAL_STUDIO_CMAKE_GENERATOR 1)
endif()

configure_file(archon/core/impl/config.h.in archon/core/impl/config.h)

target_sources(Core PUBLIC FILE_SET HEADERS BASE_DIRS "${ARCHON_BUILD_ROOT}" "${ARCHON_SOURCE_ROOT}" FILES
  "${CMAKE_CURRENT_BINARY_DIR}/archon/core/impl/config.h"
  archon/core/features.h
  archon/core/archon_version.hpp
  archon/core/span.hpp
  archon/core/type_list.hpp
  archon/core/type.hpp
  archon/core/demangle.hpp
  archon/core/formattable_value_ref.hpp
  archon/core/terminate.hpp
  archon/core/assert.hpp
  archon/core/impl/utility.hpp
  archon/core/utility.hpp
  archon/core/is_constexpr.hpp
  archon/core/scope_exit.hpp
  archon/core/value_reset_guard.hpp
  archon/core/string_span.hpp
  archon/core/integer_traits.hpp
  archon/core/integer.hpp
  archon/core/float_traits.hpp
  archon/core/float.hpp
  archon/core/math.hpp
  archon/core/algorithm.hpp
  archon/core/inexact_compare.hpp
  archon/core/impl/type_ident_impl.hpp
  archon/core/type_ident.hpp
  archon/core/memory.hpp
  archon/core/time.hpp
  archon/core/index_iterator.hpp
  archon/core/proxy_iterator.hpp
  archon/core/stride_iterator.hpp
  archon/core/hash_fnv.hpp
  archon/core/impl/hash.hpp
  archon/core/hash.hpp
  archon/core/buffer.hpp
  archon/core/buffer_contents.hpp
  archon/core/string_buffer_contents.hpp
  archon/core/array_seeded_buffer.hpp
  archon/core/circular_buffer.hpp
  archon/core/impl/vector_impl.hpp
  archon/core/vector.hpp
  archon/core/impl/flat_map_impl.hpp
  archon/core/flat_map.hpp
  archon/core/flat_multimap.hpp
  archon/core/flat_set.hpp
  archon/core/literal_hash_map.hpp
  archon/core/range_map.hpp
  archon/core/frozen_sets.hpp
  archon/core/typed_object_registry.hpp
  archon/core/char_mapper.hpp
  archon/core/string.hpp
  archon/core/var_string_ref.hpp
  archon/core/char_codec_config.hpp
  archon/core/impl/codecvt_quirks.hpp
  archon/core/impl/char_codec.hpp
  archon/core/char_codec.hpp
  archon/core/newline_codec.hpp
  archon/core/impl/text_codec_impl.hpp
  archon/core/text_codec_impl.hpp
  archon/core/text_codec.hpp
  archon/core/string_codec.hpp
  archon/core/memory_input_stream.hpp
  archon/core/memory_output_stream.hpp
  archon/core/seed_memory_output_stream.hpp
  archon/core/stream_input.hpp
  archon/core/stream_output.hpp
  archon/core/format.hpp
  archon/core/integer_formatter.hpp
  archon/core/integer_parser.hpp
  archon/core/value_formatter.hpp
  archon/core/value_parser.hpp
  archon/core/timestamp_formatter.hpp
  archon/core/as_int.hpp
  archon/core/as_list.hpp
  archon/core/format_with.hpp
  archon/core/format_as.hpp
  archon/core/format_enc.hpp
  archon/core/format_encoded.hpp
  archon/core/with_modified_locale.hpp
  archon/core/enum.hpp
  archon/core/quote.hpp
  archon/core/hex_dump.hpp
  archon/core/string_formatter.hpp
  archon/core/string_template.hpp
  archon/core/string_matcher.hpp
  archon/core/random.hpp
  archon/core/locale.hpp
  archon/core/timer.hpp
  archon/core/endianness.hpp
  archon/core/misc_error.hpp
  archon/core/filesystem.hpp
  archon/core/build_mode.hpp
  archon/core/build_environment.hpp
  archon/core/platform_info.hpp
  archon/core/platform_support.hpp
  archon/core/resource_limits.hpp
  archon/core/signal_blocker.hpp
  archon/core/thread.hpp
  archon/core/thread_guard.hpp
  archon/core/source.hpp
  archon/core/sink.hpp
  archon/core/rewindable_source.hpp
  archon/core/file.hpp
  archon/core/file_sink.hpp
  archon/core/file_source.hpp
  archon/core/text_file_config.hpp
  archon/core/text_file_error.hpp
  archon/core/impl/prim_text_file_impl.hpp
  archon/core/impl/text_file_impl.hpp
  archon/core/impl/buffered_text_file_impl.hpp
  archon/core/text_file_impl.hpp
  archon/core/text_file.hpp
  archon/core/buffered_text_file.hpp
  archon/core/text_file_stream.hpp
  archon/core/terminal.hpp
  archon/core/mul_prec_int.hpp
  archon/core/super_int.hpp
  archon/core/base64.hpp
  archon/core/histogram.hpp
  archon/core/word_wrap.hpp
  archon/core/text_formatter.hpp
  archon/core/text_parser.hpp
)

install(TARGETS Core FILE_SET HEADERS)

add_subdirectory(archon/core/test)
add_subdirectory(archon/core/demo)
