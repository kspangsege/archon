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

#ifndef ARCHON_UTIL_TUPLE_GRID_HPP
#define ARCHON_UTIL_TUPLE_GRID_HPP

#include <iterator>
#include <algorithm>

#include <archon/core/iterator.hpp>
#include <archon/core/memory.hpp>


namespace archon {
namespace Util {

/// A flexible description of the layout of a 2-D grid of tuples within some
/// simple underlying sequence of elements which normally is just raw memory.
///
/// The description tells you the position in the underlying sequence of the
/// tuple at the origin of the grid. This is the \c origin field. The \c pitch
/// field indicates the required advance in number of positions of the
/// underlying sequence to get from some tuple to its neighbor on the
/// right. Likewise, the \c stride field indicates the advance to get from some
/// tuple to its neighbor above.
///
/// The flexibility of the grid description comes from the fact that any values
/// are allowed for \c pitch and \c stride, even negative values. This gives you
/// the freedom to realize both row-major and column-major representations, and
/// to achieve any level of sparseness as long as it is sufficiently regular.
///
/// The flexibility does not, however, extend to the representation of the
/// tuples themselves. The individual elements of a tuple are required to always
/// be adjacent in the underlying sequence. Thus, if \c (x,y) denotes the the
/// grid position of a tuple, and \c i is the index of a particular element
/// within that tuple, the position whitin the underlying sequence of that
/// element can be computed as follows:
///
/// <pre>
///
///   pos  =  origin  +  y * stride  +  x * pitch  +  i
///
/// </pre>
///
/// Note that since each tuple in the grid consists of multiple elements, this
/// class may also be thought of as describing a 3-D grid, or even a 3-D
/// pointer.
template<class Iter> struct BasicTupleGrid {
public:
    using iterator_type   = Iter;
    using value_type      = typename std::iterator_traits<Iter>::value_type;
    using reference       = typename std::iterator_traits<Iter>::reference;
    using difference_type = typename std::iterator_traits<Iter>::difference_type;

    iterator_type origin;
    difference_type pitch;
    difference_type stride;

    BasicTupleGrid() = default;

    BasicTupleGrid(iterator_type origin, difference_type pitch, difference_type stride):
        origin(origin),
        pitch(pitch),
        stride(stride)
    {
    }

    reference operator*() throw()
    {
        return *origin;
    }

    /// \param i Row index.
    ///
    /// \param j Column index.
    ///
    /// \param k Component index.
    reference operator()(int i, int j = 0, int k = 0) throw()
    {
        return origin[i*stride + j*pitch + k];
    }

    void move_right(int n);
    void move_up(int n);

    /// @{ Modify this grid iterator to represent the flipped and/or turned grid.
    ///
    /// A clockwise turn by 90 degrees can be achieved by a horizontal flip
    /// followed by a diagonal flip, or a diagonal flip followed by a vertical
    /// flip.
    void horizontal_flip(int grid_width);
    void vertical_flip(int grid_height);
    void diagonal_flip();
    void antidiagonal_flip(int grid_width, int grid_height);
    void turn_90_degrees_cw(int grid_width);
    void turn_90_degrees_ccw(int grid_height);
    void turn_180_degrees(int grid_width, int grid_height);
    /// @}

    /// Copy tuples from a dense buffer whose start is indicated by the
    /// specified iterator into this grid. The first tuple retrieved from the
    /// buffer will be stored at the origin of this grid, and the remaining
    /// tuples will are assumed to be retrieved in row-major order.
    ///
    /// \param n The number of components per tuple.
    template<class Iter2> void expand_from(Iter2 i, int n, int width, int height) const;

    /// Copy tuples from this grid into a dense buffer whose start is indicated
    /// by the specified iterator. The tuple at the origin of this grid will be
    /// stored first (at the lowest iterator position) and the remaining tuples
    /// will be stored in row-major order.
    ///
    /// \param n The number of components per tuple.
    template<class Iter2> void contract_to(Iter2 i, int n, int width, int height) const;

    /// Extend this tuple grid in any or all directions by first repeating the
    /// original grid, and then extending it further by repeating the closest
    /// edge tuple from the previous stage.
    ///
    /// The algorithm tries to do this efficiently by decomposing the task into
    /// the the fewest and largest contiguous memory copy operations
    /// possible. It also attempts to access memory in the most cache friendly
    /// manner.
    ///
    /// The following is an example. The small box is the original grid, and the
    /// medium box is the result of the first stage that repeats the entire
    /// original grid, and the large box is the result of the last stage that
    /// repeats the closest edge tuple from the medium box. Each letter stands
    /// for a complete tuple:
    ///
    /// <pre>
    ///
    ///   width = 3     height = 3
    ///
    ///   l  = 1,    r  = 5,    d  = 0,    u  = 2
    ///
    ///   l2 = 1,    r2 = 4,    d2 = 0,    u2 = 2
    ///
    ///    -------------------------------------------------------
    ///   | F   F   D   E   F   D   E   F   D   E   E   E   E   E |
    ///   |                                                       |
    ///   | F   F   D   E   F   D   E   F   D   E   E   E   E   E |
    ///   |    -----------------------------------                |
    ///   | F | F   D   E   F   D   E   F   D   E | E   E   E   E |
    ///   |   |                                   |               |
    ///   | I | I   G   H   I   G   H   I   G   H | H   H   H   H |
    ///   |   |    -----------                    |               |
    ///   | C | C | A   B   C | A   B   C   A   B | B   B   B   B |
    ///   |   |   |           |                   |               |
    ///   | F | F | D   E   F | D   E   F   D   E | E   E   E   E |
    ///   |   |   |           |                   |               |
    ///   | I | I | G   H   I | G   H   I   G   H | H   H   H   H |
    ///    --- --- ----------- ------------------- ---------------
    ///
    /// </pre>
    ///
    /// \param grid Defines the memory layout of the tuple grid. A grid cell as
    /// defined in \c Grid coincodes with the first component of a tuple.
    ///
    /// \param n The number of components per tuple. Must be strictly positive.
    ///
    /// \param width, height The width and height of the original grid. Both
    /// must be strictly positive.
    ///
    /// \param l, r, d, u The amount of extension that repeats the entire
    /// original grid in the left, right, downwards, and upwards direction
    /// respectively. None of them may be negative.
    ///
    /// \param l2, r2, d2, u2 The amount of extension that repeats the closest
    /// edge tuple of stage 1 in the left, right, downwards, and upwards
    /// direction respectively. None of them may be negative.
    void extend(int n, int width, int height,
                int left, int right, int down, int up,
                int left2, int right2, int down2, int up2) const;


private:
    template<class J> friend struct BasicTupleGrid;

    void extend_0(int n, int width, int height,
                  int left, int right, int down, int up,
                  int left2, int right2, int down2, int up2);

    void extend_1(int n, int width, int height,
                  int left, int right, int down, int up,
                  int left2, int right2, int down2, int up2);

    void extend_2(int width, int height,
                  int left, int right, int down, int up,
                  int left2, int right2, int down2, int up2);
};



typedef BasicTupleGrid<char*> TupleGrid;
typedef BasicTupleGrid<const char*> ConstTupleGrid;




// Implementation

template<class I> inline void BasicTupleGrid<I>::move_right(int n)
{
    origin += n * pitch;
}

template<class I> inline void BasicTupleGrid<I>::move_up(int n)
{
    origin += n * stride;
}

template<class I> inline void BasicTupleGrid<I>::horizontal_flip(int grid_width)
{
    pitch = -pitch;
    origin -= pitch * (grid_width-1);
}

template<class I> inline void BasicTupleGrid<I>::vertical_flip(int grid_height)
{
    stride = -stride;
    origin -= stride * (grid_height-1);
}

template<class I> inline void BasicTupleGrid<I>::diagonal_flip()
{
    std::swap(pitch, stride);
}

template<class I> inline void BasicTupleGrid<I>::antidiagonal_flip(int grid_width, int grid_height)
{
    turn_180_degrees(grid_width, grid_height);
    diagonal_flip();
}

template<class I> inline void BasicTupleGrid<I>::turn_90_degrees_cw(int grid_width)
{
    horizontal_flip(grid_width);
    diagonal_flip();
}

template<class I> inline void BasicTupleGrid<I>::turn_90_degrees_ccw(int grid_height)
{
    vertical_flip(grid_height);
    diagonal_flip();
}

template<class I> inline void BasicTupleGrid<I>::turn_180_degrees(int grid_width, int grid_height)
{
    horizontal_flip(grid_width);
    vertical_flip(grid_height);
}

template<class I> template<class J>
inline void BasicTupleGrid<I>::expand_from(J s1, int n, int w, int h) const
{
    I t1 = origin;
    for (int i = 0; i < h; ++i) {
        I t2 = t1;
        for(int j = 0; j < w; ++j) {
            J s2 = s1;
            s1 += n;
            std::copy(s2, s1, t2);
            t2 += pitch;
        }
        t1 += stride;
    }
}

template<class I> template<class J>
inline void BasicTupleGrid<I>::contract_to(J t, int n, int w, int h) const
{
    I s1 = origin;
    for (int i = 0; i < h; ++i) {
        I s2 = s1;
        for (int j = 0; j < w; ++j) {
            t = std::copy(s2, s2+n, t);
            s2 += pitch;
        }
        s1 += stride;
    }
}

template<class I>
inline void BasicTupleGrid<I>::extend(int n, int width, int height,
                                      int left, int right, int down, int up,
                                      int left2, int right2, int down2, int up2) const
{
    BasicTupleGrid<I>(*this).extend_0(n, width, height,
                                      left, right, down, up, left2, right2, down2, up2);
}


template<class I>
inline void BasicTupleGrid<I>::extend_0(int n, int width, int height,
                                        int left, int right, int down, int up,
                                        int left2, int right2, int down2, int up2)
{
    // Bring the origin to the first component of the first tuple in
    // memory so that we can work forward in memory
    if (stride < 0) {
        vertical_flip(height);
        std::swap(down,  up);
        std::swap(down2, up2);
    }
    if (pitch < 0) {
        horizontal_flip(width);
        std::swap(left,  right);
        std::swap(left2, right2);
    }
    if (stride < pitch) {
        diagonal_flip();
        std::swap(width,  height);
        std::swap(left,   down);
        std::swap(right,  up);
        std::swap(left2,  down2);
        std::swap(right2, up2);
    }

    // Hide the gap between rows
    difference_type l = pitch * (left2+left);
    difference_type use = l + pitch * (width+right+right2);
    difference_type gap = stride - use;
    if (!gap) {
        extend_1(n, width, height, left, right, down, up, left2, right2, down2, up2);
    }
    else {
        using PeriodGrid = BasicTupleGrid<core::PeriodIter<I>>;
        PeriodGrid period_grid(core::PeriodIter<I>(origin, use, gap, l), pitch, use);
        period_grid.extend_1(n, width, height, left, right, down, up, left2, right2, down2, up2);
    }
}


template<class I>
inline void BasicTupleGrid<I>::extend_1(int n, int width, int height,
                                        int left, int right, int down, int up,
                                        int left2, int right2, int down2, int up2)
{
    // Hide the gap between tuples
    difference_type gap = pitch - n;
    if (!gap) {
        extend_2(width, height, left, right, down, up, left2, right2, down2, up2);
    }
    else {
        using PeriodGrid = BasicTupleGrid<core::PeriodIter<I>>;
        PeriodGrid period_grid(core::PeriodIter<I>(origin, n, gap), n, stride/pitch*n);
        period_grid.extend_2(width, height, left, right, down, up, left2, right2, down2, up2);
    }
}


template<class I>
inline void BasicTupleGrid<I>::extend_2(int width, int height,
                                        int left, int right, int down, int up,
                                        int left2, int right2, int down2, int up2)
{
    // Extend horizontally
    difference_type p = pitch, w = p * width,
        l = p * left, l2 = p * left2, r = p * right, r2 = p * right2;
    if (l||l2||r||r2) {
        I b = origin;
        for (int i = 0; i < height; ++i) {
            difference_type m = l ? core::repeat(b, w, -l) : 0;
            if (l2)
                core::repeat(b-l, p, -l2);
            I e = b + w;
            if (r)
                core::repeat(e, m-w-l, r);
            if (r2)
                core::repeat(e+r, -p, r2);
            b += stride;
        }
    }

    // Extend vertically in full width
    if (down||down2||up||up2) {
        difference_type s = stride, h = s * height,
            d = s * down, d2 = s * down2, u = s * up, u2 = s * up2;
        I b = origin;
        b -= l2 + l;
        difference_type m = d ? core::repeat(b, h, -d) : 0;
        if (d2)
            core::repeat(b-d, s, -d2);
        b += h;
        if (u)
            core::repeat(b, m-h-d, u);
        if (u2)
            core::repeat(b+u, -s, u2);
    }
}

} // namespace Util
} // namespace archon

#endif // ARCHON_UTIL_TUPLE_GRID_HPP
