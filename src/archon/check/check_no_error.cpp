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


#include <array>

#include <archon/core/string_formatter.hpp>

#include <archon/check/check_no_error.hpp>


using namespace archon;
namespace impl = check::impl;


void impl::check_no_error_failed(check::TestContext& test_context, check::Location location,
                                 std::string_view macro_name, std::string_view invoc_text, std::string_view ec_text,
                                 std::error_code ec)
{
    std::array<char, 1024> seed_memory;
    core::StringFormatter formatter(seed_memory, test_context.locale); // Throws
    std::string_view message = formatter.format("%s(%s, %s) failed with %s: %s", macro_name, invoc_text, ec_text, ec,
                                                ec.message()); // Throws
    test_context.check_failed(location, message); // Throws
}
