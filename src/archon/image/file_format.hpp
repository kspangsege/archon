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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_IMAGE_FILE_FORMAT_HPP
#define ARCHON_IMAGE_FILE_FORMAT_HPP

#include <stdexcept>
#include <string>

#include <archon/core/logger.hpp>
#include <archon/core/refcnt.hpp>
#include <archon/core/stream.hpp>
#include <archon/image/buffered_image.hpp>


namespace archon {
namespace image {

/**
 * An abstract image file format codec. All methods must be
 * individually and mutually thread safe.
 */
class FileFormat: public virtual core::CntRefObjectBase, public core::CntRefDefs<FileFormat> {
public:
    /**
     * Thrown when invalid data is encountered while reading a file
     * or a stream.
     */
    struct InvalidFormatException;

    /**
     * Must return the unique specifier for this image format. The
     * specifier is the sub-field of the \c image MIME type. That
     * is, the part of the MIME type after \c "image/". This will
     * normally correspond to the file name suffix, but not
     * always.
     *
     * \return The name of this format.
     */
    virtual std::string get_name() const = 0;

    /**
     * Check if the initial bytes from the specified stream
     * identifies the stream contents as being of this
     * format. Reading from the stream should cease as soon as this
     * can be established.
     *
     * \param in The stream from which the image data header can be
     * read.
     *
     * \return True iff the format of the data on the stream can be
     * identified as the format represented by this format object.
     *
     * \thows core::ReadException If reading from the stream
     * fails.
     */
    virtual bool check_signature(core::InputStream& in) const = 0;

    /**
     * Check if the passed suffix is a proper file name suffix for
     * this format.
     *
     * \param suffix The file name suffix to check not including the
     * dot. It must always be passed in lower case.
     *
     * \return True iff the specified file name suffix is
     * appropriate for this format.
     */
    virtual bool check_suffix(std::string suffix) const = 0;

    /**
     * Can be used with \c FileFormat::load and \c FileFormat::save
     * methods to track the progress of either. The \c load and \c
     * save methods are required only to make one call to \c
     * progress at the end of the process. The \c load method is
     * also required to make a call to \c defined sometime before
     * the first call to \c progress, however it may choose to do
     * this only at the end of the process.
     */
    class ProgressTracker {
    public:
        /**
         * Called by the \c FileFormat::load method when the image is
         * defined. Ideally this would be as soon as possible, ai. as
         * soon as the file header has been parsed, but this is not
         * required.
         *
         * \param image This must be the same object as is later
         * returned by the \c load method. Any parts of the image that
         * have not yet been loaded must have been cleared either to
         * transparent, black, or the background color specified by
         * the image file/stream. This method will not be called by
         * the \c save method.
         */
        virtual void defined(BufferedImage::ConstRefArg image) throw();

        /**
         * Called one or more times during the load/save process to
         * indicate progress. It must be called at least once at the
         * end of the process.
         *
         * \param fraction An indication of the progress. It must be a
         * value between 0 and 1. At the final call a value of 1 must
         * be passed for the \c fraction argument.
         */
	virtual void progress(double fraction) throw() = 0;

        virtual ~ProgressTracker() {}
    };

    /**
     * Create a buffered image whose initial size and contents is
     * determined by the contents of the specified stream.
     *
     * \param in The stream from which the image data can be read.
     *
     * \param l A logger through which warnings and errors
     * pertaining to the loading process will be reported.
     *
     * \param t A progress tracker through which the loading/parsing
     * progress will be reported.
     *
     * \return The loaded image.
     *
     * \throws InvalidFormatException If the stream contents could
     * not be validly decoded.
     *
     * \thows core::ReadException If reading from the stream
     * fails.
     */
    virtual BufferedImage::Ref load(core::InputStream& in, core::Logger* l =
                                    &core::Logger::get_default_logger(),
                                    ProgressTracker* t = 0) const = 0;

    /**
     * Convert the data in the specified image according to this
     * file format and write the result to the specified stream.
     *
     * \param image The image source whose data is to be converted.
     *
     * \param out The target stream to which the image data will be
     * written.
     *
     * \param l A logger through which warnings and errors
     * pertaining to the saving process will be reported.
     *
     * \param t A progress tracker through which the saving
     * progress will be reported.
     *
     * \thows core::WriteException If writing to the stream fails.
     */
    virtual void save(Image::ConstRefArg image,
                      core::OutputStream& out,
                      core::Logger* l =
                      &core::Logger::get_default_logger(),
                      ProgressTracker* t = 0) const = 0;



    /**
     * A container for a set of file formats allowing you to lookup
     * a format by name. A registry is used by \c ImageIO.load as a
     * basis for auto-detecting the format of the file or stream.
     *
     * With the exception of \c register_format, all the methods must
     * be individually and mutually thread safe. \c register_format
     * on the other hand must not be considered thread safe, and
     * it is illegal for any method to execute concurrently with \c
     * register_format. For this reason it is advised that \c
     * register_format is only used immediately after creation of a
     * new registry and before any other use of the registry.
     */
    class Registry: public virtual core::CntRefObjectBase, public core::CntRefDefs<Registry> {
    public:
        /**
         * Get the default registry. This registry will contain all
         * the standard formats that are available on this platform.
         *
         * \return The default registry.
         */
        static ConstRef get_default_registry();

        /**
         * Create a new registry initially containing the same formats
         * as the specified registry. If no registry is specified, the
         * new registry will be empty.
         *
         * \param base The registry to base the new one on.
         *
         * \return The new registry.
         *
         * \sa register_format
         */
        static Ref new_registry(Registry::ConstRefArg base = core::CntRefNullTag());

        /**
         * Get the number of formats currently in this registry.
         *
         * \return The number of formats known by this registry.
         */
        virtual int get_num_formats() const = 0;

        /**
         * Get the format at the specified index. The order of formats
         * is the order in which they ere registered.
         *
         * \param index The position of the desired format.
         *
         * \return The format at the specified index.
         *
         * \sa get_num_formats
         */
        virtual FileFormat::ConstRef get_format(int index) const = 0;

        /**
         * Lookup a format by name.
         *
         * \param name the name of the desired format.
         *
         * \return Null if there is no format with the specified name.
         */
        virtual FileFormat::ConstRef lookup(std::string name) const = 0;

        /**
         * Add a new format to this registry.
         *
         * \param format The format to be registered. It must hav a
         * name that is unique with respect to all previously
         * registered formats.
         *
         * \throw std::invalid_argument If the name of new format is
         * the same as a previously registered format.
         *
         * \note This method is not thread safe and may not execute
         * concurrently with any method in this class.
         */
        virtual void register_format(FileFormat::ConstRefArg format) = 0;
    };
};




// Implementation

class FileFormat::InvalidFormatException: public std::runtime_error {
public:
    InvalidFormatException(std::string m):
        std::runtime_error(m)
    {
    }
};


inline void FileFormat::ProgressTracker::defined(BufferedImage::ConstRefArg) throw()
{
}

} // namespace image
} // namespace archon

#endif // ARCHON_IMAGE_FILE_FORMAT_HPP
