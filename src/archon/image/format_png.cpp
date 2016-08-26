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

#include <cstdio>
#include <string>
#include <stdexcept>
#include <iostream>

#include <png.h>

// There is a bug in libpng which forces us to put this include below png.h
#include <csetjmp>

#include <archon/core/memory.hpp>
#include <archon/core/text.hpp>
#include <archon/core/blocking.hpp>
#include <archon/util/transcode.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/integer_buffer_format.hpp>


using namespace archon::core;
using namespace archon::util;
using namespace archon::image;


namespace {

class Context {
public:
    double total_rows = 0;
    FileFormat::ProgressTracker* tracker;
    Logger* logger; // For warnings
    std::string error_message;  // For fatal errors
    // 0 for none, 1 for InterruptException, 2 for IOException, 3 for other
    // exceptions from the stream, 4 for libpng initiated error or invalid
    // format detected.
    int error_type = 0;
    Context(FileFormat::ProgressTracker* tracker, Logger* logger):
        tracker{tracker},
        logger{logger}
    {
    }
};

class LoadContext: public Context {
public:
    InputStream& in;
    LoadContext(InputStream& i, FileFormat::ProgressTracker* tracker, Logger* logger):
        Context{tracker, logger},
        in{i}
    {
    }
};

class SaveContext: public Context {
public:
    OutputStream& out;
    SaveContext(OutputStream& o, FileFormat::ProgressTracker* tracker, Logger* logger):
        Context{tracker, logger},
        out{o}
    {
    }
};

class LoadWrapper {
public:
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;
    ~LoadWrapper() noexcept
    {
        if (png_ptr)
            png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : nullptr, nullptr);
    }
};

class SaveWrapper {
public:
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;
    ~SaveWrapper() noexcept
    {
        if (png_ptr)
            png_destroy_write_struct(&png_ptr, info_ptr ? &info_ptr : 0);
    }
};

void error_callback(png_structp png_ptr, png_const_charp message) throw()
{
    {
        Context* c = static_cast<Context*>(png_get_error_ptr(png_ptr));
        c->error_type    = 4;
        c->error_message = message;
    }
    longjmp(png_jmpbuf(png_ptr), 1);
}

void warning_callback(png_structp png_ptr, png_const_charp message) throw()
{
    Context* c = static_cast<Context*>(png_get_error_ptr(png_ptr));
    c->logger->log(message);
}

void progress_callback(png_structp png_ptr, png_uint_32 row, int) throw()
{
    Context* c = static_cast<Context*>(png_get_error_ptr(png_ptr));
//    c->logger->log("Progress " + Text::print(row) + "/" + Text::print(c->total_rows) + " (pass = " + Text::print(pass) + ")");
    if (c->tracker && c->total_rows)
        c->tracker->progress(row/c->total_rows);
}

/// \todo Verify that re-throwing works after long-jump
void read_callback(png_structp png_ptr, png_bytep data, png_size_t length) throw()
{
    LoadContext* c = static_cast<LoadContext*>(png_get_error_ptr(png_ptr));
    try {
        int n = c->in.read_all(reinterpret_cast<char*>(data), int(length));
        if (n == int(length))
            return;

        c->error_type = 4;
        c->error_message = "Premature end of PNG data";
    }
    catch (InterruptException& e) {
        c->error_type = 1;
    }
    catch (ReadException& e) {
        c->error_type = 2;
        c->error_message = e.what();
    }
    catch (std::exception& e) {
        c->error_type = 3;
        c->error_message = e.what();
    }

    longjmp(png_jmpbuf(png_ptr), 1);
}

void write_callback(png_structp png_ptr, png_bytep data, png_size_t length) throw()
{
    SaveContext* c = static_cast<SaveContext*>(png_get_error_ptr(png_ptr));
    try {
        c->out.write(reinterpret_cast<const char*>(data), int(length));
        return;
    }
    catch (InterruptException& e) {
        c->error_type = 1;
    }
    catch (WriteException& e) {
        c->error_type = 2;
        c->error_message = e.what();
    }
    catch (std::exception& e) {
        c->error_type = 3;
        c->error_message = e.what();
    }

    longjmp(png_jmpbuf(png_ptr), 1);
}

/// \todo Should there not be a flush method in OutputStream?
void flush_callback(png_structp)
{
}



