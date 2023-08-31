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

#ifndef ARCHON_X_UTIL_X_PIXEL_POS_HPP
#define ARCHON_X_UTIL_X_PIXEL_POS_HPP

/// \file


#include <cstddef>
#include <utility>
#include <array>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>
#include <archon/util/pixel_size.hpp>


namespace archon::util::pixel {


/// \brief Pixel position on X and Y axes.
///
/// This class offers a 2-dimensional integer position. The \ref x and \ref y components are
/// expressed in number of pixels along the X and Y axes respectively. The X-axis is the
/// horizontal axis, and the X-coordinate increases to the right. The Y-axis is the vertical
/// axis, and the Y-coordinate increases downwards.
///
/// Positions of this type are comparable. Comparison is lexicographical.
///
/// FIXME: Mention subtraction and addition operations                  
///
/// Positions can be formatted (written to an output stream), and can be parsed through a
/// value parser (\ref core::BasicValueParserSource).
///
/// When a position is formatted, the two components are separated by a comma `,`. No space
/// will be included after the comma. For example, the position `{ 16, 16 }` is formatted as
/// `16,16`.
///
/// When a position is parsed, the two components must be separated by a comma `,`. Space is
/// allowed between the comma and the second component.
///
struct Pos {
    /// \brief Components of position.
    ///
    /// These are the components of the position. `x` is the horizontal component, and `y`
    /// is the vertical component. See class-level documentation for details.
    ///
    int x, y;

    /// \{
    ///
    /// \brief Construct a size.
    ///
    /// The two-argument constructor constructs a position with the specified components (\p
    /// x and \p y).
    ///
    /// `Pos()` is a shorthand for `Pos(0, 0)`.
    ///
    constexpr Pos() noexcept;
    constexpr Pos(int x, int y) noexcept;
    /// \}

    /// \brief Whether both components are zero.
    ///
    /// This function returns `true` if both components (X and Y) are zero. Otherwise, it
    /// returns `false`.
    ///
    constexpr bool is_zero() const noexcept;

    /// \{
    ///
    /// \brief Compare two boxes.
    ///
    /// These operators compare this position with the specified position (\p
    /// other). Comparison happens lexicographically on \ref x followed by \ref y.
    ///
    constexpr bool operator==(Pos other) const noexcept;
    constexpr bool operator!=(Pos other) const noexcept;
    constexpr bool operator< (Pos other) const noexcept;
    constexpr bool operator<=(Pos other) const noexcept;
    constexpr bool operator> (Pos other) const noexcept;
    constexpr bool operator>=(Pos other) const noexcept;
    /// \}

    /// \brief Difference between positions as size.
    ///
    /// This function computes the difference between this position and the specified
    /// position (\p other). The difference is returned in the form of a size (\ref
    /// pixel::Size).
    ///
    constexpr auto operator-(Pos other) const noexcept -> pixel::Size;

    /// \{
    ///
    /// \brief Result of adding size to, or subtracting size from this position.
    ///
    /// These functions return the results of adding a size (\p size) to, and subtracting a
    /// size from this position.
    ///
    constexpr auto operator+(pixel::Size size) const noexcept -> Pos;
    constexpr auto operator-(pixel::Size size) const noexcept -> Pos;
    /// \}

    /// \{
    ///
    /// \brief Add size to, or subtract size from this position.
    ///
    /// These functions respectively add the specified size (\p size) to, and subtract the
    /// specified size from this position.
    ///
    constexpr auto operator+=(pixel::Size size) noexcept -> Pos&;
    constexpr auto operator-=(pixel::Size size) noexcept -> Pos&;
    /// \}

    /// \{
    ///
    /// \brief Projection of position onto X-axis or Y-axis.
    ///
    /// These functions return the projection of this position onto the X-axis and Y-axis
    /// respectively.
    ///
    constexpr auto proj_x() const noexcept -> Pos;
    constexpr auto proj_y() const noexcept -> Pos;
    /// \}

