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


#include <cstddef>
#include <cwchar>
#include <array>
#include <string_view>
#include <string>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/char_codec_config.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/basic_character_set.hpp>
#include <archon/core/unicode.hpp>
#include <archon/core/unicode_bridge.hpp>


using namespace archon;


using core::native_mb_to_utf8_transcoder;
using core::utf8_to_native_mb_transcoder;



native_mb_to_utf8_transcoder::native_mb_to_utf8_transcoder(const std::locale& locale)
    : m_locale(locale)
    , m_char_mapper(locale) // Throws
    , m_is_utf8_locale(core::assume_utf8_locale(locale)) // Throws
    , m_is_unicode_locale(core::assume_unicode_locale(locale)) // Throws
{
}


void native_mb_to_utf8_transcoder::transcode_l(core::StringSpan<char> string, core::Buffer<char>& buffer,
                                               std::size_t& buffer_offset) const
{
    bool force_fallback = false;

    if (ARCHON_LIKELY(m_is_utf8_locale && !force_fallback)) {
        buffer.append(string, buffer_offset); // Throws
        return;
    }

    // NOTE: On Windows `wchar_t` is only 16 bits wide and `std::codecvt` decodes to UCS-2
    // instead of to UCS-4. Consequently, any input sequence that would decode to a code
    // point greater than U+FFFF is reported as an error by a non-lenient decoder. Since
    // this is a lenient decoder, each such sequence is decoded as a single `?`.
    core::WideCharCodec::Config config;
    config.lenient = true; // Automatically produce replacement characters for invalid input
    core::WideCharCodec codec(m_locale, config); // Throws

    std::mbstate_t state = {};
    std::size_t string_offset = 0;
    bool end_of_string = true;
    std::size_t buffer_offset_2 = buffer_offset;
    bool error = false;
    std::array<wchar_t, 32> buffer_2;
    std::array<char, 32> buffer_3;
    for (;;) {
        std::size_t buffer_offset_3 = 0;
        bool complete = codec.decode(state, string, string_offset, end_of_string,
                                     buffer_2, buffer_offset_3, error); // Throws
        ARCHON_ASSERT(!error); // Because lenient mode is used
        std::wstring_view string_2 = { buffer_2.data(), buffer_offset_3 };
        if (ARCHON_LIKELY(m_is_unicode_locale && !force_fallback)) {
            core::encode_utf8<wchar_t>(string_2, buffer, buffer_offset_2); // Throws
        }
        else {
            m_char_mapper.narrow(string_2, '\0', buffer_3.data()); // Throws
            std::string_view string_3 = { buffer_3.data(), buffer_offset_3 };
            char replacement[] = {
                std::char_traits<char>::to_char_type(0xEF),
                std::char_traits<char>::to_char_type(0xBF),
                std::char_traits<char>::to_char_type(0xBD),
            };
            std::size_t n = string_3.size();
            for (std::size_t i = 0; i < n; ++i) {
                char ch = string_3[i];
                bool was_narrowable = (ch != '\0' || string_2[i] == wchar_t(0));
                if (ARCHON_LIKELY(was_narrowable)) {
                    char ch_2 = {};
                    if (core::try_map_bcs_to_ascii(ch, ch_2)) {
                        buffer.append_a(ch_2, buffer_offset_2); // Throws
                        continue;
                    }
                }
                buffer.append(replacement, buffer_offset_2); // Throws
            }
        }
        if (ARCHON_LIKELY(!complete))
            continue;
        break;
    }

    buffer_offset = buffer_offset_2;
}



utf8_to_native_mb_transcoder::utf8_to_native_mb_transcoder(const std::locale& locale)
    : m_locale(locale)
    , m_is_utf8_locale(core::assume_utf8_locale(locale)) // Throws
    , m_is_unicode_locale(core::assume_unicode_locale(locale)) // Throws
{
}


void utf8_to_native_mb_transcoder::transcode_l(core::StringSpan<char> string, core::Buffer<char>& buffer,
                                               std::size_t& buffer_offset) const
{
    bool force_fallback = false;

    if (ARCHON_LIKELY(m_is_utf8_locale && !force_fallback)) {
        buffer.append(string, buffer_offset); // Throws
        return;
    }

    // FIXME: Add remarks about wchar_t being 16 bits wide on Windows    

    std::array<wchar_t, 64> buffer_2;
    std::size_t string_offset = 0;
    std::size_t string_size = string.size();
    core::WideCharCodec codec(m_locale); // Throws
    std::mbstate_t state = {};
    std::size_t buffer_offset_3 = 0;
    while (string_offset < string_size) {
        std::size_t buffer_offset_2 = 0;
        for (;;) {
            bool in_exhausted = {};
            bool error = {};
            core::decode_utf8_incr(string, core::Span(buffer_2), string_offset, buffer_offset_2, in_exhausted, error);
            bool error_2 = (in_exhausted ? string_offset < string_size : error);
            if (ARCHON_LIKELY(!error_2 || buffer_offset_2 == buffer_2.size()))
                break;
            ARCHON_ASSERT(string_offset < string_size);
            wchar_t replacement = std::char_traits<wchar_t>::to_char_type(0xFFFD);
            buffer_2[buffer_offset_2] = replacement;
            buffer_offset_2 += 1;
            string_offset += 1;
            core::resync_utf8(string, string_offset);
        }
        std::wstring_view string_2 = { buffer_2.data(), buffer_offset_2 }; // Throws
        if (ARCHON_LIKELY(m_is_unicode_locale && !force_fallback)) {
            std::size_t string_offset_2 = 0;
            bool error_2 = {};
            bool complete = codec.encode(state, string_2, string_offset_2, buffer, buffer_offset_3, error_2); // Throws
            ARCHON_ASSERT(complete);
            ARCHON_ASSERT(string_offset_2 == string_2.size());
        }
        else {
            for (wchar_t ch_1 : string_2) {
                auto val = std::char_traits<wchar_t>::to_int_type(ch_1);
                char ch_2 = '?';
                if (ARCHON_LIKELY(!core::is_negative(val) && val < 128))
                    core::try_map_ascii_to_bcs(char(val), ch_2);
                buffer.append_a(ch_2, buffer_offset_3); // Throws
            }
        }
    }

    buffer_offset = buffer_offset_3;
}
