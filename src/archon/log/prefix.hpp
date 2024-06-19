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

#ifndef ARCHON_X_LOG_X_PREFIX_HPP
#define ARCHON_X_LOG_X_PREFIX_HPP

/// \file


#include <string_view>
#include <ostream>


namespace archon::log {


template<class C, class T = std::char_traits<C>> class BasicPrefix {
public:
    using ostream_type = std::basic_ostream<C, T>;

    virtual bool is_null_prefix() const noexcept;
    virtual void format_prefix(ostream_type&) const = 0;

protected:
    ~BasicPrefix() noexcept = default;
};


using Prefix     = BasicPrefix<char>;
using WidePrefix = BasicPrefix<wchar_t>;




template<class C, class T = std::char_traits<C>> class BasicNullPrefix
    : public log::BasicPrefix<C, T> {
public:
    using ostream_type = std::basic_ostream<C, T>;

    // Overriding functions from BasicPrefix
    bool is_null_prefix() const noexcept override final;
    void format_prefix(ostream_type&) const override final;
};


using NullPrefix     = BasicNullPrefix<char>;
using WideNullPrefix = BasicNullPrefix<wchar_t>;




template<class C, class T = std::char_traits<C>> class BasicCompoundPrefix final
    : public log::BasicPrefix<C, T> {
public:
    using ostream_type = std::basic_ostream<C, T>;
    using prefix_type  = log::BasicPrefix<C, T>;

    BasicCompoundPrefix(const prefix_type& left, const prefix_type& right) noexcept;

    auto get_simplified() const noexcept -> const prefix_type&;

    // Overriding functions from Prefix
    void format_prefix(ostream_type&) const override;

private:
    const prefix_type& m_left;
    const prefix_type& m_right;
};


using CompoundPrefix     = BasicCompoundPrefix<char>;
using WideCompoundPrefix = BasicCompoundPrefix<wchar_t>;








// Implementation


// ============================ BasicPrefix ============================


template<class C, class T>
bool BasicPrefix<C, T>::is_null_prefix() const noexcept
{
    return false;
}



// ============================ BasicNullPrefix ============================


template<class C, class T>
bool BasicNullPrefix<C, T>::is_null_prefix() const noexcept
{
    return true;
}


template<class C, class T>
void BasicNullPrefix<C, T>::format_prefix(ostream_type&) const
{
}



// ============================ BasicCompoundPrefix ============================


template<class C, class T>
inline BasicCompoundPrefix<C, T>::BasicCompoundPrefix(const prefix_type& left, const prefix_type& right) noexcept
    : m_left(left)
    , m_right(right)
{
}


template<class C, class T>
auto BasicCompoundPrefix<C, T>::get_simplified() const noexcept -> const prefix_type&
{
    if (m_right.is_null_prefix())
        return m_left;
    if (m_left.is_null_prefix())
        return m_right;
    return *this;
}


template<class C, class T>
void BasicCompoundPrefix<C, T>::format_prefix(ostream_type& out) const
{
    m_left.format_prefix(out); // Throws
    m_right.format_prefix(out); // Throws
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_PREFIX_HPP
