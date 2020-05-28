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

#ifndef ARCHON_X_LOG_X_PREFIX_LOGGER_HPP
#define ARCHON_X_LOG_X_PREFIX_LOGGER_HPP

/// \file


#include <utility>
#include <string>

#include <archon/core/char_mapper.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/channel.hpp>
#include <archon/log/logger.hpp>


namespace archon::log {


template<class C, class T = std::char_traits<C>> class BasicPrefixLogger final
    : private log::BasicPrefix<C, T>
    , public log::BasicLogger<C, T> {
public:
    using string_view_type = std::basic_string_view<C, T>;
    using string_type      = std::basic_string<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;
    using prefix_type      = log::BasicPrefix<C, T>;
    using channel_type     = log::BasicChannel<C, T>;
    using logger_type      = log::BasicLogger<C, T>;

    BasicPrefixLogger(logger_type& base_logger, const char* prefix);
    BasicPrefixLogger(logger_type& base_logger, string_view_type prefix);
    BasicPrefixLogger(logger_type& base_logger, string_type&& prefix) noexcept;
    BasicPrefixLogger(logger_type& base_logger, const char* prefix, std::string_view channel);
    BasicPrefixLogger(logger_type& base_logger, string_view_type prefix, std::string_view channel);
    BasicPrefixLogger(logger_type& base_logger, string_type&& prefix, std::string_view channel);

private:
    const prefix_type& m_parent_prefix;
    const string_type m_prefix;

    BasicPrefixLogger(logger_type& base_logger, channel_type&, string_type&& prefix) noexcept;

    static auto widen(std::string_view prefix, const logger_type& base_logger) -> string_type;

    // Overriding functions from log::BasicPrefix<C, T>
    void format_prefix(ostream_type&) const override final;
};


using PrefixLogger     = BasicPrefixLogger<char>;
using WidePrefixLogger = BasicPrefixLogger<wchar_t>;








// Implementation


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, const char* prefix)
    : BasicPrefixLogger(base_logger, widen(prefix, base_logger)) // Throws
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, string_view_type prefix)
    : BasicPrefixLogger(base_logger, string_type(prefix)) // Throws
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, string_type&& prefix) noexcept
    : BasicPrefixLogger(base_logger, base_logger.get_channel(), std::move(prefix))
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, const char* prefix,
                                                  std::string_view channel)
    : BasicPrefixLogger(base_logger, widen(prefix, base_logger), channel) // Throws
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, string_view_type prefix,
                                                  std::string_view channel)
    : BasicPrefixLogger(base_logger, string_type(prefix), channel) // Throws
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, string_type&& prefix,
                                                  std::string_view channel)
    : BasicPrefixLogger(base_logger, base_logger.find_channel(channel), std::move(prefix)) // Throws
{
}


template<class C, class T>
BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, channel_type& channel,
                                           string_type&& prefix) noexcept
    : logger_type(*this, channel, base_logger.get_channel_map())
    , m_parent_prefix(base_logger.get_prefix())
    , m_prefix(std::move(prefix))
{
}


template<class C, class T>
auto BasicPrefixLogger<C, T>::widen(std::string_view prefix, const logger_type& base_logger) -> string_type
{
    string_type prefix_2(prefix.size(), C()); // Throws
    core::BasicCharMapper<C, T> mapper(base_logger.get_locale()); // Throws
    mapper.widen(prefix, prefix_2.data()); // Throws
    return prefix_2;
}


template<class C, class T>
void BasicPrefixLogger<C, T>::format_prefix(ostream_type& out) const
{
    m_parent_prefix.format_prefix(out); // Throws
    out << m_prefix; // Throws
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_PREFIX_LOGGER_HPP
