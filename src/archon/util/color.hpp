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

#ifndef ARCHON_X_UTIL_X_COLOR_HPP
#define ARCHON_X_UTIL_X_COLOR_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <array>
#include <tuple>

#include <archon/math/vector.hpp>
#include <archon/util/color_comp.hpp>


namespace archon::util {


/// \brief sRGB color plus alpha component with 8 bits per component.
///
/// An instance of this class represents a color in the sRGB color space plus an alpha
/// component (opacity). Color components are represented in gamma-compressed form as
/// integers in the range [0,255]. The alpha channel is represented linearly as an integer
/// in the range [0,255].
///
class Color {
public:
    using comp_type = std::uint_least8_t;
    using rgba_type = std::uint_fast32_t;
    using trgb_type = std::uint_fast32_t;

    static constexpr auto size() noexcept -> std::size_t;

    /// \brief Default construct as fully transparent black.
    ///
    /// This default constructor constructs a color as fully transparent black.
    ///
    constexpr Color() noexcept = default;

    constexpr Color(comp_type r, comp_type g, comp_type b, comp_type a = 255) noexcept;

    static constexpr auto from_rgba(rgba_type rgba) noexcept -> Color;
    static constexpr auto from_trgb(trgb_type trgb) noexcept -> Color;

    constexpr auto to_rgba() const noexcept -> rgba_type;
    constexpr auto to_trgb() const noexcept -> trgb_type;

    /// \{
    ///
    /// \brief Convert to gamma-compressed floating-point form.
    ///
    /// These function convert a color to a vector of floating-point components in the range
    /// [0,1], or vice versa. Floating-point components pertaining to color channels are
    /// expressed in gamma-compressed form (\ref util::srgb_gamma_compress()). The alpha
    /// channel, is expressed linearly.
    ///
    /// \sa \ref to_compr_vec(), \ref from_compr_vec()
    ///
    template<class T> constexpr void to_compr_vec(math::Vector<4, T>& rgba) const noexcept;
    template<class T> static constexpr auto from_compr_vec(const math::Vector<3, T>& rgb, T a = 1) noexcept -> Color;
    template<class T> static constexpr auto from_compr_vec(const math::Vector<4, T>& rgba) noexcept -> Color;
    /// \}

    /// \brief Whether color is fully opaque.
    ///
    /// This function returns `true` if, and only if the color is fully opaque, that is, if
    /// the alpha component is equal to 255.
    ///
    constexpr bool is_opaque() const noexcept;

    /// \brief Whether all color components are equal.
    ///
    /// This function returns `true` if, and only if the three color components (red, green,
    /// and blue) are equal.
    ///
    constexpr bool is_gray() const noexcept;

    constexpr auto red()   const noexcept -> comp_type;
    constexpr auto green() const noexcept -> comp_type;
    constexpr auto blue()  const noexcept -> comp_type;
    constexpr auto alpha() const noexcept -> comp_type;

    constexpr auto operator[](std::size_t) noexcept       -> comp_type&;
    constexpr auto operator[](std::size_t) const noexcept -> const comp_type&;

    constexpr auto data() noexcept       -> comp_type*;
    constexpr auto data() const noexcept -> const comp_type*;

private:
    std::array<comp_type, 4> m_rgba = {};
};








// Implementation


constexpr auto Color::size() noexcept -> std::size_t
{
    return std::tuple_size_v<decltype(m_rgba)>;
}


constexpr Color::Color(comp_type r, comp_type g, comp_type b, comp_type a) noexcept
{
    m_rgba[0] = r;
    m_rgba[1] = g;
    m_rgba[2] = b;
    m_rgba[3] = a;
}


constexpr auto Color::from_rgba(rgba_type rgba) noexcept -> Color
{
    return Color(comp_type((rgba >> 24) & 0xFF),
                 comp_type((rgba >> 16) & 0xFF),
                 comp_type((rgba >>  8) & 0xFF),
                 comp_type((rgba >>  0) & 0xFF));
}


constexpr auto Color::from_trgb(trgb_type trgb) noexcept -> Color
{
    return Color(comp_type((trgb >> 16) & 0xFF),
                 comp_type((trgb >>  8) & 0xFF),
                 comp_type((trgb >>  0) & 0xFF),
                 comp_type(255 - ((trgb >> 24) & 0xFF)));
}


constexpr auto Color::to_rgba() const noexcept -> rgba_type
{
    return ((rgba_type(m_rgba[0]) << 24) |
            (rgba_type(m_rgba[1]) << 16) |
            (rgba_type(m_rgba[2]) <<  8) |
            (rgba_type(m_rgba[3]) <<  0));
}


constexpr auto Color::to_trgb() const noexcept -> trgb_type
{
    return ((trgb_type(255 - m_rgba[3]) << 24) |
            (trgb_type(m_rgba[0])       << 16) |
            (trgb_type(m_rgba[1])       <<  8) |
            (trgb_type(m_rgba[2])       <<  0));
}


template<class T> constexpr void Color::to_compr_vec(math::Vector<4, T>& rgba) const noexcept
{
    static_assert(std::is_floating_point_v<T>);
    for (int i = 0; i < 4; ++i)
        rgba[i] = util::color_comp_int_to_float<8, T>(m_rgba[i]);
}


template<class T> constexpr auto Color::from_compr_vec(const math::Vector<3, T>& rgb, T a) noexcept -> Color
{
    return from_compr_vec(math::Vector<4, T>(rgb, a));
}


template<class T> constexpr auto Color::from_compr_vec(const math::Vector<4, T>& rgba) noexcept -> Color
{
    static_assert(std::is_floating_point_v<T>);
    Color color;
    for (int i = 0; i < 4; ++i)
        color[i] = util::color_comp_float_to_int<comp_type, 8>(rgba[i]);
    return color;
}


constexpr bool Color::is_opaque() const noexcept
{
    return (alpha() == 255);
}


constexpr bool Color::is_gray() const noexcept
{
    return (red() == green() && red() == blue());
}


constexpr auto Color::red() const noexcept -> comp_type
{
    return m_rgba[0];
}


constexpr auto Color::green() const noexcept -> comp_type
{
    return m_rgba[1];
}


constexpr auto Color::blue() const noexcept -> comp_type
{
    return m_rgba[2];
}


constexpr auto Color::alpha() const noexcept -> comp_type
{
    return m_rgba[3];
}


constexpr auto Color::operator[](std::size_t i) noexcept -> comp_type&
{
    return m_rgba[i];
}


constexpr auto Color::operator[](std::size_t i) const noexcept -> const comp_type&
{
    return m_rgba[i];
}


constexpr auto Color::data() noexcept -> comp_type*
{
    return m_rgba.data();
}


constexpr auto Color::data() const noexcept -> const comp_type*
{
    return m_rgba.data();
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_COLOR_HPP
