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

#ifndef ARCHON_X_CORE_X_TEXT_PARSER_HPP
#define ARCHON_X_CORE_X_TEXT_PARSER_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <utility>
#include <string_view>
#include <vector>

#include <archon/core/features.h>
#include <archon/core/utility.hpp>
#include <archon/core/string.hpp>
#include <archon/core/value_parser.hpp>


namespace archon::core {


class TextParserBase {
public:
    enum class Error {
        missing_value,
        bad_value,
        too_many_values,
    };
};


template<class C, class T = std::char_traits<C>> class BasicTextParser
    : public TextParserBase {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type  = std::basic_string_view<C, T>;
    using value_parser_type = core::BasicValueParser<C, T>;

    BasicTextParser(value_parser_type&) noexcept;

    template<class F, class I> bool parse(string_view_type str, char_type delim, const F& fields, Error&,
                                          string_view_type& value, I& ident, std::size_t& pos);

    template<class F, class G, class I> bool parse(string_view_type str, char_type delim, const F& fields,
                                                   const G& field_seq, std::size_t min, std::size_t max, Error&,
                                                   string_view_type& value, I& ident, std::size_t& pos);

    template<class U, class I>          static auto field(U&& ref, I&& ident);
    template<class U, class I, class D> static auto field(U&& ref, I&& ident, D&& cond);

    template<class U, class I>          static auto field_seq(std::vector<U>& ref, I&& ident);
    template<class U, class I, class D> static auto field_seq(std::vector<U>& ref, I&& ident, D&& cond);

private:
    value_parser_type& m_value_parser;

    template<class U, class I>          struct Field1;
    template<class U, class I, class D> struct Field2;
    template<class U, class I>          struct FieldSeq1;
    template<class U, class I, class D> struct FieldSeq2;

    template<class U, class I>          bool set(string_view_type, const Field1<U, I>&);
    template<class U, class I, class D> bool set(string_view_type, const Field2<U, I, D>&);

    template<class U, class I>          bool add(string_view_type, const FieldSeq1<U, I>&);
    template<class U, class I, class D> bool add(string_view_type, const FieldSeq2<U, I, D>&);
};


using TextParser     = BasicTextParser<char>;
using WideTextParser = BasicTextParser<wchar_t>;








// Implementation


template<class C, class T>
template<class U, class I> struct BasicTextParser<C, T>::Field1 {
    U var; // May be a reference
    I ident;
};


template<class C, class T>
template<class U, class I, class D> struct BasicTextParser<C, T>::Field2 {
    U var; // May be a reference
    I ident;
    D cond;
};


template<class C, class T>
template<class U, class I> struct BasicTextParser<C, T>::FieldSeq1 {
    std::vector<U>& vec;
    I ident;
};


template<class C, class T>
template<class U, class I, class D> struct BasicTextParser<C, T>::FieldSeq2 {
    std::vector<U>& vec;
    I ident;
    D cond;
};


template<class C, class T>
inline BasicTextParser<C, T>::BasicTextParser(value_parser_type& value_parser) noexcept
    : m_value_parser(value_parser)
{
}


template<class C, class T>
template<class F, class I>
bool BasicTextParser<C, T>::parse(string_view_type str, char_type delim, const F& fields, Error& error,
                                  string_view_type& value, I& ident, std::size_t& pos)
{
    core::BasicStringSplitter<C, T> splitter(str, delim); // Throws
    bool success = core::for_each_tuple_elem_a(fields, [&](const auto& field) {
        string_view_type word;
        if (ARCHON_LIKELY(splitter.next(word))) {
            if (ARCHON_LIKELY(set(word, field))) // Throws
                return true;
            error = Error::bad_value;
            value = word;
            pos = std::size_t(word.data() - str.data());
        }
        else {
            error = Error::missing_value;
            pos = str.size();
        }
        ident = field.ident; // Throws
        return false;
    });
    if (ARCHON_LIKELY(success)) {
        string_view_type word;
        if (ARCHON_LIKELY(!splitter.next(word)))
            return true; // Success
        error = Error::too_many_values;
        value = word;
        pos = std::size_t(word.data() - str.data());
    }
    return false; // Failure
}


template<class C, class T>
template<class F, class G, class I>
bool BasicTextParser<C, T>::parse(string_view_type str, char_type delim, const F& fields, const G& field_seq,
                                  std::size_t min, std::size_t max, Error& error, string_view_type& value, I& ident,
                                  std::size_t& pos)
{
    core::BasicStringSplitter<C, T> splitter(str, delim); // Throws
    bool success = core::for_each_tuple_elem_a(fields, [&](const auto& field) {
        string_view_type word;
        if (ARCHON_LIKELY(splitter.next(word))) {
            if (ARCHON_LIKELY(set(word, field))) // Throws
                return true;
            error = Error::bad_value;
            value = word;
            pos = std::size_t(word.data() - str.data());
        }
        else {
            error = Error::missing_value;
            pos = str.size();
        }
        ident = field.ident; // Throws
        return false;
    });
    if (ARCHON_LIKELY(success)) {
        std::size_t i = 0;
        string_view_type word;
        while (ARCHON_LIKELY(splitter.next(word))) {
            if (ARCHON_LIKELY(i < max)) {
                if (ARCHON_LIKELY(add(word, field_seq))) { // Throws
                    ++i;
                    continue;
                }
                error = Error::bad_value;
                ident = field_seq.ident; // Throws
            }
            else {
                error = Error::too_many_values;
            }
            value = word;
            pos = std::size_t(word.data() - str.data());
            return false; // Failure
        }
        if (ARCHON_LIKELY(i >= min))
            return true; // Success
        error = Error::missing_value;
        ident = field_seq.ident; // Throws
        pos = str.size();
    }
    return false; // Failure
}


template<class C, class T>
template<class U, class I> inline auto BasicTextParser<C, T>::field(U&& ref, I&& ident)
{
    Field1<U, I> field {
        std::forward<U>(ref),
        std::forward<I>(ident),
    }; // Throws
    return field;
}


template<class C, class T>
template<class U, class I, class D> inline auto BasicTextParser<C, T>::field(U&& ref, I&& ident, D&& cond)
{
    Field2<U, I, D> field {
        std::forward<U>(ref),
        std::forward<I>(ident),
        std::forward<D>(cond),
    }; // Throws
    return field;
}


template<class C, class T>
template<class U, class I> inline auto BasicTextParser<C, T>::field_seq(std::vector<U>& ref, I&& ident)
{
    FieldSeq1<U, I> field_seq {
        ref,
        std::forward<I>(ident),
    }; // Throws
    return field_seq;
}


template<class C, class T>
template<class U, class I, class D>
inline auto BasicTextParser<C, T>::field_seq(std::vector<U>& ref, I&& ident, D&& cond)
{
    FieldSeq2<U, I, D> field_seq {
        ref,
        std::forward<I>(ident),
        std::forward<D>(cond),
    }; // Throws
    return field_seq;
}


template<class C, class T>
template<class U, class I> inline bool BasicTextParser<C, T>::set(string_view_type word, const Field1<U, I>& field)
{
    return m_value_parser.parse(word, field.var); // Throws
}


template<class C, class T>
template<class U, class I, class D>
inline bool BasicTextParser<C, T>::set(string_view_type word, const Field2<U, I, D>& field)
{
    std::remove_reference_t<U> var = {};
    if (ARCHON_LIKELY(m_value_parser.parse(word, var))) { // Throws
        if (ARCHON_LIKELY(cond(var))) { // Throws
            field.var = std::move(var);
            return true;
        }
    }
    return false;
}


template<class C, class T>
template<class U, class I>
inline bool BasicTextParser<C, T>::add(string_view_type word, const FieldSeq1<U, I>& field_seq)
{
    U var = {};
    if (ARCHON_LIKELY(m_value_parser.parse(word, var))) { // Throws
        field_seq.vec.push_back(std::move(var)); // Throws
        return true;
    }
    return false;
}


template<class C, class T>
template<class U, class I, class D>
inline bool BasicTextParser<C, T>::add(string_view_type word, const FieldSeq2<U, I, D>& field_seq)
{
    U var = {};
    if (ARCHON_LIKELY(m_value_parser.parse(word, var))) { // Throws
        if (ARCHON_LIKELY(cond(var))) { // Throws
            field_seq.vec.push_back(std::move(var)); // Throws
            return true;
        }
    }
    return false;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_PARSER_HPP
