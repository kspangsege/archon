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

#ifndef ARCHON_X_CORE_X_AS_LIST_HPP
#define ARCHON_X_CORE_X_AS_LIST_HPP

/// \file


#include <cstddef>
#include <algorithm>
#include <iterator>
#include <utility>
#include <array>
#include <string_view>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/value_parser.hpp>


namespace archon::core {


/// \brief Use of space characters when formatting and parsing as list.
///
/// These are the possible ways in which space characters can play a role when formatting or
/// parsing a sequence as a list. See \ref core::as_list().
///
enum class AsListSpace {
    /// Add space after separator unless separator is space. This mode is the same as \p
    /// allow if the element separator (\ref core::AsListConfig::separator) is the space
    /// characters. Otherwise, this mode is the same as \p tight.
    auto_,

    /// Do not generate, or accept space. In this mode, no space characters are generated
    /// when formatting, and space characters are not accepted when parsing. This does not
    /// apply to space characters that play a role as element separator (\ref
    /// core::AsListConfig::separator), or as brackets (\ref core::AsListConfig::bracketed),
    /// and it does not apply to the space characters that are part of the list elements
    /// themselves.
    none,

    /// Do not generate space, but accept it when parsing. This mode is the same as \p none,
    /// except that, when parsing, space characters are allowed after each element separator
    /// (\ref core::AsListConfig::separator). If bracketed (\ref
    /// core::AsListConfig::bracketed), space characters are also allowed after the opening
    /// bracket and before closing bracket.
    allow,

    /// Add space after separator. This mode is the same as \p allow, except that one space
    /// character is added after each element separator (\ref core::AsListConfig::separator)
    /// when formatting.
    tight,

    /// Add space after separator and inside bracket. This mode is the same as \p tight,
    /// except that for a bracketed syntax (\ref core::AsListConfig::bracketed), when the
    /// number of formatted list elements is not zero, one space character is added after
    /// the opening bracket and before the closing bracket when formatting.
    loose,
};



/// \brief Configuration parameters for formatting and parsing as list.
///
/// These are the available parameters for formatting and parsing sequences as a list. See
/// \ref core::as_list().
///
struct AsListConfig {
    /// \brief Element separating character.
    ///
    /// This is the character that separates the list elements. It can be the space
    /// character (see \ref core::as_words()). The character is specified in its unwidened
    /// form. Widening will be performed as part of the formatting, or parsing operation.
    ///
    char separator = ',';

    /// \brief Whether lists are bracketed.
    ///
    /// If set to `true`, lists will be bracketed. If set to `false`, lists will be
    /// unbracketed. See \ref core::as_rbr_list(). See also \ref opening_bracket and \ref
    /// closing_bracket.
    ///
    bool bracketed = false;

    /// \brief Bracketing characters.
    ///
    /// If bracketing is turned on (\ref bracketed), these are the character that act as
    /// opening and closing brackets respectively. The characters are specified in their
    /// unwidened form. Widening will be performed as part of the formatting, or parsing
    /// operation.
    ///
    char opening_bracket = '[', closing_bracket = ']';

