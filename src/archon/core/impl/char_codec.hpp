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

#ifndef ARCHON_X_CORE_X_IMPL_X_CHAR_CODEC_HPP
#define ARCHON_X_CORE_X_IMPL_X_CHAR_CODEC_HPP


#include <cstddef>
#include <cwchar>
#include <type_traits>
#include <limits>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <array>
#include <string_view>
#include <string>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/char_codec_config.hpp>
#include <archon/core/impl/codecvt_quirks.hpp>


namespace archon::core::impl {


// Variant: Degenerate, without leniency mode
template<class T> class CharCodec1 {
public:
    using char_type = char;
    using traits_type = T;

    using Config = core::SimpleCharCodecConfig<char, T>;

    static_assert(std::is_same_v<typename T::state_type, std::mbstate_t>);

    static constexpr bool is_degen = true;

    explicit CharCodec1(const std::locale&) noexcept;
    explicit CharCodec1(const std::locale&, Config) noexcept;
    explicit CharCodec1(const std::locale*, Config) noexcept;

    void imbue(const std::locale&) noexcept;

    bool is_stateless() const noexcept;

    bool decode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset, bool end_of_data,
                core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const noexcept;

    bool encode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset,
                core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const noexcept;

    bool unshift(std::mbstate_t&, core::Span<char> buffer, std::size_t& buffer_offset) const noexcept;

    void simul_decode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset,
                      std::size_t buffer_size) const noexcept;

    static constexpr auto max_simul_decode_size() noexcept -> std::size_t;
};



// Variant: Non-degenerate, without leniency mode
template<class C, class T> class CharCodec2 {
public:
    using char_type = C;
    using traits_type = T;

    using Config = core::SimpleCharCodecConfig<C, T>;

    static_assert(!std::is_same_v<char_type, char>);
    static_assert(std::is_same_v<typename T::state_type, std::mbstate_t>);

    static constexpr bool is_degen = false;

    explicit CharCodec2(const std::locale&);
    explicit CharCodec2(const std::locale&, Config);
    explicit CharCodec2(const std::locale*, Config);

    void imbue(const std::locale&);

    bool is_stateless() const noexcept;

    bool decode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset, bool end_of_data,
                core::Span<char_type> buffer, std::size_t& buffer_offset, bool& error) const;

    bool encode(std::mbstate_t&, core::Span<const char_type> data, std::size_t& data_offset,
                core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const;

    bool unshift(std::mbstate_t&, core::Span<char> buffer, std::size_t& buffer_offset) const;

    void simul_decode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset,
                      std::size_t buffer_size) const;

    static constexpr auto max_simul_decode_size() noexcept -> std::size_t;

protected:
    using codecvt_type = std::codecvt<char_type, char, std::mbstate_t>;

    std::locale m_locale;
    const codecvt_type* m_codecvt;
    bool m_is_stateless;

private:
    void finalize_imbue();
};



// Variant: Degenerate, with leniency mode
template<class T> class CharCodec3
    : public CharCodec1<T> {
public:
    using Config = core::CharCodecConfig<char, T>;

    using CharCodec1<T>::CharCodec1;
};



// Variant: Non-degenerate, with leniency mode
template<class C, class T> class CharCodec4
    : public CharCodec2<C, T> {
public:
    using char_type = C;
    using traits_type = T;

    using Config = core::CharCodecConfig<C, T>;

    explicit CharCodec4(const std::locale&, Config = {});
    explicit CharCodec4(const std::locale*, Config);

    bool decode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset, bool end_of_data,
                core::Span<char_type> buffer, std::size_t& buffer_offset, bool& error) const;

    bool encode(std::mbstate_t&, core::Span<const char_type> data, std::size_t& data_offset,
                core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const;

private:
    struct ReplacementInfo {
        char_type char_;
        std::string encoded_char;
    };

    using codecvt_type = std::codecvt<char_type, char, std::mbstate_t>;

    ReplacementInfo m_replacement_info;
    bool m_lenient;

    static ReplacementInfo get_replacement_info(const std::locale&, const codecvt_type&,
                                                const Config&);
    bool decode_replacement(std::mbstate_t& state, core::Span<const char> data, std::size_t& data_offset,
                            bool end_of_data, core::Span<char_type> buffer, std::size_t& buffer_offset,
                            bool& need_more_data) const;
    bool encode_replacement(std::mbstate_t& state, std::size_t& data_offset,
                            core::Span<char> buffer, std::size_t& buffer_offset) const;
};



