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
#include <optional>
#include <string_view>

#include <archon/core/span.hpp>
#include <archon/log/logger.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>


namespace archon::image {


/// \brief Configuration of image loading process.
///
/// An object of this type is used to specify parameters that control the image loading
/// process as it is invoked through \ref image::load(), \ref image::try_load(), and \ref
/// image::try_load_a().
///
/// Note that some of the available parameters are inherited from \ref
/// image::FileFormat::LoadConfig.
///
/// \sa \ref image::load(), \ref image::try_load(), \ref image::try_load_a()
/// \sa \ref image::SaveConfig
///
struct LoadConfig : image::FileFormat::LoadConfig {
    /// \brief Log through specified logger.
    ///
    /// If no logger is specified, nothing is logged during the loading process. If a logger
    /// is specified, it must use a locale that is compatible with the locale that is passed
    /// to \ref image::load(), \ref image::try_load(), or \ref image::try_load_a(). The
    /// important thing is that the character encodings agree (`std::codecvt` facet).
    ///
    log::Logger* logger = nullptr;

    /// \brief Assume specific file format for loaded image.
    ///
    /// If specified, assume that the loaded image uses that particular file format. It is
    /// specified when it is nonempty. In that case, it is taken to be the file format
    /// identifier (\ref image::FileFormat::get_ident()) for one of the file formats in the
    /// registry (\ref registry). If it is not a valid file format identifier, the loading
    /// process fails with \ref image::Error::no_such_file_format.
    ///
    /// When not explicitly specified, an attempt will be made to automatically detect the
    /// image file format. See \ref image::try_load() for details on the file format
    /// detection scheme.
    ///
    std::optional<std::string_view> file_format;

    /// \brief Alternative set of file formats to be used during file format detection.
    ///
    /// If a file format registry is specified, that set of image file formats will be
    /// considered during file format detection. See \ref image::try_load() for details on
    /// the file format detection scheme. If a file format registry is not specified, the
    /// default one will be used (\ref image::FileFormatRegistry::get_default_registry()).
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
