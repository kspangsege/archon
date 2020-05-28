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

#ifndef ARCHON_X_CLI_X_EXCEPTION_HPP
#define ARCHON_X_CLI_X_EXCEPTION_HPP

/// \file


#include <utility>
#include <exception>
#include <string>

#include <archon/cli/spec_error.hpp>
#include <archon/cli/help_spec_error.hpp>


namespace archon::cli {


/// \brief Thrown on bad command line processing specification.
///
/// An exception of this type is thrown if an invalid command line interface specification
/// (\ref cli::BasicSpec) is passed to the command line processor constructor (\ref
/// cli::BasicProcessor).
///
class BadSpec
    : public std::exception {
public:
    BadSpec(cli::SpecError, std::string message) noexcept;

    auto error_code() const noexcept -> cli::SpecError;

    auto what() const noexcept -> const char* override;

private:
    cli::SpecError m_error_code;
    std::string m_message;
};




/// \brief Thrown when parameter expansion fails on option description.
///
/// An exception of this type is thrown by cli::BasicProcessor::show_help() if parameter
/// expansion fails on an option description. See also \ref cli::HelpSpecError.
///
class BadHelpSpec
    : public std::exception {
public:
    BadHelpSpec(cli::HelpSpecError, std::string message) noexcept;

    auto error_code() const noexcept -> cli::HelpSpecError;

    auto what() const noexcept -> const char* override;

private:
    cli::HelpSpecError m_error_code;
    std::string m_message;
};








// Implementation


inline BadSpec::BadSpec(cli::SpecError error_code, std::string message) noexcept :
    m_error_code(error_code),
    m_message(std::move(message))
{
}


inline auto BadSpec::error_code() const noexcept -> cli::SpecError
{
    return m_error_code;
}


inline auto BadSpec::what() const noexcept -> const char*
{
    return m_message.c_str();
}


inline BadHelpSpec::BadHelpSpec(cli::HelpSpecError error_code, std::string message) noexcept :
    m_error_code(error_code),
    m_message(std::move(message))
{
}


inline auto BadHelpSpec::error_code() const noexcept -> cli::HelpSpecError
{
    return m_error_code;
}


inline auto BadHelpSpec::what() const noexcept -> const char*
{
    return m_message.c_str();
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_EXCEPTION_HPP
