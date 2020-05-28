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
#include <type_traits>

#include <archon/core/span.hpp>
#include <archon/core/integer.hpp>


namespace archon::core {


/// \ref Fowler/Noll/Vo hashing.
///
/// This class is an implementation of the non-cryptographic hash function created by Glenn
/// Fowler, Landon Curt Noll, and Kiem-Phong Vo.
///
/// In some cases, an object can be hashed in one go using \ref add_obj() (see caveats in
/// documentation). More generally, an object that consists of N subobjects, is hashed by
/// constructing a hasher (object of type `Hash_FNV_1a_32`) and then adding each relevant
/// subobject sequentially.
///
/// Hashing of an integer is a `constexpr` operation if done using \ref add_int(). Likewise,
/// hashing of a byte is a `constexpr` operation if done using \ref add_byte()
///
/// \sa http://www.isthe.com/chongo/tech/comp/fnv/index.html
///
class Hash_FNV_1a_32 {
public:
    using value_type = std::uint_fast32_t;

    /// \brief Digest single byte.
    ///
    /// This function digests a single byte.
    ///
    constexpr void add_byte(std::byte value) noexcept;

    /// \brief Digest object of integer type.
    ///
    /// This function digests the specified integer object. Contrary to \ref add(), this
    /// function is a `constexpr` operation.
    ///
    /// \sa \ref add_obj()
    ///
    template<class T> constexpr void add_int(T value) noexcept;

    /// \brief Digest specified object.
    ///
    /// This function digests the specified object as a sequence of bytes, i.e., the bytes
    /// that constitute the representation of that object. Note that this scheme is not
    /// appropriate for all types of objects. For example, it is not appropriate for objects
    /// of type `std::string`, because the string itself needs to be hashed, and iit is
    /// genrally not contained inside the `std::string` object. In many cases, the
    /// application must deal with each member of a class appropriately rather than passing
    /// the entire object to this function.
    ///
    /// \sa \ref add_int()
    ///
    template<class T> void add_obj(const T& value) noexcept;

    /// \brief Get hashed value.
    ///
    /// This function returns the hashed value.
    ///
    constexpr auto get() const noexcept -> value_type;

private:
    constexpr void add_octet(value_type) noexcept;

    std::uint_fast32_t m_hash = 2166136261;
};








// Implementation


constexpr void Hash_FNV_1a_32::add_byte(std::byte value) noexcept
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


template<class T> constexpr void Hash_FNV_1a_32::add_int(T value) noexcept
{
    static_assert(std::is_integral_v<T>);
    constexpr int width = core::int_width<T>();
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


template<class T> void Hash_FNV_1a_32::add_obj(const T& value) noexcept
{
    const void* ptr = &value;
    const std::byte* bytes = static_cast<const std::byte*>(ptr);
    for (std::size_t i = 0; i < sizeof (T); ++i)
        add_byte(bytes[i]);
}


constexpr auto Hash_FNV_1a_32::get() const noexcept -> value_type
{
    return value_type(m_hash & 0xFFFFFFFF);
}


constexpr void Hash_FNV_1a_32::add_octet(value_type value) noexcept
{
    // Xor the bottom bits with the incoming octet
    m_hash ^= value;

    // Multiply by the 32 bit FNV magic prime mod 2^32
    m_hash *= 16777619;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_HASH_FNV_HPP
