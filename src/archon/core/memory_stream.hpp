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

#ifndef ARCHON_CORE_MEMORY_STREAM_HPP
#define ARCHON_CORE_MEMORY_STREAM_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <istream>
#include <ostream>

namespace archon {
namespace core {

template<class C, class T = std::char_traits<C>>
class BasicMemoryInputStreambuf: public std::basic_streambuf<C,T> {
public:
    using traits_type = typename std::basic_streambuf<C,T>::traits_type;
    using int_type    = typename std::basic_streambuf<C,T>::int_type;
    using pos_type    = typename std::basic_streambuf<C,T>::pos_type;
    using off_type    = typename std::basic_streambuf<C,T>::off_type;

    BasicMemoryInputStreambuf();
    ~BasicMemoryInputStreambuf() noexcept;

    /// Behavior is undefined if the size of the specified buffer exceeds
    /// PTRDIFF_MAX.
    void set_buffer(const C* begin, const C* end) noexcept;

private:
    const C* m_begin;
    const C* m_end;
    const C* m_curr;

    int_type underflow() override;
    int_type uflow() override;
    int_type pbackfail(int_type) override;
    std::streamsize showmanyc() override;
    pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override;
    pos_type seekpos(pos_type, std::ios_base::openmode) override;

    pos_type do_seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
};

using MemoryInputStreambuf     = BasicMemoryInputStreambuf<char>;
using WideMemoryInputStreambuf = BasicMemoryInputStreambuf<wchar_t>;


template<class C, class T = std::char_traits<C>>
class BasicMemoryOutputStreambuf: public std::basic_streambuf<C,T> {
public:
    BasicMemoryOutputStreambuf();
    ~BasicMemoryOutputStreambuf() noexcept;

    /// Behavior is undefined if the size of the specified buffer exceeds
    /// PTRDIFF_MAX.
    void set_buffer(C* begin, C* end) noexcept;

    /// Returns the amount of data written to the buffer.
    std::size_t size() const noexcept;
};

using MemoryOutputStreambuf     = BasicMemoryOutputStreambuf<char>;
using WideMemoryOutputStreambuf = BasicMemoryOutputStreambuf<wchar_t>;


template<class C, class T = std::char_traits<C>>
class BasicMemoryInputStream: public std::basic_istream<C,T> {
public:
    using traits_type = typename std::basic_istream<C,T>::traits_type;

    BasicMemoryInputStream();
    ~BasicMemoryInputStream() noexcept;

    /// \{ Behavior is undefined if the size of the specified buffer exceeds
    /// PTRDIFF_MAX.
    void set_buffer(const C* begin, const C* end) noexcept;
    template<std::size_t N> void set_buffer(const C (&buffer)[N]) noexcept;
    template<class A> void set_string(const std::basic_string<C,T,A>&) noexcept;
    void set_c_string(const C* c_str) noexcept;
    /// \}

private:
    BasicMemoryInputStreambuf<C,T> m_streambuf;
};

using MemoryInputStream     = BasicMemoryInputStream<char>;
using WideMemoryInputStream = BasicMemoryInputStream<wchar_t>;


template<class C, class T = std::char_traits<C>>
class BasicMemoryOutputStream: public std::basic_ostream<C,T> {
public:
    BasicMemoryOutputStream();
    ~BasicMemoryOutputStream() noexcept;

    /// \{ Behavior is undefined if the size of the specified buffer exceeds
    /// PTRDIFF_MAX.
    void set_buffer(C* begin, C* end) noexcept;
    template<std::size_t N> void set_buffer(C (&buffer)[N]) noexcept;
    /// \}

