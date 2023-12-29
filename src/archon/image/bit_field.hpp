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

#ifndef ARCHON_X_IMAGE_X_BIT_FIELD_HPP
#define ARCHON_X_IMAGE_X_BIT_FIELD_HPP

/// \file


#include <utility>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/image/comp_types.hpp>


namespace archon::image {


/// \brief Description of bit field.
///
/// Objects of this type specify the location and width of a bit field within some integer
/// word type. The position is specified relative to the subsequent field. See \ref gap for
/// more.
///
/// In an array of bit fields, the first field in the array is understood as occupying
/// higher order bits than the last field in that array.
///
struct BitField {
    /// \brief Number of bits in field.
    ///
    /// This is the number of consecutive bit positions that are part of this bit fields.
    ///
    int width;

    /// \brief Gap between this field and next one.
    ///
    /// This is the number of unused bit positions that follow this bit field when bit
    /// positions are understood as running from highest to lowest significance.
    ///
    /// If this is the last bit field in a word, and the gap is zero, then it means that the
    /// last bit position in this field is the last bit position in the word, i.e., the one
    /// representing the value 1. And, if the gap is not zero, it means that the last bit
    /// position in the gap, that follows this field, is the last bit position in the word.
    ///
    int gap;
};


/// \brief Verify validity of sequence of bit fields.
///
/// This function verifies the validity of the specified sequence of bit fields, including
/// that they fit within the specified number of available bits.
///
/// Validity requires that all widths are greater than, or equal to 1, all gaps are greater
/// than, or equal to zero, and the sum of all widths and gaps is less than, or equal to \p
/// num_available_bits.
///
constexpr bool valid_bit_fields(const image::BitField*, int num_fields, int num_available_bits) noexcept;


/// \brief Width of bit field.
///
/// This function returns the width of the specified bit field (\p field_index). The bit
/// field is specified in terms of its index within the specified list of fields (\p
/// fields).
///
constexpr int get_bit_field_width(const image::BitField* fields, int num_fields, int field_index) noexcept;


/// \brief Shift associated with bit field.
///
/// This function returns the left-shift associated width the specified bit field (\p
/// field_index). The bit field is specified in terms of its index within the specified list
/// of fields (\p fields).
///
constexpr int get_bit_field_shift(const image::BitField* fields, int num_fields, int field_index) noexcept;


/// \brief Mask corresponding to bit field.
///
/// This function returns the bit mask corresponding to the specified bit field (\p
/// field_index). The bit field is specified in terms of its index within the specified list
/// of fields (\p fields). The mask will be packed into the specified bit medium (\p T)
/// which must be wide enough to hold the mask in packed form (see \ref
/// image::is_bit_medium_of_width). The type is wide enough if `image::bit_width<T>` is
/// greater than, or equal to the width of the field plus the left-shift of the field. See
/// \ref image::bit_width.
///
template<class T> constexpr auto get_bit_field_mask(const image::BitField* fields, int num_fields,
                                                    int field_index) noexcept -> T;


/// \brief Width of widest bit field.
///
/// This function returns the width of the widest of the specified bit fields (\p fields, \p
/// num_fields). If the number of specified fields is zero, this function returns zero.
///
constexpr int widest_bit_field(const image::BitField* fields, int num_fields) noexcept;








// Implementation


constexpr bool valid_bit_fields(const image::BitField* fields, int num_fields, int num_available_bits) noexcept
{
    int total_width = 0;
    for (int i = 0; i < num_fields; ++i) {
        const image::BitField& field = fields[i];
        if (ARCHON_UNLIKELY(field.width < 1 || field.gap < 0))
            return false;
        if (ARCHON_UNLIKELY(!core::try_int_add(total_width, field.width) ||
                            !core::try_int_add(total_width, field.gap)))
            return false;
    }
    return (total_width <= num_available_bits);
}


constexpr int get_bit_field_width(const image::BitField* fields, int num_fields, int field_index) noexcept
{
    ARCHON_ASSERT(field_index < num_fields);
    image::BitField field = fields[field_index];
    return field.width;
}


constexpr int get_bit_field_shift(const image::BitField* fields, int num_fields, int field_index) noexcept
{
    ARCHON_ASSERT(field_index < num_fields);
    int offset = 0;
    for (int i = field_index + 1; i < num_fields; ++i) {
        image::BitField field = fields[i];
        offset += field.width;
        offset += field.gap;
    }
    image::BitField field = fields[field_index];
    return offset + field.gap;
}


template<class T> constexpr auto get_bit_field_mask(const image::BitField* fields, int num_fields,
                                                    int field_index) noexcept -> T
{
    using compound_type = T;
    int width = image::get_bit_field_width(fields, num_fields, field_index);
    int shift = image::get_bit_field_shift(fields, num_fields, field_index);
    constexpr int n = image::bit_width<compound_type>;
    ARCHON_ASSERT(width <= n && shift <= n - width);
    using unpacked_type = image::unpacked_type<compound_type, n>;
    return image::pack_int<compound_type, n>(core::int_mask<unpacked_type>(width) << shift);
}


constexpr int widest_bit_field(const image::BitField* fields, int num_fields) noexcept
{
    int width = 0;
    for (int i = 0; i < num_fields; ++i) {
        const image::BitField& field = fields[i];
        if (ARCHON_LIKELY(field.width <= width))
            continue;
        width = field.width;
    }
    return width;
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_BIT_FIELD_HPP
