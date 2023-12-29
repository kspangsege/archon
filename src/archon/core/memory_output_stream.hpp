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

#ifndef ARCHON_X_CORE_X_MEMORY_OUTPUT_STREAM_HPP
#define ARCHON_X_CORE_X_MEMORY_OUTPUT_STREAM_HPP

/// \file


#include <cstddef>
#include <cwchar>
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <array>
#include <string_view>
#include <streambuf>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>


namespace archon::core {


template<class C, class T> class BasicMemoryOutputStreambuf;




/// \brief Fixed size in-memory output stream.
///
/// An output stream with an embedded fixed size in-memory output stream buffer (\ref
/// BasicMemoryOutputStreambuf).
///
template<class C, class T = std::char_traits<C>> class BasicMemoryOutputStream
    : public std::basic_ostream<C, T> {
public:
    using streambuf_type = BasicMemoryOutputStreambuf<C, T>;
    using string_view_type = typename streambuf_type::string_view_type;

    BasicMemoryOutputStream();
    ~BasicMemoryOutputStream() noexcept = default;

    /// \{
    ///
    /// With one exception, these functions have the same effect as the corresponding
    /// functions in \ref BasicMemoryOutputStreambuf.
    ///
    /// The exception is that \ref reset() clears the stream error state after invoking
    /// `reset()` on the stream buffer object.
    ///
    BasicMemoryOutputStream(core::Span<C> memory, std::size_t size = 0);
    void reset(core::Span<C> memory, std::size_t size = 0);
    auto get_base() noexcept -> C*;
    auto get_base() const noexcept -> const C*;
    auto get_capacity() const noexcept -> std::size_t;
    auto get_size() const noexcept -> std::size_t;
    auto view() const noexcept -> string_view_type;
    /// \}

private:
    streambuf_type m_streambuf;
};


template<class C, std::size_t N> explicit BasicMemoryOutputStream(C(&)[N]) -> BasicMemoryOutputStream<C>;
template<class C, std::size_t N> explicit BasicMemoryOutputStream(std::array<C, N>&) -> BasicMemoryOutputStream<C>;


using MemoryOutputStream     = BasicMemoryOutputStream<char>;
using WideMemoryOutputStream = BasicMemoryOutputStream<wchar_t>;




/// \brief Fixed size in-memory output stream buffer.
///
/// An output stream buffer that is associated with a simple memory buffer having a fixed
/// un-expandable capacity. If the buffer fills up (size becomes equal to capacity), further
/// writing will fail.
///
template<class C, class T = std::char_traits<C>> class BasicMemoryOutputStreambuf
    : public std::basic_streambuf<C, T> {
public:
    using off_type = typename std::basic_streambuf<C, T>::off_type;
    using pos_type = typename std::basic_streambuf<C, T>::pos_type;
    using string_view_type = std::basic_string_view<C, T>;

    BasicMemoryOutputStreambuf();
    ~BasicMemoryOutputStreambuf() noexcept = default;

    /// \brief Construct with buffer memory.
    ///
    /// Has the same effect as calling \ref reset() on default constructed stream buffer.
    ///
    explicit BasicMemoryOutputStreambuf(core::Span<C> memory, std::size_t size = 0);

    /// \brief Set a new underlying memory buffer.
    ///
    /// Associate this stream buffer with a new underlying memory buffer.
    ///
    /// \param size The amount of contents to be considered already present in the specified
    /// memory buffer. Another way to think of this, is as the offset of the end of the used
    /// part of the buffer. If matters when seeking "relative to the end" and for what is
    /// returned by \ref view(). It is an error to specify a size greater than the size of
    /// the specified memory buffer.
    ///
    /// The initial writing position will be at the beginning of the memory buffer regarless
    /// of the value passed for \p size. If this is not the desired writing position, it can
    /// be changed using `std::basic_ostream::seekp()`.
    ///
    /// Note: It is an error to specify a memory buffer whose size is larger than
    /// `std::numeric_limits<off_type>::max()`, or larger than
    /// `std::numeric_limits<off_type>::max() - 1` if \ref off_type is unsigned. This
    /// restriction is needed to ensure that all valid positions in the memory buffer are
    /// presentable in `off_type` without conflation with the special meaning of
    /// `off_type(-1)`. If the specified capacity is too large, these functions will throw.
    ///
    void reset(core::Span<C> memory, std::size_t size = 0);

    /// \{
    ///
    /// \brief Get base address of underlying memory buffer.
    ///
    /// Get the base address of the underlying memory buffer. This is the base address of
    /// the buffer that was passed to \ref reset(), or to the constructor, or null if \ref
    /// reset() was never called, and the stream buffer was default constructed.
    ///
    auto get_base() noexcept -> C*;
    auto get_base() const noexcept -> const C*;
    /// \}

    /// \brief Get capacity of the underlying memory buffer.
    ///
    /// Get the capacity of the underlying memory buffer. This is the size of the buffer
    /// that was passed to \ref reset(), or to the constructor, or zero if \ref reset() was
    /// never called, and the stream buffer was default constructed.
    ///
    auto get_capacity() const noexcept -> std::size_t;

    /// \brief Get amount of contents in the underlying memory buffer.
    ///
    /// Get the current amount of contents in the underlying memory buffer. Initially, this
    /// is the amount passed to \ref reset(), or to the constructor as \p size. At any time
    /// after that, the end of the contents of the buffer is the highest reached writing
    /// position, or the original end, whichever is greater.
    ///
    auto get_size() const noexcept -> std::size_t;

    /// \brief Get a view of the contents of the underlying memory buffer.
    ///
    /// Return a view of the current contents of the memory buffer, effectively
    /// `string_view_type(get_base(), get_size())`.
    ///
    auto view() const noexcept -> string_view_type;

private:
    // The highest position reached prior to the last seek operation, or the initial size,
    // whichever is greater.
    //
    std::size_t m_size_x = 0;

    auto seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) -> pos_type override final;
    auto seekpos(pos_type, std::ios_base::openmode) -> pos_type override final;

