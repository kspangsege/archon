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

#ifndef ARCHON_X_IMAGE_X_INDEXED_TRAY_IMAGE_HPP
#define ARCHON_X_IMAGE_X_INDEXED_TRAY_IMAGE_HPP

/// \file


#include <archon/core/assert.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/block.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>
#include <archon/image/writable_image.hpp>


namespace archon::image {


/// \brief Tray-based image implementation using indirect color.
///
/// An image of this type uses a tray (\ref image::Tray) to refer to an array of pixels
/// stored elsewhere. Among other things, this means that the pixels do not have to be
/// stored contiguously in memory. The pixels are stored in the form of indexes into a
/// specified palette.
///
/// An image of this type has reference semantics as opposed to value semantics, meaning
/// that if a copy is made, only the reference to the pixels is copied. The copy will refer
/// to the same per-pixel memory locations as the original.
///
/// Images of this type do not support retrieval of buffer (\ref try_get_buffer()), because
/// memory is not necessarily contiguous.
///
/// \sa \ref image::WritableIndexedTrayImage
/// \sa \ref image::TrayImage
///
template<image::CompRepr R> class IndexedTrayImage
    : public image::Image {
public:
    static constexpr image::CompRepr comp_repr = R;

    using comp_type = image::comp_type<comp_repr>;
    using tray_type = image::Tray<const comp_type>;

    IndexedTrayImage(tray_type tray, const image::Image& palette) noexcept;

    IndexedTrayImage(const image::IndexBlock<R>& block, const image::Image& palette) noexcept;
    IndexedTrayImage(const image::IndexBlock<R>& block, const image::Box& subbox,
                     const image::Image& palette) noexcept;

    // Overriding virtual member functions of `image::Image`.
    auto get_size() const noexcept -> image::Size override final;
    bool try_get_buffer(image::BufferFormat&, const void*&) const override final;
    auto get_transfer_info() const -> TransferInfo override final;
    auto get_palette() const noexcept -> const Image* override final;
    void read(image::Pos, const image::Tray<void>&) const override final;

private:
    tray_type m_tray;
    const image::Image& m_palette;
};




/// \brief Tray-based writable image implementation using indirect color.
///
/// An image of this type uses a tray (\ref image::Tray) to refer to an array of pixels
/// stored elsewhere. Among other things, this means that the pixels do not have to be
/// stored contiguously in memory. The pixels are stored in the form of indexes into a
/// specified palette.
///
/// An image of this type has reference semantics as opposed to value semantics, meaning
/// that if a copy is made, only the reference to the pixels is copied. The copy will refer
/// to the same per-pixel memory locations as the original.
///
/// Images of this type do not support retrieval of buffer (\ref try_get_buffer()), because
/// memory is not necessarily contiguous.
///
/// \sa \ref image::IndexedTrayImage
/// \sa \ref image::WritableTrayImage
///
template<image::CompRepr R> class WritableIndexedTrayImage
    : public image::WritableImage {
public:
    static constexpr image::CompRepr comp_repr = R;

    using comp_type = image::comp_type<comp_repr>;
    using tray_type = image::Tray<comp_type>;

    WritableIndexedTrayImage(tray_type tray, const image::Image& palette) noexcept;

    WritableIndexedTrayImage(image::IndexBlock<R>& block, const image::Image& palette) noexcept;
    WritableIndexedTrayImage(image::IndexBlock<R>& block, const image::Box& subbox,
                             const image::Image& palette) noexcept;

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
    const image::Image& m_palette;
};








// Implementation


// ============================ IndexedTrayImage ============================


template<image::CompRepr R>
inline IndexedTrayImage<R>::IndexedTrayImage(tray_type tray, const image::Image& palette) noexcept
    : m_tray(tray)
    , m_palette(palette)
{
}


template<image::CompRepr R>
inline IndexedTrayImage<R>::IndexedTrayImage(const image::IndexBlock<R>& block, const image::Image& palette) noexcept
    : IndexedTrayImage(block, image::Box(block.size()), palette)
{
}


template<image::CompRepr R>
inline IndexedTrayImage<R>::IndexedTrayImage(const image::IndexBlock<R>& block, const image::Box& subbox,
                                             const image::Image& palette) noexcept
    : IndexedTrayImage(block.tray().subtray(subbox), palette)
{
}


template<image::CompRepr R>
auto IndexedTrayImage<R>::get_size() const noexcept -> image::Size
{
    return m_tray.size;
}


template<image::CompRepr R>
bool IndexedTrayImage<R>::try_get_buffer(image::BufferFormat&, const void*&) const
{
    return false; // Not supported, because memory may not be contiguous.
}


template<image::CompRepr R>
auto IndexedTrayImage<R>::get_transfer_info() const -> TransferInfo
{
    return m_palette.get_transfer_info(); // Throws
}


template<image::CompRepr R>
auto IndexedTrayImage<R>::get_palette() const noexcept -> const Image*
{
    return &m_palette;
}


template<image::CompRepr R>
void IndexedTrayImage<R>::read(image::Pos pos, const image::Tray<void>& tray) const
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(m_tray.size));
    int n = 1; // Just one component per pixel, i.e., the color index
    tray.cast_to<comp_type>().copy_from(m_tray.iter + (pos - image::Pos()), n); // Throws
}



