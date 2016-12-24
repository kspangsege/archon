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

#include <stdexcept>
#include <limits>
#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include <archon/core/memory.hpp>
#include <archon/core/text.hpp>
#include <archon/util/codec.hpp>
#include <archon/util/transcode.hpp>
#include <archon/util/compress.hpp>
#include <archon/util/ticker.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/tuple_grid.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/integer_buffer_format.hpp>


using namespace archon::core;
using namespace archon::util;
using namespace archon::image;

namespace {

constexpr std::size_t g_max_comment_size = 16384;

/// Assemble two bytes into a 16-bit unsigned integer with the least significant
/// byte first. Each byte must have a value no greater than 255 and the result
/// must fit in an <tt>int</tt>.
inline int make_word(char* b)
{
    unsigned char c = b[0], d = b[1];
    if (int_less_than(255, c) || int_less_than(255, d))
        throw FileFormat::InvalidFormatException("Byte out of "
                                                 "range in GIF stream");
    unsigned v = d << 8 | c;
    if (unsigned(std::numeric_limits<int>::max()) < v)
        throw FileFormat::InvalidFormatException("Numeric field out of "
                                                 "range in GIF stream");
    return v;
}

inline void read(InputStream& in, char* buffer, int n)
{
    if (in.read_all(buffer, n) != static_cast<unsigned>(n))
        throw FileFormat::InvalidFormatException("Premature end of GIF stream");
}

/// \return True iff anything was discarded.
inline bool discard_rest(InputStream& in)
{
    return 0 < in.discard_rest();
}

/// \return True if the comment was truncated.
bool read_comment(InputStream& in, std::string& comment)
{
    // Transcode to kill any non ASCII character in the input, since the GIF
    // specification does not account for anything beyond that.
    std::unique_ptr<InputStream> transcoder =
        get_transcoding_input_stream(in, transcode_US_ASCII, transcode_UTF_8); // Throws
    std::string c, sep = (comment.size() ? "\n\n" : "");
    std::size_t s = comment.size() + sep.size(), n = 1024;
    bool end = false, overflow = false;
    if (s < g_max_comment_size) {
        for (;;) {
            std::string d = transcoder->read_all(n);
            c += d;
            // If we exceeded the limit (trimming may revert this)
            if (g_max_comment_size < s + c.size()) {
                std::string e = Text::line_trim_ascii(c);
                if (g_max_comment_size <= s + e.size()) {
                    c = e;
                    break;
                }
            }
            if (d.size() < n) { // EOI
                end = true;
                break;
            }
        }
    }
    if (!c.empty()) {
        std::string d = Text::line_trim_ascii(c.substr(0, g_max_comment_size-s));
        if (d.size() < c.size())
            overflow = true;
        comment += sep;
        comment += d;
    }

    // Make sure the stream has no more data
    if (!end) {
        if (!overflow && transcoder->discard_n(1) == 1)
            overflow = true;
        discard_rest(in);
    }

    return overflow;
}



BufferedImage::Ref create_screen(int width, int height, bool has_alpha)
{
    ColorSpace::ConstRef color_space = ColorSpace::get_RGB();
    int bits_per_byte = std::numeric_limits<unsigned char>::digits;
    int channel_pitch = bits_per_byte;
    WordType word_type = get_word_type_by_bit_width(channel_pitch);
    int num_channels = color_space->get_num_primaries() + (has_alpha?1:0);
    IntegerBufferFormat::ChannelLayout channels;
    channels.bits_per_pixel = num_channels * channel_pitch;
    for (int i = 0; i < num_channels; ++i)
        channels.add(IntegerBufferFormat::Channel(i*channel_pitch, 8));
    // We want to use the 8 least significant bits of of each byte in case the
    // byte is wider.
    bool most_sig_bit_first = false;

    IntegerBufferFormat::Ref buf_fmt =
        IntegerBufferFormat::get_format(word_type, channels, most_sig_bit_first);

//std::cerr << "---- LOAD GIF: "<<width<<"x"<<height<<" "<<buf_fmt->print(color_space, has_alpha)<<"\n";

    return BufferedImage::new_image(width, height, color_space, has_alpha, buf_fmt);
}



/// Since <tt>giflib</tt> is not thread safe, this is a reimplementation of it.
///
/// \sa http://www.w3.org/Graphics/GIF/spec-gif89a.txt
/// \sa http://sourceforge.net/projects/giflib
class FormatGif: public FileFormat {
public:
    std::string get_name() const override
    {
        return "gif";
    }

