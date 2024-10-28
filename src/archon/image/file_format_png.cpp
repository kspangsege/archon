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


#include <cstddef>
#include <csetjmp>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <memory>
#include <optional>
#include <string_view>
#include <string>
#include <exception>
#include <system_error>
#include <stdexcept>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/type_traits.hpp>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/format.hpp>
#include <archon/core/endianness.hpp>
#include <archon/core/misc_error.hpp>
#include <archon/core/source.hpp>
#include <archon/core/sink.hpp>
#include <archon/log/logger.hpp>
#include <archon/image/impl/config.h>
#include <archon/image/geom.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/image/buffer_format.hpp>
#include <archon/image/image.hpp>    
#include <archon/image/writable_image.hpp>
#include <archon/image/integer_pixel_format.hpp>
#include <archon/image/subword_pixel_format.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/image/progress_tracker.hpp>
#include <archon/image/image_provider.hpp>
#include <archon/image/error.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_png.hpp>
#include <archon/image/load_config.hpp>
#include <archon/image/save_config.hpp>

#if ARCHON_IMAGE_HAVE_PNG
#  include <png.h>
#  if PNG_LIBPNG_VER < 10504
#    error "PNG library is too old"
#  endif
#endif


using namespace archon;
using namespace std::literals;


namespace {


constexpr std::string_view g_file_format_ident = "png"sv;

constexpr std::string_view g_file_format_descr = "PNG (Portable Network Graphics)"sv;

constexpr std::string_view g_mime_types[] = {
    "image/png"sv,
};

constexpr std::string_view g_filename_extensions[] = {
    ".png"sv,
};



#if ARCHON_IMAGE_HAVE_PNG


static_assert(core::type_in<png_byte, char, signed char, unsigned char>);


constexpr bool png_byte_safe_alias = core::type_in<png_byte, char, unsigned char>;


// If `true`, an image buffer of `short` elements is compatible with libpng's 16-bit format,
// however, if `short` is stored in little-endian form, it requires turning byte swapping on
// in libpng.
constexpr bool png_16bit_as_short = (png_byte_safe_alias && core::int_width<char>() == 8 &&
                                     core::int_width<short>() == 16);


template<class W> inline auto to_png_bytep(W* ptr) noexcept
{
    using word_type = W;
    // We are not supposed to violate the type aliasing rules of C++. The check is not a
    // static assertion because we may need to instantiate this function in a way that would
    // violate type aliasing rules if it was invoked.
    constexpr bool allowed = (png_byte_safe_alias ||
                              core::type_in<std::remove_const_t<word_type>, char, signed char, unsigned char>);
    if constexpr (allowed) {
        using void_type = core::CopyConstness<word_type, void>;
        using png_byte_type = core::CopyConstness<word_type, png_byte>;
        return static_cast<png_byte_type*>(static_cast<void_type*>(ptr));
    }
    else {
        ARCHON_STEADY_ASSERT_UNREACHABLE();
    }
}


template<class W> inline auto from_png_bytep(png_bytep ptr) noexcept
{
    using word_type = W;
    // We are not supposed to violate the type aliasing rules of C++.
    static_assert(core::type_in<std::remove_const_t<word_type>, char, unsigned char>);
    using void_type = core::CopyConstness<word_type, void>;
    return static_cast<word_type*>(static_cast<void_type*>(ptr));
}


inline auto string_for_color_type(png_byte val) -> std::string_view
{
    switch (val) {
        case PNG_COLOR_TYPE_PALETTE:
            return "Palette";
        case PNG_COLOR_TYPE_GRAY:
            return "Lum";
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            return "LumA";
        case PNG_COLOR_TYPE_RGB:
            return "RGB";
        case PNG_COLOR_TYPE_RGB_ALPHA:
            return "RGBA";
        default:
            return {};
    }
}


inline auto string_for_interlace_type(png_byte val) -> std::string_view
{
    switch (val) {
        case PNG_INTERLACE_ADAM7:
            return "Adam7";
        default:
            return {};
    }
}


bool try_recognize(core::Source& source, bool& recognized, std::error_code& ec)
{
    constexpr std::size_t header_size = 8;
    char header[header_size];
    std::size_t n = 0;
    if (ARCHON_LIKELY(source.try_read(header, n, ec))) { // Throws
        recognized = (n == header_size && png_sig_cmp(to_png_bytep(header), 0, header_size) == 0);
        return true; // Success
    }
    return false; // Failure
}


struct Format {
    png_byte bit_depth;
    png_byte color_type;

    bool operator==(Format other) const noexcept
    {
        return (bit_depth == other.bit_depth &&
                color_type == other.color_type);
    }

