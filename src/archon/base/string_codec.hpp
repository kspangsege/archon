// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/// \file

#ifndef ARCHON__BASE__STRING_CODEC_HPP
#define ARCHON__BASE__STRING_CODEC_HPP

#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <array>
#include <string_view>
#include <string>
#include <locale>
#include <stdexcept>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/span.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/char_mapper.hpp>


namespace archon::base {


/// \brief Encode the specified string.
///
/// Convert a string expressed in the wide character encoding of the specified
/// locale to a string expressed in the narrow multi-byte encoding of the
/// specified locale.
///
/// This function is a shorthand for constructing a string encoder from the
/// specified locale, calling \ref BasicStringEncoder<C, T>::encode() with the
/// specified string and \p strict flag, and returning a copy of the produced
/// string.
///
template<class C, class T>
std::string encode_string(std::basic_string_view<C, T> string, const std::locale& locale = {},
                          bool strict = false);




/// \brief Locale dependent encoding and decoding of strings.
///
/// This class provides services based on those provided by `std::codecvt<C,
/// char, std::mbstate_t>`.
///
/// The encoding operation converts a string expressed in the wide character
/// encoding of the selected locale to a string expressed in the narrow
/// multi-byte encoding of the selected locale. The decoding operation does the
/// opposite.
///
/// The purpose of this class is to make it easy and efficient to perform these
/// operations. Memory allocated during one encoding operation will be reused
/// during the next, so the amortized memory allocation cost for a single
/// encoding operation is zero. Similarly for decoding operations.
///
template<class C, class T = std::char_traits<C>> class BasicStringCodec {
public:
    class EncodeBuffer;
    template<std::size_t N> class ArraySeededEncodeBuffer;

    using char_type = C;
    using traits_type = T;
    using string_view_type = std::basic_string_view<C, T>;

    /// \brief Encode the specified string.
    ///
    /// In lenient mode (when `false` is passed for \p strict), characters in
    /// the the specified string that are invalid, or incomplete according to
    /// the wide character encoding of the selected locale, and characters that
    /// have no representation in the narrow multi-byte encoding will not cause
    /// the encoding operation to fail. Instead, these characters will be
    /// replaced by a replacement character. The replacement character is U+FFFD
    /// (REPLACEMENT CHARACTER) when Unicode support is detected. Otherwise it
    /// is `?`, i.e., a character that is a member of the basic source character
    /// set.
    ///
    /// In strict mode (when `true` is passed for \p strict), this function has
    /// the same effect as \ref try_encode() except that it throws an exception
    /// of unspecified type if the encoding operation fails, i.e., if the string
    /// contains characters that cannot be encoded.
    ///
    /// The encoded form, that is referenced by the returned string view, is
    /// stored in the specified buffer (\p buffer), which means that it will be
    /// clobbered during a subsequent encoding operation that uses the same
    /// buffer.
    ///
    std::string_view encode(string_view_type string, EncodeBuffer& buffer,
                            bool strict = false) const;

    /// \brief Encode the specified string in strict mode.
    ///
    /// This function is a shorthand for calling \ref encode() with `true`
    /// passed as argument for `strict`.
    ///
    std::string_view strict_encode(string_view_type string, EncodeBuffer& buffer) const;

    /// \brief Try to encode the specified string.
    ///
    /// This function tries to encode the specified string (\p string). If
    /// successful, the encoded form is made available through the \p result
    /// argument, and `true` is returned. Otherwise `false` is returned. When
    /// `false` is returned, this function does not touch \p result.
    ///
    /// The encoding operation succeeds if, and only if all the characters
    /// contained in the specified string are valid, and complete according to
    /// the wide character encoding of the selected locale, and representations
    /// exist for all of them in the narrow multi-byte encoding.
    ///
    /// The encoded form, that is made available through \p result, is stored in
    /// the specified buffer (\p buffer), which means that it will be clobbered
    /// during a subsequent encoding operation that uses the same buffer.
    ///
    bool try_encode(string_view_type string, EncodeBuffer& buffer, std::string_view& result) const;

    /// \brief Construct a string codec object.
    ///
    /// Construct an object thar encodes, and decodes of strings according to
    /// the code conversion facet of the specified locale (`std::codecvt<C,
    /// char, std::mbstate_t>`).
    ///
    explicit BasicStringCodec(const std::locale&);

    ~BasicStringCodec() noexcept = default;

private:
    using codecvt_type = std::codecvt<C, char, std::mbstate_t>;

    // Must be large enough to allow code conversion to advance
    static constexpr std::size_t s_reserve_size = 64;

    const std::locale m_locale;
    const codecvt_type& m_codecvt;
    const C m_replacement_char;
    const bool m_degen = m_codecvt.always_noconv();
    const bool m_stateful = (m_codecvt.encoding() == -1);
    const std::string m_encoded_replacement_char;

    static C determine_replacement_char(const std::locale&, const codecvt_type&);
    static std::string get_encoded_replacement_char(C ch, bool stateful, const codecvt_type&);
    static bool try_encode_replacement_char(C ch, const codecvt_type&, std::string* out);

    bool do_try_encode(string_view_type string, EncodeBuffer&, bool strict,
                       std::string_view& result) const;
};


using StringCodec     = BasicStringCodec<char>;
using WideStringCodec = BasicStringCodec<wchar_t>;




template<class C, class T> class BasicStringCodec<C, T>::EncodeBuffer :
        private SeedMemoryBuffer<char> {
public:
    explicit EncodeBuffer(Span<char> seed_memory = {}) noexcept;
    ~EncodeBuffer() noexcept = default;

    friend class BasicStringCodec<C, T>;
};




template<class C, class T>
template<std::size_t N> class BasicStringCodec<C, T>::ArraySeededEncodeBuffer :
        private std::array<char, N>,
        public EncodeBuffer {
public:
    ArraySeededEncodeBuffer() noexcept;
    ~ArraySeededEncodeBuffer() noexcept = default;
};




/// \brief Locale dependent encoding of strings.
///
/// This class combines a string codec (\ref BasicStringCodec<C, T>) with an
/// encoding buffer (\ref BasicStringCodec<C, T>::EncodeBuffer). This makes
/// sense when the application has no need for the decoding operation.
///
/// Note, however, that while a string codec object (\ref BasicStringCodec<C,
/// T>) can be used by multiple threads, each one with its own buffer, a string
/// encoder can be used by only one thread at a time.
///
template<class C, class T = std::char_traits<C>> class BasicStringEncoder :
        private BasicStringCodec<C, T>,
        private BasicStringCodec<C, T>::EncodeBuffer {
public:
    using string_view_type = typename BasicStringCodec<C, T>::string_view_type;

    explicit BasicStringEncoder(const std::locale&, Span<char> seed_memory = {});
    ~BasicStringEncoder() noexcept = default;

    /// \brief Encode the specified string.
    ///
    /// See \ref BasicStringCodec<C, T>::encode().
    ///
    std::string_view encode(string_view_type string, bool strict = false);

    /// \brief Encode the specified string in strict mode.
    ///
    /// See \ref BasicStringCodec<C, T>::strict_encode().
    ///
    std::string_view strict_encode(string_view_type string);

    /// \brief Try to encode the specified string.
    ///
    /// See \ref BasicStringCodec<C, T>::try_encode().
    ///
    bool try_encode(string_view_type string, std::string_view& result);
};


using StringEncoder     = BasicStringEncoder<char>;
using WideStringEncoder = BasicStringEncoder<wchar_t>;








// Implementation


template<class C, class T>
std::string encode_string(std::basic_string_view<C, T> string, const std::locale& locale,
                          bool strict)
{
    BasicStringEncoder<C, T> encoder(locale); // Throws
    std::string_view encoded = encoder.encode(string, strict); // Throws
    return std::string(encoded); // Throws
}


// ============================ BasicStringCodec ============================


template<class C, class T>
inline auto BasicStringCodec<C, T>::encode(string_view_type string, EncodeBuffer& buffer,
                                           bool strict) const -> std::string_view
{
    std::string_view result;
    if (ARCHON_LIKELY(do_try_encode(string, buffer, strict, result))) // Throws
        return result;
    throw std::runtime_error("Failed to encode string: Contains unencodable characters");
}


template<class C, class T>
inline auto BasicStringCodec<C, T>::strict_encode(string_view_type string,
                                                  EncodeBuffer& buffer) const -> std::string_view
{
    bool strict = true;
    return encode(string, buffer, strict); // Throws
}


template<class C, class T>
inline bool BasicStringCodec<C, T>::try_encode(string_view_type string, EncodeBuffer& buffer,
                                               std::string_view& result) const
{
    bool strict = true;
    return do_try_encode(string, buffer, strict, result); // Throws
}


template<class C, class T> BasicStringCodec<C, T>::BasicStringCodec(const std::locale& locale) :
    m_locale(locale),
    m_codecvt(std::use_facet<codecvt_type>(m_locale)), // Throws
    m_replacement_char(determine_replacement_char(m_locale, m_codecvt)), // Throws
    m_encoded_replacement_char(get_encoded_replacement_char(m_replacement_char, m_stateful,
                                                            m_codecvt)) // Throws
{
}


template<class C, class T>
C BasicStringCodec<C, T>::determine_replacement_char(const std::locale& locale,
                                                     const codecvt_type& codecvt)
{
#if ARCHON_WCHAR_IS_UNICODE
    if constexpr (std::is_same_v<C, wchar_t>) {
        wchar_t ch = T::to_char_type(0xFFFD);
        std::string* out = nullptr;
        if (try_encode_replacement_char(ch, codecvt, out)) // Throws
            return ch;
    }
#else
    static_cast<void>(codecvt);
#endif
    BasicCharMapper<C> mapper(locale); // Throws
    return mapper.widen('?'); // Throws
}


template<class C, class T>
std::string BasicStringCodec<C, T>::get_encoded_replacement_char(C ch, bool stateful,
                                                                 const codecvt_type& codecvt)
{
    if (!stateful) {
        std::string out;
        if (try_encode_replacement_char(ch, codecvt, &out)) // Throws
            return out;
        ARCHON_ASSERT(false);
    }
    return {};
}


template<class C, class T>
bool BasicStringCodec<C, T>::try_encode_replacement_char(C ch, const codecvt_type& codecvt,
                                                         std::string* result)
{
    std::mbstate_t state {};
    const C* in_begin = &ch;
    const C* in_end   = &ch + 1;
    const C* in_pos   = nullptr;
    char buffer[s_reserve_size];
    char* out_begin = buffer;
    char* out_end   = buffer + s_reserve_size;
    char* out_pos   = nullptr;
    std::codecvt_base::result result_2 =
        codecvt.out(state, in_begin, in_end, in_pos, out_begin, out_end, out_pos);
    switch (result_2) {
        case std::codecvt_base::ok:
            if (in_pos == in_end) {
                ARCHON_ASSERT(out_pos > out_begin);
                if (result)
                    *result = { out_begin, out_pos }; // Throws
                return true;
            }
            break;
        case std::codecvt_base::partial:
        case std::codecvt_base::error:
            break;
        case std::codecvt_base::noconv:
            if constexpr (std::is_same_v<C, char>) {
                if (result)
                    *result = { &ch, &ch + 1 }; // Throws
                return true;
            }
            ARCHON_ASSERT(false);
            break;
    }
    return false;
}


template<class C, class T>
bool BasicStringCodec<C, T>::do_try_encode(string_view_type string, EncodeBuffer& buffer,
                                           bool strict, std::string_view& result) const
{
    if constexpr (std::is_same_v<C, char>) {
        if (ARCHON_LIKELY(m_degen)) {
            result = { string.data(), string.size() };
            return true;
        }
    }
    else {
        ARCHON_ASSERT(!m_degen);
    }
    std::mbstate_t state {};
    const C* in_begin = string.data();
    const C* in_end   = string.data() + string.size();
    char* out_begin = nullptr;
    char* out_end   = nullptr;
    std::size_t out_size = 0;
    auto reserve = [&](std::size_t extra_size) {
        buffer.reserve_extra(extra_size, out_size); // Throws
    };
    if (ARCHON_UNLIKELY(buffer.size() == 0))
        reserve(s_reserve_size); // Throws
    const C* in_pos  = nullptr;
    char*    out_pos = nullptr;
    auto update_out = [&]() noexcept {
        out_size = std::size_t(out_pos - buffer.data());
    };
    auto update = [&]() noexcept {
        in_begin = in_pos;
        update_out();
    };
    std::codecvt_base::result result_2;
  again:
    out_begin = buffer.data() + out_size;
    out_end   = buffer.data() + buffer.size();
    result_2 = m_codecvt.out(state, in_begin, in_end, in_pos,
                             out_begin, out_end, out_pos); // Throws
    switch (result_2) {
        case std::codecvt_base::ok:
            update();
            if (ARCHON_LIKELY(in_begin == in_end))
                goto unshift;
            goto encode_error;
        case std::codecvt_base::partial:
            update();
            if (ARCHON_LIKELY(buffer.size() - out_size < s_reserve_size)) {
                reserve(s_reserve_size); // Throws
                goto again;
            }
            if (ARCHON_LIKELY(in_begin != in_end))
                goto encode_error;
            goto reserve_size_error;
        case std::codecvt_base::error:
            update();
            if (ARCHON_LIKELY(in_begin != in_end))
                goto encode_error;
            break;
        case std::codecvt_base::noconv:
            if constexpr (std::is_same_v<C, char>) {
                if (ARCHON_LIKELY(in_begin == string.data())) {
                    result = { string.data(), string.size() };
                    return true;
                }
                std::size_t size = std::size_t(in_end - in_begin);
                reserve(size); // Throws
                out_begin = buffer.data() + out_size;
                out_pos = std::copy_n(in_begin, size, out_begin);
                update_out();
                goto unshift;
            }
            break;
    }
    ARCHON_ASSERT(false);

  unshift:
    if (ARCHON_LIKELY(!m_stateful))
        goto done;
  again_2:
    out_begin = buffer.data() + out_size;
    out_end   = buffer.data() + buffer.size();
    result_2 = m_codecvt.unshift(state, out_begin, out_end, out_pos); // Throws
    switch (result_2) {
        case std::codecvt_base::ok:
            update_out();
            goto done;
        case std::codecvt_base::partial:
            update_out();
            if (ARCHON_LIKELY(buffer.size() - out_size < s_reserve_size)) {
                reserve(s_reserve_size); // Throws
                goto again_2;
            }
            goto reserve_size_error;
        case std::codecvt_base::error:
            throw std::runtime_error("Failed to encode string: Error during unshift");
        case std::codecvt_base::noconv:
            goto done;
    }
    ARCHON_ASSERT(false);

  done:
    result = { buffer.data(), out_size };
    return true;

  encode_error:
    if (ARCHON_LIKELY(!strict)) {
        if (ARCHON_LIKELY(!m_stateful)) {
            std::string_view str = m_encoded_replacement_char;
            reserve(str.size()); // Throws
            out_begin = buffer.data() + out_size;
            out_pos = std::copy_n(str.data(), str.size(), out_begin);
            update_out();
            ++in_begin;
            goto again;
        }
        const C& ch = m_replacement_char;
      again_3:
        out_begin = buffer.data() + out_size;
        out_end   = buffer.data() + buffer.size();
        result_2 = m_codecvt.out(state, &ch, &ch + 1, in_pos, out_begin, out_end, out_pos);
        switch (result_2) {
            case std::codecvt_base::ok:
                ARCHON_ASSERT(in_pos == &ch + 1);
                update_out();
                ++in_begin;
                goto again;
            case std::codecvt_base::partial:
                ARCHON_ASSERT(in_pos == &ch);
                update_out();
                if (ARCHON_LIKELY(buffer.size() - out_size < s_reserve_size)) {
                    reserve(s_reserve_size); // Throws
                    goto again_3;
                }
                goto reserve_size_error;
            case std::codecvt_base::error:
                break;
            case std::codecvt_base::noconv:
                if constexpr (std::is_same_v<C, char>) {
                    *out_begin = ch;
                    out_pos = out_begin + 1;
                    update_out();
                    ++in_begin;
                    goto again;
                }
                ARCHON_ASSERT(false);
                break;
        }
        throw std::runtime_error("Failed to encode string: "
                                 "Could not encode replacement character");
    }
    return false;

  reserve_size_error:
    throw std::runtime_error("Failed to encode string: "
                             "Unexpected demand for buffer space");
}


// ============================ BasicStringCodec::EncodeBuffer ============================


template<class C, class T>
inline BasicStringCodec<C, T>::EncodeBuffer::EncodeBuffer(Span<char> seed_memory) noexcept :
    SeedMemoryBuffer<char>(seed_memory)
{
}


// ============================ BasicStringCodec::ArraySeededEncodeBuffer ============================


template<class C, class T> template<std::size_t N>
inline BasicStringCodec<C, T>::ArraySeededEncodeBuffer<N>::ArraySeededEncodeBuffer() noexcept :
    EncodeBuffer(static_cast<std::array<char, N>&>(*this))
{
}


// ============================ BasicStringEncoder ============================


template<class C, class T>
inline BasicStringEncoder<C, T>::BasicStringEncoder(const std::locale& locale,
                                                    Span<char> seed_memory) :
    BasicStringCodec<C, T>(locale), // Throws
    BasicStringCodec<C, T>::EncodeBuffer(seed_memory)
{
}


template<class C, class T>
inline auto BasicStringEncoder<C, T>::encode(string_view_type string, bool strict) ->
    std::string_view
{
    return BasicStringCodec<C, T>::encode(string, *this, strict); // Throws
}


template<class C, class T>
inline auto BasicStringEncoder<C, T>::strict_encode(string_view_type string) -> std::string_view
{
    return BasicStringCodec<C, T>::strict_encode(string, *this); // Throws
}


template<class C, class T>
inline bool BasicStringEncoder<C, T>::try_encode(string_view_type string, std::string_view& result)
{
    return BasicStringCodec<C, T>::try_encode(string, *this, result); // Throws
}


} // namespace archon::base

#endif // ARCHON__BASE__STRING_CODEC_HPP
