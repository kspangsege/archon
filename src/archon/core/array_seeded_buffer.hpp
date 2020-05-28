// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_ARRAY_SEEDED_BUFFER_HPP
#define ARCHON_X_CORE_X_ARRAY_SEEDED_BUFFER_HPP

/// \file


#include <cstddef>
#include <array>

#include <archon/core/span.hpp>
#include <archon/core/buffer.hpp>


namespace archon::core {


template<class T, std::size_t N> class ArraySeededBuffer
    : private std::array<T, N>
    , public core::Buffer<T> {
public:
    ArraySeededBuffer() noexcept;
    ~ArraySeededBuffer() noexcept = default;

    ArraySeededBuffer(std::size_t size);
    template<class U> ArraySeededBuffer(core::BufferDataTag, core::Span<U> data);

    using core::Buffer<T>::operator[];
    using core::Buffer<T>::data;
    using core::Buffer<T>::size;
    using core::Buffer<T>::begin;
    using core::Buffer<T>::end;
};








// Implementation


template<class T, std::size_t N>
inline ArraySeededBuffer<T, N>::ArraySeededBuffer() noexcept
    : core::Buffer<T>(static_cast<std::array<T, N>&>(*this))
{
}


template<class T, std::size_t N>
inline ArraySeededBuffer<T, N>::ArraySeededBuffer(std::size_t size)
    : core::Buffer<T>(static_cast<std::array<T, N>&>(*this), size) // Throws
{
}


template<class T, std::size_t N>
template<class U> inline ArraySeededBuffer<T, N>::ArraySeededBuffer(core::BufferDataTag, core::Span<U> data)
    : core::Buffer<T>(static_cast<std::array<T, N>&>(*this), core::BufferDataTag(), data) // Throws
{
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_ARRAY_SEEDED_BUFFER_HPP
