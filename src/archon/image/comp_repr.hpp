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

#ifndef ARCHON_X_IMAGE_X_COMP_REPR_HPP
#define ARCHON_X_IMAGE_X_COMP_REPR_HPP

/// \file


#include <type_traits>
#include <algorithm>

#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/enum.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/gamma.hpp>


namespace archon::image {



/// \brief Alternative pixel component representation schemes.
///
/// This is a set of alternative component representation schemes that is used in many
/// places in the Archon Image Library as part of the specification of a concrete pixel
/// format.
///
/// These schemes are described in terms of data type, bit width, application of gamma
/// compression, and application of alpha premultiplication. On the other hand, these
/// schemes do not prescribe a particular color space, nor whether an alpha channel is
/// present. Such information must be supplied by the application when needed.
///
/// When pixels are processed or passed from one place to another, their components
/// generally have to be expressed according to one of these schemes. Contrast this with the
/// storage of pixels inside an image where no such restriction applies. For example, \ref
/// image::BufferedImage allows for a much wider variety of component representation
/// schemes.
///
/// Each representation scheme is associated with a particular word type. Pixels are
/// represented as an ordered sequence on words of this type, with one word per channel, and
/// with each word holding the corresponding channel component. The order of channel
/// components is always the canonical order for the color space in use (\ref
/// image::ColorSpace), and the alpha channel always comes last, when an alpha channel is
/// present. The word type for a particular representation scheme, `R`, is available as
/// `image::comp_type<R>`. See \ref image::comp_type.
///
///   | Scheme   | Word type            | Image library type alias
///   |----------|----------------------|--------------------------
///   | `int8`   | `char`               | `image::int8_type`
///   | `int16`  | `std::int_least16_t` | `image::int16_type`
///   | `float_` | `float`              | `image::float_type`
///
/// An integer based representation scheme specifies a bit width, which implies a value
/// range (see below). For a floating-point based scheme, the value range is always from 0
/// to 1. For a color channel, zero refers to zero intensity and the maximum value refers to
/// maximum intensity. For an alpha channel, zero refers to completely transparent, and the
/// maximum value to fully opaque. For an index channel, the value refers to a particular
/// color in a palette, and the value range determines the maximum palette size.
///
///   | Scheme   | Scheme type    | Bit width | Value range
///   |----------|----------------|-----------|-------------
///   | `int8`   | Integer        | 8         | 0 -> 255
///   | `int16`  | Integer        | 16        | 0 -> 65535
///   | `float_` | Floating point |           | 0 -> 1
///
/// When an integer-based scheme uses N bits, but the associated word type has more than N
/// bits, it is always the N least significant bits that are used.
///
/// For an integer-based scheme, when the associated word type is signed, and the unsigned
/// version of the type has more value bits than the signed type (usually the case), the
/// sign bit is effectively available as an extra value bit. See \ref image::pack_int() and
/// \ref image::unpack_int().
///
/// In an integer-based scheme, component values are not allowed to be out of range. In
/// general, behavior is undefined if integer components are out of range when passed to the
/// Image library. Conversely, the Image library generally guarantees that integer
/// components are in range when passed to the application.
///
/// In a floating-point based scheme, component values are allowed to be out of
/// range. However, the effect of passing such values to the Image library is generally
/// unspecified. Also, the Image library does not guarantee that floating-point values are
/// in range when components are passed to the application. As a general rule, if
/// floating-point component values are in range when passed to the Image library, the
/// application should expect floating-point component values received from the Image
/// library to be in range.
///
/// In an integer-based scheme, color channels are gamma compressed using the gamma
/// compression scheme specified by sRGB (see \ref image::compressed_int_to_float() and \ref
/// image::float_to_compressed_int()). The alpha channel is never gamma
/// compressed. Floating-point based schemes do not use gamma compression at all.
///
/// In a floating-point based scheme, alpha channel premultiplication is used. This means
/// that when an alpha channel is present, the value of a color channel component is the
/// result of multiplying the linear channel intensity by the value of the alpha component
/// for the pixel. Integer-based schemes do not use alpha premultiplication.
///
///   | Scheme   | Gamma-compressed color channels | Premultiplied alpha
///   |----------|---------------------------------|---------------------
///   | `int8`   | Yes                             | No
///   | `int16`  | Yes                             | No
///   | `float_` | No                              | Yes
///
/// For an example of the use of the choice between these component representation schemes,
/// see \ref image::Pixel. There, the component representation scheme is specified via the
/// pixel representation scheme (\ref image::PixelRepr).
///
/// A specialization of \ref core::EnumTraits is provided, making stream input and output
/// immediately available.
///
enum class CompRepr {
    int8,   ///< Integer, 8 bits.
    int16,  ///< Integer, 16 bits.
    float_, ///< Floating point.
};



/// \brief Component representation used for indexes referring to colors of palette.
///
/// This is the component representation scheme that is used for transferring and processing
/// pixels that take the form of indexes into an associated palette of colors.
///
constexpr image::CompRepr color_index_repr = image::CompRepr::int8;



namespace impl {
template<image::CompRepr> struct CompType;
template<image::CompRepr> struct UnpackedCompType;
} // namespace impl



/// \brief Word type of component representation scheme.
///
/// This is the word type associated with the specified component representation scheme (\p
/// R). It is used for storing arrays of components. Each word stores one component.
///
template<image::CompRepr R> using comp_type = typename impl::CompType<R>::type;


/// \brief Default type for unpacked component values.
///
/// This is the type that is used by default to hold component values in their unpacked form
/// when using the specified component representation scheme (\p R).
///
/// When the specified component representation scheme is integer-based (see \ref
/// image::CompRepr), `image::unpacked_comp_type<R>` is `image::unpacked_type<T, N>` where
/// `T` is `image::comp_type<R>` and `N` is `image::comp_repr_int_bit_width(R)`.
///
/// When the specified component representation scheme is based on a floating-point type,
/// `image::unpacked_comp_type<R>` is `image::comp_type<R>`.
///
/// \sa \ref image::comp_repr_pack()
/// \sa \ref image::comp_repr_unpack()
/// \sa \ref image::comp_type
///
template<image::CompRepr R> using unpacked_comp_type = typename impl::UnpackedCompType<R>::type;


/// \{
///
/// \brief Pixel iterator types associated with component representation scheme.
///
/// These are the pixel iterator types (\ref image::Iter) associated with the specified
/// component representation scheme (\p R).
///
/// If `T` is `image::comp_type<R>`, `image::iter_type<R>` is `image::Iter<T>`, and
/// `image::const_iter_type<R>` is `image::Iter<const T>`.
///
/// \sa \ref image::tray_type, \ref image::const_tray_type
///
template<image::CompRepr R> using iter_type = image::Iter<image::comp_type<R>>;
template<image::CompRepr R> using const_iter_type = image::Iter<const image::comp_type<R>>;
/// \}


/// \{
///
/// \brief Pixel tray types associated with component representation scheme.
///
/// These are the pixel tray types (\ref image::Tray) associated with the specified
/// component representation scheme (\p R).
///
/// If `T` is `image::comp_type<R>`, `image::tray_type<R>` is `image::Tray<T>`, and
/// `image::const_tray_type<R>` is `image::Tray<const T>`.
///
/// \sa \ref image::iter_type, \ref image::const_iter_type
///
template<image::CompRepr R> using tray_type = image::Tray<image::comp_type<R>>;
template<image::CompRepr R> using const_tray_type = image::Tray<const image::comp_type<R>>;
/// \}



/// \brief Convert component value to packed form.
///
/// If the specified component representation scheme is integer-based (see \ref
/// image::CompRepr), this function packs a non-negative integer value (\p val) into a bit
/// medium whose type and width is determined by that component representation scheme. The
/// packing operation is performed by \ref image::pack_int(), and the bit medium width is as
/// returned by \ref image::comp_repr_int_bit_width(). If the specified value is out of
/// range, i.e., if it is negative or greater than `2^N - 1` where `N` is
/// `image::comp_repr_int_bit_width(R)`, the result is unspecified. No undefined behavior is
/// invoked, though.
///
/// If the specified component representation scheme is based on a floating-point type, this
/// function returns the specified floating-point value unchanged, and without changing its
/// type.
///
/// \sa \ref image::comp_repr_unpack()
///
template<image::CompRepr R> auto comp_repr_pack(image::unpacked_comp_type<R> val) noexcept -> image::comp_type<R>;


/// \brief Retrieve original component value from its packed form.
///
/// If the specified component representation scheme is integer-based (see \ref
/// image::CompRepr), this function retrieve the original non-negative component value from
/// the specified packed form (\p comp). If the specified packed form is not one that could
/// have been produced by `image::comp_repr_pack<R>(val)` for some non-negative N-bit value
/// `val` where N is `image::comp_repr_int_bit_width(R)` (\ref
/// image::comp_repr_int_bit_width()), the result is unspecified. No undefined behavior is
/// invoked, though.
///
/// If the specified component representation scheme is based on a floating-point type, this
/// function returns the specified floating-point value unchanged, and without changing its
/// type.
///
/// \sa \ref image::comp_repr_pack()
///
template<image::CompRepr R> auto comp_repr_unpack(image::comp_type<R> comp) noexcept -> image::unpacked_comp_type<R>;



/// \brief Convert color component to floating-point form.
///
/// This function converts the specified color component (\p comp) to floating-point and
/// linear intensity form. The specified color component is assumed to be expressed
/// according to the specified component representation scheme (\p R). Because color
/// components on integer form are gamma compressed, (see \ref image::CompRepr), this
/// function must only be used with color components, not with alpha components. If \p R is
/// `float_`, this function returns \p comp unchanged.
///
/// \sa \ref image::alpha_comp_to_float()
/// \sa \ref image::color_comp_from_float()
///
template<image::CompRepr R> auto color_comp_to_float(image::comp_type<R> comp) -> image::float_type;



/// \brief Convert alpha component to floating-point form.
///
/// This function converts the specified alpha component (\p comp) to floating-point
/// form. The specified alpha component is assumed to be expressed according to the
/// specified component representation scheme (\p R). Because alpha components on integer
/// form are not gamma compressed (see \ref image::CompRepr), this function must only be
/// used with alpha components, not with color components. If \p R is `float_`, this
/// function returns \p comp unchanged.
///
/// \sa \ref image::color_comp_to_float()
/// \sa \ref image::alpha_comp_from_float()
///
template<image::CompRepr R> auto alpha_comp_to_float(image::comp_type<R> comp) -> image::float_type;



/// \brief Convert color component from floating-point form.
///
/// This function converts the specified color component (\p comp) to the form prescribed by
/// the specified component representation scheme (\p R). The color component is specified
/// in floating-point and linear intensity form. When the component representation scheme is
/// an integer-based scheme, components will be gamma compressed (see \ref image::CompRepr),
/// so this function must only be used with color components, not with alpha components. If
/// \p R is `float_`, this function returns \p comp unchanged.
///
/// \sa \ref image::alpha_comp_from_float()
/// \sa \ref image::color_comp_to_float()
///
template<image::CompRepr R> auto color_comp_from_float(image::float_type comp) -> image::comp_type<R>;



/// \brief Convert alpha component from floating-point form.
///
/// This function converts the specified alpha component (\p comp) to the form prescribed by
/// the specified component representation scheme (\p R). The alpha component is specified
/// in floating-point form. This function does not perform any gamma-compression (see \ref
/// image::CompRepr), so it must only be used with alpha components, not with color
/// components. If \p R is `float_`, this function returns \p comp unchanged.
///
/// \sa \ref image::color_comp_from_float()
/// \sa \ref image::alpha_comp_to_float()
///
template<image::CompRepr R> auto alpha_comp_from_float(image::float_type comp) -> image::comp_type<R>;



/// \brief Generalized bit width of component representation scheme.
///
/// If the specified component representation scheme (\p R) is an integer-based scheme, this
/// function returns `image::comp_repr_int_bit_width(R)` (\see
/// image::comp_repr_int_bit_width()). Otherwise, that is, for floating-point based schemes,
/// this function returns `image::bit_width<image::comp_type<R>>` (see \ref
/// image::bit_width).
///
template<image::CompRepr R> constexpr int comp_repr_bit_width() noexcept;



/// \brief Bit width of integer-based component representation scheme.
///
/// If the specified component representation scheme (\p repr) is an integer-based scheme,
/// this function returns the bit width associated with the scheme (\see
/// image::CompRepr). Otherwise, that is, for floating-point based schemes, this function
/// returns zero.
///
constexpr int comp_repr_int_bit_width(image::CompRepr repr) noexcept;



/// \brief Maximum intensity value for given component representation scheme.
///
/// This function returns the value that denotes (nominal) maximum intensity for a component
/// expressed according to the specified component representation scheme (\p R). For
/// floating-point based schemes, this is always 1. For integer-based schemes where the
/// integer type is signed, the sign-bit may be used as a value bit. This means that the
/// returned value may be negative.
///
/// Regardless of component representation scheme, the value that denotes (nominal) minimum
/// intensity is always zero.
///
template<image::CompRepr R> constexpr auto comp_repr_max() noexcept -> image::comp_type<R>;



/// \brief Compare values expressed according to given component representation scheme.
///
/// This function compares to component values expressed according to the specified
/// component representation scheme (\p P). It returns `true` when, and only when \p a is
/// less than \p b. This comparison is non-trivial because, for integer-based schemes where
/// the integer type is signed, the sign-bit may be used as an extra value bit.
///
template<image::CompRepr R> constexpr bool comp_repr_less(image::comp_type<R> a, image::comp_type<R> b);



/// \brief Convert pixel between component representation schemes.
///
/// This function converts a pixel from one component representation scheme (\p R) to
/// another (\p S). See \ref image::CompRepr for an explanation of the notion of a
/// *component representation scheme*.
///
/// \param num_channels The number of channels per pixel, which is the number of channels in
/// the color space plus one if, and only if \p has_alpha is true.
///
template<image::CompRepr R, image::CompRepr S>
void comp_repr_convert(const image::comp_type<R>* origin, image::comp_type<S>* destin, int num_channels,
                       bool has_alpha);


/// \brief Convert array of pixels between component representation schemes.
///
/// This function converts an array of pixels from one component representation scheme (\p
/// R) to another (\p S). Each pixel is converted as if by \ref image::comp_repr_convert().
///
template<image::CompRepr R, image::CompRepr S>
void comp_repr_convert_a(const image::const_tray_type<R>& origin, const image::iter_type<S>& destin,
                         int num_channels, bool has_alpha);


/// \brief Choose suitable component representation scheme for bit depth.
///
/// This function chooses a suitable representation scheme for transfer of pixel components
/// that are otherwise represented as integer values using the specified number of bits.
///
constexpr auto choose_transf_repr(int num_bits) noexcept -> image::CompRepr;








// Implementation


} // namespace archon::image

