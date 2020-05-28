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

#ifndef ARCHON_X_CHECK_X_CHECK_NO_ERROR_HPP
#define ARCHON_X_CHECK_X_CHECK_NO_ERROR_HPP

/// \file


#include <string_view>
#include <system_error>

#include <archon/check/test_details.hpp>
#include <archon/check/test_context.hpp>


/// \brief Check success of invocation with error code.
///
/// If a function returns true on success and false on failure, and if it reports an error
/// code on failure through an error code reference argument, this macro can be used to
/// check that an invocation of that function is successful. When doing so, the error code
/// will be revealed on failure.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
#define ARCHON_CHECK_NO_ERROR(invoc, ec) X_ARCHON_CHECK_NO_ERROR(invoc, ec)








// Implementation


#define X_ARCHON_CHECK_NO_ERROR(invoc, ec)                              \
    archon::check::impl::check_no_error(bool(invoc), test_context, __FILE__, __LINE__, #invoc, #ec, ec)


namespace archon::check::impl {


void check_no_error_failed(TestContext&, Location, std::string_view macro_name, std::string_view invoc_text,
                           std::string_view ec_text, std::error_code);


inline bool check_no_error(bool success, TestContext& test_context, const char* file_path, long line_number,
                           const char* invoc_text, const char* ec_text, std::error_code ec)
{
    if (ARCHON_LIKELY(success)) {
        test_context.check_succeeded();
        return true;
    }
    Location location = { file_path, line_number };
    check_no_error_failed(test_context, location, "ARCHON_CHECK_NO_ERROR", invoc_text, ec_text, ec); // Throws
    return false;
}


} // namespace archon::check::impl


#endif // ARCHON_X_CHECK_X_CHECK_NO_ERROR_HPP
