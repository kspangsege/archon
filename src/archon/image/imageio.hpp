/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_IMAGE_IMAGEIO_HPP
#define ARCHON_IMAGE_IMAGEIO_HPP

#include <archon/image/file_format.hpp>


namespace archon {
namespace image {

class UnknownFormatException;
class UnresolvableFormatException;

/// Load an image from the specified stream. If an explicit format is not
/// specified, an attempt will be made to detect it automatically.
///
/// \param in The input stream that will provide the image data.
///
/// \param source_name A name (possibly a file name) that will be used to
/// identify the stream in the log and in exception messages. Also, if an
/// explicit format is not specified and this name has an extension (dot
/// something), that extension might be use in determining the input format.
///
/// \param format_name If specified, the stream data will be assumed to be in
/// this format. Otherwise it will be automatically detected.
///
/// \param l A logger through which warnings and errors pertaining to the
/// loading process will be reported.
///
/// \param t A progress tracker through which the loading/parsing progress will
/// be reported.
///
/// \param r An alternative format registry. If it is not specified, the default
/// registry will be used.
///
/// \return The loaded image.
///
/// \throw UnresolvableFormatException If an explicite format is not specified
/// and the format could not be detected from the initial image contents nor
/// from the suffix of the source name.
///
/// \throw UnknownFormatException If an explicite format is specified but does
/// not correspond to a known type.
///
/// \throw InvalidFormatException When a fatal error occurs during parsing of
/// the stream contents.
///
/// \throw core::ReadException If reading from the stream fails.
///
/// \sa FileFormat::Registry::new_registry
BufferedImage::Ref load_image(core::InputStream& in,
                              std::string source_name, std::string format_name = "",
                              core::Logger* l = &core::Logger::get_default_logger(),
                              FileFormat::ProgressTracker* t = nullptr,
                              FileFormat::Registry::ConstRefArg r =
                              core::CntRefNullTag());


/// Load an image from the specified file. If an explicit format is not
/// specified, an attempt will be made to detect the format automatically.
///
/// \param file_path The file system path of the file to read. Also, if an
/// explicit format is not specified and this path name has an extension (dot
/// something), that extension might be use in determining the input format.
///
/// \param format_name If specified, the stream data will be assumed to be in
/// this format. Otherwise it will be automatically detected.
///
/// \param l A logger through which warnings and errors pertaining to the
/// loading process will be reported.
///
/// \param t A progress tracker through which the loading/parsing progress will
/// be reported.
///
/// \param r An alternative format registry. If it is not specified, the default
/// registry will be used.
///
/// \return The loaded image.
///
/// \throw UnresolvableFormatException If an explicite format is not specified
/// and the format could not be detected from the initial image contents nor
/// from the suffix of the source name.
///
/// \throw UnknownFormatException If an explicite format is specified but does
/// not correspond to a known type.
///
/// \throw InvalidFormatException When a fatal error occurs during parsing of
/// the stream contents.
///
/// \throw core::ReadException If reading from the file fails.
///
/// \sa FileFormat::Registry::new_registry
BufferedImage::Ref load_image(std::string file_path, std::string format_name = "",
                              core::Logger* l = &core::Logger::get_default_logger(),
                              FileFormat::ProgressTracker* t = nullptr,
                              FileFormat::Registry::ConstRefArg r =
                              core::CntRefNullTag());


/// Write an image to the specified stream using the specified file format.
///
/// If an empty format name is passed the format is determined by the suffix of
/// the target name.
///
/// \param image The image whose contents is to saved.
///
/// \param out The target stream onto which the image data will be written.
///
/// \param target_name A name (possibly a file name) that will be used to
/// identify the stream in the log and in exception messages. Also, if an
/// explicit format is not specified and this name has an extension (dot
/// something), that extension will be use in determining the output format.
///
/// \param format_name If specified, the stream data will be written according
/// to that format. Otherwise an attempt wil be made to infer it from the target
/// name.
///
/// \param l A logger through which warnings and errors pertaining to the saving
/// process will be reported.
///
/// \param t Pass an instance of ProgressTracker if you need progress
/// indications. This is needed if eg. you want to display a progress bar.
///
/// \param r An alternative format registry. If it is not specified, the default
/// registry will be used.
///
/// \throw UnresolvableFormatException If an explicite format is not specified
/// and the format could not be inferred from the suffix of the target name.
///
/// \throw UnknownFormatException If an explicite format is specified but does
/// not correspond to a known type.
///
/// \throw core::WriteException If writing to the stream fails.
///
/// \sa FileFormat::Registry::new_registry
void save_image(Image::ConstRefArg image, core::OutputStream& out,
                std::string target_name, std::string format_name = "",
                core::Logger* l = &core::Logger::get_default_logger(),
                FileFormat::ProgressTracker* t = nullptr,
                FileFormat::Registry::ConstRefArg r =
                core::CntRefNullTag());


/// Write an image to the specified file using the specified file format.
///
/// If an empty format name is passed the format is determined by the suffix of
/// the file name.
///
/// \param image The image whose contents is to saved.
///
/// \param file_path The file system path of the file to write to. Also, if an
/// explicit format is not specified and this path name has an extension (dot
/// something), that extension will be used to infer the output format.
///
/// \param format_name If specified, the stream data will be written according
/// to that format. Otherwise an attempt wil be made to infer it from the file
/// name.
///
/// \param l A logger through which warnings and errors pertaining to the saving
/// process will be reported.
///
/// \param t Pass an instance of ProgressTracker if you need progress
/// indications. This is needed if eg. you want to display a progress bar.
///
/// \param r An alternative format registry. If it is not specified, the default
/// registry will be used.
///
/// \throw UnresolvableFormatException If an explicite format is not specified
/// and the format could not be inferred from the suffix of the file name.
///
/// \throw UnknownFormatException If an explicite format is specified but does
/// not correspond to a known type.
///
/// \throw core::WriteException If writing to the file fails.
///
/// \sa FileFormat::Registry::new_registry
void save_image(Image::ConstRefArg image,
                std::string file_path, std::string format_name = "",
                core::Logger* l = &core::Logger::get_default_logger(),
                FileFormat::ProgressTracker* t = nullptr,
                FileFormat::Registry::ConstRefArg r =
                core::CntRefNullTag());




// Implementation

class UnknownFormatException: public std::runtime_error {
public:
    UnknownFormatException(const std::string& message):
        std::runtime_error(message)
    {
    }
};

class UnresolvableFormatException: public std::runtime_error {
public:
    UnresolvableFormatException(const std::string& message):
        std::runtime_error(message)
    {
    }
};

} // namespace image
} // namespace archon

#endif // ARCHON_IMAGE_IMAGEIO_HPP
