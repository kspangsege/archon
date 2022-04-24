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

#ifndef ARCHON_X_IMAGE_X_BIT_MEDIUM_HPP
#define ARCHON_X_IMAGE_X_BIT_MEDIUM_HPP

/// \file


#include <utility>

#include <archon/core/integer.hpp>
#include <archon/image/impl/bit_medium.hpp>


namespace archon::image {


/// \brief Whether type is bit medium of given width.
///
/// This constant is `true` if, and only if the specified type (\p T) is a bit medium of
/// width \p N (see below).
///
///
/// #### Bit medium of width N
///
/// A particular integer type, `T`, is a *bit medium of width N* if it has at least N
/// available bits. Formally, `T` is a bit medium of width N if all of the following are
/// true:
///
/// * `T` conforms to the integer concept (see \ref Concept_Archon_Core_Integer).
///
/// * The inner width of `T` (see below) is greater than, or equal to N.
///
/// * `T` is unsigned (\ref core::is_unsigned()), or the inner width of the corresponding
///   unsigned type (\ref core::unsigned_type) is greater than, or equal to N.
///
/// Here, the *inner width* of a type, `V`, is to be understood as the number returned by
/// \ref core::int_inner_width(), which is the number of "fully covered" bits in the range
/// of values representable in `V`.
///
/// Note that the last criterion ensures that there exists at least one type (possibly the
/// unsigned type) in which a single non-negative value can "carry" all N bits.
///
/// Since C++20, any standard integer type (`std::is_integral`) of width N is guaranteed to
/// be a bit medium of width N. Any unsigned integer type of width N is automatically a bit
/// medium of width N. On the other hand, one can imagine a signed integer type of width N
/// that is not a bit medium of width N, for example, one where the number of value bits in
/// the corresponding unsigned type is equal to the number of value bits in the signed
/// type. Another example is a signed type that uses sign-magnitude representation of
/// negative values.
///
///
/// #### Pack non-negative value into bit medium of width N
///
/// A non-negative N-bit value can be *packed into a bit medium of width N*. The packing
/// operation is defined as follows:
///
/// If the bit medium is unsigned, or the number of value bits in the bit medium is greater
/// than, or equal to N, or the value in its unpacked form is less than two to the power of
/// (N - 1), then the value in its packed form is equal to the value it its unpacked
/// form. Otherwise, the value in its packed form, `p`, which must be negative, is chosen
/// uniquely such that `core::int_cast<U>(p)` is equal to the value in its unpacked form
/// when `U` is `core::unsigned_type<T>`, and `T` is the bit medium.
///
template<class T, int N> constexpr bool is_bit_medium_of_width = impl::is_bit_medium_of_width<T, N>();



/// \brief Default type for holding bit medium values in unpacked form.
///
/// This is the default type for holding values in their unpacked form of the bit medium, \p
/// T, of width \p N. As such, it is the return type of \ref image::unpack_int(). See \ref
/// image::is_bit_medium_of_width for an explanation of the notion of a bit medium. Note
/// that a value in its unpacked form is a non-negative value.
///
/// This type is guaranteed to have at least \p N value bits (\ref core::num_value_bits()),
/// which means that it is able to directly represent any value that can be packed into a
/// bit medium of with \p N. Any such value is a non-negative value.
///
/// Let `P` be `core::promoted_type<T>` (\ref core::promoted_type). Then, if the number of
/// value bits in `P` is greater than, or equal to \p N, `image::unpacked_type<T>` is
/// `P`. Otherwise it is `core::unsigned_type<P>` (\ref core::unsigned_type).
///
/// \tparam T An integer type that is a bit medium of width N (see \ref
/// image::is_bit_medium_of_width).
///
template<class T, int N> using unpacked_type = typename impl::UnpackedType<T, N>::type;



/// \brief Pack non-negative N-bit integer value into bit medium of width N.
///
/// This function packs a non-negative N-bit integer value (\p val) into a bit medium of
/// width N (\p T). The nature of the packing operation is described in the documentation of
/// \ref image::is_bit_medium_of_width.
///
/// The specified value must be non-negative, and it must be strictly less than two to the
/// power of N. The result is unspecified if the value is outside this range. No undefined
/// behavior would be invoked, however.
///
/// \tparam T An integer type that is a bit medium of width N (see \ref
/// image::is_bit_medium_of_width).
///
/// \tparam U A type that conforms to the integer concept (\ref
/// Concept_Archon_Core_Integer).
///
template<class T, int N, class U> constexpr auto pack_int(U val) noexcept -> T;



/// \brief Unpack non-negative N-bit integer value from bit medium of width N.
///
/// If `packed_val` is a value in its packed form, `image::unpack_int<N>(packed_val)` is
/// shorthand for `image::unpack_int_a<N, U>(packed_val)` where `U` is
/// `image::unpacked_type<T, N>`.
///
/// \sa \ref image::unpack_int_a()
///
template<int N, class T> constexpr auto unpack_int(T packed_val) noexcept -> image::unpacked_type<T, N>;



/// \brief Unpack non-negative N-bit integer value from bit medium of width N into any type.
///
/// This function unpacks a non-negative N-bit integer value from the specified packed form
/// (\p packed_val) in a bit medium, \p T, of width N. The nature of the packing operation
/// is described in the documentation of \ref image::is_bit_medium_of_width.
///
/// The result is unspecified if the packed form (\p packed_form) is not one that could have
/// been produced by `image::pack_int<T, N>(val)` for some non-negative value `val` strictly
/// less than `2^N`. No undefined behavior would be invoked in this case, however.
///
/// \tparam U An integer type with at least N value bits (\ref core::num_value_bits()). The
/// type must conform to the integer concept (\ref Concept_Archon_Core_Integer).
///
/// \tparam T An integer type that is a bit medium of width N (see \ref
/// image::is_bit_medium_of_width).
///
template<int N, class U, class T> constexpr auto unpack_int_a(T packed_val) noexcept -> U;








// Implementation


template<class T, int N, class U> constexpr auto pack_int(U val) noexcept -> T
{
    static_assert(image::is_bit_medium_of_width<T, N>);
    if constexpr (N <= core::num_value_bits<T>()) {
        return core::int_cast_a<T>(val);
    }
    else {
        using unsigned_type = core::unsigned_type<core::promoted_type<T>>;
        unsigned_type val_2 = core::int_cast<unsigned_type>(val);
        unsigned_type val_3 = core::twos_compl_sign_extend(val_2, N);
        return core::cast_from_twos_compl_a<T>(val_3);
    }
}


template<int N, class T> constexpr auto unpack_int(T val) noexcept -> image::unpacked_type<T, N>
{
    using unpacked_type = image::unpacked_type<T, N>;
    return image::unpack_int_a<N, unpacked_type>(val);
}


template<int N, class U, class T> constexpr auto unpack_int_a(T val) noexcept -> U
{
    static_assert(image::is_bit_medium_of_width<T, N>);
    static_assert(core::num_value_bits<U>() >= N);
    if constexpr (core::num_value_bits<T>() >= N) {
        return core::int_cast_a<U>(val);
    }
    else {
        using unsigned_type = core::unsigned_type<core::promoted_type<T>>;
        unsigned_type val_2 = core::int_cast_a<unsigned_type>(val) & core::int_mask<unsigned_type>(N);
        return core::int_cast_a<U>(val_2);
    }
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_BIT_MEDIUM_HPP