namespace archon::core {


template<> struct EnumTraits<image::CompRepr> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(image::CompRepr::int8),   "int8"  },
            { int(image::CompRepr::int16),  "int16" },
            { int(image::CompRepr::float_), "float" },
        };
    };
    static constexpr bool ignore_case = false;
};


} // namespace archon::core

namespace archon::image {

namespace impl {


template<> struct CompType<image::CompRepr::int8> {
    using type = image::int8_type;
};

template<> struct CompType<image::CompRepr::int16> {
    using type = image::int16_type;
};

template<> struct CompType<image::CompRepr::float_> {
    using type = image::float_type;
};


template<image::CompRepr R, class T, bool I> struct UnpackedCompTypeHelper {
    static_assert(std::is_floating_point_v<T>);
    using type = T;
};

template<image::CompRepr R, class T> struct UnpackedCompTypeHelper<R, T, true> {
    static constexpr int bit_width = image::comp_repr_int_bit_width(R);
    using type = image::unpacked_type<T, bit_width>;
};

template<image::CompRepr R> struct UnpackedCompType {
    using comp_type = image::comp_type<R>;
    using type = typename impl::UnpackedCompTypeHelper<R, comp_type, std::is_integral_v<comp_type>>::type;
};


} // namespace impl


template<image::CompRepr R> auto comp_repr_pack(image::unpacked_comp_type<R> val) noexcept -> image::comp_type<R>
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int bit_width = image::comp_repr_int_bit_width(R);
        return image::pack_int<comp_type, bit_width>(val);
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return val;
    }
}