/// An adaptor that allows the Archon image library to use the PNG file format
/// by using <tt><b>libpng</b></tt>.
///
/// \sa http://www.libpng.org/pub/png
///
/// Rewritten and tested against <tt><b>libpng</b></tt> version 1.2.8.
///
/// Later rewritten and tested against <tt><b>libpng</b></tt> version 1.2.29
///
/// Notes on using setjump/longjump in C++:
///
/// Great care must be taken when using setjump/longjump in C++ context. The
/// main problem is that stack objects will not have their destructors called
/// during a long jump stack unwinding process.
///
/// In this case we set up the long jump handler in a C++ function and then call
/// a number of C (not C++) functions, and it is only these C functions that can
/// potentially issue a long jump. This fact makes things a bit easier to
/// control.
///
/// We should be safe as long as we stick to the following rules:
///
/// <ol>
///
/// <li> Keep the setjump/longjump construction on the following canonical form:
///
/// <pre>
///
///   if (!setjmp(jmpbuf)) { // try
///       // code that may potentially issue a long jump (throw)
///   }
///   else { // catch
///       // code that handles the long jump (exception)
///   }
///
/// </pre>
///
/// <li> Keep the try-scope completely free of declarations of variables whos
/// type have either an explicit or an implied destructor. This also applies to
/// any sub-scope through which a long jump may pass. A sub-scope through which
/// a long jump cannot pass, may safly define stack objects with destructors.
///
/// <li> For each C function call that may lead to a long jump, make sure that
/// the surrounding expression does not involve creation of any temporary
/// objects of a type that has either an explicit or an implied destructor.
///
/// <li> If any of the C function calls that may lead to a long jump calls a C++
/// function (eg. as a call back function) then make sure that the called C++
/// function does not issue a long jump or lead to one being issued in a place
/// where stack variables with destructors are "live".
///
/// <li> Never long jump out of a C++ try block or catch block.
///
/// </ul>
class FormatPng: public FileFormat {
public:
    std::string get_name() const override
    {
        return "png";
    }

    bool check_signature(InputStream& in) const override
    {
        char header[8];
        return in.read_all(header, 8) == 8 &&
            !png_sig_cmp(reinterpret_cast<png_bytep>(header), 0, 8);
    }

    bool check_suffix(std::string s) const override
    {
        return (s == "png");
    }

    /// \todo FIXME: Read and sometimes transcode comments from image file.
    ///
    /// \todo FIXME: Read the comment.
    BufferedImage::Ref load(InputStream& in, Logger* logger,
                            ProgressTracker* tracker) const override
    {
        char header[8];
        if (in.read_all(header, 8) != 8 || png_sig_cmp(reinterpret_cast<png_bytep>(header), 0, 8))
            throw InvalidFormatException("Not a PNG header");

        LoadContext c(in, tracker, logger);
        LoadWrapper w;

        w.png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, &c,
                                           error_callback, warning_callback);
        if (!w.png_ptr)
            throw std::runtime_error("PNG library was unable to allocate "
                                     "a 'png_struct' structure");

        w.info_ptr = png_create_info_struct(w.png_ptr);
        if (!w.info_ptr)
            throw std::runtime_error("PNG library was unable to allocate "
                                     "first 'png_info' structure");

        png_set_read_fn(w.png_ptr, 0, read_callback);
        png_set_read_status_fn(w.png_ptr, progress_callback);

        ColorSpace::ConstRef color_space;
        IntegerBufferFormat::ChannelLayout channels;
        IntegerBufferFormat::ConstRef buf_fmt;
        BufferedImage::Ref image;
        std::unique_ptr<png_byte*[]> row_pointers;

