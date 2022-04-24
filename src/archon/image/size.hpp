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

#ifndef ARCHON_X_IMAGE_X_SIZE_HPP
#define ARCHON_X_IMAGE_X_SIZE_HPP

/// \file


#include <cstddef>
#include <utility>
#include <algorithm>
#include <array>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>


namespace archon::image {


/// \brief Size in pixels along X and Y axes.
///
/// This class offers a 2-dimensional integer size. The \ref width and \ref height
/// components are expressed in number of pixels along the X and Y axes respectively. The
/// X-axis is the horizontal axis, and the Y-axis is the vertical axis.
///
/// Sizes of this type are comparable. Comparison is lexicographical.
///
/// Sizes can be formatted (written to an output stream), and can be parsed through a value
/// parser (\ref core::BasicValueParserSource).
///
/// When a size is formatted, if the two components are equal, only one component is
/// shown. For example, the size `{ 16, 16 }` is formatted as just `16`. When the two
/// components are different, both components are shown and are separated by a comma `,`. No
/// space will be included after the comma. For example, the size `{ 16, 17 }` is formatted
/// as `16,17`.
///
/// When a size is parsed, if there is only one value, that value is used for both
/// components. If there are two value, they must be separated by a comma `,`. Space is
/// allowed between the comma and the second component.
///
struct Size {
    /// \brief Components of size.
    ///
    /// These are the components of the size. `width` is the horizontal component, and
    /// `height` is the vertical component. See class-level documentation for details.
    ///
    int width, height;

    /// \{
    ///
    /// \brief Construct a size.
    ///
    /// The two-argument constructor constructs a size with the specified components (\p
    /// width and \p height).
    ///
    /// `Size(size)` is a shorthand for `Size(size, size)`.
    ///
    constexpr Size(int = 0) noexcept;
    constexpr Size(int width, int height) noexcept;
    /// \}

    /// \brief Whether both components are zero.
    ///
    /// This function returns `true` if both width and height is zero. Otherwise, it returns
    /// `false`.
    ///
    constexpr bool is_zero() const noexcept;

    /// \brief Whether box of this size is empty.
    ///
    /// This function returns `true` if either width or height is zero. Otherwise, it
    /// returns `false`. Therefore, if both components are non-negative, this function
    /// returns `false` precisely when a box of this size would contain at least one pixel.
    ///
    constexpr bool is_empty() const noexcept;

    /// \brief Whether box of this size can be contained in box of other size.
    ///
    /// This function returns `true` if a box of this size can be contained in the a box of
    /// the specified size (\p other). Otherwise, it returns `false`.
    ///
    constexpr bool contained_in(const Size& other) const noexcept;

    /// \{
    ///
    /// \brief Compare two boxes.
    ///
    /// These operators compare this size with the specified size (\p other). Comparison
    /// happens lexicographically on \ref width followed by \ref height.
    ///
    constexpr bool operator==(Size other) const noexcept;
    constexpr bool operator!=(Size other) const noexcept;
    constexpr bool operator< (Size other) const noexcept;
    constexpr bool operator<=(Size other) const noexcept;
    constexpr bool operator> (Size other) const noexcept;
    constexpr bool operator>=(Size other) const noexcept;
    /// \}

    /// \{
    ///
    /// \brief Addition and subtraction of sizes.
    ///
    /// These functions compute the sum and difference, respectively, of this size and the
    /// specified size (\p other).
    ///
    constexpr auto operator+(Size other) const noexcept -> Size;
    constexpr auto operator-(Size other) const noexcept -> Size;
    /// \}

    /// \{
    ///
    /// \brief Add size to, and subtract size from this size.
    ///
    /// These functions respectively add the specified size (\p other) to, and subtract the
    /// specified size from this size.
    ///
    constexpr auto operator+=(Size other) noexcept -> Size&;
    constexpr auto operator-=(Size other) noexcept -> Size&;
    /// \}

    /// \{
    ///
    /// \brief Projection of size onto X-axis or Y-axis.
    ///
    /// These functions return the projection of this size onto the X-axis and Y-axis
    /// respectively.
    ///
    constexpr auto proj_x() const noexcept -> Size;
    constexpr auto proj_y() const noexcept -> Size;
    /// \}

    /// \{
    ///
    /// \brief New size with one component replaced.
    ///
    /// These functions construct a new size with the width and height components
    /// respectively replaced with the specified value (\p width and \p height).
    ///
    constexpr auto with_width(int width) const noexcept -> Size;
    constexpr auto with_height(int height) const noexcept -> Size;
    /// \}
};


/// \brief Scale size by integer factor.
///
/// This function returns the specified size (\p size) scaled by the specified integer
/// factor (\p f).
///
constexpr auto operator*(int factor, image::Size size) noexcept -> image::Size;


/// \{
///
/// \brief Component-wise minimum or maximum of two sizes.
///
/// These function compute the component-wise minimum and maximum of the two specified sizes
/// (\p a and \p b).
///
constexpr auto min(image::Size a, image::Size b) noexcept -> image::Size;
constexpr auto max(image::Size a, image::Size b) noexcept -> image::Size;
/// \}


/// \brief Write textual representation of size to output stream.
///
/// This stream output operator writes a textual representation of the specified size (\p
/// size) to the specified output stream (\p out). See \ref image::Size for information on
/// the format of the textual representation.
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>& out, image::Size size) ->
    std::basic_ostream<C, T>&;


