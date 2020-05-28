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

#ifndef ARCHON_X_CORE_X_TEXT_CODEC_HPP
#define ARCHON_X_CORE_X_TEXT_CODEC_HPP

/// \file


#include <cstddef>
#include <string_view>
#include <utility>
#include <stdexcept>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/text_codec_impl.hpp>


namespace archon::core {


/// \brief Encoding and decoding of text strings.
///
/// \tparam I The text codec implementation type to be used. This must be a type that
/// satisfies \ref Concept_Archon_Core_TextCodecImpl.
///
/// This class is empty (`std::is_empty`) if \p I is an empty class.
///
template<class I> class GenericTextCodec
    : private I {
public:
    using impl_type = I;

    using char_type   = typename impl_type::char_type;
    using traits_type = typename impl_type::traits_type;

    using string_view_type = std::basic_string_view<char_type, traits_type>;

    using Config = typename impl_type::Config;

    static constexpr bool is_degen = impl_type::is_degen;

    explicit GenericTextCodec(Config = {});
    explicit GenericTextCodec(const std::locale&, Config = {});
    explicit GenericTextCodec(const std::locale*, Config);

    auto decode(core::StringSpan<char> data, core::Buffer<char_type>& buffer) const -> string_view_type;
    auto encode(core::StringSpan<char_type> data, core::Buffer<char>& buffer) const -> std::string_view;

    void decode_a(core::StringSpan<char> data, core::Buffer<char_type>& buffer, std::size_t& buffer_offset) const;
    void encode_a(core::StringSpan<char_type> data, core::Buffer<char>& buffer, std::size_t& buffer_offset) const;

    bool try_decode(core::StringSpan<char> data, core::Buffer<char_type>& buffer, std::size_t& buffer_offset) const;
    bool try_encode(core::StringSpan<char_type> data, core::Buffer<char>& buffer, std::size_t& buffer_offset) const;

    class ShortCircuitDecodeBuffer;
    class ShortCircuitEncodeBuffer;

    auto decode_sc(core::StringSpan<char> data, ShortCircuitDecodeBuffer& buffer) const -> string_view_type;
    auto encode_sc(core::StringSpan<char_type> data, ShortCircuitEncodeBuffer& buffer) const -> std::string_view;

    bool try_decode_sc(core::StringSpan<char> data, ShortCircuitDecodeBuffer& buffer, string_view_type& result) const;
    bool try_encode_sc(core::StringSpan<char_type> data, ShortCircuitEncodeBuffer& buffer,
                       std::string_view& result) const;
};


template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicTextCodec =
    GenericTextCodec<core::TextCodecImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicPosixTextCodec =
    GenericTextCodec<core::PosixTextCodecImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicWindowsTextCodec =
    GenericTextCodec<core::WindowsTextCodecImpl<C, T, D>>;


using TextCodec        = BasicTextCodec<char>;
using PosixTextCodec   = BasicPosixTextCodec<char>; // Always degenerate
using WindowsTextCodec = BasicWindowsTextCodec<char>;

using WideTextCodec        = BasicTextCodec<wchar_t>;
using WidePosixTextCodec   = BasicPosixTextCodec<wchar_t>;
using WideWindowsTextCodec = BasicWindowsTextCodec<wchar_t>;




namespace impl {
template<class C, bool> class TextDecodeBuffer;
template<class I> using TextDecodeBuffer2 = TextDecodeBuffer<typename I::char_type, I::is_degen>;
template<bool> class TextEncodeBuffer;
template<class I> using TextEncodeBuffer2 = TextEncodeBuffer<I::is_degen>;
} // namespace impl


template<class I> class GenericTextCodec<I>::ShortCircuitDecodeBuffer
    : public impl::TextDecodeBuffer2<I> {
public:
    explicit ShortCircuitDecodeBuffer(core::Span<char_type> seed_memory = {}) noexcept;
};


template<class I> class GenericTextCodec<I>::ShortCircuitEncodeBuffer
    : public impl::TextEncodeBuffer2<I> {
public:
    explicit ShortCircuitEncodeBuffer(core::Span<char> seed_memory = {}) noexcept;
};




/// \brief Decoding of text strings.
///
/// \tparam I The text codec implementation type to be used. This must be a type that
/// satisfies \ref Concept_Archon_Core_TextCodecImpl.
///
/// This class is empty (`std::is_empty`) if \p I is an empty class.
///
template<class I> class GenericTextDecoder
    : private GenericTextCodec<I>
    , private GenericTextCodec<I>::ShortCircuitDecodeBuffer {
public:
    using impl_type = I;

    using codec_type       = GenericTextCodec<impl_type>;
    using char_type        = typename codec_type::char_type;
    using traits_type      = typename codec_type::traits_type;
    using string_view_type = std::basic_string_view<char_type, traits_type>;

    using Config = typename codec_type::Config;

    static constexpr bool is_degen = codec_type::is_degen;

    explicit GenericTextDecoder(core::Span<char_type> seed_memory = {}, Config = {});
    explicit GenericTextDecoder(const std::locale&, core::Span<char_type> seed_memory = {}, Config = {});
    explicit GenericTextDecoder(const std::locale*, core::Span<char_type> seed_memory, Config);

    auto decode_sc(core::StringSpan<char> data) -> string_view_type;

    bool try_decode_sc(core::StringSpan<char> data, string_view_type& result);
};


template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicTextDecoder =
    GenericTextDecoder<core::TextCodecImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicPosixTextDecoder =
    GenericTextDecoder<core::PosixTextCodecImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicWindowsTextDecoder =
    GenericTextDecoder<core::WindowsTextCodecImpl<C, T, D>>;


using TextDecoder        = BasicTextDecoder<char>;
using PosixTextDecoder   = BasicPosixTextDecoder<char>; // Always degenerate
using WindowsTextDecoder = BasicWindowsTextDecoder<char>;

using WideTextDecoder        = BasicTextDecoder<wchar_t>;
using WidePosixTextDecoder   = BasicPosixTextDecoder<wchar_t>;
using WideWindowsTextDecoder = BasicWindowsTextDecoder<wchar_t>;




/// \brief Encoding of text strings.
///
/// \tparam I The text codec implementation type to be used. This must be a type that
/// satisfies \ref Concept_Archon_Core_TextCodecImpl.
///
/// This class is empty (`std::is_empty`) if \p I is an empty class.
///
template<class I> class GenericTextEncoder
    : private GenericTextCodec<I>
    , private GenericTextCodec<I>::ShortCircuitEncodeBuffer {
public:
    using impl_type = I;

    using codec_type  = GenericTextCodec<impl_type>;
    using char_type   = typename codec_type::char_type;
    using traits_type = typename codec_type::traits_type;

    using Config = typename codec_type::Config;

    static constexpr bool is_degen = codec_type::is_degen;

    explicit GenericTextEncoder(core::Span<char> seed_memory = {}, Config = {});
    explicit GenericTextEncoder(const std::locale&, core::Span<char> seed_memory = {}, Config = {});
    explicit GenericTextEncoder(const std::locale*, core::Span<char> seed_memory, Config);

    auto encode_sc(core::StringSpan<char_type> data) -> std::string_view;

    bool try_encode_sc(core::StringSpan<char_type> data, std::string_view& result);
};


template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicTextEncoder =
    GenericTextEncoder<core::TextCodecImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicPosixTextEncoder =
    GenericTextEncoder<core::PosixTextCodecImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BasicWindowsTextEncoder =
    GenericTextEncoder<core::WindowsTextCodecImpl<C, T, D>>;


using TextEncoder        = BasicTextEncoder<char>;
using PosixTextEncoder   = BasicPosixTextEncoder<char>; // Always degenerate
using WindowsTextEncoder = BasicWindowsTextEncoder<char>;

using WideTextEncoder        = BasicTextEncoder<wchar_t>;
using WidePosixTextEncoder   = BasicPosixTextEncoder<wchar_t>;
using WideWindowsTextEncoder = BasicWindowsTextEncoder<wchar_t>;








// Implementation


// ============================ GenericTextCodec ============================


template<class I>
inline GenericTextCodec<I>::GenericTextCodec(Config config)
    : GenericTextCodec(nullptr, std::move(config)) // Throws
{
}


template<class I>
inline GenericTextCodec<I>::GenericTextCodec(const std::locale& locale, Config config)
    : GenericTextCodec(&locale, std::move(config)) // Throws
{
}


template<class I>
inline GenericTextCodec<I>::GenericTextCodec(const std::locale* locale, Config config)
    : I(locale, std::move(config)) // Throws
{
}


template<class I>
inline auto GenericTextCodec<I>::decode(core::StringSpan<char> data, core::Buffer<char_type>& buffer) const ->
    string_view_type
{
    std::size_t buffer_offset = 0;
    decode_a(data, buffer, buffer_offset); // Throws
    return { buffer.data(), buffer_offset };
}


template<class I>
inline auto GenericTextCodec<I>::encode(core::StringSpan<char_type> data, core::Buffer<char>& buffer) const ->
    std::string_view
{
    std::size_t buffer_offset = 0;
    encode_a(data, buffer, buffer_offset); // Throws
    return { buffer.data(), buffer_offset };
}


template<class I>
inline void GenericTextCodec<I>::decode_a(core::StringSpan<char> data, core::Buffer<char_type>& buffer,
                                          std::size_t& buffer_offset) const
{
    if (ARCHON_LIKELY(try_decode(data, buffer, buffer_offset))) // Throws
        return;
    throw std::runtime_error("Decoding failed");
}


template<class I>
inline void GenericTextCodec<I>::encode_a(core::StringSpan<char_type> data, core::Buffer<char>& buffer,
                                          std::size_t& buffer_offset) const
{
    if (ARCHON_LIKELY(try_encode(data, buffer, buffer_offset))) // Throws
        return;
    throw std::runtime_error("Encoding failed");
}


template<class I>
bool GenericTextCodec<I>::try_decode(core::StringSpan<char> data, core::Buffer<char_type>& buffer,
                                     std::size_t& buffer_offset) const
{
    using decoder_type = typename impl_type::decoder_type;
    using decoder_impl_type = typename decoder_type::impl_type;
    const decoder_impl_type& impl = impl_type::template up_cast<decoder_impl_type>(*this);
    decoder_type decoder(impl, buffer, buffer_offset); // Throws
    std::size_t data_offset = 0;
    bool end_of_data = true;
    bool success = decoder.decode(data, data_offset, end_of_data); // Throws
    ARCHON_ASSERT(!success || data_offset == data.size());
    return success;
}


template<class I>
bool GenericTextCodec<I>::try_encode(core::StringSpan<char_type> data, core::Buffer<char>& buffer,
                                     std::size_t& buffer_offset) const
{
    using encoder_type = typename impl_type::encoder_type;
    using encoder_impl_type = typename encoder_type::impl_type;
    const encoder_impl_type& impl = impl_type::template up_cast<encoder_impl_type>(*this);
    encoder_type encoder(impl, buffer, buffer_offset); // Throws
    std::size_t data_offset = 0;
    bool success =  encoder.encode(data, data_offset); // Throws
    if (ARCHON_LIKELY(success)) {
        ARCHON_ASSERT(data_offset == data.size());
        return encoder.unshift(); // Throws
    }
    return false;
}


template<class I>
inline auto GenericTextCodec<I>::decode_sc(core::StringSpan<char> data, ShortCircuitDecodeBuffer& buffer) const ->
    string_view_type
{
    string_view_type result;
    if (ARCHON_LIKELY(try_decode_sc(data, buffer, result))) // Throws
        return result;
    throw std::runtime_error("Decoding failed");
}


template<class I>
inline auto GenericTextCodec<I>::encode_sc(core::StringSpan<char_type> data, ShortCircuitEncodeBuffer& buffer) const ->
    std::string_view
{
    std::string_view result;
    if (ARCHON_LIKELY(try_encode_sc(data, buffer, result))) // Throws
        return result;
    throw std::runtime_error("Encoding failed");
}


template<class I>
bool GenericTextCodec<I>::try_decode_sc(core::StringSpan<char> data, ShortCircuitDecodeBuffer& buffer,
                                        string_view_type& result) const
{
    if constexpr (is_degen) {
        result = { data.data(), data.size() };
        return true;
    }
    else {
        std::size_t buffer_offset = 0;
        if (ARCHON_LIKELY(try_decode(data, buffer, buffer_offset))) { // Throws
            result = { buffer.data(), buffer_offset };
            return true;
        }
    }
    return false;
}


template<class I>
bool GenericTextCodec<I>::try_encode_sc(core::StringSpan<char_type> data, ShortCircuitEncodeBuffer& buffer,
                                        std::string_view& result) const
{
    if constexpr (is_degen) {
        result = { data.data(), data.size() };
        return true;
    }
    else {
        std::size_t buffer_offset = 0;
        if (ARCHON_LIKELY(try_encode(data, buffer, buffer_offset))) { // Throws
            result = { buffer.data(), buffer_offset };
            return true;
        }
    }
    return false;
}


namespace impl {


// Non-degenerate case
template<class C, bool> class TextDecodeBuffer
    : public core::Buffer<C> {
public:
    explicit TextDecodeBuffer(core::Span<C> seed_memory) noexcept
        : core::Buffer<C>(seed_memory)
    {
    }
};

// Degenerate case
template<class C> class TextDecodeBuffer<C, true> {
public:
    explicit TextDecodeBuffer(core::Span<C>) noexcept
    {
    }
};


// Non-degenerate case
template<bool> class TextEncodeBuffer
    : public core::Buffer<char> {
public:
    explicit TextEncodeBuffer(core::Span<char> seed_memory) noexcept
        : core::Buffer<char>(seed_memory)
    {
    }
};

// Degenerate case
template<> class TextEncodeBuffer<true> {
public:
    explicit TextEncodeBuffer(core::Span<char>) noexcept
    {
    }
};


} // namespace impl


template<class I>
inline GenericTextCodec<I>::ShortCircuitDecodeBuffer::ShortCircuitDecodeBuffer(core::Span<char_type> seed_memory) noexcept
    : impl::TextDecodeBuffer2<I>(seed_memory)
{
}


template<class I>
inline GenericTextCodec<I>::ShortCircuitEncodeBuffer::ShortCircuitEncodeBuffer(core::Span<char> seed_memory) noexcept
    : impl::TextEncodeBuffer2<I>(seed_memory)
{
}



// ============================ GenericTextDecoder ============================


template<class I>
inline GenericTextDecoder<I>::GenericTextDecoder(core::Span<char_type> seed_memory, Config config)
    : GenericTextDecoder(nullptr, seed_memory, std::move(config)) // Throws
{
}


template<class I>
inline GenericTextDecoder<I>::GenericTextDecoder(const std::locale& locale, core::Span<char_type> seed_memory,
                                                 Config config)
    : GenericTextDecoder(&locale, seed_memory, std::move(config)) // Throws
{
}


template<class I>
inline GenericTextDecoder<I>::GenericTextDecoder(const std::locale* locale, core::Span<char_type> seed_memory,
                                                 Config config)
    : GenericTextCodec<I>(locale, std::move(config)) // Throws
    , GenericTextCodec<I>::ShortCircuitDecodeBuffer(seed_memory)
{
}


template<class I>
inline auto GenericTextDecoder<I>::decode_sc(core::StringSpan<char> data) -> string_view_type
{
    string_view_type result;
    if (ARCHON_LIKELY(try_decode_sc(data, result))) // Throws
        return result;
    throw std::runtime_error("Decoding failed");
}


template<class I>
bool GenericTextDecoder<I>::try_decode_sc(core::StringSpan<char> data, string_view_type& result)
{
    return GenericTextCodec<I>::try_decode_sc(data, *this, result); // Throws
}



// ============================ GenericTextEncoder ============================


template<class I>
inline GenericTextEncoder<I>::GenericTextEncoder(core::Span<char> seed_memory, Config config)
    : GenericTextEncoder(nullptr, seed_memory, std::move(config)) // Throws
{
}


template<class I>
inline GenericTextEncoder<I>::GenericTextEncoder(const std::locale& locale, core::Span<char> seed_memory,
                                                 Config config)
    : GenericTextEncoder(&locale, seed_memory, std::move(config)) // Throws
{
}


template<class I>
inline GenericTextEncoder<I>::GenericTextEncoder(const std::locale* locale, core::Span<char> seed_memory,
                                                 Config config)
    : GenericTextCodec<I>(locale, std::move(config)) // Throws
    , GenericTextCodec<I>::ShortCircuitEncodeBuffer(seed_memory)
{
}


template<class I>
inline auto GenericTextEncoder<I>::encode_sc(core::StringSpan<char_type> data) -> std::string_view
{
    std::string_view result;
    if (ARCHON_LIKELY(try_encode_sc(data, result))) // Throws
        return result;
    throw std::runtime_error("Encoding failed");
}


template<class I>
bool GenericTextEncoder<I>::try_encode_sc(core::StringSpan<char_type> data, std::string_view& result)
{
    return GenericTextCodec<I>::try_encode_sc(data, *this, result); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_CODEC_HPP
