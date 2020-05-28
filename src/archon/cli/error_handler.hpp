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

#ifndef ARCHON_X_CLI_X_ERROR_HANDLER_HPP
#define ARCHON_X_CLI_X_ERROR_HANDLER_HPP

/// \file


#include <cstddef>
#include <string_view>

#include <archon/core/span.hpp>
#include <archon/cli/proc_error.hpp>


namespace archon::cli {


/// \brief Base class for handlers of errors that occur during command line processing.
///
/// An error handler of this type can be passed to the command line processor via \ref
/// cli::BasicConfig::error_handler. This allows the application to handle errors that occur
/// during processing of the command line.
///
template<class C, class T = std::char_traits<C>> class BasicErrorHandler {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    struct ErrorEntry;

    virtual void handle(core::Span<const ErrorEntry> errors, int& exit_status) = 0;

    virtual ~BasicErrorHandler() noexcept = default;
};


using ErrorHandler     = BasicErrorHandler<char>;
using WideErrorHandler = BasicErrorHandler<wchar_t>;


template<class C, class T> struct BasicErrorHandler<C, T>::ErrorEntry {
    /// \brief Index of error-causing command-line argument.
    ///
    /// The index of the command-line argument (within `argv` as passed to `main()`) that
    /// caused this error, or one plus the index of the last command-line argument (i.e,
    /// `argc`) if the error was "too few command-line arguments".
    ///
    /// Because zero refers to the command name/path (`argv[0]`), all errors will specify a
    /// command-line argument index greater than, or equal to 1 in practice.
    ///
    std::size_t arg_index;

    cli::ProcError error_code;

    string_view_type error_message;
};


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_ERROR_HANDLER_HPP