template<class C, class T> class SimpleCharCodec
    : public CharCodec2<C, T> {
public:
    using CharCodec2<C, T>::CharCodec2;
};

template<class T> class SimpleCharCodec<char, T>
    : public CharCodec1<T> {
public:
    using CharCodec1<T>::CharCodec1;
};



template<class C, class T> class CharCodec
    : public CharCodec4<C, T> {
public:
    using CharCodec4<C, T>::CharCodec4;
};

template<class T> class CharCodec<char, T>
    : public CharCodec3<T> {
public:
    using CharCodec3<T>::CharCodec3;
};








// Implementation


// ============================ CharCodec1<T> ============================


template<class T>
inline CharCodec1<T>::CharCodec1(const std::locale&) noexcept
{
}


template<class T>
inline CharCodec1<T>::CharCodec1(const std::locale&, Config) noexcept
{
}


template<class T>
inline CharCodec1<T>::CharCodec1(const std::locale*, Config) noexcept
{
}


template<class T>
inline void CharCodec1<T>::imbue(const std::locale&) noexcept
{
}


template<class T>
inline bool CharCodec1<T>::is_stateless() const noexcept
{
    return true;
}


template<class T>
inline bool CharCodec1<T>::decode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset, bool,
                                  core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(buffer_offset <= buffer.size());
    std::size_t n = std::min(std::size_t(data.size() - data_offset),
                             std::size_t(buffer.size() - buffer_offset));
    std::copy_n(data.data() + data_offset, n, buffer.data() + buffer_offset);
    data_offset += n;
    buffer_offset += n;
    if (ARCHON_LIKELY(data_offset == data.size()))
        return true;
    error = false;
    return false;
}


template<class T>
inline bool CharCodec1<T>::encode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset,
                                  core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(buffer_offset <= buffer.size());
    std::size_t n = std::min(std::size_t(data.size() - data_offset),
                             std::size_t(buffer.size() - buffer_offset));
    std::copy_n(data.data() + data_offset, n, buffer.data() + buffer_offset);
    data_offset += n;
    buffer_offset += n;
    if (ARCHON_LIKELY(data_offset == data.size()))
        return true;
    error = false;
    return false;
}


template<class T>
inline bool CharCodec1<T>::unshift(std::mbstate_t&, core::Span<char> buffer, std::size_t& buffer_offset) const noexcept
{
    ARCHON_ASSERT(buffer_offset <= buffer.size());
    return true;
}


template<class T>
inline void CharCodec1<T>::simul_decode(std::mbstate_t&, core::Span<const char> data, std::size_t& data_offset,
                                        std::size_t buffer_size) const noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());
    if (buffer_size <= data.size() - data_offset) {
        data_offset += buffer_size;
    }
    else {
        data_offset = data.size();
    }
}


template<class T>
constexpr auto CharCodec1<T>::max_simul_decode_size() noexcept -> std::size_t
{
    return std::size_t(-1);
}



// ============================ CharCodec2<C, T> ============================


template<class C, class T>
inline CharCodec2<C, T>::CharCodec2(const std::locale& locale)
    : m_locale(locale)
{
    finalize_imbue(); // Throws
}


template<class C, class T>
inline CharCodec2<C, T>::CharCodec2(const std::locale& locale, Config)
    : CharCodec2(locale) // Throws
{
}


template<class C, class T>
inline CharCodec2<C, T>::CharCodec2(const std::locale* locale, Config)
    : CharCodec2(locale ? *locale : std::locale()) // Throws
{
}


template<class C, class T>
inline void CharCodec2<C, T>::imbue(const std::locale& locale)
{
    std::locale orig = m_locale;
    m_locale = locale;
    try {
        finalize_imbue(); // Throws
    }
    catch (...) {
        m_locale = std::move(orig);
        throw;
    }
}


template<class C, class T>
inline bool CharCodec2<C, T>::is_stateless() const noexcept
{
    return m_is_stateless;
}


