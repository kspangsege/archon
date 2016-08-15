/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_UTIL_PACKED_TRGB_HPP
#define ARCHON_UTIL_PACKED_TRGB_HPP

#include <cstdint>
#include <cmath>
#include <limits>
#include <locale>
#include <string>
#include <ios>

#include <archon/core/text.hpp>
#include <archon/math/vector.hpp>
#include <archon/util/unit_frac.hpp>


namespace archon {
namespace util {

/// Special packed 32-bit TRGB format.
///
/// Colors are stored in a single integer, and the format has the form
/// <tt>0xTTRRGGBB</tt>, where \c TT is the transparency component whose value
/// is 255 minus the corresponding alpha component. This makes it easier to
/// specify RGB colors with full opacity. Note that all four channels are packed
/// into the 32 least significal bits, and that each channel uses 8 bits.
///
/// This class also allows for easy stream I/O of colors expressed according to
/// CSS3 (Cascading Style Sheets Level 3 Specification) from W3C (World Wide Web
/// Consortium). In fact one can choose between several levels when parsing and
/// formatting color values. The default is 'css3_ext' which is also used by the
/// stream I/O operators.
///
/// \sa http://www.w3.org/TR/2010/PR-css3-color-20101028/
///
/// \todo FIXME: The CSS style formatting and parsing should be separated into
/// its own file "css_color.hpp".
class PackedTRGB {
public:
    using value_type = std::uint_fast32_t;

    /// Full opacity black by default.
    explicit constexpr PackedTRGB(value_type packed_trgb = 0);

    /// Each component is automatically clamped to a value between 0 and 255.
    constexpr PackedTRGB(int red, int green, int blue, int alpha = 255);

    constexpr PackedTRGB(float r, float g, float b, float a = 1);

    int get_red() const;
    int get_green() const;
    int get_blue() const;
    int get_alpha() const;

    explicit operator value_type() const;

    value_type value() const;

    bool operator==(const PackedTRGB&) const;
    bool operator!=(const PackedTRGB&) const;

    static PackedTRGB pack_rgba(const unsigned char* rgba);

    void unpack_rgba(unsigned char* rgba) const;

    void unpack_rgba(Math::Vec4F& rgba) const;

    enum CssLevel {
        css21,    ///< Level 2 Revision 1
        css3,     ///< Level 3
        css3_ext  ///< Level 3 plus extended hex notation for RGBA
    };

    /// Returns 0 if parsing is unsuccessful, 1 if a named color was found, 2 if
    /// the 'hash mark triplet' form was found, 3 if the RGB functional form was
    /// found and it used absolute numbers, or 4 if that functional form used
    /// percentages. Values 5 and 6 are equivalent to values 3 and 4
    /// respectively except this time the colorspace is HSL.
    ///
    /// If 1 is returned, then the index of the named color is assigned to \a
    /// named_index, and \a color is unmodified. If 2 or 3 is returned, then the
    /// \a named_index is unmodified, and the first three components of \a color
    /// are set to integers in the range [0;256] representing red, green, and
    /// blue, and the fourth component of \a color is set to the alpha value in
    /// the range [0;1]. If 4 is returned, then the \a named_index is
    /// unmodified, and the first three components of \a color are set to
    /// floating point values in the range [0;100] representing red, green, and
    /// blue, and the fourth component of \a color is set to the alpha value in
    /// the range [0;1]. Again, values 5 and 6 are similar to 3 and 4 except
    /// that \a color is now expressed in the HSL color space.
    static int parse(std::string, int& named_index, Math::Vec4F& color,
                     CssLevel css_level = css3_ext);

    /// The specified type may be 2, 3, 4, 5 or 6, and these values have the
    /// same meaning as they have for parse(). Also, these values assign the
    /// same meaning to the components of \a color as they do for parse().
    ///
    /// The returned string is guaranteed to use characters from the portable
    /// character set only. See \ref CharEnc.
    static std::string format(int type, const Math::Vec4F& color, CssLevel css_level = css3_ext);

    template<class C, class T>
    friend std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T>&, const PackedTRGB&);

    template<class C, class T>
    friend std::basic_istream<C,T>& operator>>(std::basic_istream<C,T>&, PackedTRGB&);

    static PackedTRGB get_named_color(int index);

    /// Names are guaranteed to use characters from the portable character set
    /// only. See \ref CharEnc.
    static std::string get_color_name(int index);

private:
    static constexpr value_type pack_rgba(int red, int green, int blue, int alpha = 255);

