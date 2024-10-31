// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_UNICODE_HPP
#define ARCHON_X_CORE_X_UNICODE_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <stdexcept>
#include <string>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>


namespace archon::core {


/// \brief Produce UTF-8 encoding of UCS string.
///
/// This function produces the UTF-8 encoding of the specified UCS string (\p string). The
/// UTF-8 encoding is stored in the specified buffer (\p buffer) starting at the specified
/// position (\p buffer_offset). The buffer will be expanded as required (using a
/// progressive expansion scheme).
///
/// This function has the same effect as \ref core::try_encode_utf8(), except that it throws
/// instead of returning `false` if the string contains invalid code points.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::encode_utf8_l(), \ref core::encode_utf8_a(), \ref
/// core::try_encode_utf8(), \ref core::encode_utf8_incr()
///
/// \sa \ref core::decode_utf8(), \ref core::encode_utf16(), \ref core::utf16_to_utf8()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void encode_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Leniently produce UTF-8 encoding of UCS string.
///
/// This function produces the UTF-8 encoding of the specified UCS string (\p string) in a
/// lenient manner, which means that any invalid code point in the string is dealt with as
/// if it was the Unicode replacement character.
///
/// This function is implemented in terms of \ref core::encode_utf8_a().
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::encode_utf8(), \ref core::encode_utf8_a()
///
/// \sa \ref core::decode_utf8_l(), \ref core::encode_utf16_l(), \ref
/// core::utf16_to_utf8_l()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void encode_utf8_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Produce UTF-8 encoding of portion of UCS string.
///
/// This function produces a UTF-8 encoding of a portion of the specified string (\p string)
/// starting at the specified offset (\p string_offset) and ending when the end of the
/// string is reached, or when an invalid UCS code point is encountered. The produced UTF-8
/// encoding is stored in the specified buffer (\p buffer) starting at the specified offset
/// (\p buffer_offset). The buffer will be expanded as necessary (using a progressive
/// expansion scheme).
///
/// The invocation `encode_utf8_a(string, string_offset, buffer, buffer_offset)` has the
/// same effect as `try_encode_utf8(string.subspan(string_offset), buffer, buffer_offset)`,
/// except that when an invalid code point is encountered, the former invocation, i.e., this
/// function sets \p string_offset to point to the invalid code point, and \p buffer_offset
/// to point to the corresponding position in the output buffer. This allows the caller to
/// decide what to do about the invalid code point, and, if desired, to resume the encoding
/// process by skipping over some input.
///
/// Upon return, if \p string_offset is equal to `string.size()`, the encoding process
/// completed. Otherwise, it stopped because of an invalid code point.
///
/// If this function throws, \p string_offset may, or may not have been advanced, but if it
/// has been advanced, \p buffer_offset will have been advanced accordingly. If \p
/// string_offset was not advanced, \p buffer_offset will also not have been advanced. Also,
/// the buffer may have been expanded, and the buffer contents beyond \p buffer_offset may
/// have been clobbered.
///
/// Behavior is undefined if, prior to the invocation, \p string_size is greater than
/// `string.size()` or \p buffer_offset is greater than `buffer.size()`.
///
/// This function is implemented in terms of \ref core::encode_utf8_incr().
///
/// \sa \ref core::encode_utf8(), \ref core::encode_utf8_l(), \ref core::try_encode_utf8(),
/// \ref core::encode_utf8_incr()
///
/// \sa \ref core::decode_utf8_a(), \ref core::encode_utf16_a(), \ref
/// core::utf16_to_utf8_a()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void encode_utf8_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                   std::size_t& buffer_offset);


/// \brief Recover UCS string from UTF-8 encoding.
///
/// This function recovers the UCS string from the specified UTF-8 encoding (\p string). The
/// recovered string is stored in the specified buffer (\p buffer) starting at the specified
/// position (\p buffer_offset). The buffer will be expanded as required (using a
/// progressive expansion scheme).
///
/// This function has the same effect as \ref core::try_decode_utf8(), except that it throws
/// instead of returning `false` if the specified string contains an invalid UTF-8 sequence.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::decode_utf8_l(), \ref core::decode_utf8_a(), \ref
/// core::try_decode_utf8(), \ref core::decode_utf8_incr()
///
/// \sa \ref core::encode_utf8(), \ref core::decode_utf16(), \ref core::utf8_to_utf16()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void decode_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Leniently recover UCS string from UTF-8 encoding.
///
/// This function recovers the UCS string from the specified UTF-8 encoding (\p string) in a
/// lenient manner, which means that any invalid UTF-8 sequence in the specified string
/// yields a Unicode replacement character in the resulting string.
///
/// This function is implemented in terms of \ref core::decode_utf8_a() and \ref
/// core::resync_utf8().
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::decode_utf8(), \ref core::decode_utf8_a(), \ref core::resync_utf8()
///
/// \sa \ref core::encode_utf8_l(), \ref core::decode_utf16_l(), \ref
/// core::utf8_to_utf16_l()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void decode_utf8_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Recover UCS string from portion of UTF-8 encoding.
///
/// This function recovers the UCS string from a portion of the specified UTF-8 encoding (\p
/// string) starting at the specified offset (\p string_offset) and ending when the end of
/// the UTF-8 encoding is reached, or when an invalid UTF-8 sequence is encountered. The
/// recovered UCS string is stored in the specified buffer (\p buffer) starting at the
/// specified offset (\p buffer_offset). The buffer will be expanded as necessary (using a
/// progressive expansion scheme).
///
/// The invocation `decode_utf8_a(string, string_offset, buffer, buffer_offset)` has the
/// same effect as `try_decode_utf8(string.subspan(string_offset), buffer, buffer_offset)`,
/// except that when an invalid UTF-8 sequence is encountered, the former invocation, i.e.,
/// this function sets \p string_offset to point to the invalid UTF-8 sequence, and \p
/// buffer_offset to point to the corresponding position in the output buffer. This allows
/// the caller to decide what to do about the invalid sequence, and, if desired, to resume
/// the decoding process by skipping over some input (see \ref core::resync_utf8()).
///
/// Upon return, if \p string_offset is equal to `string.size()`, the decoding process
/// completed. Otherwise, it stopped because of an invalid UTF-8 sequence. An incomplete
/// UTF-8 sequence at the end of the specified string will be considered invalid.
///
/// If this function throws, \p string_offset may, or may not have been advanced, but if it
/// has been advanced, \p buffer_offset will have been advanced accordingly. If \p
/// string_offset was not advanced, \p buffer_offset will also not have been advanced. Also,
/// the buffer may have been expanded, and the buffer contents beyond \p buffer_offset may
/// have been clobbered.
///
/// Behavior is undefined if, prior to the invocation, \p string_size is greater than
/// `string.size()` or \p buffer_offset is greater than `buffer.size()`.
///
/// This function is implemented in terms of \ref core::decode_utf8_incr().
///
/// \sa \ref core::decode_utf8(), \ref core::decode_utf8_l(), \ref core::try_decode_utf8(),
/// \ref core::decode_utf8_incr(), \ref core::resync_utf8()
///
/// \sa \ref core::encode_utf8_a(), \ref core::decode_utf16_a(), \ref
/// core::utf8_to_utf16_a()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void decode_utf8_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                   std::size_t& buffer_offset);


/// \brief Produce UTF-16 encoding of UCS string.
///
/// This function produces the UTF-16 encoding of the specified UCS string (\p string). The
/// UTF-16 encoding is stored in the specified buffer (\p buffer) starting at the specified
/// position (\p buffer_offset). The buffer will be expanded as required (using a
/// progressive expansion scheme).
///
/// This function has the same effect as \ref core::try_encode_utf16(), except that it
/// throws instead of returning `false` if the string contains invalid code points.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::encode_utf16_l(), \ref core::encode_utf16_a(), \ref
/// core::try_encode_utf16(), \ref core::encode_utf16_incr()
///
/// \sa \ref core::decode_utf16(), \ref core::encode_utf8(), \ref core::utf8_to_utf16()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void encode_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Leniently produce UTF-16 encoding of UCS string.
///
/// This function produces the UTF-16 encoding of the specified UCS string (\p string) in a
/// lenient manner, which means that any invalid code point in the string is dealt with as
/// if it was the Unicode replacement character.
///
/// This function is implemented in terms of \ref core::encode_utf16_a().
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::encode_utf16(), \ref core::encode_utf16_a()
///
/// \sa \ref core::decode_utf16_l(), \ref core::encode_utf8_l(), \ref
/// core::utf8_to_utf16_l()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void encode_utf16_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Produce UTF-16 encoding of portion of UCS string.
///
/// This function produces a UTF-16 encoding of a portion of the specified string (\p
/// string) starting at the specified offset (\p string_offset) and ending when the end of
/// the string is reached, or when an invalid UCS code point is encountered. The produced
/// UTF-16 encoding is stored in the specified buffer (\p buffer) starting at the specified
/// offset (\p buffer_offset). The buffer will be expanded as necessary (using a progressive
/// expansion scheme).
///
/// The invocation `encode_utf16_a(string, string_offset, buffer, buffer_offset)` has the
/// same effect as `try_encode_utf16(string.subspan(string_offset), buffer, buffer_offset)`,
/// except that when an invalid code point is encountered, the former invocation, i.e., this
/// function sets \p string_offset to point to the invalid code point, and \p buffer_offset
/// to point to the corresponding position in the output buffer. This allows the caller to
/// decide what to do about the invalid code point, and, if desired, to resume the encoding
/// process by skipping over some input.
///
/// Upon return, if \p string_offset is equal to `string.size()`, the encoding process
/// completed. Otherwise, it stopped because of an invalid code point.
///
/// If this function throws, \p string_offset may, or may not have been advanced, but if it
/// has been advanced, \p buffer_offset will have been advanced accordingly. If \p
/// string_offset was not advanced, \p buffer_offset will also not have been advanced. Also,
/// the buffer may have been expanded, and the buffer contents beyond \p buffer_offset may
/// have been clobbered.
///
/// Behavior is undefined if, prior to the invocation, \p string_size is greater than
/// `string.size()` or \p buffer_offset is greater than `buffer.size()`.
///
/// This function is implemented in terms of \ref core::encode_utf16_incr().
///
/// \sa \ref core::encode_utf16(), \ref core::encode_utf16_l(), \ref
/// core::try_encode_utf16(), \ref core::encode_utf16_incr()
///
/// \sa \ref core::decode_utf16_a(), \ref core::encode_utf8_a(), \ref
/// core::utf8_to_utf16_a()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void encode_utf16_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                    std::size_t& buffer_offset);


/// \brief Recover UCS string from UTF-16 encoding.
///
/// This function recovers the UCS string from the specified UTF-16 encoding (\p
/// string). The recovered string is stored in the specified buffer (\p buffer) starting at
/// the specified position (\p buffer_offset). The buffer will be expanded as required
/// (using a progressive expansion scheme).
///
/// This function does not recognize a UTF-16 byte order mark (BOM). It is the
/// responsibility of the application to ensure its absence.
///
/// This function has the same effect as \ref core::try_decode_utf16(), except that it
/// throws instead of returning `false` if the specified string contains an invalid UTF-16
/// sequence.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::decode_utf16_l(), \ref core::decode_utf16_a(), \ref
/// core::try_decode_utf16(), \ref core::decode_utf16_incr()
///
/// \sa \ref core::encode_utf16(), \ref core::decode_utf8(), \ref core::utf16_to_utf8()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void decode_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Leniently recover UCS string from UTF-16 encoding.
///
/// This function recovers the UCS string from the specified UTF-16 encoding (\p string) in
/// a lenient manner, which means that any invalid UTF-16 sequence in the specified string
/// yields a Unicode replacement character in the resulting string.
///
/// This function does not recognize a UTF-16 byte order mark (BOM). It is the
/// responsibility of the application to ensure its absence.
///
/// This function is implemented in terms of \ref core::decode_utf16_a() and \ref
/// core::resync_utf16().
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::decode_utf16(), \ref core::decode_utf16_a(), \ref core::resync_utf16()
///
/// \sa \ref core::encode_utf16_l(), \ref core::decode_utf8_l(), \ref
/// core::utf16_to_utf8_l()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void decode_utf16_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Recover UCS string from portion of UTF-16 encoding.
///
/// This function recovers the UCS string from a portion of the specified UTF-16 encoding
/// (\p string) starting at the specified offset (\p string_offset) and ending when the end
/// of the UTF-16 encoding is reached, or when an invalid UTF-16 sequence is
/// encountered. The recovered UCS string is stored in the specified buffer (\p buffer)
/// starting at the specified offset (\p buffer_offset). The buffer will be expanded as
/// necessary (using a progressive expansion scheme).
///
/// This function does not recognize a UTF-16 byte order mark (BOM). It is the
/// responsibility of the application to ensure its absence.
///
/// The invocation `decode_utf16_a(string, string_offset, buffer, buffer_offset)` has the
/// same effect as `try_decode_utf16(string.subspan(string_offset), buffer, buffer_offset)`,
/// except that when an invalid UTF-16 sequence is encountered, the former invocation, i.e.,
/// this function sets \p string_offset to point to the invalid UTF-16 sequence, and \p
/// buffer_offset to point to the corresponding position in the output buffer. This allows
/// the caller to decide what to do about the invalid sequence, and, if desired, to resume
/// the decoding process by skipping over some input (see \ref core::resync_utf16()).
///
/// Upon return, if \p string_offset is equal to `string.size()`, the decoding process
/// completed. Otherwise, it stopped because of an invalid UTF-16 sequence. An incomplete
/// UTF-16 sequence at the end of the specified string will be considered invalid.
///
/// If this function throws, \p string_offset may, or may not have been advanced, but if it
/// has been advanced, \p buffer_offset will have been advanced accordingly. If \p
/// string_offset was not advanced, \p buffer_offset will also not have been advanced. Also,
/// the buffer may have been expanded, and the buffer contents beyond \p buffer_offset may
/// have been clobbered.
///
/// Behavior is undefined if, prior to the invocation, \p string_size is greater than
/// `string.size()` or \p buffer_offset is greater than `buffer.size()`.
///
/// This function is implemented in terms of \ref core::decode_utf16_incr().
///
/// \sa \ref core::decode_utf16(), \ref core::decode_utf16_l(), \ref
/// core::try_decode_utf16(), \ref core::decode_utf16_incr(), \ref core::resync_utf16()
///
/// \sa \ref core::encode_utf16_a(), \ref core::decode_utf8_a(), \ref
/// core::utf16_to_utf8_a()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void decode_utf16_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                    std::size_t& buffer_offset);


/// \brief Transcode string from UTF-8 to UTF-16.
///
/// By way of a transcoding operation, this function produces the UTF-16 encoding of the
/// string that has the specified UTF-8 encoding (\p string). The produced UTF-16 encoding
/// is stored in the specified buffer (\p buffer) starting at the specified position (\p
/// buffer_offset). The buffer will be expanded as required (using a progressive expansion
/// scheme).
///
/// This function has the same effect as \ref core::try_utf8_to_utf16(), except that it
/// throws instead of returning `false` if the specified string contains an invalid UTF-8
/// sequence.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::utf8_to_utf16_l(), \ref core::utf8_to_utf16_a(), \ref
/// core::try_utf8_to_utf16(), \ref core::utf8_to_utf16_incr()
///
/// \sa \ref core::utf16_to_utf8(), \ref core::decode_utf8(), \ref core::encode_utf16()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void utf8_to_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Leniently transcode string from UTF-8 to UTF-16.
///
/// By way of a transcoding operation, this function produces the UTF-16 encoding of the
/// string that has the specified UTF-8 encoding (\p string), and it does so in a lenient
/// manner. This means that any invalid UTF-8 sequence in the specified string will be
/// handled as if it was the valid encoding of the Unicode replacement character.
///
/// This function is implemented in terms of \ref core::decode_utf8_a() and \ref
/// core::resync_utf8().
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::utf8_to_utf16(), \ref core::utf8_to_utf16_a(), \ref core::resync_utf8()
///
/// \sa \ref core::utf16_to_utf8_l(), \ref core::decode_utf8_l(), \ref
/// core::encode_utf16_l()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void utf8_to_utf16_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Transcode portion of UTF-8 encoding to UTF-16.
///
/// This function transcodes a portion of the specified UTF-8 encoding (\p string) to UTF-16
/// starting at the specified offset (\p string_offset) and ending when the end of the UTF-8
/// encoding is reached, or when an invalid UTF-8 sequence is encountered. The produced
/// UTF-16 encoding is stored in the specified buffer (\p buffer) starting at the specified
/// offset (\p buffer_offset). The buffer will be expanded as necessary (using a progressive
/// expansion scheme).
///
/// The invocation `utf8_to_utf16_a(string, string_offset, buffer, buffer_offset)` has the
/// same effect as `try_utf8_to_utf16(string.subspan(string_offset), buffer,
/// buffer_offset)`, except that when an invalid UTF-8 sequence is encountered, the former
/// invocation, i.e., this function sets \p string_offset to point to the invalid UTF-8
/// sequence, and \p buffer_offset to point to the corresponding position in the output
/// buffer. This allows the caller to decide what to do about the invalid sequence, and, if
/// desired, to resume the transcoding process by skipping over some input (see \ref
/// core::resync_utf8()).
///
/// Upon return, if \p string_offset is equal to `string.size()`, the transcoding process
/// completed. Otherwise, it stopped because of an invalid UTF-8 sequence. An incomplete
/// UTF-8 sequence at the end of the specified string will be considered invalid.
///
/// If this function throws, \p string_offset may, or may not have been advanced, but if it
/// has been advanced, \p buffer_offset will have been advanced accordingly. If \p
/// string_offset was not advanced, \p buffer_offset will also not have been advanced. Also,
/// the buffer may have been expanded, and the buffer contents beyond \p buffer_offset may
/// have been clobbered.
///
/// Behavior is undefined if, prior to the invocation, \p string_size is greater than
/// `string.size()` or \p buffer_offset is greater than `buffer.size()`.
///
/// This function is implemented in terms of \ref core::utf8_to_utf16_incr().
///
/// \sa \ref core::utf8_to_utf16(), \ref core::utf8_to_utf16_l(), \ref
/// core::try_utf8_to_utf16(), \ref core::utf8_to_utf16_incr(), \ref core::resync_utf8()
///
/// \sa \ref core::utf16_to_utf8_a(), \ref core::decode_utf8_a(), \ref
/// core::encode_utf16_a()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void utf8_to_utf16_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                     std::size_t& buffer_offset);


/// \brief Transcode string from UTF-16 to UTF-8.
///
/// By way of a transcoding operation, this function produces the UTF-8 encoding of the
/// string that has the specified UTF-16 encoding (\p string). The produced UTF-8 encoding
/// is stored in the specified buffer (\p buffer) starting at the specified position (\p
/// buffer_offset). The buffer will be expanded as required (using a progressive expansion
/// scheme).
///
/// This function does not recognize a UTF-16 byte order mark (BOM). It is the
/// responsibility of the application to ensure its absence.
///
/// This function has the same effect as \ref core::try_utf16_to_utf8(), except that it
/// throws instead of returning `false` if the specified string contains an invalid UTF-16
/// sequence.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::utf16_to_utf8_l(), \ref core::utf16_to_utf8_a(), \ref
/// core::try_utf16_to_utf8(), \ref core::utf16_to_utf8_incr()
///
/// \sa \ref core::utf8_to_utf16(), \ref core::decode_utf16(), \ref core::encode_utf8()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void utf16_to_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Leniently transcode string from UTF-16 to UTF-8.
///
/// By way of a transcoding operation, this function produces the UTF-8 encoding of the
/// string that has the specified UTF-16 encoding (\p string), and it does so in a lenient
/// manner. This means that any invalid UTF-16 sequence in the specified string will be
/// handled as if it was the valid encoding of the Unicode replacement character.
///
/// This function does not recognize a UTF-16 byte order mark (BOM). It is the
/// responsibility of the application to ensure its absence.
///
/// This function is implemented in terms of \ref core::decode_utf16_a() and \ref
/// core::resync_utf16().
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// \sa \ref core::utf16_to_utf8(), \ref core::utf16_to_utf8_a(), \ref core::resync_utf16()
///
/// \sa \ref core::utf8_to_utf16_l(), \ref core::decode_utf16_l(), \ref
/// core::encode_utf8_l()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void utf16_to_utf8_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Transcode portion of UTF-16 encoding to UTF-8.
///
/// This function transcodes a portion of the specified UTF-16 encoding (\p string) to UTF-8
/// starting at the specified offset (\p string_offset) and ending when the end of the
/// UTF-16 encoding is reached, or when an invalid UTF-16 sequence is encountered. The
/// produced UTF-8 encoding is stored in the specified buffer (\p buffer) starting at the
/// specified offset (\p buffer_offset). The buffer will be expanded as necessary (using a
/// progressive expansion scheme).
///
/// This function does not recognize a UTF-16 byte order mark (BOM). It is the
/// responsibility of the application to ensure its absence.
///
/// The invocation `utf16_to_utf8_a(string, string_offset, buffer, buffer_offset)` has the
/// same effect as `try_utf16_to_utf8(string.subspan(string_offset), buffer,
/// buffer_offset)`, except that when an invalid UTF-16 sequence is encountered, the former
/// invocation, i.e., this function sets \p string_offset to point to the invalid UTF-16
/// sequence, and \p buffer_offset to point to the corresponding position in the output
/// buffer. This allows the caller to decide what to do about the invalid sequence, and, if
/// desired, to resume the transcoding process by skipping over some input (see \ref
/// core::resync_utf16()).
///
/// Upon return, if \p string_offset is equal to `string.size()`, the transcoding process
/// completed. Otherwise, it stopped because of an invalid UTF-16 sequence. An incomplete
/// UTF-16 sequence at the end of the specified string will be considered invalid.
///
/// If this function throws, \p string_offset may, or may not have been advanced, but if it
/// has been advanced, \p buffer_offset will have been advanced accordingly. If \p
/// string_offset was not advanced, \p buffer_offset will also not have been advanced. Also,
/// the buffer may have been expanded, and the buffer contents beyond \p buffer_offset may
/// have been clobbered.
///
/// Behavior is undefined if, prior to the invocation, \p string_size is greater than
/// `string.size()` or \p buffer_offset is greater than `buffer.size()`.
///
/// This function is implemented in terms of \ref core::utf16_to_utf8_incr().
///
/// \sa \ref core::utf16_to_utf8(), \ref core::utf16_to_utf8_l(), \ref
/// core::try_utf16_to_utf8(), \ref core::utf16_to_utf8_incr(), \ref core::resync_utf16()
///
/// \sa \ref core::utf8_to_utf16_a(), \ref core::decode_utf16_a(), \ref
/// core::encode_utf8_a()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void utf16_to_utf8_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                     std::size_t& buffer_offset);


/// \brief Try to produce UTF-8 encoding of UCS string.
///
/// This function attempts to produce the UTF-8 encoding of the specified string (\p
/// string). Each character of the string is interpreted as a Unicode code point. Encoding
/// succeeds if all specified code points are valid. A code point is valid if it is in the
/// principal range (U+0000 -> U+10FFFF) and not in the surrogate range (U+D800 -> U+DFFF),
/// and is also not one of the two non-character code points, U+FFFE and U+FFFF. The UTF-8
/// encoding is stored in the specified buffer (\p buffer) starting at the specified
/// position (\p buffer_offset). The buffer will be expanded as needed (using a progressive
/// expansion scheme).
///
/// When encoding succeeds, this function returns `true` after setting \p buffer_offset to
/// point to the end of the UTF-8 encoding in the buffer (one beyond the last code unit of
/// the UTF-8 encoding).
///
/// When encoding fails, this function returns `false` and leaves \p buffer_offset
/// unchanged. In this case, the buffer may have been expanded, and buffer contents beyond
/// \p buffer_offset may have been clobbered.
///
/// If this function throws, \p buffer_offset will be unchanged, but the buffer may have
/// been expanded, and buffer contents beyond \p buffer_offset may have been clobbered.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// This function is implemented in terms of \ref core::encode_utf8_a() which, in turn, is
/// implemented in terms of \ref core::encode_utf8_incr().
///
/// While the input character type, \p C, needs to have a bit-width of at least 21 in order
/// to hold the full range of valid Unicode code points (U+0000 -> U+10FFFF), this function
/// can be used with any input character type where `std::char_traits<C>::eof()` does not
/// collide with a valid code point (U+0000 -> U+D7FF, U+E000 -> U+FFFD, U+10000 ->
/// U+10FFFF), including ones that are too narrow to hold the full range of code points. As
/// an example, `char` can be used because `std::char_traits<char>::eof()` is required to be
/// negative. `char32_t` can be used when the full UCS range is needed.
///
/// The output character type, \p D, which is used to hold UTF-8 code units, needs to have a
/// bit-width of at least 8, and `std::char_traits<D>::eof()` must be outside the range of
/// valid UTF-8 code units, 0x00 -> 0xFF. A reasonable choice is `char`, which works because
/// it is required to have a bit width of at least 8, and `std::char_traits<char>::eof()` is
/// required to be negative. `char8_t` can also be used.
///
/// \sa \ref core::encode_utf8(), \ref core::encode_utf8_a(), \ref core::encode_utf8_incr()
///
/// \sa \ref core::try_decode_utf8(), \ref core::try_encode_utf16(), \ref
/// core::try_utf16_to_utf8()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
bool try_encode_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Try to recover UCS string from UTF-8 encoding.
///
/// This function attempts to recover the UCS string from the specified UTF-8 encoding (\p
/// string). Decoding succeeds if the specified string consists of a series of valid UTF-8
/// sequences. The recovered UCS string is stored in the specified buffer (\p buffer)
/// starting at the specified position (\p buffer_offset). The buffer will be expanded as
/// needed (using a progressive expansion scheme).
///
/// When decoding succeeds, this function returns `true` after setting \p buffer_offset to
/// point to the end of the recovered UCS string in the buffer (one beyond the last code
/// point of the recovered string).
///
/// When decoding fails, this function returns `false` and leaves \p buffer_offset
/// unchanged. In this case, the buffer may have been expanded, and buffer contents beyond
/// \p buffer_offset may have been clobbered.
///
/// If this function throws, \p buffer_offset will be unchanged, but the buffer may have
/// been expanded, and buffer contents beyond \p buffer_offset may have been clobbered.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// This function is implemented in terms of \ref core::decode_utf8_a() which, in turn, is
/// implemented in terms of \ref core::decode_utf8_incr().
///
/// The input character type, \p C, which is used to hold UTF-8 code units, needs to have a
/// bit-width of at least 8, and `std::char_traits<C>::eof()` must be outside the range of
/// valid UTF-8 code units, 0x00 -> 0xFF. A reasonable choice is `char`, which works because
/// it is required to have a bit width of at least 8, and `std::char_traits<char>::eof()` is
/// required to be negative. `char8_t` can also be used.
///
/// While the output character type, \p D, which is used to hold Unicode code points, should
/// have a bit-width of at least 21 (the smallest number of bits that can hold U+10FFFF), it
/// must have a bit-width of at least 16. This allows for `wchar_t` to be used on the
/// Windows platform. If the bit-width of \p D is less than 21, an input sequence that
/// decodes to a code point that is larger than U+FFFF, will be considered invalid. In any
/// case, `std::char_traits<D>::eof()` must not collide with a valid code point (U+0000 ->
/// U+D7FF, U+E000 -> U+FFFD, U+10000 -> U+10FFFF). Both `wchar_t` and `char32_t` can be
/// used here.
///
/// \sa \ref core::decode_utf8(), \ref core::decode_utf8_a(), \ref core::decode_utf8_incr()
///
/// \sa \ref core::try_encode_utf8(), \ref core::try_decode_utf16(), \ref
/// core::try_utf8_to_utf16()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
bool try_decode_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Try to produce UTF-16 encoding of UCS string.
///
/// This function attempts to produce the UTF-16 encoding of the specified string (\p
/// string). Each character of the string is interpreted as a Unicode code point. Encoding
/// succeeds if all specified code points are valid. A code point is valid if it is in the
/// principal range (U+0000 -> U+10FFFF) and not in the surrogate range (U+D800 -> U+DFFF),
/// and is also not one of the two non-character code points, U+FFFE and U+FFFF. The UTF-16
/// encoding is stored in the specified buffer (\p buffer) starting at the specified
/// position (\p buffer_offset). The buffer will be expanded as needed (using a progressive
/// expansion scheme).
///
/// When encoding succeeds, this function returns `true` after setting \p buffer_offset to
/// point to the end of the UTF-16 encoding in the buffer (one beyond the last code unit of
/// the UTF-16 encoding).
///
/// When encoding fails, this function returns `false` and leaves \p buffer_offset
/// unchanged. In this case, the buffer may have been expanded, and buffer contents beyond
/// \p buffer_offset may have been clobbered.
///
/// If this function throws, \p buffer_offset will be unchanged, but the buffer may have
/// been expanded, and buffer contents beyond \p buffer_offset may have been clobbered.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// This function is implemented in terms of \ref core::encode_utf16_a() which, in turn, is
/// implemented in terms of \ref core::encode_utf16_incr().
///
/// While the input character type, \p C, needs to have a bit-width of at least 21 in order
/// to hold the full range of valid Unicode code points (U+0000 -> U+10FFFF), this function
/// can be used with any input character type where `std::char_traits<C>::eof()` does not
/// collide with a valid code point (U+0000 -> U+D7FF, U+E000 -> U+FFFD, U+10000 ->
/// U+10FFFF), including ones that are too narrow to hold the full range of code points. As
/// an example, `char` can be used because `std::char_traits<char>::eof()` is required to be
/// negative. `char32_t` can be used when the full UCS range is needed.
///
/// The output character type, \p D, which is used to hold UTF-16 code units, needs to have a
/// bit-width of at least 16, and `std::char_traits<D>::eof()` must be outside the range of
/// valid UTF-16 code units, 0x0000 -> 0xFFFD. `char16_t` can be used here.
///
/// \sa \ref core::encode_utf16(), \ref core::encode_utf16_a(), \ref
/// core::encode_utf16_incr()
///
/// \sa \ref core::try_decode_utf16(), \ref core::try_encode_utf8(), \ref
/// core::try_utf8_to_utf16()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
bool try_encode_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Try to recover UCS string from UTF-16 encoding.
///
/// This function attempts to recover the UCS string from the specified UTF-16 encoding (\p
/// string). Decoding succeeds if the specified string consists of a series of valid UTF-16
/// sequences. The recovered UCS string is stored in the specified buffer (\p buffer)
/// starting at the specified position (\p buffer_offset). The buffer will be expanded as
/// needed (using a progressive expansion scheme).
///
/// This function does not recognize a UTF-16 byte order mark (BOM). It is the
/// responsibility of the application to ensure its absence.
///
/// When decoding succeeds, this function returns `true` after setting \p buffer_offset to
/// point to the end of the recovered UCS string in the buffer (one beyond the last code
/// point of the recovered string).
///
/// When decoding fails, this function returns `false` and leaves \p buffer_offset
/// unchanged. In this case, the buffer may have been expanded, and buffer contents beyond
/// \p buffer_offset may have been clobbered.
///
/// If this function throws, \p buffer_offset will be unchanged, but the buffer may have
/// been expanded, and buffer contents beyond \p buffer_offset may have been clobbered.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// This function is implemented in terms of \ref core::decode_utf16_a() which, in turn, is
/// implemented in terms of \ref core::decode_utf16_incr().
///
/// The input character type, \p C, which is used to hold UTF-16 code units, needs to have a
/// bit-width of at least 16, and `std::char_traits<C>::eof()` must be outside the range of
/// valid UTF-16 code units, 0x0000 -> 0xFFFD. `char16_t` can be used here.
///
/// While the output character type, \p D, which is used to hold Unicode code points, should
/// have a bit-width of at least 21 (the smallest number of bits that can hold U+10FFFF), it
/// must have a bit-width of at least 16. This allows for `wchar_t` to be used on the
/// Windows platform. If the bit-width of \p D is less than 21, an input sequence that
/// decodes to a code point that is larger than U+FFFF, will be considered invalid (i.e.,
/// surrogate pairs). In any case, `std::char_traits<D>::eof()` must not collide with a
/// valid code point (U+0000 -> U+D7FF, U+E000 -> U+FFFD, U+10000 -> U+10FFFF). Both
/// `wchar_t` and `char32_t` can be used here.
///
/// \sa \ref core::decode_utf16(), \ref core::decode_utf16_a(), \ref
/// core::decode_utf16_incr()
///
/// \sa \ref core::try_encode_utf16(), \ref core::try_decode_utf8(), \ref
/// core::try_utf16_to_utf8()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
bool try_decode_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Try to transcode string from UTF-8 to UTF-16.
///
/// By way of a transcoding operation, this function attempts to produce the UTF-16 encoding
/// of the string that has the specified UTF-8 encoding (\p string). Transcoding succeeds if
/// the specified string consists of a series of valid UTF-8 sequences. The resulting UTF-16
/// encoding is stored in the specified buffer (\p buffer) starting at the specified
/// position (\p buffer_offset). The buffer will be expanded as needed (using a progressive
/// expansion scheme).
///
/// When transcoding succeeds, this function returns `true` after setting \p buffer_offset
/// to point to the end of the produced UTF-16 encoding in the buffer (one beyond the last
/// code unit of the UTF-16 encoding).
///
/// When transcoding fails, this function returns `false` and leaves \p buffer_offset
/// unchanged. In this case, the buffer may have been expanded, and buffer contents beyond
/// \p buffer_offset may have been clobbered.
///
/// If this function throws, \p buffer_offset will be unchanged, but the buffer may have
/// been expanded, and buffer contents beyond \p buffer_offset may have been clobbered.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// This function is implemented in terms of \ref core::utf8_to_utf16_a() which, in turn, is
/// implemented in terms of \ref core::utf8_to_utf16_incr().
///
/// The input character type, \p C, which is used to hold UTF-8 code units, needs to have a
/// bit-width of at least 8, and `std::char_traits<C>::eof()` must be outside the range of
/// valid UTF-8 code units, 0x00 -> 0xFF. A reasonable choice is `char`, which works because
/// it is required to have a bit width of at least 8, and `std::char_traits<char>::eof()` is
/// required to be negative. `char8_t` can also be used.
///
/// The output character type, \p D, which is used to hold UTF-16 code units, needs to have a
/// bit-width of at least 16, and `std::char_traits<D>::eof()` must be outside the range of
/// valid UTF-16 code units, 0x0000 -> 0xFFFD. `char16_t` can be used here.
///
/// \sa \ref core::utf8_to_utf16(), \ref core::utf8_to_utf16_a(), \ref
/// core::utf8_to_utf16_incr()
///
/// \sa \ref core::try_utf16_to_utf8(), \ref core::try_decode_utf8(), \ref
/// core::try_encode_utf16()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
bool try_utf8_to_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Try to transcode string from UTF-16 to UTF-8.
///
/// By way of a transcoding operation, this function attempts to produce the UTF-8 encoding
/// of the string that has the specified UTF-16 encoding (\p string). Transcoding succeeds
/// if the specified string consists of a series of valid UTF-16 sequences. The resulting
/// UTF-8 encoding is stored in the specified buffer (\p buffer) starting at the specified
/// position (\p buffer_offset). The buffer will be expanded as needed (using a progressive
/// expansion scheme).
///
/// This function does not recognize a UTF-16 byte order mark (BOM). It is the
/// responsibility of the application to ensure its absence.
///
/// When transcoding succeeds, this function returns `true` after setting \p buffer_offset
/// to point to the end of the produced UTF-8 encoding in the buffer (one beyond the last
/// code unit of the UTF-8 encoding).
///
/// When transcoding fails, this function returns `false` and leaves \p buffer_offset
/// unchanged. In this case, the buffer may have been expanded, and buffer contents beyond
/// \p buffer_offset may have been clobbered.
///
/// If this function throws, \p buffer_offset will be unchanged, but the buffer may have
/// been expanded, and buffer contents beyond \p buffer_offset may have been clobbered.
///
/// Behavior is undefined if \p buffer_offset is greater than `buffer.size()` prior to the
/// invocation.
///
/// This function is implemented in terms of \ref core::utf16_to_utf8_a() which, in turn, is
/// implemented in terms of \ref core::utf16_to_utf8_incr().
///
/// The input character type, \p C, which is used to hold UTF-16 code units, needs to have a
/// bit-width of at least 16, and `std::char_traits<C>::eof()` must be outside the range of
/// valid UTF-16 code units, 0x0000 -> 0xFFFD. `char16_t` can be used here.
///
/// The output character type, \p D, which is used to hold UTF-8 code units, needs to have a
/// bit-width of at least 8, and `std::char_traits<D>::eof()` must be outside the range of
/// valid UTF-8 code units, 0x00 -> 0xFF. A reasonable choice is `char`, which works because
/// it is required to have a bit width of at least 8, and `std::char_traits<char>::eof()` is
/// required to be negative. `char8_t` can also be used.
///
/// \sa \ref core::utf16_to_utf8(), \ref core::utf16_to_utf8_a(), \ref
/// core::utf16_to_utf8_incr()
///
/// \sa \ref core::try_utf8_to_utf16(), \ref core::try_decode_utf16(), \ref
/// core::try_encode_utf8()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
bool try_utf16_to_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset);


