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

#ifndef ARCHON_X_CLI_X_COMMAND_LINE_HPP
#define ARCHON_X_CLI_X_COMMAND_LINE_HPP

/// \file


#include <string_view>
#include <stdexcept>
#include <vector>

#include <archon/base/features.h>
#include <archon/base/seed_memory_output_stream.hpp>


namespace archon::cli {


/// \brief 
///
/// 
///
template<class C, class T = std::char_traits<C>> class BasicCommandLine {
public:
    BasicCommandLine(int argc, const char* const argv[]);
    BasicCommandLine(int argc, const char* const argv[], std::string_view argv0_override,
                     bool argv0_strip_dir = false);

private:
    struct PendingErrors;
    struct Parent;

    const char* const* m_args_begin;
    const char* const* m_args_end;
    const Parent* m_parent = nullptr;

    // The values of `m_argv0` and `m_argv0_strip_dir` have meaning only if
    // `parent` is null.
    std::string_view m_argv0;
    bool m_argv0_strip_dir = false;

    CommandLine(const char* const* args_begin, const char* const* args_end,
                const Parent&) noexcept;

    // FIXME: Just use appropriate getters instead of friendship     
    template<class, class> friend class BasicSpec;                
};








// Implementation


// The number of errors is the number of entries in `ends`. The message
// associated with the error at index `i` is a range of characters in
// `out.view()`. If `i` is zero, then the message begins at the beginning of
// `out.view()`. Otherwise it begins before the character referred to by the
// index in `ends[i-1]`. It always ends before the character referred to by
// `ends[i]`.
//
struct CommandLine::PendingErrors {
    base::SeedMemoryOutputStream out; // FIXME: What aout locale?                                                                                               
    std::vector<std::size_t> ends;
};

struct CommandLine::Parent {
    PendingErrors& errors;
    const CommandLine& command_line;
    std::string_view pattern;
};


inline CommandLine::CommandLine(int argc, const char* const argv[])
{
    if (ARCHON_UNLIKELY(argc < 1))
        throw std::invalid_argument("Too few arguments (argc < 1)");
    m_args_begin = argv + 1;
    m_args_end   = argv + argc;
    m_argv0      = argv[0];
    m_argv0_strip_dir = true;
}


inline CommandLine::CommandLine(int argc, const char* const argv[],
                                std::string_view argv0_override, bool argv0_strip_dir) :
    CommandLine(argc, argv) // Throws
{
    m_argv0 = argv0_override;
    m_argv0_strip_dir = argv0_strip_dir;
}


inline CommandLine::CommandLine(const char* const* args_begin, const char* const* args_end,
                                const Parent& parent) noexcept
{
    m_args_begin = args_begin;
    m_args_end   = args_end;
    m_parent     = &parent;
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_COMMAND_LINE_HPP
