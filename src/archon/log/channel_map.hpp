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

#ifndef ARCHON_X_LOG_X_CHANNEL_MAP_HPP
#define ARCHON_X_LOG_X_CHANNEL_MAP_HPP

/// \file


#include <utility>
#include <algorithm>
#include <string_view>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/channel.hpp>


namespace archon::log {


template<class C, class T = std::char_traits<C>> class BasicChannelMap
    : public log::BasicNullPrefix<C, T> {
public:
    using char_type   = C;
    using traits_type = T;

    using channel_type = log::BasicChannel<C, T>;

    auto find_channel(std::string_view name)       -> channel_type&;
    auto find_channel(std::string_view name) const -> const channel_type&;

    // Sorted ascendingly by channel name
    auto get_channels() noexcept       -> core::Span<channel_type>;
    auto get_channels() const noexcept -> core::Span<const channel_type>;

protected:
    ~BasicChannelMap() noexcept = default;

    static auto do_find_channel(core::Span<const channel_type>, std::string_view name) noexcept -> const channel_type*;

    virtual auto do_get_channels() const noexcept -> core::Span<const channel_type> = 0;
};


using ChannelMap     = BasicChannelMap<char>;
using WideChannelMap = BasicChannelMap<wchar_t>;








// Implementation


template<class C, class T>
inline auto BasicChannelMap<C, T>::find_channel(std::string_view name) -> channel_type&
{
    const channel_type& channel = std::as_const(*this).find_channel(name); // Throws
    return const_cast<channel_type&>(channel); // Throws
}


template<class C, class T>
inline auto BasicChannelMap<C, T>::find_channel(std::string_view name) const -> const channel_type&
{
    core::Span<const channel_type> channels = get_channels();
    const channel_type* channel = do_find_channel(channels, name);
    if (ARCHON_LIKELY(channel))
        return *channel;
    throw std::invalid_argument("No such channel");
}


template<class C, class T>
inline auto BasicChannelMap<C, T>::get_channels() noexcept -> core::Span<channel_type>
{
    auto channels = do_get_channels();
    return { const_cast<channel_type*>(channels.data()), channels.size() };
}


template<class C, class T>
inline auto BasicChannelMap<C, T>::get_channels() const noexcept -> core::Span<const channel_type>
{
    return do_get_channels();
}


template<class C, class T>
auto BasicChannelMap<C, T>::do_find_channel(core::Span<const channel_type> channels, std::string_view name) noexcept ->
    const channel_type*
{
    auto comp = [](const channel_type& channel, const std::string_view& name) noexcept {
        return (channel.get_name() < name);
    };
    auto i = std::lower_bound(channels.begin(), channels.end(), name, comp);
    bool found = (i != channels.end() && i->get_name() == name);
    if (ARCHON_LIKELY(found))
        return &*i;
    return nullptr;
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_CHANNEL_MAP_HPP
