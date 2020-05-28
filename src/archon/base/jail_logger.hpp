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

#ifndef ARCHON_X_BASE_X_JAIL_LOGGER_HPP
#define ARCHON_X_BASE_X_JAIL_LOGGER_HPP

/// \file


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


/// \brief Logger that restricts access to single channel.
///
/// A jail logger object is used to restrict access to a single channel in some
/// other logger object, i.e., the target logger. From the point of view of the
/// user of the jail logger, there is only one channel, and it has an empty
/// channel name. The jail logger can be connected to any channel in the target
/// logger.
///
template<class C, class T = std::char_traits<C>> class BasicJailLogger final :
        private BasicLogger<C, T>::Map,
        public BasicLogger<C, T> {
public:
    using logger_type = BasicLogger<C, T>;

    explicit BasicJailLogger(logger_type& target_logger) noexcept;
    BasicJailLogger(logger_type& target_logger, std::string_view target_channel);

private:
    using Prefix         = typename logger_type::Prefix;
    using CompoundPrefix = typename logger_type::CompoundPrefix;
    using Channel        = typename logger_type::Channel;

    CompoundPrefix m_prefix;
    Channel m_channel;

    BasicJailLogger(logger_type& target_logger, Channel& target_channel) noexcept;

    // Overriding functions from Map
    Span<const Channel> do_get_channels() const noexcept override final;
};


using JailLogger     = BasicJailLogger<char>;
using WideJailLogger = BasicJailLogger<wchar_t>;








// Implementation


template<class C, class T>
inline BasicJailLogger<C, T>::BasicJailLogger(logger_type& target_logger) noexcept :
    BasicJailLogger(target_logger, target_logger.get_channel())
{
}


template<class C, class T>
inline BasicJailLogger<C, T>::BasicJailLogger(logger_type& target_logger,
                                              std::string_view target_channel) :
    BasicJailLogger(target_logger, target_logger.find_channel(target_channel)) // Throws
{
}


template<class C, class T>
BasicJailLogger<C, T>::BasicJailLogger(logger_type& target_logger,
                                       Channel& target_channel) noexcept :
    logger_type(target_channel.get_limit(), *this, m_channel, *this),
    m_prefix(target_channel.get_prefix(), target_logger.get_prefix()),
    m_channel("", target_channel.get_limit(), m_prefix.get_simplified(), target_channel.get_sink())
{
}


template<class C, class T>
auto BasicJailLogger<C, T>::do_get_channels() const noexcept -> Span<const Channel>
{
    return { &m_channel, 1 };
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_JAIL_LOGGER_HPP
