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

#ifndef ARCHON_X_IMAGE_X_LOAD_CONFIG_HPP
#define ARCHON_X_IMAGE_X_LOAD_CONFIG_HPP

/// \file


#include <cstddef>
#include <string_view>

#include <archon/core/span.hpp>
#include <archon/log/logger.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>


namespace archon::image {


/// \brief Configuration of image loading process.
///
/// An object of this type is used to specify parameters that control the image loading
/// process as it is invoked through \ref image::load().
///
/// Note that some of the available parameters are inherited from \ref
/// image::FileFormat::LoadConfig.
///
struct LoadConfig : image::FileFormat::LoadConfig {
    /// \brief  
    ///
    ///  
    ///
    std::string_view file_format;

    /// \brief  
    ///
    ///  
    ///
    log::Logger* logger = nullptr;

    /// \brief  
    ///
    ///  
    ///
    const image::FileFormatRegistry* registry = nullptr;

    /// \brief Read buffer size.
    ///
    /// Size of read buffer to be created by \ref image::load() if \ref read_buffer is
    /// empty.
    ///
    /// This parameter is ignored by \ref image::try_load_a().
    ///
    std::size_t read_buffer_size = 8192;

    /// \brief Alternative read buffer.
    ///
    /// If nonempty, this buffer will be used as a read buffer by \ref image::load().
    ///
    /// This parameter is ignored by \ref image::try_load_a().
    ///
    core::Span<char> read_buffer;
};


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_LOAD_CONFIG_HPP