    static unsigned uint_red(value_type trgb);
    static unsigned uint_green(value_type trgb);
    static unsigned uint_blue(value_type trgb);
    static unsigned uint_alpha(value_type trgb);

    std::string format2(CssLevel css_level, bool uppercase_hex, bool uppercase_names) const;

    static bool parse2(std::string str, PackedTRGB &trgb, CssLevel css_level);

    value_type m_value = 0; // Full opacity black
};




// Implementation

constexpr PackedTRGB::PackedTRGB(value_type packed_trgb):
    m_value(packed_trgb)
{
}

constexpr PackedTRGB::PackedTRGB(int r, int g, int b, int a):
    m_value(pack_rgba(r,g,b,a))
{
}

constexpr PackedTRGB::PackedTRGB(float r, float g, float b, float a):
    m_value(pack_rgba(std::floor(r*255 + .5),
                      std::floor(g*255 + .5),
                      std::floor(b*255 + .5),
                      std::floor(a*255 + .5)))
{
}

inline int PackedTRGB::get_red() const
{
    return uint_red(m_value);
}

inline int PackedTRGB::get_green() const
{
    return uint_green(m_value);
}

inline int PackedTRGB::get_blue() const
{
    return uint_blue(m_value);
}

inline int PackedTRGB::get_alpha() const
{
    return uint_alpha(m_value);
}

inline PackedTRGB::operator value_type() const
{
    return m_value;
}

inline PackedTRGB::value_type PackedTRGB::value() const
{
    return m_value;
}

inline bool PackedTRGB::operator==(const PackedTRGB& trgb) const
{
    return m_value == trgb.m_value;
}

inline bool PackedTRGB::operator!=(const PackedTRGB& trgb) const
{
    return m_value != trgb.m_value;
}

inline constexpr PackedTRGB::value_type PackedTRGB::pack_rgba(int r, int g, int b, int a)
{
    return
        value_type(       unsigned(core::clamp(r, 0, 255))) << 16 |
        value_type(       unsigned(core::clamp(g, 0, 255))) <<  8 |
        value_type(       unsigned(core::clamp(b, 0, 255)))       |
        value_type(255u - unsigned(core::clamp(a, 0, 255))) << 24;
}


inline PackedTRGB PackedTRGB::pack_rgba(const unsigned char* b)
{
    int n = std::numeric_limits<unsigned char>::digits;
    value_type value =
        value_type(       util::frac_adjust_bit_width<unsigned>(b[0], n, 8)) << 16 |
        value_type(       util::frac_adjust_bit_width<unsigned>(b[1], n, 8)) <<  8 |
        value_type(       util::frac_adjust_bit_width<unsigned>(b[2], n, 8))       |
        value_type(255u - util::frac_adjust_bit_width<unsigned>(b[3], n, 8)) << 24;
    return PackedTRGB(value);
}


// FIXME: frac_adjust_bit_width() assumes a very specific integer
// representation of a fraction of unity. Is this representation
// compatible with the one used by OpenGL and CSS where
// float_to_int(f) = round(f*max_int) and int_to_float(i) =
// i/max_int? That is, does float_to_int(int_to_float(i)) amount
// to bit truncation when the target integer is narrower than the
// source integer, and does it amount to bit replication when the
// target integer is wider?
//
// PARTIAL CLARITY: The general task is to find the best way in
// which we can represent fractions of unit by a range of
// integers. There are two main strategies to choose between: If
// we have at our disposal the N integers in the range [0;N-1],
// then we can either divide the unit interval into N sections of
// equal size, and have the integer i represent the (i+1)'th
// section, or we could instead divide the unit interval into N-1
// sections of equal size, and have the integer i represent the
// point where the i'th and the (i+1)'th section meet. If we are
// only interested in the optimal resolution of the unit interval,
// and we do not assign any special significance to the two
// endpoint of the unit interval (0.0 and 1.0), then the former
// strategy seems to be the best one. The main drawback is that
// the integer 0 does not naturally convert to the fractional
// value 0.0, and likewise, N-1 does not naturally convert to the
// fractional value 1.0. If it is important for us, that 0
// converts to 0.0 and N-1 to 1.0, then the second strategy is
// generally better. An example of the latter case is integer
// representations of color components. It is possible to use a
// hybrid approach where one divides into N sections, but use a
// sliding representative in each section, but then it becomes
// impossible to handle fractional values outside the unit
// interval, which for example is often desirable for color
// components.
//
// A PARTIAL CONCLUSIN: It is probably not correct to use
// frac_adjust_bit_width() in pack_rgba() as long is it is implemented as bit
// truncation, but I'm not sure. The real question here is: Does a narrowing of
// the bit width of the latter kind of integer representation amount to bit
// truncation?
//
// CONVERSIONS IN FORMER APPROACH: int_to_float(i) = (i+0.5) / N;
// float_to_int(f) = 0<f ? ceil(f*N)-1 : floor(f*N).
//
// CONVERSIONS IN LATTER APPROACH: int_to_float(i) =
// i/double(N-1); float_to_int(f) = floor(f*(N-1) + 0.5).
//
// HOW CAN WE DEFINE THE COMBINED frac_adjust_bit_width() IN EACH
// OF THE TWO APPROACHES?
//
inline void PackedTRGB::unpack_rgba(unsigned char* b) const
{
    int n = std::numeric_limits<unsigned char>::digits;
    b[0] = util::frac_adjust_bit_width(uint_red(m_value),   8, n);
    b[1] = util::frac_adjust_bit_width(uint_green(m_value), 8, n);
    b[2] = util::frac_adjust_bit_width(uint_blue(m_value),  8, n);
    b[3] = util::frac_adjust_bit_width(uint_alpha(m_value), 8, n);
}


inline void PackedTRGB::unpack_rgba(Math::Vec4F& rgba) const
{
    rgba[0] = uint_red(m_value)   * (1.0 / 255);
    rgba[1] = uint_green(m_value) * (1.0 / 255);
    rgba[2] = uint_blue(m_value)  * (1.0 / 255);
    rgba[3] = uint_alpha(m_value) * (1.0 / 255);
}


inline unsigned PackedTRGB::uint_red(value_type trgb)
{
    return unsigned(trgb>>16) & 0xFFu;
}

inline unsigned PackedTRGB::uint_green(value_type trgb)
{
    return unsigned(trgb>>8) & 0xFFu;
}

inline unsigned PackedTRGB::uint_blue(value_type trgb)
{
    return unsigned(trgb) & 0xFFu;
}

inline unsigned PackedTRGB::uint_alpha(value_type trgb)
{
    return 255u - (unsigned(trgb>>24) & 0xFFu);
}



template<class Ch, class Tr>
inline std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& out,
                                              const PackedTRGB& trgb)
{
    bool uppercase_hex = out.flags() | std::ios_base::uppercase;
    bool uppercase_names = false;
    return out << core::Text::widen_port<Ch>(trgb.format2(PackedTRGB::css3_ext, uppercase_hex,
                                                          uppercase_names), out.getloc());
}


