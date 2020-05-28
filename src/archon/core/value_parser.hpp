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

#ifndef ARCHON_X_CORE_X_VALUE_PARSER_HPP
#define ARCHON_X_CORE_X_VALUE_PARSER_HPP

/// \file


#include <type_traits>
#include <utility>
#include <string_view>
#include <string>
#include <locale>
#include <ios>

#include <archon/core/features.h>
#include <archon/core/scope_exit.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/memory_input_stream.hpp>


namespace archon::core {


template<class C, class T> class BasicValueParserSource;




/// \brief Stream-based value parsing.
///
/// This class implements a scheme for obtaining values from their string representations
/// using an input stream. For values of string type, the input stream is bypassed, and the
/// result is simply a copy of the input string.
///
/// FIXME: Explain how integer parsing is improved relative to direct use of an imput                         
/// stream. I.e., string representations of negative values are not accepted for unsigned
/// integer types.
///
/// FIXME: Explain how `parse_value(core::BasicValueParserSource<C, T>& src, V& var)` can be                         
/// defined for a custom type `V` in any namespace (this is similar to how `swap()` can be
/// defined for custom types in any namespace). If such a function is not defined, the
/// stream input operator will be used.
///
/// FIXME: Explain support for adaptors. For example, if `Foo` is an adaptor containing a                         
/// reference to the adapted variable, one would define
/// `parse_value(core::BasicValueParserSource<C, T>& src, const Foo&)`. This will allow Foo
/// objects to be passed by value to \ref parse(). See \ref core::AsList for an example of
/// how this idea can be implemented.
///
template<class C, class T = std::char_traits<C>> class BasicValueParser
    : private core::BasicCharMapper<C, T> {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    class Source;

    /// \brief Construct parser.
    ///
    /// Construct a parser with the specified locale. The input stream, that is used
    /// internally for parsing, will be imbued with this locale.
    ///
    BasicValueParser(const std::locale& locale = {});

    /// \brief Parse string as value.
    ///
    /// This function attempts to parse the specified string as a value of the type implied
    /// by the \p ref argument.
    ///
    /// The \p ref argument will be perfectly forwarded to an invocation of
    /// `parse_value()`. The idea is that if \p ref is an l-value reference, then the type
    /// of the referenced object determines the type of the parsed value, and the resulting
    /// value is assigned to the referenced object. Alternatively, \p ref ccan be an adaptor
    /// object, which can be passed either by value or by reference. In this case, a
    /// corresponding overload of `parse_value()` must exist.
    ///
    /// If no specific overload of `parse_value()` is defined for the specified \p ref
    /// argument, the fallback version of `parse_value()` will be invoked, and it will
    /// attempt to parse the value using \ref BasicValueParserSource::parse().
    ///
    template<class R> bool parse(string_view_type, R&& ref);

private:
    using memory_istream_type = core::BasicMemoryInputStream<C, T>;

    memory_istream_type m_in;
    const char_type m_dash;

    friend class core::BasicValueParserSource<C, T>;
};


using ValueParser     = BasicValueParser<char>;
using WideValueParser = BasicValueParser<wchar_t>;




template<class C, class T = std::char_traits<C>> class BasicValueParserSource {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using char_mapper_type = core::BasicCharMapper<C, T>;

    /// \brief Parse value using stream input operator.
    ///
    /// If the type of the value argument is `std::basic_string<C, T, A>` for some allocator
    /// type `A`, the assigned value will be the entire specified string. Otherwise the
    /// value is obtained using a stream input operator (`in >> value` where `in` is of type
    /// `std::basic_istream<C, T>`).
    ///
    /// If the type of the specified \p value argument is an unsigned integer type, strings
    /// with a leading dash (`-`), i.e., all negative values, will be rejected. Contrast
    /// this with the fact that the C++ standard requres the plain stream input operator for
    /// unsigned integer types to accept negative values.
    ///
    /// If parsing succeeds, this function assigns the parsed value to the \p value
    /// argument, and returns `true`. If parsing fails, this function returns `false` and
    /// leaves \p value "untouched". When the second argument is a string (see above),
    /// parsing always succeeds.
    ///
    template<class V> bool parse(V& var);

    auto string() const noexcept -> string_view_type;

    /// \{
    ///
    /// \brief Parse substring and/or parse as different type.
    ///
    /// Calling `src.delegate(x)` has the same effect as `src.delegate(src.string(), x)`.
    ///
    template<class R> bool delegate(R&& ref);
    template<class R> bool delegate(string_view_type substr, R&& ref);
    /// \}

    template<class R> bool with_modified_locale(const std::locale&, std::locale::category, R&& ref);

    auto widen(char) const -> char_type;

    auto get_char_mapper() noexcept -> const char_mapper_type&;

    auto getloc() const -> std::locale;

private:
    using memory_istream_type = core::BasicMemoryInputStream<C, T>;
    using value_parser_type   = core::BasicValueParser<C, T>;

    value_parser_type& m_parser;
    memory_istream_type& m_in;
    string_view_type m_string;

    BasicValueParserSource(value_parser_type&, memory_istream_type&, string_view_type string) noexcept;

    template<class A> bool do_parse(std::basic_string<C, T, A>& var);
    template<class V> bool do_parse(V& var);

    friend class core::BasicValueParser<C, T>;
};


using ValueParserSource     = BasicValueParserSource<char>;
using WideValueParserSource = BasicValueParserSource<wchar_t>;




template<class C, class T, class V> bool parse_value(core::BasicValueParserSource<C, T>&, V& var);








// Implementation


// ============================ BasicValueParser ============================


template<class C, class T>
inline BasicValueParser<C, T>::BasicValueParser(const std::locale& locale)
    : core::BasicCharMapper<C, T>(locale) // Throws
    , m_dash(this->widen('-')) // Throws
{
    m_in.imbue(locale); // Throws
    m_in.unsetf(std::ios_base::skipws);
}


template<class C, class T>
template<class R> inline bool BasicValueParser<C, T>::parse(string_view_type str, R&& ref)
{
    core::BasicValueParserSource<C, T> src(*this, m_in, str);
    return parse_value(src, std::forward<R>(ref)); // Throws
}



// ============================ BasicValueParserSource ============================


template<class C, class T>
template<class V> inline bool BasicValueParserSource<C, T>::parse(V& var)
{
    return do_parse(var); // Throws
}


template<class C, class T>
inline auto BasicValueParserSource<C, T>::string() const noexcept -> string_view_type
{
    return m_string;
}


template<class C, class T>
template<class R> inline bool BasicValueParserSource<C, T>::delegate(R&& ref)
{
    return delegate(string(), std::forward<R>(ref)); // Throws
}


template<class C, class T>
template<class R> inline bool BasicValueParserSource<C, T>::delegate(string_view_type substr, R&& ref)
{
    core::BasicValueParserSource<C, T> src(m_parser, m_in, substr);
    return parse_value(src, std::forward<R>(ref)); // Throws
}


template<class C, class T>
template<class R>
bool BasicValueParserSource<C, T>::with_modified_locale(const std::locale& loc, std::locale::category cat, R&& ref)
{
    std::locale orig_loc = m_in.getloc(); // Throws
    if constexpr (noexcept(m_in.imbue(orig_loc))) {
        ARCHON_SCOPE_EXIT {
            m_in.imbue(orig_loc);
        };
        m_in.imbue(std::locale(orig_loc, loc, cat)); // Throws
        return parse_value(*this, std::forward<R>(ref)); // Throws
    }
    else {
        memory_istream_type in; // Throws
        in.copyfmt(m_in); // Throws
        in.imbue(std::locale(orig_loc, loc, cat)); // Throws
        BasicValueParserSource src(m_parser, in, m_string);
        return parse_value(src, std::forward<R>(ref)); // Throws
    }
}


template<class C, class T>
inline auto BasicValueParserSource<C, T>::widen(char ch) const -> char_type
{
    return m_parser.widen(ch); // Throws
}


template<class C, class T>
inline auto BasicValueParserSource<C, T>::get_char_mapper() noexcept -> const char_mapper_type&
{
    return m_parser;
}


template<class C, class T>
inline auto BasicValueParserSource<C, T>::getloc() const -> std::locale
{
    return m_in.getloc(); // Throws
}


template<class C, class T>
inline BasicValueParserSource<C, T>::BasicValueParserSource(value_parser_type& parser, memory_istream_type& in,
                                                            string_view_type string) noexcept
    : m_parser(parser)
    , m_in(in)
    , m_string(string)
{
}


template<class C, class T>
template<class A> inline bool BasicValueParserSource<C, T>::do_parse(std::basic_string<C, T, A>& var)
{
    var = m_string; // Throws
    return true;
}


template<class C, class T>
template<class V> bool BasicValueParserSource<C, T>::do_parse(V& var)
{
    using type = std::remove_cv_t<V>;
    if constexpr (core::is_integer<type>()) {
        if constexpr (core::is_unsigned<type>()) {
            if (ARCHON_LIKELY(m_string.empty() || m_string.front() != m_parser.m_dash))
                goto parse;
            return false;
        }
    }
    goto parse;
  parse:
    m_in.reset(m_string); // Throws
    m_in >> var; // Throws
    return (m_in && m_in.peek() == T::eof()); // Throws
}



// ============================ parse_value() ============================


template<class C, class T, class V> inline bool parse_value(core::BasicValueParserSource<C, T>& src, V& var)
{
    return src.parse(var);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_VALUE_PARSER_HPP
