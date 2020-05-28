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

#ifndef ARCHON_X_CLI_X_PROCESS_HPP
#define ARCHON_X_CLI_X_PROCESS_HPP

/// \file


#include <utility>
#include <locale>

#include <archon/cli/string_holder.hpp>
#include <archon/cli/config.hpp>
#include <archon/cli/command_line.hpp>
#include <archon/cli/spec.hpp>
#include <archon/cli/processor.hpp>


namespace archon::cli {


/// \{
///
/// \brief    
///
///     
///
template<class C, class T> bool process(int argc, const char* const argv[], const cli::BasicSpec<C, T>&,
                                        int& exit_status, cli::BasicConfig<C, T> = {});
template<class C, class T> bool process(int argc, const char* const argv[], const cli::BasicSpec<C, T>&,
                                        int& exit_status, const std::locale&, cli::BasicConfig<C, T> = {});
template<class C, class T> bool process(int argc, const char* const argv[], const cli::BasicSpec<C, T>&,
                                        int& exit_status, cli::BasicStringHolder<C, T>&);
template<class C, class T> bool process(int argc, const char* const argv[], const cli::BasicSpec<C, T>&,
                                        int& exit_status, cli::BasicStringHolder<C, T>&, const std::locale&);
template<class C, class T> int process(int argc, const char* const argv[], const cli::BasicSpec<C, T>&,
                                       cli::BasicConfig<C, T> = {});
template<class C, class T> int process(int argc, const char* const argv[], const cli::BasicSpec<C, T>&,
                                       const std::locale&, cli::BasicConfig<C, T> = {});
template<class C, class T> bool process(const cli::BasicCommandLine<C, T>&, const cli::BasicSpec<C, T>&,
                                        int& exit_status);
template<class C, class T> int process(const cli::BasicCommandLine<C, T>&, const cli::BasicSpec<C, T>&);
/// \}








// Implementation


template<class C, class T>
inline bool process(int argc, const char* const argv[], const cli::BasicSpec<C, T>& spec, int& exit_status,
                    cli::BasicConfig<C, T> config)
{
    std::locale locale;
    return cli::process(argc, argv, spec, exit_status, locale, std::move(config)); // Throws
}


template<class C, class T>
bool process(int argc, const char* const argv[], const cli::BasicSpec<C, T>& spec, int& exit_status,
             const std::locale& locale, cli::BasicConfig<C, T> config)
{
    cli::BasicCommandLine<C, T> command_line(argc, argv, locale, std::move(config)); // Throws
    return cli::process(command_line, spec, exit_status); // Throws
}


template<class C, class T>
inline bool process(int argc, const char* const argv[], const cli::BasicSpec<C, T>& spec, int& exit_status,
                    cli::BasicStringHolder<C, T>& string_holder)
{
    std::locale locale;
    return cli::process(argc, argv, spec, exit_status, string_holder, locale); // Throws
}


template<class C, class T>
inline bool process(int argc, const char* const argv[], const cli::BasicSpec<C, T>& spec, int& exit_status,
                    cli::BasicStringHolder<C, T>& string_holder, const std::locale& locale)
{
    cli::BasicConfig<C, T> config;
    config.string_holder = &string_holder;
    return cli::process(argc, argv, spec, exit_status, locale, std::move(config)); // Throws
}


template<class C, class T>
inline int process(int argc, const char* const argv[], const cli::BasicSpec<C, T>& spec, cli::BasicConfig<C, T> config)
{
    std::locale locale;
    return cli::process(argc, argv, spec, locale, std::move(config)); // Throws
}


template<class C, class T>
inline int process(int argc, const char* const argv[], const cli::BasicSpec<C, T>& spec, const std::locale& locale,
                   cli::BasicConfig<C, T> config)
{
    int exit_status = EXIT_SUCCESS;
    cli::process(argc, argv, spec, exit_status, locale, std::move(config)); // Throws
    return exit_status;
}


template<class C, class T>
inline bool process(const cli::BasicCommandLine<C, T>& command_line, const cli::BasicSpec<C, T>& spec,
                    int& exit_status)
{
    cli::BasicProcessor<C, T> proc(command_line, spec); // Throws
    return proc.process(exit_status); // Throws
}


template<class C, class T>
inline int process(const cli::BasicCommandLine<C, T>& command_line, const cli::BasicSpec<C, T>& spec)
{
    int exit_status = EXIT_SUCCESS;
    cli::process(command_line, spec, exit_status); // Throws
    return exit_status;
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_PROCESS_HPP
