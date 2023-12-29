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

#ifndef ARCHON_X_LOG_X_IMPL_X_ENCODING_LOGGER_IMPL_HPP
#define ARCHON_X_LOG_X_IMPL_X_ENCODING_LOGGER_IMPL_HPP


#include <type_traits>
#include <array>
#include <string_view>

#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_codec.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/format_encoded.hpp>
#include <archon/log/limit.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/sink.hpp>
#include <archon/log/channel.hpp>
#include <archon/log/channel_map.hpp>
#include <archon/log/logger.hpp>


namespace archon::log::impl {


// Degenerate case
class EncodingLoggerImpl1 {
public:
    EncodingLoggerImpl1(log::Logger& base_logger) noexcept;

    auto get_prefix(log::Logger& base_logger) noexcept -> const log::Prefix&;
    auto get_channel(log::Logger& base_logger) noexcept -> log::Channel&;
    auto get_channel_map(log::Logger& base_logger) noexcept -> log::ChannelMap&;
};



// Non-degenerate case
template<class C, class T> class EncodingLoggerImpl2 {
public:
    using prefix_type      = log::BasicPrefix<C, T>;
    using sink_type        = log::BasicSink<C, T>;
    using channel_type     = log::BasicChannel<C, T>;
    using channel_map_type = log::BasicChannelMap<C, T>;

    EncodingLoggerImpl2(log::Logger& base_logger);

    auto get_prefix(log::Logger& base_logger) noexcept -> const prefix_type&;
    auto get_channel(log::Logger& base_logger) noexcept -> channel_type&;
    auto get_channel_map(log::Logger& base_logger) noexcept -> channel_map_type&;

private:
    using string_view_type  = std::basic_string_view<C, T>;
    using string_codec_type = core::BasicStringCodec<C, T>;

    class BridgingPrefix;

    class ChannelMapImpl
        : public log::BasicChannelMap<C, T> {
    public:
        channel_type channel;

        ChannelMapImpl(log::Channel& base_channel, sink_type&) noexcept;

    protected:
        // Overriding functions from log::BasicChannelMap<C, T>
        auto do_get_channels() const noexcept -> core::Span<const channel_type> override final;
    };

    class SinkImpl
        : public log::BasicSink<C, T> {
    public:
        SinkImpl(log::Sink& base_sink, const log::Prefix& base_channel_prefix, const log::Prefix& base_message_prefix);

        // Overriding functions from log::BasicSink<C, T>
        void sink_log(log::LogLevel, const prefix_type&, const prefix_type&, string_view_type) override final;

    private:
        log::Sink& m_base_sink;
        const log::Prefix& m_base_channel_prefix;
        const log::Prefix& m_base_message_prefix;
        string_codec_type m_string_codec;
    };

    ChannelMapImpl m_channel_map;
    SinkImpl m_sink;

    EncodingLoggerImpl2(const log::Prefix& base_prefix, log::Channel& base_channel);
};



template<class C, class T> using EncodingLoggerImpl =
    std::conditional_t<core::BasicCharCodec<C, T>::is_degen && std::is_same_v<T, std::char_traits<C>>,
                       EncodingLoggerImpl1, EncodingLoggerImpl2<C, T>>;








// Implementation


// ============================ EncodingLoggerImpl1 ============================


inline EncodingLoggerImpl1::EncodingLoggerImpl1(log::Logger&) noexcept
{
}


inline auto EncodingLoggerImpl1::get_prefix(log::Logger& base_logger) noexcept -> const log::Prefix&
{
    return base_logger.get_prefix();
}


inline auto EncodingLoggerImpl1::get_channel(log::Logger& base_logger) noexcept -> log::Channel&
{
    return base_logger.get_channel();
}


inline auto EncodingLoggerImpl1::get_channel_map(log::Logger& base_logger) noexcept -> log::ChannelMap&
{
    return base_logger.get_channel_map();
}



// ============================ EncodingLoggerImpl2 ============================


template<class C, class T>
inline EncodingLoggerImpl2<C, T>::EncodingLoggerImpl2(log::Logger& base_logger)
    : EncodingLoggerImpl2(base_logger.get_prefix(), base_logger.get_channel()) // Throws
{
}


template<class C, class T>
inline auto EncodingLoggerImpl2<C, T>::get_prefix(log::Logger&) noexcept -> const prefix_type&
{
    return m_channel_map;
}


template<class C, class T>
inline auto EncodingLoggerImpl2<C, T>::get_channel(log::Logger&) noexcept -> channel_type&
{
    return m_channel_map.channel;
}


template<class C, class T>
inline auto EncodingLoggerImpl2<C, T>::get_channel_map(log::Logger&) noexcept -> channel_map_type&
{
    return m_channel_map;
}


template<class C, class T>
class EncodingLoggerImpl2<C, T>::BridgingPrefix
    : public log::Prefix {
public:
    BridgingPrefix(const log::Prefix& prefix_1, const prefix_type& prefix_2) noexcept
        : m_prefix_1(prefix_1)
        , m_prefix_2(prefix_2)
    {
    }

    // Overriding function from log::Prefix
    void format_prefix(std::ostream& out) const override final
    {
        // FIXME: A better implementation of this function would use an encoding output
        // stream, with direct incremental forwarding to a sub-stream. It would be better
        // because it would eliminate the need for dynamic allocations entirely.                                                 
        std::array<C, 256> seed_memory;
        core::BasicSeedMemoryOutputStream out_2(seed_memory); // Throws
        out_2.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
        out_2.imbue(out.getloc()); // Throws
        m_prefix_2.format_prefix(out_2); // Throws
        m_prefix_1.format_prefix(out); // Throws
        out << core::encoded(out_2.view()); // Throws
    }

private:
    const log::Prefix& m_prefix_1;
    const prefix_type& m_prefix_2;
};


template<class C, class T>
inline EncodingLoggerImpl2<C, T>::ChannelMapImpl::ChannelMapImpl(log::Channel& base_channel, sink_type& sink) noexcept
    : channel(base_channel.get_name(), base_channel.get_limit(), *this, sink)
{
}


template<class C, class T>
inline auto EncodingLoggerImpl2<C, T>::ChannelMapImpl::do_get_channels() const noexcept ->
    core::Span<const channel_type>
{
    return { &channel, 1 };
}


template<class C, class T>
inline EncodingLoggerImpl2<C, T>::SinkImpl::SinkImpl(log::Sink& base_sink, const log::Prefix& base_channel_prefix,
                                                     const log::Prefix& base_message_prefix)
    : sink_type(base_sink.get_locale())
    , m_base_sink(base_sink)
    , m_base_channel_prefix(base_channel_prefix)
    , m_base_message_prefix(base_message_prefix)
    , m_string_codec(base_sink.get_locale()) // Throws
{
}


template<class C, class T>
inline void EncodingLoggerImpl2<C, T>::SinkImpl::sink_log(log::LogLevel log_level, const prefix_type& channel_prefix,
                                                          const prefix_type& message_prefix, string_view_type message)
{
    core::ArraySeededBuffer<char, channel_type::format_seed_memory_size> buffer;
    BridgingPrefix channel_prefix_2(m_base_channel_prefix, channel_prefix);
    BridgingPrefix message_prefix_2(m_base_message_prefix, message_prefix);
    std::string_view message_2 = m_string_codec.encode(message, buffer); // Throws
    m_base_sink.sink_log(log_level, channel_prefix_2, message_prefix_2, message_2); // Throws
}


template<class C, class T>
inline EncodingLoggerImpl2<C, T>::EncodingLoggerImpl2(const log::Prefix& base_prefix, log::Channel& base_channel)
    : m_channel_map(base_channel, m_sink)
    , m_sink(base_channel.get_sink(), base_channel.get_prefix(), base_prefix) // Throws
{
}


} // namespace archon::log::impl

#endif // ARCHON_X_LOG_X_IMPL_X_ENCODING_LOGGER_IMPL_HPP
