// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_ITERATOR_HPP
#define ARCHON_X_CORE_X_ITERATOR_HPP

/// \file


namespace archon::core {


/// \brief Sequence of objects specified by iterator pair.
///
/// An object of this type specifies a sequence of objects through the contained iterator
/// pair.
///
/// This might be most useful when an iterator pair needs to be passed to a function that
/// expects a single sequence object, such as \ref core::as_list().
///
template<class I> class iter_seq {
public:
    iter_seq(I begin, I end);

    auto begin() const noexcept -> I;
    auto end() const noexcept -> I;

private:
    I m_begin, m_end;
};








// Implementation


template<class I> inline iter_seq<I>::iter_seq(I begin, I end)
    : m_begin(begin)
    , m_end(end)
{
}


template<class I> inline auto iter_seq<I>::begin() const noexcept -> I
{
    return m_begin;
}


template<class I> inline auto iter_seq<I>::end() const noexcept -> I
{
    return m_end;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_ITERATOR_HPP