/// \brief Next step of incremental UTF-8 encoding process.
///
/// The purpose of this function is to advance an ongoing UTF-8 encoding process. An
/// invocation of this function will encode characters until all input is encoded, an
/// invalid code point is reached, or the output buffer is exhausted.
///
/// If `i` is the value of \p in_offset prior to invocation and `j` is the value of \p
/// out_offset prior to invocation, then the available input is `in.subspan(i)` and the
/// available space for output is `out.subspan(j)`.
///
/// Upon return, \p in_offset will have been advanced to point one beyond the last encoded
/// character, and \p out_offset will have been advanced to point to the corresponding
/// position in the output buffer, which is one beyond the last code unit (byte) of the
/// UTF-8 sequence corresponding to the last encoded character.
///
/// If, upon return, \p in_exhausted is set to `true`, the encoding process stopped because
/// all of the specified input was encoded. Otherwise, if \p error is set to `true`, the
/// encoding process stopped because of invalid input. Otherwise the encoding process
/// stopped because of insufficient output space for the next UTF-8 sequence.
///
/// When both \p in_exhausted and \p error are set to `false` to indicate that there is
/// insufficient output space, the input is definitely not exhausted.
///
/// When \p in_exhausted is set to `true`, \p error has no meaning and is left unchanged by
/// this function.
///
/// Upon return, \p string_offset will be equal to `string.size()` when and only when \p
/// in_exhausted was set to `true`.
///
/// A code point is valid if its unpacked value (`std::char_traits<C>::to_int_type()`) is in
/// the principal range (U+0000 -> U+10FFFF) and not in the surrogate range (U+D800 ->
/// U+DFFF), and is also not one of the two non-character code points, U+FFFE and
/// U+FFFF. Every valid code point produces one UTF-8 sequence. A UTF-8 sequence is between
/// one and four code units long (elements of type \p D).
///
/// Behavior is undefined if, prior to the invocation, \p in_size is greater than
/// `in.size()` or \p out_offset is greater than `out.size()`.
///
/// While the input character type, \p C, needs to have a bit-width of at least 21 in order
/// to hold the full range of valid Unicode code points (U+0000 -> U+10FFFF), this function
/// can be used with any input character type where `std::char_traits<C>::eof()` does not
/// collide with a valid code point (U+0000 -> U+D7FF, U+E000 -> U+FFFD, U+10000 ->
/// U+10FFFF), including ones that are too narrow to hold the full range of code points. As
/// an example, `char` can be used because `std::char_traits<char>::eof()` is required to be
/// negative. `char32_t` can be used when the full UCS range is needed. On the Windows
/// platform, `wchar_t` is too narrow to hold all Unicode code points, but can still bu used
/// for characters in the Basic Multilingual Plane (BMP) because
/// `std::char_traits<wchar_t>::eof()` is 0xFFFF on Windows, which is not a valid Unicode
/// code point.
///
/// The output character type, \p D, which is used to hold UTF-8 code units, needs to have a
/// bit-width of at least 8, and `std::char_traits<D>::eof()` must be outside the range of
/// valid UTF-8 code units, 0x00 -> 0xFF. A reasonable choice is `char`, which works because
/// it is required to have a bit width of at least 8, and `std::char_traits<char>::eof()` is
/// required to be negative. `char8_t` can also be used.
///
/// \sa \ref core::encode_utf8(), \ref core::encode_utf8_a()
///
/// \sa \ref core::decode_utf8_incr(), \ref core::encode_utf16_incr(), \ref
/// core::utf16_to_utf8_incr()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void encode_utf8_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                      bool& in_exhausted, bool& error) noexcept;


