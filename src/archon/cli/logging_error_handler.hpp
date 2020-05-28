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

#ifndef ARCHON_X_CLI_X_LOGGING_ERROR_HANDLER_HPP
#define ARCHON_X_CLI_X_LOGGING_ERROR_HANDLER_HPP

/// \file


#include <cstddef>
#include <algorithm>
#include <string_view>

#include <archon/core/span.hpp>
#include <archon/log/logger.hpp>
#include <archon/cli/error_handler.hpp>


namespace archon::cli {


template<class C, class T = std::char_traits<C>> class BasicLoggingErrorHandler
    : public cli::BasicErrorHandler<C, T> {
public:
    using logger_type = log::BasicLogger<C, T>;

    using ErrorEntry = typename cli::BasicErrorHandler<C, T>::ErrorEntry;

    /// \brief Construct logging error handler.
    ///
    /// If N is the value of \p max_errors, then, if more than N errors are passed to \ref
    /// handle(), only the first N errors will be logged.
    ///
    BasicLoggingErrorHandler(logger_type&, std::size_t max_errors = 8) noexcept;

    void handle(core::Span<const ErrorEntry>, int&) override;

private:
    logger_type& m_logger;
    std::size_t m_max_errors;
};


using LoggingErrorHandler     = BasicLoggingErrorHandler<char>;
using WideLoggingErrorHandler = BasicLoggingErrorHandler<wchar_t>;








// Implementation


template<class C, class T>
inline BasicLoggingErrorHandler<C, T>::BasicLoggingErrorHandler(logger_type& logger, std::size_t max_errors) noexcept
    : m_logger(logger)
    , m_max_errors(max_errors)
{
}


template<class C, class T>
void BasicLoggingErrorHandler<C, T>::handle(core::Span<const ErrorEntry> errors, int&)
{
    std::size_t n = std::min(errors.size(), m_max_errors);
    for (const ErrorEntry& entry : errors.subspan(0, n))
        m_logger.error("%s", entry.error_message); // Throws
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_LOGGING_ERROR_HANDLER_HPP
