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

#ifndef ARCHON_X_CORE_X_CHARENC_BRIDGE_HPP
#define ARCHON_X_CORE_X_CHARENC_BRIDGE_HPP

/// \file


#include <cstddef>
#include <locale>

#include <archon/core/string_span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_mapper.hpp>


namespace archon::core {


/// \brief Bridge between locale's character encoding and various other encodings.
///
/// An instance of this class acts as a bridge between the native character encoding for a
/// particular locale and UTF-8. Additionally, it acts as a bridge between the native
/// character encoding and Latin-1, and as a bridge between the native character encoding
/// and ASCII.
///
/// A object of this type is associated with a particular locale, which is the locale passed
/// that was passed to the constructor. The multi-byte character encoding associated with
/// that locale is *the native multi-byte encoding of the bridge*. The application must must
/// ensure that it agrees on the encoding of the basic character set with the multi-byte
/// encoding of the execution character set, i.e., the encoding of plain character
/// literals. See \ref core::try_map_bcs_to_ascii() for more on this.
///
/// \sa \ref core::try_map_bcs_to_ascii(), \ref core::try_map_ascii_to_bcs()
///
class charenc_bridge {
public:
    enum class fallback_level;

    /// \{
    ///
    /// \brief Construct bridge for particular locale.
    ///
    /// These constructors construct a character encoding bridge for the specified
    /// locale. That locale must agree on the encoding of the basic character set with the
    /// multi-byte encoding of the execution character set, i.e., the encoding of plain
    /// character literals (see \ref core::try_map_bcs_to_ascii()).
    ///
    /// The overload that takes a fallback level argument is useful for testing and
    /// debugging purposes only. The specified level is entirely ignored unless the core
    /// library is built in debug mode (\ref ARCHON_DEBUG).
    ///
    charenc_bridge(const std::locale&);
    charenc_bridge(const std::locale&, fallback_level);
    /// \}

    /// \brief Leniently transcode from native multi-byte encoding to UTF-8.
    ///
    /// Given a string expressed in terms of the native multi-byte encoding of the
    /// bridge, this function produces the UTF-8 encoding of that string. The transcoded
    /// result is placed in the specified buffer (\p buffer) starting at the specified
    /// offset (\p buffer_offset).
    ///
    /// This function operates in a lenient manner, meaning that any input character that is
    /// not representable in UTF-8 will be silently replaced by a Unicode replacement
    /// character (or multiple replacement characters).
    ///
    /// Besides being lenient with respect to non-representable characters (see above), this
    /// function also operates in a best-effort manner in the sense that it does what it can
    /// with the information it can get about the multi-byte and wide-character encodings of
    /// the locale that is associated with the bridge. In the best case, all characters that
    /// are representable in UTF-8 will be preserved. In the worst case, only characters
    /// from the basic character set will be preserved (\ref
    /// core::try_map_bcs_to_ascii()). Characters that cannot be preserved will be dealt
    /// with like non-representable characters.
    ///
    /// This function may, or may not detect invalid input, i.e., bytes or byte sequences
    /// that are invalid from the point of view of the native multi-byte encoding of the
    /// bridge. In general, invalid input will be detected when, and only when the transcode
    /// operation is non-trivial. The transcode operation is trivial when the native
    /// multi-byte encoding of the bridge is known by the bridge to be UTF-8 (\ref
    /// core::assume_utf8_locale()). When invalid input is detected, it will be replaced
    /// with one or more Unicode replacement characters.
    ///
    /// Upon return, \p buffer_offset will have been set to point one beyond the last byte
    /// of the produced string, or, if the produced string is empty, it will be left
    /// unchanged.
    ///
    /// If this function throws, \p buffer_offset is left unchanged, but the buffer may have
    /// been expanded, and buffer contents beyond \p buffer_offset may have been clobbered.
    ///
    /// Behavior is undefined if, prior to the invocation, \p buffer_offset is greater than
    /// `buffer.size()`.
    ///
    /// \sa \ref utf8_to_native_mb_l()
    ///
    /// \sa \ref native_mb_to_latin1_l(), \ref native_mb_to_ascii_l()
    ///
    void native_mb_to_utf8_l(core::StringSpan<char> string, core::Buffer<char>& buffer,
                             std::size_t& buffer_offset) const;

