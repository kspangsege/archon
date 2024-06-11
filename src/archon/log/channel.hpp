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

#ifndef ARCHON_X_LOG_X_CHANNEL_HPP
#define ARCHON_X_LOG_X_CHANNEL_HPP

/// \file


#include <cstddef>
#include <array>
#include <string_view>
#include <iostream>    

#include <archon/core/string_formatter.hpp>
#include <archon/log/log_level.hpp>
#include <archon/log/limit.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/sink.hpp>


namespace archon::log {


template<class C, class T = std::char_traits<C>> class BasicChannel {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using prefix_type      = log::BasicPrefix<C, T>;
    using sink_type        = log::BasicSink<C, T>;

    template<class... P> void channel_log(log::LogLevel, const prefix_type&, const char* message, const P&... params);
    template<class... P> void channel_log(log::LogLevel, const prefix_type&, string_view_type message,
                                          const P&... params);

    BasicChannel(std::string_view name, const log::Limit&, const prefix_type&, sink_type&) noexcept;

    auto get_name() const noexcept -> std::string_view;
    auto get_limit() const noexcept -> const log::Limit&;
    auto get_prefix() const noexcept -> const prefix_type&;
    auto get_sink() noexcept -> sink_type&;
    auto get_sink() const noexcept -> const sink_type&;

    static inline constexpr std::size_t format_seed_memory_size = 2048;

private:
    std::string_view m_name;
    const log::Limit& m_limit;
    const prefix_type& m_prefix;
    sink_type& m_sink;
};


using Channel     = BasicChannel<char>;
using WideChannel = BasicChannel<wchar_t>;








// Implementation


template<class C, class T>
template<class... P>
inline void BasicChannel<C, T>::channel_log(log::LogLevel level, const prefix_type& prefix, const char* message,
                                            const P&... params)
{
    std::cerr << "channel_log_1()\n";    
    std::array<C, format_seed_memory_size> seed_memory;
    core::BasicStringFormatter<C, T> formatter(seed_memory, m_sink.get_locale()); // Throws
    string_view_type message_2 = formatter.format(message, params...); // Throws
    m_sink.sink_log(level, m_prefix, prefix, message_2); // Throws
}


template<class C, class T>
template<class... P>
inline void BasicChannel<C, T>::channel_log(log::LogLevel level, const prefix_type& prefix, string_view_type message,
                                            const P&... params)
{
    std::cerr << "channel_log_2()\n";    
    std::array<C, format_seed_memory_size> seed_memory;
    core::BasicStringFormatter<C, T> formatter(seed_memory, m_sink.get_locale()); // Throws
    string_view_type message_2 = formatter.format(message, params...); // Throws
    m_sink.sink_log(level, m_prefix, prefix, message_2); // Throws
}


template<class C, class T>
inline BasicChannel<C, T>::BasicChannel(std::string_view name, const log::Limit& limit, const prefix_type& prefix,
                                        sink_type& sink) noexcept
    : m_name(name)
    , m_limit(limit)
    , m_prefix(prefix)
    , m_sink(sink)
{
}


template<class C, class T>
inline auto BasicChannel<C, T>::get_name() const noexcept -> std::string_view
{
    return m_name;
}


template<class C, class T>
inline auto BasicChannel<C, T>::get_limit() const noexcept -> const log::Limit&
{
    return m_limit;
}


template<class C, class T>
inline auto BasicChannel<C, T>::get_prefix() const noexcept -> const prefix_type&
{
    return m_prefix;
}


template<class C, class T>
inline auto BasicChannel<C, T>::get_sink() noexcept -> sink_type&
{
    return m_sink;
}


template<class C, class T>
inline auto BasicChannel<C, T>::get_sink() const noexcept -> const sink_type&
{
    return m_sink;
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_CHANNEL_HPP
