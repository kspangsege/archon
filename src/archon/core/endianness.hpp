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

#ifndef ARCHON_X_CORE_X_ENDIANNESS_HPP
#define ARCHON_X_CORE_X_ENDIANNESS_HPP

/// \file


#include <archon/core/integer.hpp>
#include <archon/core/enum.hpp>


namespace archon::core {


/// \brief Try to determine whether specified type is stored in big endian form.
///
/// This function returns `true` if \ref core::try_get_byte_order() returns `true` for the
/// same type, and if the determined byte order is \ref core::Endianness::big. Otherwise
/// this function returns `false`.
///
template<class T> bool is_big_endian() noexcept;


/// \brief Try to determine whether specified type is stored in little endian form.
///
/// This function returns `true` if \ref core::try_get_byte_order() returns `true` for the
/// same type, and if the determined byte order is \ref core::Endianness::little. Otherwise
/// this function returns `false`.
///
template<class T> bool is_little_endian() noexcept;


/// \brief Whether byte order can be determined for specified type.
///
/// This function returns `true` when, and only when \ref core::try_get_byte_order() returns
/// `false`.
///
template<class T> bool is_indeterminate_endian() noexcept;


/// \brief Typical options for endianness.
///
/// These are the typical options for endianness.
///
/// Most systems are little-endian, which means that in all the fundamental integer types,
/// the byte with the least significant bits occurs at the lowest address in memory.
///
/// Big-endian is also known as "network byte order" because it is often used as the
/// "neutral" form for transmission over networks. With big-endianness, the byte with the
/// most significant bits occurs at the lowest address in memory.
///
/// Endianness can also be used to specify the order in which integers other than bytes are
/// combined into larger integers.
///
/// When the bits of an integer value are divided into smaller parts (fields), endianness
/// can be used to specify whether the natural order of those parts coincide with rising or
/// falling bit significance. In this case, endianness can be referred to as bit order, and
/// little-endianness would mean that among two parts, the one that occupies the least
/// significant bits is to be considered as coming first.
///
/// A specialization of \ref core::EnumTraits is provided, making stream input and output
/// immediately available.
///
/// \sa \ref core::try_get_byte_order()
///
enum class Endianness {
    big,    ///< Big-endian. Most significant byte comes first in memory.
    little, ///< Little-endian. Least significant byte comes first in memory.
};


/// \brief Try to determine native byte order for integer type.
///
/// This function attempts to determine the byte order in effect on this platform for the
/// specified integer type. If detection succeeds, this function returns `true` after
/// setting \p byte_order to the detected byte order. Otherwise this function returns
/// `false` and leaves \p byte_order unchanged.
///
/// Given the current implementation, if the specified integer type has padding bits, the
/// detection will fail. Guaranteed failure in this case, however, is not something that the
/// caller should rely on.
///
/// If the specified integer type has ambiguous byte order because it is made up of only one
/// byte, this function marks it as big endian.
///
/// The specified type (\p T) must conform to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T> bool try_get_byte_order(core::Endianness& byte_order) noexcept;








// Implementation


template<class T> inline bool is_big_endian() noexcept
{
    core::Endianness byte_order = {};
    return (core::try_get_byte_order<T>(byte_order) && byte_order == core::Endianness::big);
}


template<class T> inline bool is_little_endian() noexcept
{
    core::Endianness byte_order = {};
    return (core::try_get_byte_order<T>(byte_order) && byte_order == core::Endianness::little);
}


template<class T> inline bool is_indeterminate_endian() noexcept
{
    core::Endianness byte_order = {}; // Dummy
    return !core::try_get_byte_order<T>(byte_order);
}


template<> struct EnumTraits<core::Endianness> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(core::Endianness::big),    "big"    },
            { int(core::Endianness::little), "little" },
        };
    };
    static constexpr bool ignore_case = false;
};


template<class T> bool try_get_byte_order(core::Endianness& byte_order) noexcept
{
    constexpr int n = int(sizeof (T));
    constexpr int m = core::int_width<char>();
    if constexpr (n == 1) {
        byte_order = core::Endianness::big;
        return true;
    }
    else if constexpr (core::int_width<T>() == n * m) {
        T val = T(1);
        const std::byte* bytes = reinterpret_cast<const std::byte*>(&val);
        if (bytes[n - 1] == std::byte(1)) {
            // Probably big endian
            int i = 1;
            for (;;) {
                val = core::int_cast_a<T>(core::promote(T(1)) << i * m);
                if (ARCHON_LIKELY(bytes[n - 1 - i] == std::byte(1))) {
                    ++i;
                    if (ARCHON_LIKELY(i < n))
                        continue;
                    byte_order = core::Endianness::big;
                    return true;
                }
                return false;
            }
        }
        else if (bytes[0] == std::byte(1)) {
            // Probably little endian
            int i = 1;
            for (;;) {
                val = core::int_cast_a<T>(core::promote(T(1)) << i * m);
                if (ARCHON_LIKELY(bytes[i] == std::byte(1))) {
                    ++i;
                    if (ARCHON_LIKELY(i < n))
                        continue;
                    byte_order = core::Endianness::little;
                    return true;
                }
                return false;
            }
        }
        return false;
    }
    else {
        return false;
    }
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_ENDIANNESS_HPP