    /// \brief Use of space in list syntax.
    ///
    /// This is the way in which space characters play a role in the list syntax.
    ///
    core::AsListSpace space = core::AsListSpace::auto_;
};



/// \{
///
/// \brief Format or parse sequence as list.
///
/// These functions return an object that can be used to format the specified sequence using
/// a particular list syntax, and can also be used to parse a string using that same list
/// syntax.
///
/// Using the default list syntax, which is unbracketed, and given a sequence containing the
/// integers 1, 2, and 3, the formatting process would produce `1, 2, 3`.
///
/// If the returned object is written to an output stream, a string representation of the
/// sequence using the specified list syntax will be written to the stream. The field width
/// of the stream will be respected, and the effect will be as if all of the generated
/// output was written to the stream as a single string object.
///
/// If the returned object is passed to \ref core::BasicValueParser::parse(), the string is
/// parsed using the specified list syntax. The string must have a list element for every
/// element in the specified sequence, and, on success of the parsing operation, new values
/// will have been assigned to all the elements of the specified sequence. Parsing will work
/// as expected only if the string representations of the list elements do not contain the
/// element separator or any other character used as part of the list syntax. If parsing
/// fails, some sequence elements may have been clobbered.
///
/// The specified sequence (\p seq) must be something such that `std::being(seq)` and
/// `std::end(seq)` can be used to obtain the beginning and end iterators for the sequence.
///
/// When using an unbracketed list syntax, if the specified sequence is empty, formatting
/// will produce no output, and parsing will succeed if, and only if the parsed string is
/// the empty string.
///
/// The overloads that take a function argument (\p func) uses the specified function to map
/// each element in the specified sequence to an object which will be formatted or parsed in
/// place of the actual sequence element. Here is an example of how it can be used:
///
/// \code{.cpp}
///
///   out << archon::core::as_list(values, [](const int& val) {
///       return archon::core::as_hex_int(val);
///   })
///
/// \endcode
///
/// \sa \ref core::as_rbr_list()
/// \sa \ref core::as_list_a()
///
template<class S> auto as_list(S&& seq, char separator, core::AsListSpace = core::AsListSpace::auto_);
template<class S> auto as_list(S&& seq, core::AsListConfig = {});
template<class S, class F> auto as_list(S&& seq, F&& func, char separator,
                                        core::AsListSpace = core::AsListSpace::auto_);
template<class S, class F> auto as_list(S&& seq, F&& func, core::AsListConfig = {});
/// \}



/// \{
///
/// \brief Format or parse sequence as bracketed list.
///
/// These functions are shorthands for calling \ref core::as_list() with a bracketed list
/// syntax. See the follow table for details.
///
/// | Function        | Type of brackets             | Characters
/// |-----------------|------------------------------|------------
/// | `as_rbr_list()` | Round brackets (parentheses) | `(`...`)`
/// | `as_sbr_list()` | Square brackets (brackets)   | `[`...`]`
/// | `as_cbr_list()` | Curly brackets (braces)      | `{`...`}`
/// | `as_abr_list()` | Angle brackets (chevrons)    | `<`...`>`
///
template<class S> auto as_rbr_list(S&& seq, char separator = ',', core::AsListSpace = core::AsListSpace::auto_);
template<class S> auto as_sbr_list(S&& seq, char separator = ',', core::AsListSpace = core::AsListSpace::auto_);
template<class S> auto as_cbr_list(S&& seq, char separator = ',', core::AsListSpace = core::AsListSpace::auto_);
template<class S> auto as_abr_list(S&& seq, char separator = ',', core::AsListSpace = core::AsListSpace::auto_);
/// \}



/// \{
///
/// \brief Format or parse sequence as list (alternative version).
///
/// These functions are similar in effect to \ref core::as_list(). The difference is that
/// these functions allow for a trailing section of the list to be elided in the string
/// representation.
///
/// Specifically, if \p copy_last is `false`, then a trailing section can be elided if all
/// the elided elements are equal to the default-valued element (see below), and the number
/// of remaining elements is greater than, or equal to \p min_elems.
///
/// Similarly, if \p copy_last is `true`, then a trailing section can be elided if all the
/// elided elements are equal to the last of the remaining elements, and the number of
/// remaining elements is greater than, or equal to \p min_elems.
///
/// The specified sequence (\p seq) must be something from which a span (\ref core::Span)
/// can be constructed. Specifically, `core::Span(seq)` must be a valid expression.
///
/// If the specified sequence is not empty, the minimum value of \p min_elems is one. If
/// zero is specified, the value will be silently bumped up to 1. As a consequence, is is
/// not possible to elide the entire specified sequence.
///
/// When formatting, the longest possible trailing section, according to the rules above,
/// will be elided.
///
/// Let `T` be the type of the sequence elements, then `T()` must produce a default-valued
/// sequence element. Now let `x` and `y` be two objects of type `T`. If the object returned
/// by `as_list_a()` is used in a formatting context, `x == y` must be a valid comparison
/// expression with the usual comparison semantics.
///
/// \sa \ref core::as_list()
///
template<class S> auto as_list_a(S&& seq, std::size_t min_elems, bool copy_last = false, core::AsListConfig = {});
template<class S, class F> auto as_list_a(S&& seq, std::size_t min_elems, bool copy_last, F&& func,
                                          core::AsListConfig = {});
/// \}



/// \{
///
/// \brief Format or parse sequence as list of words.
///
/// Calling `core::as_words(seq)` or `core::as_words(seq, func)` has the same effect as
/// calling `core::as_list(seq, ' ')` or `core::as_list(seq, func, ' ')` respectively.
///
template<class S> auto as_words(S&& seq);
template<class S, class F> auto as_words(S&& seq, F&& func);
/// \}








// Implementation


namespace impl {


template<class C, class T, class I, class F>
auto format_as_list(std::basic_ostream<C, T>& out, I begin, I end, F&& func,
                    const core::AsListConfig& config) -> std::basic_ostream<C, T>&
{
    std::array<C, 128> seed_memory;
    core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
    C separator = out.widen(config.separator); // Throws
    C space = out.widen(' '); // Throws
    bool space_inside_brackets = false;
    bool space_after_separator = false;
    switch (config.space) {
        case core::AsListSpace::auto_:
            space_after_separator = (config.separator != ' ');
            break;
        case core::AsListSpace::none:
        case core::AsListSpace::allow:
            break;
        case core::AsListSpace::tight:
            space_after_separator = true;
            break;
        case core::AsListSpace::loose:
            space_inside_brackets = true;
            space_after_separator = true;
            break;
    }
    auto i = begin;
    if (config.bracketed) {
        helper.out << out.widen(config.opening_bracket); // Throws
        if (space_inside_brackets)
            helper.out << space; // Throws
    }
    if (i != end) {
        goto element;
        do {
            helper.out << separator; // Throws
            if (space_after_separator)
                helper.out << space; // Throws
          element:
            helper.out << func(*i); // Throws
        }
        while (++i != end);
    }
    if (config.bracketed) {
        if (space_inside_brackets)
            helper.out << space; // Throws
        helper.out << out.widen(config.closing_bracket); // Throws
    }
    helper.flush(); // Throws
    return out;
}


template<class C, class T, class I>
auto format_as_list(std::basic_ostream<C, T>& out, I begin, I end,
                    const core::AsListConfig& config) -> std::basic_ostream<C, T>&
{
    auto func = [](const auto& elem) -> auto& {
        return elem; // Throws
    };
    return impl::format_as_list(out, begin, end, func, config); // Throws
}


template<class C, class T, class I, class F>
bool parse_as_list(core::BasicValueParserSource<C, T>& src, I begin, I end, I& reached, F&& func,
                   const core::AsListConfig& config)
{
    C separator = src.widen(config.separator); // Throws
    C space = src.widen(' '); // Throws
    auto str = src.string();
    auto begin_2 = str.data();
    auto end_2 = begin_2 + str.size();
    bool allow_space = false;
    switch (config.space) {
        case core::AsListSpace::auto_:
            allow_space = true;
            break;
        case core::AsListSpace::none:
            break;
        case core::AsListSpace::allow:
        case core::AsListSpace::tight:
        case core::AsListSpace::loose:
            allow_space = true;
            break;
    }
    if (config.bracketed) {
        C opening_bracket = src.widen(config.opening_bracket); // Throws
        C closing_bracket = src.widen(config.closing_bracket); // Throws
        if (ARCHON_UNLIKELY(begin_2 == end_2 || begin_2[0] != opening_bracket))
            return false; // Failure
        ++begin_2;
        if (ARCHON_UNLIKELY(begin_2 == end_2 || end_2[-1] != closing_bracket))
            return false; // Failure
        --end_2;
        if (allow_space) {
            while (begin_2 != end_2 && begin_2[0] == space)
                ++begin_2;
            while (begin_2 != end_2 && end_2[-1] == space)
                --end_2;
        }
    }
    if  (ARCHON_LIKELY(end != begin)) {
        auto i = begin_2;
        I curr = begin;
        for (;;) {
            auto j = std::find(i, end_2, separator);
            std::basic_string_view<C, T> substr(i, std::size_t(j - i));
            bool success = src.delegate(substr, func(*curr)); // Throws
            if (ARCHON_LIKELY(success)) {
                ++curr;
                if (ARCHON_LIKELY(curr != end)) {
                    if (ARCHON_LIKELY(j != end_2)) {
                        i = j + 1;
                        if (allow_space) {
                            while (i < end_2 && *i == space)
                                ++i;
                        }
                        continue;
                    }
                    reached = curr;
                    return true; // Partial success
                }
                if (ARCHON_LIKELY(j == end_2)) {
                    reached = end;
                    return true; // Full success
                }
            }
            return false; // Error
        }
    }
    if (ARCHON_LIKELY(str.empty())) {
        reached = end;
        return true; // Trivial success
    }
    return false; // Error
}


template<class C, class T, class I>
bool parse_as_list(core::BasicValueParserSource<C, T>& src, I begin, I end, I& reached,
                   const core::AsListConfig& config)
{
    auto func = [](auto& elem) -> auto& {
        return elem; // Throws
    };
    return impl::parse_as_list(src, begin, end, reached, func, config); // Throws
}


template<class S> struct AsList {
    const S& seq;
    core::AsListConfig config;
};


template<class C, class T, class S>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsList<S>& pod) -> std::basic_ostream<C, T>&
{
    auto begin = std::begin(pod.seq);
    auto end = std::end(pod.seq);
    return impl::format_as_list(out, begin, end, pod.config); // Throws
}


template<class C, class T, class S>
bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::AsList<S>& pod)
{
    using iter_type = decltype(std::begin(pod.seq));
    iter_type begin = std::begin(pod.seq);
    iter_type end = std::end(pod.seq);
    iter_type reached = iter_type();
    bool success = impl::parse_as_list(src, begin, end, reached, pod.config); // Throws
    return (success && reached == end);
}


