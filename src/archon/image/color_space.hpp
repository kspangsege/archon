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

#ifndef ARCHON_X_IMAGE_X_COLOR_SPACE_HPP
#define ARCHON_X_IMAGE_X_COLOR_SPACE_HPP

/// \file


#include <utility>
#include <optional>
#include <map>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/image/comp_types.hpp>


namespace archon::image {


/// \brief Abstract base class for individual color spaces.
///
/// Any color space that is to be used with the Archon Image Library must be represented by
/// an object of this type. The Archon Image Library provides objects for all the standard
/// color spaces (RGB and Luminance), see \ref Tag and \ref image::get_color_space().
///
/// FIXME: Explain: Applications should assume that two different color space objects represent two different color spaces. This way, color spaces can be identified by their address in memory.                        
///
/// FIXME: Explain *canonical channel order*: Each color space must specify its canonical channel order.    
///
class ColorSpace {
public:
    /// \{
    ///
    /// \brief Get references to standard color spaces.
    ///
    /// These functions return a reference to each of the standard color spaces. I.e., the
    /// color spaces for which a tag exists (\ref Tag).
    ///
    static auto get_degen() noexcept -> const ColorSpace&;
    static auto get_lum() noexcept   -> const ColorSpace&;
    static auto get_rgb() noexcept   -> const ColorSpace&;
    /// \}

    /// \{
    ///
    /// \brief Whether this color space is Degenerate, Luminance, or RGB.
    ///
    /// These functions respectively check whether this color space is the standard
    /// degenerate color space, whether it is the standard Luminance color space, and
    /// whether it is the standard RGB color space. See \ref Tag.
    ///
    bool is_degen() const noexcept;
    bool is_lum() const noexcept;
    bool is_rgb() const noexcept;
    /// \}

    enum class Tag;

    /// \brief Whether color space object represents specified standard color space.
    ///
    /// This function returns `true` if, and only if this color space object is the one that
    /// represents the specified standard color space.
    ///
    bool is(Tag) const noexcept;

    /// \brief Determine associated standard color space, if any.
    ///
    /// If this color space object represents one of the standard color spaces (\ref Tag),
    /// this function returns `true` after setting \p tag to indicate which of the standard
    /// color spaces it represents.
    ///
    /// If this color space object does not represent any of the standard color spaces, this
    /// function returns `false` and leaves \p tag unchanged.
    ///
    bool try_get_tag(Tag&) const noexcept;

    /// \brief Number of channels in color space.
    ///
    /// This function returns the number of channels in this color space. For the RGB color
    /// space (\ref get_rgb()), this function would return 3.
    ///
    int get_num_channels() const noexcept;

    /// \brief Required channel component type for color space conversions.
    ///
    /// This is the type that must be used for channel components when converting between
    /// color spaces (\ref from_rgb() and \ref to_rgb()).
    ///
    using comp_type = image::float_type;

    /// \{
    ///
    /// \brief Convert to and from the RGB color space.
    ///
    /// These functions convert to and from the RGB color space. `from_rgb()` takes a color
    /// expressed in terms of the RGB color space (\p rgb) and converts it to a color
    /// expressed in terms of the color space represented by this color space object (\p
    /// native). `to_rgb()` takes a color expressed in terms of the color space represented
    /// by this color space object (\p native) and converts it to a color expressed in terms
    /// of the RGB color space (\p rgb).
    ///
    /// \param rgb A pointer to an array of 3 color components making up a color expressed
    /// in terms of the RGB color space.
    ///
    /// \param native A pointer to an array of color components making up a color expressed
    /// in terms of the native color space, which is the color space represented by this
    /// color space object. The number of components in this array has to be the number
    /// returned by \ref get_num_channels().
    ///
    /// \param alpha The alpha value associated with the color to be converted. For color
    /// spaces such as Luminance, that have a linear relationship with the RGB color space,
    /// the alpha value makes no difference (is ignored). CIEXYZ (\ref
    /// util::cvt_sRGB_to_XYZ()) is another example of a color space that has a linear
    /// relationship with RGB (the linear form of sRGB).
    ///
    /// For both RGB and the native colors space, the order of channels is the canonical
    /// channel order for that color space.
    ///
    /// FIXME: Explain that color components are expressed linearly as opposed to in gamma compressed form.               
    ///
    virtual void from_rgb(const comp_type* rgb, comp_type* native, comp_type alpha) const = 0;
    virtual void to_rgb(const comp_type* native, comp_type* rgb, comp_type alpha) const = 0;
    /// \}

