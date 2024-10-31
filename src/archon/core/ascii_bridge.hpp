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

#ifndef ARCHON_X_CORE_X_ASCII_BRIDGE_HPP
#define ARCHON_X_CORE_X_ASCII_BRIDGE_HPP

/// \file


#include <cstddef>
#include <locale>

#include <archon/core/string_span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_mapper.hpp>


namespace archon::core {


/// \brief Transcoder from multi-byte encoding of locale to ASCII.
///
/// An instance of this class facilitates the transcoding of characters from the native
/// multi-byte encoding of a particular locale to ASCII.
///
/// A transcoder of this type is associated with a particular locale, the locale passed to
/// the constructor. The multi-byte character encoding of that locale must agree on the
/// encoding of the basic character set with the multi-byte encoding of the execution
/// character set, i.e., the encoding of plain character literals. See \ref
/// core::try_map_bcs_to_ascii() for more on this.
///
/// \sa \ref core::try_map_bcs_to_ascii()
/// \sa \ref core::ascii_to_native_mb_transcoder
/// \sa \ref core::native_mb_to_utf8_transcoder
///
class native_mb_to_ascii_transcoder {
public:
    enum class fallback_level;

    /// \{
    ///
    /// \brief Construct transcoder for particular locale.
    ///
    /// These constructors construct a transcoder for the specified locale. That locale must
    /// agree on the encoding of the basic character set with the multi-byte encoding of the
    /// execution character set, i.e., the encoding of plain character literals.
    ///
    /// The overload that takes a fallback level argument is useful for testing and
    /// debugging purposes only. The specified level is entirely ignored unless the core
    /// library is build in debug mode (\ref ARCHON_DEBUG).
    ///
    native_mb_to_ascii_transcoder(const std::locale&);
    native_mb_to_ascii_transcoder(const std::locale&, fallback_level);
    /// \}

    /// \brief Leniently transcode from native encoding to ASCII.
    ///
    /// Given a string expressed in terms of the native encoding of the transcoder, this
    /// function produces the ASCII encoding of that string. The transcoded result is placed
    /// in the specified buffer (\p buffer) starting at the specified offset (\p
    /// buffer_offset).
    ///
    /// This function operates in a lenient manner, meaning that an invalid input sequence
    /// is dealt with by producing a replacement character (or multiple replacement
    /// characters). The replacement character is `?`.
    ///
    /// Upon return, \p buffer_offset will have been set to point one beyond the last byte
    /// of the produced ASCII encoding, or, if the produced ASCII encoding is empty, it will
    /// be left unchanged.
    ///
    /// If this function throws, \p buffer_offset is left unchanged, but the buffer may have
    /// been expanded, and contents after \p buffer_offset may have been clobbered.
    ///
    /// Behavior is undefined if, prior to the invocation, \p buffer_offset is greater than
    /// `buffer.size()`.
    ///
    void transcode_l(core::StringSpan<char> string, core::Buffer<char>& buffer, std::size_t& buffer_offset) const;

private:
    std::locale m_locale;
    core::WideCharMapper m_char_mapper;
    bool m_is_unicode_locale;
    bool m_is_utf8_locale;
};



/// \brief Transcoder from ASCII to multi-byte encoding of locale.
///
/// An instance of this class facilitates the transcoding of characters from ASCII to the
/// native multi-byte encoding of a particular locale.
///
/// A transcoder of this type is associated with a particular locale, the locale passed to
/// the constructor. The multi-byte character encoding of that locale must agree on the
/// encoding of the basic character set with the multi-byte encoding of the execution
/// character set, i.e., the encoding of plain character literals. See \ref
/// core::try_map_bcs_to_ascii() for more on this.
///
/// \sa \ref core::try_map_bcs_to_ascii()
/// \sa \ref core::native_mb_to_ascii_transcoder
/// \sa \ref core::utf8_to_native_mb_transcoder
///
class ascii_to_native_mb_transcoder {
public:
    enum class fallback_level;

