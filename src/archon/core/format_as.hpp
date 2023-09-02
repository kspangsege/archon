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

#ifndef ARCHON_X_CORE_X_FORMAT_AS_HPP
#define ARCHON_X_CORE_X_FORMAT_AS_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <cmath>
#include <type_traits>
#include <limits>
#include <utility>
#include <array>
#include <stdexcept>
#include <ios>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/scope_exit.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/format_with.hpp>


namespace archon::core {



/// \brief Format a number as an ordinal.
///
/// Construct an object that, if written to an output stream, formats the specified integer
/// as an ordinal. Given the number 2, for example, the output will be `2nd`.
///
/// The field width of the target stream will be respected, and the effect will be as if all
/// of the generated output was written to the stream as a single string object.
///
/// The specified value (\p value) can have any type that conforms to the integer concept
/// (\ref Concept_Archon_Core_Integer).
///
template<class T> auto as_ordinal(T value) noexcept;



/// \brief Used with as_num_of().
///
/// See as_num_of() for information on how to use this type.
///
struct NumOfSpec {
    const char* singular_form;
    const char* plural_form;
};


/// \brief Format a number of a certain kind of thing.
///
/// Construct an object that, if written to an output stream, formats a number as a number
/// of a certain kind of thing. The specified number must be an integer or a floating point
/// number. Given the number 2 and the spec `{ "car", "cars" }`, for example, the output
/// will be `2 cars`.
///
/// The spec (\p spec) can be immediately specified or be stored in a variable for reuse:
///
/// \code{.cpp}
///
///   std::cout << core::as_num_of(2, { "car", "cars" });
///
///   core::NumOfSpec cars_spec = { "car", "cars" };
///   std::cout << core::as_num_of(2, cars_spec);
///
/// \endcode
///
/// The field width of the target stream will be respected, and the effect will be as if all
/// of the generated output was written to the stream as a single string object.
///
template<class T> auto as_num_of(T value, const core::NumOfSpec& spec) noexcept;



/// \brief Format fraction of one as percent.
///
/// Construct an object that, if written to an output stream, formats the specified number
/// using the percent notation. Here, 1 maps to `100%`. The jump from `99%` to `100%`
/// happens only when the specified value becomes greater than, or equal to 1.
///
/// The specified value must be something that is explicitly convertible to `double`.
///
/// The maximum allowed number of decimals is 6.
///
template<class T> auto as_percent(T val, int num_decimals = 0);



/// \brief Format an amount of time.
///
/// Construct an object that, if written to an output stream, formats an amount of time
/// using various forms depending on magnitude. The time is specified as a number of
/// seconds. If the time is less than a minute, it will be formatted as if by
/// `as_quant(value, "s")`. Otherwise, if it is less than an our, it will be formatted as
/// "minutes and seconds", such as in `3m17s`. Otherwise, it will be formatted as "hours and
/// minutes", such as in `9h41m`.
///
/// \sa \ref as_quant()
///
template<class T> auto as_time(double value) noexcept;



/// \brief Format an amount of memory.
///
/// Construct an object that, if written to an output stream, formats a number as an amount
/// of memory. This is a shorthand for `as_quant_bin(value, " B")` unless \p value is
/// small. If the value is small (less than 1024), it is instead a shorthand for
/// `as_num_of(value, { "byte", "bytes" })`.
///
/// \sa \ref as_quant_bin()
///
auto as_byte_size(double value) noexcept;



/// \brief Format a physical quantity using decadic prefixes.
///
/// Construct an object that, if written to an output stream, formats a number as a physical
/// quantity in the specified unit and using a decadic prefix when appropriate. The number
/// will be formatted with three significant digits, although trailing zeroes after the
/// decimal point will be omitted. Also, the decimal point will be omitted if there are no
/// digits after it.
///
/// The applicable decadic prefixes are:
///
///   | Value   | Prefix   | Name
///   |---------|----------|-------
///   |  1000^8 | `Y`      | yotta
///   |  1000^7 | `Z`      | zetta
///   |  1000^6 | `E`      | exa
///   |  1000^5 | `P`      | peta
///   |  1000^4 | `T`      | tera
///   |  1000^3 | `G`      | giga
///   |  1000^2 | `M`      | mega
///   |  1000^1 | `k`      | kilo
///   | 1000^-1 | `m`      | milli
///   | 1000^-2 | `u` (mu) | micro
///   | 1000^-3 | `n`      | nano
///   | 1000^-4 | `p`      | pico
///   | 1000^-5 | `f`      | femto
///   | 1000^-6 | `a`      | atto
///   | 1000^-7 | `z`      | zepto
///   | 1000^-8 | `y`      | yocto
///
/// When a prefix is used, it will be inserted after any leading space in the specified unit
/// string (\p unit).
///
/// Given the number 9'192'631'770, and the unit `" Hz"`, for example, the output will be
/// `9.19 GHz` (hyperfine transition frequency of caesium-133).
///
/// \sa \ref as_quant_bin()
///
auto as_quant(double value, const char* unit) noexcept;



/// \brief Format a number as a physical quantity using binary prefixes.
///
/// Construct an object that, if written to an output stream, formats a number as a physical
/// quantity in the specified unit and using a binary prefix when appropriate. The number
/// will be formatted with three significant digits, although trailing zeroes after the
/// decimal point will be omitted. Also, the decimal point will be omitted if there are no
/// digits after it.
///
/// The binary prefixes are:
///
///   | Value  | Prefix   | Name
///   |--------|----------|-------
///   | 1024^1 | `Ki`     | kibi
///   | 1024^2 | `Mi`     | mebi
///   | 1024^3 | `Gi`     | gibi
///   | 1024^4 | `Ti`     | tebi
///   | 1024^5 | `Pi`     | pebi
///   | 1024^6 | `Ei`     | exbi
///   | 1024^7 | `Zi`     | zebi
///   | 1024^8 | `Yi`     | yobi
///
/// When a prefix is used, it will be inserted after any leading space in the specified unit
/// string (\p unit).
///
/// Given the number 10'000, and the unit `" B"`, for example, the output will be `9.77
/// KiB`.
///
auto as_quant_bin(double value, const char* unit) noexcept;



/// \brief Format complex value using stand-in output stream.
///
/// This function constructs an object, that, if written to a target output stream,
/// constructs a stand-in output stream and then invokes the specified function (\p func)
/// passing the stand-in output stream as argument. Anything written to the stand-in output
/// stream by the function, will be written atomically to the target output stream when the
/// function returns. This behavior is achieved through use of \ref
/// core::BasicStreamOutputAltHelper.
///
/// This function is useful when compound values need to be formatted as atomic elements, or
/// when formatting values in contexts where the target output stream is implicit (e.g.,
/// when using \ref core::as_list() with a function argument) and the value type is not
/// intrinsically formattable. Here is an example of the latter where `out` is of type
/// `std::ostream`, `values` is a sequence of values:
///
/// \code{.cpp}
///
///   out << archon::core::as_list(values, [&](const auto& value) {
///       return archon::core::as_format_func([&](std::ostream& out_2) {
///       if (value.is_special) {
///           out_2 << value.special_value;
///       }
///       else {
///           out_2 << value.normal_value;
///       }
///   });
///
/// \endcode
///
template<class F> auto as_format_func(F func);








// Implementation


namespace impl {


template<class T> struct AsOrdinal {
    T value;
};


template<class T> struct AsNumOf {
    T value;
    const void* spec;
};


struct AsPercent {
    double value;
    int num_decimals;
};


struct AsTime {
    double value;
};


struct AsByteSize {
    double value;
};


struct AsQuantDec {
    double value;
    const char* unit;
};


struct AsQuantBin {
    double value;
    const char* unit;
};


template<class F> struct AsFormatFunc {
    F func;
};


} // namespace impl


template<class T> inline auto as_ordinal(T value) noexcept
{
    static_assert(core::is_integer<T>());
    return impl::AsOrdinal<T> { value };
}


template<class T> inline auto as_num_of(T value, const core::NumOfSpec& spec) noexcept
{
    static_assert(std::is_arithmetic_v<T>);
    static_assert(std::numeric_limits<T>::is_specialized);
    return impl::AsNumOf<T> { value, &spec };
}


template<class T> inline auto as_percent(T value, int num_decimals)
{
    static_assert(std::is_arithmetic_v<T>);
    using fp_type = decltype(impl::AsPercent::value);
    return impl::AsPercent { fp_type(value), num_decimals }; // Throws
}


inline auto as_time(double value) noexcept
{
    return impl::AsTime { value };
}


inline auto as_byte_size(double value) noexcept
{
    return impl::AsByteSize { value };
}


inline auto as_quant(double value, const char* unit) noexcept
{
    return impl::AsQuantDec { value, unit };
}


inline auto as_quant_bin(double value, const char* unit) noexcept
{
    return impl::AsQuantBin { value, unit };
}


template<class F> inline auto as_format_func(F func)
{
    return impl::AsFormatFunc<F> { std::move(func) }; // Throws
}


namespace impl {


// Largest value such that all values, that are strictly smaller than it, and greater than
// one, will be formatted without an exponent part when formatted in default mode
// (std::defaultfloat, `std::printf("%g", ...)`) with precision set to 3.
///
// The precision must be at least 3 for the implemented scheme (in as_quant() and
// as_quant_bin()) to work at all. Also, for optimal results, it should not be greater than
// 3. The reason is as follows: Each increment of the precision above 3, would require
// insertion of an extra `9` digit after the decimal point in `999.5` in the value returned
// by as_quant_limit(). However, such values would not have an exact representation in
// `double`, so there would be a risk of rounding to the wrong side and that would produce
// spurious results for some values.
//
constexpr double as_quant_limit() noexcept
{
    return 999.5;
}


template<class C, class T, class U>
auto operator<<(std::basic_ostream<C, T>& out, impl::AsOrdinal<U> pod) -> std::basic_ostream<C, T>&
{
    auto val = core::promote_strongly(pod.value);
    using type = decltype(val);
    std::array<C, 32> seed_memory;
    core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
    const char* suffixes[] = { "th", "st", "nd", "rd" };
    int last_two_digits = int(val % type(100));
    if constexpr (core::is_signed<U>()) {
        if (last_two_digits < 0)
            last_two_digits = -last_two_digits;
    }
    int suffx_index = 0;
    if (last_two_digits <= 3) {
        suffx_index = last_two_digits;
    }
    else if (last_two_digits >= 20) {
        int last_digit = last_two_digits % 10;
        if (last_digit <= 3)
            suffx_index = last_digit;
    }
    // Use simple "locale independent" integer formatter
    helper.out << core::as_int(val) << suffixes[suffx_index]; // Throws
    helper.flush(); // Throws
    return out;
}


template<class C, class T, class U>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsNumOf<U>& pod) -> std::basic_ostream<C, T>&
{
    std::array<C, 32> seed_memory;
    core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
    using lim = std::numeric_limits<U>;
    bool need_singular = (pod.value == U(1) || (lim::is_signed && pod.value == U(-1)));
    const core::NumOfSpec& spec = *static_cast<const core::NumOfSpec*>(pod.spec);
    const char* form = (need_singular ? spec.singular_form : spec.plural_form);
    helper.out << pod.value << " " << form; // Throws
    helper.flush(); // Throws
    return out;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsPercent& pod) -> std::basic_ostream<C, T>&
{
    // Not using floating point formatting here, as rounding has to be towards zero.
    using fp_type = decltype(pod.value);
    using int_type = long;
    constexpr int max_num_decimals = 6;
    static_assert(std::numeric_limits<fp_type>::digits10 >= 3 + max_num_decimals);
    static_assert(std::numeric_limits<int_type>::digits10 >= 3 + max_num_decimals);
    int n = pod.num_decimals;
    if (ARCHON_UNLIKELY(n < 0 || n > max_num_decimals))
        throw std::invalid_argument("Invalid number of decimals");
    int_type multiplier_1 = 10;
    bool success_1 = core::try_int_pow(multiplier_1, n);
    ARCHON_ASSERT(success_1);
    int_type multiplier_2 = multiplier_1;
    bool success_2 = core::try_int_mul(multiplier_2, 100);
    ARCHON_ASSERT(success_2);
    fp_type val = std::floor(pod.value * fp_type(multiplier_2)) / fp_type(multiplier_1); // Throws
    return out << core::formatted("%s%%", core::with_fixed(val, n)); // Throws
}


template<class C, class T> auto operator<<(std::basic_ostream<C, T>& out, AsTime pod) -> std::basic_ostream<C, T>&
{
    auto round = [](double value) {
        return core::float_to_int<std::uint_fast64_t>(std::floor(value + 0.5)); // Throws
    };
    double value = std::abs(pod.value);
    std::int_fast64_t minutes = round(value / 60); // Throws
    if (minutes >= 60) {
        // 1h0m -> inf
        std::int_fast64_t hours     = minutes / 60;
        std::int_fast64_t minutes_2 = minutes % 60;
        if (pod.value < 0)
            hours = -hours;
        std::array<C, 16> seed_memory;
        core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
        helper.out << hours << "h" << minutes_2 << "m"; // Throws
        helper.flush(); // Throws
        return out;
    }
    std::int_fast64_t seconds = round(value); // Throws
    if (seconds >= 60) {
        // 1m0s -> 59m59s
        std::int_fast64_t minutes   = seconds / 60;
        std::int_fast64_t seconds_2 = seconds % 60;
        if (pod.value < 0)
            minutes = -minutes;
        std::array<C, 16> seed_memory;
        core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
        helper.out << minutes << "m" << seconds_2 << "s"; // Throws
        helper.flush(); // Throws
        return out;
    }
    out << as_quant(pod.value, "s"); // Throws
    return out;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, impl::AsByteSize pod) -> std::basic_ostream<C, T>&
{
    if (std::abs(pod.value) >= as_quant_limit()) {
        out << as_quant_bin(pod.value, " B"); // Throws
    }
    else {
        out << as_num_of(pod.value, { "byte", "bytes" }); // Throws
    }
    return out;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsQuantDec& pod) -> std::basic_ostream<C, T>&
{
    std::array<C, 16> seed_memory;
    core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
    const char* small_prefixes[] = { "", "m", "u", "n", "p", "f", "a", "z", "y" };
    const char* large_prefixes[] = {     "k", "M", "G", "T", "P", "E", "Z", "Y" };
    double value = std::abs(pod.value);
    double limit = as_quant_limit();
    const char* prefix = nullptr;
    if (value < limit) {
        std::size_t n = sizeof small_prefixes / sizeof *small_prefixes;
        std::size_t i = 0;
        for (;;) {
            double value_2 = value * 1000;
            if (value_2 >= limit)
                break;
            std::size_t i_2 = i + 1;
            if (ARCHON_UNLIKELY(i_2 == n))
                break;
            value = value_2;
            i = i_2;
        }
        prefix = small_prefixes[i];
    }
    else {
        std::size_t n = sizeof large_prefixes / sizeof *large_prefixes;
        std::size_t i = 0;
        for (;;) {
            value /= 1000;
            if (value < limit)
                break;
            std::size_t i_2 = i + 1;
            if (ARCHON_UNLIKELY(i_2 == n))
                break;
            i = i_2;
        }
        prefix = large_prefixes[i];
    }
    if (pod.value < 0)
        value = -value;
    {
        auto orig_flags = helper.out.setf({}, std::ios_base::floatfield | std::ios_base::showpoint);
        auto orig_prec = helper.out.precision(3); // Must be 3 (see as_quant_limit())
        ARCHON_SCOPE_EXIT {
            helper.out.flags(orig_flags);
            helper.out.precision(orig_prec);
        };
        helper.out << value; // Throws
    }
    const char* unit = pod.unit;
    while (*unit == ' ') {
        helper.out << " "; // Throws
        ++unit;
    }
    helper.out << prefix << unit; // Throws
    helper.flush(); // Throws
    return out;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const impl::AsQuantBin& pod) -> std::basic_ostream<C, T>&
{
    std::array<C, 16> seed_memory;
    core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
    const char* binary_prefixes[] = { "", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi" };
    double value = std::abs(pod.value);
    double limit = as_quant_limit();
    const char* prefix = nullptr;
    {
        std::size_t n = sizeof binary_prefixes / sizeof *binary_prefixes;
        std::size_t i = 0;
        for (;;) {
            if (value < limit)
                break;
            std::size_t i_2 = i + 1;
            if (ARCHON_UNLIKELY(i_2 == n))
                break;
            value /= 1024;
            i = i_2;
        }
        prefix = binary_prefixes[i];
    }
    if (pod.value < 0)
        value = -value;
    {
        auto orig_flags = helper.out.setf({}, std::ios_base::floatfield | std::ios_base::showpoint);
        auto orig_prec = helper.out.precision(3); // Must be 3 (see as_quant_limit())
        ARCHON_SCOPE_EXIT {
            helper.out.flags(orig_flags);
            helper.out.precision(orig_prec);
        };
        helper.out << value; // Throws
    }
    const char* unit = pod.unit;
    while (*unit == ' ') {
        helper.out << " "; // Throws
        ++unit;
    }
    helper.out << prefix << unit; // Throws
    helper.flush(); // Throws
    return out;
}


template<class C, class T, class F>
inline auto operator<<(std::basic_ostream<C, T>& out, const impl::AsFormatFunc<F>& pod) -> std::basic_ostream<C, T>&
{
    std::array<C, 16> seed_memory;
    core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
    pod.func(helper.out); // Throws
    helper.flush(); // Throws
    return out;
}


} // namespace impl
} // namespace archon::core

#endif // ARCHON_X_CORE_X_FORMAT_AS_HPP
