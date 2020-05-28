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

#ifndef ARCHON_X_LOG_X_ENCODING_LOGGER_HPP
#define ARCHON_X_LOG_X_ENCODING_LOGGER_HPP

/// \file


#include <archon/log/logger.hpp>
#include <archon/log/impl/encoding_logger_impl.hpp>


namespace archon::log {


template<class C, class T = std::char_traits<C>> class BasicEncodingLogger
    : private impl::EncodingLoggerImpl<C, T>
    , public log::BasicLogger<C, T> {
public:
    BasicEncodingLogger(log::Logger& base_logger);
};


using EncodingLogger     = BasicEncodingLogger<char>;
using WideEncodingLogger = BasicEncodingLogger<wchar_t>;








// Implementation


template<class C, class T>
inline BasicEncodingLogger<C, T>::BasicEncodingLogger(log::Logger& base_logger)
    : impl::EncodingLoggerImpl<C, T>(base_logger) // Throws
    , log::BasicLogger<C, T>(base_logger.get_channel().get_limit(),
                             impl::EncodingLoggerImpl<C, T>::get_prefix(base_logger),
                             impl::EncodingLoggerImpl<C, T>::get_channel(base_logger),
                             impl::EncodingLoggerImpl<C, T>::get_channel_map(base_logger))
{
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_ENCODING_LOGGER_HPP
