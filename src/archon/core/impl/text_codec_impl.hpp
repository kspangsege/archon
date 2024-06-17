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

#ifndef ARCHON_X_CORE_X_IMPL_X_TEXT_CODEC_IMPL_HPP
#define ARCHON_X_CORE_X_IMPL_X_TEXT_CODEC_IMPL_HPP


#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <utility>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/newline_codec.hpp>


namespace archon::core::impl {


// Variant: Degenerate character codec
template<class P, class D, class I> class TextCodecImpl1
    : private P {
public:
    using prim_type       = P;
    using char_codec_type = D;
    using impl_type       = I;

    using char_type    = typename char_codec_type::char_type;
    using traits_type  = typename char_codec_type::traits_type;
    using decoder_type = typename prim_type::decoder_type;
    using encoder_type = typename prim_type::encoder_type;

    using Config = typename char_codec_type::Config;

    static_assert(std::is_same_v<char_type, char>);

    static constexpr bool is_degen = prim_type::is_degen;

    explicit TextCodecImpl1(const std::locale*, Config) noexcept;

    template<class J> static auto up_cast(const I&) noexcept -> const J&;
};



// Variant: Non-degenerate character codec
template<class P, class D, class I> class TextCodecImpl2
    : private P {
public:
    using prim_type       = P;
    using char_codec_type = D;
    using impl_type       = I;

    class Decoder;
    class Encoder;
    template<class E> class CompoundEncoder;

    using char_type    = typename char_codec_type::char_type;
    using traits_type  = typename char_codec_type::traits_type;
    using decoder_type = typename prim_type::template build_decoder_type<Decoder>;
    using encoder_type = typename prim_type::template build_encoder_type<Encoder, CompoundEncoder>;

    using Config = typename char_codec_type::Config;

    static constexpr bool is_degen = false;

    explicit TextCodecImpl2(const std::locale*, Config);

    template<class J> static auto up_cast(const I&) noexcept -> const J&;
    template<class J> static auto down_cast(const J&) noexcept -> const I&;

private:
    char_codec_type m_char_codec;
};



template<class P, class D, class I, bool> class TextCodecImpl3
    : public TextCodecImpl1<P, D, I> {
public:
    using TextCodecImpl1<P, D, I>::TextCodecImpl1;
};

template<class P, class D, class I> class TextCodecImpl3<P, D, I, false>
    : public TextCodecImpl2<P, D, I> {
public:
    using TextCodecImpl2<P, D, I>::TextCodecImpl2;
};



template<class P, class D> class TextCodecImpl
    : public TextCodecImpl3<P, D, TextCodecImpl<P, D>, D::is_degen> {
public:
    using TextCodecImpl3<P, D, TextCodecImpl, D::is_degen>::TextCodecImpl3;
};



class PrimPosixTextCodecImpl {
public:
    class Copier;
    class Decoder;
    class Encoder;

    static constexpr bool is_degen = true;

    using decoder_type = Decoder;
    using encoder_type = Encoder;

    template<class E> using build_decoder_type = E;
    template<class E, template<class> class> using build_encoder_type = E;

    template<class I> static auto up_cast(const PrimPosixTextCodecImpl&) noexcept -> const I&;
};



class PrimWindowsTextCodecImpl {
public:
    class Decoder;
    class Encoder;
    template<class E> class CompoundDecoder;

    static constexpr bool is_degen = false;

    using decoder_type = Decoder;
    using encoder_type = Encoder;

    template<class E> using build_decoder_type = CompoundDecoder<E>;
    template<class, template<class> class E> using build_encoder_type = E<Encoder>;

