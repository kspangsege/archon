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

#ifndef ARCHON_X_LOG_X_LOG_HPP
#define ARCHON_X_LOG_X_LOG_HPP

/// \file


#include <archon/log/logger.hpp>


namespace archon::log {


/// \{
///
/// \brief Log message to STDOUT.
///
/// These functions are shorthands for calling the equivalent function on the logger
/// returned by \ref log::Logger::get_cout(); for example, `log::info("foo")` has the same
/// effect as `log::Logger::get_cout().info("foo")`.
///
template<class... P> void fatal(const char* message, const P&... params);
template<class... P> void error(const char* message, const P&... params);
template<class... P> void warn(const char* message, const P&... params);
template<class... P> void info(const char* message, const P&... params);
template<class... P> void detail(const char* message, const P&... params);
template<class... P> void debug(const char* message, const P&... params);
template<class... P> void trace(const char* message, const P&... params);
/// \}








// Implementation


template<class... P> inline void fatal(const char* message, const P&... params)
{
    log::Logger::get_cout().fatal(message, params...); // Throws
}


template<class... P> inline void error(const char* message, const P&... params)
{
    log::Logger::get_cout().error(message, params...); // Throws
}


template<class... P> inline void warn(const char* message, const P&... params)
{
    log::Logger::get_cout().warn(message, params...); // Throws
}


template<class... P> inline void info(const char* message, const P&... params)
{
    log::Logger::get_cout().info(message, params...); // Throws
}


template<class... P> inline void detail(const char* message, const P&... params)
{
    log::Logger::get_cout().detail(message, params...); // Throws
}


template<class... P> inline void debug(const char* message, const P&... params)
{
    log::Logger::get_cout().debug(message, params...); // Throws
}


template<class... P> inline void trace(const char* message, const P&... params)
{
    log::Logger::get_cout().trace(message, params...); // Throws
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_LOG_HPP
