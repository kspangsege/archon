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

#ifndef ARCHON_X_LOG_X_CHANNEL_LOGGER_HPP
#define ARCHON_X_LOG_X_CHANNEL_LOGGER_HPP

/// \file


#include <cstddef>
#include <memory>
#include <utility>
#include <string_view>
#include <array>
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


/// FIXME: Explain: Introduce new channels.     
template<class C, class T = std::char_traits<C>> class BasicChannelLogger final
    : private log::BasicChannelMap<C, T>
    , public log::BasicLogger<C, T> {
public:
    class Rule;

    using prefix_type      = log::BasicPrefix<C, T>;
    using sink_type        = log::BasicSink<C, T>;
    using channel_type     = log::BasicChannel<C, T>;
    using channel_map_type = log::BasicChannelMap<C, T>;
    using logger_type      = log::BasicLogger<C, T>;

    /// \brief Construct logger with one new channel.
    ///
    /// Copy all the channels, as well as the channel selection, from the specified base
    /// logger (\p base_logger), then introduce a new channel (\p new_channel) that targets
    /// the selected channel in the specified target logger (\p target_logger). If a channel
    /// already exists with the specified name, it will be replaced by the new channel.
    ///
    /// This constructor is a shorthand for `BasicChannelLogger(base_logger, rules)` where
    /// `rules` contains one rule constructed as `Rule(new_channel, target_logger)`.
    ///
    BasicChannelLogger(logger_type& base_logger, std::string_view new_channel, logger_type& target_logger);

    BasicChannelLogger(logger_type& base_logger, core::Span<const Rule>);

private:
    class Interm;

    using compound_prefix_type = log::BasicCompoundPrefix<C, T>;

    std::unique_ptr<char[]> m_strings;
    core::Slab<compound_prefix_type> m_prefixes;
    core::Slab<channel_type> m_channels;

    BasicChannelLogger(Interm&&) noexcept;

    // Overriding functions from log::BasicChannelMap<C, T>
    auto do_get_channels() const noexcept -> core::Span<const channel_type> override final;
};


using ChannelLogger     = BasicChannelLogger<char>;
using WideChannelLogger = BasicChannelLogger<wchar_t>;




template<class C, class T> class BasicChannelLogger<C, T>::Rule {
public:
    Rule(std::string_view new_channel, logger_type& target_logger) noexcept;
    Rule(std::string_view new_channel, logger_type& target_logger, std::string_view target_channel);

private:
    std::string_view m_new_channel;
    logger_type& m_target_logger;
    channel_type& m_target_channel;

    friend class BasicChannelLogger<C, T>;
};








// Implementation


template<class C, class T>
class BasicChannelLogger<C, T>::Interm {
public:
    std::unique_ptr<char[]> strings;
    core::Slab<compound_prefix_type> prefixes;
    core::Slab<channel_type> channels;
    channel_type* channel = nullptr;