/// \brief Next step of incremental UTF-8 decoding process.
///
/// The purpose of this function is to advance an ongoing UTF-8 decoding process. An
/// invocation of this function will decode characters until input is exhausted, invalid
/// input is encountered, or the output buffer runs full.
///
/// If `i` is the value of \p in_offset prior to invocation and `j` is the value of \p
/// out_offset prior to invocation, then the available input is `in.subspan(i)` and the
/// available space for output is `out.subspan(j)`.
///
/// Upon return, \p in_offset will have been advanced to point one beyond the last code unit
/// of the last decoded UTF-8 sequence, and \p out_offset will have been advanced to point
/// to the corresponding position in the output buffer, which is one beyond the character
/// corresponding to the last decoded UTF-8 sequence.
///
/// If, upon return, \p in_exhausted is set to `true`, the decoding process stopped because
/// input was exhausted. Otherwise, if \p error is set to `true`, the decoding process
/// stopped because of invalid input. Otherwise the decoding process stopped because the
/// output buffer was full. In this case, input is definitely not exhausted. Input is
/// exhausted when all of the specified input has been decoded, or when the remaining input
/// is a proper prefix of a valid UTF-8 sequence.
///
/// When \p in_exhausted is set to `true` to indicate that input is exhausted, the output
/// buffer may or may not also be full. On the other hand, when both \p in_exhausted and \p
/// error are set to `false` to indicate that the buffer is full, input is definitely not
/// exhausted.
///
/// Note that during each step of an incremental decoding process, there is a chance that
/// this function will leave behind some unconsumed code units at the end of the specified
/// input corresponding to a final incomplete UTF-8 sequence. It is important that the
/// application arranges for those remaining code units to be retained as a prefix of the
/// input that is passed to this function during the next step of the incremental decoding
/// process. Presumably, the incomplete UTF-8 sequence is completed, or at least continued
/// by the additional input that is made available during that next decoding step. When the
/// specified input covers the end of input for the entire decoding process, the application
/// should probably consider any unconsumed incomplete UTF-8 sequence as an error.
///
/// When the decoding process is stopped due to invalid input, the application may wish to
/// skip over some input and attempt to resume the decoding process when possible. Such a
/// behavior can be effectively implemented using \ref core::resync_utf8(). See also \ref
/// core::decode_utf8_l() for an example of a function that uses \ref core::resync_utf8().                                                                                             
///
/// Behavior is undefined if, prior to the invocation, \p in_size is greater than
/// `in.size()` or \p out_offset is greater than `out.size()`.
///
/// The input character type, \p C, which is used to hold UTF-8 code units, needs to have a
/// bit-width of at least 8, and `std::char_traits<C>::eof()` must be outside the range of
/// valid UTF-8 code units, 0x00 -> 0xFF. A reasonable choice is `char`, which works because
/// it is required to have a bit width of at least 8, and `std::char_traits<char>::eof()` is
/// required to be negative. `char8_t` can also be used.
///
/// While the output character type, \p D, which is used to hold Unicode code points, should
/// have a bit-width of at least 21 (the smallest number of bits that can hold U+10FFFF), it
/// must have a bit-width of at least 16. This allows for `wchar_t` to be used on the
/// Windows platform. If the bit-width of \p D is less than 21, an input sequence that
/// decodes to a code point that is larger than U+FFFF, will be considered invalid. In any
/// case, `std::char_traits<D>::eof()` must not collide with a valid code point (U+0000 ->
/// U+D7FF, U+E000 -> U+FFFD, U+10000 -> U+10FFFF). Both `wchar_t` and `char32_t` can be
/// used here.
///
/// \sa \ref core::decode_utf8(), \ref core::decode_utf8_l(), \ref core::decode_utf8_a(),
/// \ref core::resync_utf8()
///
/// \sa \ref core::encode_utf8_incr(), \ref core::decode_utf16_incr(), \ref
/// core::utf8_to_utf16_incr()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void decode_utf8_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                      bool& in_exhausted, bool& error) noexcept;


