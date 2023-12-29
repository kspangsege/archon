// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_RENDER_X_IMPL_X_FINITE_SEQUENCE_MEMORY_HPP
#define ARCHON_X_RENDER_X_IMPL_X_FINITE_SEQUENCE_MEMORY_HPP


#include <cstddef>
#include <memory>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/index_iterator.hpp>


namespace archon::render::impl {


template<class T> class FiniteSequenceMemory {
public:
    using const_iterator = core::IndexIterator<const FiniteSequenceMemory>;

    FiniteSequenceMemory(std::size_t capacity);

    void push_back(const T& v) noexcept;

    void clear() noexcept;

    auto size() const noexcept -> std::size_t;

    bool empty() const noexcept;

    auto operator[](std::size_t i) const noexcept -> const T&;

    auto begin() const noexcept -> const_iterator;
    auto end() const noexcept   -> const_iterator;

private:
    std::unique_ptr<T[]> m_buffer;
    std::size_t m_capacity;
    std::size_t m_size = 0;
    std::size_t m_offset = 0;
};








// Implementation


template<class T>
inline FiniteSequenceMemory<T>::FiniteSequenceMemory(std::size_t capacity)
    : m_buffer(std::make_unique<T[]>(capacity)) // Throws
    , m_capacity(capacity)
{
    // Verify that the capacity can be represented in the iterators difference type
    core::int_cast<typename const_iterator::difference_type>(capacity); // Throws
}


template<class T>
void FiniteSequenceMemory<T>::push_back(const T& v) noexcept
{
    if (m_size == m_capacity) {
        m_buffer[m_offset] = v;
        m_offset += 1;
        if (ARCHON_UNLIKELY(m_offset == m_size))
            m_offset = 0;
        return;
    }
    ARCHON_ASSERT(m_offset == 0);
    m_buffer[m_size] = v;
    m_size += 1;
}


template<class T>
inline void FiniteSequenceMemory<T>::clear() noexcept
{
    m_offset = 0;
    m_size = 0;
}


template<class T>
inline auto FiniteSequenceMemory<T>::size() const noexcept -> std::size_t
{
    return m_size;
}


template<class T>
inline bool FiniteSequenceMemory<T>::empty() const noexcept
{
    return (m_size == 0);
}


template<class T>
inline auto FiniteSequenceMemory<T>::operator[](std::size_t i) const noexcept -> const T&
{
    std::size_t top = m_size - m_offset;
    return m_buffer[i < top ? m_offset + i : i - top];
}


template<class T>
inline auto FiniteSequenceMemory<T>::begin() const noexcept -> const_iterator
{
    return const_iterator(*this, 0);
}


template<class T>
inline auto FiniteSequenceMemory<T>::end() const noexcept -> const_iterator
{
    return const_iterator(*this, typename const_iterator::difference_type(m_size));
}


} // namespace archon::render::impl

#endif // ARCHON_X_RENDER_X_IMPL_X_FINITE_SEQUENCE_MEMORY_HPP
