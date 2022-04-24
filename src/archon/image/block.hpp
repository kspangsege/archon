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

#ifndef ARCHON_X_IMAGE_X_BLOCK_HPP
#define ARCHON_X_IMAGE_X_BLOCK_HPP

/// \file


#include <cstddef>
#include <utility>
#include <algorithm>
#include <memory>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/integer.hpp>
#include <archon/image/size.hpp>
#include <archon/image/pos.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/pixel_repr.hpp>
#include <archon/image/pixel.hpp>


namespace archon::image {


/// \brief Two-dimensional array of pixels or pixel-like items.
///
/// A block is a two-dimensional array of items (usually pixels), with each item having \p N
/// channel components. All the channels in a block are stored contiguously in memory. Each
/// set of N components make up one item of the array, and items occur in row-major order.
///
/// \sa \ref image::PixelBlock, \ref image::IndexBlock
///
template<image::CompRepr R, int N> class Block {
public:
    static constexpr image::CompRepr comp_repr = R;
    static constexpr int num_channels = N;

    using comp_type       = image::comp_type<comp_repr>;
    using span_type       = core::Span<comp_type>;
    using const_span_type = core::Span<const comp_type>;

    explicit Block() noexcept = default;
    explicit Block(image::Size size);
    explicit Block(image::Size size, span_type buffer);
    explicit Block(image::Size size, std::unique_ptr<comp_type[]> buffer, std::size_t buffer_size);

    auto size() const noexcept -> image::Size;

    auto buffer() noexcept       -> span_type;
    auto buffer() const noexcept -> const_span_type;

    using tray_type       = image::Tray<comp_type>;
    using const_tray_type = image::Tray<const comp_type>;

    auto tray() noexcept       -> tray_type;
    auto tray() const noexcept -> const_tray_type;

private:
    std::unique_ptr<comp_type[]> m_buffer_owner;
    image::Size m_size;
    span_type m_buffer;

    static void verify_buffer_size(std::size_t buffer_size, image::Size block_size);

    // Determine buffer size for block of specified size and ensure that it is also
    // representable in `std::ptrdiff_t`. This ensures no overflow in make_tray().
    static auto determine_buffer_size(image::Size block_size) -> std::size_t;

    auto make_tray() const noexcept -> tray_type;
};




/// \brief Two-dimensional array of pixels.
///
/// A pixel block is a two-dimensional array of pixels. The pixels are represented according
/// to the specified pixel representation scheme (\p R).
///
/// The specified pixel representation scheme (\p R) must be an instance of \ref
/// image::PixelRepr.
///
template<class R> class PixelBlock
    : public image::Block<R::comp_repr, R::num_channels> {
public:
    using pixel_repr_type = R;

    using comp_type = image::comp_type<R::comp_repr>;

    using image::Block<R::comp_repr, R::num_channels>::Block;

    using pixel_type = image::Pixel<pixel_repr_type>;

    auto get_pixel(image::Pos pos) const noexcept -> pixel_type;
};


using PixelBlock_Alpha_8 = image::PixelBlock<image::Alpha_8>;
using PixelBlock_Lum_8   = image::PixelBlock<image::Lum_8>;
using PixelBlock_LumA_8  = image::PixelBlock<image::LumA_8>;
using PixelBlock_RGB_8   = image::PixelBlock<image::RGB_8>;
using PixelBlock_RGBA_8  = image::PixelBlock<image::RGBA_8>;

using PixelBlock_Alpha_16 = image::PixelBlock<image::Alpha_16>;
using PixelBlock_Lum_16   = image::PixelBlock<image::Lum_16>;
using PixelBlock_LumA_16  = image::PixelBlock<image::LumA_16>;
using PixelBlock_RGB_16   = image::PixelBlock<image::RGB_16>;
using PixelBlock_RGBA_16  = image::PixelBlock<image::RGBA_16>;

using PixelBlock_Alpha_F = image::PixelBlock<image::Alpha_F>;
using PixelBlock_Lum_F   = image::PixelBlock<image::Lum_F>;
using PixelBlock_LumA_F  = image::PixelBlock<image::LumA_F>;
using PixelBlock_RGB_F   = image::PixelBlock<image::RGB_F>;
using PixelBlock_RGBA_F  = image::PixelBlock<image::RGBA_F>;




/// \brief Two-dimensional array of color indexes.
///
/// An index block is a two-dimensional array of color indexes, with each index represented
/// according to the specified component representation scheme (\p R).
///
template<image::CompRepr R> class IndexBlock
    : public image::Block<R, 1> {
public:
    using comp_type = image::comp_type<R>;
    static_assert(std::is_integral_v<comp_type>);

    static constexpr int bit_width = image::comp_repr_int_bit_width(R);

    using image::Block<R, 1>::Block;

    auto get_index(image::Pos pos) const noexcept -> std::size_t;
};


using IndexBlock_8  = image::IndexBlock<image::CompRepr::int8>;
using IndexBlock_16 = image::IndexBlock<image::CompRepr::int16>;








// Implementation


template<image::CompRepr R, int N>
inline Block<R, N>::Block(image::Size size)
{
    std::size_t buffer_size = determine_buffer_size(size); // Throws
    m_size = size;
    if (ARCHON_LIKELY(buffer_size > 0))
        m_buffer_owner = std::make_unique<comp_type[]>(buffer_size); // Throws
    m_buffer = { m_buffer_owner.get(), buffer_size };
}


template<image::CompRepr R, int N>
inline Block<R, N>::Block(image::Size size, span_type buffer)
{
    verify_buffer_size(buffer.size(), size); // Throws
    m_size = size;
    m_buffer = buffer;
}


template<image::CompRepr R, int N>
inline Block<R, N>::Block(image::Size size, std::unique_ptr<comp_type[]> buffer, std::size_t buffer_size)
{
    verify_buffer_size(buffer_size, size); // Throws
    m_size = size;
    m_buffer_owner = std::move(buffer);
    m_buffer = { m_buffer_owner.get(), buffer_size };
}


template<image::CompRepr R, int N>
inline auto Block<R, N>::size() const noexcept -> image::Size
{
    return m_size;
}


template<image::CompRepr R, int N>
inline auto Block<R, N>::buffer() noexcept -> span_type
{
    return m_buffer;
}


template<image::CompRepr R, int N>
inline auto Block<R, N>::buffer() const noexcept -> const_span_type
{
    return m_buffer;
}


template<image::CompRepr R, int N>
inline auto Block<R, N>::tray() noexcept -> tray_type
{
    return make_tray();
}


template<image::CompRepr R, int N>
inline auto Block<R, N>::tray() const noexcept -> const_tray_type
{
    return make_tray();
}


template<image::CompRepr R, int N>
void Block<R, N>::verify_buffer_size(std::size_t buffer_size, image::Size block_size)
{
    std::size_t min_buffer_size = determine_buffer_size(block_size); // Throws
    if (ARCHON_LIKELY(buffer_size >= min_buffer_size))
        return;
    throw std::invalid_argument("Buffer too small for block size");
}


template<image::CompRepr R, int N>
inline auto Block<R, N>::determine_buffer_size(image::Size block_size) -> std::size_t
{
    std::ptrdiff_t size = 1;
    core::int_mul(size, num_channels); // Throws
    core::int_mul(size, block_size.width); // Throws
    core::int_mul(size, block_size.height); // Throws
    return core::int_cast<std::size_t>(size); // Throws
}


template<image::CompRepr R, int N>
inline auto Block<R, N>::make_tray() const noexcept -> tray_type
{
    // No overflow possible due to checks in determine_buffer_size()
    std::ptrdiff_t horz_stride = std::ptrdiff_t(num_channels);
    std::ptrdiff_t vert_stride = std::ptrdiff_t(m_size.width * horz_stride);
    image::Iter<comp_type> iter = { m_buffer.data(), horz_stride, vert_stride };
    return { iter, m_size };
}


template<class R>
inline auto PixelBlock<R>::get_pixel(image::Pos pos) const noexcept -> pixel_type
{
    const comp_type* pixel_1 = this->tray()(pos);
    pixel_type pixel_2;
    std::copy_n(pixel_1, pixel_repr_type::num_channels, pixel_2.data());
    return pixel_2;
}


template<image::CompRepr R>
inline auto IndexBlock<R>::get_index(image::Pos pos) const noexcept -> std::size_t
{
    const comp_type* pixel = this->tray()(pos);
    return std::size_t(image::comp_repr_unpack<R>(pixel[0]));
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_BLOCK_HPP