    template<class I> static auto up_cast(const PrimWindowsTextCodecImpl&) noexcept -> const I&;
    template<class I> static auto down_cast(const I&) noexcept -> const PrimWindowsTextCodecImpl&;
};



#if ARCHON_WINDOWS || ARCHON_CYGWIN
using PrimTextCodecImpl = PrimWindowsTextCodecImpl;
#else
using PrimTextCodecImpl = PrimPosixTextCodecImpl;
#endif








// Implementation


// ============================ TextCodecImpl1 ============================


template<class P, class D, class I>
inline TextCodecImpl1<P, D, I>::TextCodecImpl1(const std::locale*, Config) noexcept
{
}


template<class P, class D, class I>
template<class J> inline auto TextCodecImpl1<P, D, I>::up_cast(const I& impl) noexcept -> const J&
{
    if constexpr (std::is_same_v<J, I>) {
        return impl;
    }
    else {
        return P::template up_cast<J>(impl);
    }
}



// ============================ TextCodecImpl2 ============================


template<class P, class D, class I>
class TextCodecImpl2<P, D, I>::Decoder {
public:
    using impl_type = I;

    Decoder(const impl_type& impl, core::Buffer<char_type>& buffer, std::size_t& buffer_offset) noexcept
        : m_char_codec(impl.m_char_codec)
        , m_buffer(buffer)
        , m_buffer_offset(buffer_offset)
    {
    }

    bool decode(core::Span<const char> data, std::size_t& data_offset, bool end_of_data)
    {
        bool error = false;
        for (;;) {
            bool complete = m_char_codec.decode(m_state, data, data_offset, end_of_data, m_buffer, m_buffer_offset,
                                                error); // Throws
            if (ARCHON_LIKELY(complete))
                return true;
            if (ARCHON_LIKELY(!error)) {
                m_buffer.expand(m_buffer_offset); // Throws
                continue;
            }
            return false;
        }
    }

private:
    const char_codec_type& m_char_codec;
    typename traits_type::state_type m_state = {};
    core::Buffer<char_type>& m_buffer;
    std::size_t& m_buffer_offset;
};


template<class P, class D, class I>
class TextCodecImpl2<P, D, I>::Encoder {
public:
    using impl_type = I;

    Encoder(const impl_type& impl, core::Buffer<char>& buffer, std::size_t& buffer_offset) noexcept
        : m_char_codec(impl.m_char_codec)
        , m_buffer(buffer)
        , m_buffer_offset(buffer_offset)
    {
    }

    bool encode(core::Span<const char_type> data, std::size_t& data_offset)
    {
        bool error = false;
        for (;;) {
            bool complete = m_char_codec.encode(m_state, data, data_offset, m_buffer, m_buffer_offset,
                                                error); // Throws
            if (ARCHON_LIKELY(complete))
                return true;
            if (ARCHON_LIKELY(!error)) {
                m_buffer.expand(m_buffer_offset); // Throws
                continue;
            }
            return false;
        }
    }

