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

#ifndef ARCHON_X_BASE_X_DUPLICATING_LOGGER_HPP
#define ARCHON_X_BASE_X_DUPLICATING_LOGGER_HPP

/// \file


#include <cstddef>
#include <algorithm>
#include <utility>
#include <array>
#include <string_view>
#include <map>

#include <archon/base/assert.hpp>
#include <archon/base/memory.hpp>
#include <archon/base/span.hpp>
#include <archon/base/logger.hpp>


namespace archon::base {


template<class C, class T = std::char_traits<C>> class BasicDuplicatingLogger final :
        private BasicLogger<C, T>::Map,
        public BasicLogger<C, T> {
public:
    class Rule;

    using logger_type = BasicLogger<C, T>;

    /// \brief Construct logger that duplicates messages on selected channel.
    ///
    /// Copy all the channels, as well as the channel selection, from the
    /// specified base logger (\p base_logger), then arrange for the messages on
    /// the selected channel to be duplicated onto the selected channel in the
    /// specified target logger (\p target_logger).
    ///
    /// This constructor is a shorthand for `BasicDuplicatingLogger(base_logger,
    /// base_logger.get_channel().get_name(), target_logger)`.
    ///
    BasicDuplicatingLogger(logger_type& base_logger, logger_type& target_logger);

    /// \brief Construct logger that duplicates messages on one channel.
    ///
    /// Copy all the channels, as well as the channel selection, from the
    /// specified base logger (\p base_logger), then arrange for the messages on
    /// the specified channel (\p origin_channel) to be duplicated onto the
    /// selected channel in the specified target logger (\p target_logger).
    ///
    /// This constructor is a shorthand for `BasicDuplicatingLogger(base_logger,
    /// rules)` where `rules` contains one rule constructed as
    /// `Rule(origin_channel, target_logger)`.
    ///
    BasicDuplicatingLogger(logger_type& base_logger, std::string_view origin_channel,
                           logger_type& target_logger);

    BasicDuplicatingLogger(logger_type& base_logger, Span<const Rule>);

private:
    class SinkImpl;
    class Interm;

    using Limit          = typename logger_type::Limit;
    using Prefix         = typename logger_type::Prefix;
    using NullPrefix     = typename logger_type::NullPrefix;
    using CompoundPrefix = typename logger_type::CompoundPrefix;
    using Channel        = typename logger_type::Channel;
    using Map            = typename logger_type::Map;
    using Sink           = typename logger_type::Sink;

    Slab<CompoundPrefix> m_prefixes;
    Slab<SinkImpl> m_sinks;
    Slab<Channel> m_channels;

    BasicDuplicatingLogger(Interm&&) noexcept;

    // Overriding functions from Map
    Span<const Channel> do_get_channels() const noexcept override final;
};


using DuplicatingLogger     = BasicDuplicatingLogger<char>;
using WideDuplicatingLogger = BasicDuplicatingLogger<wchar_t>;




template<class C, class T> class BasicDuplicatingLogger<C, T>::Rule {
public:
    Rule(std::string_view origin_channel, logger_type& target_logger) noexcept;
    Rule(std::string_view origin_channel, logger_type& target_logger,
         std::string_view target_channel);

private:
    std::string_view m_origin_channel;
    logger_type& m_target_logger;
    Channel& m_target_channel;

    friend class BasicDuplicatingLogger<C, T>;
};








// Implementation


template<class C, class T>
class BasicDuplicatingLogger<C, T>::SinkImpl final : public Sink, public Limit {
public:
    using string_view_type = typename logger_type::string_view_type;

    SinkImpl(const std::locale& locale, const Prefix& prefix_1, const Prefix& prefix_2,
             Channel& channel_1, Channel& channel_2) noexcept :
        Sink(locale),
        m_limit_1(channel_1.get_limit()),
        m_limit_2(channel_2.get_limit()),
        m_prefix_1(prefix_1),
        m_prefix_2(prefix_2),
        m_sink_1(channel_1.get_sink()),
        m_sink_2(channel_2.get_sink())
    {
    }

    // Overriding function from Sink
    void sink_log(LogLevel level, const Prefix& channel_prefix, const Prefix& message_prefix,
                  string_view_type message) override final
    {
        CompoundPrefix channel_prefix_1(m_prefix_1, channel_prefix);
        m_sink_1.sink_log(level, channel_prefix_1, message_prefix, message); // Throws
        CompoundPrefix channel_prefix_2(m_prefix_2, channel_prefix);
        m_sink_2.sink_log(level, channel_prefix_2, message_prefix, message); // Throws
    }

    // Overriding function from Limit
    int get_fixed_limit() const noexcept override final
    {
        int a = m_limit_1.get_fixed_limit();
        int b = m_limit_2.get_fixed_limit();
        return std::max(a, b);
    }

