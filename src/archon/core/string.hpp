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

#ifndef ARCHON_X_CORE_X_STRING_HPP
#define ARCHON_X_CORE_X_STRING_HPP

/// \file


#include <cstddef>
#include <utility>
#include <string_view>
#include <string>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/char_mapper.hpp>


namespace archon::core {


/// \{
///
/// \brief Check if string contains substring.
///
/// These functions are replacements for member functions of the same name added to
/// `std::basic_string_view` in C++23.
///
template<class C, class T> bool contains(std::basic_string_view<C, T> string,
                                         std::basic_string_view<C, T> substr) noexcept;
template<class C, class T> bool contains(std::basic_string_view<C, T> string, const C* substr);
/// \}



/// \{
///
/// \brief Concatenate strings using one allocation.
///
/// These functions construct the concatenation of the specified strings using at most one
/// dynamic memory allocation.
///
template<class C, class T> auto concat(std::basic_string_view<C, T>,
                                       std::basic_string_view<C, T>) -> std::basic_string<C, T>;
template<class C, class T> auto concat(std::basic_string_view<C, T>, std::basic_string_view<C, T>,
                                       std::basic_string_view<C, T>) -> std::basic_string<C, T>;
/// \}



/// \brief Modify string view to refer to new location of string data.
///
/// When string data is copied / moved to a new location in memory, this function can be
/// used to update a string view accordingly. More specifically, if \p str refers to a
/// string at a particular offset in a memory chunk based at \p old_base, then \p str is
/// changed to refer to a string of the same length and same offset in a memory chunk based
/// at \p new_base.
///
template<class C, class T> void rebase_string(std::basic_string_view<C, T>& str, const C* old_base, const C* new_base);



/// \{
///
/// \brief Remove trailing sequences of newline or other delimiter.
///
/// These functions remove trailing sequences of the specified delimiter \p delim.
///
/// The overload that takes a locale argument, will widen the specified delimiter as
/// necessary. The overload that does not take a locale argument assumes that the specified
/// delimiter has already been widened by the caller.
///
/// For the sake of efficiency, when performing many chomp operations, the caller should use
/// the overload that takes a pre-widened delimiter and only widen the delimiter once up
/// front.
///
/// \sa \ref core::trim()
///
template<class C, class T> auto chomp(std::basic_string_view<C, T> str, const std::locale&, char delim = '\n') ->
    std::basic_string_view<C, T>;
template<class C, class T> auto chomp(std::basic_string_view<C, T> str, C delim) noexcept ->
    std::basic_string_view<C, T>;
/// \}



/// \{
///
/// \brief Remove leading and trailing sequences of space or other delimiter.
///
/// These functions remove leading and trailing sequences of the specified delimiter \p
/// delim.
///
/// The overload that takes a locale argument, will widen the specified delimiter as
/// necessary. The overload that does not take a locale argument assumes that the specified
/// delimiter has already been widened by the caller.
///
/// For the sake of efficiency, when performing many trim operations, the caller should use
/// the overload that takes a pre-widened delimiter and only widen the delimiter once up
/// front.
///
/// \sa \ref core::chomp()
///
template<class C, class T> auto trim(std::basic_string_view<C, T> str, const std::locale&, char delim = ' ') ->
    std::basic_string_view<C, T>;
template<class C, class T> auto trim(std::basic_string_view<C, T> str, C delim) noexcept ->
    std::basic_string_view<C, T>;
/// \}



/// \brief Split string into words.
///
/// This function calls the specified function for each word in the specified string.
///
/// The specified function must be callable with a single argument, which will be a string
/// view that refers to one of the words of the string. These string views will reference
/// the same memory as is referenced by the specified string (\p str).
///
/// Splitting is done by an instance of \ref core::BasicStringSplitter.
///
template<class C, class T, class F> void for_each_word(std::basic_string_view<C, T> str, const std::locale&, F func);



/// \brief Split string based on delimiter.
///
/// This class offers a mechanism for splitting a string according to a specified
/// delimiter. When the delimiter is "space", the string can be thought of as being split
/// into words.
///
/// After construction of a splitter object, the first invocation of \ref next() will return
/// the first piece of the string, and so forth.
///
/// In the context of this splitter class, a substring is a piece if, and only if it is
/// nonempty, it does not contain the specified delimiter, it is bounded to the left either
/// by the beginning of the string or by a delimiter, and it is bounded to the right either
/// by a delimiter or by the end of the string.
///
/// \sa \ref core::for_each_word().
///
template<class C, class T = std::char_traits<C>> class BasicStringSplitter {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    /// \{
    ///
    /// \brief Initiate new splitting operation
    ///
    /// These constructors initiate a new splitting operation based on the specified
    /// delimiter.
    ///
    /// The overload that takes a locale argument, will widen the specified delimiter as
    /// necessary. The overload that does not take a locale argument assumes that the
    /// specified delimiter has already been widened by the caller.
    ///
    /// For the sake of efficiency, when performing many splitting operations, the caller
    /// should use the overload that takes a pre-widened delimiter and only widen the
    /// delimiter once up front.
    ///
    BasicStringSplitter(string_view_type str, const std::locale&, char delim = ' ');
    BasicStringSplitter(string_view_type str, char_type delim) noexcept;
    /// \}

    /// \brief Isolate next piece of the string.
    ///
    /// This function isolates the next piece of the string.
    ///
    /// If there is at least one more piece left in the string, this function returns `true`
    /// after setting \p word to refer to the first of those pieces. The memory referenced
    /// by \p word will be the same memory as is referenced by the string passed to the
    /// constructor.
    ///
    /// If there are no more pieces left in the string, this function returns `false`, and
    /// leaves \p word "untouched".
    ///
    bool next(string_view_type& word) noexcept;

private:
    const char_type* m_cur;
    const char_type* m_end;
    const char_type m_delim;

    static auto map_delim(char, const std::locale&) -> char_type;
};


using StringSplitter     = BasicStringSplitter<char>;
using WideStringSplitter = BasicStringSplitter<wchar_t>;








// Implementation


template<class C, class T>
inline bool contains(std::basic_string_view<C, T> string, std::basic_string_view<C, T> substr) noexcept
{
    return (string.find(substr) != std::basic_string_view<C, T>::npos);
}


template<class C, class T>
inline bool contains(std::basic_string_view<C, T> string, const C* substr)
{
    return contains(string, std::basic_string_view<C, T>(substr)); // Throws
}


template<class C, class T>
auto concat(std::basic_string_view<C, T> a, std::basic_string_view<C, T> b) -> std::basic_string<C, T>
{
    std::size_t size = a.size();
    if (ARCHON_LIKELY(core::try_int_add(size, b.size()))) {
        std::basic_string<C, T> string;
        string.reserve(size); // Throws
        string.append(a);
        string.append(b);
        return string;
    }
    throw std::length_error("String size");
}


template<class C, class T>
auto concat(std::basic_string_view<C, T> a, std::basic_string_view<C, T> b, std::basic_string_view<C, T> c) ->
    std::basic_string<C, T>
{
    std::size_t size = a.size();
    if (ARCHON_LIKELY(core::try_int_add(size, b.size()))) {
        if (ARCHON_LIKELY(core::try_int_add(size, c.size()))) {
            std::basic_string<C, T> string;
            string.reserve(size); // Throws
            string.append(a);
            string.append(b);
            string.append(c);
            return string;
        }
    }
    throw std::length_error("String size");
}


template<class C, class T> inline void rebase_string(std::basic_string_view<C, T>& str, const C* old_base,
                                                     const C* new_base)
{
    std::ptrdiff_t offset = str.data() - old_base;
    str = { new_base + offset, str.size() }; // Throws
}


template<class C, class T>
inline auto chomp(std::basic_string_view<C, T> str, const std::locale& loc, char delim) -> std::basic_string_view<C, T>
{
    core::BasicCharMapper<C, T> mapper(loc); // Throws
    return core::chomp(str, mapper.widen(delim)); // Throws
}


template<class C, class T> auto chomp(std::basic_string_view<C, T> str, C delim) noexcept ->
    std::basic_string_view<C, T>
{
    const C* begin = str.data();
    const C* end   = begin + str.size();
    while (end > begin && end[-1] == delim)
        --end;
    return { begin, std::size_t(end - begin) };
}


template<class C, class T>
inline auto trim(std::basic_string_view<C, T> str, const std::locale& loc, char delim) -> std::basic_string_view<C, T>
{
    core::BasicCharMapper<C, T> mapper(loc); // Throws
    return core::trim(str, mapper.widen(delim)); // Throws
}


template<class C, class T> auto trim(std::basic_string_view<C, T> str, C delim) noexcept ->
    std::basic_string_view<C, T>
{
    const C* begin = str.data();
    const C* end   = begin + str.size();
    while (begin < end && begin[0] == delim)
        ++begin;
    while (end > begin && end[-1] == delim)
        --end;
    return { begin, std::size_t(end - begin) };
}


template<class C, class T, class F>
void for_each_word(std::basic_string_view<C, T> str, const std::locale& loc, F func)
{
    core::BasicStringSplitter<C, T> splitter(str, loc); // Throws
    std::basic_string_view<C, T> word;
    while (ARCHON_LIKELY(splitter.next(word)))
        func(word); // Throws
}


template<class C, class T>
inline BasicStringSplitter<C, T>::BasicStringSplitter(string_view_type str, const std::locale& loc, char delim)
    : BasicStringSplitter(str, map_delim(delim, loc)) // Throws
{
}


template<class C, class T>
inline BasicStringSplitter<C, T>::BasicStringSplitter(string_view_type str, char_type delim) noexcept
    : m_cur(str.data())
    , m_end(m_cur + str.size())
    , m_delim(delim)
{
}


template<class C, class T>
bool BasicStringSplitter<C, T>::next(string_view_type& word) noexcept
{
    for (;;) {
        if (ARCHON_LIKELY(m_cur < m_end)) {
            if (ARCHON_LIKELY(*m_cur == m_delim)) {
                ++m_cur;
                continue;
            }
            break;
        }
        return false;
    }
    const char_type* begin = m_cur;
    do ++m_cur;
    while (ARCHON_LIKELY(m_cur < m_end && *m_cur != m_delim));
    const char_type* end = m_cur;
    word = { begin, std::size_t(end - begin) };
    return true;
}


template<class C, class T>
inline auto BasicStringSplitter<C, T>::map_delim(char ch, const std::locale& loc) -> char_type
{
    core::BasicCharMapper<C, T> mapper(loc); // Throws
    return mapper.widen(ch); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STRING_HPP
