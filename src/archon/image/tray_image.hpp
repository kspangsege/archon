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

#ifndef ARCHON_X_IMAGE_X_TRAY_IMAGE_HPP
#define ARCHON_X_IMAGE_X_TRAY_IMAGE_HPP

/// \file


#include <archon/core/assert.hpp>
#include <archon/image/size.hpp>
#include <archon/image/pos.hpp>
#include <archon/image/box.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/block.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>
#include <archon/image/writable_image.hpp>


namespace archon::image {


/// \brief Tray-based image implementation using direct color.
///
/// An image of this type uses a tray (\ref image::Tray) to refer to an array of pixels
/// stored elsewhere. Among other things, this means that the pixels do not have to be
/// stored contiguously in memory.
///
/// An image of this type has reference semantics as opposed to value semantics, meaning
/// that if a copy is made, only the reference to the pixels is copied. The copy will refer
/// to the same per-pixel memory locations as the original.
///
/// Images of this type do not support retrieval of buffer (\ref try_get_buffer()), because
/// memory is not necessarily contiguous.
///
/// \sa \ref image::WritableTrayImage
/// \sa \ref image::IndexedTrayImage
///
template<image::CompRepr R> class TrayImage
    : public image::Image {
public:
    static constexpr image::CompRepr comp_repr = R;

    using comp_type = image::comp_type<comp_repr>;
    using tray_type = image::Tray<const comp_type>;

    TrayImage(tray_type tray, const image::ColorSpace& color_space, bool has_alpha) noexcept;

    template<class S> TrayImage(const image::PixelBlock<S>& block) noexcept;
    template<class S> TrayImage(const image::PixelBlock<S>& block, const image::Box& subbox) noexcept;

    // Overriding virtual member functions of `image::Image`.
    auto get_size() const noexcept -> image::Size override final;
    bool try_get_buffer(image::BufferFormat&, const void*&) const override final;
    auto get_transfer_info() const -> TransferInfo override final;
    auto get_palette() const noexcept -> const Image* override final;
    void read(image::Pos, const image::Tray<void>&) const override final;

private:
    tray_type m_tray;
    const image::ColorSpace* m_color_space;
    bool m_has_alpha;
};


template<class S> TrayImage(const image::PixelBlock<S>&) -> TrayImage<S::comp_repr>;
template<class S> TrayImage(const image::PixelBlock<S>&, const image::Box&) -> TrayImage<S::comp_repr>;




/// \brief Tray-based writable image implementation using direct color.
///
/// An image of this type uses a tray (\ref image::Tray) to refer to an array of pixels
/// stored elsewhere. Among other things, this means that the pixels do not have to be
/// stored contiguously in memory.
///
/// An image of this type has reference semantics as opposed to value semantics, meaning
/// that if a copy is made, only the reference to the pixels is copied. The copy will refer
/// to the same per-pixel memory locations as the original.
///
/// Images of this type do not support retrieval of buffer (\ref try_get_buffer()), because
/// memory is not necessarily contiguous.
///
/// \sa \ref image::TrayImage
/// \sa \ref image::WritableIndexedTrayImage
///
template<image::CompRepr R> class WritableTrayImage
    : public image::WritableImage {
public:
    static constexpr image::CompRepr comp_repr = R;

    using comp_type = image::comp_type<comp_repr>;
    using tray_type = image::Tray<comp_type>;

    WritableTrayImage(tray_type tray, const image::ColorSpace& color_space, bool has_alpha) noexcept;

    template<class S> WritableTrayImage(image::PixelBlock<S>& block) noexcept;
    template<class S> WritableTrayImage(image::PixelBlock<S>& block, const image::Box& subbox) noexcept;

    // Overriding virtual member functions of `image::Image`.
    auto get_size() const noexcept -> image::Size override final;
    bool try_get_buffer(image::BufferFormat&, const void*&) const override final;
    auto get_transfer_info() const -> TransferInfo override final;
    auto get_palette() const noexcept -> const Image* override final;
    void read(image::Pos, const image::Tray<void>&) const override final;

    // Overriding virtual member functions of `image::WritableImage`.
    bool try_get_writable_buffer(image::BufferFormat&, void*&) override final;
    void write(image::Pos, const image::Tray<const void>&) override final;
    void fill(const image::Box&, const void*) override final;

private:
    tray_type m_tray;
    const image::ColorSpace* m_color_space;
    bool m_has_alpha;
};


template<class S> WritableTrayImage(image::PixelBlock<S>&) -> WritableTrayImage<S::comp_repr>;
template<class S> WritableTrayImage(image::PixelBlock<S>&, const image::Box&) -> WritableTrayImage<S::comp_repr>;








// Implementation


// ============================ TrayImage ============================


template<image::CompRepr R>
inline TrayImage<R>::TrayImage(tray_type tray, const image::ColorSpace& color_space, bool has_alpha) noexcept
    : m_tray(tray)
    , m_color_space(&color_space)
    , m_has_alpha(has_alpha)
{
}


template<image::CompRepr R>
template<class S> inline TrayImage<R>::TrayImage(const image::PixelBlock<S>& block) noexcept
    : TrayImage(block, image::Box(block.size()))
{
}


template<image::CompRepr R>
template<class S> inline TrayImage<R>::TrayImage(const image::PixelBlock<S>& block, const image::Box& subbox) noexcept
    : TrayImage(block.tray().subtray(subbox), S::get_color_space(), S::has_alpha)
{
}


template<image::CompRepr R>
auto TrayImage<R>::get_size() const noexcept -> image::Size
{
    return m_tray.size;
}


template<image::CompRepr R>
bool TrayImage<R>::try_get_buffer(image::BufferFormat&, const void*&) const
{
    return false; // Not supported, because memory may not be contiguous.
}


template<image::CompRepr R>
auto TrayImage<R>::get_transfer_info() const -> TransferInfo
{
    return {
        comp_repr,
        m_color_space,
        m_has_alpha,
        image::comp_repr_bit_width<comp_repr>(),
    };
}


template<image::CompRepr R>
auto TrayImage<R>::get_palette() const noexcept -> const Image*
{
    return nullptr;
}


template<image::CompRepr R>
void TrayImage<R>::read(image::Pos pos, const image::Tray<void>& tray) const
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(m_tray.size));
    int n = m_color_space->get_num_channels() + int(m_has_alpha);
    tray.cast_to<comp_type>().copy_from(m_tray.iter + (pos - image::Pos()), n); // Throws
}



