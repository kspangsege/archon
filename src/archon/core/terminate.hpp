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

#ifndef ARCHON_X_CORE_X_TERMINATE_HPP
#define ARCHON_X_CORE_X_TERMINATE_HPP

/// \file


#include <cstdlib>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/formattable_value_ref.hpp>


/// \brief Terminate the program immediately.
///
/// Terminate the program immediately through invocation of `std::abort()`.
///
/// When the library is compiled in debug mode (`ARCHON_DEBUG`), an attempt will be made to
/// write the specified message to STDERR before the program is terminated.
///
/// In all cases, this macro expands to a an invocation of a function that will never return
/// (declared with attribute `[[noreturn]]`) and never throw (declared `noexcept`).
///
#define ARCHON_TERMINATE(message)               \
    X_ARCHON_TERMINATE(message)



/// \{
///
/// \brief Terminate the program and dump values.
///
/// These macros terminate the program immediately, just like \ref
/// ARCHON_TERMINATE(). However, they also dump information about the specified values (\p
/// a, \p b, \p c, ...).
///
#define ARCHON_TERMINATE_1(message, a)                                  \
    ARCHON_TERMINATE_N(message,                                         \
                       ARCHON_TERMINATE_VAL(a))
#define ARCHON_TERMINATE_2(message, a, b)                               \
    ARCHON_TERMINATE_N(message,                                         \
                       ARCHON_TERMINATE_VAL(a),                         \
                       ARCHON_TERMINATE_VAL(b))
#define ARCHON_TERMINATE_3(message, a, b, c)                            \
    ARCHON_TERMINATE_N(message,                                         \
                       ARCHON_TERMINATE_VAL(a),                         \
                       ARCHON_TERMINATE_VAL(b),                         \
                       ARCHON_TERMINATE_VAL(c))
#define ARCHON_TERMINATE_4(message, a, b, c, d)                         \
    ARCHON_TERMINATE_N(message,                                         \
                       ARCHON_TERMINATE_VAL(a),                         \
                       ARCHON_TERMINATE_VAL(b),                         \
                       ARCHON_TERMINATE_VAL(c),                         \
                       ARCHON_TERMINATE_VAL(d))
#define ARCHON_TERMINATE_5(message, a, b, c, d, e)                      \
    ARCHON_TERMINATE_N(message,                                         \
                       ARCHON_TERMINATE_VAL(a),                         \
                       ARCHON_TERMINATE_VAL(b),                         \
                       ARCHON_TERMINATE_VAL(c),                         \
                       ARCHON_TERMINATE_VAL(d),                         \
                       ARCHON_TERMINATE_VAL(e))
#define ARCHON_TERMINATE_6(message, a, b, c, d, e, f)                   \
    ARCHON_TERMINATE_N(message,                                         \
                       ARCHON_TERMINATE_VAL(a),                         \
                       ARCHON_TERMINATE_VAL(b),                         \
                       ARCHON_TERMINATE_VAL(c),                         \
                       ARCHON_TERMINATE_VAL(d),                         \
                       ARCHON_TERMINATE_VAL(e),                         \
                       ARCHON_TERMINATE_VAL(f))
#define ARCHON_TERMINATE_7(message, a, b, c, d, e, f, g)                \
    ARCHON_TERMINATE_N(message,                                         \
                       ARCHON_TERMINATE_VAL(a),                         \
                       ARCHON_TERMINATE_VAL(b),                         \
                       ARCHON_TERMINATE_VAL(c),                         \
                       ARCHON_TERMINATE_VAL(d),                         \
                       ARCHON_TERMINATE_VAL(e),                         \
                       ARCHON_TERMINATE_VAL(f),                         \
                       ARCHON_TERMINATE_VAL(g))
