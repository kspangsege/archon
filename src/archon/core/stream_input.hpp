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

#ifndef ARCHON_X_CORE_X_STREAM_INPUT_HPP
#define ARCHON_X_CORE_X_STREAM_INPUT_HPP

/// \file


#include <string_view>
#include <streambuf>
#include <istream>

#include <archon/core/features.h>


namespace archon::core {


/// \brief Implementation aid for stream input operators.
///
/// This function can be used to simplify the implementation of stream input operators,
/// especially in cases where you want to bypass the stream-level functions `get()` and
/// `read()`, and read directly from the underlying streambuffer for performance reasons.
///
/// This function constructs a sentry object as required, and then calls the specified
/// function. The specified function must return `true` to indicate success. If it returns
/// `false`, `istream_sentry()` will update the error state of the stream object
/// appropriately.
///
/// If the specified function throws, `istream_sentry()` will catch the exception and set
/// the error state of the stream object appropriately. The exception will not be rethrown,
/// but if exceptions are enabled on the stream for `std::ios_base::badbit`, an exception of
/// type `std::ios_base::failure` will be thrown in its place. This "unnatural" behaviour is
/// unfortunately mandated by the C++ standard.
///
/// This function will construct a \ref BasicStreamInputHelper object, and pass it to the
/// specified function (no other arguments will be passed). This object simplifies reading
/// from the stream buffer, and it tracks whether the end of input has been observed.
///
/// \sa \ref ostream_sentry()
///
template<class C, class T, class F> auto istream_sentry(std::basic_istream<C, T>&, F func) ->
    std::basic_istream<C, T>&;



/// \brief Simplify reading from stream buffer object.
///
/// This class is used by \ref istream_sentry() to make it easier to read from a stream
/// buffer object, and to track whether the end of input has been observed.
///
template<class C, class T = std::char_traits<C>> class BasicStreamInputHelper {
public:
    using char_type      = C;
    using traits_type    = T;
    using streambuf_type = std::basic_streambuf<C, T>;

    /// \brief Peek at the next character.
    ///
    /// This function calls `sgetc()` on the associated stream buffer object. If `sgetc()`
    /// returns `T::eof()`, then this function returns false after setting the "end of
    /// input" flag to true (\ref end_of_input()). Otherwise this function sets \p ch to the
    /// extracted character and returns true.
    ///
    bool peek(C& ch);

    /// \brief Advance input position.
    ///
    /// This function calls `snextc()` on the associated stream buffer object.
    ///
    void discard();

    /// \brief Advance input position, then peek at the next character.
    ///
    /// This function calls `snextc()` on the associated stream buffer object. If `snextc()`
    /// returns `T::eof()`, then this function returns false after setting the "end of
    /// input" flag to true (\ref end_of_input()). Otherwise this function sets \p ch to the
    /// extracted character and returns true.
    ///
    bool next(C& ch);

    /// \brief Construct stream input helper.
    ///
    /// Construct a stream input helper with the specified associated stream buffer object.
    ///
    BasicStreamInputHelper(streambuf_type&);

    /// \ref Has end of input been observed?
    ///
    /// This function returns `true` if the end of input was seen by \ref peek(), \ref
    /// discard(), or \ref next().
    ///
    bool end_of_input() const noexcept;

private:
    streambuf_type& m_streambuf;
    bool m_end_of_input = false;
};


using StreamInputHelper     = BasicStreamInputHelper<char>;
using WideStreamInputHelper = BasicStreamInputHelper<wchar_t>;








// Implementation


template<class C, class T, class F>
auto istream_sentry(std::basic_istream<C, T>& in, F func) -> std::basic_istream<C, T>&
{
    typename std::basic_istream<C, T>::sentry sentry(in); // Throws
    if (ARCHON_LIKELY(sentry)) {
        std::ios_base::iostate state;
        try {
            BasicStreamInputHelper<C, T> helper(*in.rdbuf());
            bool success = func(helper); // Throws
            state = ((helper.end_of_input() ? std::ios_base::eofbit : std::ios_base::goodbit) |
                     (success ? std::ios_base::goodbit : std::ios_base::failbit));
        }
        catch (...) {
            state = std::ios_base::badbit;
        }
        in.setstate(state); // Throws
    }
    return in;
}


template<class C, class T>
inline bool BasicStreamInputHelper<C, T>::peek(C& ch)
{
    typename T::int_type val = m_streambuf.sgetc(); // Throws
    if (ARCHON_LIKELY(!T::eq_int_type(val, T::eof()))) {
        ch = T::to_char_type(val);
        return true;
    }
    m_end_of_input = true;
    return false;
}


template<class C, class T>
inline void BasicStreamInputHelper<C, T>::discard()
{
    typename T::int_type val = m_streambuf.snextc(); // Throws
    if (ARCHON_LIKELY(!T::eq_int_type(val, T::eof())))
        return;
    m_end_of_input = true;
}


template<class C, class T>
inline bool BasicStreamInputHelper<C, T>::next(C& ch)
{
    typename T::int_type val = m_streambuf.snextc(); // Throws
    if (ARCHON_LIKELY(!T::eq_int_type(val, T::eof()))) {
        ch = T::to_char_type(val);
        return true;
    }
    m_end_of_input = true;
    return false;
}


template<class C, class T>
inline BasicStreamInputHelper<C, T>::BasicStreamInputHelper(streambuf_type& streambuf)
    : m_streambuf(streambuf)
{
}


template<class C, class T>
inline bool BasicStreamInputHelper<C, T>::end_of_input() const noexcept
{
    return m_end_of_input;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STREAM_INPUT_HPP