template<class C, class T>
bool CharCodec2<C, T>::decode(std::mbstate_t& state, core::Span<const char> data, std::size_t& data_offset,
                              bool end_of_data, core::Span<char_type> buffer, std::size_t& buffer_offset,
                              bool& error) const
{
    if constexpr (impl::codecvt_quirk_ok_result_on_zero_size_buffer) {
        bool has_buffer_space = (buffer_offset < buffer.size());
        if (ARCHON_UNLIKELY(!has_buffer_space)) {
            bool has_data = (data_offset < data.size());
            if (ARCHON_LIKELY(has_data)) {
                error = false;
                return false;
            }
            return true;
        }
    }

    std::mbstate_t state_2 = state;
    std::size_t data_offset_2 = data_offset;

  again:
    std::size_t data_size = std::size_t(data.size() - data_offset_2);
    std::size_t data_size_2 = std::min(data_size, max_simul_decode_size());
    std::mbstate_t state_3 = state_2;
    const char* from      = data.data() + data_offset_2;
    const char* from_end  = from + data_size_2;
    const char* from_next = nullptr;
    char_type* to      = buffer.data() + buffer_offset;
    char_type* to_end  = buffer.data() + buffer.size();
    char_type* to_next = nullptr;
    std::codecvt_base::result result =
        m_codecvt->in(state_3, from, from_end, from_next, to, to_end, to_next); // Throws

    // If both the "partial result instead of ok result" and "partial result instead of
    // error result" quirks were present, we would not know whether "partial" means "ok" or
    // "error".
    static_assert(!(impl::codecvt_quirk_partial_result_on_partial_char &&
                    impl::codecvt_quirk_partial_result_on_invalid_byte_seq));

    switch (result) {
        case std::codecvt_base::ok:
            goto ok;
        case std::codecvt_base::partial:
            if constexpr (impl::codecvt_quirk_partial_result_on_partial_char) {
                if (to_next < to_end)
                    goto ok;
            }
            else if constexpr (impl::codecvt_quirk_partial_result_on_invalid_byte_seq) {
                if (to_next < to_end)
                    goto error;
            }
            state = state_2;
            data_offset   = std::size_t(from_next - data.data());
            buffer_offset = std::size_t(to_next - buffer.data());
            ARCHON_ASSERT(buffer_offset == buffer.size());
            error = false;
            return false;
        case std::codecvt_base::error:
            ARCHON_ASSERT(from_next < from_end);
            // Assumption: So long as the state passed to std::codecvt::in() is the
            // zero-initialized state or is the result of decoding a number of complete and
            // valid input sequences, std::codecvt::in() will never have consumed part of an
            // input sequence when it reports an error, so, in this case, what lies between
            // `from` and `from_next` is zero or more complete and valid input sequences.
            goto error;
        case std::codecvt_base::noconv:
            // Since std::codecvt_base::noconv implies that the internal character type is
            // `char`, and since CharCodec2 is not used for that case, we can never get
            // here.
            bool noconv_case = true;
            ARCHON_ASSERT(!noconv_case);
            break;
    }
    goto error;

  ok:
    // Revert back to end of last completely consumed byte sequence
    {
        // Assumption: So long as the input passed to std::codecvt::length() is some prefix
        // of valid input, and so long as the `max` argument passed to
        // std::codecvt::length() is less than, or equal to the number of logical characters
        // present in that prefix, std::codecvt::length() will never consume a partial input
        // sequence.
        //
        // In particular, for a stateful encoding, if the input prefix is a complete
        // sequence corresponding to one logical character followed by half of a
        // state-changing sequence, then std::codecvt::length() will end its consumption of
        // input before the partial state-changing sequence when `max` is 1.
        //
        ARCHON_ASSERT(std::size_t(from_end - from) <= max_simul_decode_size());
        int n = m_codecvt->length(state_2, from, from_end, std::size_t(to_next - to)); // Throws
        from_next = from + n;
    }
    if (ARCHON_LIKELY(data_size_2 == data_size)) {
        if (ARCHON_LIKELY(!end_of_data || (from_next == from_end))) {
            state = state_2;
            data_offset   = std::size_t(from_next - data.data());
            buffer_offset = std::size_t(to_next - buffer.data());
            return true;
        }
        goto error;
    }
    {
        std::size_t new_data_offset = std::size_t(from_next - data.data());
        if (ARCHON_LIKELY(new_data_offset < data_offset_2)) {
            data_offset_2 = new_data_offset;
            goto again;
        }
    }
    throw std::runtime_error("Unexpected lack of decoding progress");

  error:
    state = state_2;
    data_offset   = std::size_t(from_next - data.data());
    buffer_offset = std::size_t(to_next - buffer.data());
    error = true;
    return false;
}


