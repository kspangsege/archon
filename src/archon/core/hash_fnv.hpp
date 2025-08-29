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

#ifndef ARCHON_X_CORE_X_HASH_FNV_HPP
#define ARCHON_X_CORE_X_HASH_FNV_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <cmath>
#include <type_traits>

#include <archon/core/integer.hpp>


namespace archon::core {


/// \ref Fowler/Noll/Vo hashing.
///
/// This class is an implementation of FNV 1a, which is a non-cryptographic hash function
/// created by Glenn Fowler, Landon Curt Noll, and Kiem-Phong Vo.
///
/// In some cases, an object can be hashed in one go using \ref add_obj() (see caveats in
/// documentation). More generally, an object that consists of N sub-objects, is hashed by
/// constructing a hasher (object of type `Hash_FNV_1a`) and then adding each relevant
/// sub-object sequentially.
///
/// Hashing of an integer is a `constexpr` operation if done using \ref add_int(). Likewise,
/// hashing of a byte is a `constexpr` operation if done using \ref add_byte()
///
/// \sa \ref core::Hash_FNV_1a_32, \ref core::Hash_FNV_1a_64
/// \sa \ref core::Hash_FNV_1a_Default
///
/// \sa http://www.isthe.com/chongo/tech/comp/fnv/index.html
///
template<class T, int W, T B, T P> class Hash_FNV_1a {
public:
    using value_type = T;

    static constexpr int bit_width = W;
    static constexpr value_type offset_basis = B;
    static constexpr value_type prime = P;

    static_assert(core::is_unsigned<value_type>());
    static_assert(bit_width <= core::int_width<value_type>());

    /// \brief Digest single byte.
    ///
    /// This function digests a single byte.
    ///
    constexpr void add_byte(std::byte value) noexcept;

    /// \brief Digest object of integer type.
    ///
    /// This function digests the specified integer object. Contrary to \ref add_obj(), this
    /// function is a `constexpr` operation.
    ///
    /// \sa \ref add_obj()
    ///
    template<class I> constexpr void add_int(I value) noexcept;

    /// \brief Digest specified object.
    ///
    /// This function digests the specified object as a sequence of bytes, i.e., the bytes
    /// that constitute the representation of that object. Note that this scheme is not
    /// appropriate for all types of objects. For example, it is not appropriate for objects
    /// of type `std::string`, because the string itself needs to be hashed, and it is
    /// generally not contained inside the `std::string` object. In many cases, the
    /// application must deal with each member of a class appropriately rather than passing
    /// the entire object to this function.
    ///
    /// \sa \ref add_int()
    ///
    template<class O> void add_obj(const O& obj) noexcept;

    /// \brief Get hashed value.
    ///
    /// This function returns the hashed value.
    ///
    constexpr auto get() const noexcept -> value_type;

    /// \brief Scale hash value to floating-point interval [0, 1).
    ///
    /// This function returns the current hash value (\ref get()) scaled to the half-open
    /// floating-point interval [0, 1). The returned value can be understood as having been
    /// drawn from a uniform random distribution over the half-open interval [0, 1).
    ///
    /// FIXME: Make constexpr when switching to C++23
    ///
    template<class F> auto get_as_float() const noexcept -> F;

private:
    constexpr void add_octet(value_type) noexcept;

    value_type m_hash = offset_basis;
};


/// \brief 32-bit version of FNV 1a hash function
///
/// This is the 32-bit version of FNV 1a, which is a non-cryptographic hash function created
/// by Glenn Fowler, Landon Curt Noll, and Kiem-Phong Vo).
///
/// It uses an offset basis of 2166136261 and the prime 16777619.
///
/// \sa \ref core::Hash_FNV_1a_64
/// \sa \ref core::Hash_FNV_1a_Default
/// \sa \ref core::Hash_FNV_1a
///
using Hash_FNV_1a_32 = core::Hash_FNV_1a<std::uint_fast32_t, 32, 2166136261U, 16777619U>;


/// \brief 64-bit version of FNV 1a hash function
///
/// This is the 64-bit version of FNV 1a, which is a non-cryptographic hash function created
/// by Glenn Fowler, Landon Curt Noll, and Kiem-Phong Vo).
///
/// It uses an offset basis of 14695981039346656037 and the prime 1099511628211.
///
/// \sa \ref core::Hash_FNV_1a_32
/// \sa \ref core::Hash_FNV_1a_Default
/// \sa \ref core::Hash_FNV_1a
///
using Hash_FNV_1a_64 = core::Hash_FNV_1a<std::uint_fast64_t, 64, 14695981039346656037U, 1099511628211U>;


/// \brief Default version of FNV 1a hash function
///
/// This is a type alias for \ref core::Hash_FNV_1a_32 if the number of bits in
/// `std::size_t` is less than or equal to 32. Otherwise this is a type alias for \ref
/// core::Hash_FNV_1a_64.
///
/// \sa \ref core::Hash_FNV_1a_32, \ref core::Hash_FNV_1a_64
///
using Hash_FNV_1a_Default = std::conditional_t<core::int_width<std::size_t>() <= 32, Hash_FNV_1a_32, Hash_FNV_1a_64>;








// Implementation


template<class T, int W, T B, T P>
constexpr void Hash_FNV_1a<T, W, B, P>::add_byte(std::byte value) noexcept
{
    constexpr int width = core::int_width<char>();
    using uchar = unsigned char;
    value_type value_2 = uchar(value);
    if constexpr (width > 8) {
        int offset = 0;
        while (width - offset > 8) {
            add_octet(value_type(value_2 & 0xFF));
            value_2 >>= 8;
            offset += 8;
        }
    }
    add_octet(value_2);
}


template<class T, int W, T B, T P>
template<class I> constexpr void Hash_FNV_1a<T, W, B, P>::add_int(I value) noexcept
{
    static_assert(std::is_integral_v<I>);
    constexpr int width = core::int_width<I>();
    auto value_2 = core::to_unsigned(core::promote(value));
    if constexpr (width > 8) {
        int offset = 0;
        while (width - offset > 8) {
            add_octet(value_type(value_2 & 0xFF));
            value_2 >>= 8;
            offset += 8;
        }
    }
    add_octet(value_type(value_2));
}


template<class T, int W, T B, T P>
template<class O> void Hash_FNV_1a<T, W, B, P>::add_obj(const O& obj) noexcept
{
    const void* ptr = &obj;
    const std::byte* bytes = static_cast<const std::byte*>(ptr);
    for (std::size_t i = 0; i < sizeof (O); ++i)
        add_byte(bytes[i]);
}


template<class T, int W, T B, T P>
constexpr auto Hash_FNV_1a<T, W, B, P>::get() const noexcept -> value_type
{
    return value_type(m_hash & core::int_mask<value_type>(bit_width));
}


template<class T, int W, T B, T P>
template<class F> auto Hash_FNV_1a<T, W, B, P>::get_as_float() const noexcept -> F
{
    static_assert(std::is_floating_point_v<F>);
    return std::ldexp(F(get()), -bit_width);
}


template<class T, int W, T B, T P>
constexpr void Hash_FNV_1a<T, W, B, P>::add_octet(value_type value) noexcept
{
    // Xor the bottom bits with the incoming octet
    m_hash ^= value;

    // Multiply by the 32 bit FNV magic prime
    m_hash *= prime;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_HASH_FNV_HPP
