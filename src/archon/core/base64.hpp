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

#ifndef ARCHON_X_CORE_X_BASE64_HPP
#define ARCHON_X_CORE_X_BASE64_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <array>

#include <archon/core/features.h>
#include <archon/core/buffer.hpp>
#include <archon/core/char_mapper.hpp>


// =========================> RFC 4648 paragraph 4: base64 (standard)              
// =========================> RFC 4648 paragraph 5: base64url (URL- and filename-safe standard)             


namespace archon::core::base64 {


/// \brief 
///
/// 
///
struct EncodeConfig {
    bool url_safe_variant = false;
    bool use_padding = false;
    std::size_t line_size = 0;
};


/// \brief 
///
/// 
///
struct DecodeConfig {
    enum Padding : char { allow, reject, require };
    bool url_safe_variant = false;
    Padding padding = Padding::allow;
    bool allow_whitespace = false;
};




/// \{
///
/// \brief 
///
/// Throws exception of unspecified type on invalid input (character value > 255).
///
/// This function may write to the buffer during an invocation that leads to failure.
///
template<class C, class T = std::char_traits<C>>
auto encode(core::Span<const char> data, core::Buffer<C>&, EncodeConfig = {}) -> std::basic_string_view<C, T>;
template<class C, class T = std::char_traits<C>>
auto encode(core::Span<const char> data, core::Buffer<C>&, const std::locale&, EncodeConfig = {}) ->
    std::basic_string_view<C, T>;
/// \}




/// \brief 
///
/// On success -> returns true and assigns to \p size. Does not assign to \p insufficient_buffer_space.
///
/// On failure -> returns false and assigns to \p insufficient_buffer_space. Does not assign to \p size. Reason for failure is lack of buffer space if \p insufficient_buffer_space is true. Otherwise it is due to bad input (character value > 255).
///
/// This function may write to the buffer during an invocation that leads to failure.
///
bool encode(core::Span<const char> data, core::Span<char> buffer, std::size_t& size, bool& insufficient_buffer_space,
            EncodeConfig = {}) noexcept;




template<class C, class T>
auto decode(std::basic_string_view<C, T> data, core::Buffer<char>&, DecodeConfig = {}) -> core::Span<const char>;
template<class C, class T = std::char_traits<C>>
auto decode(std::basic_string_view<C, T> data, core::Buffer<char>&, const std::locale&, DecodeConfig = {}) ->
    core::Span<const char>;




/// \brief 
///
/// 
///
bool decode(std::string_view data, core::Span<char> buffer, std::size_t& size, bool& insufficient_buffer_space,
            DecodeConfig = {}) noexcept;




constexpr auto encode_buffer_size(std::size_t data_size, EncodeConfig = {}) -> std::size_t;


constexpr auto decode_buffer_size(std::size_t data_size) -> std::size_t;




template<class C, class T = std::char_traits<C>> class BasicEncoder
    : private core::BasicCharMapper<C, T> {
public:
    using string_view_type = std::basic_string_view<C, T>;

    BasicEncoder(EncodeConfig = {});
    BasicEncoder(const std::locale&, EncodeConfig = {});

    auto encode(core::Span<const char> data, core::Buffer<C>&) const -> string_view_type;

private:
    EncodeConfig m_config;
};




template<class C, class T = std::char_traits<C>> class BasicDecoder
    : private core::BasicCharMapper<C, T> {
public:
    using string_view_type = std::basic_string_view<C, T>;

    BasicDecoder(DecodeConfig = {});
    BasicDecoder(const std::locale&, DecodeConfig = {});

    auto decode(string_view_type data, core::Buffer<char>&) const -> core::Span<const char>;

private:
    DecodeConfig m_config;
};




/// \brief Produce Base64 encoding of binary data stream.
///
/// This class can be used to incrementally produce a Base64 or Base64url encoding of a
/// stream of binary data. The two alternative encodings, Base64 and Base64url, are as
/// defined in RFC 4648 sections 4 and 5 respectively.
///
/// \sa https://tools.ietf.org/html/rfc4648
///
class IncrementalEncoder {
public:
    IncrementalEncoder(EncodeConfig = {}) noexcept;

    ///     
    /// All input consumed and all output produced -> return true
    ///
    /// Otherwise -> return false
    ///
    ///   -> Output buffer is full
    ///
    ///   -> Invalid input character at \p data_begin (value higher than 255)
    ///
    bool encode(const char*& data_begin, const char* data_end, bool end_of_input,
                char*& buffer_begin, char* buffer_end) noexcept;

private:
    EncodeConfig m_config;
    bool m_holding_output = false;
    std::int_least8_t m_holding_size = 0;
    std::array<char, 4> m_hold_buffer;
    std::size_t m_line_size = 0;
};




class IncrementalDecoder {
public:
    IncrementalDecoder(DecodeConfig = {}) noexcept;