    // Overriding function from Limit
    LogLevel get_level_limit() const noexcept override final
    {
        LogLevel a = m_limit_1.get_level_limit();
        LogLevel b = m_limit_2.get_level_limit();
        return LogLevel(std::max(int(a), int(b)));
    }

private:
    const Limit& m_limit_1;
    const Limit& m_limit_2;
    const Prefix& m_prefix_1;
    const Prefix& m_prefix_2;
    Sink& m_sink_1;
    Sink& m_sink_2;
};


template<class C, class T> class BasicDuplicatingLogger<C, T>::Interm {
public:
    Slab<CompoundPrefix> prefixes;
    Slab<SinkImpl> sinks;
    Slab<Channel> channels;
    Channel* channel = nullptr;

    Interm(logger_type& base_logger, Span<const Rule> rules, const NullPrefix& null_prefix)
    {
        Map& base_map = base_logger.get_map();
        std::map<const void*, std::pair<logger_type*, Channel*>> map;
        for (const Rule& rule : rules) {
            const Channel& origin_channel = base_map.find_channel(rule.m_origin_channel); // Throws
            map[&origin_channel] = { &rule.m_target_logger, &rule.m_target_channel }; // Throws
        }
        std::size_t num_prefixes = 0;
        for (const auto& entry : map) {
            logger_type& target_logger = *entry.second.first;
            Channel& target_channel = *entry.second.second;
            bool logger_has_prefix  = !target_logger.get_prefix().is_null_prefix();
            bool channel_has_prefix = !target_channel.get_prefix().is_null_prefix();
            if (channel_has_prefix && logger_has_prefix)
                ++num_prefixes;
        }
        bool base_logger_has_prefix = !base_logger.get_prefix().is_null_prefix();
        auto base_channels = base_map.get_channels();
        for (const Channel& channel : base_channels) {
            bool channel_has_prefix = !channel.get_prefix().is_null_prefix();
            if (channel_has_prefix && base_logger_has_prefix)
                ++num_prefixes;
        }
        std::size_t num_sinks = map.size();
        std::size_t num_channels = base_channels.size();
        prefixes.recreate(num_prefixes); // Throws
        sinks.recreate(num_sinks); // Throws
        channels.recreate(num_channels); // Throws
        for (Channel& base_channel : base_channels) {
            const Prefix& base_channel_prefix = base_channel.get_prefix();
            const Prefix& base_logger_prefix = base_logger.get_prefix();
            const Prefix* prefix_1 = nullptr;
            if (!base_logger_has_prefix) {
                prefix_1 = &base_channel_prefix;
            }
            else if (base_channel_prefix.is_null_prefix()) {
                prefix_1 = &base_logger_prefix;
            }
            else {
                prefix_1 = &prefixes.add(base_channel_prefix, base_logger_prefix);
            }
            Sink& sink = base_channel.get_sink();
            std::string_view channel_name = base_channel.get_name();
            auto i = map.find(&base_channel);
            if (i == map.end()) {
                const Limit& limit = base_channel.get_limit();
                channels.add(channel_name, limit, *prefix_1, sink);
            }
            else {
                logger_type& target_logger = *i->second.first;
                Channel& target_channel = *i->second.second;
                const Prefix& target_channel_prefix = target_channel.get_prefix();
                const Prefix& target_logger_prefix = target_logger.get_prefix();
                const Prefix* prefix_2 = nullptr;
                if (target_logger_prefix.is_null_prefix()) {
                    prefix_2 = &target_channel_prefix;
                }
                else if (target_channel_prefix.is_null_prefix()) {
                    prefix_2 = &target_logger_prefix;
                }
                else {
                    prefix_2 = &prefixes.add(target_channel_prefix, target_logger_prefix);
                }
                SinkImpl& sink_2 = sinks.add(sink.get_locale(), *prefix_1, *prefix_2, base_channel,
                                             target_channel);
                const Limit& limit = sink_2;
                channels.add(channel_name, limit, null_prefix, sink_2);
            }
        }
        ARCHON_ASSERT(prefixes.size() == num_prefixes);
        ARCHON_ASSERT(sinks.size()    == num_sinks);
        ARCHON_ASSERT(channels.size() == num_channels);
        const Channel* channel_2 =
            Map::do_find_channel({ channels.data(), channels.size() },
                                 base_logger.get_channel().get_name());
        ARCHON_ASSERT(channel_2);
        channel = const_cast<Channel*>(channel_2);
    }
};


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::BasicDuplicatingLogger(logger_type& base_logger,
                                                            logger_type& target_logger) :
    BasicDuplicatingLogger(base_logger, base_logger.get_channel().get_name(),
                           target_logger) // Throws
{
}


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::BasicDuplicatingLogger(logger_type& base_logger,
                                                            std::string_view origin_channel,
                                                            logger_type& target_logger) :
    BasicDuplicatingLogger(base_logger,
                           std::array { Rule(origin_channel, target_logger) }) // Throws
{
}


template<class C, class T>
BasicDuplicatingLogger<C, T>::BasicDuplicatingLogger(logger_type& base_logger,
                                                     Span<const Rule> rules) :
    BasicDuplicatingLogger(Interm(base_logger, rules, *this)) // Throws
{
}


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::BasicDuplicatingLogger(Interm&& i) noexcept :
    logger_type(*this, *i.channel, *this),
    m_prefixes(std::move(i.prefixes)),
    m_sinks(std::move(i.sinks)),
    m_channels(std::move(i.channels))
{
}


template<class C, class T>
auto BasicDuplicatingLogger<C, T>::do_get_channels() const noexcept -> Span<const Channel>
{
    return { m_channels.data(), m_channels.size() };
}


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::Rule::Rule(std::string_view origin_channel,
                                                logger_type& target_logger) noexcept :
    m_origin_channel(origin_channel),
    m_target_logger(target_logger),
    m_target_channel(target_logger.get_channel())
{
}


template<class C, class T>
inline BasicDuplicatingLogger<C, T>::Rule::Rule(std::string_view origin_channel,
                                                logger_type& target_logger,
                                                std::string_view target_channel) :
    m_origin_channel(origin_channel),
    m_target_logger(target_logger),
    m_target_channel(target_logger.find_channel(target_channel)) // Throws
{
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_DUPLICATING_LOGGER_HPP
