// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_BASE_X_SEED_MEMORY_OUTPUT_STREAM_HPP
#define ARCHON_X_BASE_X_SEED_MEMORY_OUTPUT_STREAM_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <streambuf>
#include <ios>

#include <archon/base/features.h>
#include <archon/base/integer.hpp>
#include <archon/base/memory.hpp>


namespace archon::base {


template<class C, class T> class BasicSeedMemoryStreambuf;




template<class C, class T = std::char_traits<C>> class BasicSeedMemoryOutputStream :
        public std::basic_ostream<C, T> {
public:
    BasicSeedMemoryOutputStream();
    explicit BasicSeedMemoryOutputStream(base::Span<C> seed_memory);

    std::basic_string_view<C, T> view() const noexcept;

    /// \brief Crear contents and error state.
    ///
    /// Besides resetting the error state by calling `clear()`, this function
    /// clears previously written contents by calling \ref
    /// BasicSeedMemoryStreambuf::clear_contents() on the contained stream
    /// buffer object.
    ///
    void full_clear() noexcept;

    BasicSeedMemoryStreambuf<C, T>& streambuf() noexcept;
    const BasicSeedMemoryStreambuf<C, T>& streambuf() const noexcept;

private:
    BasicSeedMemoryStreambuf<C, T> m_streambuf;
};


template<class C, std::size_t N>
explicit BasicSeedMemoryOutputStream(C (&)[N]) -> BasicSeedMemoryOutputStream<C>;
template<class C, std::size_t N>
explicit BasicSeedMemoryOutputStream(std::array<C, N>&) -> BasicSeedMemoryOutputStream<C>;


using SeedMemoryOutputStream     = BasicSeedMemoryOutputStream<char>;
using WideSeedMemoryOutputStream = BasicSeedMemoryOutputStream<wchar_t>;




template<class C, class T = std::char_traits<C>> class BasicSeedMemoryStreambuf :
        public std::basic_streambuf<C, T> {
public:
    static_assert(std::is_trivial_v<C>);

    using int_type    = typename std::basic_streambuf<C, T>::int_type;
    using traits_type = typename std::basic_streambuf<C, T>::traits_type;

    BasicSeedMemoryStreambuf();
    ~BasicSeedMemoryStreambuf() noexcept;

    explicit BasicSeedMemoryStreambuf(base::Span<C> seed_memory);

    // Disable copying
    BasicSeedMemoryStreambuf(const BasicSeedMemoryStreambuf&) = delete;
    BasicSeedMemoryStreambuf& operator=(const BasicSeedMemoryStreambuf&) = delete;

    C* data() noexcept;
    const C* data() const noexcept;
    std::size_t size() const noexcept;
    std::basic_string_view<C, T> view() const noexcept;

    void reserve_extra(std::size_t min_extra_capacity);

    // Maximally efficient append operations
    void append(const C* data, std::size_t size);
    void append(std::size_t size, C value);

    /// This function does not release any allocated memory.
    ///
    void clear_contents() noexcept;

private:
    C* const m_seed_memory;
    C* m_memory;

    std::streamsize xsputn(const C*, std::streamsize) override final;
    int_type overflow(int_type) override final;

    void do_reserve_extra(std::size_t min_extra_capacity);

    void dealloc() noexcept;
    void bump_put_ptr(std::size_t size) noexcept;
};


template<class C, std::size_t N>
explicit BasicSeedMemoryStreambuf(C (&)[N]) -> BasicSeedMemoryStreambuf<C>;
template<class C, std::size_t N>
explicit BasicSeedMemoryStreambuf(std::array<C, N>&) -> BasicSeedMemoryStreambuf<C>;


using SeedMemoryStreambuf     = BasicSeedMemoryStreambuf<char>;
using WideSeedMemoryStreambuf = BasicSeedMemoryStreambuf<wchar_t>;








// Implementation


// ============================ BasicSeedMemoryOutputStream ============================


template<class C, class T>
inline BasicSeedMemoryOutputStream<C, T>::BasicSeedMemoryOutputStream() :
    BasicSeedMemoryOutputStream(base::Span<C>()) // Throws
{
}


template<class C, class T>
inline BasicSeedMemoryOutputStream<C, T>::BasicSeedMemoryOutputStream(base::Span<C> seed_memory) :
    std::basic_ostream<C, T>(nullptr), // Throws
    m_streambuf(seed_memory) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class C, class T>
inline std::basic_string_view<C, T> BasicSeedMemoryOutputStream<C, T>::view() const noexcept
{
    return m_streambuf.view();
}


template<class C, class T> inline void BasicSeedMemoryOutputStream<C, T>::full_clear() noexcept
{
    m_streambuf.clear_contents();
    // Assuming that std::basic_ostream<C, T>::clear() does not throw when no
    // argument is specified and rdbuf() does not return null.
    std::basic_ostream<C, T>::clear();
}


template<class C, class T>
inline auto BasicSeedMemoryOutputStream<C, T>::streambuf() noexcept ->
    BasicSeedMemoryStreambuf<C, T>&
{
    return m_streambuf;
}


template<class C, class T>
inline auto BasicSeedMemoryOutputStream<C, T>::streambuf() const noexcept ->
    const BasicSeedMemoryStreambuf<C, T>&
{
    return m_streambuf;
}



// ============================ BasicSeedMemoryStreambuf ============================


template<class C, class T> inline BasicSeedMemoryStreambuf<C, T>::BasicSeedMemoryStreambuf() :
    BasicSeedMemoryStreambuf(base::Span<C>()) // Throws
{
}


template<class C, class T>
inline BasicSeedMemoryStreambuf<C, T>::~BasicSeedMemoryStreambuf() noexcept
{
    dealloc();
}


template<class C, class T>
inline BasicSeedMemoryStreambuf<C, T>::BasicSeedMemoryStreambuf(base::Span<C> seed_memory) :
    std::basic_streambuf<C, T>(), // Throws
    m_seed_memory(seed_memory.data())
{
    m_memory = m_seed_memory;
    this->setp(m_memory, m_memory + seed_memory.size());
}


template<class C, class T> inline C* BasicSeedMemoryStreambuf<C, T>::data() noexcept
{
    return m_memory;
}


template<class C, class T> inline const C* BasicSeedMemoryStreambuf<C, T>::data() const noexcept
{
    return m_memory;
}


template<class C, class T> inline std::size_t BasicSeedMemoryStreambuf<C, T>::size() const noexcept
{
    return std::size_t(this->pptr() - m_memory);
}


template<class C, class T>
inline std::basic_string_view<C, T> BasicSeedMemoryStreambuf<C, T>::view() const noexcept
{
    return { data(), size() };
}


template<class C, class T>
inline void BasicSeedMemoryStreambuf<C, T>::reserve_extra(std::size_t min_extra_capacity)
{
    std::size_t extra_capacity = std::size_t(this->epptr() - this->pptr());
    if (ARCHON_LIKELY(extra_capacity >= min_extra_capacity))
        return;
    do_reserve_extra(min_extra_capacity); // Throws
}


template<class C, class T>
void BasicSeedMemoryStreambuf<C, T>::append(const C* data, std::size_t size)
{
    reserve_extra(size); // Throws
    std::copy_n(data, size, this->pptr());
    bump_put_ptr(size);
}


template<class C, class T> void BasicSeedMemoryStreambuf<C, T>::append(std::size_t size, C value)
{
    reserve_extra(size); // Throws
    std::fill_n(this->pptr(), size, value);
    bump_put_ptr(size);
}


template<class C, class T> inline void BasicSeedMemoryStreambuf<C, T>::clear_contents() noexcept
{
    std::size_t capacity = std::size_t(this->epptr() - m_memory);
    this->setp(m_memory, m_memory + capacity);
}


template<class C, class T>
std::streamsize BasicSeedMemoryStreambuf<C, T>::xsputn(const C* s, std::streamsize count)
{
    if (ARCHON_LIKELY(count >= 0)) {
        std::streamsize offset = 0;
        using uint = std::make_unsigned_t<std::streamsize>;
        std::size_t max = std::numeric_limits<std::size_t>::max();
        if (ARCHON_UNLIKELY(uint(count) >= max)) {
            do {
                append(s + offset, max); // Throws
                offset += std::streamsize(max);
            }
            while (uint(count - offset) >= max);
        }
        append(s + offset, std::streamsize(count - offset)); // Throws
    }
    return count;
}


template<class C, class T> auto BasicSeedMemoryStreambuf<C, T>::overflow(int_type ch) -> int_type
{
    reserve_extra(1); // Throws
    if (!traits_type::eq_int_type(ch, traits_type::eof()))
        *this->pptr() = traits_type::to_char_type(ch);
    this->pbump(1);
    return 0;
}


template<class C, class T>
void BasicSeedMemoryStreambuf<C, T>::do_reserve_extra(std::size_t min_extra_capacity)
{
    std::size_t capacity = std::size_t(this->epptr() - m_memory);
    std::size_t used_size = size();
    std::size_t min_capacity = used_size;
    if (ARCHON_LIKELY(base::try_int_add(min_capacity, min_extra_capacity))) {
        std::size_t new_capacity = suggest_new_buffer_size(capacity, min_capacity);
        C* new_memory = new C[new_capacity]; // Throws
        std::copy_n(m_memory, used_size, new_memory);
        dealloc();
        m_memory = new_memory;
        this->setp(m_memory + used_size, m_memory + new_capacity);
        return;
    }
    throw std::length_error("Buffer size");
}


template<class C, class T> inline void BasicSeedMemoryStreambuf<C, T>::dealloc() noexcept
{
    bool has_allocation = (m_memory != m_seed_memory);
    if (!has_allocation)
        return;
    delete[] data();
}


template<class C, class T>
void BasicSeedMemoryStreambuf<C, T>::bump_put_ptr(std::size_t size) noexcept
{
    std::size_t size_2 = size;
    int max = std::numeric_limits<int>::max();
    if (ARCHON_UNLIKELY(size_2 >= unsigned(max))) {
        do {
            this->pbump(max);
            size_2 -= unsigned(max);
        }
        while (size_2 >= unsigned(max));
    }
    this->pbump(int(size_2));
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_SEED_MEMORY_OUTPUT_STREAM_HPP