    bool unshift()
    {
        for (;;) {
            bool complete = m_char_codec.unshift(m_state, m_buffer, m_buffer_offset); // Throws
            if (ARCHON_LIKELY(complete))
                return true;
            m_buffer.expand(m_buffer_offset); // Throws
        }
    }

private:
    const char_codec_type& m_char_codec;
    typename traits_type::state_type m_state = {};
    core::Buffer<char>& m_buffer;
    std::size_t& m_buffer_offset;
};


template<class P, class D, class I>
template<class E> class TextCodecImpl2<P, D, I>::CompoundEncoder
    : private E {
public:
    using impl_type = I;

    CompoundEncoder(const impl_type& impl, core::Buffer<char>& buffer, std::size_t& buffer_offset)
        : E(impl_type::template up_cast<typename E::impl_type>(impl)
        ,  buffer, buffer_offset) // Throws
        , m_char_codec(impl.m_char_codec)
    {
    }

    bool encode(core::Span<const char_type> data, std::size_t& data_offset)
    {
        for (;;) {
            std::size_t buffer_offset = 0;
            bool error = false;
            bool complete = m_char_codec.encode(m_state, data, data_offset, m_buffer, buffer_offset, error); // Throws
            core::Span<const char> data_2(m_buffer.data(), buffer_offset);
            std::size_t data_offset_2 = 0;
            bool success = E::encode(data_2, data_offset_2); // Throws
            if (ARCHON_LIKELY(success)) {
                ARCHON_ASSERT(data_offset_2 == data_2.size());
                if (ARCHON_LIKELY(complete))
                    return true;
                if (ARCHON_LIKELY(!error)) {
                    if (buffer_offset == 0) {
                        std::size_t used_size = 0;
                        m_buffer.expand(used_size); // Throws
                    }
                    continue;
                }
            }
            return false;
        }
    }

    bool unshift()
    {
        for (;;) {
            std::size_t buffer_offset = 0;
            bool complete = m_char_codec.unshift(m_state, m_buffer, buffer_offset); // Throws
            core::Span<const char> data_2(m_buffer.data(), buffer_offset);
            std::size_t data_offset_2 = 0;
            bool success = E::encode(data_2, data_offset_2); // Throws
            if (ARCHON_LIKELY(success)) {
                ARCHON_ASSERT(data_offset_2 == data_2.size());
                if (ARCHON_LIKELY(complete))
                    return true;
                if (buffer_offset == 0) {
                    std::size_t used_size = 0;
                    m_buffer.expand(used_size); // Throws
                }
                continue;
            }
            return false;
        }
    }

private:
    const char_codec_type& m_char_codec;
    typename traits_type::state_type m_state = {};
    core::ArraySeededBuffer<char, 512> m_buffer;
    std::size_t m_buffer_offset = 0;
};


template<class P, class D, class I>
inline TextCodecImpl2<P, D, I>::TextCodecImpl2(const std::locale* locale, Config config)
    : m_char_codec(locale, std::move(config)) // Throws
{
}


template<class P, class D, class I>
template<class J> inline auto TextCodecImpl2<P, D, I>::up_cast(const I& impl) noexcept -> const J&
{
    if constexpr (std::is_same_v<J, I>) {
        return impl;
    }
    else {
        return P::template up_cast<J>(impl);
    }
}


template<class P, class D, class I>
template<class J> inline auto TextCodecImpl2<P, D, I>::down_cast(const J& impl) noexcept -> const I&
{
    if constexpr (std::is_same_v<J, I>) {
        return impl;
    }
    else {
        return static_cast<const I&>(P::down_cast(impl));
    }
}



// ============================ PrimPosixTextCodecImpl ============================


class PrimPosixTextCodecImpl::Copier {
public:
    void copy(core::Span<const char> data, std::size_t& data_offset)
    {
        ARCHON_ASSERT(data_offset <= data.size());
        std::size_t n = std::size_t(data.size() - data_offset);
        m_buffer.reserve_extra(n, m_buffer_offset); // Throws
        std::copy_n(data.data() + data_offset, n, m_buffer.data() + m_buffer_offset);
        m_buffer_offset += n;
        data_offset += n;
    }

protected:
    Copier(core::Buffer<char>& buffer, std::size_t& buffer_offset) noexcept
        : m_buffer(buffer)
        , m_buffer_offset(buffer_offset)
    {
    }

    ~Copier() noexcept = default;

private:
    core::Buffer<char>& m_buffer;
    std::size_t& m_buffer_offset;
};


class PrimPosixTextCodecImpl::Decoder
    : private Copier {
public:
    using impl_type = PrimPosixTextCodecImpl;

    Decoder(const PrimPosixTextCodecImpl&, core::Buffer<char>& buffer, std::size_t& buffer_offset) noexcept
        : Copier(buffer, buffer_offset)
    {
    }

    bool decode(core::Span<const char> data, std::size_t& data_offset, bool)
    {
        copy(data, data_offset); // Throws
        return true;
    }
};


class PrimPosixTextCodecImpl::Encoder
    : private Copier {
public:
    using impl_type = PrimPosixTextCodecImpl;

    Encoder(const PrimPosixTextCodecImpl&, core::Buffer<char>& buffer, std::size_t& buffer_offset) noexcept
        : Copier(buffer, buffer_offset)
    {
    }

    bool encode(core::Span<const char> data, std::size_t& data_offset)
    {
        copy(data, data_offset); // Throws
        return true;
    }

    bool unshift() noexcept
    {
        return true; // No-op
    }
};


template<class I>
inline auto PrimPosixTextCodecImpl::up_cast(const PrimPosixTextCodecImpl& impl) noexcept -> const I&
{
    return impl;
}



// ============================ PrimWindowsTextCodecImpl ============================


class PrimWindowsTextCodecImpl::Decoder {
public:
    using impl_type = PrimWindowsTextCodecImpl;

