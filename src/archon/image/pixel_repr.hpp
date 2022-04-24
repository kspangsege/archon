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

#ifndef ARCHON_X_IMAGE_X_PIXEL_REPR_HPP
#define ARCHON_X_IMAGE_X_PIXEL_REPR_HPP

/// \file


#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>


namespace archon::image {


/// \brief Compile-time specification of pixel representation scheme.
///
/// A type, that is an instance of this class template, is a compile-time specification of a
/// pixel representation scheme. Such a type can be used with \ref image::Pixel and \ref
/// image::PixelBlock.
///
template<image::ColorSpace::Tag C, bool A, image::CompRepr R> struct PixelRepr {
    static constexpr image::ColorSpace::Tag color_space_tag = C;
    static constexpr bool has_alpha = A;
    static constexpr image::CompRepr comp_repr = R;

    static constexpr int num_channels = image::get_num_channels(color_space_tag) + int(has_alpha);

    static auto get_color_space() noexcept -> const image::ColorSpace&;

    using comp_type       = image::comp_type<R>;
    using iter_type       = image::Iter<comp_type>;
    using const_iter_type = image::Iter<const comp_type>;
    using tray_type       = image::Tray<comp_type>;
    using const_tray_type = image::Tray<const comp_type>;

    /// \brief Promoted pixel representation scheme.
    ///
    /// This type specifies the promoted version of the containing pixel representation
    /// scheme. This promoted version is derived from the containing pixel representation by
    /// switching to the floating-point component representation scheme (see \ref
    /// image::CompRepr), and by making the alpha channel present (set \ref has_alpha to
    /// `true`). The color space (\ref color_space_tag) remains unchanged. Note that
    /// promotion is an idempotent operation, so `promoted_type::promoted_type` is the same
    /// type as `promoted_type`.
    ///
    using promoted_type = PixelRepr<color_space_tag, true, image::CompRepr::float_>;
};


using Alpha_8 = image::PixelRepr<image::ColorSpace::Tag::degen, true,  image::CompRepr::int8>;
using Lum_8   = image::PixelRepr<image::ColorSpace::Tag::lum,   false, image::CompRepr::int8>;
using LumA_8  = image::PixelRepr<image::ColorSpace::Tag::lum,   true,  image::CompRepr::int8>;
using RGB_8   = image::PixelRepr<image::ColorSpace::Tag::rgb,   false, image::CompRepr::int8>;
using RGBA_8  = image::PixelRepr<image::ColorSpace::Tag::rgb,   true,  image::CompRepr::int8>;

using Alpha_16 = image::PixelRepr<image::ColorSpace::Tag::degen, true,  image::CompRepr::int16>;
using Lum_16   = image::PixelRepr<image::ColorSpace::Tag::lum,   false, image::CompRepr::int16>;
using LumA_16  = image::PixelRepr<image::ColorSpace::Tag::lum,   true,  image::CompRepr::int16>;
using RGB_16   = image::PixelRepr<image::ColorSpace::Tag::rgb,   false, image::CompRepr::int16>;
using RGBA_16  = image::PixelRepr<image::ColorSpace::Tag::rgb,   true,  image::CompRepr::int16>;

using Alpha_F = image::PixelRepr<image::ColorSpace::Tag::degen, true,  image::CompRepr::float_>;
using Lum_F   = image::PixelRepr<image::ColorSpace::Tag::lum,   false, image::CompRepr::float_>;
using LumA_F  = image::PixelRepr<image::ColorSpace::Tag::lum,   true,  image::CompRepr::float_>;
using RGB_F   = image::PixelRepr<image::ColorSpace::Tag::rgb,   false, image::CompRepr::float_>;
using RGBA_F  = image::PixelRepr<image::ColorSpace::Tag::rgb,   true,  image::CompRepr::float_>;








// Implementation


template<image::ColorSpace::Tag C, bool A, image::CompRepr R>
inline auto PixelRepr<C, A, R>::get_color_space() noexcept -> const image::ColorSpace&
{
    return image::get_color_space(color_space_tag);
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_PIXEL_REPR_HPP
