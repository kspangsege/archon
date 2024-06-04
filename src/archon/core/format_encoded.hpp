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

#ifndef ARCHON_X_CORE_X_FORMAT_ENCODED_HPP
#define ARCHON_X_CORE_X_FORMAT_ENCODED_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <array>
#include <string_view>
#include <locale>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/value_formatter.hpp>


namespace archon::core {


namespace impl {
template<class C, class T> struct AsEncoded;
} // namespace impl



/// \brief Potentiate writing of encoding of string to stream.
///
/// Construct an object that, if written to an output stream, causes an encoded version of
/// the specified string to be written to the target stream. Encoding occurs as per the
/// locale of the target stream.
///
/// The character type of the target stream must be `char`.
///
/// If `core::BasicCharCodec<C, T>::is_degen` is `true`, no encoding takes place. Otherwise,
/// `core::CharCodec<C, T>` is used to perform the encoding. See \ref core::CharCodec.
///
/// One advantage of using this function, is that it operates without any dynamic memory
/// allocation.
///
/// \sa \ref core::encoded_a()
///
template<class C, class T> auto encoded(std::basic_string_view<C, T>) noexcept;



/// \brief Potentiate formatting of specified value and encoding of result.
///
/// Construct an object that, if written to an output stream, causes the specified value to
/// be formatted with respect to the specified character type and character traits (\p C and
/// \p T), and causes an encoded version of the formatted result to be written to that
/// stream. The encoding occurs as if by \ref core::encoded().
///
/// The character type of the target stream must be `char`.
///
/// \sa \ref core::encoded()
///
template<class C = wchar_t, class T = std::char_traits<C>, class V> auto encoded_a(const V& val) noexcept;








// Implementation


namespace impl {


template<class C, class T> struct AsEncoded {
    std::basic_string_view<C, T> string;
};


template<class T, class C, class U>
void write_encoded(std::basic_ostream<char, T>& out, std::basic_string_view<C, U> data)
{
    core::ostream_sentry(out, [&](core::BasicStreamOutputHelper<char, T>& helper) {
        std::array<char, 512> seed_memory;
        core::Buffer buffer(seed_memory);
        std::size_t buffer_offset = 0;
        auto flush = [&] {
            helper.write({ buffer.data(), buffer_offset }); // Throws
            buffer_offset = 0;
        };
        core::BasicCharCodec<C, U> codec(out.getloc()); // Throws
        using state_type = typename U::state_type;
        state_type state = {};
        std::size_t data_offset = 0;
        for (;;) {
            bool error = false;
            bool complete = codec.encode(state, data, data_offset, buffer, buffer_offset, error); // Throws
            if (ARCHON_LIKELY(complete)) {
                ARCHON_ASSERT(data_offset == data.size());
                break;
            }
            if (ARCHON_LIKELY(!error)) {
                if (ARCHON_LIKELY(buffer_offset > 0)) {
                    flush(); // Throws
                    continue;
                }
                buffer.expand(0); // Throws
                continue;
            }
            // Write everything up to the point of the failure
            flush(); // Throws
            return false; // Failure
        }
        for (;;) {
            bool complete = codec.unshift(state, buffer, buffer_offset); // Throws
            if (ARCHON_LIKELY(complete))
                break;
            if (ARCHON_LIKELY(buffer_offset > 0)) {
                flush(); // Throws
                continue;
            }
            buffer.expand(0); // Throws
        }
        flush(); // Throws
        return true; // Success
    });
}


template<class T, class C, class U>
inline auto operator<<(std::basic_ostream<char, T>& out, impl::AsEncoded<C, U> pod) -> std::basic_ostream<char, T>&
{
    if constexpr (core::BasicCharCodec<C, U>::is_degen) {
        std::basic_string_view<char, T> string(pod.string.data(), pod.string.size());
        out << string; // Throws
    }
    else {
        impl::write_encoded(out, pod.string); // Throws
    }
    return out;
}


template<class C, class T, class V> struct AsEncodedA {
    const V& val;
};


template<class T, class C, class U, class V>
inline auto operator<<(std::basic_ostream<char, T>& out, impl::AsEncodedA<C, U, V> pod) -> std::basic_ostream<char, T>&
{
    if constexpr (core::BasicCharCodec<C, U>::is_degen) {
        out << pod.val; // Throws
    }
    else {
        std::array<C, 512> seed_memory;
        std::locale locale = out.getloc(); // Throws
        core::BasicValueFormatter<C, U> formatter(seed_memory, locale); // Throws
        std::basic_string_view string = formatter.format(pod.val); // Throws
        impl::write_encoded(out, string); // Throws
    }
    return out;
}


} // namespace impl


template<class C, class T> inline auto encoded(std::basic_string_view<C, T> string) noexcept
{
    return impl::AsEncoded<C, T> { string };
}


template<class C, class T, class V> auto encoded_a(const V& val) noexcept
{
    return impl::AsEncodedA<C, T, V> { val };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FORMAT_ENCODED_HPP
