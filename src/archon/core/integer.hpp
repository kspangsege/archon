/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_CORE_INTEGER_HPP
#define ARCHON_CORE_INTEGER_HPP

#include <limits>
#include <algorithm>

namespace archon {
namespace core {

/// Find position of most significant digit in the specified value and
/// with respect to the specified base/radix. This works for negative
/// values too. Returns -1 if the specified value is zero. Otherwise
/// it always returns a nonnegative value.
template<class T> constexpr int int_find_msd_pos(T value, int base);

/// Find the maximum number of digits needed to represent any positive
/// or negative value of the specified integer type in the specified
/// base/radix. This function does not count the sign itself as a
/// digit.
template<class T> constexpr int int_max_digits(int base);




// Implementation

template<class T> constexpr int int_find_msd_pos(T value, int base)
{
    return (value == 0 ? -1 : 1 + int_find_msd_pos(T(value/base), base));
}

/// Find the maximum number of digits needed to represent any positive
/// or negative value of the specified integer type in the specified
/// base/radix. This function does not count the sign itself as a
/// digit.
template<class T> constexpr int int_max_digits(int base)
{
    using lim = std::numeric_limits<T>;
    static_assert(lim::is_specialized, "");
    static_assert(lim::is_integer, "");
    return 1 + std::max(int_find_msd_pos(lim::min(), base), int_find_msd_pos(lim::max(), base));
}

} // namespace core
} // namespace archon

#endif // ARCHON_CORE_INTEGER_HPP