    bool operator!=(Format other) const noexcept
    {
        return !(*this == other);
    }
};


struct ReadTransformations {
    bool palette_to_rgb;
    bool unpack_subbyte;
    bool transparency_to_alpha;
    bool alpha_first;
    bool rgb_to_bgr;
    bool swap_bytes;
    bool swap_bits;
};


struct WriteTransformations {
    bool alpha_first;
    bool rgb_to_bgr;
    bool swap_bytes;
    bool swap_bits;
};


bool try_match_save_format(const image::BufferFormat& buffer_format, image::Size image_size, Format& format,
                           WriteTransformations& xforms, std::size_t& bytes_per_row)
{
    image::BufferFormat::IntegerFormat integer_format = {};
    image::BufferFormat::IntegerType word_type = image::BufferFormat::IntegerType::byte;
    if (ARCHON_LIKELY(buffer_format.try_cast_to(integer_format, word_type))) { // Throws
        if (ARCHON_LIKELY(integer_format.bits_per_word == 8)) {
            png_byte bit_depth;
            bool swap_bytes = false;
            if (ARCHON_LIKELY(integer_format.words_per_channel == 1)) {
                bit_depth = 8;
            }
            else if (ARCHON_LIKELY(integer_format.words_per_channel == 2)) {
                bit_depth = 16;
                swap_bytes = (integer_format.word_order == core::Endianness::little);
            }
            else {
                return false;
            }
            png_byte color_type;
            if (ARCHON_LIKELY(integer_format.channel_conf.color_space->is_rgb())) {
                color_type = (integer_format.channel_conf.has_alpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB);
            }
            else if (ARCHON_LIKELY(integer_format.channel_conf.color_space->is_lum())) {
                color_type = (integer_format.channel_conf.has_alpha ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY);
            }
            else {
                return false;
            }
            bool alpha_first = (integer_format.channel_conf.alpha_channel_first !=
                                integer_format.channel_conf.reverse_order);
            bool rgb_to_bgr = integer_format.channel_conf.reverse_order;
            bool swap_bits = false;
            std::size_t bytes_per_row_2 = integer_format.get_words_per_row(image_size.width); // Throws
            format = { bit_depth, color_type };
            xforms = { alpha_first, rgb_to_bgr, swap_bytes, swap_bits };
            bytes_per_row = bytes_per_row_2;
            return true;
        }
        return false;
    }
    image::BufferFormat::SubwordFormat subword_format = {};
    if (ARCHON_LIKELY(buffer_format.try_cast_to(subword_format, word_type))) { // Throws
        switch (subword_format.bits_per_channel) {
            case 1:
            case 2:
            case 4:
                break;
            default:
                return false;
        }
        if (!subword_format.channel_conf.color_space->is_lum())
            return false;
        if (subword_format.channel_conf.has_alpha)
            return false;
        int pixels_per_byte = 8 / subword_format.bits_per_channel;
        if (subword_format.pixels_per_word != pixels_per_byte)
            return false;
        int bytes_per_row_2 = image_size.width / pixels_per_byte;
        int remainder       = image_size.width % pixels_per_byte;
        if (remainder != 0) {
            if (!subword_format.word_aligned_rows)
                return false;
            bytes_per_row_2 += 1;
        }
        format = {};
        xforms = {};
        format.bit_depth  = png_byte(subword_format.bits_per_channel);
        format.color_type = PNG_COLOR_TYPE_GRAY;
        xforms.swap_bits = (subword_format.bit_order != core::Endianness::big);
        bytes_per_row = core::int_cast<std::size_t>(bytes_per_row_2); // Throws
        return true;
    }
/*
    image::BufferFormat::IndexedFormat indexed_format = {};
    if (ARCHON_LIKELY(buffer_format.try_cast_to(indexed_format, word_type))) { // Throws
        
        ARCHON_STEADY_ASSERT_UNREACHABLE();                                     
        return false;
    }
*/
    return false;
}


template<class F>
auto create_image_1(image::Size size, png_byte*& buffer, std::size_t& bytes_per_row) ->
    std::unique_ptr<image::WritableImage>
{
    using format_type = F;
    using image_type = image::BufferedImage<format_type>;
    auto image = std::make_unique<image_type>(size); // Throws
    std::size_t words_per_row = format_type::get_words_per_row(size.width); // Throws
    buffer = to_png_bytep(image->get_buffer().data());
    bytes_per_row = words_per_row * sizeof (typename format_type::word_type);
    return image;
}


template<class C>
auto create_image_2(image::Size size, png_byte bit_depth, bool use_short_int, png_byte*& buffer,
                    std::size_t& bytes_per_row) -> std::unique_ptr<image::WritableImage>
{
    using channel_spec_type = C;
    constexpr bool alpha_channel_first = false;
    constexpr bool reverse_channel_order = false;
    if (ARCHON_LIKELY(!use_short_int)) {
        using word_type = image::int8_type;
        constexpr int bits_per_word = 8;
        constexpr core::Endianness word_order =
            core::Endianness::big; // libpng uses network byte order which is big endian
        if (ARCHON_LIKELY(bit_depth == 8)) {
            using comp_type = word_type;
            constexpr int words_per_channel = 1;
            using format_type =
                image::IntegerPixelFormat<channel_spec_type, word_type, bits_per_word, comp_type, words_per_channel,
                                          word_order, alpha_channel_first, reverse_channel_order>;
            return create_image_1<format_type>(size, buffer, bytes_per_row); // Throws
        }
        else if (ARCHON_LIKELY(bit_depth == 16)) {
            using comp_type = image::int16_type;
            constexpr int words_per_channel = 2;
            using format_type =
                image::IntegerPixelFormat<channel_spec_type, word_type, bits_per_word, comp_type, words_per_channel,
                                          word_order, alpha_channel_first, reverse_channel_order>;
            return create_image_1<format_type>(size, buffer, bytes_per_row); // Throws
        }
    }
    else {
        ARCHON_ASSERT(core::int_width<char>() == 8);
        ARCHON_ASSERT(core::int_width<short>() == 16);
        ARCHON_ASSERT(bit_depth == 16);
        using word_type = short;
        constexpr int bits_per_word = 16;
        using comp_type = word_type;
        constexpr int words_per_channel = 1;
        constexpr core::Endianness word_order = core::Endianness::big; // Value is immaterial
        using format_type =
            image::IntegerPixelFormat<channel_spec_type, word_type, bits_per_word, comp_type, words_per_channel,
                                      word_order, alpha_channel_first, reverse_channel_order>;
        return create_image_1<format_type>(size, buffer, bytes_per_row); // Throws
    }
    ARCHON_ASSERT_UNREACHABLE();
    return nullptr;
}


auto create_image(image::Size size, const Format& format, bool use_short_int, png_byte*& buffer,
                  std::size_t& bytes_per_row) -> std::unique_ptr<image::WritableImage>
{
    switch (format.color_type) {
        case PNG_COLOR_TYPE_GRAY: {
            if (ARCHON_LIKELY(format.bit_depth >= 8)) {
                return create_image_2<image::ChannelSpec_Lum>(size, format.bit_depth, use_short_int, buffer,
                                                              bytes_per_row); // Throws
            }
            ARCHON_ASSERT(!use_short_int);
            using word_type = unsigned char;
            constexpr core::Endianness bit_order =
                core::Endianness::big; // libpng uses big endian bit order
            constexpr bool word_aligned_rows = true;
            if (format.bit_depth == 1) {
                constexpr int bits_per_channel = 1;
                constexpr int pixels_per_word = 8;
                using format_type =
                    image::SubwordPixelFormat_Lum<word_type, bits_per_channel, pixels_per_word,
                                                  bit_order, word_aligned_rows>;
                return create_image_1<format_type>(size, buffer, bytes_per_row); // Throws
            }
            if (format.bit_depth == 2) {
                constexpr int bits_per_channel = 2;
                constexpr int pixels_per_word = 4;
                using format_type =
                    image::SubwordPixelFormat_Lum<word_type, bits_per_channel, pixels_per_word,
                                                  bit_order, word_aligned_rows>;
                return create_image_1<format_type>(size, buffer, bytes_per_row); // Throws
            }
            if (format.bit_depth == 4) {
                constexpr int bits_per_channel = 4;
                constexpr int pixels_per_word = 2;
                using format_type =
                    image::SubwordPixelFormat_Lum<word_type, bits_per_channel, pixels_per_word,
                                                  bit_order, word_aligned_rows>;
                return create_image_1<format_type>(size, buffer, bytes_per_row); // Throws
            }
            break;
        }
        case PNG_COLOR_TYPE_GRAY_ALPHA: {
            return create_image_2<image::ChannelSpec_LumA>(size, format.bit_depth, use_short_int,
                                                           buffer, bytes_per_row); // Throws
        }
        case PNG_COLOR_TYPE_RGB: {
            return create_image_2<image::ChannelSpec_RGB>(size, format.bit_depth, use_short_int,
                                                          buffer, bytes_per_row); // Throws
        }
        case PNG_COLOR_TYPE_RGB_ALPHA: {
            return create_image_2<image::ChannelSpec_RGBA>(size, format.bit_depth, use_short_int,
                                                           buffer, bytes_per_row); // Throws
        }
        default:
            break;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return nullptr;
}


// Notes on using setjmp/longjmp in C++:
//
// Great care must be taken when using setjmp/longjmp in C++ context. The main problem is
// that stack objects will not have their destructors called during a long jump stack
// unwinding process.
//
// A secondary problem is that an automatic variable in the scope from which setjmp() is
// called has indeterminate state upon a long jump if it was modified after the invocation
// of setjmp().
//
// In order to be on the safe side, the following restrictions should be adhered to:
//
//  * Keep the setjmp/longjmp construction on the following canonical form:
//
//      if (setjmp(jmpbuf) == 0) { // try
//         // code that may potentially issue a long jump (throw)
//      }
//      else { // catch
//          // code that handles the long jump (exception)
//      }
//
//  * Keep the "try"-scope completely free of automatic variables of nontrivial type
//    (std::is_trivial). This also applies to any sub-scope from which a long jump may be
//    directly or indirectly initiated. A sub-scope from which a long jump can not be
//    initiated, may safely have automatic variable of nontrivial type.
//
//  * Keep statements, that directly or indirectly may initiate a long jump, completely free
//    of temporaries of nontrivial type.
//
//  * Keep the scope from which setjmp() is called completely free of automatic variables.
//
//  * Never long jump out of a C++ try block or catch block.
//


struct Context {
    bool is_save = false;
    log::Logger* logger = nullptr;
    image::ProgressTracker* progress_tracker = nullptr;

    // Progress notification
    const image::Image* image = nullptr;
    int num_rows_per_notification = 1;
    int num_rows_total = 0;
    int num_rows_at_next_notification = 1;

    // Error handling
    std::error_code ec;
    bool has_error = false;
    std::exception_ptr exception;
};


[[noreturn]] void error_callback(png_structp png_ptr, png_const_charp message) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() may be called or through with stack unwinding may pass (see notes on
    // long jump safety above).

    Context& ctx = *static_cast<Context*>(png_get_error_ptr(png_ptr));
    try {
        ctx.logger->error("%s", std::string_view(message)); // Throws
        if (ctx.is_save) {
            ctx.ec = image::Error::saving_process_failed;
        }
        else {
            ctx.ec = image::Error::bad_file;
        }
        ctx.has_error = true;
    }
    catch (...) {
        ctx.exception = std::current_exception();
    }
    std::longjmp(png_jmpbuf(png_ptr), 1);
}


void warning_callback(png_structp png_ptr, png_const_charp message) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() may be called or through with stack unwinding may pass (see notes on
    // long jump safety above).

    Context& ctx = *static_cast<Context*>(png_get_error_ptr(png_ptr));
    try {
        ctx.logger->warn("%s", std::string_view(message)); // Throws
        return;
    }
    catch (...) {
        ctx.exception = std::current_exception();
    }
    std::longjmp(png_jmpbuf(png_ptr), 1);
}


void progress_callback(png_structp png_ptr, png_uint_32 row, int pass) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() may be called or through with stack unwinding may pass (see notes on
    // long jump safety above).

    // This function must only be used with non-interlaced images
    ARCHON_ASSERT(row > 0);
    ARCHON_ASSERT(pass == 0);
    Context& ctx = *static_cast<Context*>(png_get_error_ptr(png_ptr));
    try {
        int num_rows = int(row);
        if (ARCHON_LIKELY(num_rows < ctx.num_rows_at_next_notification))
            return;

        ARCHON_ASSERT(ctx.image);
        ARCHON_ASSERT(num_rows <= ctx.num_rows_total);
        ARCHON_ASSERT(num_rows <= ctx.num_rows_at_next_notification);
        ARCHON_ASSERT(ctx.num_rows_total > 0);
        double fraction = double(num_rows) / ctx.num_rows_total;
        ctx.progress_tracker->progress(*ctx.image, fraction); // Throws

        int remain = ctx.num_rows_total - ctx.num_rows_at_next_notification;
        ctx.num_rows_at_next_notification += std::min(ctx.num_rows_per_notification, remain);
        return;
    }
    catch (...) {
        ctx.exception = std::current_exception();
    }
    std::longjmp(png_jmpbuf(png_ptr), 1);
}


class LoadContext
    : public Context {
public:
    image::ImageProvider* image_provider = nullptr;
    core::Source* source = nullptr;
    const std::locale* locale = nullptr;

    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    png_uint_32 width = 0, height = 0;
    Format raw_format = {};
    bool has_transparency_chunk = false;
    png_byte interlace_type = 0;
    image::Size image_size;
    Format format = {};
    ReadTransformations xforms = {};
    bool use_short_int = false;
    int num_passes = 0;
    Format format_2 = {};
    std::size_t bytes_per_row = 0;
    std::unique_ptr<image::WritableImage> image_2;
    std::unique_ptr<png_byte*[]> rows;

    ~LoadContext() noexcept
    {
        if (png_ptr)
            png_destroy_read_struct(&png_ptr, (info_ptr ? &info_ptr : nullptr), nullptr);
    }

    template<class... P> [[noreturn]] void fatal(const char* message, const P&... params)
    {
        std::string message_2 = core::format(*locale, "Failed to load PNG image: %s",
                                             core::formatted(message, params...)); // Throws
        throw std::runtime_error(std::move(message_2)); // Throws
    }

    bool process_stage_1()
    {
        if (ARCHON_UNLIKELY(width < 1 || height < 1 ||
                            !core::try_int_cast(width, image_size.width) ||
                            !core::try_int_cast(height, image_size.height))) {
            ec = image::Error::image_size_out_of_range;
            has_error = true;
            return false;
        }

        //                                          Possible number
        //                              Number of   of bits per color /   Possible number
        //  Color type                  channels    alpha channel         of bits per pixel
        // ---------------------------------------------------------------------------------
        //  PNG_COLOR_TYPE_PALETTE      3           8                     1, 2, 4, 8
        //  PNG_COLOR_TYPE_GRAY         1           1, 2, 4, 8, 16        1, 2, 3, 8, 16
        //  PNG_COLOR_TYPE_GRAY_ALPHA   2           8, 16                 16, 32
        //  PNG_COLOR_TYPE_RGB          3           8, 16                 24, 48
        //  PNG_COLOR_TYPE_RGB_ALPHA    4           8, 16                 32, 64

        switch (raw_format.bit_depth) {
            case 1:
            case 2:
            case 4:
            case 8:
            case 16:
                break;
            default:
                fatal("Unexpected bit depth: %s", core::promote(raw_format.bit_depth)); // Throws
        }

        switch (raw_format.color_type) {
            case PNG_COLOR_TYPE_PALETTE:
            case PNG_COLOR_TYPE_GRAY:
            case PNG_COLOR_TYPE_GRAY_ALPHA:
            case PNG_COLOR_TYPE_RGB:
            case PNG_COLOR_TYPE_RGB_ALPHA:
                break;
            default:
                fatal("Unexpected color type: %s", core::promote(raw_format.color_type)); // Throws
        }

        format = raw_format;
        if (!image_provider) {
            // Ask libpng to convert palette-based formats into regular formats
            if (format.color_type == PNG_COLOR_TYPE_PALETTE) {
                xforms.palette_to_rgb = true;
                format.color_type = PNG_COLOR_TYPE_RGB;
                format.bit_depth = 8;
            }

            // Ask libpng to convert transparency information to a proper alpha channel
            if (has_transparency_chunk) {
                switch (format.color_type) {
                    case PNG_COLOR_TYPE_GRAY:
                        if (format.bit_depth < 8) {
                            xforms.unpack_subbyte = true;
                            format.bit_depth = 8;
                        }
                        xforms.transparency_to_alpha = true;
                        format.color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
                        break;
                    case PNG_COLOR_TYPE_RGB:
                        if (format.bit_depth < 8) {
                            xforms.unpack_subbyte = true;
                            format.bit_depth = 8;
                        }
                        xforms.transparency_to_alpha = true;
                        format.color_type = PNG_COLOR_TYPE_RGBA;
                        break;
                }
            }

            // FIXME: Consider adding support for palette-type image object and then stop                 
            // asking libpng to make this conversion, then read the palette using
            // `png_get_PLTE()`

            // Swap byte order for 16-bit images if that allows us to use a 16-bit word type
            // in the image buffer. This will generally allow for more efficient pixel
            // access.
            if constexpr (png_16bit_as_short) {
                core::Endianness byte_order = {};
                if (ARCHON_UNLIKELY(format.bit_depth == 16 && core::try_get_byte_order<short>(byte_order))) {
                    use_short_int = true;
                    if (ARCHON_LIKELY(byte_order == core::Endianness::little))
                        xforms.swap_bytes = true;
                }
            }

            return true;
        }

        // Available read transformations:
        //
        //   16 -> 8 bits per channel          --> png_set_scale_16(png_ptr)
        //   Gray: <8 -> 8 bits per pixel      --> png_set_expand_gray_1_2_4_to_8(png_ptr)
        //   Reduce number of bits per channel --> png_set_shift(png_ptr, sig_bit_p);
        //   Invert color channel order
        //   Put alpha channel first
        //   Swap bytes
        //   Add alpha                         --> png_set_filler(png_ptr, 0xffff, PNG_FILLER_AFTER);

/*
        image::ImageTransferInfo transfer_info;
        
        image::Image* image_3 = nullptr;
        image::Pos pos;
        if (ctx.image_provider->provide_image(image_size, transfer_info, image_3, pos, ec)) { // Throws
            
            return true;
        }

        has_error = true;
        return false;
*/
        ARCHON_STEADY_ASSERT_UNREACHABLE();     
    }

    bool process_stage_2()
    {
        if (ARCHON_UNLIKELY(format_2 != format))
            fatal("Unexpected format"); // Throws

        logger->detail("Image size: %s", image_size); // Throws
        if (ARCHON_LIKELY(format.bit_depth == raw_format.bit_depth)) {
            logger->detail("Bit depth: %s", core::promote(format.bit_depth)); // Throws
        }
        else {
            logger->detail("Bit depth: %s (originally: %s)", core::promote(format.bit_depth),
                           core::promote(raw_format.bit_depth)); // Throws
        }
        if (ARCHON_LIKELY(format.color_type == raw_format.color_type)) {
            logger->detail("Color type: %s", string_for_color_type(format.color_type)); // Throws
        }
        else {
            logger->detail("Color type: %s (originally: %s)",
                           string_for_color_type(format.color_type),
                           string_for_color_type(raw_format.color_type)); // Throws
        }
        if (interlace_type == PNG_INTERLACE_NONE) {
            logger->detail("Interlace type: None"); // Throws
        }
        else {
            logger->detail("Interlace type: %s (number of passes: %s)",
                           string_for_interlace_type(interlace_type),
                           core::promote(num_passes)); // Throws
        }

        png_byte* buffer = nullptr;
        std::size_t bytes_per_row_2 = 0;
        image_2 = create_image(image_size, format, use_short_int, buffer,
                               bytes_per_row_2); // Throws

        // Sanity check: Must agree with libpng on number of bytes per row
        if (bytes_per_row_2 != bytes_per_row) {
            fatal("Unexpected number of bytes per row: %s vs %s", bytes_per_row,
                  bytes_per_row_2); // Throws
        }

        // Progress tracking
        image = image_2.get();
        num_rows_per_notification = std::max(4096 / image_size.width, 1);
        num_rows_total = image_size.height;

        // Make row array
        rows = std::make_unique<png_byte*[]>(std::size_t(image_size.height)); // Throws
        {
            png_byte* row = buffer;
            for (std::size_t i = 0; i < std::size_t(image_size.height); ++i) {
                rows[i] = row;
                row += bytes_per_row;
            }
        }

        return true;
    }
};


void read_callback(png_structp png_ptr, png_bytep data, std::size_t size) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() may be called or through with stack unwinding may pass (see notes on
    // long jump safety above).

    LoadContext& ctx = *static_cast<LoadContext*>(png_get_error_ptr(png_ptr));
    try {
        core::Span<char> buffer(from_png_bytep<char>(data), size);
        std::size_t n = 0;
        std::error_code ec;
        if (ARCHON_LIKELY(ctx.source->try_read(buffer, n, ec))) { // Throws
            if (ARCHON_LIKELY(n == size))
                return;
            ec = core::MiscError::premature_end_of_input;
        }
        ctx.ec = ec;
        ctx.has_error = true;
    }
    catch (...) {
        ctx.exception = std::current_exception();
    }
    std::longjmp(png_jmpbuf(png_ptr), 1);
}


bool do_load(LoadContext& ctx)
{
    // Long jump safety: No non-volatile automatic variables in the scope from which
    // setjmp() is called (see notes on long jump safety above).

    ctx.png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, &ctx, error_callback, warning_callback);
    if (ARCHON_UNLIKELY(!ctx.png_ptr))
        ctx.fatal("Failed to create libpng 'read' structure"); // Throws

    ctx.info_ptr = png_create_info_struct(ctx.png_ptr);
    if (ARCHON_UNLIKELY(!ctx.info_ptr))
        ctx.fatal("Failed to create libpng 'info' structure"); // Throws

    // Catch long jumps from one of the callback functions.
    if (setjmp(png_jmpbuf(ctx.png_ptr)) == 0) {
        // Long jump safety: No automatic variables of nontrivial type in the "try"-scope,
        // or in any subscope that may directly or indirectly initiate a long jump (see
        // notes on long jump safety above).

        png_set_sig_bytes(ctx.png_ptr, 8);
        png_set_read_fn(ctx.png_ptr, 0, read_callback);

        // Discard chunks of unknown type
        png_set_keep_unknown_chunks(ctx.png_ptr, PNG_HANDLE_CHUNK_NEVER, nullptr, 0);

        // FIXME: Tend to gamma encode/decode settings           

        // Process all chunks up to but not including the image data
        png_read_info(ctx.png_ptr, ctx.info_ptr);

        ctx.width                  = png_get_image_width(ctx.png_ptr, ctx.info_ptr);
        ctx.height                 = png_get_image_height(ctx.png_ptr, ctx.info_ptr);
        ctx.raw_format.bit_depth   = png_get_bit_depth(ctx.png_ptr, ctx.info_ptr);
        ctx.raw_format.color_type  = png_get_color_type(ctx.png_ptr, ctx.info_ptr);
        ctx.has_transparency_chunk = (png_get_valid(ctx.png_ptr, ctx.info_ptr, PNG_INFO_tRNS) != 0);
        ctx.interlace_type         = png_get_interlace_type(ctx.png_ptr, ctx.info_ptr);

        if (ARCHON_UNLIKELY(!ctx.process_stage_1())) // Throws
            return false;

        // Set PNG read transformations
        if (ctx.xforms.palette_to_rgb)
            png_set_palette_to_rgb(ctx.png_ptr);
        if (ctx.xforms.unpack_subbyte)
            png_set_expand_gray_1_2_4_to_8(ctx.png_ptr); // FIXME: What is the difference between this and png_set_packing(png_ptr)? Answer: The latter does not scale channel values up to the larger number of bits.                       
        if (ctx.xforms.transparency_to_alpha)
            png_set_tRNS_to_alpha(ctx.png_ptr);
        if (ctx.xforms.alpha_first)
            png_set_swap_alpha(ctx.png_ptr);
        if (ctx.xforms.rgb_to_bgr)
            png_set_bgr(ctx.png_ptr);
        if (ctx.xforms.swap_bytes)
            png_set_swap(ctx.png_ptr);
        if (ctx.xforms.swap_bits)
            png_set_packswap(ctx.png_ptr);

        ctx.num_passes = png_set_interlace_handling(ctx.png_ptr);

        // Update header information
        png_read_update_info(ctx.png_ptr, ctx.info_ptr);
        ctx.format_2.bit_depth  = png_get_bit_depth(ctx.png_ptr, ctx.info_ptr);
        ctx.format_2.color_type = png_get_color_type(ctx.png_ptr, ctx.info_ptr);
        ctx.bytes_per_row       = png_get_rowbytes(ctx.png_ptr, ctx.info_ptr);

        if (ARCHON_UNLIKELY(!ctx.process_stage_2())) // Throws
            return false;

        // Track progress using callback function, but only when interlacing is not in use
        // (when interlacing is in use, progress is tracked in a different way)
        bool is_interlaced = (ctx.interlace_type != PNG_INTERLACE_NONE);
        if (ctx.progress_tracker && !is_interlaced)
            png_set_read_status_fn(ctx.png_ptr, progress_callback);

        // Read image data
        if (!is_interlaced || !ctx.progress_tracker) {
            png_read_image(ctx.png_ptr, ctx.rows.get());
        }
        else {
            for (int i = 0; i < ctx.num_passes; ++i) {
                png_read_rows(ctx.png_ptr, nullptr, ctx.rows.get(), ctx.height);
                double fraction = double(i + 1) / ctx.num_passes;
                ctx.progress_tracker->progress(*ctx.image, fraction); // Throws
            }
        }

        // Read final chunks after image data, if any
        png_read_end(ctx.png_ptr, ctx.info_ptr);
    }
    else {
        // Long jumps from the callback functions land here.
        return false;
    }

    return true;
}


bool load(core::Source& source, std::unique_ptr<image::WritableImage>& image, const std::locale& loc,
          log::Logger& logger, image::ProgressTracker* progress_tracker, image::ImageProvider* image_provider,
          const image::PNGLoadConfig&, std::error_code& ec)
{
    // FIXME: Read text comments using `png_get_text()`            

    // FIXME: Get background color using `png_get_bKGD()`             

    bool recognized = {};
    if (ARCHON_UNLIKELY(!try_recognize(source, recognized, ec))) // Throws
        return false; // Failure
    if (ARCHON_UNLIKELY(!recognized)) {
        ec = image::Error::bad_file;
        return false; // Failure
    }

    LoadContext ctx;
    ctx.logger = &logger;
    ctx.progress_tracker = progress_tracker;
    ctx.image_provider = image_provider;
    ctx.source = &source;
    ctx.locale = &loc;

    if (ARCHON_LIKELY(do_load(ctx))) { // Throws
        image = std::move(ctx.image_2);
        return true; // Success
    }

    if (ARCHON_UNLIKELY(ctx.exception))
        std::rethrow_exception(std::move(ctx.exception)); // Throws
    ARCHON_ASSERT(ctx.has_error);
    ec = ctx.ec;
    return false; // Failure
}


class SaveContext
    : public Context {
public:
    core::Sink* sink = nullptr;
    const std::locale* locale = nullptr;

    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;

    png_uint_32 width = 0, height = 0;
    Format format = {};
    WriteTransformations xforms = {};
    std::size_t bytes_per_row = 0;
    std::unique_ptr<const png_byte*[]> rows;
    bool use_interlacing = false;

    SaveContext() noexcept
    {
        is_save = true;
    }

    ~SaveContext() noexcept
    {
        if (png_ptr)
            png_destroy_write_struct(&png_ptr, (info_ptr ? &info_ptr : nullptr));
    }

    template<class... P> [[noreturn]] void fatal(const char* message, const P&... params)
    {
        std::string message_2 = core::format(*locale, "Failed to save PNG image: %s",
                                             core::formatted(message, params...)); // Throws
        throw std::runtime_error(std::move(message_2)); // Throws
    }
};


void write_callback(png_structp png_ptr, png_bytep data, png_size_t size) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() may be called or through with stack unwinding may pass (see notes on
    // long jump safety above).

    SaveContext& ctx = *static_cast<SaveContext*>(png_get_error_ptr(png_ptr));
    try {
        core::Span<const char> data_2(from_png_bytep<const char>(data), size);
        std::size_t n = 0;
        std::error_code ec;
        if (ARCHON_LIKELY(ctx.sink->try_write(data_2, n, ec))) { // Throws
            ARCHON_ASSERT(n == size);
            return;
        }
        ctx.ec = ec;
        ctx.has_error = true;
    }
    catch (...) {
        ctx.exception = std::current_exception();
    }
    std::longjmp(png_jmpbuf(png_ptr), 1);
}


void flush_callback(png_structp) noexcept
{
    // No-op. If the sink needs to be flushed, it is the job of the application to do it.
}


bool do_save(SaveContext& ctx)
{
    // Long jump safety: No non-volatile automatic variables in the scope from which
    // setjmp() is called (see notes on long jump safety above).

    ctx.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, &ctx, error_callback, warning_callback);
    if (ARCHON_UNLIKELY(!ctx.png_ptr))
        ctx.fatal("Failed to create libpng 'write' structure"); // Throws

