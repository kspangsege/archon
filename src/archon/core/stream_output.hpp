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

#ifndef ARCHON_X_CORE_X_STREAM_OUTPUT_HPP
#define ARCHON_X_CORE_X_STREAM_OUTPUT_HPP

/// \file


#include <type_traits>
#include <array>
#include <optional>
#include <string_view>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/seed_memory_output_stream.hpp>


namespace archon::core {


/// \brief Implementation aid for stream output operators.
///
/// This function can be used to simplify the implementation of stream output operators,
/// especially in cases where you want to bypass the stream-level functions `put()` and
/// `write()`, and write directly to the underlying streambuffer for performance reasons.
///
/// This function constructs a sentry object as required, and then calls the specified
/// function (\p func).
///
/// If the specified function throws, `ostream_sentry()` will catch the exception and set
/// the error state of the stream object appropriately. The exception will not be rethrown,
/// but if exceptions are enabled on the stream for `std::ios_base::badbit`, an exception of
/// type `std::ios_base::failure` will be thrown in its place. This "unnatural" behaviour is
/// unfortunately mandated by the C++ standard (C++20).
///
/// If the return type of the specified function is not `void`, it must be something that is
/// convertible to bool. In that case, if the returned value evaluates to `false`, it is
/// taken as a signal of failure, and the error state of the stream object is set
/// appropriately.
///
/// This function will construct a \ref BasicStreamOutputHelper object, and pass it to the
/// specified function (no other arguments will be passed). This object simplifies writing
/// to the stream buffer. It also handles field adjustments by accumulating output in memory
/// and writing it to the stream as a single string object when the field width is non-zero.
///
/// Here is an example of how it can be used:
///
/// \code{.cpp}
///
///   std::ostream& operator<<(std::ostream& out, const Foo& foo)
///   {
///       using namespace ac = archon::core;
///       return ac::ostream_sentry(out, [&](ac::StreamOutputHelper& helper) {
///           helper.write(foo.part_1);
///           helper.write(foo.part_2);
///       });
///   }
///
/// \endcode
///
/// If seed memory is passed, it will be used as the initial memory buffer for accumulating
/// output when the field width of the target stream is non-zero.
///
/// \sa \ref BasicStreamOutputAltHelper
/// \sa \ref istream_sentry()
///
template<class C, class T, class F>
auto ostream_sentry(std::basic_ostream<C, T>&, F func, core::Span<C> seed_memory = {}) -> std::basic_ostream<C, T>&;



/// \brief Simplify writing to stream buffer object.
///
/// This class is used by \ref ostream_sentry() to make it easier to write directly to a
/// stream buffer object, bypassing the stream object.
///
/// Functions \ref put() and \ref write() may be used once either \ref init(), or \ref
/// init_with_field_width_check() has been called. If init_with_field_width_check() is
/// called, it is necessary to follow up with a call to \ref flush() once all output has
/// been submitted.
///
/// If \ref init_with_field_width_check() is called and the field width of the output stream
/// (`std::basic_ostream::width()`) is non-zero, submitted output will be buffered, and only
/// forwarded to the output stream once \ref flush() is called. At that time, it will be
/// written to the output stream as a single string object to allow for proper field
/// adjustments.
///
/// If \ref init() is called, or if the field width of the output stream is zero, submitted
/// output will be sent directly to the stream buffer.
///
template<class C, class T = std::char_traits<C>> class BasicStreamOutputHelper {
public:
    using char_type        = C;
    using traits_type      = T;
    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;

    /// \{
    ///
    /// \brief Write the specified character or string.
    ///
    /// Unless output is buffered (see class-level documentation), these functions pass the
    /// specified character or string to `sputc()` or `sputn()` on the stream buffer
    /// respectively. The error status from `sputn()` is registered internally in the helper
    /// object.
    ///
    void put(char_type);
    void write(string_view_type);
    /// \}

    BasicStreamOutputHelper(ostream_type&) noexcept;
    void init();
    void init_with_field_width_check(core::Span<C> seed_memory = {});
    void flush();

private:
    using streambuf_type = std::basic_streambuf<C, T>;

    class Rep;

    ostream_type& m_out;
    std::optional<Rep> m_rep;
    streambuf_type* m_streambuf = nullptr;
    bool m_error = false;
};


using StreamOutputHelper     = BasicStreamOutputHelper<char>;
using WideStreamOutputHelper = BasicStreamOutputHelper<wchar_t>;




namespace impl {
template<class C, class T> class StreamOutputAltHelperBase;
} // namespace impl




/// \brief Helper for field width support in stream output operators.
///
/// This class is intended to be used when implementing stream output operators which are
/// supposed to respect field width specifications (`std::basic_ostream::width()`) and when
/// the number of generated characters is not known until after the completion of the
/// formatting process.
///
/// Here is an example of how it can be used:
///
/// \code{.cpp}
///
///   std::ostream& operator<<(std::ostream& out, const Foo& foo)
///   {
///       archon::core::BasicStreamOutputAltHelper helper(out);
///       helper.out << "Foo(" << foo.value << ")";
///       helper.flush();
///       return out;
///   }
///
/// \endcode
///
/// If a nonzero field width is specified in the main output stream (the one passed to the
/// constructor), \ref out is set to refer to a separate output stream object, which is
/// backed by an in-memory buffer. When \ref flush() is called, everything that was written
/// to this output stream will be forwarded to the main output stream as a single string
/// object, ensuring that field adjustments work as intended. The separate output stream
/// will be set up with the same locale, format flags and precision setting as the main
/// output stream.
///
/// If the field width is zero, \ref out is instead set to refer to the main output stream,
/// i.e., the one that is passed to the constructor.
///
template<class C, class T = std::char_traits<C>> class BasicStreamOutputAltHelper
    : private impl::StreamOutputAltHelperBase<C, T> {
public:
    using char_type    = C;
    using traits_type  = T;
    using ostream_type = std::basic_ostream<C, T>;

    /// \brief Output stream to be used in output operator implementations.
    ///
    /// If the field width in the main output stream was nonzero, this is a reference to a
    /// contained output stream which accumulates output in a memory buffer. If instead the
    /// field width was zero, this is a reference to the main output stream.
    ///
    ostream_type& out;

    explicit BasicStreamOutputAltHelper(ostream_type&, core::Span<C> seed_memory = {});

    /// \brief Write accumulated output to main output stream.
    ///
    /// If the field width in the main output stream was nonzero, this function writes the
    /// accumulated output as a single string object to the main output stream. If instead
    /// the field width was zero, this function does nothing.
    ///
    void flush();
};


template<class C, class T, std::size_t N> BasicStreamOutputAltHelper(std::basic_ostream<C, T>&, C (&)[N]) ->
    BasicStreamOutputAltHelper<C, T>;
template<class C, class T, std::size_t N> BasicStreamOutputAltHelper(std::basic_ostream<C, T>&, std::array<C, N>&) ->
    BasicStreamOutputAltHelper<C, T>;


using StreamOutputAltHelper     = BasicStreamOutputAltHelper<char>;
using WideStreamOutputAltHelper = BasicStreamOutputAltHelper<wchar_t>;








// Implementation


template<class C, class T, class F>
auto ostream_sentry(std::basic_ostream<C, T>& out, F func, core::Span<C> seed_memory) -> std::basic_ostream<C, T>&
{
    typename std::basic_ostream<C, T>::sentry sentry(out); // Throws
    if (ARCHON_LIKELY(sentry)) {
        BasicStreamOutputHelper<C, T> helper(out);
        bool success;
        try {
            helper.init_with_field_width_check(seed_memory); // Throws
            if constexpr (std::is_convertible_v<decltype(func(helper)), bool>) {
                success = func(helper); // Throws
            }
            else {
                static_assert(std::is_same_v<decltype(func(helper)), void>);
                func(helper); // Throws
                success = true;
            }
        }
        catch (...) {
            out.setstate(std::ios_base::badbit); // Throws
            return out;
        }
        helper.flush(); // Throws
        if (ARCHON_UNLIKELY(!success))
            out.setstate(std::ios_base::badbit); // Throws
    }
    return out;
}


template<class C, class T>
inline void BasicStreamOutputHelper<C, T>::put(char_type ch)
{
    if (ARCHON_LIKELY(!m_error)) {
        using int_type = typename traits_type::int_type;
        int_type ret = m_streambuf->sputc(ch);
        if (ARCHON_LIKELY(ret != traits_type::eof()))
            return;
        m_error = true;
    }
}


template<class C, class T>
inline void BasicStreamOutputHelper<C, T>::write(string_view_type string)
{
    if (ARCHON_LIKELY(!m_error)) {
        const char_type* data = string.data();
        std::size_t size = string.size();
        std::streamsize max_1 = std::numeric_limits<std::streamsize>::max();
        auto max_2 = core::to_unsigned(max_1);
        for (;;) {
            if (ARCHON_LIKELY(size <= max_2)) {
                std::streamsize size_2 = std::streamsize(size);
                std::streamsize n = m_streambuf->sputn(data, size_2);
                if (ARCHON_LIKELY(n == size_2))
                    return;
                break;
            }
            std::streamsize size_2 = max_1;
            std::streamsize n = m_streambuf->sputn(data, size_2);
            if (ARCHON_LIKELY(n == size_2)) {
                data += max_2;
                size = std::size_t(size - max_2);
                continue;
            }
            break;
        }
        m_error = true;
    }
}


template<class C, class T>
inline BasicStreamOutputHelper<C, T>::BasicStreamOutputHelper(ostream_type& out) noexcept
    : m_out(out)
{
}


template<class C, class T>
inline void BasicStreamOutputHelper<C, T>::init()
{
    m_streambuf = m_out.rdbuf(); // Throws
}


template<class C, class T>
inline void BasicStreamOutputHelper<C, T>::init_with_field_width_check(core::Span<C> seed_memory)
{
    if (ARCHON_LIKELY(m_out.width() == 0)) {
        m_streambuf = m_out.rdbuf(); // Throws
        return;
    }

    m_rep.emplace(seed_memory); // Throws
    m_streambuf = &m_rep->get_streambuf();
}


template<class C, class T>
inline void BasicStreamOutputHelper<C, T>::flush()
{
    if (ARCHON_LIKELY(!m_rep.has_value())) {
        if (ARCHON_LIKELY(!m_error))
            return;
        m_out.setstate(std::ios_base::badbit); // Throws
        return;
    }

    ARCHON_ASSERT(!m_error);
    m_rep->flush(m_out); // Throws
}


template<class C, class T>
class BasicStreamOutputHelper<C, T>::Rep {
public:
    Rep(core::Span<C> seed_memory)
        : m_streambuf(seed_memory) // Throws
    {
    }

