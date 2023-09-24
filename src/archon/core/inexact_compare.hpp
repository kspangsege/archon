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

#ifndef ARCHON_X_CORE_X_INEXACT_COMPARE_HPP
#define ARCHON_X_CORE_X_INEXACT_COMPARE_HPP

/// \file


#include <cmath>
#include <algorithm>


namespace archon::core {


/// \{
///
/// \brief Inexact floating point comparisons.
///
/// These are the four inexact floating point comparisons defined by Donald. E. Knuth. in
/// volume II of his "The Art of Computer Programming" 3rd edition, section 4.2.2 "Accuracy
/// of Floating Point Arithmetic", definitions (21)-(24):
///
///   | Comparison              | Meaning
///   |-------------------------|-------------------------------------------
///   | approximately equal     | abs(a-b) <= max(abs(a), abs(b)) * epsilon
///   | essentially equal       | abs(a-b) <= min(abs(a), abs(b)) * epsilon
///   | definitely less than    | b - a    >  max(abs(a), abs(b)) * epsilon
///   | definitely greater than | a - b    >  max(abs(a), abs(b)) * epsilon
///
/// In general you should set \p epsilon to some small multiple of the machine epsilon for
/// the floating point type used in your computations
/// (e.g. `std::numeric_limits<double>::epsilon()`). As a general rule, a longer and more
/// complex computation needs a higher multiple of the machine epsilon.
///
bool approximately_equal(long double a, long double b, long double epsilon) noexcept;
bool essentially_equal(long double a, long double b, long double epsilon) noexcept;
bool definitely_less(long double a, long double b, long double epsilon) noexcept;
bool definitely_greater(long double a, long double b, long double epsilon) noexcept;
/// \}








// Implementation


inline bool approximately_equal(long double a, long double b, long double epsilon) noexcept
{
    return (std::abs(a - b) <= std::max(std::abs(a), std::abs(b)) * epsilon);
}


inline bool essentially_equal(long double a, long double b, long double epsilon) noexcept
{
    return (std::abs(a - b) <= std::min(std::abs(a), std::abs(b)) * epsilon);
}


inline bool definitely_less(long double a, long double b, long double epsilon) noexcept
{
    return (b - a > std::max(std::abs(a), std::abs(b)) * epsilon);
}


inline bool definitely_greater(long double a, long double b, long double epsilon) noexcept
{
    return core::definitely_less(b, a, epsilon);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_INEXACT_COMPARE_HPP
