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

#ifndef ARCHON_X_IMAGE_X_BUFFERED_IMAGE_HPP
#define ARCHON_X_IMAGE_X_BUFFERED_IMAGE_HPP

/// \file


#include <cstddef>
#include <memory>
#include <utility>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/endianness.hpp>
#include <archon/image/size.hpp>
#include <archon/image/pos.hpp>
#include <archon/image/box.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/writable_image.hpp>
#include <archon/image/integer_pixel_format.hpp>


namespace archon::image {


/// \brief Buffered image with compile-time specified pixel storage.
///
/// This class defines an image that stores its pixels in a buffer. The scheme by which the
/// pixels are stored in the buffer is specified through \p F.
///
/// FIXME: Further explain \p F, and give examples of what it could be.                                                                   
///
template<class F> class BufferedImage
    : public image::WritableImage {
public:
    using format_type = F;

    using word_type       = typename format_type::word_type;
    using span_type       = core::Span<word_type>;
    using const_span_type = core::Span<const word_type>;

    /// \{
    ///
    /// \brief    
    ///
    /// If no image size is specified (\p image_size), a zero-size image will be
    /// constructed. If a size is specified, but not a buffer, a new buffer will be
    /// allocated and cleared (all words set to zero).
    ///
    /// Behaviour is undefined if a size is specified with a negative component.
    ///
    explicit BufferedImage(format_type = {});
    explicit BufferedImage(image::Size image_size, format_type = {});
    explicit BufferedImage(image::Size image_size, span_type buffer, format_type = {});
    explicit BufferedImage(image::Size image_size, std::unique_ptr<word_type[]> buffer, std::size_t buffer_size,
                           format_type = {});
    /// \}

    auto get_buffer() noexcept       -> span_type;
    auto get_buffer() const noexcept -> const_span_type;

    // Overriding functions in image::Image
    auto get_size() const noexcept -> image::Size override final;
    bool try_get_buffer(image::BufferFormat&, const void*&) const override final;
    auto get_transfer_info() const -> TransferInfo override final;
    auto get_palette() const noexcept -> const Image* override final;
    void read(image::Pos, const image::Tray<void>&) const override final;

    // Overriding functions in image::WritableImage
    bool try_get_writable_buffer(image::BufferFormat&, void*&) override final;
    void write(image::Pos, const image::Tray<const void>&) override final;
    void fill(const image::Box&, const void*) override final;

private:
    using transf_comp_type = image::comp_type<format_type::transf_repr>;

    struct State : format_type {
        image::Size size;
        std::unique_ptr<word_type[]> buffer_owner;
        span_type buffer;
        State(image::Size image_size, format_type&&);
        State(image::Size image_size, span_type buffer, format_type&&);
        State(image::Size image_size, std::unique_ptr<word_type[]> buffer, std::size_t buffer_size, format_type&&);
    };
    State m_state;

    auto format() const noexcept -> const format_type&;
};


template<class W = image::int8_type, int B = image::bit_width<W>, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big>
using BufferedImage_Lum = image::BufferedImage<image::IntegerPixelFormat_Lum<W, B, S, D, E>>;

template<class W = image::int8_type, int B = image::bit_width<W>, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big>
using BufferedImage_LumA = image::BufferedImage<image::IntegerPixelFormat_LumA<W, B, S, D, E>>;

template<class W = image::int8_type, int B = image::bit_width<W>, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big>
using BufferedImage_RGB = image::BufferedImage<image::IntegerPixelFormat_RGB<W, B, S, D, E>>;

template<class W = image::int8_type, int B = image::bit_width<W>, class S = W, int D = 1,
         core::Endianness E = core::Endianness::big>
using BufferedImage_RGBA = image::BufferedImage<image::IntegerPixelFormat_RGBA<W, B, S, D, E>>;


using BufferedImage_Lum_8  = image::BufferedImage_Lum<image::int8_type, 8>;
using BufferedImage_LumA_8 = image::BufferedImage_LumA<image::int8_type, 8>;
using BufferedImage_RGB_8  = image::BufferedImage_RGB<image::int8_type, 8>;
using BufferedImage_RGBA_8 = image::BufferedImage_RGBA<image::int8_type, 8>;

using BufferedImage_Lum_16  = image::BufferedImage_Lum<image::int16_type, 16>;
using BufferedImage_LumA_16 = image::BufferedImage_LumA<image::int16_type, 16>;
using BufferedImage_RGB_16  = image::BufferedImage_RGB<image::int16_type, 16>;
using BufferedImage_RGBA_16 = image::BufferedImage_RGBA<image::int16_type, 16>;

using BufferedImage_Lum_32  = image::BufferedImage_Lum<image::int32_type, 32>;
using BufferedImage_LumA_32 = image::BufferedImage_LumA<image::int32_type, 32>;
using BufferedImage_RGB_32  = image::BufferedImage_RGB<image::int32_type, 32>;
using BufferedImage_RGBA_32 = image::BufferedImage_RGBA<image::int32_type, 32>;








// Implementation


template<class F>
inline BufferedImage<F>::BufferedImage(format_type format)
    : m_state(image::Size(0), std::move(format)) // Throws
{
}


template<class F>
inline BufferedImage<F>::BufferedImage(image::Size image_size, format_type format)
    : m_state(image_size, std::move(format)) // Throws
{
}


template<class F>
inline BufferedImage<F>::BufferedImage(image::Size image_size, span_type buffer, format_type format)
    : m_state(image_size, buffer, std::move(format)) // Throws
{
}


template<class F>
inline BufferedImage<F>::BufferedImage(image::Size image_size, std::unique_ptr<word_type[]> buffer,
                                       std::size_t buffer_size, format_type format)
    : m_state(image_size, std::move(buffer), buffer_size, std::move(format)) // Throws
{
}


template<class F>
inline auto BufferedImage<F>::get_buffer() noexcept -> span_type
{
    return m_state.buffer;
}


template<class F>
inline auto BufferedImage<F>::get_buffer() const noexcept -> const_span_type
{
    return m_state.buffer;
}


template<class F>
inline auto BufferedImage<F>::get_size() const noexcept -> image::Size
{
    return m_state.size;
}


template<class F>
inline bool BufferedImage<F>::try_get_buffer(image::BufferFormat& format, const void*& buffer) const
{
    if (this->format().try_describe(format)) { // Throws
        buffer = m_state.buffer.data();
        return true;
    }
    return false;
}


template<class F>
inline auto BufferedImage<F>::get_transfer_info() const -> TransferInfo
{
    return format().get_transfer_info();
}


template<class F>
inline auto BufferedImage<F>::get_palette() const noexcept -> const Image*
{
    if constexpr (format_type::is_indexed_color) {
        return &format().get_palette();
    }
    else {
        return nullptr;
    }
}


template<class F>
void BufferedImage<F>::read(image::Pos pos, const image::Tray<void>& tray) const
{
    format().read(get_buffer().data(), m_state.size, pos, tray.cast_to<transf_comp_type>()); // Throws
}


template<class F>
inline bool BufferedImage<F>::try_get_writable_buffer(image::BufferFormat& format, void*& buffer)
{
    if (this->format().try_describe(format)) { // Throws
        buffer = m_state.buffer.data();
        return true;
    }
    return false;
}


template<class F>
void BufferedImage<F>::write(image::Pos pos, const image::Tray<const void>& tray)
{
    format().write(get_buffer().data(), m_state.size, pos, tray.cast_to<const transf_comp_type>()); // Throws
}


template<class F>
void BufferedImage<F>::fill(const image::Box& area, const void* color)
{
    format().fill(get_buffer().data(), m_state.size, area, static_cast<const transf_comp_type*>(color)); // Throws
}


template<class F>
inline BufferedImage<F>::State::State(image::Size image_size, format_type&& format)
    : format_type(std::move(format)) // Throws
    , size(image_size)
{
    std::size_t buffer_size = format_type::get_buffer_size(image_size); // Throws
    if (ARCHON_LIKELY(image_size > 0)) {
        // Buffere is cleared due to value initialization by std::make_unique().
        buffer_owner = std::make_unique<word_type[]>(buffer_size); // Throws
        buffer = { buffer_owner.get(), buffer_size }; // Throws
    }
}


template<class F>
inline BufferedImage<F>::State::State(image::Size image_size, span_type buffer, format_type&& format)
    : format_type(std::move(format)) // Throws
    , size(image_size)
{
    std::size_t buffer_size = format_type::get_buffer_size(image_size); // Throws
    if (ARCHON_LIKELY(buffer_size <= buffer.size())) {
        this->buffer = buffer;
        return;
    }
    throw std::invalid_argument("Buffer too small for image size");
}


template<class F>
inline BufferedImage<F>::State::State(image::Size image_size, std::unique_ptr<word_type[]> buffer,
                                      std::size_t buffer_size, format_type&& format)
    : State(image_size, { buffer.get(), buffer_size }, std::move(format)) // Throws
{
    buffer_owner = std::move(buffer);
}


template<class F>
inline auto BufferedImage<F>::format() const noexcept -> const format_type&
{
    return m_state;
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_BUFFERED_IMAGE_HPP