    /// \brief Leniently transcode from UTF-8 to native multi-byte encoding.
    ///
    /// Given a string expressed in terms of UTF-8, this function produces the corresponding
    /// string expressed in terms of the native multi-byte encoding of the bridge. The
    /// transcoded result is placed in the specified buffer (\p buffer) starting at the
    /// specified offset (\p buffer_offset).
    ///
    /// This function operates in a lenient manner, meaning that any input character that is
    /// not representable in the native multi-byte encoding of the bridge will be silently
    /// replaced by a replacement character (or multiple replacement characters).
    ///
    /// Besides being lenient with respect to non-representable characters (see above), this
    /// function also operates in a best-effort manner in the sense that it does what it can
    /// with the information it can get about the multi-byte and wide-character encodings of
    /// the locale that is associated with the bridge. In the best case, all characters that
    /// are representable in the native multi-byte encoding of the bridge will be
    /// preserved. In the worst case, only characters from the basic character set will be
    /// preserved (\ref core::try_map_bcs_to_ascii()). Characters that cannot be preserved
    /// will be dealt with like non-representable characters.
    ///
    /// This function may, or may not detect invalid input, i.e., bytes or byte sequences
    /// that are invalid from the point of view of UTF-8. In general, invalid input will be
    /// detected when, and only when the transcode operation is non-trivial. The transcode
    /// operation is trivial when the native multi-byte encoding of the bridge is known by
    /// the bridge to be UTF-8 (\ref core::assume_utf8_locale()). When invalid input is
    /// detected, it will be replaced with one or more Unicode replacement characters.
    ///
    /// Upon return, \p buffer_offset will have been set to point one beyond the last byte
    /// of the produced string, or, if the produced string is empty, it will be left
    /// unchanged.
    ///
    /// If this function throws, \p buffer_offset is left unchanged, but the buffer may have
    /// been expanded, and buffer contents beyond \p buffer_offset may have been clobbered.
    ///
    /// Behavior is undefined if, prior to the invocation, \p buffer_offset is greater than
    /// `buffer.size()`.
    ///
    /// \sa \ref native_mb_to_utf8_l()
    ///
    /// \sa \ref latin1_to_native_mb_l(), \ref ascii_to_native_mb_l()
    ///
    void utf8_to_native_mb_l(core::StringSpan<char> string, core::Buffer<char>& buffer,
                             std::size_t& buffer_offset) const;

    /// \brief Leniently transcode from native multi-byte encoding to Latin-1.
    ///
    /// Given a string expressed in terms of the native multi-byte encoding of the bridge,
    /// this function produces the Latin-1 encoding (see below) of that string. The
    /// transcoded result is placed in the specified buffer (\p buffer) starting at the
    /// specified offset (\p buffer_offset).
    ///
    /// In this context, Latin-1 refers to the character encoding that is the combination of
    /// ISO/IEC 8859-1 (ASCII + Latin supplement) and ISO/IEC 6429 (C1 control
    /// codes). Latin-1 can also be thought of as the first two Unicode blocks (ISO/IEC
    /// 10646), i.e., Basic Latin and Latin-1 Supplement.
    ///
    /// This function has the same effect as \ref native_mb_to_utf8_l() except that the
    /// target character encoding is Latin-1 instead of UTF-8. This means that input
    /// characters, that are not representable in Latin-1, will be replaced by replacement
    /// characters (probably `?`).
    ///
    /// \sa \ref latin1_to_native_mb_l()
    ///
    /// \sa \ref native_mb_to_utf8_l(), \ref native_mb_to_ascii_l()
    ///
    void native_mb_to_latin1_l(core::StringSpan<char> string, core::Buffer<char>& buffer,
                               std::size_t& buffer_offset) const;

