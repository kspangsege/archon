/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
/// \file
///
/// \author Kristian Spangsege
 */

#ifndef ARCHON_MATH_INTERVAL_HPP
#define ARCHON_MATH_INTERVAL_HPP

#include <cmath>
#include <type_traits>
#include <algorithm>
#include <ostream>


namespace archon {
namespace math {

/// An abstract interval.
template<class T> class BasicInterval {
public:
    static_assert(std::is_nothrow_default_constructible<T>::value, "");
    static_assert(std::is_nothrow_destructible<T>::value, "");
    static_assert(noexcept(T() == T() && T() != T()), "");

    T begin = T(), end = T();

    /// Set beginning and end to zero.
    BasicInterval() noexcept;

    BasicInterval(const T& begin, const T& end);

    /// Construct an origin centered interval of the specified size.
    explicit BasicInterval(const T& size);

    BasicInterval& set(const T& begin, const T& end);

    /// Get the center of this interval.
    T get_center() const;

    /// Get the length of this interval.
    T get_length() const;

    /// Translate this interval by the specified amount. This does not change
    /// the length of the interval, only its location.
    BasicInterval& translate(const T&);

    /// Reflect this interval about the origin.
    BasicInterval& reflect();

    /// Expand this interval just enough to cover the specified one.
    ///
    /// That is, make this interval the least interval that includes both itself
    /// and the specified interval.
    BasicInterval& include(const BasicInterval&);

    /// Scale this interval by the specified scaling factor.
    BasicInterval& operator*=(const T&);

    bool operator==(const BasicInterval&) const noexcept;
    bool operator!=(const BasicInterval&) const noexcept;
};


template<class C, class T, class U>
std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T>&, const BasicInterval<U>&);


using Interval  = BasicInterval<double>;
using IntervalF = BasicInterval<float>;
using IntervalL = BasicInterval<long double>;




// Implementation

template<class T> inline BasicInterval<T>::BasicInterval() noexcept
{
}

template<class T> inline BasicInterval<T>::BasicInterval(const T& b, const T& e):
    begin(b),
    end(e)
{
}

template<class T> inline BasicInterval<T>::BasicInterval(const T& size):
    begin(-0.5*size),
    end(0.5*size)
{
}

template<class T> inline BasicInterval<T>& BasicInterval<T>::set(const T& b, const T& e)
{
    begin = b;
    end = e;
    return *this;
}

template<class T> inline T BasicInterval<T>::get_center() const
{
    return 0.5*(begin + end);
}

template<class T> inline T BasicInterval<T>::get_length() const
{
    using std::abs;
    return abs(end - begin);
}

template<class T> inline BasicInterval<T>& BasicInterval<T>::translate(const T& v)
{
    begin += v;
    end   += v;
    return *this;
}

template<class T> inline BasicInterval<T>& BasicInterval<T>::reflect()
{
    using std::swap;
    swap(begin, end);
    begin = -begin;
    end   = -end;
    return *this;
}

template<class T> inline BasicInterval<T>& BasicInterval<T>::include(BasicInterval<T> const &i)
{
    using std::min;
    using std::max;
    begin = min(begin, i.begin);
    end   = max(end,   i.end);
    return *this;
}

template<class T> inline BasicInterval<T>& BasicInterval<T>::operator*=(const T& v)
{
    begin *= v;
    end *= v;
    return *this;
}

template<class T> inline bool BasicInterval<T>::operator==(const BasicInterval& i) const noexcept
{
    return begin == i.begin && end == i.end;
}

template<class T> inline bool BasicInterval<T>::operator!=(const BasicInterval& i) const noexcept
{
    return begin != i.begin || end != i.end;
}

template<class C, class T, class U>
inline std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T>& out, const BasicInterval<U>& i)
{
    out << '[' << i.begin << ", " << i.end << ']';
    return out;
}

} // namespace math
} // namespace archon

#endif // ARCHON_MATH_INTERVAL_HPP