    /// Returns the amount of data written to the installed buffer
    /// (the one passed to set_buffer()).
    std::size_t size() const noexcept;

private:
    BasicMemoryOutputStreambuf<C,T> m_streambuf;
};

using MemoryOutputStream     = BasicMemoryOutputStream<char>;
using WideMemoryOutputStream = BasicMemoryOutputStream<wchar_t>;




// Implementation

template<class C, class T>
inline BasicMemoryInputStreambuf<C,T>::BasicMemoryInputStreambuf():
    m_begin{nullptr},
    m_end{nullptr},
    m_curr{nullptr}
{
}

template<class C, class T>
inline BasicMemoryInputStreambuf<C,T>::~BasicMemoryInputStreambuf() noexcept
{
}

template<class C, class T>
inline void BasicMemoryInputStreambuf<C,T>::set_buffer(const C* begin, const C* end) noexcept
{
    m_begin = begin;
    m_end = end;
    m_curr = begin;
}

template<class C, class T>
inline auto BasicMemoryInputStreambuf<C,T>::underflow() -> int_type
{
    if (m_curr == m_end)
        return traits_type::eof();
    return traits_type::to_int_type(*m_curr);
}

template<class C, class T>
inline auto BasicMemoryInputStreambuf<C,T>::uflow() -> int_type
{
    if (m_curr == m_end)
        return traits_type::eof();
    return traits_type::to_int_type(*m_curr++);
}

template<class C, class T>
inline auto BasicMemoryInputStreambuf<C,T>::pbackfail(int_type ch) -> int_type
{
    if (m_curr == m_begin || (ch != traits_type::eof() && ch != m_curr[-1]))
        return traits_type::eof();
    return traits_type::to_int_type(*--m_curr);
}

template<class C, class T>
inline std::streamsize BasicMemoryInputStreambuf<C,T>::showmanyc()
{
    std::ptrdiff_t n = m_end - m_curr;
    std::streamsize max = std::numeric_limits<std::streamsize>::max();
    return (n <= max ? std::streamsize(n) : max);
}

template<class C, class T>
inline auto BasicMemoryInputStreambuf<C,T>::seekoff(off_type offset, std::ios_base::seekdir dir,
                                                    std::ios_base::openmode which) -> pos_type
{
    return do_seekoff(offset, dir, which);
}

template<class C, class T>
inline auto BasicMemoryInputStreambuf<C,T>::seekpos(pos_type pos,
                                                    std::ios_base::openmode which) -> pos_type
{
    off_type offset = off_type(pos);
    return do_seekoff(offset, std::ios_base::beg, which);
}

template<class C, class T>
inline auto BasicMemoryInputStreambuf<C,T>::do_seekoff(off_type offset, std::ios_base::seekdir dir,
                                                       std::ios_base::openmode which) -> pos_type
{
    // off_type is guaranteed to be std::streamoff when traits_type is
    // std::char_traits<char> or std::char_traits<wchar_t>. std::streamoff is
    // guaranteed to be signed.
    using off_lim = std::numeric_limits<off_type>;
    static_assert(off_lim::is_signed, "");
    // For this function to work properly, the size of the installed buffer must
    // never exceed the maximum stream size (off_type). To avoid checking the
    // size of the buffer at run time, we simply assume that pos_type is at
    // least as wide as std::ptrdiff_t. While this constraint is not necessary,
    // it is sufficient due to the requirement that the size of the installed
    // buffer (set_buffer()) is less than the maximum value of std::ptrdiff_t.
    static_assert(off_lim::max() >= std::numeric_limits<std::ptrdiff_t>::max(), "");
    const C* anchor;
    if (which == std::ios_base::in) {
        // Note: For file streams, `offset` is understood as an index into the
        // byte sequence that makes up the file (even when `char_type` is not
        // `char`). However, since BasicMemoryInputStreambuf generally has no
        // underlying byte sequence (in particular when when `char_type` is not
        // `char`), `offset` is taken to be an index into a sequence of elements
        // of type `char_type`. This choice is consistent with GCC's
        // implementation of `std::basic_istringstream`.
        switch (dir) {
            case std::ios_base::beg:
                anchor = m_begin;
                goto good;
            case std::ios_base::cur:
                anchor = m_curr;
                goto good;
            case std::ios_base::end:
                anchor = m_end;
                goto good;
            default:
                break;
        }
    }
    goto bad;

  good:
    if (offset >= (m_begin - anchor) && offset <= (m_end - anchor)) {
        m_curr = anchor + std::ptrdiff_t(offset);
        return pos_type(off_type(m_curr - m_begin));
    }

  bad:
    return pos_type(off_type(-1)); // Error
}

template<class C, class T>
inline BasicMemoryOutputStreambuf<C,T>::BasicMemoryOutputStreambuf()
{
}

template<class C, class T>
inline BasicMemoryOutputStreambuf<C,T>::~BasicMemoryOutputStreambuf() noexcept
{
}

template<class C, class T>
inline void BasicMemoryOutputStreambuf<C,T>::set_buffer(C* begin, C* end) noexcept
{
    setp(begin, end);
}

template<class C, class T>
inline std::size_t BasicMemoryOutputStreambuf<C,T>::size() const noexcept
{
    return std::size_t(this->pptr() - this->pbase());
}


template<class C, class T>
inline BasicMemoryInputStream<C,T>::BasicMemoryInputStream():
    std::istream{&m_streambuf}
{
}

template<class C, class T>
inline BasicMemoryInputStream<C,T>::~BasicMemoryInputStream() noexcept
{
}

template<class C, class T>
inline void BasicMemoryInputStream<C,T>::set_buffer(const C* begin, const C* end) noexcept
{
    m_streambuf.set_buffer(begin, end);
    this->clear();
}

template<class C, class T> template<std::size_t N>
inline void BasicMemoryInputStream<C,T>::set_buffer(const C (&buffer)[N]) noexcept
{
    const C* begin = buffer;
    const C* end = begin + N;
    set_buffer(begin, end);
}

template<class C, class T> template<class A>
inline void BasicMemoryInputStream<C,T>::set_string(const std::basic_string<C,T,A>& str) noexcept
{
    const C* begin = str.data();
    const C* end = begin + str.size();
    set_buffer(begin, end);
}

template<class C, class T>
inline void BasicMemoryInputStream<C,T>::set_c_string(const C* c_str) noexcept
{
    const C* begin = c_str;
    const C* end = begin + traits_type::length(c_str);
    set_buffer(begin, end);
}


template<class C, class T>
inline BasicMemoryOutputStream<C,T>::BasicMemoryOutputStream():
    std::ostream{&m_streambuf}
{
}

template<class C, class T>
inline BasicMemoryOutputStream<C,T>::~BasicMemoryOutputStream() noexcept
{
}

template<class C, class T>
inline void BasicMemoryOutputStream<C,T>::set_buffer(C* begin, C* end) noexcept
{
    m_streambuf.set_buffer(begin, end);
    this->clear();
}

template<class C, class T> template<std::size_t N>
inline void BasicMemoryOutputStream<C,T>::set_buffer(C (&buffer)[N]) noexcept
{
    C* begin = buffer;
    C* end = begin + N;
    set_buffer(begin, end);
}

template<class C, class T>
inline std::size_t BasicMemoryOutputStream<C,T>::size() const noexcept
{
    return m_streambuf.size();
}

} // namespace core
} // namespace archon

#endif // ARCHON_CORE_MEMORY_STREAM_HPP