    Decoder(const PrimWindowsTextCodecImpl&, core::Buffer<char>& buffer, std::size_t& buffer_offset) noexcept
        : m_buffer(buffer)
        , m_buffer_offset(buffer_offset)
    {
    }

    bool decode(core::Span<const char> data, std::size_t& data_offset, bool end_of_data)
    {
        std::size_t clear_offset = 0; // Dummy
        std::size_t clear; // Dummy
        for (;;) {
            core::newline_codec::decode(data, data_offset, end_of_data, m_buffer, m_buffer_offset,
                                        clear_offset, clear);
            if (ARCHON_LIKELY(data_offset == data.size() || m_buffer_offset < m_buffer.size()))
                return true;
            m_buffer.expand(m_buffer_offset); // Throws
        }
    }

private:
    core::Buffer<char>& m_buffer;
    std::size_t& m_buffer_offset;
};


class PrimWindowsTextCodecImpl::Encoder {
public:
    using impl_type = PrimWindowsTextCodecImpl;

    Encoder(const PrimWindowsTextCodecImpl&, core::Buffer<char>& buffer, std::size_t& buffer_offset) noexcept
        : m_buffer(buffer)
        , m_buffer_offset(buffer_offset)
    {
    }

    bool encode(core::Span<const char> data, std::size_t& data_offset)
    {
        for (;;) {
            core::newline_codec::encode(data, data_offset, m_buffer, m_buffer_offset);
            if (ARCHON_LIKELY(data_offset == data.size()))
                return true;
            m_buffer.expand(m_buffer_offset); // Throws
        }
    }

    bool unshift() noexcept
    {
        return true; // No-op
    }

private:
    core::Buffer<char>& m_buffer;
    std::size_t& m_buffer_offset;
};


template<class E> class PrimWindowsTextCodecImpl::CompoundDecoder
    : private E {
public:
    using impl_type = PrimWindowsTextCodecImpl;

    template<class C>
    CompoundDecoder(const PrimWindowsTextCodecImpl& impl, core::Buffer<C>& buffer, std::size_t& buffer_offset)
        : E(E::impl_type::down_cast(impl), buffer, buffer_offset) // Throws
    {
    }

    bool decode(core::Span<const char> data, std::size_t& data_offset, bool end_of_data)
    {
        std::size_t clear; // Dummy
        for (;;) {
            std::size_t clear_offset = 0; // Dummy
            core::newline_codec::decode(data, data_offset, end_of_data, m_buffer, m_buffer_offset,
                                        clear_offset, clear);
            core::Span<const char> data_2(m_buffer.data(), m_buffer_offset);
            std::size_t data_offset_2 = 0;
            bool end_of_data_2 = (end_of_data && data_offset == data.size());
            bool success = E::decode(data_2, data_offset_2, end_of_data_2); // Throws
            if (ARCHON_LIKELY(success)) {
                // Move any remaining data to start of buffer
                char* base = m_buffer.data();
                char* i = std::copy(base + data_offset_2, base + m_buffer_offset, base);
                bool done = (data_offset == data.size() || m_buffer_offset < m_buffer.size());
                m_buffer_offset = std::size_t(i - base);
                if (ARCHON_LIKELY(done))
                    return true;
                m_buffer.reserve_extra(1, m_buffer_offset); // Throws
                continue;
            }
            return false;
        }
    }

private:
    core::ArraySeededBuffer<char, 512> m_buffer;
    std::size_t m_buffer_offset = 0;
};


template<class I>
inline auto PrimWindowsTextCodecImpl::up_cast(const PrimWindowsTextCodecImpl& impl) noexcept -> const I&
{
    return impl;
}


template<class I> inline auto PrimWindowsTextCodecImpl::down_cast(const I& impl) noexcept ->
    const PrimWindowsTextCodecImpl&
{
    return impl;
}


} // namespace archon::core::impl

#endif // ARCHON_X_CORE_X_IMPL_X_TEXT_CODEC_IMPL_HPP
