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

#ifndef ARCHON_X_LOG_X_DUPLICATING_LOGGER_HPP
#define ARCHON_X_LOG_X_DUPLICATING_LOGGER_HPP

/// \file


#include <cstddef>
#include <algorithm>
#include <utility>
#include <array>
#include <string_view>
#include <map>

#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/memory.hpp>
#include <archon/log/limit.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/sink.hpp>
#include <archon/log/channel.hpp>
#include <archon/log/channel_map.hpp>
#include <archon/log/logger.hpp>


namespace archon::log {


template<class C, class T = std::char_traits<C>> class BasicDuplicatingLogger final
    : private log::BasicChannelMap<C, T>
    , public log::BasicLogger<C, T> {
public:
    class Rule;

    using prefix_type      = log::BasicPrefix<C, T>;
    using sink_type        = log::BasicSink<C, T>;
    using channel_type     = log::BasicChannel<C, T>;
    using channel_map_type = log::BasicChannelMap<C, T>;
    using logger_type      = log::BasicLogger<C, T>;

    /// \brief Construct logger that duplicates messages on selected channel.
    ///
    /// Copy all the channels, as well as the channel selection, from the specified base
    /// logger (\p base_logger), then arrange for the messages on the selected channel to be
    /// duplicated onto the selected channel in the specified target logger (\p
    /// target_logger).
    ///
    /// This constructor is a shorthand for `BasicDuplicatingLogger(base_logger,
    /// base_logger.get_channel().get_name(), target_logger)`.
    ///
    BasicDuplicatingLogger(logger_type& base_logger, logger_type& target_logger);

    /// \brief Construct logger that duplicates messages on one channel.
    ///
    /// Copy all the channels, as well as the channel selection, from the specified base
    /// logger (\p base_logger), then arrange for the messages on the specified channel (\p
    /// origin_channel) to be duplicated onto the selected channel in the specified target
    /// logger (\p target_logger).
    ///
    /// This constructor is a shorthand for `BasicDuplicatingLogger(base_logger, rules)`
    /// where `rules` contains one rule constructed as `Rule(origin_channel,
    /// target_logger)`.
    ///
    BasicDuplicatingLogger(logger_type& base_logger, std::string_view origin_channel, logger_type& target_logger);

    BasicDuplicatingLogger(logger_type& base_logger, core::Span<const Rule>);

private:
    class SinkImpl;
    class Interm;

    using null_prefix_type     = log::BasicNullPrefix<C, T>;
    using compound_prefix_type = log::BasicCompoundPrefix<C, T>;

    core::Slab<compound_prefix_type> m_prefixes;
    core::Slab<SinkImpl> m_sinks;
    core::Slab<channel_type> m_channels;

    BasicDuplicatingLogger(Interm&&) noexcept;

    // Overriding functions from log::BasicChannelMap<C, T>
    auto do_get_channels() const noexcept -> core::Span<const channel_type> override;
};


using DuplicatingLogger     = BasicDuplicatingLogger<char>;
using WideDuplicatingLogger = BasicDuplicatingLogger<wchar_t>;




template<class C, class T> class BasicDuplicatingLogger<C, T>::Rule {
public:
    Rule(std::string_view origin_channel, logger_type& target_logger) noexcept;
    Rule(std::string_view origin_channel, logger_type& target_logger, std::string_view target_channel);

private:
    std::string_view m_origin_channel;
    logger_type& m_target_logger;
    channel_type& m_target_channel;

    friend class BasicDuplicatingLogger<C, T>;
};








// Implementation


template<class C, class T>
class BasicDuplicatingLogger<C, T>::SinkImpl final
    : public log::BasicSink<C, T>
    , public log::Limit {
public:
    using string_view_type = typename logger_type::string_view_type;

    SinkImpl(const std::locale& locale, const prefix_type& prefix_1, const prefix_type& prefix_2,
             channel_type& channel_1, channel_type& channel_2) noexcept
        : sink_type(locale)
        , m_limit_1(channel_1.get_limit())
        , m_limit_2(channel_2.get_limit())
        , m_prefix_1(prefix_1)
        , m_prefix_2(prefix_2)
        , m_sink_1(channel_1.get_sink())
        , m_sink_2(channel_2.get_sink())
    {
    }

    // Overriding function from log::BasicSink<C, T>
    void sink_log(log::LogLevel level, const prefix_type& channel_prefix, const prefix_type& message_prefix,
                  string_view_type message) override
    {
        compound_prefix_type channel_prefix_1(m_prefix_1, channel_prefix);
        m_sink_1.sink_log(level, channel_prefix_1, message_prefix, message); // Throws
        compound_prefix_type channel_prefix_2(m_prefix_2, channel_prefix);
        m_sink_2.sink_log(level, channel_prefix_2, message_prefix, message); // Throws
    }

    // Overriding function from log::Limit
    int get_fixed_limit() const noexcept override
    {
        int a = m_limit_1.get_fixed_limit();
        int b = m_limit_2.get_fixed_limit();
        return std::max(a, b);
    }

    // Overriding function from log::Limit
    auto get_level_limit() const noexcept -> log::LogLevel override
    {
        log::LogLevel a = m_limit_1.get_level_limit();
        log::LogLevel b = m_limit_2.get_level_limit();
        return LogLevel(std::max(int(a), int(b)));
    }

private:
    const log::Limit& m_limit_1;
    const log::Limit& m_limit_2;
    const prefix_type& m_prefix_1;
    const prefix_type& m_prefix_2;
    sink_type& m_sink_1;
    sink_type& m_sink_2;
};


template<class C, class T>
class BasicDuplicatingLogger<C, T>::Interm {
public:
    core::Slab<compound_prefix_type> prefixes;
    core::Slab<SinkImpl> sinks;
    core::Slab<channel_type> channels;
    channel_type* channel = nullptr;

