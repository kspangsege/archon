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

#ifndef ARCHON_X_CLI_X_CONFIG_HPP
#define ARCHON_X_CLI_X_CONFIG_HPP

/// \file


#include <cstddef>
#include <string_view>

#include <archon/cli/string_holder.hpp>
#include <archon/cli/error_handler.hpp>


namespace archon::cli {


/// \brief Parameters controlling operation of commandline processor.
///
/// These parameters allow for some control over the operation of the commandline processor.
///
template<class C, class T = std::char_traits<C>> struct BasicConfig {
    using char_type   = C;
    using traits_type = T;

    using string_holder_type = cli::BasicStringHolder<C, T>;
    using error_handler_type = cli::BasicErrorHandler<C, T>;

    /// \brief 
    ///
    /// 
    ///
    string_holder_type* string_holder = nullptr;

    /// \brief Custom error handler.
    ///
    /// A custom handler for command-line processing errors can be specified here.
    ///
    /// If no error handler is specified, an instance of \ref cli::LoggingErrorHandler will
    /// be used. It will log to STDERR, and have `max_errors` set to its default value.
    ///
    /// The errors passed to the handler will occur in order of increasing argument index
    /// (\ref cli::ErrorHandler::ErrorEntry::arg_index).
    ///
    error_handler_type* error_handler = nullptr;

    /// \brief 
    ///
    /// FIXME: Explain: Applies to values shown in error messages produced by the
    /// command-line processor.
    ///
    std::size_t show_arg_max_size = 16;
};


using Config     = BasicConfig<char>;
using WideConfig = BasicConfig<wchar_t>;


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_CONFIG_HPP
