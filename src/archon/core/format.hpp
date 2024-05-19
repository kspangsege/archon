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

#ifndef ARCHON_X_CORE_X_FORMAT_HPP
#define ARCHON_X_CORE_X_FORMAT_HPP

/// \file


#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
#include <tuple>
#include <exception>
#include <string_view>
#include <sstream>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/formattable_value_ref.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/with_modified_locale.hpp>


namespace archon::core {


/// \{
///
/// \brief Format string with parameters in global locale.
///
/// This function formats the specified parameterized string (\p c_str or \p string) using
/// the specified parameter values.
///
/// Formatting occurs as if by `format(out, c_str, params...)` or `format(out, string,
/// params...)` where `out` is an output stream created as follows:
///
/// \code{.cpp}
///
///   std::basic_ostringstream<C, T> out;
///   out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
///
/// \endcode
///
/// The current global locale will be used for the purpose of formatting values. See also
/// the overloads that have a locale argument.
///
/// When specifying a null-terminated parameterized string (\p c_str), if the character type
/// of the string is `char`, but `C` is not `char`, the characters of the parameterized
/// string will be widened as if by `widen()` of an output stream with character type `C`
/// and imbued with the global locale. It is therefore only safe to use characters from the
/// basic source character set in the parameterized string in this case.
///
/// \throws core::BadFormatString If the specified parameterized string is invalid.
///
template<class C, class... P> auto format(const char* c_str, const P&... params) -> std::basic_string<C>;
template<class C, class... P> auto format(const C* c_str, const P&... params) -> std::basic_string<C>;
template<class C, class T, class... P> auto format(std::basic_string_view<C, T> string, const P&... params) ->
    std::basic_string<C, T>;
/// \}



/// \{
///
/// \brief Format string with parameters in specified locale.
///
/// This function formats the specified parameterized string (\p c_str or \p string) using
/// the specified parameter values and the specified locale.
///
/// Formatting occurs as if by `format(out, c_str, params...)` or `format(out, string,
/// params...)` where `out` is an output stream created as follows:
///
/// \code{.cpp}
///
///   std::basic_ostringstream<C, T> out;
///   out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
///   out.imbue(locale);
///
/// \endcode
///
/// When specifying a null-terminated parameterized string (\p c_str), if the character type
/// of the string is `char`, but `C` is not `char`, the characters of the parameterized
/// string will be widened as if by `widen()` of an output stream with character type `C`
/// and imbued with the specified locale. It is therefore only safe to use characters from
/// the basic source character set in the parameterized string in this case.
///
/// \throws core::BadFormatString If the specified parameterized string is invalid.
///
template<class C, class... P> auto format(const std::locale& locale, const char* c_str,
                                          const P&... params) -> std::basic_string<C>;
template<class C, class... P> auto format(const std::locale& locale, const C* c_str,
                                          const P&... params) -> std::basic_string<C>;
template<class C, class T, class... P> auto format(const std::locale& locale, std::basic_string_view<C, T> string,
                                                   const P&... params) -> std::basic_string<C, T>;
/// \}



/// \{
///
/// \brief Format string with parameters to output stream.
///
/// This function uses the specified output stream to format the specified parameterized
/// string (\p c_str or \p string) using the specified parameter values.
///
/// When specifying a null-terminated parameterized string (\p c_str), if the character type
/// of the string is `char`, but the character type of the specified output stream is not
/// `char`, the characters of the parameterized string will be widened as if by `widen()` of
/// \p out. It is therefore only safe to use characters from the basic source character set
/// in the parameterized string in this case.
///
/// Each occurrence of a parameter substitution marker (`%s`) in the parameterized string
/// will be replaced by a string representation of the corresponding parameter value. The
/// string representation is produced by outputting the parameter value to the specified
/// output stream. The N'th parameter substitution marker refers to the N'th specified
/// parameter value. The parameterized string is invalid if the number of parameter
/// substitution markers is different from the number of specified parameter values.
///
/// Each occurrence of the special substitution marker (`%%`) in the parameterized string
/// will be replaced by a single percent sign (`%`).
///
/// Substitution markers are identified by scanning the parameterized string from left to
/// right for percent signs (`%`). If one is found, and it is followed by `s`, then the two
/// characters (`%s`) is a parameter substitution marker. Otherwise, if it is followed by
/// `%`, then the two characters (`%%`) is the special substitution marker. Otherwise the
/// parameterized string is invalid. When a valid substitution marker is found, scanning
/// resumes with the character immediately following the one that was just found.
///
/// \throws core::BadFormatString If the specified parameterized string is invalid.
///
template<class T, class... P> void format(std::basic_ostream<char, T>& out, const char* c_str, const P&... params);
template<class C, class T, class... P> void format(std::basic_ostream<C, T>& out, const char* c_str,
                                                   const P&... params);
template<class C, class T, class... P> void format(std::basic_ostream<C, T>& out, const C* c_str, const P&... params);
template<class C, class T, class... P> void format(std::basic_ostream<C, T>&, std::basic_string_view<C, T> string,
                                                   const P&... params);
/// \}



/// \{
///
/// \brief Potentiate formatting of parameterized string.
///
/// Construct an object that, if written to an output stream, formats the specified
/// parameterized string (\p c_str or \p string) using the specified parameter values.
///
/// If `out` is an output stream, then `out << formatted(c_str, params...)` has the same
/// effect as `format(out, c_str, params...)`, and `out << formatted(string, params...)` has
/// the same effect as `format(out, string, params...)`.
///
/// For example, the following two statements have the same effect:
///
/// \code{.cpp}
///
///   std::cout << formatted("<%s:%s>\n", x, y);
///   format(std::cout, "<%s:%s>\n", x, y);
///
/// \endcode
///
/// Note how this enables efficient multi-level hierarchical formatting, such as in:
///
/// \code{.cpp}
///
///   format(std::cout, "%s: %s\n", qual, formatted("<%s:%s>", x, y));
///
/// \endcode
///
/// When specifying a null-terminated parameterized string (\p c_str), if the character type
/// of the string is `char`, but the returned object is written to an output stream whose
/// character type is not `char`, the characters of the parameterized string will be widened
/// as if by `widen()` of the output stream. It is therefore only safe to use characters
/// from the basic source character set in the parameterized string in this case.
///
/// \sa \ref core::formatted_wrn()
///
template<class C, class... P> auto formatted(const C* c_str, const P&... params) noexcept;
template<class C, class T, class... P> auto formatted(std::basic_string_view<C, T> string,
                                                      const P&... params) noexcept;
/// \}



/// \{
///
/// \brief Potentiate formatting of parameterized string with reverted numerics.
///
/// These functions have the same effect as \ref core::formatted(), but with numeric facets
/// reverted to the classic locale. Specifically, `formatted_wrn(str, params...)` has the
/// same effect as `core::with_reverted_numerics(core::formatted(str, params...))`.
///
/// \sa \ref core::formatted()
/// \sa \ref core::with_reverted_numerics()
///
template<class C, class... P> auto formatted_wrn(const C* c_str, const P&... params) noexcept;
template<class C, class T, class... P> auto formatted_wrn(std::basic_string_view<C, T> string,
                                                          const P&... params) noexcept;
/// \}



/// \{
///
/// \brief Format string with type-erased parameters.
///
/// This function uses the specified output stream to format the specified parameterized
/// string (\p c_str or \p string) using the specified type-erased parameter value
/// references.
///
/// If `typed_params...` is a pack of typed parameter values, then `format(out, c_str,
/// typed_params...)` has the same effect as `format_alt(out, c_str, params)`, and
/// `format(out, string, typed_params...)` has the same effect as `format_alt(out, string,
/// params)`, where `params` is a pack of type eased parameter value references created as
/// follows:
///
/// \code{.cpp}
///
///   BasicFormattableValueRef<C, T> buffer[sizeof... (typed_params)];
///   BasicFormattableValueRef<C, T>::record(buffer, params...);
///   Span<const BasicFormattableValueRef<C, T>> params(buffer);
///
/// \endcode
///
/// When specifying a null-terminated parameterized string (\p c_str), if the character type
/// of the string is `char`, but the character type of the specified output stream is not
/// `char`, the characters of the parameterized string will be widened as if by `widen()` of
/// \p out. It is therefore only safe to use characters from the basic source character set
/// in the parameterized string in this case.
///
template<class T> void format_alt(std::basic_ostream<char, T>& out, const char* c_str,
                                  core::Span<const core::BasicFormattableValueRef<char, T>> params);
template<class C, class T> void format_alt(std::basic_ostream<C, T>& out, const char* c_str,
                                           core::Span<const core::BasicFormattableValueRef<C, T>> params);
template<class C, class T> void format_alt(std::basic_ostream<C, T>& out, const C* c_str,
                                           core::Span<const core::BasicFormattableValueRef<C, T>> params);
template<class C, class T> void format_alt(std::basic_ostream<C, T>& out, std::basic_string_view<C, T> string,
                                           core::Span<const core::BasicFormattableValueRef<C, T>> params);
/// \}



/// \brief Invalid parameterized string.
///
/// Thrown by \ref format() if the specified parameterized string is invalid. Also throw
/// when writing the object returned by \ref formatted() to an output stream, if the
/// specified parameterized string is invalid.
///
class BadFormatString
    : public std::exception {
public:
    explicit BadFormatString(const char* message) noexcept;
    auto what() const noexcept -> const char* override;
private:
    const char* m_message;
};








// Implementation


namespace impl {


template<class C, class... P> struct AsFormatted1 {
    const C* c_str;
    std::tuple<const P&...> params;
};


template<class C, class T, class... P> struct AsFormatted2 {
    std::basic_string_view<C, T> string;
    std::tuple<const P&...> params;
};


template<class C, class T> class Format
    : public core::BasicCharMapper<C, T> {
public:
    using ostream_type     = std::basic_ostream<C, T>;
    using string_view_type = std::basic_string_view<C, T>;

    template<class F, std::size_t... I>
    static void unpack_1(ostream_type& out, const F& pod, std::index_sequence<I...>)
    {
        core::format(out, pod.c_str, std::get<I>(pod.params)...); // Throws
    }

    template<class F, std::size_t... I>
    static void unpack_2(ostream_type& out, const F& pod, std::index_sequence<I...>)
    {
        core::format(out, pod.string, std::get<I>(pod.params)...); // Throws
    }

    Format(ostream_type& out)
        : core::BasicCharMapper<C, T>(out) // Throws
        , m_out(out)
        , m_char_s(this->widen('s')) // Throws
        , m_char_percent(this->widen('%')) // Throws
    {
    }

    void format(string_view_type string)
    {
        advance_2(string); // Throws
    }

    template<class P, class... Q> void format(string_view_type string, const P& param, const Q&... params)
    {
        std::size_t i = advance_1(string); // Throws
        ARCHON_ASSERT(i > 0);
        m_out << param; // Throws
        format(string.substr(i), params...); // Throws
    }

    void format_alt(string_view_type string, core::Span<const core::BasicFormattableValueRef<C, T>> params)
    {
        string_view_type string_2 = string;
        for (auto param : params) {
            std::size_t i = advance_1(string_2); // Throws
            ARCHON_ASSERT(i > 0);
            m_out << param; // Throws
            string_2 = string_2.substr(i);
        }
        advance_2(string_2); // Throws
    }

private:
    ostream_type& m_out;
    C m_char_s, m_char_percent;

    auto advance_1(string_view_type string) -> std::size_t
    {
        std::size_t i = advance_3(string); // Throws
        if (i == 0)
            throw core::BadFormatString("Too few parameter substitution markers (`%s`)");
        return i;
    }

    void advance_2(string_view_type string)
    {
        std::size_t i = advance_3(string); // Throws
        if (i > 0)
            throw core::BadFormatString("Too many parameter substitution markers (`%s`)");
    }

    auto advance_3(string_view_type string) -> std::size_t
    {
        std::size_t n = string.size();
        std::size_t i = 0;
        for (;;) {
            std::size_t j = string.find(m_char_percent, i);
            if (ARCHON_LIKELY(j != string_view_type::npos))
                goto found;
            j = n;
          found:
            m_out << string.substr(i, j - i); // Throws
            if (ARCHON_LIKELY(j < n)) {
                ++j;
                if (ARCHON_LIKELY(j < n)) {
                    if (ARCHON_LIKELY(string[j] == m_char_s))
                        return j + 1;
                    if (ARCHON_LIKELY(string[j] == m_char_percent)) {
                        m_out << m_char_percent; // Throws
                        i = j + 1;
                        continue;
                    }
                    throw core::BadFormatString("Invalid character after `%`");
                }
                throw core::BadFormatString("Missing character after `%`");
            }
            return 0;
        }
    }
};


template<class C, class T, class D, class... P>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsFormatted1<D, P...>& pod) -> std::basic_ostream<C, T>&
{
    core::BasicStreamOutputAltHelper helper(out); // Throws
    impl::Format<C, T>::unpack_1(helper.out, pod, std::index_sequence_for<P...>()); // Throws
    helper.flush(); // Throws
    return out;
}


template<class C, class T, class... P>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsFormatted2<C, T, P...>& pod) -> std::basic_ostream<C, T>&
{
    core::BasicStreamOutputAltHelper helper(out); // Throws
    impl::Format<C, T>::unpack_2(helper.out, pod, std::index_sequence_for<P...>()); // Throws
    helper.flush(); // Throws
    return out;
}


} // namespace impl