    auto get_streambuf() noexcept -> streambuf_type&
    {
        return m_streambuf;
    }

    void flush(ostream_type& out)
    {
        out << m_streambuf.view(); // Throws
    }

private:
    core::BasicSeedMemoryStreambuf<C, T> m_streambuf;
};


template<class C, class T> class impl::StreamOutputAltHelperBase {
protected:
    using ostream_type = std::basic_ostream<C, T>;

    class Rep;
    std::optional<Rep> m_rep;

    StreamOutputAltHelperBase(ostream_type& out, core::Span<C> seed_memory)
    {
        if (ARCHON_LIKELY(out.width() == 0)) // Throws
            return;
        m_rep.emplace(out, seed_memory); // Throws
    }
};


template<class C, class T>
class impl::StreamOutputAltHelperBase<C, T>::Rep {
public:
    Rep(ostream_type& out, core::Span<C> seed_memory)
        : m_out_1(out)
        , m_out_2(seed_memory) // Throws
    {
        m_out_2.copyfmt(out); // Throws
        m_out_2.width(0); // Throws
        m_out_2.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    }

    auto get_out() noexcept -> ostream_type&
    {
        return m_out_2;
    }

    void flush()
    {
        m_out_1 << m_out_2.view(); // Throws
    }

private:
    ostream_type& m_out_1;
    core::BasicSeedMemoryOutputStream<C, T> m_out_2;
};


template<class C, class T>
inline BasicStreamOutputAltHelper<C, T>::BasicStreamOutputAltHelper(ostream_type& out_2, core::Span<C> seed_memory)
    : impl::StreamOutputAltHelperBase<C, T>(out_2, seed_memory)
    , out(this->m_rep.has_value() ? this->m_rep->get_out() : out_2)
{
}


template<class C, class T>
inline void BasicStreamOutputAltHelper<C, T>::flush()
{
    if (ARCHON_LIKELY(!this->m_rep.has_value()))
        return;
    this->m_rep->flush(); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STREAM_OUTPUT_HPP