    bool check_signature(InputStream& in) const override
    {
        char header[6];
        return in.read_all(header, 6)==6 &&
            (std::equal(header, header+6, "GIF87a") ||
             std::equal(header, header+6, "GIF89a"));
    }

    bool check_suffix(std::string s) const override
    {
        return (s == "gif");
    }

    BufferedImage::Ref load(InputStream& in, Logger* logger,
                            ProgressTracker* tracker) const override
    {
        if (!check_signature(in))
            throw InvalidFormatException("Not a GIF header");

        // Read the Logical Screen Descriptor and the Global Color Table and
        // create the "canvas" image.
        int screen_width;
        int screen_height;
        BufferedImage::Ref screen;
        int num_global_colors;
        std::unique_ptr<char[]> global_color_map;
        int background_color_index;
        {
            char buffer[7];
            read(in, buffer, sizeof(buffer));

            screen_width  = make_word(buffer+0);
            screen_height = make_word(buffer+2);
            if (screen_width < 1 || screen_height < 1)
                throw InvalidFormatException("Bad screen size in GIF stream");

            bool global_color_table_flag = buffer[4] & 0x80;
//            int color_resolution = ((buffer[4] & 0x70) >> 4) + 1;
            num_global_colors = 1 << ((buffer[4] & 0x07) + 1);
            background_color_index = static_cast<unsigned char>(buffer[5]);
//            int pixel_aspect_ratio = static_cast<unsigned char>(buffer[6]);

            // Read the Global Color Table if present
            if (global_color_table_flag) {
                global_color_map = std::make_unique<char[]>(3*num_global_colors); // Throws
                read(in, global_color_map.get(), 3*num_global_colors);
            }
        }
        // Loop over remaining blocks in stream
        bool comment_overflow = false;
        std::string comment;
        bool got_graphical_extension = false;
        int disposal_method = 0;
//        bool user_input_flag = ...;
        bool transparent_color_flag = false;
//        int delay_time; // In hundredths (1/100) of a second
        int transparent_color_index = 0;
        int max_pixel_warnings = 100;
        unsigned long global_pixels_seen=0,
            global_pixels_expected = 0; // For progress tracker
        AdaptiveTicker ticker(1000); // once every second
        for (;;) {
            unsigned block_type;
            {
                char c;
                read(in, &c, 1);
                block_type = static_cast<unsigned char>(c);
            }
            switch (block_type) {
                case 0x21: { // Extension block
                    char c;
                    read(in, &c, 1);
                    unsigned extension_type = static_cast<unsigned char>(c);
                    switch (extension_type) {
                        case 0x01: // Plain Text Extension
                            logger->log("WARNING: Plain Text frame not yet supported, "
                                        "skipped");
                            discard_rest(*block_codec->get_dec_in_stream(in));
                            break;
                        case 0xF9: { // Graphic Control Extension
                            if (got_graphical_extension)
                                logger->log("WARNING: Too many Graphic Control Extensions");
                            char buffer[6];
                            read(in, buffer, sizeof buffer);
                            disposal_method        = buffer[1] >> 2 & 0x7;
//                user_input_flag        = buffer[1]      & 0x2;
                            transparent_color_flag = buffer[1]      & 0x1;
//                delay_time = make_word(buffer+2);
                            transparent_color_index = static_cast<unsigned char>(buffer[4]);

                            got_graphical_extension = true;
                            break;
                        }
                        case 0xFE: { // Comment Extension
                            std::unique_ptr<InputStream> block_reader =
                                block_codec->get_dec_in_stream(in); // Throws
                            if (comment_overflow) {
                                discard_rest(*block_reader);
                            }
                            else if(read_comment(*block_reader, comment)) {
                                comment_overflow = true;
                                logger->log("WARNING: Comment was truncated");
                            }
                            break;
                        }
                        case 0xFF: { // Application Extension
                            char buffer[12];
                            read(in, buffer, sizeof(buffer));
                            logger->log("WARNING: Application Extension Block: '"+
                                        std::string(buffer+1, 8)+"', skipped");
                            discard_rest(*block_codec->get_dec_in_stream(in));
                            break;
                        }
                        default:
                            logger->log("WARNING: Unexpected extension type '"+
                                        Text::print(extension_type)+"', attempting to skip");
                            discard_rest(*block_codec->get_dec_in_stream(in));
                    }
                    break;
                }

                case 0x2C: { // Image descriptor
                    bool is_transparent = got_graphical_extension && transparent_color_flag;

                    // If this is the first image to be processed, create the
                    // pixel buffer
                    if (!screen) {
                        screen = create_screen(screen_width, screen_height, is_transparent);
                        if (is_transparent || !global_color_map) {
                            screen->clear();
                        }
                        else {
                            char* c = global_color_map.get() + 3*background_color_index;
                            unsigned char r = c[0], g = c[1], b = c[2];
                            screen->fill(PackedTRGB(r,g,b));
                        }
                        if (tracker)
                            tracker->defined(screen);
                    }

                    char buffer[9];
                    read(in, buffer, sizeof buffer);

                    int width  = make_word(buffer+4);
                    int height = make_word(buffer+6);
                    int left   = make_word(buffer+0);
                    int bottom = screen_height-make_word(buffer+2)-height;

                    bool local_color_table_flag = buffer[8] & 0x80;
                    bool interlace_flag         = buffer[8] & 0x40;
                    int num_local_colors        = 1 << ((buffer[8] & 0x07) + 1);

                    // Read the Local Color Table if present
                    std::unique_ptr<char[]> local_color_map;
                    if (local_color_table_flag) {
                        local_color_map = std::make_unique<char[]>(3*num_local_colors); // Throws
                        read(in, local_color_map.get(), 3*num_local_colors);
                    }

                    // Need initial code size for LZW decoder
                    read(in, buffer, 1);
                    int lzw_bits_per_pixel = static_cast<unsigned char>(*buffer);
                    if (lzw_bits_per_pixel < 2 || 8 < lzw_bits_per_pixel)
                        throw InvalidFormatException("Bad bits-per-pixel for LZW");

                    // Choose the right color map
                    char* color_map;
                    int num_colors;
                    std::unique_ptr<char[]> gray_map;
                    if (local_color_map) {
                        color_map = local_color_map.get();
                        num_colors = num_local_colors;
                    }
                    else if(global_color_map) {
                        color_map = global_color_map.get();
                        num_colors = num_global_colors;
                    }
                    else {
                        // FIXME: This assumes grayscale data, is that OK?
                        logger->log("WARNING: No color map, assuming grayscale");
                        num_colors = 1 << lzw_bits_per_pixel;
                        gray_map = std::make_unique<char[]>(3*num_colors); // Throws
                        color_map = gray_map.get();
                        for (int i = 0; i < num_colors; ++i) {
                            std::fill(color_map+3*i, color_map+3*i+3,
                                      frac_adjust_bit_width(i, lzw_bits_per_pixel, 8));
                        }
                    }

                    // Prepare an LZW decoder
                    std::unique_ptr<const Codec> lzw = get_lempel_ziv_welch_codec(lzw_bits_per_pixel);
                    std::unique_ptr<InputStream> block_reader = block_codec->get_dec_in_stream(in);
                    std::unique_ptr<InputStream> decoder = lzw->get_dec_in_stream(*block_reader);

                    ssize_t pitch = screen->get_num_channels();
                    ssize_t stride = screen_width * pitch;
                    char* pixel_buffer = static_cast<char*>(screen->get_buffer_ptr());

                    // Prepare a buffer for reading decoded data
                    std::size_t decode_buffer_size = 1024;
                    std::unique_ptr<char[]> decode_buffer = std::make_unique<char[]>(decode_buffer_size); // Throws
                    char* decode_buffer_begin = decode_buffer.get();
                    char* decode_buffer_end   = decode_buffer_begin;

                    // Configure interlacing (last column for non-interlaced)
                    static int pass_offset[]   = { 0, 4, 2, 1, 0 };
                    static int pass_step[]     = { 8, 8, 4, 2, 1 };
                    static int pass_rep_up[]   = { 3, 1, 0, 0, 0 };
                    static int pass_rep_down[] = { 4, 2, 1, 0, 0 };
                    int pass_begin = interlace_flag ? 0 : 4;
                    int pass_end   = interlace_flag ? 4 : 5;

                    int top   = bottom + height - 1;
                    int right = left   + width  - 1;

                    // For progress tracking
                    unsigned long seen = 0, expected =
                        static_cast<unsigned long>(width) * height;
                    global_pixels_expected += expected;

                    for (int p = pass_begin; p < pass_end; ++p) {
                        int y_step = pass_step[p];
                        for (int y = top-pass_offset[p]; bottom <= y; y -= y_step) {
                            if (tracker && ticker.tick()) {
                                tracker->progress(double(global_pixels_seen+seen) /
                                                  global_pixels_expected);
                            }

                            char* strip = pixel_buffer + y*stride + left*pitch;
                            char* pixel = strip;
                            for (int x = left; x <= right; ++x, pixel += pitch, ++seen) {
                                // Decode more data if we are dry
                                if (decode_buffer_begin == decode_buffer_end) {
                                    std::size_t n = decoder->read(decode_buffer.get(), decode_buffer_size);
                                    if (n == 0)
                                        throw std::runtime_error("Got EOI from decoder"
                                                                 " - this must never happen");
                                    decode_buffer_begin = decode_buffer.get();
                                    decode_buffer_end   = decode_buffer_begin + n;
                                }

                                // Fetch the color index for the next pixel
                                int index = static_cast<unsigned char>(*decode_buffer_begin++);

                                // Skip the pixel if it is outside the screen border
                                if (y < 0 || screen_height <= y || x < 0 || screen_width <= x)
                                    continue;

                                // Skip transparent pixels
                                if (is_transparent && index == transparent_color_index)
                                    continue;

                                // Also skip pixels with invalid color indices
                                if (num_colors <= index) {
                                    if (max_pixel_warnings) {
                                        logger->log("WARNING: Color index "+Text::print(index)+
                                                    "/"+Text::print(num_colors-1)+" out of range");
                                        --max_pixel_warnings;
                                    }
                                    continue;
                                }

                                // Write the pixel into the buffer
                                char* color = color_map + 3*index;
                                std::copy(color, color+3, pixel);
                                if (is_transparent) // Set full alpha
                                    reinterpret_cast<unsigned char&>(pixel[3]) = 0xFF;
                            }

                            // Fill entire image area in each pass if a progress
                            // tracker is passed. We cannot do it though, if
                            // this image has transparency, because then we
                            // might not get the pixels corrected.
                            if (interlace_flag && tracker && !is_transparent) {
                                int up   = std::min(pass_rep_up[p], screen_height-1-y);
                                int down = std::min(pass_rep_down[p], y);
                                int o = std::max(-left, 0);                     // Left edge clip
                                int w = std::min(width, screen_width-left) - o; // Right edge clip
                                if (0 <= up && 0 <= down && 0 < w && (up || down)) { // Non-empty intersect
                                    TupleGrid(strip + o*pitch, pitch, stride).
                                        extend(pitch, w, 1, 0, 0, down, up, 0, 0, 0, 0);
                                }
                            }
                        }
                    }

                    global_pixels_seen += seen;
                    if (tracker)
                        tracker->progress(global_pixels_expected ?
                                          double(global_pixels_seen) /
                                          global_pixels_expected : 1);

                    // We want the first complete image, possibly made of of
                    // multiple layers. We stop when the next image is a
                    // complete replacement.
                    if (disposal_method != 1)
                        return screen;

                    // Discard any extraneous pixel data
                    discard_rest(*block_reader);


                    got_graphical_extension = false;
                    break;
                }

                case 0x3B: // Trailer
                    if (!screen)
                        logger->log("WARNING: GIF stream with no images");
                    logger->log("Comment was: " + comment);
                    if (tracker)
                        tracker->progress(1);
                    return screen;

                default:
                    throw InvalidFormatException("Unexpected block type '"+
                                                 Text::print(block_type)+"'");
            }
        }
    }


    void save(Image::ConstRefArg, OutputStream&, Logger*, ProgressTracker*) const override
    {
        /*
          Ideas:

          Construct an image whose pixels are indexes into the palette.

        */

        throw std::runtime_error("Not yet implemented");
    }

    FormatGif():
        block_codec(get_block_codec()) // Throws
    {
    }

    const std::unique_ptr<const Codec> block_codec;
};

} // unnamed namespace


namespace archon {
namespace image {

FileFormat::ConstRef get_default_gif_file_format()
{
    static FileFormat::ConstRef f(new FormatGif());
    return f;
}

} // namespace image
} // namespace archon