template<image::CompRepr R> auto comp_repr_unpack(image::comp_type<R> comp) noexcept -> image::unpacked_comp_type<R>
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int bit_width = image::comp_repr_int_bit_width(R);
        return image::unpack_int<bit_width>(comp);
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return comp;
    }
}


template<image::CompRepr R> inline auto color_comp_to_float(image::comp_type<R> comp) -> image::float_type
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int depth = image::comp_repr_bit_width<R>();
        return image::compressed_int_to_float<depth>(comp); // Throws
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return image::float_type(comp);
    }
}


template<image::CompRepr R> inline auto alpha_comp_to_float(image::comp_type<R> comp) -> image::float_type
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int depth = image::comp_repr_bit_width<R>();
        return image::int_to_float<depth, image::float_type>(comp); // Throws
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return image::float_type(comp);
    }
}


template<image::CompRepr R> inline auto color_comp_from_float(image::float_type comp) -> image::comp_type<R>
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int depth = image::comp_repr_bit_width<R>();
        return image::float_to_compressed_int<comp_type, depth>(comp); // Throws
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return comp_type(comp);
    }
}


template<image::CompRepr R> inline auto alpha_comp_from_float(image::float_type comp) -> image::comp_type<R>
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int depth = image::comp_repr_bit_width<R>();
        return image::float_to_int<comp_type, depth>(comp); // Throws
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return comp_type(comp);
    }
}