    auto seek(std::size_t pos) noexcept -> pos_type;
    void set_pos(std::size_t pos) noexcept;
    auto update_and_get_size_x() noexcept -> std::size_t;
    auto get_pos() const noexcept -> std::size_t;
    static bool is_init_mbstate(const std::mbstate_t&) noexcept;
};


template<class C, std::size_t N> explicit BasicMemoryOutputStreambuf(C(&)[N]) -> BasicMemoryOutputStreambuf<C>;
template<class C, std::size_t N> explicit BasicMemoryOutputStreambuf(std::array<C, N>&) ->
    BasicMemoryOutputStreambuf<C>;


using MemoryOutputStreambuf     = BasicMemoryOutputStreambuf<char>;
using WideMemoryOutputStreambuf = BasicMemoryOutputStreambuf<wchar_t>;








// Implementation


// ============================ BasicMemoryOutputStream ============================


template<class C, class T>
inline BasicMemoryOutputStream<C, T>::BasicMemoryOutputStream()
    : std::basic_ostream<C, T>(nullptr) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class C, class T>
inline BasicMemoryOutputStream<C, T>::BasicMemoryOutputStream(core::Span<C> memory, std::size_t size)
    : std::basic_ostream<C, T>(nullptr) // Throws
    , m_streambuf(memory, size) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class C, class T>
inline void BasicMemoryOutputStream<C, T>::reset(core::Span<C> memory, std::size_t size)
{
    m_streambuf.reset(memory, size); // Throws
    this->clear();
}


template<class C, class T>
inline auto BasicMemoryOutputStream<C, T>::get_base() noexcept -> C*
{
    return m_streambuf.get_base();
}


template<class C, class T>
inline auto BasicMemoryOutputStream<C, T>::get_base() const noexcept -> const C*
{
    return m_streambuf.get_base();
}


