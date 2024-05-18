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

#ifndef ARCHON_X_DISPLAY_X_NOINST_X_EDID_HPP
#define ARCHON_X_DISPLAY_X_NOINST_X_EDID_HPP


#include <optional>
#include <string_view>
#include <locale>

#include <archon/core/index_range.hpp>
#include <archon/core/string_buffer_contents.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/core/locale.hpp>


namespace archon::display::impl {


struct EdidInfo {
    int major, minor;
    std::optional<core::IndexRange> monitor_name;
};


class EdidParser {
public:
    EdidParser(const std::locale& locale);

    bool parse(std::string_view str, impl::EdidInfo& info, core::StringBufferContents& string_data) const;

private:
    const bool m_is_utf8_locale;
    const bool m_is_unicode_locale;
    core::WideStringCodec m_string_codec;
};








// Implementation


inline EdidParser::EdidParser(const std::locale& locale)
    : m_is_utf8_locale(core::assume_utf8_locale(locale)) // Throws
    , m_is_unicode_locale(core::assume_unicode_locale(locale)) // Throws
    , m_string_codec(locale) // Throws
{
}


} // namespace archon::display::impl

#endif // ARCHON_X_DISPLAY_X_NOINST_X_EDID_HPP
