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

#ifndef ARCHON_X_IMAGE_X_LOAD_HPP
#define ARCHON_X_IMAGE_X_LOAD_HPP

/// \file


#include <memory>
#include <string_view>
#include <system_error>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/filesystem.hpp>
#include <archon/image/writable_image.hpp>
#include <archon/image/load_config.hpp>
#include <archon/image/input.hpp>


namespace archon::image {


/// \{
///
/// \brief Load image from file.
///
/// These functions are shorthands for calling \ref try_load() and, on success, returning
/// the loaded image, or, on failure, throwing an `std::system_error`.
///
/// The overload, where the path is specified as a string view, constructs a proper
/// filesystem path object using \ref core::make_fs_path_generic().
///
/// \sa \ref image::save()
/// \sa \ref image::try_load()
///
auto load(std::string_view path, const std::locale&, const image::LoadConfig& = {}) ->
    std::unique_ptr<image::WritableImage>;
auto load(core::FilesystemPathRef path, const std::locale&, const image::LoadConfig& = {}) ->
    std::unique_ptr<image::WritableImage>;
/// \}



/// \{
///
/// \brief Try to load image from file.
///
/// This function attempts to load an image from the specified file (\p path), or input
/// stream (\p input).
///
/// On success, this function returns `true` after assigning the loaded image to \p
/// image. \p ec is left unchanged in this case.
///
/// On failure, this function returns `false` after setting \p ec to an error code that
/// reflects the cause of the failure (see file format detection below). \p image is left
/// unchanged in this case.
///
///
/// #### File format detection scheme
///
/// If a particular file format is specified through \p config (\ref
/// image::LoadConfig::file_format), and such a file format exists in the registry (\ref
/// image::LoadConfig::registry), that file format is used. If it does not exists in the
/// registry, file format detection fails with error \ref image::Error::no_such_file_format.
///
/// Otherwise, if a MIME type is specified through the \p input argument (\ref
/// image::Input::mime_type), and it matches any available file formats in the registry
/// (\ref image::FileFormat::is_available()), and if any of those file formats recognize the
/// file's contents (\ref image::FileFormat::try_recognize()), use the one that occurs first
/// in the registry.
///
/// Otherwise, if a filename extension is specified through the \p path, or through the \p
/// input argument (\ref image::Input::filename_extension), and it matches any available
/// file formats in the registry, and if any of those file formats recognize the file's
/// contents, use the one that occurs first in the registry.
///
/// Otherwise, if there are any file formats in the registry that recognize the file's
/// contents, use the one that occurs first.
///
/// Otherwise, if a MIME type was specified and did match any available file formats in the
/// registry, use the one that occurs first. It will not recognize the file contents, but
/// use it anyway.
///
/// Otherwise, if a filename extension was specified and did match any available file
/// formats in the registry, use the one that occurs first. It will not recognize the file
/// contents, but use it anyway.
///
/// Otherwise, if a MIME type was specified and did match any file formats in the registry,
/// use the one that occurs first. It will be unavailable, but use it anyway.
///
/// Otherwise, if a filename extension was specified and did match any file formats in the
/// registry, use the one that occurs first. It will be unavailable, but use it anyway.
///
/// Otherwise, fail with \ref image::Error::file_format_detection_failed.
///
///
/// #### Buffered read from file
///
/// Unless disabled, `try_load()` will read from the specified file in a buffered
/// manner. Specifically, if \ref image::LoadConfig::read_buffer is nonempty, that buffer
/// will be used as a read buffer. Otherwise, if image::LoadConfig::read_buffer_size is
/// nonzero, a read buffer of that size will be created and used. Otherwise, reading will be
/// unbuffered.
///
/// `try_load_a()` completely ignores \ref image::LoadConfig::read_buffer_size and \ref
/// image::LoadConfig::read_buffer.
///
///
/// \sa \ref image::load()
/// \sa \ref image::try_save()
/// \sa \ref image::LoadConfig
///
bool try_load(core::FilesystemPathRef path, std::unique_ptr<image::WritableImage>& image, const std::locale&,
              const image::LoadConfig& config, std::error_code& ec);
bool try_load_a(const image::Input& input, std::unique_ptr<image::WritableImage>& image, const std::locale&,
                const image::LoadConfig& config, std::error_code& ec);
/// \}








// Implementation


inline auto load(std::string_view path, const std::locale& loc,
                 const image::LoadConfig& config) -> std::unique_ptr<image::WritableImage>
{
    namespace fs = std::filesystem;
    fs::path path_2 = core::make_fs_path_generic(path, loc); // Throws
    return image::load(path_2, loc, config); // Throws
}


inline auto load(core::FilesystemPathRef path, const std::locale& loc,
                 const image::LoadConfig& config) -> std::unique_ptr<image::WritableImage>
{
    std::unique_ptr<image::WritableImage> image;
    std::error_code ec;
    if (ARCHON_LIKELY(try_load(path, image, loc, config, ec))) // Throws
        return image; // Success
    throw std::system_error(ec, "Failed to load image");
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_LOAD_HPP
