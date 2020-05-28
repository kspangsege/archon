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

#ifndef ARCHON_X_CORE_X_IS_CONSTEXPR_HPP
#define ARCHON_X_CORE_X_IS_CONSTEXPR_HPP

/// \file


#include <utility>


/// \brief   
///
///   
///
#define ARCHON_IS_CONSTEXPR(expr)               \
    X_ARCHON_IS_CONSTEXPR(expr)








// Implementation


#define X_ARCHON_IS_CONSTEXPR(expr)                                     \
    archon::core::impl::is_constexpr_2(                                 \
        [](auto x) -> archon::core::impl::IsConstexprTmpl<decltype(x)(archon::core::impl::is_constexpr_func(expr))> { \
            return {};                                                  \
        })


namespace archon::core::impl {


template<bool> struct IsConstexprTmpl {};


constexpr bool is_constexpr_func(...)
{
    return false;
}


template<class F, class = decltype(std::declval<F>()(false))> constexpr bool is_constexpr_1(int)
{
    return true;
}

template<class F> constexpr bool is_constexpr_1(long)
{
    return false;
}


template<class F> constexpr bool is_constexpr_2(F&&)
{
    return impl::is_constexpr_1<F>(0);
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IS_CONSTEXPR_HPP