template<class C, class T>
inline auto BasicMemoryOutputStream<C, T>::get_capacity() const noexcept -> std::size_t
{
    return m_streambuf.get_capacity();
}


template<class C, class T>
inline auto BasicMemoryOutputStream<C, T>::get_size() const noexcept -> std::size_t
{
    return m_streambuf.get_size();
}


template<class C, class T>
inline auto BasicMemoryOutputStream<C, T>::view() const noexcept -> string_view_type
{
    return m_streambuf.view();
}



// ============================ BasicMemoryOutputStreambuf ============================


template<class C, class T>
inline BasicMemoryOutputStreambuf<C, T>::BasicMemoryOutputStreambuf()
    : std::basic_streambuf<C, T>() // Throws
{
}


template<class C, class T>
inline BasicMemoryOutputStreambuf<C, T>::BasicMemoryOutputStreambuf(core::Span<C> memory, std::size_t size)
    : std::basic_streambuf<C, T>() // Throws
{
    reset(memory, size); // Throws
}


template<class C, class T>
inline void BasicMemoryOutputStreambuf<C, T>::reset(core::Span<C> memory, std::size_t size)
{
    C* base = memory.data();
    std::size_t capacity = memory.size();
    ARCHON_ASSERT(capacity >= size);
    using off_lim = std::numeric_limits<off_type>;
    static_assert(off_lim::is_specialized);
    off_type max = (off_lim::is_signed ? off_lim::max() : off_lim::max() - 1);
    if (ARCHON_UNLIKELY(core::int_greater(capacity, max)))
        throw std::runtime_error("Buffer size");
    this->setp(base, base + capacity);
    m_size_x = size;
    if (ARCHON_UNLIKELY(m_size_x > capacity))
        m_size_x = capacity;
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::get_base() noexcept -> C*
{
    return this->pbase();
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::get_base() const noexcept -> const C*
{
    return this->pbase();
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::get_capacity() const noexcept -> std::size_t
{
    return std::size_t(this->epptr() - this->pbase());
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::get_size() const noexcept -> std::size_t
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
        if (ARCHON_LIKELY(core::try_int_add(pos, off)))
            return seek(pos);
    }
    return pos_type(off_type(-1));
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::seekpos(pos_type pos, std::ios_base::openmode which) -> pos_type
{
    if (ARCHON_LIKELY(is_init_mbstate(pos.state()) && which == std::ios_base::out)) {
        std::size_t pos_2 = 0;
        if (ARCHON_LIKELY(core::try_int_cast(off_type(pos), pos_2)))
            return seek(pos_2);
    }
    return pos_type(off_type(-1));
}


template<class C, class T>
inline auto BasicMemoryOutputStreambuf<C, T>::seek(std::size_t pos) noexcept -> pos_type
{
    // Note: For file streams, `pos` is understood as an index into the byte sequence that
    // makes up the file, even when `char_type` is not `char`. However, since
    // BasicMemoryOutputStreambuf has no underlying byte sequence (in particular when when
    // `char_type` is not `char`), `pos` is taken to be an index into a sequence of elements
    // of type `char_type`. This choice is consistent with GCC's implementation of
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
        // The capacity check in reset() ensures the following
        ARCHON_ASSERT(core::can_int_cast<off_type>(pos));
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
inline auto BasicMemoryOutputStreambuf<C, T>::update_and_get_size_x() noexcept -> std::size_t
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
inline auto BasicMemoryOutputStreambuf<C, T>::get_pos() const noexcept -> std::size_t
{
    return std::size_t(this->pptr() - this->pbase());
}


template<class C, class T>
inline bool BasicMemoryOutputStreambuf<C, T>::is_init_mbstate(const std::mbstate_t& mbstate) noexcept
{
    return (std::mbsinit(&mbstate) != 0);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_MEMORY_OUTPUT_STREAM_HPP