template<class C, class... P> inline auto format(const char* c_str, const P&... params) -> std::basic_string<C>
{
    std::basic_ostringstream<C> out;
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    format(out, c_str, params...); // Throws
    return std::move(out).str();
}


template<class C, class... P> inline auto format(const C* c_str, const P&... params) -> std::basic_string<C>
{
    std::basic_string_view<C> string = c_str;
    return format(string, params...); // Throws
}


template<class C, class T, class... P>
inline auto format(std::basic_string_view<C, T> string, const P&... params) -> std::basic_string<C, T>
{
    std::basic_ostringstream<C, T> out;
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    format(out, string, params...); // Throws
    return std::move(out).str();
}


template<class C, class... P>
auto format(const std::locale& locale, const char* c_str, const P&... params) -> std::basic_string<C>
{
    std::basic_ostringstream<C> out;
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(locale); // Throws
    format(out, c_str, params...); // Throws
    return std::move(out).str();
}


template<class C, class... P>
inline auto format(const std::locale& locale, const C* c_str, const P&... params) -> std::basic_string<C>
{
    std::basic_string_view<C> string = c_str;
    return format(locale, string, params...); // Throws
}


template<class C, class T, class... P>
inline auto format(const std::locale& locale, std::basic_string_view<C, T> string, const P&... params) ->
    std::basic_string<C, T>
{
    std::basic_ostringstream<C, T> out;
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(locale); // Throws
    format(out, string, params...); // Throws
    return std::move(out).str();
}