    /// \brief Leniently transcode from Latin-1 to native multi-byte encoding.
    ///
    /// Given a string expressed in terms of Latin-1, this function produces the
    /// corresponding string expressed in terms of the native multi-byte encoding of the
    /// bridge. The transcoded result is placed in the specified buffer (\p buffer) starting
    /// at the specified offset (\p buffer_offset).
    ///
    /// See \ref native_mb_to_latin1_l() for the exact meaning of Latin-1 in this context.
    ///
    /// This function has the same effect as \ref utf8_to_native_mb_l() except that the
    /// original character encoding is Latin-1 instead of UTF-8.
    ///
    /// \sa \ref native_mb_to_latin1_l()
    ///
    /// \sa \ref utf8_to_native_mb_l(), \ref ascii_to_native_mb_l()
    ///
    void latin1_to_native_mb_l(core::StringSpan<char> string, core::Buffer<char>& buffer,
                               std::size_t& buffer_offset) const;

    /// \brief Leniently transcode from native multi-byte encoding to ASCII.
    ///
    /// Given a string expressed in terms of the native multi-byte encoding of the bridge,
    /// this function produces the ASCII encoding of that string. The transcoded result is
    /// placed in the specified buffer (\p buffer) starting at the specified offset (\p
    /// buffer_offset).
    ///
    /// This function has the same effect as \ref native_mb_to_utf8_l() except that the
    /// target character encoding is ASCII instead of UTF-8. This means that input
    /// characters, that are not representable in ASCII, will be replaced by replacement
    /// characters (probably `?`).
    ///
    /// \sa \ref ascii_to_native_mb_l()
    ///
    /// \sa \ref native_mb_to_utf8_l(), \ref native_mb_to_latin1_l()
    ///
    void native_mb_to_ascii_l(core::StringSpan<char> string, core::Buffer<char>& buffer,
                              std::size_t& buffer_offset) const;

    /// \brief Leniently transcode from ASCII to native multi-byte encoding.
    ///
    /// Given a string expressed in terms of ASCII, this function produces the corresponding
    /// string expressed in terms of the native multi-byte encoding of the bridge. The
    /// transcoded result is placed in the specified buffer (\p buffer) starting at the
    /// specified offset (\p buffer_offset).
    ///
    /// This function has the same effect as \ref ascii_to_native_mb_l() except that the
    /// original character encoding is ASCII instead of UTF-8.
    ///
    /// \sa \ref native_mb_to_ascii_l()
    ///
    /// \sa \ref utf8_to_native_mb_l(), \ref latin1_to_native_mb_l()
    ///
    void ascii_to_native_mb_l(core::StringSpan<char> string, core::Buffer<char>& buffer,
                              std::size_t& buffer_offset) const;

private:
    std::locale m_locale;
    core::WideCharMapper m_char_mapper;
    bool m_is_ucs_locale;
    bool m_is_utf8_locale;
};



/// \brief Force fallback behavior in debug mode.
///
/// When the core library is compiled in debug mode (\ref ARCHON_DEBUG), the bridge can be
/// put into one of several fallback modes where it abstains from taking shortcuts in its
/// implementation even when such shortcuts are available. This allows for testing of the
/// fallback behavior. It is intended for that purpose only.
///
enum class charenc_bridge::fallback_level {
    normal,                    ///< Normal mode.
    no_ucs_assumption,         ///< Do not assume that locale is UCS (\ref core::assume_ucs_locale()).
    no_utf8_assumption,        ///< Do not assume that locale is UTF-8 (\ref core::assume_utf8_locale()).
    no_ucs_or_utf8_assumption, ///< Neither assume that locale is UCS, nor that it is UTF-8.
};








// Implementation


inline charenc_bridge::charenc_bridge(const std::locale& locale)
    : charenc_bridge(locale, fallback_level::normal) // Throws
{
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_CHARENC_BRIDGE_HPP
