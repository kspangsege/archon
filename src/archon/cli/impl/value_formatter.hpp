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

#ifndef ARCHON_X_CLI_X_IMPL_X_VALUE_FORMATTER_HPP
#define ARCHON_X_CLI_X_IMPL_X_VALUE_FORMATTER_HPP

/// \file


#include <type_traits>
#include <optional>
#include <string_view>
#include <string>
#include <locale>
#include <ostream>
#include <filesystem>

#include <archon/core/string_codec.hpp>
#include <archon/core/filesystem.hpp>


namespace archon::cli::impl {


template<class C, class T> class ValueFormatter {
public:
    using char_type   = C;
    using traits_type = T;

    using string_type      = std::basic_string<C, T>;
    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;

    ValueFormatter(const std::locale&);

    template<class U> bool format(const U&, ostream_type&);
    template<class U> bool format(const std::optional<U>&, ostream_type&);

private:
    const std::locale m_locale;
    core::BasicStringDecoder<C, T> m_string_decoder;

    template<class U> void do_format(const U&, ostream_type&);
};








// Implementation


template<class C, class T>
inline ValueFormatter<C, T>::ValueFormatter(const std::locale& locale)
    : m_locale(locale)
    , m_string_decoder(m_locale) // Throws
{
}


template<class C, class T>
template<class U> inline bool ValueFormatter<C, T>::format(const U& val, ostream_type& out)
{
    do_format(val, out); // Throws
    return true;
}


template<class C, class T>
template<class U> inline bool ValueFormatter<C, T>::format(const std::optional<U>& val, ostream_type& out)
{
    if (val.has_value()) {
        do_format(val.value(), out); // Throws
        return true;
    }
    return false;
}


template<class C, class T>
template<class U> inline void ValueFormatter<C, T>::do_format(const U& val, ostream_type& out)
{
    if constexpr (std::is_same_v<U, string_type>) {
        out << val; // Throws
    }
    else if constexpr (std::is_same_v<U, std::string>) {
        string_view_type val_2 = m_string_decoder.decode_sc(val); // Throws
        out << val_2; // Throws
    }
    else if constexpr (std::is_same_v<U, string_view_type>) {
        out << val; // Throws
    }
    else if constexpr (std::is_same_v<U, std::string_view>) {
        string_view_type val_2 = m_string_decoder.decode_sc(val); // Throws
        out << val_2; // Throws
    }
    else if constexpr (std::is_same_v<U, std::filesystem::path>) {
        // An advantage of not using the native format here, is that the produced
        // backslashes would be escaped by the quoting parameter substitution `@Q` of
        // impl::HelpFormatter.

        // See also impl::ValueParser::do_parse().
        std::string val_2 = core::path_to_string_generic(val, m_locale); // Throws
        string_view_type val_3 = m_string_decoder.decode_sc(val_2); // Throws
        out << val_3; // Throws
    }
    else {
        static_assert(!std::is_pointer_v<U>);
        out << val; // Throws
    }
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_VALUE_FORMATTER_HPP