/// \brief Next step of lenient incremental UTF-8 decoding process.
///
/// This function advances an ongoing incremental UTF-8 decoding process just like \ref
/// core::decode_utf8_incr(). The difference is that this function operates with leniency in
/// the sense that it accepts invalid input and automatically replaces it with replacement
/// characters. It is implemented in terms of \ref core::decode_utf8_incr() and \ref
/// core::resync_utf8().
///
/// For the meaning of parameters \p in, \p out, \p in_offset, and \p out_offset, see \ref
/// core::decode_utf8_incr().
///
/// If this function returns `true` and \p end_of_input was `true`, it means that all of the
/// specified input was decoded. If \p end_of_input was `false`, it means that if any input
/// remains unconsumed, it must be a prefix of some valid UTF-8 sequence. The idea is that
/// the caller sets \p end_of_input to `true` when the caller knows that the passed input
/// includes all of the remaining input for the whole decoding process. The caller then
/// knows that the decoding process is complete when this function returns `true` and the
/// end of input for the whole decoding process was included.
///
/// If this function returns `false`, it means that output space was exhausted before all
/// input was decoded. In that case, this function should be called again with at least some
/// available output space.
///
/// \sa \ref core::decode_utf8_incr()
///    
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
bool decode_utf8_incr_l(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                        bool end_of_input) noexcept;


/// \brief Next step of incremental UTF-16 encoding process.
///
/// The purpose of this function is to advance an ongoing UTF-16 encoding process. An
/// invocation of this function will encode characters until all input is encoded, an
/// invalid code point is reached, or the output buffer is exhausted.
///
/// If `i` is the value of \p in_offset prior to invocation and `j` is the value of \p
/// out_offset prior to invocation, then the available input is `in.subspan(i)` and the
/// available space for output is `out.subspan(j)`.
///
/// Upon return, \p in_offset will have been advanced to point one beyond the last encoded
/// character, and \p out_offset will have been advanced to point to the corresponding
/// position in the output buffer, which is one beyond the last code unit of the UTF-16
/// sequence corresponding to the last encoded character.
///
/// If, upon return, \p in_exhausted is set to `true`, the encoding process stopped because
/// all of the specified input was encoded. Otherwise, if \p error is set to `true`, the
/// encoding process stopped because of invalid input. Otherwise the encoding process
/// stopped because of insufficient output space for the next UTF-16 sequence.
///
/// When both \p in_exhausted and \p error are set to `false` to indicate that there is
/// insufficient output space, the input is definitely not exhausted.
///
/// When \p in_exhausted is set to `true`, \p error has no meaning and is left unchanged by
/// this function.
///
/// Upon return, \p string_offset will be equal to `string.size()` when and only when \p
/// in_exhausted was set to `true`.
///
/// A code point is valid if its unpacked value (`std::char_traits<C>::to_int_type()`) is in
/// the principal range (U+0000 -> U+10FFFF) and not in the surrogate range (U+D800 ->
/// U+DFFF), and is also not one of the two non-character code points, U+FFFE and
/// U+FFFF. Every valid code point produces one UTF-16 sequence. A UTF-16 sequence is one or
/// two code units long (elements of type \p D).
///
/// Behavior is undefined if, prior to the invocation, \p in_size is greater than
/// `in.size()` or \p out_offset is greater than `out.size()`.
///
/// While the input character type, \p C, needs to have a bit-width of at least 21 in order
/// to hold the full range of valid Unicode code points (U+0000 -> U+10FFFF), this function
/// can be used with any input character type where `std::char_traits<C>::eof()` does not
/// collide with a valid code point (U+0000 -> U+D7FF, U+E000 -> U+FFFD, U+10000 ->
/// U+10FFFF), including ones that are too narrow to hold the full range of code points. As
/// an example, `char` can be used because `std::char_traits<char>::eof()` is required to be
/// negative. `char32_t` can be used when the full UCS range is needed. On the Windows
/// platform, `wchar_t` is too narrow to hold all Unicode code points, but can still bu used
/// for characters in the Basic Multilingual Plane (BMP) because
/// `std::char_traits<wchar_t>::eof()` is 0xFFFF on Windows, which is not a valid Unicode
/// code point.
///
/// The output character type, \p D, which is used to hold UTF-16 code units, needs to have a
/// bit-width of at least 16, and `std::char_traits<D>::eof()` must be outside the range of
/// valid UTF-16 code units, 0x0000 -> 0xFFFD. `char16_t` can be used here.
///
/// \sa \ref core::encode_utf16(), \ref core::encode_utf16_a()
///
/// \sa \ref core::decode_utf16_incr(), \ref core::encode_utf8_incr(), \ref
/// core::utf8_to_utf16_incr()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void encode_utf16_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                       bool& in_exhausted, bool& error) noexcept;


/// \brief Next step of incremental UTF-16 decoding process.
///
/// The purpose of this function is to advance an ongoing UTF-16 decoding process. An
/// invocation of this function will decode characters until input is exhausted, invalid
/// input is encountered, or the output buffer runs full.
///
/// This function does not and could not distinguish between a UTF-16 byte order mark (BOM,
/// U+FEFF) and the invisible zero-width non-breaking space (ZWNBSP). It is the
/// responsibility of the application to remove any byte order mark from the input.
///
/// If `i` is the value of \p in_offset prior to invocation and `j` is the value of \p
/// out_offset prior to invocation, then the available input is `in.subspan(i)` and the
/// available space for output is `out.subspan(j)`.
///
/// Upon return, \p in_offset will have been advanced to point one beyond the last code unit
/// of the last decoded UTF-16 sequence, and \p out_offset will have been advanced to point
/// to the corresponding position in the output buffer, which is one beyond the character
/// corresponding to the last decoded UTF-16 sequence.
///
/// If, upon return, \p in_exhausted is set to `true`, the decoding process stopped because
/// input was exhausted. Otherwise, if \p error is set to `true`, the decoding process
/// stopped because of invalid input. Otherwise the decoding process stopped because the
/// output buffer was full. Input is exhausted when all of the specified input has been
/// decoded, or when the remaining input is a proper prefix of a valid UTF-16 sequence.
///
/// When \p in_exhausted is set to `true` to indicate that input is exhausted, the output
/// buffer may or may not also be full. On the other hand, when both \p in_exhausted and \p
/// error are set to `false` to indicate that the buffer is full, input is definitely not
/// exhausted.
///
/// When \p in_exhausted is set to `true`, \p error has no meaning and is left unchanged by
/// this function.
///
/// Note that during each step of an incremental decoding process, there is a chance that
/// this function will leave behind an unconsumed code unit at the end of the specified
/// input corresponding to a final incomplete UTF-16 sequence. It is important that the
/// application arranges for this remaining code unit to be retained as a prefix of the
/// input that is passed to this function during the next step of the incremental decoding
/// process. Presumably, the incomplete UTF-16 sequence is completed by the additional input
/// that is made available during that next decoding step. When the specified input covers
/// the end of input for the entire decoding process, the application should probably
/// consider any unconsumed incomplete UTF-16 sequence as an error.
///
/// When the decoding process is stopped due to invalid input, the application may wish to
/// skip over some input and attempt to resume the decoding process when possible. Such a
/// behavior can be effectively implemented using \ref core::resync_utf16(). See also \ref
/// core::decode_utf16_l() for an example of a function that uses \ref core::resync_utf16().
///
/// Behavior is undefined if, prior to the invocation, \p in_size is greater than
/// `in.size()` or \p out_offset is greater than `out.size()`.
///
/// The input character type, \p C, which is used to hold UTF-16 code units, needs to have a
/// bit-width of at least 16, and `std::char_traits<C>::eof()` must be outside the range of
/// valid UTF-16 code units, 0x0000 -> 0xFFFD. `char16_t` can be used here.
///
/// While the output character type, \p D, which is used to hold Unicode code points, should
/// have a bit-width of at least 21 (the smallest number of bits that can hold U+10FFFF), it
/// must have a bit-width of at least 16. This allows for `wchar_t` to be used on the
/// Windows platform. If the bit-width of \p D is less than 21, an input sequence that
/// decodes to a code point that is larger than U+FFFF, will be considered invalid (i.e.,
/// surrogate pairs). In any case, `std::char_traits<D>::eof()` must not collide with a
/// valid code point (U+0000 -> U+D7FF, U+E000 -> U+FFFD, U+10000 -> U+10FFFF). Both
/// `wchar_t` and `char32_t` can be used here.
///
/// \sa \ref core::decode_utf16(), \ref core::decode_utf16_l(), \ref core::decode_utf16_a(),
/// \ref core::resync_utf16()
///
/// \sa \ref core::encode_utf16_incr(), \ref core::decode_utf8_incr(), \ref
/// core::utf16_to_utf8_incr()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void decode_utf16_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                       bool& in_exhausted, bool& error) noexcept;


