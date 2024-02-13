// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_INDEX_RANGE_HPP
#define ARCHON_X_CORE_X_INDEX_RANGE_HPP

/// \file


#include <cstddef>
#include <string_view>

#include <archon/core/span.hpp>


namespace archon::core {


/// \brief A reference to a range of positions within some application-given sequence.
///
/// In the same sense as an integer index refers to a particular position within some
/// sequence, an index range of this type refers to a range of positions in such a
/// sequence. What sequence is determined by the application. See \ref resolve() and \ref
/// resolve_string().
///
struct IndexRange {
    /// \brief Offset of range.
    ///
    /// This is the offset of the range of indexes, which is also the index of the first
    /// position in the referenced range.
    ///
    std::size_t offset;

    /// \brief Size of range.
    ///
    /// This is the size of the referenced range, which is the number of consecutive
    /// positions in that range. An index range can be empty, so the size can be zero.
    ///
    std::size_t size;

    /// \brief Resolve index range with respect to memory-contiguous sequence of objects.
    ///
    /// If \p base is a pointer to the base of a memory-contiguous sequence of objects, this
    /// function resolves the index range with respect to that sequence. The resulting span
    /// refers to the same positions in the sequence as did the resolved index range.
    ///
    template<class T> auto resolve(T* base) const noexcept -> core::Span<T>;

    /// \brief Resolve index range with respect to memory-contiguous sequence of objects.
    ///
    /// If \p base is a pointer to the base of a memory-contiguous sequence of characters, this
    /// function resolves the index range with respect to that sequence. The resulting string view
    /// refers to the same characters in the sequence as did the resolved index range.
    ///
    template<class C, class T = std::char_traits<C>> auto resolve_string(const C* base) const ->
        std::basic_string_view<C, T>;
};








// Implementation


template<class T> inline auto IndexRange::resolve(T* base) const noexcept -> core::Span<T>
{
    return { base + offset, size };
}


template<class C, class T>
inline auto IndexRange::resolve_string(const C* base) const -> std::basic_string_view<C, T>
{
    return { base + offset, size }; // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_INDEX_RANGE_HPP
