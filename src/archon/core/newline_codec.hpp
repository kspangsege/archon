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

#ifndef ARCHON_X_CORE_X_NEWLINE_CODEC_HPP
#define ARCHON_X_CORE_X_NEWLINE_CODEC_HPP

/// \file


#include <cstddef>

#include <archon/core/span.hpp>


/// \brief Windows-style newline encoding and decoding (NL <-> CR LF).
///
/// This namespace provides function for encoding and decoding of newline characters
/// according to the style used by Microsoft Windows. Under this style, the encoded form of
/// a newline character (NL) is `carriage return` followed by `line feed` (CR LF).
///
namespace archon::core::newline_codec {


/// \brief Advance incremental newline decoding process.
///
/// This function is used to start, or to advance an incremental newline decoding
/// process. This process is a matter of copying the specified data into the specified
/// buffer while replacing all occurrences of CR LF with NL.
///
/// The decoding process stops when all the specified data has been decoded, or when the
/// specified buffer runs full. If the last byte in \p data is a CR, and if \p end_of_data
/// is `false`, processing stops immediately before that CR, unless the buffer runs full
/// prior to that. If \p end_of_data is `true`, a final CR will be copied into the buffer,
/// assuming that it is not a part of a CR-LF sequence.
///
/// \param data_offset On entry, this is the offset of the first byte in \p data to be
/// processed, usually zero. On exit, this is one plus the offset of the last byte that was
/// consumed. If no bytes were consumed, its value on exit will be equal to its value on
/// entry.
///
/// \param buffer_offset On entry, this is the offset of the first slot in \p buffer to be
/// filled with decoded characters, usually zero. On exit, this is one plus the offset of
/// the last character placed in the buffer. If no characters were placed in the buffer, its
/// value on exit will be equal to its value on entry.
///
/// \param clear_offset, clear If any newline decoding takes place, \p clear is set to the
/// sum of \p clear_offset and the position in the buffer following the last decoded newline
/// character. Otherwise, \p clear is left untouched.
///
void decode(core::Span<const char> data, std::size_t& data_offset, bool end_of_data, core::Span<char> buffer,
            std::size_t& buffer_offset, std::size_t clear_offset, std::size_t& clear) noexcept;


/// \brief Advance incremental newline encoding process.
///
/// This function is used to start, or to advance an incremental newline encoding
/// process. This process is a matter of copying the specified data into the specified
/// buffer while replacing all occurrences of NL with CR LF.
///
/// The encoding process stops when all the specified data has been encoded, or when the
/// specified buffer runs full. If, at some point during the encoding process, the next byte
/// in \p data is NL, and there is only space for one more byte in \p buffer, the encoding
/// process stops immediately before that NL.
///
/// \param data_offset On entry, this is the offset of the first byte in \p data to be
/// processed, usually zero. On exit, this is one plus the offset of the last byte that was
/// consumed. If no bytes were consumed, its value on exit will be equal to its value on
/// entry.
///
/// \param buffer_offset On entry, this is the offset of the first slot in \p buffer to be
/// filled with encoded characters, usually zero. On exit, this is one plus the offset of
/// the last byte placed in the buffer. If no bytes were placed in the buffer, its value on
/// exit will be equal to its value on entry.
///
void encode(core::Span<const char> data, std::size_t& data_offset, core::Span<char> buffer,
            std::size_t& buffer_offset) noexcept;


/// \brief Simulate newline decoding process.
///
/// This function can be used to "measure" how many bytes would be consumed by \ref decode()
/// given a particular amount of buffer space for the decoded characters.
///
/// Let N be the number of characters that would be decoded by \ref decode() given the same
/// data, the same initial data offset, `true` for `end_of_data`, and an "infinite" buffer.
///
/// If \p buffer_size is less than, or equal to N, `simul_decode()` will return `true` after
/// advancing \p data_offset by the same amount as \ref decode() would, given the same data,
/// `true` for `end_of_data`, and an amount of available buffer space equal to \p
/// buffer_size.
///
/// If \p buffer_size is greater than N, `simul_decode()` will return `false` and leave \p
/// data_offset unchanged.
///
/// This function is useful in application where the decoding process is generally ahead of
/// the logical read position (due to buffering, and for efficiency reasons). In such
/// applications, if the undecoded data is retained from a point that precedes the point
/// that corresponds to the logical read position, `simul_decode()` can be used to
/// efficiently calculate the position in the underlying byte sequence that corresponds to
/// the logical read position.
///
bool simul_decode(core::Span<const char> data, std::size_t& data_offset, std::size_t buffer_size) noexcept;


} // namespace archon::core::newline_codec

#endif // ARCHON_X_CORE_X_NEWLINE_CODEC_HPP
