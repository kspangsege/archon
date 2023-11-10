// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


// Do not include this header file. It exists only to specify the canonical header order,
// which is a topological dependency ordering of all the header files of the Archon Core
// Library, including any that must never be included by applications.
#error "Do not include this header file"


// Various foundational utilities
#include <archon/core/core_namespace.hpp>
#include <archon/core/impl/config.h>
#include <archon/core/features.h>
#include <archon/core/archon_version.hpp>
#include <archon/core/pair.hpp>
#include <archon/core/span.hpp>
#include <archon/core/type_list.hpp>
#include <archon/core/type.hpp>
#include <archon/core/concepts.hpp>
#include <archon/core/demangle.hpp>
#include <archon/core/formattable_value_ref.hpp>
#include <archon/core/terminate.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/impl/utility.hpp>
#include <archon/core/utility.hpp>
#include <archon/core/scope_exit.hpp>
#include <archon/core/value_reset_guard.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/integer_traits.hpp>
#include <archon/core/integer_concept.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float_traits.hpp>
#include <archon/core/float_concept.hpp>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/algorithm.hpp>
#include <archon/core/inexact_compare.hpp>
#include <archon/core/impl/type_ident_impl.hpp>
#include <archon/core/type_ident.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/time.hpp>

// Utility iterators
#include <archon/core/index_iterator.hpp>
#include <archon/core/proxy_iterator.hpp>
#include <archon/core/stride_iterator.hpp>

// Hashing
#include <archon/core/hash_fnv.hpp>
#include <archon/core/impl/hash.hpp>
#include <archon/core/hash.hpp>

// Containers
#include <archon/core/buffer.hpp>
#include <archon/core/buffer_contents.hpp>
#include <archon/core/string_buffer_contents.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/circular_buffer.hpp>
#include <archon/core/impl/vector_impl.hpp>
#include <archon/core/vector.hpp>
#include <archon/core/impl/flat_map_impl.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/flat_multimap.hpp>
#include <archon/core/flat_set.hpp>
#include <archon/core/literal_hash_map.hpp>
#include <archon/core/range_map.hpp>
#include <archon/core/frozen_sets.hpp>
#include <archon/core/typed_object_registry.hpp>

// Strings, streams, and formatting
#include <archon/core/char_mapper.hpp>
#include <archon/core/string.hpp>
#include <archon/core/var_string_ref.hpp>
#include <archon/core/char_codec_config.hpp>
#include <archon/core/char_codec_concept.hpp>
#include <archon/core/impl/codecvt_quirks.hpp>
#include <archon/core/impl/char_codec.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/newline_codec.hpp>
#include <archon/core/text_codec_impl_concept.hpp>
#include <archon/core/impl/text_codec_impl.hpp>
#include <archon/core/text_codec_impl.hpp>
#include <archon/core/text_codec.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/core/memory_input_stream.hpp>
#include <archon/core/memory_output_stream.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/stream_input.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/format.hpp>
#include <archon/core/integer_formatter.hpp>
#include <archon/core/integer_parser.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/timestamp_formatter.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_with.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/format_enc.hpp>
#include <archon/core/format_encoded.hpp>
#include <archon/core/with_modified_locale.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/quote.hpp>
#include <archon/core/hex_dump.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/string_template.hpp>
#include <archon/core/string_matcher.hpp>

// Build information and platform interface
#include <archon/core/random.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/timer.hpp>
#include <archon/core/endianness.hpp>
#include <archon/core/misc_error.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_mode.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/platform_info.hpp>
#include <archon/core/platform_support.hpp>
#include <archon/core/resource_limits.hpp>
#include <archon/core/signal_blocker.hpp>
#include <archon/core/thread.hpp>
#include <archon/core/thread_guard.hpp>

// Sources and sinks
#include <archon/core/source.hpp>
#include <archon/core/sink.hpp>
#include <archon/core/rewindable_source.hpp>

// Files and file system
#include <archon/core/file.hpp>
#include <archon/core/file_sink.hpp>
#include <archon/core/file_source.hpp>
#include <archon/core/text_file_config.hpp>
#include <archon/core/text_file_error.hpp>
#include <archon/core/text_file_impl_concept.hpp>
#include <archon/core/impl/prim_text_file_impl.hpp>
#include <archon/core/impl/text_file_impl.hpp>
#include <archon/core/buffered_text_file_impl_concept.hpp>
#include <archon/core/impl/buffered_text_file_impl.hpp>
#include <archon/core/text_file_impl.hpp>
#include <archon/core/text_file.hpp>
#include <archon/core/buffered_text_file.hpp>
#include <archon/core/text_file_stream.hpp>
#include <archon/core/terminal.hpp>

// Misc. utilities
#include <archon/core/mul_prec_int.hpp>
#include <archon/core/super_int.hpp>
#include <archon/core/base64.hpp>
#include <archon/core/histogram.hpp>
#include <archon/core/word_wrap.hpp>
#include <archon/core/text_formatter.hpp>
#include <archon/core/text_parser.hpp>
