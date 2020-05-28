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

#ifndef ARCHON__BASE__CHANNEL_LOGGER_HPP
#define ARCHON__BASE__CHANNEL_LOGGER_HPP

#include <cstddef>
#include <memory>
#include <utility>
#include <string_view>
#include <array>
#include <map>

#include <archon/base/assert.hpp>
#include <archon/base/memory.hpp>
#include <archon/base/span.hpp>
#include <archon/base/logger.hpp>


namespace archon::base {


/// FIXME: Explain: Introduce new channels.     
template<class C, class T = std::char_traits<C>> class BasicChannelLogger final :
        private BasicLogger<C, T>::Map,
        public BasicLogger<C, T> {
public:
    class Rule;

    using logger_type = BasicLogger<C, T>;

    /// \brief Construct logger with one new channel.
    ///
    /// Copy all the channels, as well as the channel selection, from the
    /// specified base logger (\p base_logger), then introduce a new channel (\p
    /// new_channel) that targets the selected channel in the specified target
    /// logger (\p target_logger). If a channel already exists with the
    /// specified name, it will be replaced by the new channel.
    ///
    /// This constructor is a shorthand for `BasicChannelLogger(base_logger,
    /// rules)` where `rules` contains one rule constructed as
    /// `Rule(new_channel, target_logger)`.
    ///
    BasicChannelLogger(logger_type& base_logger, std::string_view new_channel,
                       logger_type& target_logger);

    BasicChannelLogger(logger_type& base_logger, Span<const Rule>);

private:
    class Interm;

    using Limit          = typename logger_type::Limit;
    using Prefix         = typename logger_type::Prefix;
    using CompoundPrefix = typename logger_type::CompoundPrefix;
    using Channel        = typename logger_type::Channel;
    using Map            = typename logger_type::Map;
    using Sink           = typename logger_type::Sink;

    std::unique_ptr<char[]> m_strings;
    Slab<CompoundPrefix> m_prefixes;
    Slab<Channel> m_channels;

    BasicChannelLogger(Interm&&) noexcept;

    // Overriding functions from Map
    Span<const Channel> do_get_channels() const noexcept override final;
};


using ChannelLogger     = BasicChannelLogger<char>;
using WideChannelLogger = BasicChannelLogger<wchar_t>;




template<class C, class T> class BasicChannelLogger<C, T>::Rule {
public:
    Rule(std::string_view new_channel, logger_type& target_logger) noexcept;
    Rule(std::string_view new_channel, logger_type& target_logger,
         std::string_view target_channel);

private:
    std::string_view m_new_channel;
    logger_type& m_target_logger;
    Channel& m_target_channel;

    friend class BasicChannelLogger<C, T>;
};








// Implementation


template<class C, class T> class BasicChannelLogger<C, T>::Interm {
public:
    std::unique_ptr<char[]> strings;
    Slab<CompoundPrefix> prefixes;
    Slab<Channel> channels;
    Channel* channel = nullptr;

    Interm(logger_type& base_logger, Span<const Rule> rules)
    {
        std::map<std::string_view, std::pair<logger_type*, Channel*>> map;
        Map& base_map = base_logger.get_map();
        auto base_channels = base_map.get_channels();
        for (Channel& channel : base_channels) {
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
        const Prefix& base_logger_prefix = base_logger.get_prefix();
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
            Channel& channel = *entry.second.second;
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
            const Prefix* logger_prefix = &base_logger_prefix;
            logger_type* logger = entry.second.first;
            bool inherited = !logger;
            if (!inherited) {
                // New channel name
                channel_name_2 = { strings_ptr, channel_name.size() };
                strings_ptr = std::copy_n(channel_name.data(), channel_name.size(), strings_ptr);
                logger_prefix = &logger->get_prefix();
            }
            bool logger_has_prefix = !logger_prefix->is_null_prefix();
            Channel& channel = *entry.second.second;
            const Prefix& channel_prefix = channel.get_prefix();
            const Prefix* prefix = nullptr;
            if (!logger_has_prefix) {
                prefix = &channel_prefix;
            }
            else if (channel_prefix.is_null_prefix()) {
                prefix = logger_prefix;
            }
            else {
                prefix = &prefixes.add(channel_prefix, *logger_prefix);
            }
            const Limit& limit = channel.get_limit();
            Sink& sink = channel.get_sink();
            channels.add(channel_name_2, limit, *prefix, sink);
        }
        ARCHON_ASSERT(strings_ptr == strings.get() + strings_size);
        ARCHON_ASSERT(prefixes.size() == num_prefixes);
        ARCHON_ASSERT(channels.size() == num_channels);
        const Channel* channel_2 =
            Map::do_find_channel({ channels.data(), channels.size() },
                                 base_logger.get_channel().get_name());
        ARCHON_ASSERT(channel_2);
        channel = const_cast<Channel*>(channel_2);
    }
};


template<class C, class T>
inline BasicChannelLogger<C, T>::BasicChannelLogger(logger_type& base_logger,
                                                    std::string_view new_channel,
                                                    logger_type& target_logger) :
    BasicChannelLogger(base_logger, std::array { Rule(new_channel, target_logger) }) // Throws
{
}


template<class C, class T>
BasicChannelLogger<C, T>::BasicChannelLogger(logger_type& base_logger, Span<const Rule> rules) :
    BasicChannelLogger(Interm(base_logger, rules)) // Throws
{
}


template<class C, class T>
inline BasicChannelLogger<C, T>::BasicChannelLogger(Interm&& i) noexcept :
    logger_type(*this, *i.channel, *this),
    m_strings(std::move(i.strings)),
    m_prefixes(std::move(i.prefixes)),
    m_channels(std::move(i.channels))
{
}


template<class C, class T>
auto BasicChannelLogger<C, T>::do_get_channels() const noexcept -> Span<const Channel>
{
    return { m_channels.data(), m_channels.size() };
}


template<class C, class T>
inline BasicChannelLogger<C, T>::Rule::Rule(std::string_view new_channel,
                                            logger_type& target_logger) noexcept :
    m_new_channel(new_channel),
    m_target_logger(target_logger),
    m_target_channel(target_logger.get_channel())
{
}


template<class C, class T>
inline BasicChannelLogger<C, T>::Rule::Rule(std::string_view new_channel,
                                            logger_type& target_logger,
                                            std::string_view target_channel) :
    m_new_channel(new_channel),
    m_target_logger(target_logger),
    m_target_channel(target_logger.find_channel(target_channel)) // Throws
{
}


} // namespace archon::base

#endif // ARCHON__BASE__CHANNEL_LOGGER_HPP
