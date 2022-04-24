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

#ifndef ARCHON_X_UTIL_X_CSS_COLOR_HPP
#define ARCHON_X_UTIL_X_CSS_COLOR_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <utility>
#include <string_view>

#include <archon/core/string_span.hpp>
#include <archon/core/buffer.hpp>


namespace archon::util {


/// \brief CSS color.
///
/// An instance of this class is a specification of a particular color and transparency
/// level. The specification can take any of the forms allowed by CSS Level 3.
///
/// RGB color space is red, green, and blue. HSL color space is hue, saturation, and
/// lightness.
///
/// See https://www.w3.org/TR/css-color-3/.
///
class CssColor {
public:
    /// \brief Default construct CSS color.
    ///
    /// A default constructed CSS color is `#00000000`, which means that it is black, fully
    /// transparent, and is expressed in the "hex" form.
    ///
    CssColor() noexcept;

    using int_comp_type = std::uint_least8_t;
    using flt_comp_type = float;

    static auto hex(int_comp_type r, int_comp_type g, int_comp_type b, int_comp_type a) noexcept   -> CssColor;
    static auto name(std::size_t index)                                                            -> CssColor;
    static auto rgb(flt_comp_type r, flt_comp_type g, flt_comp_type b, flt_comp_type a) noexcept   -> CssColor;
    static auto rgb_p(flt_comp_type r, flt_comp_type g, flt_comp_type b, flt_comp_type a) noexcept -> CssColor;
    static auto hsl(flt_comp_type h, flt_comp_type s, flt_comp_type l, flt_comp_type a) noexcept   -> CssColor;

    struct Hex {
        int_comp_type r, g, b, a;
    };

    struct Name {
        std::size_t index;
    };

    struct Rgb {
        flt_comp_type r, g, b, a;
    };

    struct RgbP {
        flt_comp_type r, g, b, a;
    };

    struct Hsl {
        flt_comp_type h, s, l, a;
    };

    CssColor(Hex) noexcept;
    CssColor(Name) noexcept;
    CssColor(Rgb) noexcept;
    CssColor(RgbP) noexcept;
    CssColor(Hsl) noexcept;

    enum class Form {
        hex,
        name,
        rgb,
        rgb_p,
        hsl,
    };

    auto form() const noexcept -> Form;

    bool get_if(Hex&) const noexcept;
    bool get_if(Name&) const noexcept;
    bool get_if(Rgb&) const noexcept;
    bool get_if(RgbP&) const noexcept;
    bool get_if(Hsl&) const noexcept;

    auto get_as_hex() const -> Hex;

    static auto get_named_color(const Name&) noexcept -> Hex;
    static bool find_named_color_by_name(std::string_view name, Name&) noexcept;
    static bool find_named_color_by_value(const Hex&, Name&) noexcept;
    static auto get_num_named_colors() noexcept -> std::size_t;

    enum class CssLevel {
        css21,    ///< CSS Level 2 Revision 1
        css3,     ///< CSS Level 3
        css3_ext  ///< CSS Level 3 plus extended hex notation for RGBA
    };

    struct FormatConfig {
        CssLevel css_level = CssLevel::css3_ext;
        bool disable_short_hex_form = false;
    };

    auto format(core::Buffer<char>&) const -> std::string_view;
    auto format(core::Buffer<char>&, FormatConfig) const -> std::string_view;

    void format(core::Buffer<char>&, std::size_t& offset, FormatConfig) const;

    bool parse(core::StringSpan<char>, CssLevel = CssLevel::css3_ext);

private:
    Form m_form;

    union {
        Hex  m_hex;
        Name m_name;
        Rgb  m_rgb;
        RgbP m_rgb_p;
        Hsl  m_hsl;
    };
};



/// \brief 
///
///    
///
template<class D> auto as_css_color(D&& color);








// Implementation


inline CssColor::CssColor() noexcept :
    CssColor(Hex{})
{
}


inline auto CssColor::hex(int_comp_type r, int_comp_type g, int_comp_type b, int_comp_type a) noexcept -> CssColor
{
    return Hex { r, g, b, a };
}


inline auto CssColor::rgb(flt_comp_type r, flt_comp_type g, flt_comp_type b, flt_comp_type a) noexcept -> CssColor
{
    return Rgb { r, g, b, a };
}


inline auto CssColor::rgb_p(flt_comp_type r, flt_comp_type g, flt_comp_type b, flt_comp_type a) noexcept -> CssColor
{
    return RgbP { r, g, b, a };
}


inline auto CssColor::hsl(flt_comp_type h, flt_comp_type s, flt_comp_type l, flt_comp_type a) noexcept -> CssColor
{
    return Hsl { h, s, l, a };
}


inline CssColor::CssColor(Hex hex) noexcept
{
    m_form = Form::hex;
    m_hex = hex;
}


inline CssColor::CssColor(Name name) noexcept
{
    m_form = Form::name;
    m_name = name;
}


inline CssColor::CssColor(Rgb rgb) noexcept
{
    m_form = Form::rgb;
    m_rgb = rgb;
}


inline CssColor::CssColor(RgbP rgb_p) noexcept
{
    m_form = Form::rgb_p;
    m_rgb_p = rgb_p;
}


inline CssColor::CssColor(Hsl hsl) noexcept
{
    m_form = Form::hsl;
    m_hsl = hsl;
}


inline auto CssColor::form() const noexcept -> Form
{
    return m_form;
}


inline bool CssColor::get_if(Hex& hex) const noexcept
{
    if (m_form == Form::hex) {
        hex = m_hex;
        return true;
    }
    return false;
}


inline bool CssColor::get_if(Name& name) const noexcept
{
    if (m_form == Form::name) {
        name = m_name;
        return true;
    }
    return false;
}


inline bool CssColor::get_if(Rgb& rgb) const noexcept
{
    if (m_form == Form::rgb) {
        rgb = m_rgb;
        return true;
    }
    return false;
}


inline bool CssColor::get_if(RgbP& rgb_p) const noexcept
{
    if (m_form == Form::rgb_p) {
        rgb_p = m_rgb_p;
        return true;
    }
    return false;
}


inline bool CssColor::get_if(Hsl& hsl) const noexcept
{
    if (m_form == Form::hsl) {
        hsl = m_hsl;
        return true;
    }
    return false;
}


inline auto CssColor::format(core::Buffer<char>& buffer) const -> std::string_view
{
    return format(buffer, {}); // Throws
}


inline auto CssColor::format(core::Buffer<char>& buffer, FormatConfig config) const -> std::string_view
{
    std::size_t offset = 0;
    format(buffer, offset, std::move(config)); // Throws
    return { buffer.data(), offset };
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_CSS_COLOR_HPP
