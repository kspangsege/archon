// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_MATH_X_IMPL_X_VEC_ADAPT_REP_HPP
#define ARCHON_X_MATH_X_IMPL_X_VEC_ADAPT_REP_HPP

/// \file


#include <cstddef>


namespace archon::math::impl {


template<std::size_t N, class T, std::size_t P> class VecAdaptRep {
public:
    constexpr VecAdaptRep(T* components) noexcept
        : m_components(components)
    {
    }

    constexpr auto comp(std::size_t i) const noexcept -> T&
    {
        return m_components[i * P];
    }

private:
    T* m_components;
};


} // namespace archon::math::impl

#endif // ARCHON_X_MATH_X_IMPL_X_VEC_ADAPT_REP_HPP
