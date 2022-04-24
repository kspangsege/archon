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

#ifndef ARCHON_X_IMAGE_X_BOX_HPP
#define ARCHON_X_IMAGE_X_BOX_HPP

/// \file


#include <algorithm>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/format.hpp>
#include <archon/image/size.hpp>
#include <archon/image/pos.hpp>


namespace archon::image {


/// \brief Rectangular area within pixel grid.
///
/// Objects of this type specify the position and size of a rectangular area within a grid
/// of pixels, possibly an image.
///
/// Boxes are comparable.
///
/// Boxes can be formatted as text which happens when they are written to an output
/// stream. The format is `<position>;<size>`.
///
struct Box {
    /// \brief Position of upper-left corner of box.
    ///
    /// This is the position of the upper-right corner of the rectangular area that is this
    /// box.
    ///
    image::Pos pos;

    /// \brief Size of box.
    ///
    /// This is the size of the rectangular area that is this box.
    ///
    image::Size size;

    /// \{
    ///
    /// \brief Construct a box.
    ///
    /// The two-argument constructor constructs a box with the specified position (\p pos)
    /// and size (\p size).
    ///
    /// `Box(size)` is a shorthand for `Box({}, size)`, which sets the position to 0,0.
    ///
    constexpr Box(image::Size size = 0) noexcept;
    constexpr Box(image::Pos pos, image::Size size) noexcept;
    /// \}

    /// \brief Whether box is empty.
    ///
    /// This function returns `false` if the box contains at least one pixel. Otherwise, it
    /// returns `true`.
    ///
    constexpr bool is_empty() const noexcept;

    /// \brief Whether this box is contained in other box.
    ///
    /// This function returns `true` if this box is contained in the specified box (\p
    /// other). Otherwise, it returns `false`.
    ///
    constexpr bool contained_in(const Box& other) const noexcept;

    /// \brief Clip specified box to this box.
    ///
    /// If this box intersects the specified box (\p other), and the intersection contains
    /// at least one pixel, then this function returns `true` after setting \p other to the
    /// intersection, which is then a nonempty box that is contained in both this box and in
    /// the specified box (original value of \p other).
    ///
    /// If this box does not intersect the specified box (\p other), or if the intersection
    /// contains no pixels, this function returns `false` and leaves \p other unchanged.
    ///
    /// Not that here, *clipping a box* means reducing it to the intersection between it and
    /// the box it is clipped against.
    ///
    constexpr bool clip(Box& other) const noexcept;

    /// \{
    ///
    /// \brief Compare two boxes.
    ///
    /// These operators compare this box with the specified box (\p other). Comparison
    /// happens lexicographically on \ref pos followed by \ref size.
    ///
    constexpr bool operator==(const Box& other) const noexcept;
    constexpr bool operator!=(const Box& other) const noexcept;
    constexpr bool operator< (const Box& other) const noexcept;
    constexpr bool operator<=(const Box& other) const noexcept;
    constexpr bool operator> (const Box& other) const noexcept;
    constexpr bool operator>=(const Box& other) const noexcept;
    /// \}
};


/// \brief Write textual representation of box to output stream.
///
/// This stream output operator writes a textual representation of the specified box (\p
/// box) to the specified output stream (\p out). See \ref image::Box for information on the
/// format of the textual representation.
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>& out, const image::Box& box) ->
    std::basic_ostream<C, T>&;


/// \brief Combine X-axis extent from one box with Y-axis extent from other box.
///
/// This function constructs a new box with X coordinate of the position and the width taken
/// from the first argument (\p x) and with the Y coordinate of the position and the height
/// taken from the second argument (\p y).
///
auto splice(const image::Box& x, const image::Box& y) noexcept -> image::Box;








// Implementation


constexpr Box::Box(image::Size size) noexcept
    : Box({}, size)
{
}


constexpr Box::Box(image::Pos pos_2, image::Size size_2) noexcept
    : pos(pos_2)
    , size(size_2)
{
}


constexpr bool Box::is_empty() const noexcept
{
    return size.is_empty();
}


constexpr bool Box::contained_in(const Box& other) const noexcept
{
    bool horz = (pos.x >= other.pos.x && size.width <= other.size.width &&
                 pos.x - other.pos.x <= other.size.width - size.width);
    bool vert = (pos.y >= other.pos.y && size.height <= other.size.height &&
                 pos.y - other.pos.y <= other.size.height - size.height);
    return (horz && vert);
}


constexpr bool Box::clip(Box& other) const noexcept
{
    image::Pos pos_2   = other.pos;
    image::Size size_2 = other.size;

    if (ARCHON_LIKELY(pos_2.x >= pos.x)) {
        int discr = size.width - (pos_2.x - pos.x);
        if (ARCHON_UNLIKELY(discr <= 0 || size_2.width <= 0))
            return false;
        size_2.width = std::min(size_2.width, discr);
    }
    else {
        int discr = size_2.width - (pos.x - pos_2.x);
        if (ARCHON_UNLIKELY(discr <= 0 || size.width <= 0))
            return false;
        pos_2.x = pos.x;
        size_2.width = std::min(size.width, discr);
    }

    if (ARCHON_LIKELY(pos_2.y >= pos.y)) {
        int discr = size.height - (pos_2.y - pos.y);
        if (ARCHON_UNLIKELY(discr <= 0 || size_2.height <= 0))
            return false;
        size_2.height = std::min(size_2.height, discr);
    }
    else {
        int discr = size_2.height - (pos.y - pos_2.y);
        if (ARCHON_UNLIKELY(discr <= 0 || size.height <= 0))
            return false;
        pos_2.y = pos.y;
        size_2.height = std::min(size.height, discr);
    }

    other = { pos_2, size_2 };
    return true;
}


constexpr bool Box::operator==(const Box& other) const noexcept
{
    return (pos == other.pos && size == other.size);
}


constexpr bool Box::operator!=(const Box& other) const noexcept
{
    return !(*this == other);
}


constexpr bool Box::operator<(const Box& other) const noexcept
{
    return (pos < other.pos || (pos == other.pos && size < other.size));
}


constexpr bool Box::operator<=(const Box& other) const noexcept
{
    return !(*this > other);
}


constexpr bool Box::operator>(const Box& other) const noexcept
{
    return (other < *this);
}


constexpr bool Box::operator>=(const Box& other) const noexcept
{
    return  !(*this < other);
}


template<class C, class T>
inline auto operator<<(std::basic_ostream<C, T>& out, const image::Box& box) -> std::basic_ostream<C, T>&
{
    core::format(out, "%s;%s", box.pos, box.size); // Throws
    return out;
}


inline auto splice(const image::Box& x, const image::Box& y) noexcept -> image::Box
{
    return { splice(x.pos, y.pos), splice(x.size, y.size) };
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_BOX_HPP