    Interm(logger_type& base_logger, core::Span<const Rule> rules, const null_prefix_type& null_prefix)
    {
        channel_map_type& base_map = base_logger.get_channel_map();
        std::map<const void*, std::pair<logger_type*, channel_type*>> map;
        for (const Rule& rule : rules) {
            const channel_type& origin_channel = base_map.find_channel(rule.m_origin_channel); // Throws
            map[&origin_channel] = { &rule.m_target_logger, &rule.m_target_channel }; // Throws
        }
        std::size_t num_prefixes = 0;
        for (const auto& entry : map) {
            logger_type& target_logger = *entry.second.first;
            channel_type& target_channel = *entry.second.second;
            bool logger_has_prefix  = !target_logger.get_prefix().is_null_prefix();
            bool channel_has_prefix = !target_channel.get_prefix().is_null_prefix();
            if (channel_has_prefix && logger_has_prefix)
                ++num_prefixes;
        }
        bool base_logger_has_prefix = !base_logger.get_prefix().is_null_prefix();
        auto base_channels = base_map.get_channels();
        for (const channel_type& channel : base_channels) {
            bool channel_has_prefix = !channel.get_prefix().is_null_prefix();
            if (channel_has_prefix && base_logger_has_prefix)
                ++num_prefixes;
        }
        std::size_t num_sinks = map.size();
        std::size_t num_channels = base_channels.size();
        prefixes.recreate(num_prefixes); // Throws
        sinks.recreate(num_sinks); // Throws
        channels.recreate(num_channels); // Throws
        for (channel_type& base_channel : base_channels) {
            const prefix_type& base_channel_prefix = base_channel.get_prefix();
            const prefix_type& base_logger_prefix = base_logger.get_prefix();
            const prefix_type* prefix_1 = nullptr;
            if (!base_logger_has_prefix) {
                prefix_1 = &base_channel_prefix;
            }
            else if (base_channel_prefix.is_null_prefix()) {
                prefix_1 = &base_logger_prefix;
            }
            else {
                prefix_1 = &prefixes.add(base_channel_prefix, base_logger_prefix);
            }
            sink_type& sink = base_channel.get_sink();
            std::string_view channel_name = base_channel.get_name();
            auto i = map.find(&base_channel);
            if (i == map.end()) {
                const log::Limit& limit = base_channel.get_limit();
                channels.add(channel_name, limit, *prefix_1, sink);
            }
            else {
                logger_type& target_logger = *i->second.first;
                channel_type& target_channel = *i->second.second;
                const prefix_type& target_channel_prefix = target_channel.get_prefix();
                const prefix_type& target_logger_prefix = target_logger.get_prefix();
                const prefix_type* prefix_2 = nullptr;
                if (target_logger_prefix.is_null_prefix()) {
                    prefix_2 = &target_channel_prefix;
                }
                else if (target_channel_prefix.is_null_prefix()) {
                    prefix_2 = &target_logger_prefix;
                }
                else {
                    prefix_2 = &prefixes.add(target_channel_prefix, target_logger_prefix);
                }
                SinkImpl& sink_2 = sinks.add(sink.get_locale(), *prefix_1, *prefix_2, base_channel, target_channel);
                const log::Limit& limit = sink_2;
                channels.add(channel_name, limit, null_prefix, sink_2);
            }
        }
        ARCHON_ASSERT(prefixes.size() == num_prefixes);
        ARCHON_ASSERT(sinks.size()    == num_sinks);
        ARCHON_ASSERT(channels.size() == num_channels);
        const channel_type* channel_2 = channel_map_type::do_find_channel({ channels.data(), channels.size() },
                                                                          base_logger.get_channel().get_name());
        ARCHON_ASSERT(channel_2);
        channel = const_cast<channel_type*>(channel_2);
    }
};


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::BasicDuplicatingLogger(logger_type& base_logger, logger_type& target_logger)
    : BasicDuplicatingLogger(base_logger, base_logger.get_channel().get_name(), target_logger) // Throws
{
}


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::BasicDuplicatingLogger(logger_type& base_logger, std::string_view origin_channel,
                                                            logger_type& target_logger)
    : BasicDuplicatingLogger(base_logger, std::array { Rule(origin_channel, target_logger) }) // Throws
{
}


template<class C, class T>
BasicDuplicatingLogger<C, T>::BasicDuplicatingLogger(logger_type& base_logger, core::Span<const Rule> rules)
    : BasicDuplicatingLogger(Interm(base_logger, rules, *this)) // Throws
{
}


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::BasicDuplicatingLogger(Interm&& i) noexcept
    : logger_type(*this, *i.channel, *this)
    , m_prefixes(std::move(i.prefixes))
    , m_sinks(std::move(i.sinks))
    , m_channels(std::move(i.channels))
{
}


template<class C, class T>
auto BasicDuplicatingLogger<C, T>::do_get_channels() const noexcept -> core::Span<const channel_type>
{
    return { m_channels.data(), m_channels.size() };
}


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::Rule::Rule(std::string_view origin_channel, logger_type& target_logger) noexcept
    : m_origin_channel(origin_channel)
    , m_target_logger(target_logger)
    , m_target_channel(target_logger.get_channel())
{
}


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::Rule::Rule(std::string_view origin_channel, logger_type& target_logger,
                                                std::string_view target_channel)
    : m_origin_channel(origin_channel)
    , m_target_logger(target_logger)
    , m_target_channel(target_logger.find_channel(target_channel)) // Throws
{
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_DUPLICATING_LOGGER_HPP
