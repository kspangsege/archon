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

#ifndef ARCHON_X_CORE_X_MEMORY_INPUT_STREAM_HPP
#define ARCHON_X_CORE_X_MEMORY_INPUT_STREAM_HPP

/// \file


#include <cstddef>
#include <cwchar>
#include <limits>
#include <array>
#include <string_view>
#include <stdexcept>
#include <streambuf>
#include <istream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/integer.hpp>


namespace archon::core {


template<class C, class T> class BasicMemoryInputStreambuf;




/// \brief Fixed size in-memory input stream.
///
/// An input stream with an embedded fixed size in-memory input stream buffer (\ref
/// BasicMemoryInputStreambuf).
///
template<class C, class T = std::char_traits<C>> class BasicMemoryInputStream
    : public std::basic_istream<C, T> {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using streambuf_type   = core::BasicMemoryInputStreambuf<C, T>;

    BasicMemoryInputStream();
    ~BasicMemoryInputStream() noexcept = default;

    /// \brief Construct input stream from chunk of memory.
    ///
    /// This constructor has the same effect as calling \ref reset() on a default
    /// constructed input stream.
    ///
    explicit BasicMemoryInputStream(core::StringSpan<C> memory);

    /// \brief Read from new chunk of memory.
    ///
    /// This function has the same effect as \ref BasicMemoryInputStreambuf::reset() except
    /// that it also clears the stream error state.
    ///
    void reset(core::StringSpan<C> memory);

    /// \brief Get associated memory chunk.
    ///
    /// This function has the same effect as \ref BasicMemoryInputStreambuf::view().
    ///
    auto view() const noexcept -> string_view_type;

private:
    streambuf_type m_streambuf;
};


using MemoryInputStream     = BasicMemoryInputStream<char>;
using WideMemoryInputStream = BasicMemoryInputStream<wchar_t>;




/// \brief Fixed size in-memory input stream buffer.
///
/// A stream buffer that facilitates input from a fixed size chunk of memory.
///
template<class C, class T = std::char_traits<C>> class BasicMemoryInputStreambuf
    : public std::basic_streambuf<C, T> {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    using off_type = typename std::basic_streambuf<C, T>::off_type;
    using pos_type = typename std::basic_streambuf<C, T>::pos_type;

    BasicMemoryInputStreambuf();
    ~BasicMemoryInputStreambuf() noexcept = default;

    /// \brief Construct stream buffer from chunk of memory.
    ///
    /// This constructor has the same effect as calling \ref reset() on a default
    /// constructed stream buffer.
    ///
    explicit BasicMemoryInputStreambuf(core::StringSpan<C> memory);

    /// \brief Read from new chunk of memory.
    ///
    /// This function associates this stream buffer with the specified chunk of memory, and
    /// resets the reading position to zero (beginning of specified chunk). After this, the
    /// reading position can be changed using std::basic_istream::seekg().
    ///
    /// Note: It is an error to specify a chunk of memory with a size that is greater than
    /// `std::numeric_limits<off_type>::max()`, or greater than
    /// `std::numeric_limits<off_type>::max() - 1` if \ref off_type is unsigned. This
    /// restriction is needed to ensure that all valid positions in the specified memory
    /// chunk are representable in `off_type` without conflation with the special meaning of
    /// `off_type(-1)`. If the specified chunk size is too hight, this functions will throw.
    ///
    void reset(core::StringSpan<C> memory);

    /// \brief Get associated memory chunk.
    ///
    /// Get the memory chunk that was last passed to \ref reset(), or to the constructor, or
    /// an empty chunk if \ref reset() was enver called, and the stream buffer was default
    /// constructed.
    ///
    auto view() const noexcept -> string_view_type;

private:
    auto showmanyc() -> std::streamsize override;
    auto seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) -> pos_type override;
    auto seekpos(pos_type, std::ios_base::openmode) -> pos_type override;

    auto seek(std::size_t pos) noexcept -> pos_type;
    void set_pos(std::size_t pos) noexcept;
    auto get_pos() const noexcept -> std::size_t;
    auto get_size() const noexcept -> std::size_t;
    static bool is_init_mbstate(const std::mbstate_t&) noexcept;
};


using MemoryInputStreambuf     = BasicMemoryInputStreambuf<char>;
using WideMemoryInputStreambuf = BasicMemoryInputStreambuf<wchar_t>;








// Implementation


// ============================ BasicMemoryInputStream ============================


template<class C, class T>
inline BasicMemoryInputStream<C, T>::BasicMemoryInputStream()
    : std::basic_istream<C, T>(nullptr) // Throws
    , m_streambuf() // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class C, class T>
