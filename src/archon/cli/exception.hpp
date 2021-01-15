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

#ifndef ARCHON_X_CLI_X_EXCEPTION_HPP
#define ARCHON_X_CLI_X_EXCEPTION_HPP

/// \file


#include <utility>
#include <exception>
#include <string>


namespace archon::cli::detail {


class BadCommandLineInterfaceSpec : public std::exception {
public:
    BadCommandLineInterfaceSpec(std::string message) noexcept;

    const char* what() const noexcept override;

private:
    std::string m_message;
};








// Implementation


inline BadCommandLineInterfaceSpec::BadCommandLineInterfaceSpec(std::string message) noexcept :
    m_message(std::move(message))
{
}


inline const char* BadCommandLineInterfaceSpec::what() const noexcept
{
    return m_message.c_str();
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_EXCEPTION_HPP
