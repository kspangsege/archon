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

#ifndef ARCHON_X_IMAGE_X_PIXEL_HPP
#define ARCHON_X_IMAGE_X_PIXEL_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <array>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/with_modified_locale.hpp>
#include <archon/util/color.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/blend.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/pixel_convert.hpp>
#include <archon/image/pixel_repr.hpp>


namespace archon::image {


/// \brief Collection of channel components making up one pixel.
///
/// An instance of this class is an ordered sequence of channel components making up one
/// complete pixel. The component representation scheme, color space, and presence of an
/// alpha channel is specified through the pixel representation scheme (\p R).
///
/// A default constructed pixel has all channel components set to zero.
///
/// The specified pixel representation scheme (\p R) must be an instance of \ref
/// image::PixelRepr.
///
/// Two instantiations of this template are *similar pixel types* if, and only if their
/// pixel representation schemes specify the same color space (\ref
/// image::PixelRepr::color_space_tag). Therefore, they can be similar when they use
/// different component representation schemes, and when one has an alpha channel, but the
/// other does not.
///
/// Pixels of this type are comparable. Comparison is lexicographical in terms of channel
/// components.
///
/// Pixels of this type can be formatted (written to an output stream), and can be parsed
/// through a value parser (\ref core::BasicValueParserSource). The textual representation
/// of a pixel with 3 channel components, `a`, `b`, and `c` is `[a, b, c]`. Each component
/// is either an integer or a floating-point number formatted according to the classic
/// locale (std::locale::classic()). When parsing, additional white-space is allowed before
/// and after each component.
///
/// \sa \ref image::operator*(image::float_type, const image::Pixel<R>&)
/// \sa \ref image::operator+(const image::Pixel<R>, const image::Pixel<S>&)
///
template<class R> class Pixel {
public:
    using repr_type = R;

    using comp_type = typename repr_type::comp_type;

    static constexpr image::ColorSpace::Tag color_space_tag = repr_type::color_space_tag;
    static constexpr bool has_alpha = repr_type::has_alpha;
    static constexpr int num_channels = repr_type::num_channels;
    static constexpr image::CompRepr comp_repr = repr_type::comp_repr;
    static constexpr bool is_rgba_8 = std::is_same_v<R, image::RGBA_8>;
    static auto get_color_space() noexcept -> const image::ColorSpace&;

    Pixel() noexcept = default;
    Pixel(std::array<comp_type, num_channels> components) noexcept;

    /// \brief Construct pixel from other pixel of similar type.
    ///
    /// This constructor constructs a pixel from another pixel (\p other) of similar pixel
    /// type (same color space). Specifying a pixel of a dissimilar pixel type is a
    /// compile-time error.
    ///
    /// FIXME: Explain lossless conversion and conditions for it (same component representation, no non-solid alpha removal)                        
    ///
    /// \sa \ref convert()
    ///
    template<class S> explicit Pixel(const Pixel<S>& other) noexcept;

    explicit Pixel(util::Color);
    explicit operator util::Color() const;

    auto operator[](std::size_t) noexcept       -> comp_type&;
    auto operator[](std::size_t) const noexcept -> const comp_type&;

    auto data() noexcept       -> comp_type*;
    auto data() const noexcept -> const comp_type*;

    constexpr auto size() const noexcept -> std::size_t;

    /// \{
    ///
    /// \brief Compare two pixels.
    ///
    /// These operators compare this pixel with the specified pixel (\p other). Comparison
    /// happens lexicographically in terms of components.
    ///
    bool operator==(const Pixel& other) const noexcept;
    bool operator!=(const Pixel& other) const noexcept;
    bool operator< (const Pixel& other) const noexcept;
    bool operator<=(const Pixel& other) const noexcept;
    bool operator> (const Pixel& other) const noexcept;
    bool operator>=(const Pixel& other) const noexcept;
    /// \}

    /// \brief Get implicit or explicit alpha component of this pixel.
    ///
    /// If this pixel has an alpha component, this function returns that component,
    /// otherwise it returns `image::comp_repr_max<R>()` (\ref image::comp_repr_max())
    /// corresponding to an implicit alpha component at maximum value (maximum opacity).
    ///
    auto opacity() const noexcept -> comp_type;

    /// \brief Get canonicalized version of pixel.
    ///
    /// This function returns a canonicalized version of this pixel. Canonicalization is the
    /// process of setting all channel values to zero if the alpha component is zero. If the
    /// alpha component is non-zero, or the pixel has no alpha component, canonicalization
    /// does not change the pixel.
    ///
    auto canonicalize() const noexcept -> Pixel;

    /// \brief Convert pixel to different representation scheme.
    ///
    /// This function converts this pixel to the specified pixel representation scheme (\p
    /// S). The specified pixel representation scheme must be an instance of \ref
    /// image::PixelRepr.
    ///
    /// The actual conversion is performed by \ref image::pixel_convert_a().
    ///
    /// FIXME: Explain lossless operation and conditions for it (same component representation, same color space, no non-solid alpha removal)                        
    ///
    template<class S> auto convert(const image::ColorSpaceConverter* custom_converter = nullptr) const -> Pixel<S>;

    /// \brief Get result of blending this pixel with specified pixel.
    ///
    /// This function computes the result of blending this pixel with the specified pixel
    /// (\p pixel) using the specified blend mode (\p mode). The specified opacity (\p
    /// opacity) will be used to modulate the opacity of this pixel.
    ///
    /// The actual blend operation is performed by \ref image::blend() with this pixel
    /// passed as the first pixel argument, and the specified pixel passed as the second
    /// pixel argument.
    ///
    auto blend(const Pixel& pixel, image::BlendMode mode, image::float_type opacity = 1) noexcept -> Pixel;

    /// \brief Promoted pixel type.
    ///
    /// This is the type of the promoted version of this pixel type (see \ref promote()).
    ///
    using promoted_pixel_type = Pixel<typename repr_type::promoted_type>;

    /// \brief Return promoted version of pixel.
    ///
    /// This function returns a promoted version of this pixel. The promoted version always
    /// uses the floating-point component representation scheme (\ref image::CompRepr) and
    /// it always has an alpha channel component. The promoted type and the type of this
    /// pixel are similar pixel types (same color space).
    ///
    auto promote() const noexcept -> promoted_pixel_type;

private:
    std::array<comp_type, num_channels> m_components = {};
};


Pixel(util::Color) -> Pixel<image::RGBA_8>;


using Pixel_Alpha_8 = image::Pixel<image::Alpha_8>;
using Pixel_Lum_8   = image::Pixel<image::Lum_8>;
using Pixel_LumA_8  = image::Pixel<image::LumA_8>;
using Pixel_RGB_8   = image::Pixel<image::RGB_8>;
using Pixel_RGBA_8  = image::Pixel<image::RGBA_8>;

using Pixel_Alpha_16 = image::Pixel<image::Alpha_16>;
using Pixel_Lum_16   = image::Pixel<image::Lum_16>;
using Pixel_LumA_16  = image::Pixel<image::LumA_16>;
using Pixel_RGB_16   = image::Pixel<image::RGB_16>;
using Pixel_RGBA_16  = image::Pixel<image::RGBA_16>;

using Pixel_Alpha_F = image::Pixel<image::Alpha_F>;
using Pixel_Lum_F   = image::Pixel<image::Lum_F>;
using Pixel_LumA_F  = image::Pixel<image::LumA_F>;
using Pixel_RGB_F   = image::Pixel<image::RGB_F>;
using Pixel_RGBA_F  = image::Pixel<image::RGBA_F>;



/// \brief Perform OVER operation.
///
/// This operator performs the OVER operation (see \ref image::BlendMode)). The result is
/// returned in promoted form. If `a` and `b` are pixels of similar type (same color space),
/// `a + b` is shorthand for `a.promote().blend(b.promote(),
/// image::BlendMode::over)`. Specifying pixels of dissimilar type is a compile-time error.
///
/// \sa \ref image::operator*(image::float_type, const image::Pixel<R>&)
///
template<class R, class S> auto operator+(const image::Pixel<R>& a, const Pixel<S>& b) noexcept ->
    image::Pixel<typename R::promoted_type>;


/// \brief Modulate opacity of pixel.
///
/// This operator modulates the opacity of the specified pixel (\p pixel) using the
/// specified opacity factor (\p alpha). The resulting alpha component is the alpha
/// component of the specified pixel multiplied by the specified factor. The result is
/// returned in promoted form.
///
/// \sa \ref image::operator+(const image::Pixel<R>, const image::Pixel<S>&)
///
template<class R> auto operator*(image::float_type alpha, const image::Pixel<R>& pixel) noexcept ->
    image::Pixel<typename R::promoted_type>;


/// \brief Write textual representation of pixel to output stream.
///
/// This stream output operator writes a textual representation of the specified pixel (\p
/// pixel) to the specified output stream (\p out). See \ref image::Pixel for information on
/// the format of the textual representation.
///
template<class C, class T, class R> auto operator<<(std::basic_ostream<C, T>& out, const image::Pixel<R>& pixel) ->
    std::basic_ostream<C, T>&;


/// \brief Read textual representation of pixel from source.
///
/// This function reads a textual representation of a pixel (\p size) from the specified
/// value parser source (\p src). See \ref image::Pixel for information on the format of the
/// textual representation. This function is intended to be invoked by a value parser, see
/// \ref core::BasicValueParser for more information.
///
template<class C, class T, class R> bool parse_value(core::BasicValueParserSource<C, T>& src, image::Pixel<R>& pixel);








// Implementation


template<class R>
inline auto Pixel<R>::get_color_space() noexcept -> const image::ColorSpace&
{
    return repr_type::get_color_space();
}


template<class R>
inline Pixel<R>::Pixel(std::array<comp_type, num_channels> components) noexcept
    : m_components(components)
{
}


template<class R>
template<class S> inline Pixel<R>::Pixel(const Pixel<S>& other) noexcept
{
    using other_type = Pixel<S>;
    static_assert(color_space_tag == other_type::color_space_tag);
    constexpr int num_color_space_channels = image::get_num_channels(color_space_tag);
    image::pixel_convert<S::comp_repr, comp_repr>(other.data(), other_type::has_alpha, data(), has_alpha,
                                                  num_color_space_channels);
}


template<class R>
inline Pixel<R>::Pixel(util::Color color)
{
    image::int8_type origin[4];
    for (int i = 0; i < 4; ++i)
        origin[i] = image::int_to_int<8, image::int8_type, 8>(color[i]);
    constexpr image::CompRepr origin_comp_repr = image::CompRepr::int8;
    const image::ColorSpace& origin_color_space = image::ColorSpace::get_rgb();
    bool origin_has_alpha = true;
    comp_type* destin = m_components.data();
    const image::ColorSpace& destin_color_space = get_color_space();
    image::float_type interm[std::max(4, num_channels)];
    // A custom color space converter is neither needed nor allowed when either color space
    // is RGB
    const image::ColorSpaceConverter* custom_converter = nullptr;
    image::pixel_convert_a<origin_comp_repr, comp_repr>(origin, origin_color_space, origin_has_alpha,
                                                        destin, destin_color_space, has_alpha,
                                                        interm, custom_converter); // Throws
}


template<class R>
inline Pixel<R>::operator util::Color() const
{
    const comp_type* origin = m_components.data();
    const image::ColorSpace& origin_color_space = get_color_space();
    constexpr image::CompRepr destin_comp_repr = image::CompRepr::int8;
    image::int8_type destin[4];
    const image::ColorSpace& destin_color_space = image::ColorSpace::get_rgb();
    bool destin_has_alpha = true;
    image::float_type interm[std::max(num_channels, 4)];
    // A custom color space converter is neither needed nor allowed when either color space
    // is RGB
    const image::ColorSpaceConverter* custom_converter = nullptr;
    image::pixel_convert_a<comp_repr, destin_comp_repr>(origin, origin_color_space, has_alpha,
                                                        destin, destin_color_space, destin_has_alpha,
                                                        interm, custom_converter); // Throws
    util::Color color;
    for (int i = 0; i < 4; ++i)
        color[i] = image::int_to_int<8, util::Color::comp_type, 8>(origin[i]);
    return color;
}


template<class R>
inline auto Pixel<R>::operator[](std::size_t i) noexcept -> comp_type&
{
    return m_components[i];
}


template<class R>
inline auto Pixel<R>::operator[](std::size_t i) const noexcept -> const comp_type&
{
    return m_components[i];
}


template<class R>
inline auto Pixel<R>::data() noexcept -> comp_type*
{
    return m_components.data();
}


template<class R>
inline auto Pixel<R>::data() const noexcept -> const comp_type*
{
    return m_components.data();
}


template<class R>
constexpr auto Pixel<R>::size() const noexcept -> std::size_t
{
    return m_components.size();
}


template<class R>
inline bool Pixel<R>::operator==(const Pixel& other) const noexcept
{
    return std::equal(data(), data() + size(), other.data());
}


template<class R>
inline bool Pixel<R>::operator!=(const Pixel& other) const noexcept
{
    return !(*this == other);
}


template<class R>
inline bool Pixel<R>::operator<(const Pixel& other) const noexcept
{
    std::size_t n = num_channels;
    return std::lexicographical_compare(data(), data() + n, other.data(), other.data() + n);
}


template<class R>
inline bool Pixel<R>::operator<=(const Pixel& other) const noexcept
{
    return !(*this > other);
}


template<class R>
inline bool Pixel<R>::operator>(const Pixel& other) const noexcept
{
    return (other < *this);
}


template<class R>
inline bool Pixel<R>::operator>=(const Pixel& other) const noexcept
{
    return  !(*this < other);
}


template<class R>
inline auto Pixel<R>::opacity() const noexcept -> comp_type
{
    if constexpr (has_alpha) {
        return m_components[num_channels - 1];
    }
    else {
        return image::comp_repr_max<comp_repr>();
    }
}


template<class R>
inline auto Pixel<R>::canonicalize() const noexcept -> Pixel
{
    if (ARCHON_LIKELY(opacity() != 0))
        return *this;
    return {};
}


template<class R>
template<class S> inline auto Pixel<R>::convert(const image::ColorSpaceConverter* custom_converter) const -> Pixel<S>
{
    using repr_type_2 = S;
    constexpr image::CompRepr comp_repr_2 = repr_type_2::comp_repr;
    Pixel<S> pixel;
    image::float_type interm[std::max(repr_type::num_channels, repr_type_2::num_channels)];
    image::pixel_convert_a<comp_repr, comp_repr_2>(data(), repr_type::get_color_space(), repr_type::has_alpha,
                                                   pixel.data(), repr_type_2::get_color_space(),
                                                   repr_type_2::has_alpha, interm, custom_converter); // Throws
    return pixel;
}


template<class R>
auto Pixel<R>::blend(const Pixel& other, image::BlendMode mode, image::float_type opacity) noexcept -> Pixel
{
    promoted_pixel_type a = promote();
    promoted_pixel_type b = other.promote();
    int n = promoted_pixel_type::num_channels;
    for (int i = 0; i < n; ++i)
        a[i] *= opacity;
    promoted_pixel_type c;
    image::blend(a.data(), b.data(), c.data(), n, mode);
    return Pixel(c);
}


template<class R>
inline auto Pixel<R>::promote() const noexcept -> promoted_pixel_type
{
    return promoted_pixel_type(*this);
}


template<class R, class S> inline auto operator+(const Pixel<R>& a, const Pixel<S>& b) noexcept ->
    image::Pixel<typename R::promoted_type>
{
    return a.promote().blend(b.promote(), image::BlendMode::over);
}


template<class R> inline auto operator*(image::float_type alpha, const image::Pixel<R>& pixel) noexcept ->
    image::Pixel<typename R::promoted_type>
{
    using promoted_pixel_type = image::Pixel<typename R::promoted_type>;
    promoted_pixel_type pixel_2 = pixel.promote();
    for (int i = 0; i < promoted_pixel_type::num_channels; ++i)
        pixel_2[i] *= alpha;
    return pixel_2;
}


template<class C, class T, class R> auto operator<<(std::basic_ostream<C, T>& out, const image::Pixel<R>& pixel) ->
    std::basic_ostream<C, T>&
{
    using repr_type = R;
    constexpr image::CompRepr comp_repr = repr_type::comp_repr;
    using comp_type = image::comp_type<comp_repr>;
    core::AsListConfig config;
    config.bracketed = true;
    config.space = core::AsListSpace::tight;
    return out << core::as_list(core::Span(pixel), [](const comp_type& comp) {
        if constexpr (std::is_integral_v<comp_type>) {
            return core::as_int(image::comp_repr_unpack<comp_repr>(comp));
        }
        else {
            static_assert(std::is_floating_point_v<comp_type>);
            return core::with_reverted_numerics(comp); // Throws
        }
    }, std::move(config)); // Throws
}


template<class C, class T, class R> bool parse_value(core::BasicValueParserSource<C, T>& src, image::Pixel<R>& pixel)
{
    using repr_type = R;
    constexpr image::CompRepr comp_repr = repr_type::comp_repr;
    using unpacked_comp_type = image::unpacked_comp_type<comp_repr>;
    std::array<unpacked_comp_type, repr_type::num_channels> unpacked_components;
    core::AsListConfig config;
    config.bracketed = true;
    config.space = core::AsListSpace::tight;
    bool success = src.delegate(core::as_list(core::Span(unpacked_components), [](unpacked_comp_type& val) {
        if constexpr (std::is_integral_v<unpacked_comp_type>) {
            return core::as_int(val);
        }
        else {
            static_assert(std::is_floating_point_v<unpacked_comp_type>);
            return core::with_reverted_numerics(val); // Throws
        }
    }, std::move(config))); // Throws
    if (ARCHON_LIKELY(success)) {
        for (int i = 0; i < repr_type::num_channels; ++i)
            pixel[i] = image::comp_repr_pack<comp_repr>(unpacked_components[i]);
        return true;
    }
    return false;
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_PIXEL_HPP