    ///     
    bool decode(const char*& data_begin, const char* data_end, bool end_of_input,
                char*& buffer_begin, char* buffer_end) noexcept;

private:
    DecodeConfig m_config;
    bool m_holding_output = false;
    std::int_least8_t m_padding_size = 0;
    std::int_least8_t m_holding_size = 0;
    std::array<char, 4> m_hold_buffer;
};








// Implementation


template<class C, class T>
inline auto encode(core::Span<const char> data, core::Buffer<C>& buffer,
                   EncodeConfig config) -> std::basic_string_view<C, T>
{
    BasicEncoder<C, T> encoder(config); // Throws
    return encoder.encode(data, buffer); // Throws
}


template<class C, class T>
inline auto encode(core::Span<const char> data, core::Buffer<C>& buffer, const std::locale& locale,
                   EncodeConfig config) -> std::basic_string_view<C, T>
{
    BasicEncoder<C, T> encoder(locale, config); // Throws
    return encoder.encode(data, buffer); // Throws
}


inline bool encode(core::Span<const char> data, core::Span<char> buffer, std::size_t& size,
                   bool& insufficient_buffer_space, EncodeConfig config) noexcept
{
    IncrementalEncoder encoder(config);
    const char* data_begin = data.data();
    const char* data_end   = data.data() + data.size();
    bool end_of_input = true;
    char* buffer_begin = buffer.data();
    char* buffer_end   = buffer.data() + buffer.size();
    bool done = encoder.encode(data_begin, data_end, end_of_input, buffer_begin, buffer_end);
    if (ARCHON_LIKELY(done)) {
        size = std::size_t(buffer_begin - buffer.data());
        return true;
    }
    insufficient_buffer_space = (buffer_begin == buffer_end);
    return false;
}


template<class C, class T>
auto decode(std::basic_string_view<C, T> data, core::Buffer<char>& buffer,
            DecodeConfig config) -> core::Span<const char>
{
    BasicDecoder<C, T> decoder(config); // Throws
    return decoder.decode(data, buffer); // Throws
}


template<class C, class T>
auto decode(std::basic_string_view<C, T> data, core::Buffer<char>& buffer, const std::locale& locale,
            DecodeConfig config) -> core::Span<const char>
{
    BasicDecoder<C, T> decoder(locale, config); // Throws
    return decoder.decode(data, buffer); // Throws
}


inline bool decode(std::string_view data, core::Span<char> buffer, std::size_t& size, bool& insufficient_buffer_space,
                   DecodeConfig config) noexcept
{
    IncrementalDecoder decoder(config);
    const char* data_begin = data.data();
    const char* data_end   = data.data() + data.size();
    bool end_of_input = true;
    char* buffer_begin = buffer.data();
    char* buffer_end   = buffer.data() + buffer.size();
    bool done = decoder.decode(data_begin, data_end, end_of_input, buffer_begin, buffer_end);
    if (ARCHON_LIKELY(done)) {
        size = std::size_t(buffer_begin - buffer.data());
        return true;
    }
    insufficient_buffer_space = (buffer_begin == buffer_end);
    return false;
}


template<class C, class T>
inline BasicEncoder<C, T>::BasicEncoder(EncodeConfig config)
    : BasicCharMapper<C, T>() // Throws
    , m_config(config)
{
}


template<class C, class T>
inline BasicEncoder<C, T>::BasicEncoder(const std::locale& locale, EncodeConfig config)
    : BasicCharMapper<C, T>(locale) // Throws
    , m_config(config)
{
}


template<class C, class T>
auto BasicEncoder<C, T>::encode(core::Span<const char> data, core::Buffer<C>& buffer) const -> string_view_type
{
    buffer.reserve(encode_buffer_size(data.size(), m_config)); // Throws
    if constexpr (BasicCharMapper<char>::is_trivial) {
        std::size_t size = 0;
        bool insufficient_buffer_space = false;
        bool success = base64::encode(data, core::Span(buffer), size, insufficient_buffer_space, m_config);
        ARCHON_ASSERT(!insufficient_buffer_space);
        if (ARCHON_LIKELY(success || std::numeric_limits<unsigned char>::digits == 8))
            return { buffer.data(), size };
    }
    else {
        IncrementalEncoder encoder(m_config);
        std::array<char, 256> buffer_2;
        const char* data_begin = data.data();
        const char* data_end = data_begin + data.size();
        char* buffer_end = buffer_2.data() + buffer_2.size();
        char* dest = buffer.data();
        for (;;) {
            bool end_of_input = true;
            char* buffer_begin = buffer_2.data();
            bool done = encoder.encode(data_begin, data_end, end_of_input, buffer_begin, buffer_end);
            std::size_t size = std::size_t(buffer_begin - buffer_2.data());
            ARCHON_ASSERT(size <= buffer.size());
            ARCHON_ASSERT(std::size_t(dest - buffer.data()) <= buffer.size() - size);
            this->widen({ buffer_2.data(), size }, dest); // Throws
            dest += size;
            if (ARCHON_LIKELY(!done)) {
                if (ARCHON_LIKELY(buffer_begin == buffer_end))
                    continue;
                break;
            }
            return { buffer.data(), std::size_t(dest - buffer.data()) };
        }
    }
    // Byte with value that does not fit in 8 bits
    throw std::runtime_error("Invalid byte during Base64 encoding");
}


template<class C, class T>
inline BasicDecoder<C, T>::BasicDecoder(DecodeConfig config)
    : BasicCharMapper<C, T>() // Throws
    , m_config(config)
{
}


template<class C, class T>
inline BasicDecoder<C, T>::BasicDecoder(const std::locale& locale, DecodeConfig config)
    : BasicCharMapper<C, T>(locale) // Throws
    , m_config(config)
{
}


template<class C, class T>
auto BasicDecoder<C, T>::decode(string_view_type data, core::Buffer<char>& buffer) const -> core::Span<const char>
{
    buffer.reserve(decode_buffer_size(data.size())); // Throws
    if constexpr (BasicCharMapper<char>::is_trivial) {
        std::size_t size = 0;
        bool insufficient_buffer_space = false;
        bool success = base64::decode(data, core::Span(buffer), size, insufficient_buffer_space, m_config);
        ARCHON_ASSERT(!insufficient_buffer_space);
        if (ARCHON_LIKELY(success))
            return { buffer.data(), size };
    }
    else {
        IncrementalDecoder decoder(m_config);
        std::array<char, 256> buffer_2;
        const C* i   = data.data();
        const C* end = i + data.size();
        char* buffer_begin = buffer.data();
        char* buffer_end   = buffer_begin + buffer.size();
        for (;;) {
            std::size_t remain = std::size_t(end - i);
            bool end_of_input = (remain <= buffer_2.size());
            std::size_t size = (end_of_input ? remain : buffer_2.size());
            char replacement = '\0';
            this->narrow(string_view_type(i, size), replacement, buffer_2.data()); // Throws
            const char* data_begin = buffer_2.data();
            const char* data_end   = data_begin + size;
            bool done = decoder.decode(data_begin, data_end, end_of_input, buffer_begin, buffer_end);
            if (ARCHON_LIKELY(!done)) {
                ARCHON_ASSERT(buffer_begin != buffer_end);
                if (ARCHON_LIKELY(data_begin == data_end)) {
                    ARCHON_ASSERT(!end_of_input);
                    i += size;
                    continue;
                }
                break; // Bad input
            }
            ARCHON_ASSERT(data_begin == data_end);
            ARCHON_ASSERT(end_of_input);
            return { buffer.data(), std::size_t(buffer_begin - buffer.data()) };
        }
    }
    throw std::runtime_error("Invalid character during Base64 decoding");
}


constexpr auto encode_buffer_size(std::size_t data_size, EncodeConfig config) -> std::size_t
{
    std::size_t size = data_size;
    std::size_t extra = std::size_t(size / 3);
    std::size_t rest = std::size_t(size - extra * 3);
    if (rest > 0) {
        ++extra;
        if (config.use_padding)
            extra += 3 - rest;
    }
    if (ARCHON_LIKELY(core::try_int_add(size, extra))) {
        if (config.line_size == 0)
            return size;
        extra = std::size_t(size / config.line_size);
        rest = std::size_t(size - extra * config.line_size);
        if (rest > 0)
            ++extra;
        if (ARCHON_LIKELY(core::try_int_add(size, extra)))
            return size;
    }
    throw std::length_error("Data size");
}


constexpr auto decode_buffer_size(std::size_t data_size) -> std::size_t
{
    std::size_t size = data_size;
    std::size_t extra = std::size_t(size / 4);
    std::size_t rest = std::size_t(size - extra * 4);
    if (rest > 0)
        ++extra;
    return size - extra;
}


inline IncrementalEncoder::IncrementalEncoder(EncodeConfig config) noexcept
    : m_config(config)
{
}


inline IncrementalDecoder::IncrementalDecoder(DecodeConfig config) noexcept
    : m_config(config)
{
}


} // namespace archon::core::base64

#endif // ARCHON_X_CORE_X_BASE64_HPP
