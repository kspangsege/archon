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

#ifndef ARCHON_X_CORE_X_STRING_CODEC_HPP
#define ARCHON_X_CORE_X_STRING_CODEC_HPP

/// \file


#include <archon/core/char_codec.hpp>
#include <archon/core/text_codec.hpp>


namespace archon::core {


template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>>
using BasicStringCodec = core::BasicPosixTextCodec<C, T, D>;

using StringCodec     = BasicStringCodec<char>; // Always degenerate
using WideStringCodec = BasicStringCodec<wchar_t>;



template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>>
using BasicStringDecoder = core::BasicPosixTextDecoder<C, T, D>;

using StringDecoder     = BasicStringDecoder<char>; // Always degenerate
using WideStringDecoder = BasicStringDecoder<wchar_t>;



template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>>
using BasicStringEncoder = core::BasicPosixTextEncoder<C, T, D>;

using StringEncoder     = BasicStringEncoder<char>; // Always degenerate
using WideStringEncoder = BasicStringEncoder<wchar_t>;



/// \brief Decode string according to locale.
///
/// Convert a string expressed in the narrow multi-byte encoding of the specified locale to
/// a string expressed in the wide character encoding of that locale.
///
/// This function is a shorthand for constructing a string decoder from the specified
/// locale, calling \ref BasicStringDecoder<C, T, D>::decode_sc() with the specified string
/// and configuration, and returning a copy of the produced string.
///
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>>
auto decode_string(std::string_view string, const std::locale& locale = {},
                   typename BasicStringDecoder<C, T, D>::Config = {}) -> std::basic_string<C, T>;


/// \brief Encode string according to locale.
///
/// Convert a string expressed in the wide character encoding of the specified locale to a
/// string expressed in the narrow multi-byte encoding of that locale.
///
/// This function is a shorthand for constructing a string encoder from the specified
/// locale, calling \ref BasicStringEncoder<C, T, D>::encode_sc() with the specified string
/// and configuration, and returning a copy of the produced string.
///
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>>
auto encode_string(std::basic_string_view<C, T> string, const std::locale& locale = {},
                   typename BasicStringEncoder<C, T, D>::Config = {}) -> std::string;








// Implementation


template<class C, class T, class D>
auto decode_string(std::string_view string, const std::locale& locale,
                   typename BasicStringDecoder<C, T, D>::Config config) -> std::basic_string<C, T>
{
    std::array<C, 256> seed_memory;
    BasicStringDecoder<C, T, D> decoder(locale, seed_memory, std::move(config)); // Throws
    return std::basic_string<C, T>(decoder.decode_sc(string)); // Throws
}


template<class C, class T, class D>
auto encode_string(std::basic_string_view<C, T> string, const std::locale& locale,
                   typename BasicStringEncoder<C, T, D>::Config config) -> std::string
{
    std::array<char, 256> seed_memory;
    BasicStringEncoder<C, T, D> encoder(locale, seed_memory, std::move(config)); // Throws
    return std::string(encoder.encode_sc(string)); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STRING_CODEC_HPP