template<image::CompRepr R> constexpr int comp_repr_bit_width() noexcept
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        return image::comp_repr_int_bit_width(R);
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return image::bit_width<comp_type>;
    }
}


constexpr int comp_repr_int_bit_width(image::CompRepr repr) noexcept
{
    switch (repr) {
        case image::CompRepr::int8:
            return 8;
        case image::CompRepr::int16:
            return 16;
        case image::CompRepr::float_:
            break;
    }
    return 0;
}


template<image::CompRepr R> constexpr auto comp_repr_max() noexcept -> image::comp_type<R>
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int n = image::comp_repr_bit_width<R>();
        using unpacked_type = image::unpacked_type<comp_type, n>;
        return image::pack_int<comp_type, n>(core::int_mask<unpacked_type>(n));
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return comp_type(1);
    }
}


template<image::CompRepr R> constexpr bool comp_repr_less(image::comp_type<R> a, image::comp_type<R> b)
{
    using comp_type = image::comp_type<R>;
    if constexpr (std::is_integral_v<comp_type>) {
        constexpr int n = image::comp_repr_bit_width<R>();
        return (image::unpack_int<n>(a) < image::unpack_int<n>(b));
    }
    else {
        static_assert(std::is_floating_point_v<comp_type>);
        return (a < b);
    }
}


