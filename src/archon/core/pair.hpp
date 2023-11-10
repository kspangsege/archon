// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_PAIR_HPP
#define ARCHON_X_CORE_X_PAIR_HPP

/// \file


namespace archon::core {


/// \brief Pair concept with strong exception guarantees.
///
/// This class serves purposes similar to that of std::pair but provides stronger exception
/// guarantees.
///
template<class T, class U> struct Pair {
    T first;
    U second;

    /// \brief Comparison operator.
    ///
    /// This operator compares the two pairs. It allows for pairs to be sorted, and to be
    /// used as keys in maps.
    ///
    constexpr bool operator<(const Pair& other) const noexcept;
};








// Implementation


template<class T, class U> constexpr bool Pair<T, U>::operator<(const Pair& other) const noexcept
{
    static_assert(noexcept(first < other.first));
    static_assert(noexcept(first == other.first));
    static_assert(noexcept(second < other.second));
    return (first < other.first || (first == other.first && second < other.second));
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_PAIR_HPP
