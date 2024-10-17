// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <archon/image/error.hpp>


using namespace archon;


namespace {


auto get_error_message(image::Error err) noexcept -> const char*
{
    switch (err) {
        case image::Error::file_format_unavailable:
            return "Attempted use of unavailable file format";
        case image::Error::image_size_out_of_range:
            return "Image size out of range";
        case image::Error::unsupported_image_parameter:
            return "Unsupported image parameter during load or save";
        case image::Error::no_such_file_format:
            return "Invalid image file format identifier";
        case image::Error::file_format_detection_failed:
            return "Image file format could not be detected";
        case image::Error::bad_file:
            return "Invalid file contents or wrong file format";
        case image::Error::loading_process_failed:
            return "Image loading process failed";
        case image::Error::saving_process_failed:
            return "Image saving process failed";
    }
    ARCHON_ASSERT_UNREACHABLE();
    return nullptr;
}


} // unnamed namespace


auto image::impl::ErrorCategory::name() const noexcept -> const char*
{
    return "archon:image";
}


auto image::impl::ErrorCategory::message(int err) const -> std::string
{
    return std::string(::get_error_message(image::Error(err))); // Throws
}
