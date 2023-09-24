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

#ifndef ARCHON_X_CORE_X_ALGORITHM_HPP
#define ARCHON_X_CORE_X_ALGORITHM_HPP

/// \file


#include <archon/core/features.h>


namespace archon::core {


/// \brief Sort elements in a stable fashion at compile time.
///
/// This function performs the same sorting operation as `std::stable_sort()` but in a way
/// that can occur at compile time.
///
/// FIXME: In C++26, `std::stable_sort()` becomes `constexpr`, which makes this function
/// superfluous.
///
template<class I, class C> constexpr void stable_sort(I begin, I end, C compare);








// Implementation


template<class I, class C> constexpr void stable_sort(I begin, I end, C compare)
{
    for (I i = begin; i != end; ++i) {
        for (I j = begin; j != i; ++j) {
            if (ARCHON_LIKELY(!(compare(*i, *j))))
                continue;
            auto val = *i;
            I k = i;
            do {
                I l = k;
                --k;
                *l = *k;
            }
            while (k != j);
            *j = val;
            break;
        }
    }
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_ALGORITHM_HPP