    Interm(logger_type& base_logger, core::Span<const Rule> rules)
    {
        std::map<std::string_view, std::pair<logger_type*, channel_type*>> map;
        channel_map_type& base_map = base_logger.get_channel_map();
        auto base_channels = base_map.get_channels();
        for (channel_type& channel : base_channels) {
            std::string_view channel_name = channel.get_name();
            map[channel_name] = { nullptr, &channel }; // Throws
        }
        for (const Rule& rule : rules) {
            std::string_view channel_name = rule.m_new_channel;
            map[channel_name] = { &rule.m_target_logger, &rule.m_target_channel }; // Throws
        }
        std::size_t strings_size = 0;
        std::size_t num_prefixes = 0;
        std::size_t num_channels = map.size();
        const prefix_type& base_logger_prefix = base_logger.get_prefix();
        bool base_logger_has_prefix = !base_logger_prefix.is_null_prefix();
        for (const auto& entry : map) {
            bool logger_has_prefix = base_logger_has_prefix;
            logger_type* logger = entry.second.first;
            bool inherited = !logger;
            if (!inherited) {
                std::string_view channel_name = entry.first;
                strings_size += channel_name.size();
                logger_has_prefix = !logger->get_prefix().is_null_prefix();
            }
            channel_type& channel = *entry.second.second;
            bool channel_has_prefix = !channel.get_prefix().is_null_prefix();
            if (channel_has_prefix && logger_has_prefix)
                ++num_prefixes;
        }
        strings = std::make_unique<char[]>(strings_size); // Throws
        prefixes.recreate(num_prefixes); // Throws
        channels.recreate(num_channels); // Throws
        char* strings_ptr = strings.get();
        for (const auto& entry : map) {
            std::string_view channel_name = entry.first;
            std::string_view channel_name_2 = channel_name;
            const prefix_type* logger_prefix = &base_logger_prefix;
            logger_type* logger = entry.second.first;
            bool inherited = !logger;
            if (!inherited) {
                // New channel name
                channel_name_2 = { strings_ptr, channel_name.size() };
                strings_ptr = std::copy_n(channel_name.data(), channel_name.size(), strings_ptr);
                logger_prefix = &logger->get_prefix();
            }
            bool logger_has_prefix = !logger_prefix->is_null_prefix();
            channel_type& channel = *entry.second.second;
            const prefix_type& channel_prefix = channel.get_prefix();
            const prefix_type* prefix = nullptr;
            if (!logger_has_prefix) {
                prefix = &channel_prefix;
            }
            else if (channel_prefix.is_null_prefix()) {
                prefix = logger_prefix;
            }
            else {
                prefix = &prefixes.add(channel_prefix, *logger_prefix);
            }
            const log::Limit& limit = channel.get_limit();
            sink_type& sink = channel.get_sink();
            channels.add(channel_name_2, limit, *prefix, sink);
        }
        ARCHON_ASSERT(strings_ptr == strings.get() + strings_size);
        ARCHON_ASSERT(prefixes.size() == num_prefixes);
        ARCHON_ASSERT(channels.size() == num_channels);
        const channel_type* channel_2 = channel_map_type::do_find_channel({ channels.data(), channels.size() },
                                                                          base_logger.get_channel().get_name());
        ARCHON_ASSERT(channel_2);
        channel = const_cast<channel_type*>(channel_2);
    }
};


template<class C, class T>
inline BasicChannelLogger<C, T>::BasicChannelLogger(logger_type& base_logger, std::string_view new_channel,
                                                    logger_type& target_logger)
    : BasicChannelLogger(base_logger, std::array { Rule(new_channel, target_logger) }) // Throws
{
}


template<class C, class T>
BasicChannelLogger<C, T>::BasicChannelLogger(logger_type& base_logger, core::Span<const Rule> rules)
    : BasicChannelLogger(Interm(base_logger, rules)) // Throws
{
}


template<class C, class T>
inline BasicChannelLogger<C, T>::BasicChannelLogger(Interm&& i) noexcept
    : logger_type(*this, *i.channel, *this)
    , m_strings(std::move(i.strings))
    , m_prefixes(std::move(i.prefixes))
    , m_channels(std::move(i.channels))
{
}


template<class C, class T>
auto BasicChannelLogger<C, T>::do_get_channels() const noexcept -> core::Span<const channel_type>
{
    return { m_channels.data(), m_channels.size() };
}


template<class C, class T>
inline BasicChannelLogger<C, T>::Rule::Rule(std::string_view new_channel, logger_type& target_logger) noexcept
    : m_new_channel(new_channel)
    , m_target_logger(target_logger)
    , m_target_channel(target_logger.get_channel())
{
}


template<class C, class T>
inline BasicChannelLogger<C, T>::Rule::Rule(std::string_view new_channel, logger_type& target_logger,
                                            std::string_view target_channel)
    : m_new_channel(new_channel)
    , m_target_logger(target_logger)
    , m_target_channel(target_logger.find_channel(target_channel)) // Throws
{
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_CHANNEL_LOGGER_HPP