template<class C, class T>
bool CharCodec2<C, T>::encode(std::mbstate_t& state, core::Span<const char_type> data, std::size_t& data_offset,
                              core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const
{
    if constexpr (impl::codecvt_quirk_ok_result_on_zero_size_buffer) {
        bool has_buffer_space = (buffer_offset < buffer.size());
        if (ARCHON_UNLIKELY(!has_buffer_space)) {
            bool has_data = (data_offset < data.size());
            if (ARCHON_LIKELY(has_data)) {
                error = false;
                return false;
            }
            return true;
        }
    }

    std::mbstate_t state_2 = state;
    const char_type* from      = data.data() + data_offset;
    const char_type* from_end  = data.data() + data.size();
    const char_type* from_next = nullptr;
    char* to      = buffer.data() + buffer_offset;
    char* to_end  = buffer.data() + buffer.size();
    char* to_next = nullptr;
    std::codecvt_base::result result =
        m_codecvt->out(state_2, from, from_end, from_next, to, to_end, to_next); // Throws

    state = state_2;
    data_offset   = std::size_t(from_next - data.data());
    buffer_offset = std::size_t(to_next - buffer.data());

    switch (result) {
        case std::codecvt_base::ok:
            ARCHON_ASSERT(data_offset == data.size());
            return true;
        case std::codecvt_base::partial:
            error = false;
            return false;
        case std::codecvt_base::error:
            break;
        case std::codecvt_base::noconv:
            // Since std::codecvt_base::noconv implies that the internal character type is
            // `char`, and since CharCodec2 is not used for that case, we can never get
            // here.
            bool noconv_case = true;
            ARCHON_ASSERT(!noconv_case);
            break;
    }

    error = true;
    return false;
}


template<class C, class T>
bool CharCodec2<C, T>::unshift(std::mbstate_t& state, core::Span<char> buffer, std::size_t& buffer_offset) const
{
    std::mbstate_t state_2 = state;
    char* to      = buffer.data() + buffer_offset;
    char* to_end  = buffer.data() + buffer.size();
    char* to_next = nullptr;
    std::codecvt_base::result result = m_codecvt->unshift(state_2, to, to_end, to_next); // Throws

    state = state_2;
    buffer_offset = std::size_t(to_next - buffer.data());

    switch (result) {
        case std::codecvt_base::ok:
        case std::codecvt_base::noconv:
            return true;
        case std::codecvt_base::partial:
            return false;
        case std::codecvt_base::error:
            // This case makes no sense, but the C++ standard (C++20) allows for it
            // (unshift() is allowed to return `error`).
            throw std::runtime_error("Unshift error");
    }

    return true;
}


template<class C, class T>
void CharCodec2<C, T>::simul_decode(std::mbstate_t& state, core::Span<const char> data, std::size_t& data_offset,
                                    std::size_t buffer_size) const
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(std::size_t(data.size() - data_offset) <= max_simul_decode_size());
    const char* begin = data.data() + data_offset;
    const char* end   = data.data() + data.size();
    int n = m_codecvt->length(state, begin, end, buffer_size);
    ARCHON_ASSERT(n >= 0);
    data_offset += std::size_t(n);
}


template<class C, class T>
constexpr auto CharCodec2<C, T>::max_simul_decode_size() noexcept -> std::size_t
{
    std::size_t max_1 = std::numeric_limits<std::size_t>::max();
    int max_2 = std::numeric_limits<int>::max();
    using uint = unsigned int;
    if (max_1 >= uint(max_2))
        return max_1;
    return std::size_t(max_2);
}


template<class C, class T>
inline void CharCodec2<C, T>::finalize_imbue()
{
    m_codecvt = &std::use_facet<codecvt_type>(m_locale); // Throws
    m_is_stateless = (m_codecvt->encoding() != -1);
}



