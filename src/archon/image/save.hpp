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

#ifndef ARCHON_X_IMAGE_X_SAVE_HPP
#define ARCHON_X_IMAGE_X_SAVE_HPP

/// \file


#include <string_view>
#include <system_error>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/filesystem.hpp>
#include <archon/image/image.hpp>
#include <archon/image/save_config.hpp>
#include <archon/image/output.hpp>


namespace archon::image {


/// \{
///
/// \brief Save image to file.
///
/// These functions are shorthands for calling \ref try_save() and throwing an
/// `std::system_error` on failure.
///
/// The overload, where the path is specified as a string view, constructs a proper
/// filesytem path object using \ref core::make_fs_path_generic().
///
void save(const image::Image& image, std::string_view path, const std::locale&, const image::SaveConfig& = {});
void save(const image::Image& image, core::FilesystemPathRef path, const std::locale&, const image::SaveConfig& = {});
/// \}



/// \{
///
/// \brief Try to save image to file.
///
/// These functions attempt to save the specified image to the specified file (\p path), or
/// output stream (\p output).
///
/// On success, these functions return `true` and leave \p ec unchanged.
///
/// On failure, these functions return `false` after seting \p ec to an error code that
/// reflects the cause of the failure (see file format detection below).
///
///
/// #### File format detection scheme
///
/// If a particular file format is specified through \p config (\ref
/// image::SaveConfig::file_format), and such a file format exists in the registry (\ref
/// image::SaveConfig::registry), that file format is used. If it does not exists in the
/// registry, file format detection fails with error \ref
/// image::Error::file_format_unavailable.
///
/// Otherwise, if a MIME type is specified through the \p output argument, and there are
/// file formats associated with that MIME type in the registry, the first registered of
/// those file formats is used.
///
/// Otherwise, if a filename extension is specified through the \p path or the \p output
/// argument, and there are file formats associated with that filename extension in the
/// registry, the first registered of those file formats is used.
///
/// Otherwise, file format detection fails with error \ref
/// image::Error::file_format_detection_failed.
///
///
/// #### Buffered write to file
///
/// Unless disabled, `try_save()` will write to the specified file in a buffered
/// manner. Specifically, if \ref image::SaveConfig::write_buffer is nonempty, that buffer
/// will be used as a write buffer. Otherwise, if image::SaveConfig::write_buffer_size is
/// nonzero, a write buffer of that size will be created and used. Otherwise, writing will
/// be unbuffered.
///
/// `try_save_a()` completely ignores \ref image::SaveConfig::write_buffer_size and \ref
/// image::SaveConfig::write_buffer.
///
///
/// \sa \ref image::try_load()
///
bool try_save(const image::Image& image, core::FilesystemPathRef path, const std::locale&,
              const image::SaveConfig& config, std::error_code& ec);
bool try_save_a(const image::Image& image, const image::Output& output, const std::locale&,
                const image::SaveConfig& config, std::error_code& ec);
/// \}








// Implementation


inline void save(const image::Image& image, std::string_view path, const std::locale& loc,
                 const image::SaveConfig& config)
{
    namespace fs = std::filesystem;
    fs::path path_2 = core::make_fs_path_generic(path, loc); // Throws
    image::save(image, path_2, loc, config); // Throws
}


inline void save(const image::Image& image, core::FilesystemPathRef path, const std::locale& loc,
                 const image::SaveConfig& config)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_save(image, path, loc, config, ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to save image");
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_SAVE_HPP