    virtual ~ColorSpace() noexcept = default;

protected:
    constexpr ColorSpace(int num_channels) noexcept;
    constexpr ColorSpace(Tag) noexcept;

private:
    std::optional<Tag> m_tag;
    int m_num_channels;
};


/// \brief Standard color spaces.
///
/// These are the *standard color spaces*.
///
/// | Value   | Channels
/// |---------|------------------
/// | `degen` | *none*
/// | `lum`   | Luminance
/// | `rgb`   | Red, green, blue
///
/// The canonical channel order is as shown here.
///
/// The degenerate color space has no channels, and can only represent one color, which is
/// white.
///
enum class ColorSpace::Tag {
    degen,
    lum,
    rgb,
};


/// \brief Get color space object for specified standard color space.
///
/// This function returns a reference to the color space object for the specified standard
/// color space (\p tag).
///
auto get_color_space(image::ColorSpace::Tag tag) noexcept -> const image::ColorSpace&;


/// \brief Number of channels in standard color space.
///
/// This function returns the number of channels in the specified standard color space.
///
constexpr int get_num_channels(image::ColorSpace::Tag) noexcept;



/// \brief Base class for custom color space converters.
///
/// This is the base class for custom color space converters. A custom color space converter
/// converts pixels from one color space to another. Implementation of this class can be
/// registered in a color space converter registry (\ref
/// image::ColorSpaceConverterRegistry). They can also be used directly with \ref
/// image::color_space_convert().
///
/// The primary role of a color space converter is to override the default color space
/// conversion in a reader or writer (\ref image::Reader, \ref image::Writer). When neither
/// the origin, nor the destination color space is RGB, the default color space conversion
/// first converts to RGB, and then from RGB to the destination color space. For some
/// combinations of color spaces, conversion through RGB is unsuitable, and a custom color
/// space converter can be used instead. Readers and writers delegate color space conversion
/// to \ref image::color_space_convert().
///
class ColorSpaceConverter {
public:
    /// \brief Convert one pixel.
    ///
    /// This function converts a single pixel from the origin color space to the destination
    /// color space.
    ///
    /// This function operates by reading the original pixel from the specified array of
    /// channel components (\p pixel), and then storing the converted pixel back into that
    /// same array. This means that the array must be big enough to hold the pixel expressed
    /// in terms of both color spaces, not including any alpha channel components. To be
    /// more precise, if `a` is the origin color space, and `b` is the destination color
    /// space, then the size of the array pointed to by \p pixel, must be greater than, or
    /// equal to both `a.get_num_channels()` and `b.get_num_channels()`.
    ///
    /// When the converted pixel / color carries an alpha channel component, that alpha
    /// channel component must be passed as \p alpha. When there is no alpha channel
    /// component, \p alpha must be set to 1.
    ///
    /// If a particular custom color space conversion is a linear transformation (eg., RGB
    /// <-> Lum), the implementation of this function can ignore the alpha component. If it
    /// is non-linear, the implementation of this function must apply the effect of the
    /// alpha component before the conversion, and the undo the effect after the conversion.
    ///
    virtual void convert(image::float_type* pixel, image::float_type alpha) const = 0;
};


/// \brief Collection of custom color space converters.
///
/// This class functions as a collection of custom color space converters, and offers a way
/// to look up a particular converter within the collection (\ref find()). A color space is
/// identified by the address of the color space object (singleton), so a particular color
/// space converter is identified by the pair of addresses of the origin and destination
/// color spaces.
///
/// Custom color space converters are added using \ref add().
///
/// A color space converter registry can be installed in an image reader, or writer using
/// \ref image::Reader::set_custom_color_space_converters().
///
class ColorSpaceConverterRegistry {
public:
    /// \brief Add color space converter to registry.
    ///
    /// This function registers a color space converter in this registry. The destination
    /// color space must be different from the origin color space, and neither is allowed to
    /// be RGB (\ref image::ColorSpace::is_rgb()).
    ///
    void add(const image::ColorSpace& origin, const image::ColorSpace& destin,
             const image::ColorSpaceConverter& converter);