// ============================ CharCodec4<C, T> ============================


template<class C, class T>
inline CharCodec4<C, T>::CharCodec4(const std::locale& locale, Config config)
    : CharCodec2<C, T>(locale) // Throws
    , m_replacement_info(get_replacement_info(this->m_locale, *this->m_codecvt, config)) // Throws
    , m_lenient(config.lenient)
{
}


template<class C, class T>
inline CharCodec4<C, T>::CharCodec4(const std::locale* locale, Config config)
    : CharCodec4<C, T>(locale ? *locale : std::locale(), std::move(config)) // Throws
{
}


template<class C, class T>
inline bool CharCodec4<C, T>::decode(std::mbstate_t& state, core::Span<const char> data, std::size_t& data_offset,
                                     bool end_of_data, core::Span<char_type> buffer, std::size_t& buffer_offset,
                                     bool& error) const
{
    std::mbstate_t state_2 = state;
    std::size_t data_offset_2 = data_offset;
    std::size_t buffer_offset_2 = buffer_offset;
    bool error_2 = false;
  again:
    bool complete = CharCodec2<C, T>::decode(state_2, data, data_offset_2, end_of_data,
                                             buffer, buffer_offset_2, error_2); // Throws
    if (ARCHON_LIKELY(complete)) {
      complete:
        state = state_2;
        data_offset = data_offset_2;
        buffer_offset = buffer_offset_2;
        return true;
    }
    if (ARCHON_LIKELY(!error_2 || !m_lenient)) {
      incomplete:
        state = state_2;
        data_offset = data_offset_2;
        buffer_offset = buffer_offset_2;
        error = error_2;
        return false;
    }
    ARCHON_ASSERT(data_offset_2 < data.size());
    bool need_more_data = false;
    bool success = decode_replacement(state_2, data, data_offset_2, end_of_data, buffer, buffer_offset_2,
                                      need_more_data); // Throws
    if (ARCHON_LIKELY(success))
        goto again;
    if (ARCHON_LIKELY(need_more_data))
        goto complete;
    error_2 = false;
    goto incomplete;
}


template<class C, class T>
inline bool CharCodec4<C, T>::encode(std::mbstate_t& state, core::Span<const char_type> data, std::size_t& data_offset,
                                     core::Span<char> buffer, std::size_t& buffer_offset, bool& error) const
{
    std::mbstate_t state_2 = state;
    std::size_t data_offset_2 = data_offset;
    std::size_t buffer_offset_2 = buffer_offset;
    bool error_2 = false;
  again:
    bool complete = CharCodec2<C, T>::encode(state_2, data, data_offset_2, buffer, buffer_offset_2, error_2); // Throws
    if (ARCHON_LIKELY(complete)) {
        state = state_2;
        data_offset = data_offset_2;
        buffer_offset = buffer_offset_2;
        return true;
    }
    if (ARCHON_LIKELY(!error_2 || !m_lenient)) {
      incomplete:
        state = state_2;
        data_offset = data_offset_2;
        buffer_offset = buffer_offset_2;
        error = error_2;
        return false;
    }
    ARCHON_ASSERT(data_offset_2 < data.size());
    bool success = encode_replacement(state_2, data_offset_2, buffer, buffer_offset_2); // Throws
    if (ARCHON_LIKELY(success))
        goto again;
    error_2 = false;
    goto incomplete;
}


