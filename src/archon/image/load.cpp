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


#include <memory>
#include <utility>
#include <system_error>
#include <string>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/buffer.hpp>
#include <archon/core/buffer_contents.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/flat_set.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/rewindable_source.hpp>
#include <archon/core/file.hpp>
#include <archon/core/file_source.hpp>
#include <archon/log/logger.hpp>
#include <archon/image/image.hpp>
#include <archon/image/error.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>
#include <archon/image/input.hpp>
#include <archon/image/load.hpp>


using namespace archon;


namespace {


bool determinae_file_format(core::RewindableSource& source, std::string_view mime_type,
                            std::string_view filename_extension, const std::locale& loc, log::Logger& logger,
                            const image::LoadConfig& config, const image::FileFormat*& format, std::error_code& ec)
{
    const image::FileFormatRegistry* registry = config.registry;
    if (ARCHON_LIKELY(!registry))
        registry = &image::FileFormatRegistry::get_default_registry(); // Throws

    if (ARCHON_UNLIKELY(config.file_format.has_value())) {
        const image::FileFormat* format_2 = registry->lookup(config.file_format.value());
        if (ARCHON_UNLIKELY(!format_2)) {
            ec = image::Error::no_such_file_format;
            return false; // Failure
        }
        format = format_2;
        return true; // Success
    }

    core::FlatSet<const image::FileFormat*, 4> checked_formats;

    // If MIME type is specified and matches available and recognizing file format
    core::ArraySeededBuffer<const image::FileFormat*, 4> buffer_1;
    core::BufferContents tray_1(buffer_1);
    if (!mime_type.empty()) {
        registry->lookup_by_mime_type(mime_type, tray_1); // Throws
        for (const image::FileFormat* format_2 : tray_1) {
            if (ARCHON_UNLIKELY(checked_formats.contains(format_2)))
                continue;
            if (ARCHON_UNLIKELY(!format_2->is_available()))
                continue;
            bool recognized = false;
            if (ARCHON_UNLIKELY(!format_2->try_recognize(source, recognized, loc, logger, ec))) // Throws
                return false;  // Failure
            source.rewind();
            if (ARCHON_UNLIKELY(!recognized))
                continue;
            format = format_2;
            return true;  // Success
        }
        checked_formats.insert(tray_1.begin(), tray_1.end()); // Throws
    }

    // If filename extension is specified and matches available and recognizing file format
    core::ArraySeededBuffer<const image::FileFormat*, 4> buffer_2;
    core::BufferContents tray_2(buffer_2);
    if (!filename_extension.empty()) {
        registry->lookup_by_extension(filename_extension, tray_2); // Throws
        for (const image::FileFormat* format_2 : tray_2) {
            if (ARCHON_UNLIKELY(checked_formats.contains(format_2)))
                continue;
            if (ARCHON_UNLIKELY(!format_2->is_available()))
                continue;
            bool recognized = false;
            if (ARCHON_UNLIKELY(!format_2->try_recognize(source, recognized, loc, logger, ec))) // Throws
                return false;  // Failure
            source.rewind();
            if (ARCHON_UNLIKELY(!recognized))
                continue;
            format = format_2;
            return true;  // Success
        }
        checked_formats.insert(tray_2.begin(), tray_2.end()); // Throws
    }

    // If any available file format recognizing file contents
    int n = registry->get_num_file_formats();
    for (int i = 0; i < n; ++i) {
        const image::FileFormat& format_2 = registry->get_file_format(i); // Throws
        if (ARCHON_UNLIKELY(checked_formats.contains(&format_2)))
            continue;
        if (ARCHON_UNLIKELY(!format_2.is_available()))
            continue;
        bool recognized = false;
        if (ARCHON_UNLIKELY(!format_2.try_recognize(source, recognized, loc, logger, ec))) // Throws
            return false;  // Failure
        source.rewind();
        if (ARCHON_UNLIKELY(!recognized))
            continue;
        format = &format_2;
        return true;  // Success
    }

    // If MIME type was specified and did match an available file format
    for (const image::FileFormat* format_2 : tray_1) {
        if (ARCHON_LIKELY(format_2->is_available())) {
            format = format_2;
            return true;  // Success
        }
    }

    // If filename extension was specified and did match an available file format
    for (const image::FileFormat* format_2 : tray_2) {
        if (ARCHON_LIKELY(format_2->is_available())) {
            format = format_2;
            return true;  // Success
        }
    }

    // If MIME type was specified and did match any file format
    if (!tray_1.empty()) {
        format = tray_1.front();
        return true;  // Success
    }

    // If filename extension was specified and did match any file format
    if (!tray_2.empty()) {
        format = tray_2.front();
        return true;  // Success
    }

    ec = image::Error::file_format_detection_failed;
    return false; // Failure
}


} // unnamed namespace


bool image::try_load(core::FilesystemPathRef path, std::unique_ptr<image::WritableImage>& image,
                     const std::locale& loc, const image::LoadConfig& config, std::error_code& ec)
{
    core::File file;
    if (ARCHON_UNLIKELY(!file.try_open(path, core::File::Mode::read, ec))) // Throws
        return false; // Failure
    namespace fs = std::filesystem;
    fs::path ext = path.get().extension(); // Throws
    std::string ext_2 = core::path_to_string_generic(ext, loc); // Throws
    if (!config.read_buffer.empty()) {
        core::BufferedFileSource source(file, config.read_buffer);
        image::Input input(source);
        input.filename_extension = ext_2;
        return image::try_load_a(input, image, loc, config, ec); // Throws
    }
    if (config.read_buffer_size > 0) {
        core::Buffer<char> buffer(config.read_buffer_size); // Throws
        core::BufferedFileSource source(file, buffer);
        image::Input input(source);
        input.filename_extension = ext_2;
        return image::try_load_a(input, image, loc, config, ec); // Throws
    }
    core::FileSource source(file);
    image::Input input(source);
    input.filename_extension = ext_2;
    return image::try_load_a(input, image, loc, config, ec); // Throws
}


bool image::try_load_a(const image::Input& input, std::unique_ptr<image::WritableImage>& image,
                       const std::locale& loc, const image::LoadConfig& config, std::error_code& ec)
{
    core::ArraySeededBuffer<char, 256> buffer;
    core::RewindableSource source(input.source, buffer);
    log::Logger& logger = log::Logger::or_null(config.logger);
    const image::FileFormat* format = {};
    if (ARCHON_UNLIKELY(!determinae_file_format(source, input.mime_type, input.filename_extension, loc, logger, config,
                                                format, ec))) // Throws
        return false; // Failure
    source.release();
    return format->try_load(source, image, loc, logger, config, ec); // Throws
}
