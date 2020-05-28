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

#ifndef ARCHON_X_CLI_X_COMMAND_LINE_HPP
#define ARCHON_X_CLI_X_COMMAND_LINE_HPP

/// \file


#include <cstddef>
#include <array>
#include <memory>
#include <utility>
#include <string_view>
#include <string>
#include <locale>

#include <archon/core/span.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/cli/config.hpp>
#include <archon/cli/impl/root_state.hpp>
#include <archon/cli/impl/error_accum.hpp>
#include <archon/cli/processor_fwd.hpp>
#include <archon/cli/impl/option_invocation.hpp>


namespace archon::cli {


/// \brief 
///
/// FIXME: Explain that on the Windows platform, a newline character in a command-line
/// argument (in `argv`) is assumed to be represented as `\n\r`.                 
///
template<class C, class T = std::char_traits<C>> class BasicCommandLine {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    using Config = cli::BasicConfig<C, T>;

    BasicCommandLine(int argc, const char* const argv[], Config = {});
    BasicCommandLine(int argc, const char* const argv[], const std::locale&, Config = {});
    BasicCommandLine(int argc, const char* const argv[], const char* argv0_override, const std::locale&, Config = {});
    BasicCommandLine(int argc, const char* const argv[], string_view_type argv0_override, const std::locale&,
                     Config = {});

private:
    using string_type            = std::basic_string<C, T>;
    using root_state_type        = impl::RootState<C, T>;
    using option_invocation_type = impl::OptionInvocation<C, T>;
    using error_accum_type       = impl::ErrorAccum<C, T>;

    struct Parent;

    // INVARIANT: m_parent is null when, and only when m_root_state_owner is not
    // null.
    const std::unique_ptr<root_state_type> m_root_state_owner;
    const Parent* const m_parent = nullptr;
    const root_state_type& m_root_state;
    const std::size_t m_args_offset;

    BasicCommandLine(const std::locale&, Config);

    BasicCommandLine(const Parent&, std::size_t args_offset) noexcept;

    friend class cli::BasicProcessor<C, T>;
};


using CommandLine     = BasicCommandLine<char>;
using WideCommandLine = BasicCommandLine<wchar_t>;








// Implementation


template<class C, class T>
struct BasicCommandLine<C, T>::Parent {
    const BasicCommandLine& command_line;
    core::Span<const option_invocation_type> option_invocations;
    const error_accum_type& error_accum;
    bool has_error;
    string_view_type pattern;
};


template<class C, class T>
inline BasicCommandLine<C, T>::BasicCommandLine(int argc, const char* const argv[], Config config)
    : BasicCommandLine(argc, argv, {}, std::move(config)) // Throws
{
}


template<class C, class T>
inline BasicCommandLine<C, T>::BasicCommandLine(int argc, const char* const argv[], const std::locale& locale,
                                                Config config)
    : BasicCommandLine(locale, std::move(config)) // Throws
{
    m_root_state_owner->set_args(argc, argv); // Throws
}


template<class C, class T>
inline BasicCommandLine<C, T>::BasicCommandLine(int argc, const char* const argv[], const char* argv0_override,
                                                const std::locale& locale, Config config)
    : BasicCommandLine(argc, argv, locale, std::move(config)) // Throws
{
    std::array<C, 64> seed_memory;
    core::BasicStringWidener widener(locale, seed_memory); // Throws
    m_root_state_owner->argv0_override = string_type(widener.widen(argv0_override)); // Throws
}


template<class C, class T>
inline BasicCommandLine<C, T>::BasicCommandLine(int argc, const char* const argv[], string_view_type argv0_override,
                                                const std::locale& locale, Config config)
    : BasicCommandLine(argc, argv, locale, std::move(config)) // Throws
{
    m_root_state_owner->argv0_override = string_type(argv0_override); // Throws
}


template<class C, class T>
inline BasicCommandLine<C, T>::BasicCommandLine(const std::locale& locale, Config config)
    : m_root_state_owner(std::make_unique<root_state_type>(locale, std::move(config))) // Throws
    , m_root_state(*m_root_state_owner)
    , m_args_offset(1)
{
}


template<class C, class T>
inline BasicCommandLine<C, T>::BasicCommandLine(const Parent& parent, std::size_t args_offset) noexcept
    : m_parent(&parent)
    , m_root_state(parent.command_line.m_root_state)
    , m_args_offset(args_offset)
{
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_COMMAND_LINE_HPP
