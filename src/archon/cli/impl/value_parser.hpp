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

#ifndef ARCHON_X_CLI_X_IMPL_X_VALUE_PARSER_HPP
#define ARCHON_X_CLI_X_IMPL_X_VALUE_PARSER_HPP


#include <type_traits>
#include <optional>
#include <string_view>
#include <string>
#include <locale>
#include <filesystem>

#include <archon/core/string_codec.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/cli/string_holder.hpp>


namespace archon::cli::impl {


template<class C, class T> class ValueParser {
public:
    using char_type   = C;
    using traits_type = T;

    using string_type        = std::basic_string<C, T>;
    using string_view_type   = std::basic_string_view<C, T>;
    using string_holder_type = cli::BasicStringHolder<C, T>;

    ValueParser(string_holder_type&, const std::locale&);

    template<class R> bool parse(string_view_type val, R&& ref);
    template<class V> bool parse(string_view_type val, std::optional<V>& var);

private:
    string_holder_type& m_string_holder;
    const std::locale m_locale;
    core::BasicStringEncoder<C, T> m_string_encoder;
    core::BasicValueParser<C, T> m_value_parser;

    template<class R> bool do_parse(string_view_type val, R&& ref);
    template<class V> bool do_parse(string_view_type val, V& var);
};








// Implementation


template<class C, class T>
inline ValueParser<C, T>::ValueParser(string_holder_type& string_holder, const std::locale& locale)
    : m_string_holder(string_holder)
    , m_locale(locale)
    , m_string_encoder(m_locale) // Throws
    , m_value_parser(m_locale) // Throws
{
}


template<class C, class T>
template<class R> inline bool ValueParser<C, T>::parse(string_view_type val, R&& ref)
{
    return do_parse(val, std::forward<R>(ref)); // Throws
}


template<class C, class T>
template<class V> inline bool ValueParser<C, T>::parse(string_view_type val, std::optional<V>& var)
{
    var.emplace(); // Throws
    return do_parse(val, var.value()); // Throws
}


template<class C, class T>
template<class R> inline bool ValueParser<C, T>::do_parse(string_view_type val, R&& ref)
{
    return m_value_parser.parse(val, std::forward<R>(ref)); // Throws
}


template<class C, class T>
template<class V> inline bool ValueParser<C, T>::do_parse(string_view_type val, V& var)
{
    if constexpr (std::is_same_v<V, string_type>) {
        var = val; // Throws
    }
    else if constexpr (std::is_same_v<V, std::string>) {
        std::string_view val_2 = m_string_encoder.encode_sc(val); // Throws
        var = val_2; // Throws
    }
    else if constexpr (std::is_same_v<V, string_view_type>) {
        var = m_string_holder.add(val); // Throws
    }
    else if constexpr (std::is_same_v<V, std::string_view>) {
        std::string_view val_2 = m_string_encoder.encode_sc(val); // Throws
        var = m_string_holder.add_encoded(val_2); // Throws
    }
    else if constexpr (std::is_same_v<V, std::filesystem::path>) {
        std::string_view val_2 = m_string_encoder.encode_sc(val); // Throws
        // See also impl::ValueFormatter::do_format().
        var = core::make_fs_path_auto(val_2, m_locale); // Throws
    }
    else {
        static_assert(!std::is_pointer_v<V>);
        return m_value_parser.parse(val, var); // Throws
    }
    return true;
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_VALUE_PARSER_HPP
