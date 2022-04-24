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


#include <utility>
#include <optional>
#include <string>
#include <locale>
#include <system_error>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/buffer.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/file.hpp>
#include <archon/core/file_sink.hpp>
#include <archon/log/logger.hpp>
#include <archon/image/image.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>
#include <archon/image/output.hpp>
#include <archon/image/error.hpp>
#include <archon/image/save.hpp>


using namespace archon;


namespace {


auto detect_file_format(std::string_view mime_type, std::string_view filename_extension,
                        const image::SaveConfig& config, std::error_code& ec) -> const image::FileFormat*
{
    const image::FileFormatRegistry* registry = config.registry;
    if (ARCHON_LIKELY(!registry))
        registry = &image::FileFormatRegistry::get_default_registry(); // Throws
    if (ARCHON_LIKELY(config.file_format.empty())) {
        if (!mime_type.empty()) {
            const image::FileFormat* format = registry->lookup_by_mime_type(mime_type); // Throws
            if (ARCHON_LIKELY(format))
                return format;
        }
        if (!filename_extension.empty()) {
            const image::FileFormat* format = registry->lookup_by_extension(filename_extension); // Throws
            if (ARCHON_LIKELY(format))
                return format;
        }
        ec = image::Error::file_format_detection_failed;
        return nullptr;
    }
    const image::FileFormat* format = registry->lookup(config.file_format);
    if (ARCHON_LIKELY(format))
        return format;
    ec = image::Error::file_format_unavailable;
    return nullptr;
}


} // unnamed namespace


bool image::try_save(const image::Image& image, core::FilesystemPathRef path, const std::locale& loc,
                     const image::SaveConfig& config, std::error_code& ec)
{
    namespace fs = std::filesystem;
    fs::path ext = path.get().extension(); // Throws
    std::string ext_2 = core::path_to_string_generic(ext, loc); // Throws
    std::string_view mime_type; // Unspecified
    const image::FileFormat* format = detect_file_format(mime_type, ext_2, config, ec); // Throws
    if (ARCHON_LIKELY(format)) {
        core::File file;
        if (ARCHON_LIKELY(file.try_open(path, core::File::Mode::write, ec))) { // Throws
            log::Logger& logger = log::Logger::or_null(config.logger);
            if (!config.write_buffer.empty()) {
                core::BufferedFileSink sink(file, config.write_buffer);
                return (format->try_save(image, sink, loc, logger, config, ec) && sink.try_flush(ec)); // Throws
            }
            if (config.write_buffer_size > 0) {
                core::Buffer<char> buffer(config.write_buffer_size); // Throws
                core::BufferedFileSink sink(file, buffer);
                return (format->try_save(image, sink, loc, logger, config, ec) && sink.try_flush(ec)); // Throws
            }
            core::FileSink sink(file);
            return format->try_save(image, sink, loc, logger, config, ec); // Throws
        }
    }
    return false; // Failure
}


bool image::try_save_a(const image::Image& image, const image::Output& output, const std::locale& loc,
                       const image::SaveConfig& config, std::error_code& ec)
{
    const image::FileFormat* format = detect_file_format(output.mime_type, output.filename_extension, config,
                                                         ec); // Throws
    if (ARCHON_LIKELY(format)) {
        log::Logger& logger = log::Logger::or_null(config.logger);
        return format->try_save(image, output.sink, loc, logger, config, ec); // Throws
    }
    return false; // Failure
}
