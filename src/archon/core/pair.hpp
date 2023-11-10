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


#include <type_traits>
#include <utility>


namespace archon::core {


/// \brief Pair concept with strong exception guarantees.
///
/// This class serves purposes similar to those of `std::pair` but provides stronger
/// exception guarantees.
///
template<class T, class U> class Pair {
public:
    using first_type  = T;
    using second_type = U;

    /// \{
    ///
    /// \brief Components of pair.
    ///
    /// These are the two components of the pair.
    ///
    first_type  first;
    second_type second;
    /// \}

    /// \brief Construct pair from two components.
    ///
    /// This constructor constructs a pair from its two components.
    ///
    template<class V, class W> constexpr Pair(V&&, W&&)
        noexcept(std::is_nothrow_constructible_v<T, V&&> && std::is_nothrow_constructible_v<U, W&&>);

    auto operator<=>(const Pair&) const = default;
};








// Implementation


template<class T, class U>
template<class V, class W> constexpr Pair<T, U>::Pair(V&& f, W&& s)
    noexcept(std::is_nothrow_constructible_v<T, V&&> && std::is_nothrow_constructible_v<U, W&&>)
    : first(std::forward<V>(f)) // Throws
    , second(std::forward<W>(s)) // Throws
{
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_PAIR_HPP