#define ARCHON_TERMINATE_8(message, a, b, c, d, e, f, g, h)             \
    ARCHON_TERMINATE_N(message,                                         \
                       ARCHON_TERMINATE_VAL(a),                         \
                       ARCHON_TERMINATE_VAL(b),                         \
                       ARCHON_TERMINATE_VAL(c),                         \
                       ARCHON_TERMINATE_VAL(d),                         \
                       ARCHON_TERMINATE_VAL(e),                         \
                       ARCHON_TERMINATE_VAL(f),                         \
                       ARCHON_TERMINATE_VAL(g),                         \
                       ARCHON_TERMINATE_VAL(h))
/// \}



/// \{
///
/// \brief Terminate the program and dump any number of values.
///
/// The macro, \ref ARCHON_TERMINATE_N(), is like \ref ARCHON_TERMINATE_1(), and friends,
/// but, with some help from \ref ARCHON_TERMINATE_VAL(), it can be used to pass any number
/// of values to be dumped along with the specified message.
///
/// Here is an example with three values (even though it would have been easier to use \ref
/// ARCHON_TERMINATE_3() in this case):
///
/// \code{.cpp}
///
///   ARCHON_TERMINATE_N("Bad",
///                      ARCHON_TERMINATE_VAL(x),
///                      ARCHON_TERMINATE_VAL(y)),
///                      ARCHON_TERMINATE_VAL(z)));
///
/// \endcode
///
#define ARCHON_TERMINATE_N(message, ...)        \
    X_ARCHON_TERMINATE_N(message, __VA_ARGS__)
#define ARCHON_TERMINATE_VAL(x)                                         \
    archon::core::impl::make_terminate_val(ARCHON_STRINGIFY(x), x)
/// \}








// Implementation


#if ARCHON_DEBUG
#  define X_ARCHON_TERMINATE(message)                                   \
    archon::core::impl::terminate(message, __FILE__, __LINE__)
#  define X_ARCHON_TERMINATE_N(message, ...)                            \
    archon::core::impl::terminate(message, __FILE__, __LINE__, __VA_ARGS__)
#else
#  define X_ARCHON_TERMINATE(message)                                   \
    (static_cast<void>(sizeof (archon::core::impl::terminate(message, __FILE__, __LINE__), 0)), \
     std::abort())
#  define X_ARCHON_TERMINATE_N(message, ...)                            \
    (static_cast<void>(sizeof (archon::core::impl::terminate(message, __FILE__, __LINE__, __VA_ARGS__), 0)), \
     std::abort())
#endif


namespace archon::core::impl {


template<class T> struct TerminateVal {
    const char* text;
    const T& value;
};


template<class T> inline auto make_terminate_val(const char* text, const T& value) noexcept -> impl::TerminateVal<T>
{
    return { text, value };
}


class TerminateVal2 {
public:
    const char* text = 0;
    core::FormattableValueRef value;
    template<class... T> static void record(impl::TerminateVal2* buffer, impl::TerminateVal<T>... values) noexcept
    {
        do_record(buffer, values...);
    }
private:
    static void do_record(impl::TerminateVal2*) noexcept
    {
    }
    template<class T, class... U>
    static void do_record(impl::TerminateVal2* buffer, impl::TerminateVal<T> value,
                          impl::TerminateVal<U>... values) noexcept
    {
        impl::TerminateVal2& entry = *buffer;
        entry.text        = value.text;
        entry.value       = FormattableValueRef(value.value);
        do_record(buffer + 1, values...);
    }
};


[[noreturn]] void do_terminate(const char* message, const char* file, long line,
                               core::Span<const impl::TerminateVal2>) noexcept;


template<class... T>
[[noreturn]] inline void terminate(const char* message, const char* file, long line,
                                   impl::TerminateVal<T>... values) noexcept
{
    const std::size_t n = sizeof... (values);
    impl::TerminateVal2 buffer[std::max(n, std::size_t(1))];
    impl::TerminateVal2::record(buffer, values...);
    do_terminate(message, file, line, { buffer, n });
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_TERMINATE_HPP