    ctx.info_ptr = png_create_info_struct(ctx.png_ptr);
    if (ARCHON_UNLIKELY(!ctx.info_ptr))
        ctx.fatal("Failed to create libpng 'info' structure"); // Throws

    // Catch long jumps from one of the callback functions.
    if (setjmp(png_jmpbuf(ctx.png_ptr)) == 0) {
        // Long jump safety: No automatic variables of nontrivial type in the "try"-scope,
        // or in any subscope that may directly or indirectly initiate a long jump (see
        // notes on long jump safety above).

        png_set_write_fn(ctx.png_ptr, 0, write_callback, flush_callback);

        // Track progress using callback function, but only when interlacing is not turned
        // on (when interlacing is turned on, progress is tracked in a different way)
        if (ctx.progress_tracker && !ctx.use_interlacing)
            png_set_write_status_fn(ctx.png_ptr, progress_callback);

        png_byte interlace_type = (ctx.use_interlacing ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE);
        png_set_IHDR(ctx.png_ptr, ctx.info_ptr, ctx.width, ctx.height, ctx.format.bit_depth, ctx.format.color_type,
                     interlace_type, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        // Sanity check: Must agree with libpng on number of bytes per row
        {
            std::size_t bytes_per_row = png_get_rowbytes(ctx.png_ptr, ctx.info_ptr);
            if (ctx.bytes_per_row != bytes_per_row) {
                ctx.fatal("Unexpected number of bytes per row: %s vs %s", ctx.bytes_per_row, bytes_per_row); // Throws
            }
        }

        // FIXME: Set palette here                                        

        // FIXME: Set gamma here               

        // FIXME: Set background color here        

        png_write_info(ctx.png_ptr, ctx.info_ptr);

        // Set PNG write transformations
        if (ctx.xforms.alpha_first)
            png_set_swap_alpha(ctx.png_ptr);
        if (ctx.xforms.rgb_to_bgr)
            png_set_bgr(ctx.png_ptr);
        if (ctx.xforms.swap_bytes)
            png_set_swap(ctx.png_ptr);
        if (ctx.xforms.swap_bits)
            png_set_packswap(ctx.png_ptr);

        // Write image data
        if (!ctx.use_interlacing || !ctx.progress_tracker) {
            png_write_image(ctx.png_ptr, const_cast<png_byte**>(ctx.rows.get()));
        }
        else {
            int num_passes = png_set_interlace_handling(ctx.png_ptr);
            for (int i = 0; i < num_passes; ++i) {
                png_write_rows(ctx.png_ptr, const_cast<png_byte**>(ctx.rows.get()), ctx.height);
                double fraction = double(i + 1) / num_passes;
                ctx.progress_tracker->progress(*ctx.image, fraction); // Throws
            }
        }

/*
        // Add the comment after the pixel data
        if (text.text)
            png_set_text(ctx.png_ptr, ctx.info_ptr, &text, 1);       
*/

        png_write_end(ctx.png_ptr, ctx.info_ptr);
    }
    else {
        // Long jumps from the callback functions land here.
        return false;
    }

    return true;
}


bool save(const image::Image& image, core::Sink& sink, const std::locale& loc, log::Logger& logger,
          image::ProgressTracker* progress_tracker, const image::PNGSaveConfig& config, std::error_code& ec)
{
    SaveContext ctx;
    ctx.logger = &logger;
    ctx.progress_tracker = progress_tracker;
    ctx.sink = &sink;
    ctx.locale = &loc;

    image::Size image_size = image.get_size();
    if (ARCHON_UNLIKELY(image_size.width < 1 || image_size.height < 1 ||
                        !core::try_int_cast(image_size.width, ctx.width) ||
                        !core::try_int_cast(image_size.height, ctx.height))) {
        ec = image::Error::image_size_out_of_range;
        return false;
    }

    // Progress tracking
    ctx.image = &image;
    ctx.num_rows_per_notification = std::max(4096 / image_size.width, 1);
    ctx.num_rows_total = image_size.height;

    std::unique_ptr<image::WritableImage> converted_image;
    const png_byte* buffer = nullptr;
    bool format_matched = false;
    {
        image::BufferFormat buffer_format = {};
        const void* buffer_2 = nullptr;
        if (ARCHON_LIKELY(image.try_get_buffer(buffer_format, buffer_2) &&
                          try_match_save_format(buffer_format, image_size, ctx.format, ctx.xforms,
                                                ctx.bytes_per_row))) { // Throws
            buffer = static_cast<const png_byte*>(buffer_2);
            format_matched = true;
        }
    }
    if (!format_matched) {
        // FIXME: Replicate indexed format if origin image is indirect color image and color space is Lum or RGB and bit depth is less than, or equal to 8 (requires that `is_indexed` and `palette_size` are added to `image::Image::TransferInfo`, requires also that functions are added to `image::Image` that allow for extraction of pixels as indexes into palette and extraction of palette entries)                                                       
        Format format = {};
        WriteTransformations xforms = {};
        image::Image::TransferInfo info = image.get_transfer_info(); // Throws
        bool use_rgb = !info.color_space->is_lum();
        if (ARCHON_LIKELY(use_rgb)) {
            format.color_type = (info.has_alpha ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB);
        }
        else {
            format.color_type = (info.has_alpha ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY);
        }
        bool use_short_int = false;
        bool use_subbyte_format = (format.color_type == PNG_COLOR_TYPE_GRAY && info.bit_depth <= 4);
        if (ARCHON_LIKELY(!use_subbyte_format)) {
            if (ARCHON_LIKELY(info.bit_depth <= 8)) {
                // Use 8-bit Lum, LumA, RGB, or RGBA format
                format.bit_depth = 8;
            }
            else {
                // Use 16-bit Lum, LumA, RGB, or RGBA format
                format.bit_depth = 16;
                if constexpr (png_16bit_as_short) {
                    core::Endianness byte_order = {};
                    if (ARCHON_UNLIKELY(core::try_get_byte_order<short>(byte_order))) {
                        use_short_int = true;
                        xforms.swap_bytes = (byte_order == core::Endianness::little);
                    }
                }
            }
        }
        else if (info.bit_depth <= 1) {
            // Use 1-bit Lum format
            format.bit_depth = 1;
        }
        else if (info.bit_depth <= 2) {
            // Use 2-bit Lum format
            format.bit_depth = 2;
        }
        else {
            // Use 4-bit Lum format
            format.bit_depth = 4;
        }
        png_byte* buffer_2 = nullptr;
        std::size_t bytes_per_row = 0;
        converted_image = create_image(image_size, format, use_short_int, buffer_2, bytes_per_row); // Throws
        image::Pos pos = { 0, 0 };
        bool blend = false;
        converted_image->put_image(pos, image, blend); // Throws
        ctx.image = converted_image.get();
        buffer = buffer_2;
        ctx.format = format;
        ctx.xforms = xforms;
        ctx.bytes_per_row = bytes_per_row;
    }

    ctx.rows = std::make_unique<const png_byte*[]>(std::size_t(image_size.height)); // Throws
    {
        const png_byte* row = buffer;
        for (std::size_t i = 0; i < std::size_t(image_size.height); ++i) {
            ctx.rows[i] = row;
            row += ctx.bytes_per_row;
        }
    }

    ctx.use_interlacing = config.use_adam7_interlacing;

    if (ARCHON_LIKELY(do_save(ctx))) // Throws
        return true; // Success

    if (ARCHON_UNLIKELY(ctx.exception))
        std::rethrow_exception(std::move(ctx.exception)); // Throws
    ARCHON_ASSERT(ctx.has_error);
    ec = ctx.ec;
    return false; // Failure
}



class FileFormatImpl final
    : public image::FileFormat {
public:
    auto get_ident() const noexcept -> std::string_view override
    {
        return g_file_format_ident;
    }

    auto get_descr() const -> std::string_view override
    {
        return g_file_format_descr;
    }

    auto get_mime_types() const noexcept -> core::Span<const std::string_view> override
    {
        return g_mime_types;
    }

    auto get_filename_extensions() const noexcept -> core::Span<const std::string_view> override
    {
        return g_filename_extensions;
    }

    bool is_available() const noexcept override
    {
        return true;
    }

    bool try_recognize(core::Source& source, bool& recognized, const std::locale&, log::Logger&,
                       std::error_code& ec) const override
    {
        return ::try_recognize(source, recognized, ec); // Throws
    }

    bool do_try_load(core::Source& source, std::unique_ptr<image::WritableImage>& image, const std::locale& loc,
                     log::Logger& logger, const LoadConfig& config, std::error_code& ec) const override
    {
        image::ProgressTracker* progress_tracker = config.progress_tracker;
        image::ImageProvider* image_provider = config.image_provider;
        const image::PNGLoadConfig* config_2 = nullptr;
        if (config.special) {
            const image::PNGLoadConfig* config_3 =
                config.special->get<const image::PNGLoadConfig>();
            if (config_3)
                config_2 = config_3;
        }
        std::optional<image::PNGLoadConfig> config_4;
        if (!config_2)
            config_2 = &config_4.emplace(); // Throws
        return ::load(source, image, loc, logger, progress_tracker, image_provider, *config_2, ec); // Throws
    }

    bool do_try_save(const image::Image& image, core::Sink& sink, const std::locale& loc, log::Logger& logger,
                     const SaveConfig& config, std::error_code& ec) const override
    {
        image::ProgressTracker* progress_tracker = config.progress_tracker;
        const image::PNGSaveConfig* config_2 = nullptr;
        if (config.special) {
            const image::PNGSaveConfig* config_3 = config.special->get<const image::PNGSaveConfig>();
            if (config_3)
                config_2 = config_3;
        }
        std::optional<image::PNGSaveConfig> config_4;
        if (!config_2)
            config_2 = &config_4.emplace(); // Throws
        return ::save(image, sink, loc, logger, progress_tracker, *config_2, ec); // Throws
    }
};


#else // !ARCHON_IMAGE_HAVE_PNG


class FileFormatImpl final
    : public image::FileFormat {
public:
    auto get_ident() const noexcept -> std::string_view override
    {
        return g_file_format_ident;
    }

    auto get_descr() const -> std::string_view override
    {
        return g_file_format_descr;
    }

    auto get_mime_types() const noexcept -> core::Span<const std::string_view> override
    {
        return g_mime_types;
    }

    auto get_filename_extensions() const noexcept -> core::Span<const std::string_view> override
    {
        return g_filename_extensions;
    }

    bool is_available() const noexcept override
    {
        return false;
    }

    bool try_recognize(core::Source&, bool&, const std::locale&, log::Logger&, std::error_code& ec) const override
    {
        ec = image::Error::file_format_unavailable;
        return false; // Failure
    }

    bool do_try_load(core::Source&, std::unique_ptr<image::WritableImage>&, const std::locale&, log::Logger&,
                     const LoadConfig&, std::error_code& ec) const override
    {
        ec = image::Error::file_format_unavailable;
        return false; // Failure
    }

    bool do_try_save(const image::Image&, core::Sink&, const std::locale&, log::Logger&, const SaveConfig&,
                     std::error_code& ec) const override
    {
        ec = image::Error::file_format_unavailable;
        return false; // Failure
    }
};


#endif // !ARCHON_IMAGE_HAVE_PNG


} // unnamed namespace


auto image::get_file_format_png() noexcept -> const image::FileFormat&
{
    static FileFormatImpl impl;
    return impl;
}