template<class T, class... P>
inline void format(std::basic_ostream<char, T>& out, const char* c_str, const P&... params)
{
    std::basic_string_view<char, T> string = c_str;
    format(out, string, params...); // Throws
}


template<class C, class T, class... P>
void format(std::basic_ostream<C, T>& out, const char* c_str, const P&... params)
{
    static_assert(!std::is_same_v<C, char>);
    impl::Format f(out); // Throws
    typename core::BasicCharMapper<C, T>::template ArraySeededWidenBuffer<512> widen_buffer;
    std::basic_string_view<C, T> string = f.widen(c_str, widen_buffer); // Throws
    f.format(string, params...); // Throws
}


template<class C, class T, class... P>
inline void format(std::basic_ostream<C, T>& out, const C* c_str, const P&... params)
{
    std::basic_string_view<C, T> string = c_str;
    format(out, string, params...); // Throws
}


template<class C, class T, class... P>
inline void format(std::basic_ostream<C, T>& out, std::basic_string_view<C, T> string, const P&... params)
{
    impl::Format f(out); // Throws
    f.format(string, params...); // Throws
}


template<class C, class... P> inline auto formatted(const C* c_str , const P&... params) noexcept
{
    return impl::AsFormatted1<C, P...> { c_str, { params... } };
}