/// \brief Next step of incremental UTF-8 to UTF-16 transcoding process.
///
/// The purpose of this function is to advance an ongoing UTF-8 to UTF-16 transcoding
/// process. An invocation of this function will transcode characters until input is
/// exhausted, invalid input is encountered, or there is insufficient space for output.
///
/// If `i` is the value of \p in_offset prior to invocation and `j` is the value of \p
/// out_offset prior to invocation, then the available input is `in.subspan(i)` and the
/// available space for output is `out.subspan(j)`.
///
/// Upon return, \p in_offset will have been advanced to point one beyond the last code unit
/// of the last transcoded UTF-8 sequence, and \p out_offset will have been advanced to
/// point to the corresponding position in the output buffer, which is one beyond the last
/// code unit of the UTF-16 sequence corresponding to the last transcoded UTF-8 sequence.
///
/// If, upon return, \p in_exhausted is set to `true`, the transcoding process stopped
/// because input was exhausted. Otherwise, if \p error is set to `true`, the transcoding
/// process stopped because of invalid input. Otherwise the transcoding process stopped
/// because there was insufficient output space for the next UTF-16 sequence. Input is
/// exhausted when all of the specified input has been transcoded, or when the remaining
/// input is a proper prefix of a valid UTF-8 sequence.
///
/// When both \p in_exhausted and \p error are set to `false` to indicate that there is
/// insufficient output space, the input is definitely not exhausted.
///
/// When \p in_exhausted is set to `true`, \p error has no meaning and is left unchanged by
/// this function.
///
/// Note that during each step of an incremental transcoding process, there is a chance that
/// this function will leave behind some unconsumed UTF-8 code units at the end of the
/// specified input corresponding to a final incomplete UTF-8 sequence. It is important that
/// the application arranges for those remaining code units to be retained as a prefix of
/// the input that is passed to this function during the next step of the incremental
/// transcoding process. Presumably, the incomplete UTF-8 sequence is completed, or at least
/// continued by the additional input that is made available during that next transcoding
/// step. When the specified input covers the end of input for the entire transcoding
/// process, the application should probably consider any unconsumed incomplete UTF-8
/// sequence as an error.
///
/// When the transcoding process is stopped due to invalid input, the application may wish
/// to skip over some input and attempt to resume the transcoding process when
/// possible. Such a behavior can be effectively implemented using \ref
/// core::resync_utf8(). See also \ref core::utf8_to_utf16_l() for an example of a function
/// that uses \ref core::resync_utf8().
///
/// Behavior is undefined if, prior to the invocation, \p in_size is greater than
/// `in.size()` or \p out_offset is greater than `out.size()`.
///
/// The input character type, \p C, which is used to hold UTF-8 code units, needs to have a
/// bit-width of at least 8, and `std::char_traits<C>::eof()` must be outside the range of
/// valid UTF-8 code units, 0x00 -> 0xFF. A reasonable choice is `char`, which works because
/// it is required to have a bit width of at least 8, and `std::char_traits<char>::eof()` is
/// required to be negative. `char8_t` can also be used.
///
/// The output character type, \p D, which is used to hold UTF-16 code units, needs to have a
/// bit-width of at least 16, and `std::char_traits<D>::eof()` must be outside the range of
/// valid UTF-16 code units, 0x0000 -> 0xFFFD. `char16_t` can be used here.
///
/// \sa \ref core::utf8_to_utf16(), \ref core::utf8_to_utf16_l(), \ref
/// core::utf8_to_utf16_a(), \ref core::resync_utf8()
///
/// \sa \ref core::utf16_to_utf8_incr(), \ref core::decode_utf8_incr(), \ref
/// core::encode_utf16_incr()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void utf8_to_utf16_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                        bool& in_exhausted, bool& error) noexcept;


/// \brief Next step of incremental UTF-16 to UTF-8 transcoding process.
///
/// The purpose of this function is to advance an ongoing UTF-16 to UTF-8 transcoding
/// process. An invocation of this function will transcode characters until input is
/// exhausted, invalid input is encountered, or there is insufficient space for output.
///
/// This function does not and could not distinguish between a UTF-16 byte order mark (BOM,
/// U+FEFF) and the invisible zero-width non-breaking space (ZWNBSP). It is the
/// responsibility of the application to remove any byte order mark from the input.
///
/// If `i` is the value of \p in_offset prior to invocation and `j` is the value of \p
/// out_offset prior to invocation, then the available input is `in.subspan(i)` and the
/// available space for output is `out.subspan(j)`.
///
/// Upon return, \p in_offset will have been advanced to point one beyond the last code unit
/// of the last transcoded UTF-16 sequence, and \p out_offset will have been advanced to
/// point to the corresponding position in the output buffer, which is one beyond the last
/// code unit of the UTF-8 sequence corresponding to the last transcoded UTF-16 sequence.
///
/// If, upon return, \p in_exhausted is set to `true`, the transcoding process stopped
/// because input was exhausted. Otherwise, if \p error is set to `true`, the transcoding
/// process stopped because of invalid input. Otherwise the transcoding process stopped
/// because there was insufficient output space for the next UTF-8 sequence. Input is
/// exhausted when all of the specified input has been transcoded, or when the remaining
/// input is a proper prefix of a valid UTF-16 sequence.
///
/// When both \p in_exhausted and \p error are set to `false` to indicate that there is
/// insufficient output space, the input is definitely not exhausted.
///
/// When \p in_exhausted is set to `true`, \p error has no meaning and is left unchanged by
/// this function.
///
/// Note that during each step of an incremental transcoding process, there is a chance that
/// this function will leave behind an unconsumed UTF-16 code unit at the end of the
/// specified input corresponding to a final incomplete UTF-16 sequence. It is important
/// that the application arranges for this remaining code unit to be retained as a prefix of
/// the input that is passed to this function during the next step of the incremental
/// transcoding process. Presumably, the incomplete UTF-16 sequence is completed by the
/// additional input that is made available during that next transcoding step. When the
/// specified input covers the end of input for the entire transcoding process, the
/// application should probably consider any unconsumed incomplete UTF-16 sequence as an
/// error.
///
/// When the transcoding process is stopped due to invalid input, the application may wish
/// to skip over some input and attempt to resume the transcoding process when
/// possible. Such a behavior can be effectively implemented using \ref
/// core::resync_utf16(). See also \ref core::utf16_to_utf8_l() for an example of a function
/// that uses \ref core::resync_utf16().
///
/// Behavior is undefined if, prior to the invocation, \p in_size is greater than
/// `in.size()` or \p out_offset is greater than `out.size()`.
///
/// The input character type, \p C, which is used to hold UTF-16 code units, needs to have a
/// bit-width of at least 16, and `std::char_traits<C>::eof()` must be outside the range of
/// valid UTF-16 code units, 0x0000 -> 0xFFFD. `char16_t` can be used here.
///
/// The output character type, \p D, which is used to hold UTF-8 code units, needs to have a
/// bit-width of at least 8, and `std::char_traits<D>::eof()` must be outside the range of
/// valid UTF-8 code units, 0x00 -> 0xFF. A reasonable choice is `char`, which works because
/// it is required to have a bit width of at least 8, and `std::char_traits<char>::eof()` is
/// required to be negative. `char8_t` can also be used.
///
/// \sa \ref core::utf16_to_utf8(), \ref core::utf16_to_utf8_l(), \ref
/// core::utf16_to_utf8_a(), \ref core::resync_utf16()
///
/// \sa \ref core::utf8_to_utf16_incr(), \ref core::decode_utf16_incr(), \ref
/// core::encode_utf8_incr()
///
template<class C, class D, class T = std::char_traits<C>, class U = std::char_traits<D>>
void utf16_to_utf8_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                        bool& in_exhausted, bool& error) noexcept;


/// \brief Discard rest of invalid UTF-8 sequence.
///
/// This function can be used as part of a UTF-8 decoding process to discard the rest of an
/// invalid UTF-8 sequence. It increments \p in_offset while \p in_offset is strictly less
/// than `in.size()` and while `in[in_offset]` does not look like the beginning of a UTF-8
/// sequence. In order for the resynchronization operation to work correctly, the
/// application must arrange for the first code unit of an invalid input sequence to be
/// discarded before this function is called.
///
/// If this function consumes all of the specified input, it has not yet found what will
/// count as the the beginning of the next UTF-8 sequence, so the resynchronization process
/// is incomplete. This means that if additional input becomes available, this function
/// should be called again to resume the incomplete resynchronization process.
///
/// This function is most useful when performing a lenient decoding operation (\ref
/// core::decode_utf8_l()) where invalid input is dealt with by generating a replacement
/// character. In this case, this function makes it possible to only generate one
/// replacement character for every invalid UTF-8 sequence rather than for every code unit
/// that is part of an invalid UTF-8 sequence.
///
/// Behavior is undefined if, prior to the invocation, \p in_size is greater than
/// `in.size()`.
///
/// \sa \ref core::decode_utf8_l(), \ref core::utf8_to_utf16_l(), \ref
/// core::decode_utf8_incr(), \ref core::utf8_to_utf16_incr()
///
/// \sa \ref core::resync_utf16()
///
template<class C, class T = std::char_traits<C>>
void resync_utf8(core::Span<const C> in, std::size_t& in_offset) noexcept;


/// \brief Discard rest of invalid UTF-16 sequence.
///
/// This function can be used as part of a UTF-16 decoding process to discard the rest of an
/// invalid UTF-16 sequence. It increments \p in_offset while \p in_offset is strictly less
/// than `in.size()` and while `in[in_offset]` does not look like the beginning of a UTF-16
/// sequence. In order for the resynchronization operation to work correctly, the
/// application must arrange for the first code unit of an invalid input sequence to be
/// discarded before this function is called.
///
/// If this function consumes all of the specified input, it has not yet found what will
/// count as the the beginning of the next UTF-16 sequence, so the resynchronization process
/// is incomplete. This means that if additional input becomes available, this function
/// should be called again to resume the incomplete resynchronization process.
///
/// This function is most useful when performing a lenient decoding operation (\ref
/// core::decode_utf16_l()) where invalid input is dealt with by generating a replacement
/// character. In this case, this function makes it possible to only generate one
/// replacement character for every invalid UTF-16 sequence rather than for every code unit
/// that is part of an invalid UTF-16 sequence.
///
/// Behavior is undefined if, prior to the invocation, \p in_size is greater than
/// `in.size()`.
///
/// \sa \ref core::decode_utf16_l(), \ref core::utf16_to_utf8_l(), \ref
/// core::decode_utf16_incr(), \ref core::utf16_to_utf8_incr()
///
/// \sa \ref core::resync_utf8()
///
template<class C, class T = std::char_traits<C>>
void resync_utf16(core::Span<const C> in, std::size_t& in_offset) noexcept;








// Implementation


namespace impl {


template<class T> constexpr bool valid_unicode_code_point(T val) noexcept
{
    return ((val >= 0 && val < 0xD800) ||
            (val >= 0xE000 && val < 0xFFFE) ||
            (val >= 0x10000 && val < 0x110000));
}


} // namespace impl


template<class C, class D, class T, class U>
inline void encode_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    bool success = core::try_encode_utf8<C, D, T, U>(string, buffer, buffer_offset); // Throws
    if (ARCHON_LIKELY(success))
        return;
    throw std::runtime_error("Could not encode to UTF-8");
}


template<class C, class D, class T, class U>
void encode_utf8_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    for (;;) {
        core::encode_utf8_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
        bool success = (string_offset == string.size());
        if (ARCHON_LIKELY(success)) {
            buffer_offset = buffer_offset_2;
            return;
        }
        ARCHON_ASSERT(string_offset < string.size());
        string_offset += 1;
        using char_type = D;
        using traits_type = U;
        char_type replacement[] = {
            traits_type::to_char_type(0xEF),
            traits_type::to_char_type(0xBF),
            traits_type::to_char_type(0xBD),
        };
        buffer.append(replacement, buffer_offset); // Throws
    }
}


template<class C, class D, class T, class U>
void encode_utf8_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                   std::size_t& buffer_offset)
{
    bool string_exhausted = {};
    bool error = {};
    for (;;) {
        core::encode_utf8_incr<C, D, T, U>(string, buffer, string_offset, buffer_offset, string_exhausted, error);
        if (ARCHON_LIKELY(string_exhausted)) {
            ARCHON_ASSERT(string_offset == string.size());
            return;
        }
        ARCHON_ASSERT(string_offset < string.size());
        if (ARCHON_LIKELY(!error)) {
            buffer.expand(buffer_offset); // Throws
            continue;
        }
        return;
    }
}


template<class C, class D, class T, class U>
inline void decode_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    bool success = core::try_decode_utf8<C, D, T, U>(string, buffer, buffer_offset); // Throws
    if (ARCHON_LIKELY(success))
        return;
    throw std::runtime_error("Could not decode from UTF-8");
}


template<class C, class D, class T, class U>
void decode_utf8_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    for (;;) {
        bool end_of_string = true;
        bool complete = core::decode_utf8_incr_l<C, D, T, U>(string, buffer, string_offset, buffer_offset,
                                                             end_of_string);
        if (ARCHON_LIKELY(complete)) {
            ARCHON_ASSERT(string_offset == string.size());
            return;
        }
        ARCHON_ASSERT(string_offset < string.size());
        buffer.expand(buffer_offset); // Throws
    }
}


template<class C, class D, class T, class U>
void decode_utf8_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                   std::size_t& buffer_offset)
{
    bool string_exhausted = {};
    bool error = {};
    for (;;) {
        core::decode_utf8_incr<C, D, T, U>(string, buffer, string_offset, buffer_offset, string_exhausted, error);
        if (ARCHON_LIKELY(string_exhausted))
            return;
        ARCHON_ASSERT(string_offset < string.size());
        if (ARCHON_LIKELY(!error)) {
            ARCHON_ASSERT(buffer_offset == buffer.size());
            buffer.expand(buffer_offset); // Throws
            continue;
        }
        return;
    }
}


template<class C, class D, class T, class U>
inline void encode_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    bool success = core::try_encode_utf16<C, D, T, U>(string, buffer, buffer_offset); // Throws
    if (ARCHON_LIKELY(success))
        return;
    throw std::runtime_error("Could not encode to UTF-16");
}


template<class C, class D, class T, class U>
void encode_utf16_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    for (;;) {
        core::encode_utf16_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
        bool success = (string_offset == string.size());
        if (ARCHON_LIKELY(success)) {
            buffer_offset = buffer_offset_2;
            return;
        }
        ARCHON_ASSERT(string_offset < string.size());
        string_offset += 1;
        using char_type = D;
        using traits_type = U;
        char_type replacement[] = {
            traits_type::to_char_type(0xFFFD),
        };
        buffer.append(replacement, buffer_offset); // Throws
    }
}