template<class S, class F> struct AsListFunc {
    const S& seq;
    F func;
    core::AsListConfig config;
};


template<class C, class T, class S, class F>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsListFunc<S, F>& pod) -> std::basic_ostream<C, T>&
{
    auto begin = std::begin(pod.seq);
    auto end = std::end(pod.seq);
    return impl::format_as_list(out, begin, end, pod.func, pod.config); // Throws
}


template<class C, class T, class S, class F>
bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::AsListFunc<S, F>& pod)
{
    using iter_type = decltype(std::begin(pod.seq));
    iter_type begin = std::begin(pod.seq);
    iter_type end = std::end(pod.seq);
    iter_type reached = iter_type();
    bool success = impl::parse_as_list(src, begin, end, reached, pod.func, pod.config); // Throws
    return (success && reached == end);
}


template<class V> struct AsListA {
    core::Span<V> seq;
    std::size_t min_elems;
    bool copy_last;
    core::AsListConfig config;
};


template<class C, class T, class V>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsListA<V>& pod) -> std::basic_ostream<C, T>&
{
    std::size_t min_elems = std::max(pod.min_elems, std::size_t(1));
    std::size_t i = pod.seq.size();
    if (pod.copy_last) {
        while (i > min_elems && pod.seq[i - 1] == pod.seq[i - 2])
            --i;
    }
    else {
        while (i > min_elems && pod.seq[i - 1] == V())
            --i;
    }
    auto begin = pod.seq.data();
    auto end = begin + i;
    return impl::format_as_list(out, begin, end, pod.config); // Throws
}


