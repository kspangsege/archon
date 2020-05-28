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

#ifndef ARCHON__BASE__PREFIX_LOGGER_HPP
#define ARCHON__BASE__PREFIX_LOGGER_HPP

#include <utility>
#include <string>

#include <archon/base/char_mapper.hpp>
#include <archon/base/logger.hpp>


namespace archon::base {


template<class C, class T = std::char_traits<C>> class BasicPrefixLogger final :
        private BasicLogger<C, T>::Prefix,
        public BasicLogger<C, T> {
public:
    using logger_type = BasicLogger<C, T>;

    using string_view_type = typename logger_type::string_view_type;
    using ostream_type     = typename logger_type::ostream_type;

    BasicPrefixLogger(logger_type& base_logger, const char* prefix);
    BasicPrefixLogger(logger_type& base_logger, string_view_type prefix);
    BasicPrefixLogger(logger_type& base_logger, const char* prefix, std::string_view channel);
    BasicPrefixLogger(logger_type& base_logger, string_view_type prefix, std::string_view channel);

private:
    using Prefix  = typename logger_type::Prefix;
    using Channel = typename logger_type::Channel;

    using string_type  = std::basic_string<C, T>;

    const Prefix& m_parent_prefix;
    const string_type m_prefix;

    BasicPrefixLogger(logger_type& base_logger, Channel&, const char* prefix);
    BasicPrefixLogger(logger_type& base_logger, Channel&, string_type prefix) noexcept;

    static string_type widen(std::string_view prefix, const logger_type& base_logger);

    // Overriding functions from Prefix
    void format_prefix(ostream_type&) const override final;
};


using PrefixLogger     = BasicPrefixLogger<char>;
using WidePrefixLogger = BasicPrefixLogger<wchar_t>;








// Implementation


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, const char* prefix) :
    BasicPrefixLogger(base_logger, base_logger.get_channel(), prefix) // Throws
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger,
                                                  string_view_type prefix) :
    BasicPrefixLogger(base_logger, base_logger.get_channel(), string_type(prefix)) // Throws
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, const char* prefix,
                                                  std::string_view channel) :
    BasicPrefixLogger(base_logger, base_logger.find_channel(channel), prefix) // Throws
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger,
                                                  string_view_type prefix,
                                                  std::string_view channel) :
    BasicPrefixLogger(base_logger, base_logger.find_channel(channel),
                      string_type(prefix)) // Throws
{
}


template<class C, class T>
inline BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, Channel& channel,
                                                  const char* prefix) :
    BasicPrefixLogger(base_logger, channel, widen(prefix, base_logger)) // Throws
{
}


template<class C, class T>
BasicPrefixLogger<C, T>::BasicPrefixLogger(logger_type& base_logger, Channel& channel,
                                           string_type prefix) noexcept :
    logger_type(*this, channel, base_logger.get_map()),
    m_parent_prefix(base_logger.get_prefix()),
    m_prefix(std::move(prefix))
{
}


template<class C, class T>
auto BasicPrefixLogger<C, T>::widen(std::string_view prefix,
                                    const logger_type& base_logger) -> string_type
{
    string_type prefix_2(prefix.size(), C()); // Throws
    BasicCharMapper<C, T> mapper(base_logger.get_locale()); // Throws
    mapper.widen(prefix, prefix_2.data()); // Throws
    return prefix_2;
}


template<class C, class T> void BasicPrefixLogger<C, T>::format_prefix(ostream_type& out) const
{
    m_parent_prefix.format_prefix(out); // Throws
    out << m_prefix; // Throws
}


} // namespace archon::base

#endif // ARCHON__BASE__PREFIX_LOGGER_HPP