// ============================ WritableTrayImage ============================


template<image::CompRepr R>
inline WritableTrayImage<R>::WritableTrayImage(tray_type tray, const image::ColorSpace& color_space,
                                               bool has_alpha) noexcept
    : m_tray(tray)
    , m_color_space(&color_space)
    , m_has_alpha(has_alpha)
{
}


template<image::CompRepr R>
template<class S> inline WritableTrayImage<R>::WritableTrayImage(image::PixelBlock<S>& block) noexcept
    : WritableTrayImage(block, image::Box(block.size()))
{
}


template<image::CompRepr R>
template<class S> inline WritableTrayImage<R>::WritableTrayImage(image::PixelBlock<S>& block,
                                                                 const image::Box& subbox) noexcept
    : WritableTrayImage(block.tray().subtray(subbox), S::get_color_space(), S::has_alpha)
{
}


template<image::CompRepr R>
auto WritableTrayImage<R>::get_size() const noexcept -> image::Size
{
    return m_tray.size;
}


template<image::CompRepr R>
bool WritableTrayImage<R>::try_get_buffer(image::BufferFormat&, const void*&) const
{
    return false; // Not supported, because memory may not be contiguous.
}


template<image::CompRepr R>
auto WritableTrayImage<R>::get_transfer_info() const -> TransferInfo
{
    return {
        comp_repr,
        m_color_space,
        m_has_alpha,
        image::comp_repr_bit_width<comp_repr>(),
    };
}


template<image::CompRepr R>
auto WritableTrayImage<R>::get_palette() const noexcept -> const Image*
{
    return nullptr;
}


template<image::CompRepr R>
void WritableTrayImage<R>::read(image::Pos pos, const image::Tray<void>& tray) const
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(m_tray.size));
    int n = m_color_space->get_num_channels() + int(m_has_alpha);
    tray.cast_to<comp_type>().copy_from(m_tray.iter + (pos - image::Pos()), n); // Throws
}


template<image::CompRepr R>
bool WritableTrayImage<R>::try_get_writable_buffer(image::BufferFormat&, void*&)
{
    return false; // Not supported, because memory may not be contiguous.
}


template<image::CompRepr R>
void WritableTrayImage<R>::write(image::Pos pos, const image::Tray<const void>& tray)
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(m_tray.size));
    int n = m_color_space->get_num_channels() + int(m_has_alpha);
    tray.cast_to<const comp_type>().copy_to(m_tray.iter + (pos - image::Pos()), n); // Throws
}


template<image::CompRepr R>
void WritableTrayImage<R>::fill(const image::Box& area, const void* color)
{
    ARCHON_ASSERT(area.contained_in(m_tray.size));
    int n = m_color_space->get_num_channels() + int(m_has_alpha);
    m_tray.subtray(area).fill(static_cast<const comp_type*>(color), n); // Throws
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_TRAY_IMAGE_HPP
