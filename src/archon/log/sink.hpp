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

#ifndef ARCHON_X_LOG_X_SINK_HPP
#define ARCHON_X_LOG_X_SINK_HPP

/// \file


#include <string_view>
#include <locale>

#include <archon/log/log_level.hpp>
#include <archon/log/prefix.hpp>


namespace archon::log {


template<class C, class T = std::char_traits<C>> class BasicSink {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using prefix_type      = log::BasicPrefix<C, T>;

    static auto get_level_prefix(log::LogLevel) -> const char*;

    auto get_locale() const noexcept -> const std::locale&;

    virtual void sink_log(log::LogLevel, const prefix_type& channel_prefix, const prefix_type& message_prefix,
                          string_view_type message) = 0;

protected:
    BasicSink(const std::locale&) noexcept;

    ~BasicSink() noexcept = default;

private:
    const std::locale m_locale;
};


using Sink     = BasicSink<char>;
using WideSink = BasicSink<wchar_t>;








// Implementation


template<class C, class T>
auto BasicSink<C, T>::get_level_prefix(log::LogLevel level) -> const char*
{
    switch (level) {
        case LogLevel::all:
        case LogLevel::trace:
        case LogLevel::debug:
        case LogLevel::detail:
        case LogLevel::info:
        case LogLevel::off:
            return "";
        case LogLevel::warn:
            return "WARNING: ";
        case LogLevel::error:
            return "ERROR: ";
        case LogLevel::fatal:
            return "FATAL: ";
    }
    ARCHON_ASSERT(false);
    return nullptr;
}


template<class C, class T>
inline auto BasicSink<C, T>::get_locale() const noexcept -> const std::locale&
{
    return m_locale;
}


template<class C, class T>
inline BasicSink<C, T>::BasicSink(const std::locale& locale) noexcept
    : m_locale(locale)
{
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_SINK_HPP