template<class C, class T, class V>
bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::AsListA<V>& pod)
{
    V* begin = pod.seq.data();
    V* end = begin + pod.seq.size();
    V* reached = nullptr;
    bool success = impl::parse_as_list(src, begin, end, reached, pod.config); // Throws
    if (ARCHON_LIKELY(success)) {
        if (ARCHON_LIKELY(!pod.seq.empty())) {
            std::size_t elision_offset = std::size_t(reached - begin);
            ARCHON_ASSERT(elision_offset > 0);
            V fill_val = {}; // Throws
            if (ARCHON_UNLIKELY(pod.copy_last))
                fill_val = pod.seq[elision_offset - 1]; // Throws
            std::fill(begin + elision_offset, end, fill_val); // Throws
        }
        return true; // Success
    }
    return false; // Failure
}


template<class V, class F> struct AsListFuncA {
    core::Span<V> seq;
    std::size_t min_elems;
    bool copy_last;
    F func;
    core::AsListConfig config;
};


template<class C, class T, class V, class F>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsListFuncA<V, F>& pod) -> std::basic_ostream<C, T>&
{
    std::size_t min_elems = std::max(pod.min_elems, std::size_t(1));
    std::size_t i = pod.seq.size();
    if (pod.copy_last) {
        while (i > min_elems && pod.seq[i - 1] == pod.seq[i - 2])
            --i;
    }
    else {
        while (i > min_elems && pod.seq[i - 1] == V())
            --i;
    }
    auto begin = pod.seq.data();
    auto end = begin + i;
    return impl::format_as_list(out, begin, end, pod.func, pod.config); // Throws
}