// ============================ WritableIndexedTrayImage ============================


template<image::CompRepr R>
inline WritableIndexedTrayImage<R>::WritableIndexedTrayImage(tray_type tray, const image::Image& palette) noexcept
    : m_tray(tray)
    , m_palette(palette)
{
}


template<image::CompRepr R>
inline WritableIndexedTrayImage<R>::WritableIndexedTrayImage(image::IndexBlock<R>& block,
                                                             const image::Image& palette) noexcept
    : WritableIndexedTrayImage(block, image::Box(block.size()), palette)
{
}


template<image::CompRepr R>
inline WritableIndexedTrayImage<R>::WritableIndexedTrayImage(image::IndexBlock<R>& block,
                                                             const image::Box& subbox,
                                                             const image::Image& palette) noexcept
    : WritableIndexedTrayImage(block.tray().subtray(subbox), palette)
{
}


template<image::CompRepr R>
auto WritableIndexedTrayImage<R>::get_size() const noexcept -> image::Size
{
    return m_tray.size;
}


template<image::CompRepr R>
bool WritableIndexedTrayImage<R>::try_get_buffer(image::BufferFormat&, const void*&) const
{
    return false; // Not supported, because memory may not be contiguous.
}


template<image::CompRepr R>
auto WritableIndexedTrayImage<R>::get_transfer_info() const -> TransferInfo
{
    return m_palette.get_transfer_info(); // Throws
}


template<image::CompRepr R>
auto WritableIndexedTrayImage<R>::get_palette() const noexcept -> const Image*
{
    return &m_palette;
}


template<image::CompRepr R>
void WritableIndexedTrayImage<R>::read(image::Pos pos, const image::Tray<void>& tray) const
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(m_tray.size));
    int n = 1; // Just one component per pixel, i.e., the color index
    tray.cast_to<comp_type>().copy_from(m_tray.iter + (pos - image::Pos()), n); // Throws
}


template<image::CompRepr R>
bool WritableIndexedTrayImage<R>::try_get_writable_buffer(image::BufferFormat&, void*&)
{
    return false; // Not supported, because memory may not be contiguous.
}


template<image::CompRepr R>
void WritableIndexedTrayImage<R>::write(image::Pos pos, const image::Tray<const void>& tray)
{
    ARCHON_ASSERT(image::Box(pos, tray.size).contained_in(m_tray.size));
    int n = 1; // Just one component per pixel, i.e., the color index
    tray.cast_to<const comp_type>().copy_to(m_tray.iter + (pos - image::Pos()), n); // Throws
}


template<image::CompRepr R>
void WritableIndexedTrayImage<R>::fill(const image::Box& area, const void* color)
{
    ARCHON_ASSERT(area.contained_in(m_tray.size));
    int n = 1; // Just one component per pixel, i.e., the color index
    m_tray.subtray(area).fill(static_cast<const comp_type*>(color), n); // Throws
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_INDEXED_TRAY_IMAGE_HPP