    /// \{
    ///
    /// \brief Construct transcoder for particular locale.
    ///
    /// These constructors construct a transcoder for the specified locale. That locale must
    /// agree on the encoding of the basic character set with the multi-byte encoding of the
    /// execution character set, i.e., the encoding of plain character literals.
    ///
    /// The overload that takes a fallback level argument is useful for testing and
    /// debugging purposes only. The specified level is entirely ignored unless the core
    /// library is build in debug mode (\ref ARCHON_DEBUG).
    ///
    ascii_to_native_mb_transcoder(const std::locale&);
    ascii_to_native_mb_transcoder(const std::locale&, fallback_level);
    /// \}

    /// \brief Leniently transcode from ASCII to native encoding.
    ///
    /// Given a string expressed in terms of ASCII, this function produces the corresponding
    /// string expressed in terms of the native encoding of the transcoder. The transcoded
    /// result is placed in the specified buffer (\p buffer) starting at the specified
    /// offset (\p buffer_offset).
    ///
    /// This function operates in a lenient manner, meaning that an invalid input sequence
    /// is dealt with by producing a replacement character (or multiple replacement
    /// characters).
    ///
    /// Upon return, \p buffer_offset will have been set to point one beyond the last byte
    /// of the produced string, or, if the produced string is empty, it will be left
    /// unchanged.
    ///
    /// If this function throws, \p buffer_offset is left unchanged, but the buffer may have
    /// been expanded, and contents after \p buffer_offset may have been clobbered.
    ///
    /// Behavior is undefined if, prior to the invocation, \p buffer_offset is greater than
    /// `buffer.size()`.
    ///
    void transcode_l(core::StringSpan<char> string, core::Buffer<char>& buffer, std::size_t& buffer_offset) const;

private:
    std::locale m_locale;
    bool m_is_unicode_locale;
    bool m_is_utf8_locale;
};



/// \brief Force fallback behavior in debug mode.
///
/// When the core library is compiled in debug mode (\ref ARCHON_DEBUG), the transcoder can
/// be put into one of several fallback modes where it abstains from taking shortcuts in its
/// implementation even when such shortcuts are available. This allows for testing of the
/// fallback behavior. It is intended for that purpose only.
///
enum class native_mb_to_ascii_transcoder::fallback_level {
    normal,                        ///< Normal mode.
    no_unicode_assumption,         ///< Do not assume that locale is Unicode (\ref core::assume_unicode_locale()).
    no_utf8_assumption,            ///< Do not assume that locale is UTF-8 (\ref core::assume_utf8_locale()).
    no_unicode_or_utf8_assumption, ///< Neither assume that locale is Unicode, nor that it is UTF-8.
};


/// \brief Force fallback behavior in debug mode.
///
/// When the core library is compiled in debug mode (\ref ARCHON_DEBUG), the transcoder can
/// be put into one of several fallback modes where it abstains from taking shortcuts in its
/// implementation even when such shortcuts are available. This allows for testing of the
/// fallback behavior. It is intended for that purpose only.
///
enum class ascii_to_native_mb_transcoder::fallback_level {
    normal,                        ///< Normal mode.
    no_unicode_assumption,         ///< Do not assume that locale is Unicode (\ref core::assume_unicode_locale()).
    no_utf8_assumption,            ///< Do not assume that locale is UTF-8 (\ref core::assume_utf8_locale()).
    no_unicode_or_utf8_assumption, ///< Neither assume that locale is Unicode, nor that it is UTF-8.
};








// Implementation


inline native_mb_to_ascii_transcoder::native_mb_to_ascii_transcoder(const std::locale& locale)
    : native_mb_to_ascii_transcoder(locale, fallback_level::normal) // Throws
{
}


inline ascii_to_native_mb_transcoder::ascii_to_native_mb_transcoder(const std::locale& locale)
    : ascii_to_native_mb_transcoder(locale, fallback_level::normal) // Throws
{
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_ASCII_BRIDGE_HPP