inline BasicMemoryInputStream<C, T>::BasicMemoryInputStream(core::StringSpan<C> memory)
    : std::basic_istream<C, T>(nullptr) // Throws
    , m_streambuf(memory) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class C, class T>
inline void BasicMemoryInputStream<C, T>::reset(core::StringSpan<C> memory)
{
    m_streambuf.reset(memory); // Throws
    this->clear();
}


template<class C, class T>
inline auto BasicMemoryInputStream<C, T>::view() const noexcept -> string_view_type
{
    return m_streambuf.view();
}



// ============================ BasicMemoryInputStreambuf ============================


template<class C, class T>
inline BasicMemoryInputStreambuf<C, T>::BasicMemoryInputStreambuf()
    : std::basic_streambuf<C, T>() // Throws
{
}


template<class C, class T>
inline BasicMemoryInputStreambuf<C, T>::BasicMemoryInputStreambuf(core::StringSpan<C> memory)
    : std::basic_streambuf<C, T>() // Throws
{
    reset(memory); // Throws
}


template<class C, class T>
inline void BasicMemoryInputStreambuf<C, T>::reset(core::StringSpan<C> memory)
{
    using off_lim = std::numeric_limits<off_type>;
    static_assert(off_lim::is_specialized);
    off_type max = (off_lim::is_signed ? off_lim::max() : off_lim::max() - 1);
    if (ARCHON_UNLIKELY(core::int_greater(memory.size(), max)))
        throw std::runtime_error("Buffer size");
    // The following const cast is safe because the default "put back" behaviour is to fail
    // if an attempt is made to put back a character that is different from the one
    // originally read from that position.
    C* base = const_cast<C*>(memory.data());
    this->setg(base, base, base + memory.size());
}


template<class C, class T>
inline auto BasicMemoryInputStreambuf<C, T>::view() const noexcept -> string_view_type
{
    return { this->eback(), get_size() };
}


template<class C, class T>
inline auto BasicMemoryInputStreambuf<C, T>::showmanyc() -> std::streamsize
{
    return -1;
}


template<class C, class T>
inline auto BasicMemoryInputStreambuf<C, T>::seekoff(off_type off, std::ios_base::seekdir dir,
                                                     std::ios_base::openmode which) -> pos_type
{
    if (ARCHON_LIKELY(which == std::ios_base::in)) {
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
inline auto BasicMemoryInputStreambuf<C, T>::seekpos(pos_type pos, std::ios_base::openmode which) -> pos_type
{
    if (ARCHON_LIKELY(is_init_mbstate(pos.state()) && which == std::ios_base::in)) {
        std::size_t pos_2 = 0;
        if (ARCHON_LIKELY(core::try_int_cast(off_type(pos), pos_2)))
            return seek(pos_2);
    }
    return pos_type(off_type(-1));
}


template<class C, class T>
inline auto BasicMemoryInputStreambuf<C, T>::seek(std::size_t pos) noexcept -> pos_type
{
    // Note: For file streams, `pos` is understood as an index into the byte sequence that
    // makes up the file, even when `char_type` is not `char`. However, since
    // BasicMemoryInputStreambuf has no underlying byte sequence (in particular when when
    // `char_type` is not `char`), `pos` is taken to be an index into a sequence of elements
    // of type `char_type`. This choice is consistent with GCC's implementation of
    // `std::basic_istringstream`.
    std::size_t size = get_size();
    if (ARCHON_LIKELY(pos <= size)) {
        set_pos(pos);
        // The size check in reset() ensures the following
        ARCHON_ASSERT(core::can_int_cast<off_type>(pos));
        ARCHON_ASSERT(off_type(pos) != off_type(-1));
        return pos_type(off_type(pos));
    }
    return pos_type(off_type(-1));
}


template<class C, class T>
inline void BasicMemoryInputStreambuf<C, T>::set_pos(std::size_t pos) noexcept
{
    C* base = this->eback();
    this->setg(base, base + pos, this->egptr());
}


template<class C, class T>
inline auto BasicMemoryInputStreambuf<C, T>::get_pos() const noexcept -> std::size_t
{
    return std::size_t(this->gptr() - this->eback());
}


template<class C, class T>
inline auto BasicMemoryInputStreambuf<C, T>::get_size() const noexcept -> std::size_t
{
    return std::size_t(this->egptr() - this->eback());
}


template<class C, class T>
inline bool BasicMemoryInputStreambuf<C, T>::is_init_mbstate(const std::mbstate_t& mbstate) noexcept
{
    return (std::mbsinit(&mbstate) != 0);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_MEMORY_INPUT_STREAM_HPP
