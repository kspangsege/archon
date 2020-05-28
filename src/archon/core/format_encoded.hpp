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
#include <string_view>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/stream_output.hpp>


namespace archon::core {


namespace impl {
template<class C, class T> struct AsEncoded;
} // namespace impl



/// \brief Write encoding of string to stream.
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
template<class C, class T> auto encoded(std::basic_string_view<C, T>) noexcept;








// Implementation


namespace impl {


template<class C, class T> struct AsEncoded {
    std::basic_string_view<C, T> string;
};


template<class T, class C, class U>
void write_encoded(std::basic_ostream<char, T>& out, std::basic_string_view<C, U> data)
{
    core::ostream_sentry(out, [&](core::BasicStreamOutputHelper<char, T>& helper) {
        core::ArraySeededBuffer<char, 1024> buffer;
        std::size_t buffer_offset = 0;
        auto flush = [&] {
            helper.write({ buffer.data(), buffer_offset }); // Throws
            buffer_offset = 0;
        };
        auto expand_buffer = [&] {
            buffer.expand(1, 0); // Throws
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
                expand_buffer(); // Throws
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
            expand_buffer(); // Throws
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


} // namespace impl


template<class C, class T> inline auto encoded(std::basic_string_view<C, T> string) noexcept
{
    return impl::AsEncoded<C, T> { string };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FORMAT_ENCODED_HPP