template<class C, class T>
auto CharCodec4<C, T>::get_replacement_info(const std::locale& locale, const codecvt_type& codecvt,
                                            const Config& config) -> ReplacementInfo
{
    if (ARCHON_LIKELY(!config.lenient))
        return {};

#if ARCHON_WCHAR_IS_UNICODE

    if constexpr (std::is_same_v<char_type, wchar_t>) {
        if (!config.use_fallback_replacement_char) {
            wchar_t ch = traits_type::to_char_type(0xFFFD);
            std::mbstate_t state {};
            const C* in_begin = &ch;
            const C* in_end   = &ch + 1;
            const C* in_pos   = nullptr;
            core::ArraySeededBuffer<char, 8> buffer;
            std::size_t min_extra_size = std::size_t(codecvt.max_length());
            std::size_t used_size = 0;
            for (;;) {
                buffer.reserve_extra(min_extra_size, used_size); // Throws
                char* out_begin = buffer.data() + used_size;
                char* out_end   = buffer.data() + buffer.size();
                char* out_pos   = nullptr;
                std::codecvt_base::result result =
                    codecvt.out(state, in_begin, in_end, in_pos, out_begin, out_end, out_pos); // Throws
                switch (result) {
                    case std::codecvt_base::ok:
                        ARCHON_ASSERT(in_pos == in_end);
                        ARCHON_ASSERT(out_pos > out_begin);
                        return { ch, std::string(buffer.data(), out_pos) }; // Throws
                    case std::codecvt_base::partial:
                        ARCHON_ASSERT(out_pos > out_begin);
                        used_size = std::size_t(out_pos - buffer.data());
                        continue;
                    case std::codecvt_base::error:
                        break;
                    case std::codecvt_base::noconv:
                        ARCHON_ASSERT(false);
                        break;
                }
                break;
            }
        }
    }

#else
    static_cast<void>(codecvt);
#endif

    core::BasicCharMapper<char_type> mapper(locale); // Throws
    char ch = '?';
    char_type ch_2 = mapper.widen(ch); // Throws
    return { ch_2, std::string(1, ch) }; // Throws
}


template<class C, class T>
bool CharCodec4<C, T>::decode_replacement(std::mbstate_t& state, core::Span<const char> data, std::size_t& data_offset,
                                          bool end_of_data, core::Span<char_type> buffer, std::size_t& buffer_offset,
                                          bool& need_more_data) const
{
    // The idea here, when an invalid byte sequence is detected, is to search for the first
    // byte that is the start of a valid byte sequence, starting the search from the second
    // byte following the last valid byte sequence. Then, when the start of a new valid byte
    // sequence is found, replace all skipped bytes with a single replacement character.
    //
    // FIXME: Unfortunately, this desired behavior is not guaranteed with the implementation
    // below because implementations of `codecvt.in()` are not required to report "error"
    // when presented with just one byte that is an invalid start of a byte sequence.

  again:
    if (ARCHON_LIKELY(std::size_t(data.size() - data_offset) > 1 || end_of_data)) {
        std::mbstate_t state_2 = {};
        core::Span data_2 = data.subspan(1, 1);
        std::size_t data_offset_2 = 0;
        bool end_of_data = false;
        std::array<char_type, 1> buffer_2;
        std::size_t buffer_offset_2 = 0;
        bool error = false;
        decode(state_2, data_2, data_offset_2, end_of_data, buffer_2, buffer_offset_2, error); // Throws
        if (ARCHON_LIKELY(error)) {
            state = {};
            data_offset += 1;
            goto again;
        }
        if (ARCHON_LIKELY(buffer_offset < buffer.size())) {
            state = {};
            data_offset += 1;
            buffer[buffer_offset] = m_replacement_info.char_;
            buffer_offset += 1;
            return true;
        }
        need_more_data = false;
        return false;
    }
    need_more_data = true;
    return false;
}


template<class C, class T>
bool CharCodec4<C, T>::encode_replacement(std::mbstate_t& state, std::size_t& data_offset,
                                          core::Span<char> buffer, std::size_t& buffer_offset) const
{
    if (ARCHON_LIKELY(this->m_is_stateless)) {
        std::size_t n = m_replacement_info.encoded_char.size();
        if (ARCHON_LIKELY(n <= std::size_t(buffer.size() - buffer_offset))) {
            std::copy_n(m_replacement_info.encoded_char.data(), n, buffer.data() + buffer_offset);
            data_offset += 1;
            buffer_offset += n;
            return true;
        }
        return false;
    }
    std::mbstate_t state_2 = state;
    std::basic_string_view<char_type> data_2 { &m_replacement_info.char_, 1 };
    std::size_t data_offset_2 = 0;
    std::size_t buffer_offset_2 = buffer_offset;
    bool error = false;
    bool complete = encode(state_2, data_2, data_offset_2, buffer, buffer_offset_2, error); // Throws
    if (ARCHON_LIKELY(complete)) {
        state = state_2;
        data_offset += 1;
        buffer_offset = buffer_offset_2;
        return true;
    }
    ARCHON_ASSERT(!error);
    return false;
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_CHAR_CODEC_HPP