template<class C, class D, class T, class U>
void encode_utf16_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                    std::size_t& buffer_offset)
{
    bool string_exhausted = {};
    bool error = {};
    for (;;) {
        core::encode_utf16_incr<C, D, T, U>(string, buffer, string_offset, buffer_offset, string_exhausted, error);
        if (ARCHON_LIKELY(string_exhausted)) {
            ARCHON_ASSERT(string_offset == string.size());
            return;
        }
        ARCHON_ASSERT(string_offset < string.size());
        if (ARCHON_LIKELY(!error)) {
            buffer.expand(buffer_offset); // Throws
            continue;
        }
        return;
    }
}


template<class C, class D, class T, class U>
inline void decode_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    bool success = core::try_decode_utf16<C, D, T, U>(string, buffer, buffer_offset); // Throws
    if (ARCHON_LIKELY(success))
        return;
    throw std::runtime_error("Could not decode from UTF-16");
}


template<class C, class D, class T, class U>
void decode_utf16_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    for (;;) {
        core::decode_utf16_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
        bool success = (string_offset == string.size());
        if (ARCHON_LIKELY(success)) {
            buffer_offset = buffer_offset_2;
            return;
        }
        ARCHON_ASSERT(string_offset < string.size());
        string_offset += 1;
        core::resync_utf16<C, T>(string, string_offset);
        using char_type = D;
        using traits_type = U;
        char_type replacement[] = {
            traits_type::to_char_type(0xFFFD),
        };
        buffer.append(replacement, buffer_offset); // Throws
    }
}


template<class C, class D, class T, class U>
void decode_utf16_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                    std::size_t& buffer_offset)
{
    bool string_exhausted = {};
    bool error = {};
    for (;;) {
        core::decode_utf16_incr<C, D, T, U>(string, buffer, string_offset, buffer_offset, string_exhausted, error);
        if (ARCHON_LIKELY(string_exhausted))
            return;
        ARCHON_ASSERT(string_offset < string.size());
        if (ARCHON_LIKELY(!error)) {
            ARCHON_ASSERT(buffer_offset == buffer.size());
            buffer.expand(buffer_offset); // Throws
            continue;
        }
        return;
    }
}


template<class C, class D, class T, class U>
inline void utf8_to_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    bool success = core::try_utf8_to_utf16<C, D, T, U>(string, buffer, buffer_offset); // Throws
    if (ARCHON_LIKELY(success))
        return;
    throw std::runtime_error("Could not convert from UTF-8 to UTF-16");
}


template<class C, class D, class T, class U>
void utf8_to_utf16_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    for (;;) {
        core::utf8_to_utf16_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
        bool success = (string_offset == string.size());
        if (ARCHON_LIKELY(success)) {
            buffer_offset = buffer_offset_2;
            return;
        }
        ARCHON_ASSERT(string_offset < string.size());
        string_offset += 1;
        core::resync_utf8<C, T>(string, string_offset);
        using char_type = D;
        using traits_type = U;
        char_type replacement[] = {
            traits_type::to_char_type(0xFFFD),
        };
        buffer.append(replacement, buffer_offset); // Throws
    }
}


template<class C, class D, class T, class U>
void utf8_to_utf16_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                     std::size_t& buffer_offset)
{
    bool string_exhausted = {};
    bool error = {};
    for (;;) {
        core::utf8_to_utf16_incr<C, D, T, U>(string, buffer, string_offset, buffer_offset, string_exhausted, error);
        if (ARCHON_LIKELY(string_exhausted))
            return;
        ARCHON_ASSERT(string_offset < string.size());
        if (ARCHON_LIKELY(!error)) {
            buffer.expand(buffer_offset); // Throws
            continue;
        }
        return;
    }
}


template<class C, class D, class T, class U>
inline void utf16_to_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    bool success = core::try_utf16_to_utf8<C, D, T, U>(string, buffer, buffer_offset); // Throws
    if (ARCHON_LIKELY(success))
        return;
    throw std::runtime_error("Could not convert from UTF-16 to UTF-8");
}


template<class C, class D, class T, class U>
void utf16_to_utf8_l(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    for (;;) {
        core::utf16_to_utf8_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
        bool success = (string_offset == string.size());
        if (ARCHON_LIKELY(success)) {
            buffer_offset = buffer_offset_2;
            return;
        }
        ARCHON_ASSERT(string_offset < string.size());
        string_offset += 1;
        core::resync_utf16<C, T>(string, string_offset);
        using char_type = D;
        using traits_type = U;
        char_type replacement[] = {
            traits_type::to_char_type(0xEF),
            traits_type::to_char_type(0xBF),
            traits_type::to_char_type(0xBD),
        };
        buffer.append(replacement, buffer_offset); // Throws
    }
}


template<class C, class D, class T, class U>
void utf16_to_utf8_a(core::StringSpan<C> string, std::size_t& string_offset, core::Buffer<D>& buffer,
                     std::size_t& buffer_offset)
{
    bool string_exhausted = {};
    bool error = {};
    for (;;) {
        core::utf16_to_utf8_incr<C, D, T, U>(string, buffer, string_offset, buffer_offset, string_exhausted, error);
        if (ARCHON_LIKELY(string_exhausted))
            return;
        ARCHON_ASSERT(string_offset < string.size());
        if (ARCHON_LIKELY(!error)) {
            buffer.expand(buffer_offset); // Throws
            continue;
        }
        return;
    }
}


template<class C, class D, class T, class U>
inline bool try_encode_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    core::encode_utf8_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
    bool success = (string_offset == string.size());
    if (ARCHON_LIKELY(success)) {
        buffer_offset = buffer_offset_2;
        return true;
    }
    ARCHON_ASSERT(string_offset < string.size());
    return false;
}


template<class C, class D, class T, class U>
inline bool try_decode_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    core::decode_utf8_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
    bool success = (string_offset == string.size());
    if (ARCHON_LIKELY(success)) {
        buffer_offset = buffer_offset_2;
        return true;
    }
    ARCHON_ASSERT(string_offset < string.size());
    return false;
}


template<class C, class D, class T, class U>
inline bool try_encode_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    core::encode_utf16_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
    bool success = (string_offset == string.size());
    if (ARCHON_LIKELY(success)) {
        buffer_offset = buffer_offset_2;
        return true;
    }
    ARCHON_ASSERT(string_offset < string.size());
    return false;
}


template<class C, class D, class T, class U>
inline bool try_decode_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    core::decode_utf16_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
    bool success = (string_offset == string.size());
    if (ARCHON_LIKELY(success)) {
        buffer_offset = buffer_offset_2;
        return true;
    }
    ARCHON_ASSERT(string_offset < string.size());
    return false;
}


template<class C, class D, class T, class U>
inline bool try_utf8_to_utf16(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    core::utf8_to_utf16_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
    bool success = (string_offset == string.size());
    if (ARCHON_LIKELY(success)) {
        buffer_offset = buffer_offset_2;
        return true;
    }
    ARCHON_ASSERT(string_offset < string.size());
    return false;
}


template<class C, class D, class T, class U>
inline bool try_utf16_to_utf8(core::StringSpan<C> string, core::Buffer<D>& buffer, std::size_t& buffer_offset)
{
    std::size_t string_offset = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    core::utf16_to_utf8_a<C, D, T, U>(string, string_offset, buffer, buffer_offset_2); // Throws
    bool success = (string_offset == string.size());
    if (ARCHON_LIKELY(success)) {
        buffer_offset = buffer_offset_2;
        return true;
    }
    ARCHON_ASSERT(string_offset < string.size());
    return false;
}


template<class C, class D, class T, class U>
void encode_utf8_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                      bool& in_exhausted, bool& error) noexcept
{
    using char_type_1 = C;
    using char_type_2 = D;

    using traits_type_1 = T;
    using traits_type_2 = U;

    using int_type_1 = typename traits_type_1::int_type;
    using int_type_2 = typename traits_type_2::int_type;

    static_assert(!std::is_const_v<char_type_1> && !std::is_volatile_v<char_type_1>);
    static_assert(!std::is_const_v<char_type_2> && !std::is_volatile_v<char_type_2>);

    static_assert(core::num_value_bits<int_type_2>() >= 8);
    static_assert(traits_type_2::to_int_type(traits_type_2::to_char_type(0xFF)) == 0xFF);

    static_assert(!impl::valid_unicode_code_point(traits_type_1::eof()));
    static_assert(traits_type_2::eof() < 0 || traits_type_2::eof() > 0xFF);

    ARCHON_ASSERT(in_offset <= in.size());
    ARCHON_ASSERT(out_offset <= out.size());

    const char_type_1* i_1 = in.data() + in_offset;
    const char_type_1* end_1 = in.data() + in.size();

    char_type_2* i_2 = out.data() + out_offset;
    char_type_2* end_2 = out.data() + out.size();

    for (;;) {
        if (ARCHON_LIKELY(i_1 < end_1)) {
            using promoted_type = core::promoted_type<int_type_1>;
            promoted_type v = core::promote(traits_type_1::to_int_type(*i_1));
            if (ARCHON_LIKELY(v < 0x80)) {
                // UTF-8 layout: 0xxxxxxx (7 payload bits)
                // Code point range: U+0000 -> U+007F
                if (ARCHON_LIKELY(end_2 - i_2 >= 1)) {
                    *i_2++ = traits_type_2::to_char_type(int_type_2(v));
                    ++i_1;
                    continue;
                }
                // Output exhausted
                break;
            }
            if (ARCHON_LIKELY(v < 0x800)) {
                // UTF-8 layout: 110xxxxx 10xxxxxx (11 payload bits)
                // Code point range: U+0080 -> U+07FF
                if (ARCHON_LIKELY(end_2 - i_2 >= 2)) {
                    *i_2++ = traits_type_2::to_char_type(int_type_2(0xC0 + v / 0x40));
                    *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v % 0x40));
                    ++i_1;
                    continue;
                }
                // Output exhausted
                break;
            }
            if (ARCHON_LIKELY(v < 0xFFFE)) {
                // UTF-8 layout: 1110xxxx 10xxxxxx 10xxxxxx (16 payload bits)
                // Code point range: U+0800 -> U+FFFF
                if (ARCHON_LIKELY(v < 0xD800 || v >= 0xE000)) {
                    if (ARCHON_LIKELY(end_2 - i_2 >= 3)) {
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0xE0 + v / 0x1000));
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v / 0x40 % 0x40));
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v % 0x40));
                        ++i_1;
                        continue;
                    }
                    // Output exhausted
                    break;
                }
                error = true; // Code point in surrogate range
                break;
            }
            if (ARCHON_LIKELY(v >= 0x10000)) {
                if (ARCHON_LIKELY(v < 0x110000)) {
                    // UTF-8 layout: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (21 payload bits)
                    // Code point range: U+010000 -> U+10FFFF
                    if (ARCHON_LIKELY(end_2 - i_2 >= 4)) {
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0xF0 + v / 0x40000));
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v / 0x1000 % 0x40));
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v / 0x40 % 0x40));
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v % 0x40));
                        ++i_1;
                        continue;
                    }
                    // Output exhausted
                    break;
                }
                error = true; // Code point out of range
                break;
            }
            error = true; // Non-character code point
            break;
        }
        in_exhausted = true;
        break;
    }

    in_offset  = std::size_t(i_1 - in.data());
    out_offset = std::size_t(i_2 - out.data());
}


