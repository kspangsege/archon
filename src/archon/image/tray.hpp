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

#ifndef ARCHON_X_IMAGE_X_TRAY_HPP
#define ARCHON_X_IMAGE_X_TRAY_HPP

/// \file


#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>


namespace archon::image {


/// \brief Reference to storage for a rectangular block of pixels.
///
/// A tray is a reference to storage for a rectangular block of pixels. A tray is generally
/// used for passing pixels along in read and write operations on images.
///
/// Trays do not require that pixels are stored in a memory-contiguous manner. Instead,
/// arbitrary horizontal and vertical strides can by used (see \ref image::Iter).
///
/// A tray is either typed or untyped. In a typed tray, the iterator (\ref iter) is typed,
/// and in an untyped tray, the iterator is untyped.
///
/// \tparam T For an typed tray, this is the type of memory words that pixels are made
/// of. It may, or may not be `const` qualified. For an untyped tray, this is `void` or
/// `const void`.
///
template<class T> class Tray {
public:
    using comp_type = T;

    using iter_type       = image::Iter<comp_type>;
    using const_iter_type = image::Iter<const comp_type>;

    /// \brief Iterator referring to top-left pixel.
    ///
    /// This is an iterator pointing to the storage for the top-left pixel in the tray. This
    /// iterator also provides the means by which one obtains new iterators that point to
    /// the other pixels in the tray.
    ///
    iter_type iter;

    /// \brief Size of tray.
    ///
    /// This is the size of the tray.
    ///
    image::Size size;

    /// \brief Whether tray is empty.
    ///
    /// This function returns `false` if the tray contains at least one pixel. Otherwise, it
    /// returns `true`.
    ///
    constexpr bool is_empty() const noexcept;

    /// \brief Implicit conversion to compatible tray type.
    ///
    /// This function attempts to construct a tray with the specified word type from the
    /// iterator and strides of this tray. This will only work if the iterator (\ref iter)
    /// is implicitly convertible to `image::Iter<U>`.
    ///
    /// This conversion operations allows for a typed tray to be implicitly converted to an
    /// untyped tray, and also for a tray, whose word type is not `const`-qualified, to be
    /// implicitly converted to a tray whose word type is `const`-qualified.
    ///
    template<class U> constexpr operator Tray<U>() const noexcept;

    /// \brief Cast untyped tray to typed tray.
    ///
    /// This function attempts to cast the iterator (\ref iter) to an iterator with the
    /// specified word type (`image::Iter<U>`) and then construct a tray from that iterator
    /// and the size from this iterator (\ref size).
    ///
    /// This operation is intended for casting an untyped tray to a typed tray. Such a cast
    /// makes sense only when casting to the type that is actually the type of the words at
    /// the memory address pointed to by \ref iter.
    ///
    template<class U> constexpr auto cast_to() const noexcept -> Tray<U>;

    /// \{
    ///
    /// \brief Get pointer to pixel at specified position.
    ///
    /// If `tray` is a typed tray, then `tray(x, y)` is shorthand for `tray.iter(x,
    /// y)`. Likewise, `tray(pos)` is shorthand for `tray.iter(pos)`.
    ///
    /// These operators are not available for untyped trays.
    ///
    /// \sa \ref image::Iter::operator()()
    ///
    constexpr auto operator()(int x, int y) const noexcept -> comp_type*;
    constexpr auto operator()(image::Pos) const noexcept -> comp_type*;
    /// \}

    /// \brief Get tray for subsection.
    ///
    /// This function returns a tray for the specified subsection of this tray.
    ///
    /// \param origin The position corresponding to the top-left corner of the tray from
    /// which the sub-tray is extracted.
    ///
    constexpr auto subtray(const image::Box&, image::Pos origin = { 0, 0 }) const noexcept -> Tray;

    /// \brief Copy pixels to locations specified by iterator.
    ///
    /// This function copies pixels from this tray to the memory locations referenced by \p
    /// other. The number of components per pixel is specified by \p n.
    ///
    template<class U> void copy_to(image::Iter<U> other, int n) const;

    /// \brief Copy pixels from locations specified by iterator.
    ///
    /// This function copies pixels to this tray from the memory locations referenced by \p
    /// other. The number of components per pixel is specified by \p n.
    ///
    void copy_from(const_iter_type other, int n) const;

    /// \brief Fill tray with single pixel.
    ///
    /// This function fills the tray with copies of the specified pixel (\p pixel). The
    /// number of components per pixel is specified by \p n.
    ///
    void fill(const comp_type* pixel, int n) const;

    /// \brief Fill tray with repetitions of subsection of itself.
    ///
    /// Given a pattern (\p pattern) as a nonempty rectangular subsection of this tray, this
    /// function fills the area outside the pattern with copies of the pattern.
    ///
    /// `tray.repeat(pattern, n)` is a shorthand for `tray.iter.repeat(pattern, tray.size,
    /// n)`.
    ///
    void repeat(const image::Box& pattern, int n) const;
};

template<class T> Tray(image::Iter<T>, image::Size) -> Tray<T>;








// Implementation


template<class T>
constexpr bool Tray<T>::is_empty() const noexcept
{
    return size.is_empty();
}


template<class T>
template<class U> constexpr Tray<T>::operator Tray<U>() const noexcept
{
    return { iter, size };
}


template<class T>
template<class U> constexpr auto Tray<T>::cast_to() const noexcept -> Tray<U>
{
    return { iter.template cast_to<U>(), size };
}


template<class T>
constexpr auto Tray<T>::operator()(int x, int y) const noexcept -> comp_type*
{
    return iter(x, y);
}


template<class T>
constexpr auto Tray<T>::operator()(image::Pos pos) const noexcept -> comp_type*
{
    return iter(pos);
}


template<class T>
constexpr auto Tray<T>::subtray(const image::Box& box, image::Pos origin) const noexcept -> Tray
{
    return { iter + (box.pos - origin), box.size };
}


template<class T>
template<class U> inline void Tray<T>::copy_to(image::Iter<U> other, int n) const
{
    iter.copy_to(other, size, n); // Throws
}


template<class T>
inline void Tray<T>::copy_from(const_iter_type other, int n) const
{
    other.copy_to(iter, size, n); // Throws
}


template<class T>
inline void Tray<T>::fill(const comp_type* pixel, int n) const
{
    iter.fill(pixel, size, n); // Throws
}


template<class T>
inline void Tray<T>::repeat(const image::Box& pattern, int n) const
{
    iter.repeat(pattern, size, n); // Throws
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_TRAY_HPP
