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

#ifndef ARCHON_X_CORE_X_IMPL_X_CODECVT_QUIRKS_HPP
#define ARCHON_X_CORE_X_IMPL_X_CODECVT_QUIRKS_HPP


#include <archon/core/features.h>


namespace archon::core::impl {


// std::codecvt::in() and std::codecvt::out() may report an `ok` result when the size of the
// specified output buffer is zero, even when presented with a nonzero amount of input. See
// also https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37475.
#if ARCHON_GNU_LIBCXX
inline constexpr bool codecvt_quirk_ok_result_on_zero_size_buffer = true;
#else
inline constexpr bool codecvt_quirk_ok_result_on_zero_size_buffer = false;
#endif

// std::codecvt::in() reports a `partial` result, rather than an `ok` result when presented
// with an incomplete byte sequence (regardless of whether none, some, or all of the
// incomplete byte sequence has been consumed), and there is enough available space in the
// output buffer to decode another character.
#if ARCHON_MSVC_LIBCXX
inline constexpr bool codecvt_quirk_partial_result_on_partial_char = true;
#else
inline constexpr bool codecvt_quirk_partial_result_on_partial_char = false;
#endif

// std::codecvt::in() reports a partial result, rather than an error when presented with an
// invalid byte sequence.
#if ARCHON_LLVM_LIBCXX
inline constexpr bool codecvt_quirk_partial_result_on_invalid_byte_seq = true;
#else
inline constexpr bool codecvt_quirk_partial_result_on_invalid_byte_seq = false;
#endif

// When the presented part of the input ends part way through a valid byte sequence, and the
// output buffer is not full, the presented part of the input is consumed.
#if ARCHON_GNU_LIBCXX || ARCHON_LLVM_LIBCXX
inline constexpr bool codecvt_quirk_consume_partial_char = true;
#else
inline constexpr bool codecvt_quirk_consume_partial_char = false;
#endif

// Even though partial byte sequences are generally consumed, leading valid bytes of an
// invalid byte sequence are not consumed when the presented part of the input contains
// enough bytes to expose the invalidity.
#if ARCHON_GNU_LIBCXX || ARCHON_LLVM_LIBCXX
inline constexpr bool codecvt_quirk_consume_partial_char_but_not_good_bytes_on_error = true;
#else
inline constexpr bool codecvt_quirk_consume_partial_char_but_not_good_bytes_on_error = false;
#endif


static_assert(!codecvt_quirk_consume_partial_char_but_not_good_bytes_on_error ||
              codecvt_quirk_consume_partial_char);


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_CHAR_CODEC_HPP