template<class C, class D, class T, class U>
void decode_utf8_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                      bool& in_exhausted, bool& error) noexcept
{
    using char_type_1 = C;
    using char_type_2 = D;

    using traits_type_1 = T;
    using traits_type_2 = U;

    using int_type_1 = typename traits_type_1::int_type;
    using int_type_2 = typename traits_type_2::int_type;

    constexpr bool reduced_range = (core::int_inner_width<char_type_2>() < 21);
    constexpr int_type_2 max = (reduced_range ? 0xFFFD : 0x10FFFF);

    static_assert(!std::is_const_v<char_type_1> && !std::is_volatile_v<char_type_1>);
    static_assert(!std::is_const_v<char_type_2> && !std::is_volatile_v<char_type_2>);

    static_assert(core::num_value_bits<int_type_1>() >= 8);
    static_assert(core::num_value_bits<int_type_2>() >= 16);

    static_assert(traits_type_1::to_int_type(traits_type_1::to_char_type(0xFF)) == 0xFF);
    static_assert(traits_type_2::to_int_type(traits_type_2::to_char_type(max)) == max);

    static_assert(traits_type_1::eof() < 0 || traits_type_1::eof() > 0xFF);
    static_assert(traits_type_2::eof() < 0 || traits_type_2::eof() > max);

    ARCHON_ASSERT(in_offset <= in.size());
    ARCHON_ASSERT(out_offset <= out.size());

    const char_type_1* i_1 = in.data() + in_offset;
    const char_type_1* end_1 = in.data() + in.size();

    char_type_2* i_2 = out.data() + out_offset;
    char_type_2* end_2 = out.data() + out.size();

    for (;;) {
        if (ARCHON_LIKELY(end_1 - i_1 >= 1)) {
            using promoted_type = core::promoted_type<int_type_1>;
            promoted_type v_1 = core::promote(traits_type_1::to_int_type(i_1[0]));
            if (ARCHON_LIKELY(v_1 < 0x80)) {
                // UTF-8 layout: 0xxxxxxx (7 payload bits)
                // Code point range: U+0000 -> U+007F
                if (ARCHON_LIKELY(i_2 < end_2)) {
                    *i_2++ = traits_type_2::to_char_type(int_type_2(v_1));
                    i_1 += 1;
                    continue;
                }
                // Output exhausted
                break;
            }
            if (ARCHON_LIKELY(v_1 >= 0xC0)) {
                if (ARCHON_LIKELY(v_1 < 0xE0)) {
                    // UTF-8 layout: 110xxxxx 10xxxxxx (11 payload bits)
                    // Code point range: U+0080 -> U+07FF
                    if (ARCHON_LIKELY(end_1 - i_1 >= 2)) {
                        promoted_type v_2 = core::promote(traits_type_1::to_int_type(i_1[1]));
                        if (ARCHON_LIKELY((v_2 & 0xC0) == 0x80)) {
                            promoted_type v = (((v_1 & 0x1F) << 6) | (v_2 & 0x3F));
                            if (ARCHON_LIKELY(v >= 0x80)) {
                                if (ARCHON_LIKELY(i_2 < end_2)) {
                                    *i_2++ = traits_type_2::to_char_type(int_type_2(v));
                                    i_1 += 2;
                                    continue;
                                }
                                // Output exhausted
                                break;
                            }
                            error = true; // Overlong encoding
                            break;
                        }
                        error = true; // Invalid continuation byte
                        break;
                    }
                    in_exhausted = true; // Incomplete UTF-8 sequence
                    break;
                }
                if (ARCHON_LIKELY(v_1 < 0xF0)) {
                    // UTF-8 layout: 1110xxxx 10xxxxxx 10xxxxxx (16 payload bits)
                    // Code point range: U+0800 -> U+FFFF
                    if (ARCHON_LIKELY(end_1 - i_1 >= 3)) {
                        promoted_type v_2 = core::promote(traits_type_1::to_int_type(i_1[1]));
                        promoted_type v_3 = core::promote(traits_type_1::to_int_type(i_1[2]));
                        if (ARCHON_LIKELY((v_2 & 0xC0) == 0x80 && (v_3 & 0xC0) == 0x80)) {
                            using type = decltype(promoted_type() + std::uint_least16_t());
                            type v = ((type(v_1 & 0x0F) << 12) | ((v_2 & 0x3F) << 6) | (v_3 & 0x3F));
                            if (ARCHON_LIKELY(v >= 0x800)) {
                                if (ARCHON_LIKELY(v < 0xD800 || v >= 0xE000)) {
                                    if (ARCHON_LIKELY(v < 0xFFFE)) {
                                        if (ARCHON_LIKELY(i_2 < end_2)) {
                                            *i_2++ = traits_type_2::to_char_type(int_type_2(v));
                                            i_1 += 3;
                                            continue;
                                        }
                                        // Output exhausted
                                        break;
                                    }
                                    error = true; // Non-character code point
                                    break;
                                }
                                error = true; // Code point in surrogate range
                                break;
                            }
                            error = true; // Overlong encoding
                            break;
                        }
                        error = true; // Invalid continuation byte
                        break;
                    }
                    in_exhausted = true; // Incomplete UTF-8 sequence
                    break;
                }
                if (ARCHON_LIKELY(v_1 < 0xF8)) {
                    // UTF-8 layout: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (21 payload bits)
                    // Code point range: U+010000 -> U+10FFFF
                    if (ARCHON_LIKELY(end_1 - i_1 >= 4)) {
                        promoted_type v_2 = core::promote(traits_type_1::to_int_type(i_1[1]));
                        promoted_type v_3 = core::promote(traits_type_1::to_int_type(i_1[2]));
                        promoted_type v_4 = core::promote(traits_type_1::to_int_type(i_1[3]));
                        if (ARCHON_LIKELY((v_2 & 0xC0) == 0x80 && (v_3 & 0xC0) == 0x80) && (v_4 & 0xC0) == 0x80) {
                            using type = decltype(promoted_type() + std::int_least32_t());
                            type v = ((type(v_1 & 0x07) << 18) | (type(v_2 & 0x3F) << 12) |
                                      ((v_3 & 0x3F) << 6) | (v_4 & 0x3F));
                            if (ARCHON_LIKELY(v >= 0x10000)) {
                                if (ARCHON_LIKELY(v < 0x110000)) {
                                    if constexpr (!reduced_range) {
                                        if (ARCHON_LIKELY(i_2 < end_2)) {
                                            *i_2++ = traits_type_2::to_char_type(int_type_2(v));
                                            i_1 += 4;
                                            continue;
                                        }
                                        // Output exhausted
                                        break;
                                    }
                                    else {
                                        error = true; // Unrepresentable code point
                                        break;
                                    }
                                }
                                error = true; // Code point out of range
                                break;
                            }
                            error = true; // Overlong encoding
                            break;
                        }
                        error = true; // Invalid continuation byte
                        break;
                    }
                    in_exhausted = true; // Incomplete UTF-8 sequence
                    break;
                }
                error = true; // Invalid first byte of UTF-8 sequence or byte value out of range
                break;
            }
            error = true; // Invalid first byte of UTF-8 sequence
            break;
        }
        in_exhausted = true;
        break;
    }

    in_offset  = std::size_t(i_1 - in.data());
    out_offset = std::size_t(i_2 - out.data());
}


template<class C, class D, class T, class U>
bool decode_utf8_incr_l(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                        bool end_of_input) noexcept
{
    using char_type = D;
    using traits_type = U;
    char_type replacement = traits_type::to_char_type(0xFFFD);
    bool in_exhausted = {};
    bool error = {};
    for (;;) {
        core::decode_utf8_incr<C, D, T, U>(in, out, in_offset, out_offset, in_exhausted, error);
        if (ARCHON_LIKELY(in_exhausted)) {
            if (ARCHON_LIKELY(in_offset == in.size() || !end_of_input ))
                return true;
            if (ARCHON_LIKELY(out_offset < out.size())) {
                in_offset = in.size();
                out[out_offset] = replacement;
                out_offset += 1;
                return true;
            }
            return false;
        }
        if (ARCHON_LIKELY(!error)) {
            ARCHON_ASSERT(out_offset == out.size());
            return false;
        }
        ARCHON_ASSERT(in_offset < in.size());
        if (ARCHON_LIKELY(out_offset < out.size())) {
            out[out_offset] = replacement;
            out_offset += 1;
            in_offset += 1;
            core::resync_utf8<C, T>(in, in_offset);
            continue;
        }
        return false;
    }
}


template<class C, class D, class T, class U>
void encode_utf16_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                       bool& in_exhausted, bool& error) noexcept
{
    using char_type_1 = C;
    using char_type_2 = D;

    using traits_type_1 = T;
    using traits_type_2 = U;

    using int_type_1 = typename traits_type_1::int_type;
    using int_type_2 = typename traits_type_2::int_type;

    static_assert(!std::is_const_v<char_type_1> && !std::is_volatile_v<char_type_1>);
    static_assert(!std::is_const_v<char_type_2> && !std::is_volatile_v<char_type_2>);

    static_assert(core::num_value_bits<int_type_2>() >= 16);
    static_assert(traits_type_2::to_int_type(traits_type_2::to_char_type(0xFFFD)) == 0xFFFD);

    static_assert(!impl::valid_unicode_code_point(traits_type_1::eof()));
    static_assert(traits_type_2::eof() < 0 || traits_type_2::eof() > 0xFFFD);

    ARCHON_ASSERT(in_offset <= in.size());
    ARCHON_ASSERT(out_offset <= out.size());

    const char_type_1* i_1 = in.data() + in_offset;
    const char_type_1* end_1 = in.data() + in.size();

    char_type_2* i_2 = out.data() + out_offset;
    char_type_2* end_2 = out.data() + out.size();

    for (;;) {
        if (ARCHON_LIKELY(i_1 < end_1)) {
            using promoted_type = core::promoted_type<int_type_1>;
            promoted_type v = core::promote(traits_type_1::to_int_type(*i_1));
            if (ARCHON_LIKELY(v < 0xFFFE)) {
                // Code point range: U+0000 -> U+FFFF
                if (ARCHON_LIKELY(v < 0xD800 || v >= 0xE000)) {
                    if (ARCHON_LIKELY(end_2 - i_2 >= 1)) {
                        *i_2++ = traits_type_2::to_char_type(int_type_2(v));
                        ++i_1;
                        continue;
                    }
                    // Output exhausted
                    break;
                }
                error = true; // Code point in surrogate range
                break;
            }
            if (ARCHON_LIKELY(v >= 0x10000)) {
                if (ARCHON_LIKELY(v < 0x110000)) {
                    // Code point range: U+010000 -> U+10FFFF
                    if (ARCHON_LIKELY(end_2 - i_2 >= 2)) {
                        v -= 0x10000;
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0xD800 + v / 0x400));
                        *i_2++ = traits_type_2::to_char_type(int_type_2(0xDC00 + v % 0x400));
                        ++i_1;
                        continue;
                    }
                    // Output exhausted
                    break;
                }
                error = true; // Code point out of range
                break;
            }
            error = true; // Non-character code point
            break;
        }
        in_exhausted = true;
        break;
    }

    in_offset  = std::size_t(i_1 - in.data());
    out_offset = std::size_t(i_2 - out.data());
}


template<class C, class D, class T, class U>
void decode_utf16_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                       bool& in_exhausted, bool& error) noexcept
{
    using char_type_1 = C;
    using char_type_2 = D;

    using traits_type_1 = T;
    using traits_type_2 = U;

    using int_type_1 = typename traits_type_1::int_type;
    using int_type_2 = typename traits_type_2::int_type;

    constexpr bool reduced_range = (core::int_inner_width<char_type_2>() < 21);
    constexpr int_type_2 max = (reduced_range ? 0xFFFD : 0x10FFFF);

    static_assert(!std::is_const_v<char_type_1> && !std::is_volatile_v<char_type_1>);
    static_assert(!std::is_const_v<char_type_2> && !std::is_volatile_v<char_type_2>);

    static_assert(core::num_value_bits<int_type_1>() >= 16);
    static_assert(core::num_value_bits<int_type_2>() >= 21);

    static_assert(traits_type_1::to_int_type(traits_type_1::to_char_type(0xFFFD)) == 0xFFFD);
    static_assert(traits_type_2::to_int_type(traits_type_2::to_char_type(max)) == max);

    static_assert(traits_type_1::eof() < 0 || traits_type_1::eof() > 0xFFFD);
    static_assert(traits_type_2::eof() < 0 || traits_type_2::eof() > max);

    ARCHON_ASSERT(in_offset <= in.size());
    ARCHON_ASSERT(out_offset <= out.size());

    const char_type_1* i_1 = in.data() + in_offset;
    const char_type_1* end_1 = in.data() + in.size();

    char_type_2* i_2 = out.data() + out_offset;
    char_type_2* end_2 = out.data() + out.size();

    for (;;) {
        if (ARCHON_LIKELY(end_1 - i_1 >= 1)) {
            using promoted_type = core::promoted_type<int_type_1>;
            promoted_type v_1 = core::promote(traits_type_1::to_int_type(i_1[0]));
            if (ARCHON_LIKELY(v_1 < 0x10000)) {
                if (ARCHON_LIKELY(v_1 < 0xD800 || v_1 >= 0xE000)) {
                    // Code point range: U+0000 -> U+FFFF
                    if (ARCHON_LIKELY(v_1 < 0xFFFE)) {
                        if (ARCHON_LIKELY(i_2 < end_2)) {
                            *i_2++ = traits_type_2::to_char_type(int_type_2(v_1));
                            i_1 += 1;
                            continue;
                        }
                        // Output exhausted
                        break;
                    }
                    error = true; // Non-character code point
                    break;
                }
                if (ARCHON_LIKELY(v_1 < 0xDC00)) {
                    // Code point range: U+010000 -> U+10FFFF
                    if (ARCHON_LIKELY(end_1 - i_1 >= 2)) {
                        promoted_type v_2 = core::promote(traits_type_1::to_int_type(i_1[1]));
                        if (ARCHON_LIKELY(v_2 >= 0xDC00 && v_2 < 0xE000)) {
                            using type = decltype(promoted_type() + std::int_least32_t());
                            type v = 0x10000 + ((type(v_1 - 0xD800) << 10) | (v_2 - 0xDC00));
                            if constexpr (!reduced_range) {
                                if (ARCHON_LIKELY(i_2 < end_2)) {
                                    *i_2++ = traits_type_2::to_char_type(int_type_2(v));
                                    i_1 += 2;
                                    continue;
                                }
                                // Output exhausted
                                break;
                            }
                            else {
                                error = true; // Unrepresentable code point
                                break;
                            }
                        }
                        error = true; // Invalid second half of surrogate pair
                        break;
                    }
                    in_exhausted = true; // Incomplete surrogate pair
                    break;
                }
                error = true; // Stray second half of surrogate pair
                break;
            }
            error = true; // Code unit out of range
            break;
        }
        in_exhausted = true;
        break;
    }

    in_offset  = std::size_t(i_1 - in.data());
    out_offset = std::size_t(i_2 - out.data());
}


