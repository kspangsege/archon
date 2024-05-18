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

#ifndef ARCHON_X_IMAGE_X_COMPUTED_IMAGE_HPP
#define ARCHON_X_IMAGE_X_COMPUTED_IMAGE_HPP

/// \file


#include <algorithm>
#include <utility>

#include <archon/core/type_traits.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>


namespace archon::image {


/// \brief Image whose pixels are computed on demand.
///
/// This class offers an image imaplementation where pixels are computed on demand, rather
/// than being read from memory.
///
/// Here is an example of how it might be used:
///
/// \code{.cpp}
///
///   archon::image::ComputedImage image(image_size, [&](archon::image::Pos pos) {
///       archon::image::float_type val = noise(pos);
///       return archon::image::Pixel_Lum_F({ val });
///   });
///   archon::image::save(image, path, locale);
///
/// \endcode
///
/// FIXME: Find a way to support custom color spaces                   
///
template<class R, class F> class ComputedImage
    : public image::Image {
public:
    using pixel_repr_type = R;
    using func_type       = F;

    using pixel_type = image::Pixel<pixel_repr_type>;

    ComputedImage(image::Size size, func_type func);

    // Overriding virtual member functions of `image::Image`.
    auto get_size() const noexcept -> image::Size override final;
    bool try_get_buffer(image::BufferFormat&, const void*&) const override final;
    auto get_transfer_info() const -> TransferInfo override final;
    auto get_palette() const noexcept -> const Image* override final;
    void read(image::Pos, const image::Tray<void>&) const override final;

private:
    image::Size m_size;
    func_type m_func;
};


template<class F> ComputedImage(image::Size, F) -> ComputedImage<typename core::ReturnType<F>::repr_type, F>;








// Implementation


template<class R, class F>
inline ComputedImage<R, F>::ComputedImage(image::Size size, func_type func)
    : m_size(size)
    , m_func(std::move(func)) // Throws
{
}


template<class R, class F>
auto ComputedImage<R, F>::get_size() const noexcept -> image::Size
{
    return m_size;
}


template<class R, class F>
bool ComputedImage<R, F>::try_get_buffer(image::BufferFormat&, const void*&) const
{
    return false;
}


template<class R, class F>
auto ComputedImage<R, F>::get_transfer_info() const -> TransferInfo
{
    return {
        pixel_repr_type::comp_repr,
        &image::get_color_space(pixel_repr_type::color_space_tag),
        pixel_repr_type::has_alpha,
        image::comp_repr_bit_width<pixel_repr_type::comp_repr>(),
    };
}


template<class R, class F>
auto ComputedImage<R, F>::get_palette() const noexcept -> const Image*
{
    return nullptr;
}


template<class R, class F>
void ComputedImage<R, F>::read(image::Pos pos, const image::Tray<void>& tray) const
{
    using comp_type = typename pixel_repr_type::comp_type;
    image::Tray tray_2 = tray.cast_to<comp_type>();
    for (int x = 0; x < tray_2.size.width; ++x) {
        for (int y = 0; y < tray_2.size.height; ++y) {
            image::Pos pos_2 = pos + image::Size(x, y);
            pixel_type pixel = m_func(pos_2); // Throws
            const comp_type* origin = pixel.data();
            comp_type* destin = tray_2(x, y);
            std::copy(origin, origin + pixel_repr_type::num_channels, destin);
        }
    }
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_COMPUTED_IMAGE_HPP