        // Handle termination and catch errors occuring in any of the following
        // PNG-functions.
        if (!setjmp(png_jmpbuf(w.png_ptr))) { // try
            // CAUTION: No stack variables involving destructors may be defined
            // in this scope, or any sub-scope where a longjump might pass
            // through.

            png_set_sig_bytes(w.png_ptr, 8);
            png_read_info(w.png_ptr, w.info_ptr);

            // Determine channel bit depth, and color space, and ask for palette
            // formats to be converted automatically to RGB.
            png_byte bit_depth;
            png_byte color_type = png_get_color_type(w.png_ptr, w.info_ptr);
            if (color_type == PNG_COLOR_TYPE_PALETTE) {
                png_set_palette_to_rgb(w.png_ptr);
                // This is fixed by the specification
                bit_depth = 8;
            }
            else {
                bit_depth = png_get_bit_depth(w.png_ptr, w.info_ptr);
            }

            // Now, choose a suitable word type, channel pitch and bit order
            WordType word_type;
            int channel_pitch;
            bool most_sig_bit_first = false;
            int bits_per_byte = std::numeric_limits<unsigned char>::digits;
            if (bit_depth < 8) {
                // If the channel bit depth is 1, 2, or 4 then there is only one
                // channel per pixel and Libpng packs multiple pixels into one
                // byte, however if there is more than 8 bits per byte on this
                // platform, then there will be unused bits in the buffer and we
                // do not have a fixed pixel bit stride. Since such a buffer
                // format is not currently supported by the Archon image wrapper
                // we must ask Libpng to unpack the data such that we get one
                // pixel per byte. This does not cause channel values to be
                // scaled by Libpng.
                if (bits_per_byte != 8) {
                    png_set_packing(w.png_ptr);
                    word_type = word_type_UChar;
                    channel_pitch = bits_per_byte;
                }
                else {
                    // In this case we just mimic the raw packed format used by
                    // Libpng
                    word_type = word_type_UChar;
                    channel_pitch = bit_depth;
                    most_sig_bit_first = true; // Libpng uses most significat bits first
                }
            }
            else if (bit_depth == 8) {
                // We can represent this directly - no conversions needed.
                word_type = word_type_UChar;
                channel_pitch = bits_per_byte;
            }
            else { // bit_depth = 16
                // We can only handle 16 bit channels if the byte is 8 bits
                // wide, otherwise we would have unused bits in the middel, and
                // the Archon wrapper cannot handle that, so in that case we
                // must ask Libpng to convert the channels to 8 bits.
                if (bits_per_byte != 8) {
                    png_set_strip_16(w.png_ptr);
                    word_type = word_type_UChar;
                    bit_depth = 8;
                    channel_pitch = bits_per_byte;
                }
                else { // 8-bit bytes
                    // If a 'short' is exactly twice as wide as a byte, it will
                    // be attractive to use it as the word type. If it was
                    // wider, we will prefer using chars instead.
                    if (sizeof (short) == 2) {
                        word_type = word_type_UShort;
                        channel_pitch = 16;
                        // For perfromance reasons we prefer to use the native
                        // endianness for the stored short's. Libpng store
                        // 16-bit data in the big-endian way by default
                        // (independant of platform endianness), so if this
                        // platform is little-endian, we will ask Libpng to swap
                        // bytes.
                        if (!native_endianness[0])
                            png_set_swap(w.png_ptr);
                    }
                    else {
                        // So we will use bytes instead, but then the Archon
                        // library must combine the bytes, which means we will
                        // have to consider bit ordering. It is the bit ordering
                        // property of the pixel format that determines the
                        // order in which words are combined.
                        word_type = word_type_UChar;
                        channel_pitch = 16;
                        most_sig_bit_first = true;
                    }
                }
            }

            // Automatically convert tRNS chunk into ordinary an alpha channel.
            if (png_get_valid(w.png_ptr, w.info_ptr, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(w.png_ptr);

            png_read_update_info(w.png_ptr, w.info_ptr); // Update header information

            color_type = png_get_color_type(w.png_ptr, w.info_ptr); // Re-read
            bool has_alpha = false;
            switch (color_type) {
                case PNG_COLOR_TYPE_GRAY_ALPHA: // possible bit depths 8, 16
                    has_alpha = true;
                case PNG_COLOR_TYPE_GRAY:       // possible bit depths 1, 2, 4, 8, 16
                    color_space = ColorSpace::get_Lum();
                    break;
                case PNG_COLOR_TYPE_RGB_ALPHA:  // possible bit_depths 8, 16
                    has_alpha = true;
                case PNG_COLOR_TYPE_RGB:        // possible bit_depths 8, 16
                    color_space = ColorSpace::get_RGB();
                    break;
                default:
                    throw std::runtime_error("Unexpected color space for PNG data "
                                             "("+Text::print(static_cast<int>(color_type))+")");
            }

            // We now create the buffer format where channels are always evenly
            // spaced in terms of number of buffer bits.
            int num_channels = color_space->get_num_primaries() + (has_alpha?1:0);
            channels.bits_per_pixel = num_channels * channel_pitch;
            for (int i = 0; i < num_channels; ++i)
                channels.add(IntegerBufferFormat::Channel(i*channel_pitch, bit_depth));
            // It seems that due to the fact that each row has an address,
            // Libpng must do byte aligned strips for packed formats
            bool word_align_strip = true;

            buf_fmt = IntegerBufferFormat::get_format(word_type, channels,
                                                      most_sig_bit_first, word_align_strip);

            int width  = png_get_image_width (w.png_ptr, w.info_ptr);
            int height = png_get_image_height(w.png_ptr, w.info_ptr);

            // Sanity check - it would be a grave error if Archon and Libpng
            // disagreed on bufer size
            int bytes_per_strip = buf_fmt->get_bytes_per_strip(width);
            int bytes_per_strip2 = png_get_rowbytes(w.png_ptr, w.info_ptr);
            if (bytes_per_strip != bytes_per_strip2)
                throw std::runtime_error("Mismatching bytes per strip reported by "
                                         "Archon("+Text::print(bytes_per_strip)+") and Libpng"
                                         "("+Text::print(bytes_per_strip2)+")");

//std::cerr << "---- LOAD PNG: "<<width<<"x"<<height<<" "<<buf_fmt->print(color_space, has_alpha)<<"\n";

            // Construct an image with an uninitialized pixel buffer
            image = BufferedImage::new_image(width, height, color_space, has_alpha, buf_fmt);
            if (tracker) {
                image->clear();
                tracker->defined(image);
            }

            // Make row array
            png_byte* pix_buf = reinterpret_cast<png_byte*>(image->get_buffer_ptr());
            row_pointers = std::make_unique<png_byte*[]>(height); // Throws
            for (int i = 0; i < height; ++i)
                row_pointers[i] = pix_buf + (height-i-1)*bytes_per_strip;

            c.total_rows = height; // For the progress tracker

            png_read_image(w.png_ptr, row_pointers.get());
            png_read_end(w.png_ptr, w.info_ptr);

/*
            png_textp text_ptr = nullptr;
            int num_text = 0;
            if (png_get_text(w.png_ptr, w.info_ptr, &text_ptr, &num_text) > 0) {
                for (int i = 0; i < num_text; ++i) {
                    // ...
                }
            }
*/

            // PNG structures are freed by ~LoadWrapper
        }
        else { // catch
            switch (c.error_type) {
                case 1:
                    throw InterruptException();
                case 2:
                    throw ReadException(c.error_message);
                case 3:
                    throw std::runtime_error(c.error_message);
            }
            throw InvalidFormatException(c.error_message);
        }

        return image;
    }


    /// \todo FIXME: Verify the correctness of comment compression.
    ///
    /// \todo FIXME: Try to prevent the buffer copy step. This can be done if
    /// the incoming image is buffered and the pixel format is compatible with
    /// Libpng.
    void save(Image::ConstRefArg image, OutputStream& out, Logger* logger,
              ProgressTracker* tracker) const override
    {
        // The idea is to first construct a pixel format that is supported by
        // Libpng, and at the same time, as closely as possible, matches the
        // pixel format of the incoming image, preferrably without loosing
        // precision. Then, if the constructed format turns out to be memory
        // compatible with the one used by the incoming image, we can use its
        // buffer directly, otherwise we need to copy the input image into a new
        // buffered image of the desired format.

        ColorSpace::ConstRef orig_color_space = image->get_color_space();
        ColorSpace::ConstRef color_space =
            orig_color_space->get_num_primaries() == 1 ? ColorSpace::get_Lum() : ColorSpace::get_RGB();
        bool has_alpha = image->has_alpha_channel();

        // Choose a channel width that is supported by Libpng
        int channel_width = image->get_channel_width();
        if(channel_width <= 1) {
            channel_width = 1;
        }
        else if(channel_width <= 2) {
            channel_width = 2;
        }
        else if(channel_width <= 4) {
            channel_width = 4;
        }
        else if(channel_width <= 8) {
            channel_width = 8;
        }
        else {
            channel_width = 16;
        }

        WordType word_type;
        int channel_pitch;
        bool most_sig_bit_first = false;
        bool request_packing = false;
        bool request_byte_swap = false;
        int bits_per_byte = std::numeric_limits<unsigned char>::digits;
        if (channel_width < 8) {
            // In this case we would like to pack multiple pixels into a byte,
            // however that is only possible if a byte has 8 bits, since
            // otherwise it would require skipped bits after some but not all
            // pixels, and this is not supported by the Archon image library. So
            // if a byte is not 8 bits wide, we must use one byte per pixel and
            // instruct Libpng to pack them for us.
            if (bits_per_byte == 8) {
                word_type = word_type_UChar;
                channel_pitch = channel_width;
                most_sig_bit_first = true; // The native mode for Libpng
            }
            else {
                word_type = word_type_UChar;
                channel_pitch = bits_per_byte;
                request_packing = true;
            }
        }
        else if (channel_width == 8) {
            word_type = word_type_UChar;
            channel_pitch = bits_per_byte;
        }
        else { // channel_width == 16
            // We can only handle 16 bit channels if a byte on this platform has
            // exactly 8 bits, and that is because Libpng expects to read it as
            // two bytes with 8 bits in each. If a byte has more that 8 bits, it
            // measn there are unused bits in the middel, and that is not
            // supported by the Archin image library. So in that case, we must
            // choose an 8 bit format instead.
            if (bits_per_byte != 8) {
                channel_width = 8;
                word_type = word_type_UChar;
                channel_pitch = bits_per_byte;
            }
            else { // 8-bit bytes
                // In this case we can use a word type of 'short', however that
                // is only attractive if sizeof(short) == 2, if not, we will use
                // chars.
                if (sizeof (short) == 2) {
                    word_type = word_type_UShort;
                    channel_pitch = 16;
                    // Libpng expects 16-bit data in big-endian format by
                    // default, so if this platform is little-endian we will ask
                    // Libpng to swap bytes.
                    if (!native_endianness[0])
                        request_byte_swap = true;
                }
                else {
                    word_type = word_type_UChar;
                    channel_pitch = 16;
                    // Libpng expects 16-bit data in big-endian form, however,
                    // by default, the Archon image library will store the least
                    // significant word at the lowest address (least significant
                    // bits first), so we need to switch to MSB mode.
                    most_sig_bit_first = true;
                }
            }
        }

        // We now create the buffer format where channels are always evenly
        // spaced in terms of number of buffer bits.
        int num_channels = color_space->get_num_primaries() + (has_alpha?1:0);
        IntegerBufferFormat::ChannelLayout channels;
        channels.bits_per_pixel = num_channels * channel_pitch;
        for (int i = 0; i < num_channels; ++i)
            channels.add(IntegerBufferFormat::Channel(i*channel_pitch, channel_width));
        // It seems that due to the fact that each row has an address, Libpng
        // must do byte aligned strips for packed formats
        bool word_align_strip = true;

        IntegerBufferFormat::ConstRef buf_fmt =
            IntegerBufferFormat::get_format(word_type, channels, most_sig_bit_first, word_align_strip);

        int width  = image->get_width();
        int height = image->get_height();
        int bytes_per_strip = buf_fmt->get_bits_per_strip(width)/bits_per_byte;

        BufferedImage::ConstRef buf_img;
        if (const BufferedImage* i = dynamic_cast<const BufferedImage*>(image.get())) {
            if (orig_color_space == color_space && i->has_equiv_buffer_format(buf_fmt))
                buf_img.reset(i);
        }
        if (!buf_img) {
            BufferedImage::Ref i =
                BufferedImage::new_image(width, height, color_space, has_alpha, buf_fmt);
            i->put_image(image, 0, 0, false); // Copy / transcode, no blending
            buf_img = i;
        }

//std::cerr << "---- SAVE PNG: "<<width<<"x"<<height<<" "<<buf_fmt->print(color_space, has_alpha)<<" "<<
//  (buf_img == image ? "Can use incoming buffer" : "Can not use incoming buffer")<<"\n";

        png_byte color_type;
        switch (num_channels) {
            case 1:
                color_type = PNG_COLOR_TYPE_GRAY;
                break;
            case 2:
                color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
                break;
            case 3:
                color_type = PNG_COLOR_TYPE_RGB;
                break;
            case 4:
                color_type = PNG_COLOR_TYPE_RGB_ALPHA;
                break;
            default:
                throw std::runtime_error("Unexpected number of channels " +
                                         Text::print(num_channels));
        };

        const png_byte* pix_buf =
            reinterpret_cast<const png_byte*>(buf_img->get_buffer_ptr());
        std::unique_ptr<const png_byte*[]> row_pointers =
            std::make_unique<const png_byte*[]>(height); // Throws
        for (int i = 0; i < height; i++)
            row_pointers[i] = pix_buf + (height-i-1)*bytes_per_strip;


        SaveContext c(out, tracker, logger);
        SaveWrapper w;

        c.total_rows = height; // For the progress tracker

        w.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &c,
                                            error_callback, warning_callback);
        if (!w.png_ptr)
            throw std::runtime_error("PNG library was unable to allocate "
                                     "a 'png_struct' structure");

        w.info_ptr = png_create_info_struct(w.png_ptr);
        if (!w.info_ptr)
            throw std::runtime_error("PNG library was unable to allocate "
                                     "first 'png_info' structure");

        png_set_write_fn(w.png_ptr, 0, write_callback, flush_callback);
        png_set_write_status_fn(w.png_ptr, progress_callback);

        // Text to be inserted into image
        std::string software = "The Archon image library";
        png_text text;
        if (software.empty()) {
            text.text = nullptr;
        }
        else {
            // Get rid of NUL characters in the comment. The PNG spec does not
            // allow them.
            std::string::size_type i = 0;
            for (;;) {
                auto p = software.find('\0');
                if (p == std::string::npos)
                    break;
                software.replace(i, 1, "\xEF\xBF\xBD"); // Unicode replacement character
            }

#ifdef PNG_iTXt_SUPPORTED
            text.text_length = 0;
            text.itxt_length = software.size();
            text.lang        = 0;
            text.lang_key    = 0;
            text.compression = (software.size() < 1000 ? PNG_ITXT_COMPRESSION_NONE :
                                PNG_ITXT_COMPRESSION_zTXt);
#else
            software = transcode(software, transcode_UTF_8, transcode_ISO_8859_1);
            text.text_length = software.size();
            text.compression = (software.size() < 1000 ? PNG_TEXT_COMPRESSION_NONE :
                                PNG_TEXT_COMPRESSION_zTXt);
#endif
            text.key  = const_cast<char*>("Software");
            text.text = const_cast<char*>(software.data());
        }

        save(c, w, width, height, channel_width, color_type, request_packing, request_byte_swap,
             row_pointers.get(), text); // Throws
    }

private:
    void save(const SaveContext& c, const SaveWrapper& w, int width, int height, int channel_width,
              png_byte color_type, bool request_packing, bool request_byte_swap,
              const png_byte* const* row_pointers, png_text& text) const
    {
        // Handle termination and catch errors occuring in any of the following
        // PNG-functions.
        if (!setjmp(png_jmpbuf(w.png_ptr))) { // try
            // CAUTION: No stack variables involving destructors may be defined
            // in this scope, or any sub-scope where a longjump might pass
            // through.

            png_set_IHDR(w.png_ptr, w.info_ptr, width, height,
                         channel_width, color_type, PNG_INTERLACE_NONE,
                         PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

            png_write_info(w.png_ptr, w.info_ptr);

            // Set PNG write transformations
            if (request_packing)
                png_set_packing(w.png_ptr);
            if (request_byte_swap)
                png_set_swap(w.png_ptr);

            png_write_image(w.png_ptr, const_cast<png_byte**>(row_pointers));

            // Add the comment after the pixel data
            if (text.text)
                png_set_text(w.png_ptr, w.info_ptr, &text, 1);

            png_write_end(w.png_ptr, w.info_ptr);
        }
        else { // catch
            switch (c.error_type) {
                case 1:
                    throw InterruptException();
                case 2:
                    throw WriteException(c.error_message);
                case 3:
                    throw std::runtime_error(c.error_message);
            }
            throw InvalidFormatException(c.error_message);
        }
    }
};

} // unnamed namespace


namespace archon {
namespace image {

FileFormat::ConstRef get_default_png_file_format()
{
    static FileFormat::ConstRef f(new FormatPng());
    return f;
}

} // namespace image
} // namespace archon
