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

#ifndef ARCHON_X_IMAGE_X_SAVE_CONFIG_HPP
#define ARCHON_X_IMAGE_X_SAVE_CONFIG_HPP

/// \file


#include <cstddef>
#include <string_view>

#include <archon/core/span.hpp>
#include <archon/log/logger.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>


namespace archon::image {


/// \brief Configuration of image saving process.
///
/// An object of this type is used to specify parameters that control the image saving
/// process as it is invoked through \ref image::save().
///
/// Note that some of the available parameters are inherited from \ref
/// image::FileFormat::SaveConfig.
///
struct SaveConfig : image::FileFormat::SaveConfig {
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

    /// \brief Write buffer size.
    ///
    /// Size of write buffer to be created by \ref image::save() if \ref write_buffer is
    /// empty.
    ///
    /// This parameter is ignored by \ref image::try_save_a().
    ///
    std::size_t write_buffer_size = 8192;

    /// \brief Alternative write buffer.
    ///
    /// If nonempty, this buffer will be used as a write buffer by \ref image::save().
    ///
    /// This parameter is ignored by \ref image::try_save_a().
    ///
    core::Span<char> write_buffer;
};


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_SAVE_CONFIG_HPP
