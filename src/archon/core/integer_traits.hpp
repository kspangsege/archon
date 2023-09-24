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

#ifndef ARCHON_X_CORE_X_INTEGER_TRAITS_HPP
#define ARCHON_X_CORE_X_INTEGER_TRAITS_HPP

/// \file


#include <type_traits>
#include <bit>
#include <limits>
#include <utility>
#include <array>

#include <archon/core/features.h>


namespace archon::core {


/// \brief Traits of integer types.
///
/// This class template represents a homogeneous low-level interface for working with
/// different kinds of integer types that conform to the "integer" concept as defined in
/// \ref Concept_Archon_Core_Integer.
///
/// Applications are allowed to specialize this class template for new integer types
/// according to the rules layed out in \ref Concept_Archon_Core_Integer.
///
/// If `T` does not fully conform to the integer concept, `IntegerTraits<T>::is_specialized`
/// must be `false`.
///
template<class T> struct IntegerTraits;








// Implementation


namespace impl {


template<class T, bool I> struct IntegerTraits {
    static constexpr bool is_specialized = false;
};


template<> struct IntegerTraits<bool, true> {
    using int_type      = bool;
    using unsigned_type = bool;
    using part_type     = unsigned;

    static constexpr bool is_specialized   = true;
    static constexpr int  num_value_bits   = 1;
    static constexpr bool is_signed        = false;
    static constexpr int  num_parts        = 1;
    static constexpr bool has_divmod       = true;
    static constexpr bool has_find_msb_pos = false;

    static constexpr bool min() noexcept;
    static constexpr bool max() noexcept;
    static constexpr auto get_parts(bool) noexcept -> std::array<unsigned, 1>;
    static constexpr bool from_parts(std::array<unsigned, 1>) noexcept;

    struct DivMod {
        bool quot;
        bool rem;
    };
    static constexpr auto divmod(bool, bool) noexcept -> DivMod;
};


constexpr bool IntegerTraits<bool, true>::min() noexcept
{
    return false;
}


constexpr bool IntegerTraits<bool, true>::max() noexcept
{
    return true;
}


constexpr auto IntegerTraits<bool, true>::get_parts(bool val) noexcept -> std::array<unsigned, 1>
{
    return { unsigned(val) };
}


constexpr bool IntegerTraits<bool, true>::from_parts(std::array<unsigned, 1> parts) noexcept
{
    return bool(parts[0] & 1);
}


constexpr auto IntegerTraits<bool, true>::divmod(bool a, bool b) noexcept -> DivMod
{
    int quot = int(a) / int(b);
    int rem  = int(a) % int(b);
    return { bool(quot & 1), bool(rem & 1) };
}


template<class T> struct IntegerTraits<T, true> {
    using int_type      = T;
    using lim_type      = std::numeric_limits<T>;
    using unsigned_type = std::make_unsigned_t<T>;
    using promoted_type = decltype(+std::declval<T>());
    using part_type     = std::make_unsigned_t<promoted_type>;

    static_assert(lim_type::is_specialized);
    static_assert(lim_type::is_integer);

    static constexpr bool is_specialized   = true;
    static constexpr int  num_value_bits   = lim_type::digits;
    static constexpr bool is_signed        = lim_type::is_signed;
    static constexpr int  width            = num_value_bits + int(is_signed);
    static constexpr int  part_width       = std::numeric_limits<part_type>::digits;
    static constexpr int  num_parts        = (width <= part_width ? 1 : 2);
    static constexpr bool has_divmod       = true;
    static constexpr bool has_find_msb_pos = true;

    static constexpr auto min() noexcept -> T;
    static constexpr auto max() noexcept -> T;
    static constexpr auto get_parts(T) noexcept -> std::array<part_type, num_parts>;
    static constexpr auto from_parts(std::array<part_type, num_parts>) noexcept -> T;
    static constexpr int find_msb_pos(T) noexcept;

    struct DivMod {
        T quot;
        T rem;
    };
    static constexpr auto divmod(T, T) noexcept -> DivMod;
};


template<class T>
constexpr auto IntegerTraits<T, true>::min() noexcept -> T
{
    return lim_type::min();
}


template<class T>
constexpr auto IntegerTraits<T, true>::max() noexcept -> T
{
    return lim_type::max();
}


template<class T>
constexpr auto IntegerTraits<T, true>::get_parts(T val) noexcept -> std::array<part_type, num_parts>
{
    if constexpr (num_parts == 1) {
        return { part_type(val) };
    }
    else {
        static_assert(num_parts == 2);
        static_assert(is_signed);
        return { part_type(val), part_type(val >= 0 ? 0 : -1) };
    }
}


template<class T>
constexpr auto IntegerTraits<T, true>::from_parts(std::array<part_type, num_parts> parts) noexcept -> T
{
    if constexpr (!is_signed) {
        static_assert(num_parts == 1);
        return T(parts[0]);
    }
    else {
        // Signed integer type
        bool nonnegative = ((parts[num_parts - 1] & (part_type(1) << (part_width - 1))) == 0);
        if (ARCHON_LIKELY(nonnegative))
            return T(parts[0]);
        // Negative value
        return T(-1 - promoted_type(part_type(-1) - parts[0]));
    }
}


template<class T>
constexpr int IntegerTraits<T, true>::find_msb_pos(T val) noexcept
{
    constexpr int n = std::numeric_limits<unsigned_type>::digits;
    static_assert(n == width); // Guaranteed by C++20 since T is one of the standard, or extended integer types.
    return (n - 1) - std::countl_zero(unsigned_type(val));
}


template<class T>
constexpr auto IntegerTraits<T, true>::divmod(T a, T b) noexcept -> DivMod
{
    auto quot = a / b;
    auto rem  = a % b;
    return { T(quot), T(rem) };
}


} // namespace impl


template<class T> struct IntegerTraits
    : impl::IntegerTraits<T, std::is_integral_v<T>> {
};


} // namespace archon::core

#endif // ARCHON_X_CORE_X_INTEGER_TRAITS_HPP
