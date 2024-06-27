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

#ifndef ARCHON_X_CORE_X_UNICODE_BRIDGE_HPP
#define ARCHON_X_CORE_X_UNICODE_BRIDGE_HPP

/// \file


#include <cstddef>
#include <locale>

#include <archon/core/string_span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_mapper.hpp>


namespace archon::core {


/// \brief Transcoder from multi-byte encoding of locale to UTF-8.
///
/// An instance of this class facilitates the transcoding of characters from the native
/// multi-byte encoding of a particular locale to UTF-8.
///
/// A transcoder of this type is associated with a particular locale, the locale passed to
/// the constructor. The multi-byte character encoding of that locale must agree on the
/// encoding of the basic character set with the multi-byte encoding of the execution
/// character set, i.e., the encoding of plain character literals. See \ref
/// core::try_map_bcs_to_ascii() for more on this.
///
/// \sa \ref core::try_map_bcs_to_ascii()
/// \sa \ref core::utf8_to_native_mb_transcoder
///
class native_mb_to_utf8_transcoder {
public:
    /// \brief Construct transcoder for particular locale.
    ///
    /// This constructor constructs a transcoder for the specified locale. That locale must
    /// agree on the encoding of the basic character set with the multi-byte encoding of the
    /// execution character set, i.e., the encoding of plain character literals.
    ///
    native_mb_to_utf8_transcoder(const std::locale&);

    /// \brief Leniently transcode from native encoding to UTF-8.
    ///
    /// Given a string expressed in terms of the native encoding of the transcoder, this
    /// function produces the UTF-8 encoding of that string. The transcoded result is placed
    /// in the specified buffer (\p buffer) starting at the specified offset (\p
    /// buffer_offset).
    ///
    /// This function operates in a lenient manner, meaning that an invalid input sequence
    /// is dealt with by producing a Unicode replacement character (or multiple replacement
    /// characters).
    ///
    /// Upon return, \p buffer_offset will have been set to point one beyond the last byte
    /// of the produced UTF-8 encoding, or, if the produced UTF-8 encoding is empty, it will
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
    bool m_is_utf8_locale;
    bool m_is_unicode_locale;
};



/// \brief Transcoder from UTF-8 to multi-byte encoding of locale.
///
/// An instance of this class facilitates the transcoding of characters from UTF-8 to the
/// native multi-byte encoding of a particular locale.
///
/// A transcoder of this type is associated with a particular locale, the locale passed to
/// the constructor. The multi-byte character encoding of that locale must agree on the
/// encoding of the basic character set with the multi-byte encoding of the execution
/// character set, i.e., the encoding of plain character literals. See \ref
/// core::try_map_bcs_to_ascii() for more on this.
///
/// \sa \ref core::try_map_bcs_to_ascii()
/// \sa \ref core::native_mb_to_utf8_transcoder
///
class utf8_to_native_mb_transcoder {
public:
    /// \brief Construct transcoder for particular locale.
    ///
    /// This constructor constructs a transcoder for the specified locale. That locale must
    /// agree on the encoding of the basic character set with the multi-byte encoding of the
    /// execution character set, i.e., the encoding of plain character literals.
    ///
    utf8_to_native_mb_transcoder(const std::locale&);

    /// \brief Leniently transcode from UTF-8 to native encoding.
    ///
    /// Given a string expressed in terms of UTF-8, this function produces the corresponding
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
    bool m_is_utf8_locale;
    bool m_is_unicode_locale;
};


} // namespace archon::core

#endif // ARCHON_X_CORE_X_UNICODE_BRIDGE_HPP
