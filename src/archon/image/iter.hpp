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

#ifndef ARCHON_X_IMAGE_X_ITER_HPP
#define ARCHON_X_IMAGE_X_ITER_HPP

/// \file


#include <cstddef>
#include <algorithm>

#include <archon/core/assert.hpp>
#include <archon/image/geom.hpp>


namespace archon::image {


/// \brief Pixel iterator.
///
/// An object of this type, i.e., a pixel iterator, combines a pointer, which is assumed to
/// refer to a pixel, with information about how to move to the next pixel in either the
/// horizontal or vertical direction.
///
/// A pixel iterator does not specify how many memory words make up each pixel, nor does it
/// specify the boundaries of the pixel grid.
///
/// An iterator is either typed or untyped. In a typed iterator, the word type is known,
/// whereas in an untyped iterator, the word type is unknown (\p T is `void` or `const
/// void`).
///
/// \tparam T For an typed iterator, this is the type of memory words that pixels are made
/// of. It may, or may not be `const` qualified. For an untyped iterator, this is `void` or
/// `const void`.
///
/// A pixel iterator is allowed to refer one pixel beyond the underlying pixel grid (just
/// like a regular pointer is allowed to point one passed the last element in an underlying
/// array). This means that if `iter` points to the base of a pixel grid of size `size`,
/// then all iterators obtained as `iter + image::Size(x, y)` are valid so long as `x` is
/// less than, or equal to `size.width` and `y` is less than, or equal to
/// `size.height`. Behavior is undefined if any other iterator is constructed, even if it is
/// not otherwise used.
///
template<class T> class Iter {
public:
    using comp_type = T;

    /// \brief Pointer to first word of pixel.
    ///
    /// This pointer refers to the first word of a pixel.
    ///
    comp_type* base;

    /// \{
    ///
    /// \brief Horizontal and vertical strides.
    ///
    /// These are the horizontal and vertical strides respectively. The horizontal stride is
    /// the number that must be added to \ref base in order to move one pixel to the right,
    /// or subtracted to move one pixel to the left. The vertical stride is the number that
    /// must be added to \ref base in order to move one pixel downwards, or subtracted to
    /// move one pixel upwards. Both strides must be positive. Either one is allowed to be
    /// greater than the other. Behavior is undefined if either stride is zero or negative.
    ///
    std::ptrdiff_t horz_stride;
    std::ptrdiff_t vert_stride;
    /// \}

    /// \brief Implicit conversion to compatible iterator type.
    ///
    /// This function attempts to construct an iterator with the specified word type from
    /// the pointer and strides of this iterator. This will only work if the pointer (\ref
    /// base) is implicitly convertible to `U*`.
    ///
    /// This conversion operations allows for a typed iterator to be implicitly converted to
    /// an untyped iterator, and also for an iterator, whose word type is not
    /// `const`-qualified, to be implicitly converted to an iterator whose word type is
    /// `const`-qualified.
    ///
    template<class U> constexpr operator Iter<U>() const noexcept;

    /// \brief Cast untyped iterator to typed iterator.
    ///
    /// This function attempts to cast the pointer (\ref base) to a pointer to the specified
    /// word type (`U*`) and then construct a pixel iterator from that pointer and the
    /// horizontal and vertical strides from this iterator (\ref horz_stride and \ref
    /// vert_stride).
    ///
    /// This operation is intended for casting an untyped iterator to a typed iterator. Such
    /// a cast makes sense only when casting to the type that is actually the type of the
    /// words at the memory address pointed to by \ref base.
    ///
    template<class U> constexpr auto cast_to() const noexcept -> Iter<U>;

    /// \{
    ///
    /// \brief Get pointer to pixel at specified position.
    ///
    /// If `iter` is a typed iterator, then `iter(x, y)` is shorthand for `(iter +
    /// image::Size(x, y)).base`. Likewise, `iter(pos)` is shorthand for `(iter + (pos -
    /// image::Pos())).base`.
    ///
    /// These operators are not available for untyped iterators.
    ///
    /// \sa \ref operator+() and \ref operator-()
    ///
    constexpr auto operator()(int x, int y) const noexcept -> comp_type*;
    constexpr auto operator()(image::Pos pos) const noexcept -> comp_type*;
    /// \}

    /// \{
    ///
    /// \brief Construct moved iterator.
    ///
    /// If `iter` is a typed iterator, then `iter + size` is a shorthand for making a copy
    /// of `iter`, say `iter_2`, then doing `iter_2 += size`, and finally returning
    /// `iter_2`. Similarly, `iter - size` is a shorthand for making a copy, `iter_2`, and
    /// doing `iter_2 -= size`.
    ///
    /// These operators are not available for untyped iterators.
    ///
    /// \sa \ref operator+=() and \ref operator-=()
    ///
    constexpr auto operator+(image::Size size) const noexcept -> Iter;
    constexpr auto operator-(image::Size size) const noexcept -> Iter;
    /// \}

    /// \{
    ///
    /// \brief Move typed iterators to different pixel in grid.
    ///
    /// For typed iterators, these operators move the iterator by the specified distances in
    /// the horizontal (`size.width`) and vertical (`size.height`) directions.
    ///
    /// These operators are not available for untyped iterators.
    ///
    /// Behavior is undefined if the iterator is moved outside the boundaries of the pixel
    /// grid.
    ///
    constexpr auto operator+=(image::Size size) noexcept -> Iter&;
    constexpr auto operator-=(image::Size size) noexcept -> Iter&;
    /// \}

    /// \brief Copy array of pixels.
    ///
    /// This function copies an array pixels of the specified size (\p size) from the memory
    /// locations referenced by this iterator to the memory locations referenced by \p
    /// other. The number of components per pixel is specified by \p n.
    ///
    template<class U> void copy_to(image::Iter<U> other, image::Size size, int n) const;

    /// \brief Fill array with single pixel.
    ///
    /// This function fills the specified area (\p area) with copies of the specified pixel
    /// (\p pixel). The number of components per pixel is specified by \p n.
    ///
    /// Naturally, boundaries of the specified area (\p area) must not escape the boundaries
    /// of the underlying pixel array (as pointed to be this iterator). If they do, behavior
    /// is undefined.
    ///
    void fill(const comp_type* pixel, const image::Box& area, int n) const;

    /// \brief Fill area with repetitions of subsection of that area.
    ///
    /// Given a rectangular area of pixels (\p area), and a pattern (\p pattern) as a
    /// nonempty rectangular subsection of that area, this function fills the area outside
    /// the pattern with copies of the pattern. The number of components per pixel is
    /// specified by \p n.
    ///
    /// A pixel, P, inside the area but outside the pattern is replaced with a copy of the
    /// pixel in the pattern whose position is uniquely obtained by shifting the position of
    /// P horizontally by an integer number of pattern widths and vertically by an integer
    /// number of pattern heights.
    ///
    /// The specified pattern (\p pattern) must be confined to the specified area (\p
    /// area). If it is not, behavior is undefined.
    ///
    /// Naturally, boundaries of the specified area (\p area) must not escape the boundaries
    /// of the underlying pixel array (as pointed to be this iterator). If they do, behavior
    /// is undefined.
    ///
    void repeat(const image::Box& pattern, const image::Box& area, int n) const;

    /// \{
    ///
    /// \brief Repeat pattern to the left, right, upwards, or downwards.
    ///
    /// These four functions repeat the specified pattern (\p pattern) to the left, to the
    /// right, upwards, and downwards respectively.
    ///
    /// In the case of `repeat_left()`, a pixel is modified if it occurs to the left of the
    /// pattern (at the same Y coordinate as a pixel in the pattern) and within a distance
    /// of \p size pixels from the left edge of the pattern. A modified pixel, P, is
    /// replaced with a copy of the pixel in the pattern whose position is uniquely obtained
    /// by shifting the position of P to the right by an integer number of pattern widths.
    ///
    /// In the case of `repeat_right()`, a pixel is modified if it occurs to the right of
    /// the pattern (at the same Y coordinate as a pixel in the pattern) and within a
    /// distance of \p size pixels from the right edge of the pattern. A modified pixel, P,
    /// is replaced with a copy of the pixel in the pattern whose position is uniquely
    /// obtained by shifting the position of P to the left by an integer number of pattern
    /// widths.
    ///
    /// In the case of `repeat_up()`, a pixel is modified if it occurs above the pattern (at
    /// the same X coordinate as a pixel in the pattern) and within a distance of \p size
    /// pixels from the top of the pattern. A modified pixel, P, is replaced with a copy of
    /// the pixel in the pattern whose position is uniquely obtained by shifting the
    /// position of P downwards by an integer number of pattern heights.
    ///
    /// In the case of `repeat_down()`, a pixel is modified if it occurs below the pattern
    /// (at the same X coordinate as a pixel in the pattern) and within a distance of \p
    /// size pixels from the bottom of the pattern. A modified pixel, P, is replaced with a
    /// copy of the pixel in the pattern whose position is uniquely obtained by shifting the
    /// position of P upwards by an integer number of pattern heights.
    ///
    /// The number of components per pixel is specified by \p n.
    ///
    /// Naturally, the boundaries of the specified pattern (\pattern) must not escape the
    /// boundaries of the underlying pixel array (as pointed to be this iterator). If they
    /// do, behavior is undefined. Likewise, the boundaries of area modified by these
    /// functions must also not escape the boundaries of the underlying pixel array. If they
    /// do, behavior is undefined.
    ///
    /// For `repeat_left()` and `repeat_right()`, the width of the specified pattern must be
    /// greater than zero. If the width is less than, or equal to zero, behavior is
    /// undefined. Similarly, for `repeat_up()` and `repeat_down()`, the height of the
    /// specified pattern must be greater than zero. If the height is less than, or equal to
    /// zero, behavior is undefined.
    ///
    /// The specified size (\p size) must be non-negative. If it is negative, behavior is
    /// undefined.
    ///
    void repeat_left(const image::Box& pattern, int size, int n) const;
    void repeat_right(const image::Box& pattern, int size, int n) const;
    void repeat_up(const image::Box& pattern, int size, int n) const;
    void repeat_down(const image::Box& pattern, int size, int n) const;
    /// \}
};

template<class T> Iter(T*, std::ptrdiff_t, std::ptrdiff_t) -> Iter<T>;








// Implementation


template<class T>
template<class U> constexpr Iter<T>::operator Iter<U>() const noexcept
{
    return { base, horz_stride, vert_stride };
}


template<class T>
template<class U> constexpr auto Iter<T>::cast_to() const noexcept -> Iter<U>
{
    return { static_cast<U*>(base), horz_stride, vert_stride };
}


template<class T>
constexpr auto Iter<T>::operator()(int x, int y) const noexcept -> comp_type*
{
    return operator()({ x, y });
}


template<class T>
constexpr auto Iter<T>::operator()(image::Pos pos) const noexcept -> comp_type*
{
    comp_type* ptr = base;
    ptr += pos.x * horz_stride;
    ptr += pos.y * vert_stride;
    return ptr;
}


template<class T>
constexpr auto Iter<T>::operator+(image::Size size) const noexcept -> Iter
{
    Iter iter = *this;
    return iter += size;
}


template<class T>
constexpr auto Iter<T>::operator-(image::Size size) const noexcept -> Iter
{
    Iter iter = *this;
    return iter -= size;
}


template<class T>
constexpr auto Iter<T>::operator+=(image::Size size) noexcept -> Iter&
{
    base += size.width  * horz_stride;
    base += size.height * vert_stride;
    return *this;
}


template<class T>
constexpr auto Iter<T>::operator-=(image::Size size) noexcept -> Iter&
{
    base -= size.width  * horz_stride;
    base -= size.height * vert_stride;
    return *this;
}


template<class T>
template<class U> void Iter<T>::copy_to(image::Iter<U> other, image::Size size, int n) const
{
    for (int y = 0; y < size.height; ++y) {
        for (int x = 0; x < size.width; ++x) {
            const comp_type* origin = (*this)(x, y);
            U* destin = other(x, y);
            std::copy(origin, origin + n, destin);
        }
    }
}


template<class T>
void Iter<T>::fill(const comp_type* pixel, const image::Box& area, int n) const
{
    for (int y = 0; y < area.size.height; ++y) {
        for (int x = 0; x < area.size.width; ++x)
            std::copy(pixel, pixel + n, (*this)(area.pos + image::Size(x, y)));
    }
}


template<class T>
void Iter<T>::repeat(const image::Box& pattern, const image::Box& area, int n) const
{
    ARCHON_ASSERT(area.contains(pattern));
    image::Size lead_size = pattern.pos - area.pos;
    image::Size trail_size = area.size - (lead_size + pattern.size);
    repeat_left(pattern, lead_size.width, n); // Throws
    repeat_right(pattern, trail_size.width, n); // Throws
    image::Box pattern_2 = splice(area, pattern);
    repeat_up(pattern_2, lead_size.height, n); // Throws
    repeat_down(pattern_2, trail_size.height, n); // Throws
}


template<class T>
void Iter<T>::repeat_left(const image::Box& pattern, int size, int n) const
{
    ARCHON_ASSERT(pattern.size.width > 0);
    ARCHON_ASSERT(size >= 0);
    Iter iter = *this + (pattern.pos - image::Pos());
    int offset = 0;
    while (size - offset >= pattern.size.width) {
        offset += pattern.size.width;
        iter.copy_to(iter - image::Size(offset, 0), pattern.size, n); // Throws
    }
    int rest = size - offset;
    Iter iter_2 = iter + image::Size(pattern.size.width - rest, 0);
    iter_2.copy_to(iter - image::Size(size, 0), pattern.size.with_width(rest), n); // Throws
}


template<class T>
void Iter<T>::repeat_right(const image::Box& pattern, int size, int n) const
{
    ARCHON_ASSERT(pattern.size.width > 0);
    ARCHON_ASSERT(size >= 0);
    Iter iter = *this + (pattern.pos - image::Pos());
    int offset = 0;
    while (size - offset >= pattern.size.width) {
        offset += pattern.size.width;
        iter.copy_to(iter + image::Size(offset, 0), pattern.size, n); // Throws
    }
    int rest = size - offset;
    offset += pattern.size.width;
    iter.copy_to(iter + image::Size(offset, 0), pattern.size.with_width(rest), n); // Throws
}


template<class T>
void Iter<T>::repeat_up(const image::Box& pattern, int size, int n) const
{
    ARCHON_ASSERT(pattern.size.height > 0);
    ARCHON_ASSERT(size >= 0);
    Iter iter = *this + (pattern.pos - image::Pos());
    int offset = 0;
    while (size - offset >= pattern.size.height) {
        offset += pattern.size.height;
        iter.copy_to(iter - image::Size(0, offset), pattern.size, n); // Throws
    }
    int rest = size - offset;
    Iter iter_2 = iter + image::Size(0, pattern.size.height - rest);
    iter_2.copy_to(iter - image::Size(0, size), pattern.size.with_height(rest), n); // Throws
}


template<class T>
void Iter<T>::repeat_down(const image::Box& pattern, int size, int n) const
{
    ARCHON_ASSERT(pattern.size.height > 0);
    ARCHON_ASSERT(size >= 0);
    Iter iter = *this + (pattern.pos - image::Pos());
    int offset = 0;
    while (size - offset >= pattern.size.height) {
        offset += pattern.size.height;
        iter.copy_to(iter + image::Size(0, offset), pattern.size, n); // Throws
    }
    int rest = size - offset;
    offset += pattern.size.height;
    iter.copy_to(iter + image::Size(0, offset), pattern.size.with_height(rest), n); // Throws
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_ITER_HPP
