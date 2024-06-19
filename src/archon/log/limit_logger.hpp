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

#ifndef ARCHON_X_LOG_X_LIMIT_LOGGER_HPP
#define ARCHON_X_LOG_X_LIMIT_LOGGER_HPP

/// \file


#include <cstddef>
#include <utility>
#include <atomic>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/string_matcher.hpp>
#include <archon/log/limit.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/sink.hpp>
#include <archon/log/channel.hpp>
#include <archon/log/channel_map.hpp>
#include <archon/log/logger.hpp>


namespace archon::log {


template<class C, class T = std::char_traits<C>> class BasicLimitLogger final
    : private log::BasicChannelMap<C, T>
    , public log::BasicLogger<C, T> {
public:
    using prefix_type      = log::BasicPrefix<C, T>;
    using sink_type        = log::BasicSink<C, T>;
    using channel_type     = log::BasicChannel<C, T>;
    using channel_map_type = log::BasicChannelMap<C, T>;
    using logger_type      = log::BasicLogger<C, T>;

    explicit BasicLimitLogger(logger_type& base_logger, std::string_view channel_pattern = "*");
    BasicLimitLogger(logger_type& base_logger, log::LogLevel fixed_limit, std::string_view channel_pattern = "*");

    void set_log_level_limit(log::LogLevel) noexcept;

private:
    class LimitImpl;
    class SinkImpl;
    class Interm;

    LimitImpl m_limit;
    core::Slab<SinkImpl> m_sinks;
    core::Slab<channel_type> m_channels;

    BasicLimitLogger(Interm&&, int fixed_limit) noexcept;

    // Overriding functions from log::BasicChannelMap<C, T>
    auto do_get_channels() const noexcept -> core::Span<const channel_type> override;
};


using LimitLogger     = BasicLimitLogger<char>;
using WideLimitLogger = BasicLimitLogger<wchar_t>;








// Implementation


template<class C, class T>
class BasicLimitLogger<C, T>::LimitImpl final
    : public log::Limit {
public:
    LimitImpl(int fixed_limit) noexcept
        : m_fixed_limit(fixed_limit)
        , m_variable_limit(fixed_limit == std::numeric_limits<int>::max() ? log::LogLevel::info :
                           log::LogLevel(fixed_limit))
    {
    }

    void set(LogLevel limit) noexcept
    {
        ARCHON_ASSERT(m_fixed_limit != std::numeric_limits<int>::max());
        m_variable_limit.store(limit, std::memory_order_relaxed);
    }

    // Overriding function from log::Limit
    int get_fixed_limit() const noexcept override
    {
        return m_fixed_limit;
    }

    // Overriding function from log::Limit
    auto get_level_limit() const noexcept -> log::LogLevel override
    {
        return m_variable_limit.load(std::memory_order_relaxed);
    }

private:
    const int m_fixed_limit;
    std::atomic<log::LogLevel> m_variable_limit;
};


template<class C, class T>
class BasicLimitLogger<C, T>::SinkImpl final
    : public log::BasicSink<C, T> {
public:
    using string_view_type = typename logger_type::string_view_type;

    SinkImpl(const LimitImpl& limit, sink_type& sink) noexcept
        : sink_type(sink.get_locale())
        , m_limit(limit)
        , m_sink(sink)
    {
    }

    // Overriding function from log::BasicSink<C, T>
    void sink_log(log::LogLevel level, const prefix_type& channel_prefix, const prefix_type& message_prefix,
                  string_view_type message) override
    {
        log::LogLevel limit = m_limit.get_level_limit();
        if (level <= limit)
            m_sink.sink_log(level, channel_prefix, message_prefix, message); // Throws
    }

private:
    const LimitImpl& m_limit;
    sink_type& m_sink;
};


template<class C, class T>
class BasicLimitLogger<C, T>::Interm {
public:
    const prefix_type& prefix;
    core::Slab<SinkImpl> sinks;
    core::Slab<channel_type> channels;
    channel_type* channel = nullptr;

    Interm(logger_type& base_logger, std::string_view channel_pattern, const LimitImpl& limit)
        : prefix(base_logger.get_prefix())
    {
        core::StringMatcher matcher(core::StringMatcher::PatternType::wildcard, channel_pattern,
                                    base_logger.get_locale()); // Throws
        auto base_channels = base_logger.get_channel_map().get_channels();
        std::size_t num_sinks = 0;
        for (const channel_type& channel : base_channels) {
            std::string_view channel_name = channel.get_name();
            bool is_match = matcher.match(channel_name);
            if (is_match)
                ++num_sinks;
        }
        if (ARCHON_UNLIKELY(num_sinks == 0))
            throw std::invalid_argument("No such channel");
        std::size_t num_channels = base_channels.size();
        sinks.recreate(num_sinks); // Throws
        channels.recreate(num_channels); // Throws
        for (channel_type& channel : base_channels) {
            std::string_view channel_name = channel.get_name();
            const log::Limit* limit_2 = &channel.get_limit();
            const prefix_type& prefix = channel.get_prefix();
            sink_type* sink = &channel.get_sink();
            bool is_match = matcher.match(channel_name);
            if (is_match) {
                limit_2 = &limit;
                sink = &sinks.add(limit, *sink);
            }
            channels.add(channel_name, *limit_2, prefix, *sink);
        }
        ARCHON_ASSERT(sinks.size()    == num_sinks);
        ARCHON_ASSERT(channels.size() == num_channels);
        const channel_type* channel_2 = channel_map_type::do_find_channel({ channels.data(), channels.size() },
                                                                          base_logger.get_channel().get_name());
        ARCHON_ASSERT(channel_2);
        channel = const_cast<channel_type*>(channel_2);
    }
};


template<class C, class T>
inline BasicLimitLogger<C, T>::BasicLimitLogger(logger_type& base_logger, std::string_view channel_pattern)
    : BasicLimitLogger(Interm(base_logger, channel_pattern, m_limit), std::numeric_limits<int>::max()) // Throws
{
}


template<class C, class T>
inline BasicLimitLogger<C, T>::BasicLimitLogger(logger_type& base_logger, log::LogLevel fixed_limit,
                                                std::string_view channel_pattern)
    : BasicLimitLogger(Interm(base_logger, channel_pattern, m_limit), int(fixed_limit)) // Throws
{
}


template<class C, class T>
inline void BasicLimitLogger<C, T>::set_log_level_limit(log::LogLevel limit) noexcept
{
    m_limit.set(limit);
}


template<class C, class T>
inline BasicLimitLogger<C, T>::BasicLimitLogger(Interm&& i, int fixed_limit) noexcept
    : logger_type(fixed_limit, m_limit, i.prefix, *i.channel, *this)
    , m_limit(fixed_limit)
    , m_sinks(std::move(i.sinks))
    , m_channels(std::move(i.channels))
{
}


template<class C, class T>
auto BasicLimitLogger<C, T>::do_get_channels() const noexcept -> core::Span<const channel_type>
{
    return { m_channels.data(), m_channels.size() };
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_LIMIT_LOGGER_HPP
