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

#ifndef ARCHON_X_LOG_X_STREAM_LOGGER_HPP
#define ARCHON_X_LOG_X_STREAM_LOGGER_HPP

/// \file


#include <string_view>
#include <ostream>

#include <archon/log/logger.hpp>


namespace archon::log {


/// \brief A logger that sends messages to a stream.
///
/// This is a root logger that sends messages to the specified stream.
///
/// An instance of \ref BasicStreamLogger is thread-safe in so far as the targeted stream is                       
/// not accessed in ways other than through the logger instance during the entire lifetime
/// of the logger instance.
///
template<class C, class T = std::char_traits<C>> class BasicStreamLogger final
    : public log::BasicRootLogger<C, T> {
public:
    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;
    using root_logger_type = log::BasicRootLogger<C, T>;

    explicit BasicStreamLogger(ostream_type&);

protected:
    // Overriding functions from log::BasicRootLogger<C, T>
    void root_log(string_view_type message) override;

private:
    ostream_type& m_out;
};


using StreamLogger     = BasicStreamLogger<char>;
using WideStreamLogger = BasicStreamLogger<wchar_t>;








// Implementation


template<class C, class T>
inline BasicStreamLogger<C, T>::BasicStreamLogger(ostream_type& out)
    : root_logger_type(out.getloc()) // Throws
    , m_out(out)
{
}


template<class C, class T>
void BasicStreamLogger<C, T>::root_log(string_view_type message)
{
    m_out.write(message.data(), message.size()); // Throws
    m_out.flush(); // Throws
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_STREAM_LOGGER_HPP
