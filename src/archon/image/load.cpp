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
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>
#include <archon/image/input.hpp>
#include <archon/image/error.hpp>
#include <archon/image/load.hpp>


using namespace archon;


bool image::try_load(core::FilesystemPathRef path, std::unique_ptr<image::WritableImage>& image,
                     const std::locale& loc, const image::LoadConfig& config, std::error_code& ec)
{
    core::File file;
    if (ARCHON_LIKELY(file.try_open(path, core::File::Mode::read, ec))) { // Throws
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
    return false;
}


bool image::try_load_a(const image::Input& input, std::unique_ptr<image::WritableImage>& image,
                       const std::locale& loc, const image::LoadConfig& config, std::error_code& ec)
{
    const image::FileFormatRegistry* registry = config.registry;
    if (ARCHON_LIKELY(!registry))
        registry = &image::FileFormatRegistry::get_default_registry(); // Throws
    log::Logger& logger = log::Logger::or_null(config.logger);
    if (ARCHON_LIKELY(config.file_format.empty())) {
        const image::FileFormat* format = nullptr;
        core::ArraySeededBuffer<char, 256> buffer;
        core::RewindableSource source(input.source, buffer);
        core::FlatSet<const image::FileFormat*, 4> checked_formats;
        if (!input.mime_type.empty()) {
            core::ArraySeededBuffer<const image::FileFormat*, 4> buffer_2;
            core::BufferContents tray(buffer_2);
            registry->lookup_by_mime_type(input.mime_type, tray); // Throws
            for (const image::FileFormat* format_2 : tray) {
                if (ARCHON_LIKELY(!checked_formats.contains(format_2))) {
                    bool success = format_2->recognize(source); // Throws
                    source.rewind();
                    if (success) {
                        format = format_2;
                        goto format_found;
                    }
                }
            }
            checked_formats.insert(tray.begin(), tray.end()); // Throws
        }
        if (!input.filename_extension.empty()) {
            core::ArraySeededBuffer<const image::FileFormat*, 4> buffer_2;
            core::BufferContents tray(buffer_2);
            registry->lookup_by_extension(input.filename_extension, tray); // Throws
            for (const image::FileFormat* format_2 : tray) {
                if (ARCHON_LIKELY(!checked_formats.contains(format_2))) {
                    bool success = format_2->recognize(source); // Throws
                    source.rewind();
                    if (success) {
                        format = format_2;
                        goto format_found;
                    }
                }
            }
            checked_formats.insert(tray.begin(), tray.end()); // Throws
        }
        {
            int n = registry->get_num_file_formats();
            for (int i = 0; i < n; ++i) {
                const image::FileFormat& format_2 = registry->get_file_format(i); // Throws
                if (ARCHON_LIKELY(!checked_formats.contains(&format_2))) {
                    bool success = format_2.recognize(source); // Throws
                    source.rewind();
                    if (success) {
                        format = &format_2;
                        goto format_found;
                    }
                }
            }
        }
        ec = image::Error::file_format_detection_failed;
        return false;
      format_found:
        source.release();
        return format->try_load(source, image, loc, logger, config, ec); // Throws
    }
    const image::FileFormat* format = registry->lookup(config.file_format);
    if (ARCHON_LIKELY(format))
        return format->try_load(input.source, image, loc, logger, config, ec); // Throws
    ec = image::Error::file_format_unavailable;
    return false;
}