template<image::CompRepr R, image::CompRepr S>
inline void comp_repr_convert(const image::comp_type<R>* origin, image::comp_type<S>* destin, int num_channels,
                              bool has_alpha)
{
    constexpr image::CompRepr repr_1 = R;
    constexpr image::CompRepr repr_2 = S;
    using comp_type_1 = image::comp_type<repr_1>;
    using comp_type_2 = image::comp_type<repr_2>;
    constexpr bool is_float_1 = std::is_floating_point_v<comp_type_1>;
    constexpr bool is_float_2 = std::is_floating_point_v<comp_type_2>;
    ARCHON_ASSERT(num_channels > 0);

    if constexpr (repr_1 == repr_2 || (is_float_1 && is_float_2)) {
        // Alternative 1: No conversion or float -> float
        std::copy(origin, origin + num_channels, destin);
    }
    else if constexpr (is_float_1) {
        // Alternative 2: float -> int
        static_assert(repr_1 == image::CompRepr::float_);
        constexpr int depth = image::comp_repr_bit_width<repr_2>();
        if (!has_alpha) {
            for (int i = 0; i < num_channels; ++i)
                destin[i] = image::float_to_compressed_int<comp_type_2, depth>(origin[i]); // Throws
        }
        else {
            // Undo premultiplication of alpha
            int last = num_channels - 1;
            comp_type_1 alpha = origin[last];
            auto inv_alpha = (alpha != 0 ? 1.0 / alpha : 0.0);
            for (int i = 0; i < last; ++i) {
                comp_type_1 value = comp_type_1(inv_alpha * origin[i]);
                destin[i] = image::float_to_compressed_int<comp_type_2, depth>(value); // Throws
            }
            destin[last] = image::float_to_int<comp_type_2, depth>(alpha); // Throws
        }
    }
    else if constexpr (is_float_2) {
        // Alternative 3: int -> float
        static_assert(repr_2 == image::CompRepr::float_);
        constexpr int depth = image::comp_repr_bit_width<repr_1>();
        if (!has_alpha) {
            for (int i = 0; i < num_channels; ++i)
                destin[i] = image::compressed_int_to_float<depth>(origin[i]); // Throws
        }
        else {
            // Premultiply alpha
            int last = num_channels - 1;
            comp_type_2 alpha = image::int_to_float<depth, comp_type_2>(origin[last]); // Throws
            for (int i = 0; i < last; ++i)
                destin[i] = alpha * image::compressed_int_to_float<depth>(origin[i]); // Throws
            destin[last] = alpha;
        }
    }
    else {
        // Alternative 4: int -> int
        constexpr int depth_1 = image::comp_repr_bit_width<repr_1>();
        constexpr int depth_2 = image::comp_repr_bit_width<repr_2>();
        for (int i = 0; i < num_channels; ++i)
            destin[i] = image::int_to_int<depth_1, comp_type_2, depth_2>(origin[i]);
    }
}


template<image::CompRepr R, image::CompRepr S>
void comp_repr_convert_a(const image::const_tray_type<R>& origin, const image::iter_type<S>& destin,
                         int num_channels, bool has_alpha)
{
    constexpr image::CompRepr repr_1 = R;
    constexpr image::CompRepr repr_2 = S;
    using comp_type_1 = image::comp_type<repr_1>;
    using comp_type_2 = image::comp_type<repr_2>;

    for (int y = 0; y < origin.size.height; ++y) {
        for (int x = 0; x < origin.size.width; ++x) {
            const comp_type_1* origin_2 = origin(x, y);
            comp_type_2* destin_2 = destin(x, y);
            image::comp_repr_convert<repr_1, repr_2>(origin_2, destin_2, num_channels, has_alpha); // Throws
        }
    }
}


constexpr auto choose_transf_repr(int num_bits) noexcept -> image::CompRepr
{
    if (num_bits <= 8)
        return image::CompRepr::int8;
    if (num_bits <= 16)
        return image::CompRepr::int16;
    return image::CompRepr::float_;
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_COMP_REPR_HPP