template<class C, class D, class T, class U>
void utf8_to_utf16_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                        bool& in_exhausted, bool& error) noexcept
{
    using char_type_1 = C;
    using char_type_2 = D;

    using traits_type_1 = T;
    using traits_type_2 = U;

    using int_type_1 = typename traits_type_1::int_type;
    using int_type_2 = typename traits_type_2::int_type;

    static_assert(!std::is_const_v<char_type_1> && !std::is_volatile_v<char_type_1>);
    static_assert(!std::is_const_v<char_type_2> && !std::is_volatile_v<char_type_2>);

    static_assert(core::num_value_bits<int_type_1>() >= 8);
    static_assert(core::num_value_bits<int_type_2>() >= 16);

    static_assert(traits_type_1::to_int_type(traits_type_1::to_char_type(0xFF)) == 0xFF);
    static_assert(traits_type_2::to_int_type(traits_type_2::to_char_type(0xFFFD)) == 0xFFFD);

    static_assert(traits_type_1::eof() < 0 || traits_type_1::eof() > 0xFF);
    static_assert(traits_type_2::eof() < 0 || traits_type_2::eof() > 0xFFFD);

    ARCHON_ASSERT(in_offset <= in.size());
    ARCHON_ASSERT(out_offset <= out.size());

    const char_type_1* i_1 = in.data() + in_offset;
    const char_type_1* end_1 = in.data() + in.size();

    char_type_2* i_2 = out.data() + out_offset;
    char_type_2* end_2 = out.data() + out.size();

    for (;;) {
        if (ARCHON_LIKELY(end_1 - i_1 >= 1)) {
            using promoted_type = core::promoted_type<int_type_1>;
            promoted_type v_1 = core::promote(traits_type_1::to_int_type(i_1[0]));
            if (ARCHON_LIKELY(v_1 < 0x80)) {
                // UTF-8 layout: 0xxxxxxx (7 payload bits)
                // Code point range: U+0000 -> U+007F
                if (ARCHON_LIKELY(end_2 - i_2 >= 1)) {
                    *i_2++ = traits_type_2::to_char_type(int_type_2(v_1));
                    i_1 += 1;
                    continue;
                }
                // Output exhausted
                break;
            }
            if (ARCHON_LIKELY(v_1 >= 0xC0)) {
                if (ARCHON_LIKELY(v_1 < 0xE0)) {
                    // UTF-8 layout: 110xxxxx 10xxxxxx (11 payload bits)
                    // Code point range: U+0080 -> U+07FF
                    if (ARCHON_LIKELY(end_1 - i_1 >= 2)) {
                        promoted_type v_2 = core::promote(traits_type_1::to_int_type(i_1[1]));
                        if (ARCHON_LIKELY((v_2 & 0xC0) == 0x80)) {
                            promoted_type v = (((v_1 & 0x1F) << 6) | (v_2 & 0x3F));
                            if (ARCHON_LIKELY(v >= 0x80)) {
                                if (ARCHON_LIKELY(end_2 - i_2 >= 1)) {
                                    *i_2++ = traits_type_2::to_char_type(int_type_2(v));
                                    i_1 += 2;
                                    continue;
                                }
                                // Output exhausted
                                break;
                            }
                            error = true; // Overlong encoding
                            break;
                        }
                        error = true; // Invalid continuation byte
                        break;
                    }
                    in_exhausted = true; // Incomplete UTF-8 sequence
                    break;
                }
                if (ARCHON_LIKELY(v_1 < 0xF0)) {
                    // UTF-8 layout: 1110xxxx 10xxxxxx 10xxxxxx (16 payload bits)
                    // Code point range: U+0800 -> U+FFFF
                    if (ARCHON_LIKELY(end_1 - i_1 >= 3)) {
                        promoted_type v_2 = core::promote(traits_type_1::to_int_type(i_1[1]));
                        promoted_type v_3 = core::promote(traits_type_1::to_int_type(i_1[2]));
                        if (ARCHON_LIKELY((v_2 & 0xC0) == 0x80 && (v_3 & 0xC0) == 0x80)) {
                            using type = decltype(promoted_type() + std::uint_least16_t());
                            type v = ((type(v_1 & 0x0F) << 12) | ((v_2 & 0x3F) << 6) | (v_3 & 0x3F));
                            if (ARCHON_LIKELY(v >= 0x800)) {
                                if (ARCHON_LIKELY(v < 0xD800 || v >= 0xE000)) {
                                    if (ARCHON_LIKELY(v < 0xFFFE)) {
                                        if (ARCHON_LIKELY(end_2 - i_2 >= 1)) {
                                            *i_2++ = traits_type_2::to_char_type(int_type_2(v));
                                            i_1 += 3;
                                            continue;
                                        }
                                        // Output exhausted
                                        break;
                                    }
                                    error = true; // Non-character code point
                                    break;
                                }
                                error = true; // Code point in surrogate range
                                break;
                            }
                            error = true; // Overlong encoding
                            break;
                        }
                        error = true; // Invalid continuation byte
                        break;
                    }
                    in_exhausted = true; // Incomplete UTF-8 sequence
                    break;
                }
                if (ARCHON_LIKELY(v_1 < 0xF8)) {
                    // UTF-8 layout: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (21 payload bits)
                    // Code point range: U+010000 -> U+10FFFF
                    if (ARCHON_LIKELY(end_1 - i_1 >= 4)) {
                        promoted_type v_2 = core::promote(traits_type_1::to_int_type(i_1[1]));
                        promoted_type v_3 = core::promote(traits_type_1::to_int_type(i_1[2]));
                        promoted_type v_4 = core::promote(traits_type_1::to_int_type(i_1[3]));
                        if (ARCHON_LIKELY((v_2 & 0xC0) == 0x80 && (v_3 & 0xC0) == 0x80) && (v_4 & 0xC0) == 0x80) {
                            using type = decltype(promoted_type() + std::int_least32_t());
                            type v = ((type(v_1 & 0x07) << 18) | (type(v_2 & 0x3F) << 12) |
                                      ((v_3 & 0x3F) << 6) | (v_4 & 0x3F));
                            if (ARCHON_LIKELY(v >= 0x10000)) {
                                if (ARCHON_LIKELY(v < 0x110000)) {
                                    if (ARCHON_LIKELY(end_2 - i_2 >= 2)) {
                                        v -= 0x10000;
                                        *i_2++ = traits_type_2::to_char_type(int_type_2(0xD800 + v / 0x400));
                                        *i_2++ = traits_type_2::to_char_type(int_type_2(0xDC00 + v % 0x400));
                                        i_1 += 4;
                                        continue;
                                    }
                                    // Output exhausted
                                    break;
                                }
                                error = true; // Code point out of range
                                break;
                            }
                            error = true; // Overlong encoding
                            break;
                        }
                        error = true; // Invalid continuation byte
                        break;
                    }
                    in_exhausted = true; // Incomplete UTF-8 sequence
                    break;
                }
                error = true; // Invalid first byte of UTF-8 sequence or byte value out of range
                break;
            }
            error = true; // Invalid first byte of UTF-8 sequence
            break;
        }
        in_exhausted = true;
        break;
    }

    in_offset  = std::size_t(i_1 - in.data());
    out_offset = std::size_t(i_2 - out.data());
}


template<class C, class D, class T, class U>
void utf16_to_utf8_incr(core::Span<const C> in, core::Span<D> out, std::size_t& in_offset, std::size_t& out_offset,
                        bool& in_exhausted, bool& error) noexcept
{
    using char_type_1 = C;
    using char_type_2 = D;

    using traits_type_1 = T;
    using traits_type_2 = U;

    using int_type_1 = typename traits_type_1::int_type;
    using int_type_2 = typename traits_type_2::int_type;

    static_assert(!std::is_const_v<char_type_1> && !std::is_volatile_v<char_type_1>);
    static_assert(!std::is_const_v<char_type_2> && !std::is_volatile_v<char_type_2>);

    static_assert(core::num_value_bits<int_type_1>() >= 16);
    static_assert(core::num_value_bits<int_type_2>() >= 8);

    static_assert(traits_type_1::to_int_type(traits_type_1::to_char_type(0xFFFD)) == 0xFFFD);
    static_assert(traits_type_2::to_int_type(traits_type_2::to_char_type(0xFF)) == 0xFF);

    static_assert(traits_type_1::eof() < 0 || traits_type_1::eof() > 0xFFFD);
    static_assert(traits_type_2::eof() < 0 || traits_type_2::eof() > 0xFF);

    ARCHON_ASSERT(in_offset <= in.size());
    ARCHON_ASSERT(out_offset <= out.size());

    const char_type_1* i_1 = in.data() + in_offset;
    const char_type_1* end_1 = in.data() + in.size();

    char_type_2* i_2 = out.data() + out_offset;
    char_type_2* end_2 = out.data() + out.size();

    for (;;) {
        if (ARCHON_LIKELY(end_1 - i_1 >= 1)) {
            using promoted_type = core::promoted_type<int_type_1>;
            promoted_type v_1 = core::promote(traits_type_1::to_int_type(i_1[0]));
            if (ARCHON_LIKELY(v_1 < 0x80)) {
                // UTF-8 layout: 0xxxxxxx (7 payload bits)
                // Code point range: U+0000 -> U+007F
                if (ARCHON_LIKELY(end_2 - i_2 >= 1)) {
                    *i_2++ = traits_type_2::to_char_type(int_type_2(v_1));
                    i_1 += 1;
                    continue;
                }
                // Output exhausted
                break;
            }
            if (ARCHON_LIKELY(v_1 < 0x800)) {
                // UTF-8 layout: 110xxxxx 10xxxxxx (11 payload bits)
                // Code point range: U+0080 -> U+07FF
                if (ARCHON_LIKELY(end_2 - i_2 >= 2)) {
                    *i_2++ = traits_type_2::to_char_type(int_type_2(0xC0 + v_1 / 0x40));
                    *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v_1 % 0x40));
                    i_1 += 1;
                    continue;
                }
                // Output exhausted
                break;
            }
            if (ARCHON_LIKELY(v_1 < 0x10000)) {
                if (ARCHON_LIKELY(v_1 < 0xD800 || v_1 >= 0xE000)) {
                    // UTF-8 layout: 1110xxxx 10xxxxxx 10xxxxxx (16 payload bits)
                    // Code point range: U+0800 -> U+FFFF
                    if (ARCHON_LIKELY(v_1 < 0xFFFE)) {
                        if (ARCHON_LIKELY(end_2 - i_2 >= 3)) {
                            *i_2++ = traits_type_2::to_char_type(int_type_2(0xE0 + v_1 / 0x1000));
                            *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v_1 / 0x40 % 0x40));
                            *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v_1 % 0x40));
                            i_1 += 1;
                            continue;
                        }
                        // Output exhausted
                        break;
                    }
                    error = true; // Non-character code point
                    break;
                }
                if (ARCHON_LIKELY(v_1 < 0xDC00)) {
                    // UTF-8 layout: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (21 payload bits)
                    // Code point range: U+010000 -> U+10FFFF
                    if (ARCHON_LIKELY(end_1 - i_1 >= 2)) {
                        promoted_type v_2 = core::promote(traits_type_1::to_int_type(i_1[1]));
                        if (ARCHON_LIKELY(v_2 >= 0xDC00 && v_2 < 0xE000)) {
                            using type = decltype(promoted_type() + std::int_least32_t());
                            type v = 0x10000 + ((type(v_1 - 0xD800) << 10) | (v_2 - 0xDC00));
                            if (ARCHON_LIKELY(end_2 - i_2 >= 4)) {
                                *i_2++ = traits_type_2::to_char_type(int_type_2(0xF0 + v / 0x40000));
                                *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v / 0x1000 % 0x40));
                                *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v / 0x40 % 0x40));
                                *i_2++ = traits_type_2::to_char_type(int_type_2(0x80 + v % 0x40));
                                i_1 += 2;
                                continue;
                            }
                            // Output exhausted
                            break;
                        }
                        error = true; // Invalid second half of surrogate pair
                        break;
                    }
                    in_exhausted = true; // Incomplete surrogate pair
                    break;
                }
                error = true; // Stray second half of surrogate pair
                break;
            }
            error = true; // Code unit out of range
            break;
        }
        in_exhausted = true;
        break;
    }

    in_offset  = std::size_t(i_1 - in.data());
    out_offset = std::size_t(i_2 - out.data());
}


template<class C, class T> void resync_utf8(core::Span<const C> in, std::size_t& in_offset) noexcept
{
    using char_type = C;
    using traits_type = T;

    using int_type = typename traits_type::int_type;

    static_assert(!std::is_const_v<char_type> && !std::is_volatile_v<char_type>);
    static_assert(core::num_value_bits<int_type>() >= 8);
    static_assert(traits_type::to_int_type(traits_type::to_char_type(0xFF)) == 0xFF);
    static_assert(traits_type::eof() < 0 || traits_type::eof() > 0xFF);

    ARCHON_ASSERT(in_offset <= in.size());

    const char_type* i = in.data() + in_offset;
    const char_type* end = in.data() + in.size();

    while (ARCHON_LIKELY(end - i >= 1)) {
        using promoted_type = core::promoted_type<int_type>;
        promoted_type v = core::promote(traits_type::to_int_type(i[0]));
        if (ARCHON_LIKELY(v < 0x80 || (v >= 0xC0 && v < 0xF8)))
            break;
        i += 1; // Discard this byte
    }

    in_offset  = std::size_t(i - in.data());
}


template<class C, class T> void resync_utf16(core::Span<const C> in, std::size_t& in_offset) noexcept
{
    using char_type = C;
    using traits_type = T;

    using int_type = typename traits_type::int_type;

    static_assert(!std::is_const_v<char_type> && !std::is_volatile_v<char_type>);
    static_assert(core::num_value_bits<int_type>() >= 16);
    static_assert(traits_type::to_int_type(traits_type::to_char_type(0xFFFD)) == 0xFFFD);
    static_assert(traits_type::eof() < 0 || traits_type::eof() > 0xFFFD);

    ARCHON_ASSERT(in_offset <= in.size());

    const char_type* i = in.data() + in_offset;
    const char_type* end = in.data() + in.size();

    while (ARCHON_LIKELY(end - i >= 1)) {
        using promoted_type = core::promoted_type<int_type>;
        promoted_type v = core::promote(traits_type::to_int_type(i[0]));
        // Any code unit that is not the second half of a surrogate pair is a valid start of
        // a sequence
        if (ARCHON_LIKELY(v < 0xDC00 || v >= 0xE000))
            break;
        i += 1; // Discard this code unit
    }

    in_offset  = std::size_t(i - in.data());
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_UNICODE_HPP
