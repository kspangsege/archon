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

#ifndef ARCHON_X_IMAGE_X_PIXEL_CONVERT_HPP
#define ARCHON_X_IMAGE_X_PIXEL_CONVERT_HPP

/// \file


#include <type_traits>
#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>


namespace archon::image {


/// \brief Convert pixel between component representation schemes and alpha channel
/// presence.
///
/// This function converts a pixel from one component representation scheme (\p R) to
/// another (\p S), and also handles introduction or removal of the alpha channel
/// component. When an alpha channel component is eliminated, the result is as if the
/// specified color is blended with fully opaque black (color OVER black).
///
/// FIXME: Explain lossless operation and conditions for it (same component representation, no non-solid alpha removal)                        
///
template<image::CompRepr R, image::CompRepr S>
void pixel_convert(const image::comp_type<R>* origin, bool origin_has_alpha, image::comp_type<S>* destin,
                   bool destin_has_alpha, int num_color_space_channels) noexcept;



/// \brief Convert between pixel representations.
///
/// This function converts a pixel from one representation (\p R, \p origin_color_space, \p
/// origin_has_alpha) to another (\p S, \p destin_color_space, \p destin_has_alpha).
///
/// If color space conversion is needed, and a custom color space converter is specified (\p
/// custom_converter), that custom color space converter will be used.
///
/// If color space conversion is needed, and no custom color space converter is specified,
/// color space conversion falls back to conversion through RGB, which means that the pixel
/// is first converted to RGB, then to the destination color space.
///
/// It is an error if a custom converter is specified when the two color spaces are the
/// same, or when either one is RGB (\ref image::ColorSpace::is_rgb()). Doing so causes
/// undefined behavior. Note that a color space is identified by the memory address of the
/// color space object.
///
/// FIXME: Explain: `interm` must point to array whose size is no less than `max(origin_num_channels, destin_num_channels)` where `origin_num_channels` is `origin_color_space.get_num_channels() + int(origin_has_alpha)` and `destin_num_channels` is `destin_color_space.get_num_channels() + int(destin_has_alpha)`                    
///
/// FIXME: Explain lossless operation and conditions for it (same component representation, same color space, no non-solid alpha removal)                        
///
template<image::CompRepr R, image::CompRepr S>
void pixel_convert_a(const image::comp_type<R>* origin, const image::ColorSpace& origin_color_space,
                     bool origin_has_alpha, image::comp_type<S>* destin, const image::ColorSpace& destin_color_space,
                     bool destin_has_alpha, image::float_type* interm,
                     const image::ColorSpaceConverter* custom_converter);








// Implementation


namespace impl {


template<image::CompRepr R, image::CompRepr S>
void pixel_convert_remove_alpha(const image::comp_type<R>* origin, image::comp_type<S>* destin,
                                int num_color_space_channels) noexcept
{
    ARCHON_ASSERT(R != image::CompRepr::float_);
    // Blend with black (color OVER black)
    int n = num_color_space_channels;
    image::float_type alpha = image::alpha_comp_to_float<R>(origin[n]);
    for (int i = 0; i < n; ++i) {
        image::float_type comp = image::color_comp_to_float<R>(origin[i]);
        destin[i] = image::color_comp_from_float<S>(alpha * comp);
    }
}


} // namespace impl


template<image::CompRepr R, image::CompRepr S>
inline void pixel_convert(const image::comp_type<R>* origin, bool origin_has_alpha, image::comp_type<S>* destin,
                          bool destin_has_alpha, int num_color_space_channels) noexcept
{
    bool origin_is_float (R == image::CompRepr::float_);
    bool add_alpha = (!origin_has_alpha && destin_has_alpha);
    bool remove_alpha = (origin_has_alpha && !destin_has_alpha);
    auto is_solid = [&]() noexcept {
        ARCHON_ASSERT(origin_has_alpha);
        return (origin[num_color_space_channels] == image::comp_repr_max<R>());
    };
    bool short_circuit = (origin_is_float || !remove_alpha || is_solid());
    if (ARCHON_LIKELY(short_circuit)) {
        int num_channels = num_color_space_channels + int(origin_has_alpha);
        bool has_alpha = origin_has_alpha;
        if (remove_alpha) {
            num_channels = num_color_space_channels;
            has_alpha = false;
        }
        image::comp_repr_convert<R, S>(origin, destin, num_channels, has_alpha);
        if (add_alpha)
            destin[num_color_space_channels] = image::comp_repr_max<S>();
        return;
    }
    impl::pixel_convert_remove_alpha<R, S>(origin, destin, num_color_space_channels);
}


template<image::CompRepr R, image::CompRepr S>
inline void pixel_convert_a(const image::comp_type<R>* origin, const image::ColorSpace& origin_color_space,
                            bool origin_has_alpha, image::comp_type<S>* destin,
                            const image::ColorSpace& destin_color_space, bool destin_has_alpha,
                            image::float_type* interm, const image::ColorSpaceConverter* custom_converter)
{
    // When is conversion to float not needed?
    // - Both are integer types
    // - Same color space
    // - origin color is solid or destination format has alpha channel

    constexpr image::CompRepr origin_comp_repr = R;
    constexpr image::CompRepr destin_comp_repr = S;

    using origin_comp_type = image::comp_type<origin_comp_repr>;
    using destin_comp_type = image::comp_type<destin_comp_repr>;

    int origin_num_channels = origin_color_space.get_num_channels() + int(origin_has_alpha);
    int destin_num_channels = destin_color_space.get_num_channels() + int(destin_has_alpha);

    constexpr bool int_to_int = (!std::is_floating_point_v<origin_comp_type> &&
                                 !std::is_floating_point_v<destin_comp_type>);
    if constexpr (int_to_int) {
        bool same_color_space = (&origin_color_space == &destin_color_space);
        bool is_solid = (!origin_has_alpha || origin[origin_num_channels - 1] ==
                         image::comp_repr_max<origin_comp_repr>());
        if (ARCHON_LIKELY(same_color_space && (is_solid || destin_has_alpha))) {
            constexpr int origin_bit_width = image::comp_repr_bit_width<origin_comp_repr>();
            constexpr int destin_bit_width = image::comp_repr_bit_width<destin_comp_repr>();
            int n = std::min(origin_num_channels, destin_num_channels);
            for (int i = 0; i < n; ++i) {
                destin[i] = image::int_to_int<origin_bit_width, destin_comp_type,
                                              destin_bit_width>(origin[i]);
            }
            if (n < destin_num_channels) {
                destin_comp_type alpha = image::comp_repr_max<destin_comp_repr>();
                ARCHON_ASSERT(n == destin_num_channels - 1);
                destin[n] = alpha;
            }
            return;
        }
    }

    // Note the somewhat arbitrary choice of throwing away the alpha component after color
    // space conversion, rather than before. Fortunately, this makes no difference for
    // linear color space conversions, which most color space conversions are assumed to be.

    // Convert to floating point component representation scheme
    constexpr image::CompRepr float_comp_repr = image::CompRepr::float_;
    image::comp_repr_convert<origin_comp_repr, float_comp_repr>(origin, interm, origin_num_channels, origin_has_alpha);

    // Convert to destination color space
    {
        image::float_type alpha = 1.0;
        if (origin_has_alpha)
            alpha = interm[origin_num_channels - 1];
        image::color_space_convert(interm, alpha, origin_color_space, destin_color_space, custom_converter); // Throws
        if (destin_has_alpha)
            interm[destin_num_channels - 1] = alpha;
    }

    // Convert to destination component representation scheme
    image::comp_repr_convert<float_comp_repr, destin_comp_repr>(interm, destin, destin_num_channels, destin_has_alpha);
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_PIXEL_CONVERT_HPP