template<class C, class T, class... P>
inline auto formatted(std::basic_string_view<C, T> string, const P&... params) noexcept
{
    return impl::AsFormatted2<C, T, P...> { string, { params... } };
}


template<class C, class... P> inline auto formatted_wrn(const C* c_str, const P&... params) noexcept
{
    return core::with_reverted_numerics(core::formatted(c_str, params...)); // Throws
}


template<class C, class T, class... P>
inline auto formatted_wrn(std::basic_string_view<C, T> string, const P&... params) noexcept
{
    return core::with_reverted_numerics(core::formatted(string, params...)); // Throws
}


template<class T>
void format_alt(std::basic_ostream<char, T>& out, const char* c_str,
                core::Span<const core::BasicFormattableValueRef<char, T>> params)
{
    std::basic_string_view<char, T> string = c_str;
    format_alt(out, string, params); // Throws
}


template<class C, class T>
void format_alt(std::basic_ostream<C, T>& out, const char* c_str,
                core::Span<const core::BasicFormattableValueRef<C, T>> params)
{
    static_assert(!std::is_same_v<C, char>);
    impl::Format f(out); // Throws
    typename core::BasicCharMapper<C, T>::template ArraySeededWidenBuffer<512> widen_buffer;
    std::basic_string_view<C, T> string = f.widen(c_str, widen_buffer); // Throws
    f.format_alt(string, params); // Throws
}


template<class C, class T>
void format_alt(std::basic_ostream<C, T>& out, const C* c_str,
                core::Span<const core::BasicFormattableValueRef<C, T>> params)
{
    std::basic_string_view<C, T> string = c_str;
    format_alt(out, string, params); // Throws
}


template<class C, class T>
void format_alt(std::basic_ostream<C, T>& out, std::basic_string_view<C, T> string,
                core::Span<const core::BasicFormattableValueRef<C, T>> params)
{
    impl::Format f(out); // Throws
    f.format_alt(string, params); // Throws
}


inline BadFormatString::BadFormatString(const char* message) noexcept
    : m_message(message)
{
}


inline auto BadFormatString::what() const noexcept -> const char*
{
    return m_message;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FORMAT_HPP