    /// \{
    ///
    /// \brief New position with one coordinate replaced.
    ///
    /// These functions construct a new position with the X and Y coordinates respectively
    /// replaced with the specified value (\p x and \p y).
    ///
    constexpr auto with_x(int x) const noexcept -> Pos;
    constexpr auto with_y(int y) const noexcept -> Pos;
    /// \}
};


/// \brief Write textual representation of position to output stream.
///
/// This stream output operator writes a textual representation of the specified position
/// (\p pos) to the specified output stream (\p out). See \ref pixel::Pos for information on
/// the format of the textual representation.
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>& out, pixel::Pos pos) -> std::basic_ostream<C, T>&;


/// \brief Read textual representation of position from source.
///
/// This function reads a textual representation of a position (\p pos) from the specified
/// value parser source (\p src). See \ref pixel::Pos for information on the format of the
/// textual representation. This function is intended to be invoked by a value parser, see
/// \ref core::BasicValueParser for more information.
///
template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>& src, pixel::Pos& pos);


/// \brief Combine X coordinate from one position with Y coordinate from other position.
///
/// This function constructs a new position with the X coordinate taken from the first
/// argument (\p x) and the Y coordinate taken from the second argument (\p y).
///
auto splice(pixel::Pos x, pixel::Pos y) noexcept -> pixel::Pos;








// Implementation


constexpr Pos::Pos() noexcept
    : Pos(0, 0)
{
}


constexpr Pos::Pos(int x_2, int y_2) noexcept
    : x(x_2)
    , y(y_2)
{
}


constexpr bool Pos::is_zero() const noexcept
{
    return (x == 0 && y == 0);
}


constexpr bool Pos::operator==(Pos other) const noexcept
{
    return (x == other.x && y == other.y);
}


constexpr bool Pos::operator!=(Pos other) const noexcept
{
    return !(*this == other);
}


constexpr bool Pos::operator<(Pos other) const noexcept
{
    return (x < other.x || (x == other.x && y < other.y));
}


constexpr bool Pos::operator<=(Pos other) const noexcept
{
    return !(*this > other);
}


constexpr bool Pos::operator>(Pos other) const noexcept
{
    return (other < *this);
}


constexpr bool Pos::operator>=(Pos other) const noexcept
{
    return  !(*this < other);
}


constexpr auto Pos::operator-(Pos other) const noexcept -> pixel::Size
{
    return { x - other.x, y - other.y };
}


constexpr auto Pos::operator+(pixel::Size size) const noexcept -> Pos
{
    return { x + size.width, y + size.height };
}


constexpr auto Pos::operator-(pixel::Size size) const noexcept -> Pos
{
    return { x - size.width, y - size.height };
}


constexpr auto Pos::operator+=(pixel::Size size) noexcept -> Pos&
{
    return *this = *this + size;
}


constexpr auto Pos::operator-=(pixel::Size size) noexcept -> Pos&
{
    return *this = *this - size;
}


constexpr auto Pos::proj_x() const noexcept -> Pos
{
    return { x, 0 };
}


constexpr auto Pos::proj_y() const noexcept -> Pos
{
    return { 0, y };
}


constexpr auto Pos::with_x(int x_2) const noexcept -> Pos
{
    return { x_2, y };
}


constexpr auto Pos::with_y(int y_2) const noexcept -> Pos
{
    return { x, y_2 };
}


template<class C, class T>
inline auto operator<<(std::basic_ostream<C, T>& out, pixel::Pos pos) -> std::basic_ostream<C, T>&
{
    std::array<int, 2> components = { pos.x, pos.y };
    auto func = [](const int& val) {
        return core::as_int(val);
    };
    core::AsListConfig config;
    config.space = core::AsListSpace::allow;
    return out << core::as_list(components, std::move(func), std::move(config)); // Throws
}


template<class C, class T> inline bool parse_value(core::BasicValueParserSource<C, T>& src, pixel::Pos& pos)
{
    std::array<int, 2> components = {};
    auto func = [](int& val) {
        return core::as_int(val);
    };
    core::AsListConfig config;
    config.space = core::AsListSpace::allow;
    bool success = src.delegate(core::as_list(components, std::move(func), std::move(config))); // Throws
    if (ARCHON_LIKELY(success)) {
        pos = { components[0], components[1] };
        return true;
    }
    return false;
}


inline auto splice(pixel::Pos x, pixel::Pos y) noexcept -> pixel::Pos
{
    return { x.x, y.y };
}


} // namespace archon::util::pixel

#endif // ARCHON_X_UTIL_X_PIXEL_POS_HPP
