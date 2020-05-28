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


#include <archon/core/assert.hpp>

#include <archon/core/misc_error.hpp>


using namespace archon;


namespace {


auto get_error_message(core::MiscError err) noexcept -> const char*
{
    switch (err) {
        case core::MiscError::other:
            return "Unknow type of error";
        case core::MiscError::operation_not_supported:
            return "Operation not supported";
        case core::MiscError::premature_end_of_input:
            return "Premature end of input";
        case core::MiscError::delim_not_found:
            return "Delimiter not found";
    }
    ARCHON_ASSERT_UNREACHABLE();
    return nullptr;
}


} // unnamed namespace



auto core::impl::MiscErrorCategory::name() const noexcept -> const char*
{
    return "archon:core:misc";
}


auto core::impl::MiscErrorCategory::message(int err) const -> std::string
{
    return std::string(::get_error_message(core::MiscError(err))); // Throws
}