template<class Ch, class Tr>
inline std::basic_istream<Ch, Tr>& operator>>(std::basic_istream<Ch, Tr>& in,
                                              PackedTRGB& trgb)
{
    if (in.bad() || in.fail())
        return in;
    std::ctype<Ch> const &ctype = std::use_facet<std::ctype<Ch> >(in.getloc());
    Ch hash(ctype.widen('#')), lpar(ctype.widen('(')), rpar(ctype.widen(')'));
    std::basic_string<Ch> str;
    int parenth_level = 0;
    for (;;) {
        Ch ch;
        // Allow white-spaces to be skipped when stream is configured
        // that way
        if (str.empty()) {
            in >> ch;
        }
        else {
            in.get(ch);
        }
        if (!in) {
            if (in.bad())
                return in;
            in.clear(in.rdstate() & ~std::ios_base::failbit);
            break;
        }
        if (!ctype.is(std::ctype_base::alnum, ch) && ch != hash) {
            if (0 < parenth_level) {
                if (ch == rpar)
                    --parenth_level;
            }
            else {
                if (ch != lpar) {
                    in.unget();
                    break;
                }
                ++parenth_level;
            }
        }
        str += ch;
    }

    std::string str2;
    if (!core::Text::narrow_port(str, str2, in.getloc()) ||
        !PackedTRGB::parse2(str2, trgb, PackedTRGB::css3_ext))
        in.setstate(std::ios_base::badbit);
    return in;
}

} // namespace util
} // namespace archon

#endif // ARCHON_UTIL_PACKED_TRGB_HPP
