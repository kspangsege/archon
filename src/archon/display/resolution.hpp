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

#ifndef ARCHON_X_DISPLAY_X_RESOLUTION_HPP
#define ARCHON_X_DISPLAY_X_RESOLUTION_HPP

/// \file


#include <cstddef>
#include <utility>
#include <array>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/value_parser.hpp>
#include <archon/core/with_modified_locale.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_list.hpp>


namespace archon::display {


/// \brief Physical resolution associated with viewport and attached monitor.
///
/// An object of this type specifies the horizontal and vertical resolutions in pixels per
/// centimeter. It is primarily intended to specify the resolution associated with a
/// viewport (\ref display::Viewport) and an attached monitor. A monitor needs to be
/// attached so that the physical dimensions of the pixels are known.
///
/// To get the resolution in pixels per inch, multiply by 2.54 cm/in.
///
/// A resolution object can be formatted, i.e., it can be written to an output stream. If
/// the two components are equal, only one component is shown. For example, the resolution
/// `{ 43, 43 }` is formatted as just `43`. When the two components are different, both
/// components are shown and are separated by a comma `,`. No space will be included after
/// the comma. For example, the resolution `{ 43, 47.5 }` is formatted as `43,47.5`. Note
/// that within the two components, a dot (`.`) is used as decimal point.
///
/// A resolution object can be parsed through a value parser (\ref
/// core::BasicValueParserSource). If the parsed string contains only one value, that value
/// is used for both components. If there are two values, they must be separated by a comma
/// `,`. Space is allowed between the comma and the second component.
///
/// \sa \ref display::Viewport
///
struct Resolution {
    /// \brief Pixels per centimeter in horizontal direction.
    ///
    /// This field specifies the number of pixels per centimeter in the horizontal
    /// direction.
    ///
    double horz_ppcm;

    /// \brief Pixels per centimeter in vertical direction.
    ///
    /// This field specifies the number of pixels per centimeter in the vertical direction.
    ///
    double vert_ppcm;

    /// \{
    ///
    /// \brief Construct a resolution.
    ///
    /// The two-argument constructor constructs a resolution with the specified components (\p
    /// width and \p height).
    ///
    /// `Resolution(ppcm)` is a shorthand for `Resolution(ppcm, ppcm)`.
    ///
    constexpr Resolution(double ppcm = 0) noexcept;
    constexpr Resolution(double horz_ppcm, double vert_ppcm) noexcept;
    /// \}

    /// \{
    ///
    /// \brief Compare two resolutions.
    ///
    /// These operators compare this resolution with the specified one (\p
    /// other). Comparison happens lexicographically on \ref horz_ppcm followed by \ref
    /// vert_ppcm.
    ///
    constexpr bool operator==(const Resolution& other) const noexcept;
    constexpr bool operator!=(const Resolution& other) const noexcept;
    constexpr bool operator< (const Resolution& other) const noexcept;
    constexpr bool operator<=(const Resolution& other) const noexcept;
    constexpr bool operator> (const Resolution& other) const noexcept;
    constexpr bool operator>=(const Resolution& other) const noexcept;
    /// \}
};


/// \brief Write textual representation of resolution to output stream.
///
/// This stream output operator writes a textual representation of the specified resolution
/// (\p resl) to the specified output stream (\p out). See \ref display::Resolution for
/// information on the format of the textual representation.
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>& out, const display::Resolution& resl) ->
    std::basic_ostream<C, T>&;


/// \brief Read textual representation of resolution from source.
///
/// This function reads a textual representation of a resolution object (\p resl) from the
/// specified value parser source (\p src). See \ref display::Resolution for information on
/// the format of the textual representation. This function is intended to be invoked by a
/// value parser, see \ref core::BasicValueParser for more information.
///
template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>& src, display::Resolution& resl);








// Implementation


constexpr Resolution::Resolution(double ppcm) noexcept
    : Resolution(ppcm, ppcm)
{
}


constexpr Resolution::Resolution(double horz_ppcm, double vert_ppcm) noexcept
{
    this->horz_ppcm = horz_ppcm;
    this->vert_ppcm = vert_ppcm;
}


constexpr bool Resolution::operator==(const Resolution& other) const noexcept
{
    return (horz_ppcm == other.horz_ppcm && vert_ppcm == other.vert_ppcm);
}


constexpr bool Resolution::operator!=(const Resolution& other) const noexcept
{
    return !(*this == other);
}


constexpr bool Resolution::operator<(const Resolution& other) const noexcept
{
    return (horz_ppcm < other.horz_ppcm || (horz_ppcm == other.horz_ppcm && vert_ppcm < other.vert_ppcm));
}


constexpr bool Resolution::operator<=(const Resolution& other) const noexcept
{
    return !(*this > other);
}


constexpr bool Resolution::operator>(const Resolution& other) const noexcept
{
    return (other < *this);
}


constexpr bool Resolution::operator>=(const Resolution& other) const noexcept
{
    return  !(*this < other);
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const display::Resolution& resl) -> std::basic_ostream<C, T>&
{
    if (ARCHON_LIKELY(resl.vert_ppcm == resl.horz_ppcm))
        return out << core::with_reverted_numerics(resl.horz_ppcm); // Throws
    return out << core::formatted_wrn("%s,%s", resl.horz_ppcm, resl.vert_ppcm); // Throws
}


template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>& src, display::Resolution& resl)
{
    std::array<double, 2> components = {};
    std::size_t min_elems = 1;
    bool copy_last = true;
    core::AsListConfig config;
    config.space = core::AsListSpace::allow;
    bool success = src.delegate(core::with_reverted_numerics(core::as_list_a(components, min_elems, copy_last,
                                                                             std::move(config)))); // Throws
    if (ARCHON_LIKELY(success)) {
        resl = { components[0], components[1] };
        return true;
    }
    return false;
}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_RESOLUTION_HPP
