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


/// \file

#ifndef ARCHON__BASE__MEMORY_OUTPUT_STREAM_HPP
#define ARCHON__BASE__MEMORY_OUTPUT_STREAM_HPP

#include <cstddef>
#include <cwchar>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <streambuf>
#include <ostream>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/integer.hpp>


namespace archon::base {


/// \brief Fixed size in-memory output stream buffer.
///
/// An output stream buffer that is associated with an in-memory sink buffer
/// having a fixed un-expandable capacity. If the buffer fills up (size becomes
/// equal to capacity), further writing will fail.
///
template<class C, class T = std::char_traits<C>> class BasicMemoryOutputStreambuf :
        public std::basic_streambuf<C, T> {
public:
    using off_type = typename std::basic_streambuf<C, T>::off_type;
    using pos_type = typename std::basic_streambuf<C, T>::pos_type;
    using string_view_type = std::basic_string_view<C, T>;

    BasicMemoryOutputStreambuf();
    ~BasicMemoryOutputStreambuf() noexcept = default;

    /// \{
    ///
    /// \brief Construct with buffer.
    ///
    /// Has the same effect as calling set_buffer() on default constructed
    /// stream buffer.
    ///
    BasicMemoryOutputStreambuf(C* base, std::size_t capacity, std::size_t size = 0);
    template<std::size_t N>
    explicit BasicMemoryOutputStreambuf(C (&buffer)[N], std::size_t size = 0);
    /// \}

    /// \{
    ///
    /// \brief Set a new underlying buffer.
    ///
    /// Associate this stream buffer with a new underlying sink buffer.
    ///
    /// Even if \p size is nonzero, the initial writing position is always set
    /// to zero. The writing position can be changed using
    /// std::basic_ostream::seekp().
    ///
    /// \param base The base address of the new sink buffer.
    ///
    /// \param capacity The capacity of the new sink buffer (its size).
    ///
    /// \param size The amount of contents already present in the new sink
    /// buffer.
    ///
    /// Note: It is an error to specify a buffer with a capacity (\p capacity)
    /// that is greater than `std::numeric_limits<off_type>::max()`, or greater
    /// than `std::numeric_limits<off_type>::max() - 1` if \ref off_type is
    /// unsigned. This restriction is needed to ensure that all valid positions
    /// in the buffer are presentable in `off_type` without conflation with the
    /// special meaning of `off_type(-1)`. If the specified capacity is too
    /// hight, these functions will throw.
    ///
    void set_buffer(C* base, std::size_t capacity, std::size_t size = 0);
    template<std::size_t N> void set_buffer(C (&buffer)[N], std::size_t size = 0);
    /// \}

    /// \{
    ///
    /// \brief Get base address of underlying buffer.
    ///
    /// Get the base address of the underlying sink buffer. This is the base
    /// address that was passed to set_buffer(), or to the constructor, or null
    /// if set_buffer() was never called, and the stream buffer was default
    /// constructed.
    ///
    C* get_base() noexcept;
    const C* get_base() const noexcept;
    /// \}

    /// \brief Get total capacity of the underlying buffer.
    ///
    /// Get the total capacity of the underlying sink buffer. This is the
    /// capacity that was passed to set_buffer(), or to the constructor, or zero
    /// if set_buffer() was never called, and the stream buffer was default
    /// constructed.
    ///
    std::size_t get_capacity() const noexcept;

    /// \brief Get amount of contents in the underlying buffer.
    ///
    /// Get the current amount of contents in the underlying sink
    /// buffer. Initially, this is the amount passed to set_buffer(), or to the
    /// constructor as \p size. At any time after that, the end of the contents
    /// of the buffer is the highest reached writing position, or the original
    /// end, whichever is greater.
    ///
    std::size_t get_size() const noexcept;

    /// \brief Get a view of the contents of the underlying buffer.
    ///
    /// Return a view of the current contents of the sink buffer, effectively
    /// `string_view_type(get_base(), get_size())`.
    ///
    string_view_type view() const noexcept;

private:
    // The highest position reached prior to the last seek operation, or the
    // initial size, whichever is greater.
    //
    std::size_t m_size_x = 0;

    pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override final;
    pos_type seekpos(pos_type, std::ios_base::openmode) override final;

    pos_type seek(std::size_t pos) noexcept;
    void set_pos(std::size_t pos) noexcept;
    std::size_t update_and_get_size_x() noexcept;
    std::size_t get_pos() const noexcept;
    static bool is_init_mbstate(const std::mbstate_t&) noexcept;
};


using MemoryOutputStreambuf     = BasicMemoryOutputStreambuf<char>;
using WideMemoryOutputStreambuf = BasicMemoryOutputStreambuf<wchar_t>;




/// \brief Fixed size in-memory output stream.
///
/// An output stream with an embedded fixed size in-memory output stream buffer.
///
template<class C, class T = std::char_traits<C>> class BasicMemoryOutputStream :
        public std::basic_ostream<C, T> {
public:
    using streambuf_type = BasicMemoryOutputStreambuf<C, T>;
    using string_view_type = typename streambuf_type::string_view_type;

    BasicMemoryOutputStream();
    ~BasicMemoryOutputStream() noexcept = default;

    /// \{
    ///
    /// With one exception, these functions have the same effect as the
    /// corresponding function in \ref streambuf_type.
    ///
    /// The exception is that set_buffer() clears the stream error state after
    /// invoking set_buffer() in the stream buffer object.
    ///
    BasicMemoryOutputStream(C* base, std::size_t capacity, std::size_t size = 0);
    template<std::size_t N> explicit BasicMemoryOutputStream(C (&buffer)[N], std::size_t size = 0);
    void set_buffer(C* base, std::size_t capacity, std::size_t size = 0);
    template<std::size_t N> void set_buffer(C (&buffer)[N], std::size_t size = 0);
    C* get_base() noexcept;
    const C* get_base() const noexcept;
    std::size_t get_capacity() const noexcept;
    std::size_t get_size() const noexcept;
    string_view_type view() const noexcept;
    /// \}

private:
    streambuf_type m_streambuf;
};


using MemoryOutputStream     = BasicMemoryOutputStream<char>;
using WideMemoryOutputStream = BasicMemoryOutputStream<wchar_t>;








// Implementation


// ============================ BasicMemoryOutputStreambuf ============================


template<class C, class T> inline BasicMemoryOutputStreambuf<C, T>::BasicMemoryOutputStreambuf() :
    std::basic_streambuf<C, T>() // Throws
{
}


template<class C, class T>
inline BasicMemoryOutputStreambuf<C, T>::BasicMemoryOutputStreambuf(C* base, std::size_t capacity,
                                                                    std::size_t size) :
    std::basic_streambuf<C, T>() // Throws
{
    set_buffer(base, capacity, size); // Throws
}


template<class C, class T> template<std::size_t N>
inline BasicMemoryOutputStreambuf<C, T>::BasicMemoryOutputStreambuf(C (&buffer)[N],
                                                                    std::size_t size) :
    BasicMemoryOutputStreambuf(buffer, N, size) // Throws
{
}


template<class C, class T>
inline void BasicMemoryOutputStreambuf<C, T>::set_buffer(C* base, std::size_t capacity,
                                                         std::size_t size)
{
    ARCHON_ASSERT(capacity >= size);
    using off_lim = std::numeric_limits<off_type>;
    static_assert(off_lim::is_specialized);
    off_type max = (off_lim::is_signed ? off_lim::max() : off_lim::max() - 1);
    if (ARCHON_UNLIKELY(base::int_greater_than(capacity, max)))
        throw std::runtime_error("Buffer size");
    this->setp(base, base + capacity);
    m_size_x = size;
    if (ARCHON_UNLIKELY(capacity < m_size_x))
        m_size_x = capacity;
}


template<class C, class T> template<std::size_t N>
inline void BasicMemoryOutputStreambuf<C, T>::set_buffer(C (&buffer)[N], std::size_t size)
{
    set_buffer(buffer, N, size); // Throws
}


template<class C, class T> inline C* BasicMemoryOutputStreambuf<C, T>::get_base() noexcept
{
    return this->pbase();
}


template<class C, class T>
inline const C* BasicMemoryOutputStreambuf<C, T>::get_base() const noexcept
{
    return this->pbase();
}


template<class C, class T>
inline std::size_t BasicMemoryOutputStreambuf<C, T>::get_capacity() const noexcept
{
    return std::size_t(this->epptr() - this->pbase());
}


template<class C, class T>
inline std::size_t BasicMemoryOutputStreambuf<C, T>::get_size() const noexcept
{
    std::size_t pos = get_pos();
    return std::max(m_size_x, pos);
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::view() const noexcept -> string_view_type
{
    return { get_base(), get_size() };
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::seekoff(off_type off, std::ios_base::seekdir dir,
                                                      std::ios_base::openmode which) -> pos_type
{
    if (ARCHON_LIKELY(which == std::ios_base::out)) {
        std::size_t pos = 0;
        switch (dir) {
            case std::ios_base::end:
                pos = get_size();
                break;
            case std::ios_base::cur:
                pos = get_pos();
                break;
            default:
                break;
        }
        if (ARCHON_LIKELY(base::try_int_add(pos, off)))
            return seek(pos);
    }
    return pos_type(off_type(-1));
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::seekpos(pos_type pos,
                                                      std::ios_base::openmode which) -> pos_type
{
    if (ARCHON_LIKELY(is_init_mbstate(pos.state()) && which == std::ios_base::out)) {
        std::size_t pos_2 = 0;
        if (ARCHON_LIKELY(base::try_int_cast(off_type(pos), pos_2)))
            return seek(pos_2);
    }
    return pos_type(off_type(-1));
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::seek(std::size_t pos) noexcept -> pos_type
{
    // Note: For file streams, `pos` is understood as an index into the byte
    // sequence that makes up the file, even when `char_type` is not
    // `char`. However, since BasicMemoryOutputStreambuf has no underlying byte
    // sequence (in particular when when `char_type` is not `char`), `pos` is
    // taken to be an index into a sequence of elements of type
    // `char_type`. This choice is consistent with GCC's implementation of
    // `std::basic_ostringstream`.
    //
    std::size_t capacity = get_capacity();
    if (ARCHON_LIKELY(pos <= capacity)) {
        std::size_t size = update_and_get_size_x();
        if (pos >= size) {
            C* base = get_base();
            std::fill(base + size, base + pos, C());
        }
        set_pos(pos);
        // The capacity check in set_buffer() ensures the following
        ARCHON_ASSERT(base::can_int_cast<off_type>(pos));
        ARCHON_ASSERT(off_type(pos) != off_type(-1));
        return pos_type(off_type(pos));
    }
    return pos_type(off_type(-1));
}


template<class C, class T>
inline void BasicMemoryOutputStreambuf<C, T>::set_pos(std::size_t pos) noexcept
{
    std::size_t orig_pos = get_pos();
    if (pos >= orig_pos) {
        std::size_t inc = pos - orig_pos;
        int max = std::numeric_limits<int>::max();
        if (ARCHON_UNLIKELY(inc >= unsigned(max))) {
            do {
                this->pbump(max);
                inc -= unsigned(max);
            }
            while (inc >= unsigned(max));
        }
        this->pbump(int(inc));
    }
    else {
        std::size_t dec = orig_pos - pos;
        int max = std::numeric_limits<int>::max();
        if (ARCHON_UNLIKELY(dec >= unsigned(max))) {
            do {
                this->pbump(-max);
                dec -= unsigned(max);
            }
            while (dec >= unsigned(max));
        }
        this->pbump(-int(dec));
    }
}


template<class C, class T>
inline std::size_t BasicMemoryOutputStreambuf<C, T>::update_and_get_size_x() noexcept
{
    std::size_t pos = get_pos();
    std::size_t size = m_size_x;
    if (pos > size) {
        size = pos;
        m_size_x = size;
    }
    return size;
}


template<class C, class T>
inline std::size_t BasicMemoryOutputStreambuf<C, T>::get_pos() const noexcept
{
    return std::size_t(this->pptr() - this->pbase());
}


template<class C, class T>
inline bool BasicMemoryOutputStreambuf<C, T>::is_init_mbstate(const std::mbstate_t& mbstate) noexcept
{
    return (std::mbsinit(&mbstate) != 0);
}


// ============================ BasicMemoryOutputStream ============================


template<class C, class T>
inline BasicMemoryOutputStream<C, T>::BasicMemoryOutputStream() :
    std::basic_ostream<C, T>(nullptr) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class C, class T>
inline BasicMemoryOutputStream<C, T>::BasicMemoryOutputStream(C* base, std::size_t capacity,
                                                              std::size_t size) :
    std::basic_ostream<C, T>(nullptr), // Throws
    m_streambuf(base, capacity, size) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class C, class T> template<std::size_t N>
inline BasicMemoryOutputStream<C, T>::BasicMemoryOutputStream(C (&buffer)[N], std::size_t size) :
    BasicMemoryOutputStream(buffer, N, size) // Throws
{
}


template<class C, class T>
inline void BasicMemoryOutputStream<C, T>::set_buffer(C* base, std::size_t capacity,
                                                      std::size_t size)
{
    m_streambuf.set_buffer(base, capacity, size); // Throws
    this->clear();
}


template<class C, class T> template<std::size_t N>
inline void BasicMemoryOutputStream<C, T>::set_buffer(C (&buffer)[N], std::size_t size)
{
    set_buffer(buffer, N, size); // Throws
}


template<class C, class T> inline C* BasicMemoryOutputStream<C, T>::get_base() noexcept
{
    return m_streambuf.get_base();
}


template<class C, class T> inline const C* BasicMemoryOutputStream<C, T>::get_base() const noexcept
{
    return m_streambuf.get_base();
}


template<class C, class T>
inline std::size_t BasicMemoryOutputStream<C, T>::get_capacity() const noexcept
{
    return m_streambuf.get_capacity();
}


template<class C, class T>
inline std::size_t BasicMemoryOutputStream<C, T>::get_size() const noexcept
{
    return m_streambuf.get_size();
}


template<class C, class T>
inline auto BasicMemoryOutputStream<C, T>::view() const noexcept -> string_view_type
{
    return m_streambuf.view();
}


} // namespace archon::base

#endif // ARCHON__BASE__MEMORY_OUTPUT_STREAM_HPP