    /// \brief Find specific color space converter in registry.
    ///
    /// This function searches this registry for a converter that converts from the
    /// specified origin color space (\p origin) to the specified destination color space
    /// (\p destin). If one is found, it is returned, otherwise null is returned.
    ///
    auto find(const image::ColorSpace& origin, const image::ColorSpace& destin) const noexcept ->
        const image::ColorSpaceConverter*;

private:
    using key_type = std::pair<const image::ColorSpace*, const image::ColorSpace*>;
    std::map<key_type, const image::ColorSpaceConverter*> m_map;

    auto do_find(const image::ColorSpace& origin, const image::ColorSpace& destin) const noexcept ->
        const image::ColorSpaceConverter*;
};


/// \brief Convert pixel from one color space to another.
///
/// This function converts a pixel from the specified origin color space (\p
/// origin_color_space) to the specified destination color space (\p destin_color_space). If
/// a custom color space converter is specified (\p custom_converter), it will be used,
/// otherwise, this function falls back to conversion through RGB, which means that the
/// pixel is first converted to RGB, then to the destination color space.
///
/// This function operates by reading the original pixel from the specified array of channel
/// components (\p pixel), and then storing the converted pixel back into that same
/// array. This means that the array must be big enough to hold the pixel expressed in terms
/// of both color spaces, not including any alpha channel components. To be more precise,
/// the size of the array pointed to by \p pixel, must be greater than, or equal to both
/// `origin_color_space.get_num_channels()` and `destin_color_space.get_num_channels()`.
///
/// When the converted pixel carries an alpha channel component, that alpha channel
/// component must be passed as \p alpha. When there is no alpha channel component, \p alpha
/// must be set to 1.
///
/// It is an error if a custom converter is specified when the two color spaces are the
/// same, or when either one is RGB (\ref image::ColorSpace::is_rgb()). Doing so causes
/// undefined behavior. Note that a color space is identified by the memory address of the
/// color space object.
///
void color_space_convert(image::float_type* pixel, image::float_type alpha,
                         const image::ColorSpace& origin_color_space, const image::ColorSpace& destin_color_space,
                         const image::ColorSpaceConverter* custom_converter);








// Implementation


inline bool ColorSpace::is_degen() const noexcept
{
    return is(Tag::degen);
}


inline bool ColorSpace::is_lum() const noexcept
{
    return is(Tag::lum);
}


inline bool ColorSpace::is_rgb() const noexcept
{
    return is(Tag::rgb);
}


inline bool ColorSpace::is(Tag tag) const noexcept
{
    return (m_tag && m_tag.value() == tag);
}


inline bool ColorSpace::try_get_tag(Tag& tag) const noexcept
{
    if (ARCHON_LIKELY(m_tag)) {
        tag = m_tag.value();
        return true;
    }
    return false;
}


inline int ColorSpace::get_num_channels() const noexcept
{
    return m_num_channels;
}


constexpr ColorSpace::ColorSpace(int num_channels) noexcept
    : m_num_channels(num_channels)
{
}


constexpr ColorSpace::ColorSpace(Tag tag) noexcept
    : m_tag(tag)
    , m_num_channels(image::get_num_channels(tag))
{
}


inline auto get_color_space(image::ColorSpace::Tag tag) noexcept -> const image::ColorSpace&
{
    switch (tag) {
        case image::ColorSpace::Tag::degen:
            return image::ColorSpace::get_degen();
        case image::ColorSpace::Tag::lum:
            return image::ColorSpace::get_lum();
        case image::ColorSpace::Tag::rgb:
            return image::ColorSpace::get_rgb();
    }
    ARCHON_STEADY_ASSERT_UNREACHABLE();
}


constexpr int get_num_channels(image::ColorSpace::Tag tag) noexcept
{
    switch (tag) {
        case image::ColorSpace::Tag::degen:
            return 0;
        case image::ColorSpace::Tag::lum:
            return 1;
        case image::ColorSpace::Tag::rgb:
            return 3;
    }
    ARCHON_STEADY_ASSERT_UNREACHABLE();
}


namespace impl {


class ColorSpaceDegen
    : public image::ColorSpace {
public:
    constexpr ColorSpaceDegen()
        : image::ColorSpace(Tag::degen)
    {
    }

    void from_rgb(const comp_type*, comp_type*, comp_type) const override final;
    void to_rgb(const comp_type*, comp_type*, comp_type) const override final;
};


class ColorSpaceLum
    : public image::ColorSpace {
public:
    constexpr ColorSpaceLum()
        : image::ColorSpace(Tag::lum)
    {
    }

    void from_rgb(const comp_type*, comp_type*, comp_type) const override final;
    void to_rgb(const comp_type*, comp_type*, comp_type) const override final;
};


class ColorSpaceRGB
    : public image::ColorSpace {
public:
    constexpr ColorSpaceRGB()
        : image::ColorSpace(Tag::rgb)
    {
    }

    void from_rgb(const comp_type*, comp_type*, comp_type) const override final;
    void to_rgb(const comp_type*, comp_type*, comp_type) const override final;
};


inline constinit const ColorSpaceDegen g_color_space_degen;
inline constinit const ColorSpaceLum g_color_space_lum;
inline constinit const ColorSpaceRGB g_color_space_rgb;


} // namespace impl


inline auto ColorSpace::get_degen() noexcept -> const ColorSpace&
{
    return impl::g_color_space_degen;
}


inline auto ColorSpace::get_lum() noexcept -> const ColorSpace&
{
    return impl::g_color_space_lum;
}


inline auto ColorSpace::get_rgb() noexcept -> const ColorSpace&
{
    return impl::g_color_space_rgb;
}


inline void ColorSpaceConverterRegistry::add(const image::ColorSpace& origin, const image::ColorSpace& destin,
                                             const image::ColorSpaceConverter& converter)
{
    ARCHON_ASSERT(&origin != &destin);
    ARCHON_ASSERT(!origin.is_rgb());
    ARCHON_ASSERT(!destin.is_rgb());
    key_type key = std::make_pair(&origin, &destin);
    m_map.emplace(key, &converter); // Throws
}


inline auto ColorSpaceConverterRegistry::find(const image::ColorSpace& origin,
                                              const image::ColorSpace& destin) const noexcept ->
    const image::ColorSpaceConverter*
{
    if (ARCHON_LIKELY(&origin == &destin || origin.is_rgb() || destin.is_rgb()))
        return nullptr;
    return do_find(origin, destin);
}


namespace impl {


void color_space_convert(image::float_type* pixel, image::float_type alpha,
                         const image::ColorSpace& origin_color_space, const image::ColorSpace& destin_color_space,
                         const image::ColorSpaceConverter* custom_converter);


} // namespace impl


inline void color_space_convert(image::float_type* pixel, image::float_type alpha,
                                const image::ColorSpace& origin_color_space,
                                const image::ColorSpace& destin_color_space,
                                const image::ColorSpaceConverter* custom_converter)
{
    if (ARCHON_LIKELY(&origin_color_space == &destin_color_space)) {
        ARCHON_ASSERT(!custom_converter);
        return;
    }
    impl::color_space_convert(pixel, alpha, origin_color_space, destin_color_space, custom_converter); // Throws
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_COLOR_SPACE_HPP