template<class C, class T, class V, class F>
bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::AsListFuncA<V, F>& pod)
{
    V* begin = pod.seq.data();
    V* end = begin + pod.seq.size();
    V* reached = nullptr;
    bool success = impl::parse_as_list(src, begin, end, reached, pod.func, pod.config); // Throws
    if (ARCHON_LIKELY(success)) {
        if (ARCHON_LIKELY(!pod.seq.empty())) {
            std::size_t elision_offset = std::size_t(reached - begin);
            ARCHON_ASSERT(elision_offset > 0);
            V fill_val = {}; // Throws
            if (ARCHON_UNLIKELY(pod.copy_last))
                fill_val = pod.seq[elision_offset - 1]; // Throws
            std::fill(begin + elision_offset, end, fill_val); // Throws
        }
        return true; // Success
    }
    return false; // Failure
}


} // namespace impl


template<class S> inline auto as_list(S&& seq, char separator, core::AsListSpace space)
{
    core::AsListConfig config;
    config.separator = separator;
    config.space     = space;
    return core::as_list(std::forward<S>(seq), std::move(config)); // Throws
}


template<class S> inline auto as_list(S&& seq, core::AsListConfig config)
{
    return impl::AsList<S> { std::forward<S>(seq), std::move(config) }; // Throws
}


template<class S, class F> auto as_list(S&& seq, F&& func, char separator, core::AsListSpace space)
{
    core::AsListConfig config;
    config.separator = separator;
    config.space     = space;
    return core::as_list(std::forward<S>(seq), std::forward<F>(func), std::move(config)); // Throws
}


template<class S, class F> inline auto as_list(S&& seq, F&& func, core::AsListConfig config)
{
    return impl::AsListFunc<S, F> {
        std::forward<S>(seq),
        std::forward<F>(func),
        std::move(config),
    }; // Throws
}


template<class S> inline auto as_rbr_list(S&& seq, char separator, core::AsListSpace space)
{
    core::AsListConfig config;
    config.separator       = separator;
    config.bracketed       = true;
    config.opening_bracket = '(';
    config.closing_bracket = ')';
    config.space           = space;
    return core::as_list(std::forward<S>(seq), std::move(config)); // Throws
}


template<class S> inline auto as_sbr_list(S&& seq, char separator, core::AsListSpace space)
{
    core::AsListConfig config;
    config.separator       = separator;
    config.bracketed       = true;
    config.opening_bracket = '[';
    config.closing_bracket = ']';
    config.space           = space;
    return core::as_list(std::forward<S>(seq), std::move(config)); // Throws
}


template<class S> inline auto as_cbr_list(S&& seq, char separator, core::AsListSpace space)
{
    core::AsListConfig config;
    config.separator       = separator;
    config.bracketed       = true;
    config.opening_bracket = '{';
    config.closing_bracket = '}';
    config.space           = space;
    return core::as_list(std::forward<S>(seq), std::move(config)); // Throws
}


template<class S> inline auto as_abr_list(S&& seq, char separator, core::AsListSpace space)
{
    core::AsListConfig config;
    config.separator       = separator;
    config.bracketed       = true;
    config.opening_bracket = '<';
    config.closing_bracket = '>';
    config.space           = space;
    return core::as_list(std::forward<S>(seq), std::move(config)); // Throws
}


template<class S> inline auto as_list_a(S&& seq, std::size_t min_elems, bool copy_last, core::AsListConfig config)
{
    core::Span seq_2(std::forward<S>(seq));
    using elem_type = typename decltype(seq_2)::element_type;
    return impl::AsListA<elem_type> { seq_2, min_elems, copy_last, std::move(config) }; // Throws
}


template<class S, class F>
inline auto as_list_a(S&& seq, std::size_t min_elems, bool copy_last, F&& func, core::AsListConfig config)
{
    core::Span seq_2(std::forward<S>(seq));
    using elem_type = typename decltype(seq_2)::element_type;
    return impl::AsListFuncA<elem_type, F> {
        seq_2,
        min_elems,
        copy_last,
        std::forward<F>(func),
        std::move(config),
    }; // Throws
}


template<class S> inline auto as_words(S&& seq)
{
    return core::as_list(std::forward<S>(seq), ' '); // Throws
}


template<class S, class F> inline auto as_words(S&& seq, F&& func)
{
    return core::as_list(std::forward<S>(seq), std::forward<F>(func), ' '); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_AS_LIST_HPP