/// \brief Read textual representation of size from source.
///
/// This function reads a textual representation of a size (\p size) from the specified
/// value parser source (\p src). See \ref image::Size for information on the format of the
/// textual representation. This function is intended to be invoked by a value parser, see
/// \ref core::BasicValueParser for more information.
///
template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>& src, image::Size& size);


/// \brief Combine width from one size with height from other size.
///
/// This function constructs a new size with the width component taken from the first
/// argument (\p x) and the height component taken from the second argument (\p y).
///
auto splice(image::Size x, image::Size y) noexcept -> image::Size;








// Implementation


constexpr Size::Size(int val) noexcept
    : width(val)
    , height(val)
{
}


constexpr Size::Size(int width_2, int height_2) noexcept
    : width(width_2)
    , height(height_2)
{
}


constexpr bool Size::is_zero() const noexcept
{
    return (width == 0 && height == 0);
}


constexpr bool Size::is_empty() const noexcept
{
    return (width == 0 || height == 0);
}


constexpr bool Size::contained_in(const Size& other) const noexcept
{
    return (width <= other.width && height <= other.height);
}


constexpr bool Size::operator==(Size other) const noexcept
{
    return (width == other.width && height == other.height);
}


constexpr bool Size::operator!=(Size other) const noexcept
{
    return !(*this == other);
}


constexpr bool Size::operator<(Size other) const noexcept
{
    return (width < other.width || (width == other.width && height < other.height));
}


constexpr bool Size::operator<=(Size other) const noexcept
{
    return !(*this > other);
}


constexpr bool Size::operator>(Size other) const noexcept
{
    return (other < *this);
}


constexpr bool Size::operator>=(Size other) const noexcept
{
    return  !(*this < other);
}


constexpr auto Size::operator+(Size other) const noexcept -> Size
{
    return {
        width + other.width,
        height + other.height,
    };
}


constexpr auto Size::operator-(Size other) const noexcept -> Size
{
    return {
        width - other.width,
        height - other.height,
    };
}


constexpr auto Size::operator+=(Size other) noexcept -> Size&
{
    return *this = *this + other;
}


constexpr auto Size::operator-=(Size other) noexcept -> Size&
{
    return *this = *this - other;
}


constexpr auto Size::proj_x() const noexcept -> Size
{
    return { width, 0 };
}


constexpr auto Size::proj_y() const noexcept -> Size
{
    return { 0, height };
}


constexpr auto Size::with_width(int width_2) const noexcept -> Size
{
    return { width_2, height };
}


constexpr auto Size::with_height(int height_2) const noexcept -> Size
{
    return { width, height_2 };
}


constexpr auto operator*(int factor, image::Size size) noexcept -> image::Size
{
    return {
        factor * size.width,
        factor * size.height,
    };
}


constexpr auto min(image::Size a, image::Size b) noexcept -> image::Size
{
    return {
        std::min(a.width, b.width),
        std::min(a.height, b.height),
    };
}


constexpr auto max(image::Size a, image::Size b) noexcept -> image::Size
{
    return {
        std::max(a.width, b.width),
        std::max(a.height, b.height),
    };
}


template<class C, class T>
inline auto operator<<(std::basic_ostream<C, T>& out, image::Size size) -> std::basic_ostream<C, T>&
{
    std::array<int, 2> components = { size.width, size.height };
    std::size_t min_elems = 1;
    bool copy_last = true;
    auto func = [](const int& val) {
        return core::as_int(val);
    };
    core::AsListConfig config;
    config.space = core::AsListSpace::allow;
    return out << core::as_list_a(components, min_elems, copy_last,
                                  std::move(func), std::move(config)); // Throws
}


template<class C, class T> inline bool parse_value(core::BasicValueParserSource<C, T>& src, image::Size& size)
{
    std::array<int, 2> components = {};
    std::size_t min_elems = 1;
    bool copy_last = true;
    auto func = [](int& val) {
        return core::as_int(val);
    };
    core::AsListConfig config;
    config.space = core::AsListSpace::allow;
    bool success = src.delegate(core::as_list_a(components, min_elems, copy_last, std::move(func),
                                                std::move(config))); // Throws
    if (ARCHON_LIKELY(success)) {
        size = { components[0], components[1] };
        return true;
    }
    return false;
}


inline auto splice(image::Size x, image::Size y) noexcept -> image::Size
{
    return { x.width, y.height };
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_SIZE_HPP
