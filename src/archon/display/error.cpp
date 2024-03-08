// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <string>

#include <archon/core/assert.hpp>
#include <archon/display/error.hpp>


using namespace archon;


namespace {


auto get_error_message(display::Error err) noexcept -> const char*
{
    switch (err) {
        case display::Error::display_uses_unsupported_pixel_format:
            return "Display uses unsupported pixel format";
    }
    ARCHON_ASSERT_UNREACHABLE();
    return nullptr;
}


} // unnamed namespace


auto display::impl::ErrorCategory::name() const noexcept -> const char*
{
    return "archon:display";
}


auto display::impl::ErrorCategory::message(int err) const -> std::string
{
    return std::string(::get_error_message(display::Error(err))); // Throws
}
