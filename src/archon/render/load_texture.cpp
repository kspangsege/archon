// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <stdexcept>
#include <optional>
#include <initializer_list>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/image.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/image/integer_pixel_format.hpp>
#include <archon/image/float_pixel_format.hpp>
#include <archon/display/opengl.hpp>
#include <archon/render/load_texture.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL

using namespace archon;


namespace {


// NOTE: Section 3.7.2 "Transfer of Pixel Rectangles" of the OpenGL 4.1 specification
// (https://registry.khronos.org/OpenGL/specs/gl/glspec41.core.pdf) is of particular
// importance to the code below.


template<class W> constexpr auto map_integer_type() noexcept -> std::optional<image::BufferFormat::IntegerType>
{
    image::BufferFormat::IntegerType type = {};
    if (ARCHON_LIKELY(image::BufferFormat::try_map_integer_type<W>(type)))
        return type;
    return {};
}


template<class W> constexpr auto map_float_type() noexcept -> std::optional<image::BufferFormat::FloatType>
{
    image::BufferFormat::FloatType type = {};
    if (ARCHON_LIKELY(image::BufferFormat::try_map_float_type<W>(type)))
        return type;
    return {};
}


struct gl_format {
    GLenum format;
    GLenum type;
    GLint alignment;
    bool includes_alpha;
    bool uses_srgb;
};


bool try_match_integer_format(const image::BufferFormat::IntegerFormat& format, bool no_alpha, bool pres_prec,
                              gl_format& format_2)
{
    if (ARCHON_UNLIKELY(format.words_per_channel != 1))
        return false;

    if (ARCHON_UNLIKELY(pres_prec && format.bits_per_word > 8))
        return false;

    const image::BufferFormat::ChannelConf& channel_conf = format.channel_conf;
    if (ARCHON_UNLIKELY(!channel_conf.color_space->is_rgb()))
        return false;

    // FIXME: In OpenGL ES 3.2, only GL_UNSIGNED_BYTE is supported in combination with one
    // of the sRGB internal formats (GL_SRGB8 or GL_SRGB8_ALPHA8)                

    constexpr std::optional<image::BufferFormat::IntegerType> word_type_short = map_integer_type<GLushort>();
    constexpr std::optional<image::BufferFormat::IntegerType> word_type_int   = map_integer_type<GLuint>();

    GLenum type = {};
    if (image::BufferFormat::get_bytes_per_word(format.word_type) == 1 && format.bits_per_word == 8) {
        type = GL_UNSIGNED_BYTE;
        goto proceed_1;
    }
    if constexpr (word_type_short.has_value()) {
        if (format.word_type == word_type_short.value() && format.bits_per_word == 16) {
            type = GL_UNSIGNED_SHORT;
            goto proceed_1;
        }
    }
    if constexpr (word_type_int.has_value()) {
        if (format.word_type == word_type_int.value() && format.bits_per_word == 32) {
            type = GL_UNSIGNED_INT;
            goto proceed_1;
        }
    }
    return false;

  proceed_1:
    GLenum format_3 = {};
    if (!channel_conf.has_alpha) {
        format_3 = (channel_conf.reverse_order ? GL_BGR : GL_RGB);
        goto proceed_2;
    }
    if (!no_alpha && channel_conf.reverse_order == channel_conf.alpha_first) {
        format_3 = (channel_conf.reverse_order ? GL_BGRA : GL_RGBA);
        goto proceed_2;
    }
    return false;

  proceed_2:
    GLint alignment = 1; // No special row alignment
    bool includes_alpha = channel_conf.has_alpha;
    bool uses_srgb = true;
    format_2 = {
        format_3,
        type,
        alignment,
        includes_alpha,
        uses_srgb,
    };
    return true;
}


bool try_match_packed_format(const image::BufferFormat::PackedFormat& format, bool no_alpha, bool pres_prec,
                             gl_format& format_2)
{
    // In OpenGL's packed formats, the first number in the name always refers to the bit
    // field covering the bit positions of greatest significance, and the last number refers
    // to the bit field covering the bit positions of least significance.
    //
    // In names that do not carry the `_REV` suffix, the first number pertains to the
    // channel that is first with respect to the channel order specified by the `format`
    // argument (GL_RGB, GL_RGBA, or GL_BGRA). In names that do carry the `_REV` suffix, the
    // first number pertains instead to the last channel.
    //
    // With three channel packed formats, only a `type` argument of GL_RGB is allowed. With
    // four channel formats, the `type` argument can be either GL_RGBA or GL_BGRA.

    // FIXME: In OpenGL ES 3.2, no packed formats are supported in combination with any of
    // the sRGB internal formats (GL_SRGB8 or GL_SRGB8_ALPHA8)                       

    if (ARCHON_UNLIKELY(format.words_per_pixel != 1))
        return false;

    const image::BufferFormat::ChannelConf& channel_conf = format.channel_conf;
    if (ARCHON_UNLIKELY(!channel_conf.color_space->is_rgb()))
        return false;

    constexpr std::optional<image::BufferFormat::IntegerType> word_type_short = map_integer_type<GLushort>();
    constexpr std::optional<image::BufferFormat::IntegerType> word_type_int   = map_integer_type<GLuint>();

    int num_channels = channel_conf.get_num_channels();
    auto bit_fields_match = [&](std::initializer_list<int> field_widths) noexcept {
        return image::bit_fields_match(format.bit_fields.data(), num_channels, field_widths);
    };

    GLenum format_3 = {};
    GLenum type = {};
    bool alpha_actually_first = (channel_conf.reverse_order != channel_conf.alpha_first);
    if (image::BufferFormat::get_bytes_per_word(format.word_type) == 1) {
        if (bit_fields_match({ 3, 3, 2 }) && !channel_conf.reverse_order) {
            format_3 = GL_RGB;
            type = GL_UNSIGNED_BYTE_3_3_2;
            goto proceed;
        }
        if (bit_fields_match({ 2, 3, 3 }) && channel_conf.reverse_order) {
            format_3 = GL_RGB;
            type = GL_UNSIGNED_BYTE_2_3_3_REV;
            goto proceed;
        }
    }
    if constexpr (word_type_short.has_value()) {
        if (format.word_type == word_type_short.value()) {
            if (bit_fields_match({ 5, 6, 5 })) {
                format_3 = GL_RGB;
                type = (channel_conf.reverse_order ? GL_UNSIGNED_SHORT_5_6_5_REV : GL_UNSIGNED_SHORT_5_6_5);
                goto proceed;
            }
            if (!no_alpha) {
                if (bit_fields_match({ 4, 4, 4, 4 })) {
                    format_3 = (channel_conf.alpha_first ? GL_BGRA : GL_RGBA);
                    type = (alpha_actually_first ? GL_UNSIGNED_SHORT_4_4_4_4_REV : GL_UNSIGNED_SHORT_4_4_4_4);
                    goto proceed;
                }
                if (bit_fields_match({ 5, 5, 5, 1 }) && !alpha_actually_first) {
                    format_3 = (channel_conf.alpha_first ? GL_BGRA : GL_RGBA);
                    type = GL_UNSIGNED_SHORT_5_5_5_1;
                    goto proceed;
                }
                if (bit_fields_match({ 1, 5, 5, 5 }) && alpha_actually_first) {
                    format_3 = (channel_conf.alpha_first ? GL_BGRA : GL_RGBA);
                    type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
                    goto proceed;
                }
            }
        }
    }
    if constexpr (word_type_int.has_value()) {
        if (format.word_type == word_type_int.value() && !no_alpha) {
            if (bit_fields_match({ 8, 8, 8, 8 })) {
                format_3 = (channel_conf.alpha_first ? GL_BGRA : GL_RGBA);
                type = (alpha_actually_first ? GL_UNSIGNED_INT_8_8_8_8_REV : GL_UNSIGNED_INT_8_8_8_8);
                goto proceed;
            }
            if (!pres_prec) {
                if (bit_fields_match({ 10, 10, 10, 2 }) && !alpha_actually_first) {
                    format_3 = (channel_conf.alpha_first ? GL_BGRA : GL_RGBA);
                    type = GL_UNSIGNED_INT_10_10_10_2;
                    goto proceed;
                }
                if (bit_fields_match({ 2, 10, 10, 10 }) && alpha_actually_first) {
                    format_3 = (channel_conf.alpha_first ? GL_BGRA : GL_RGBA);
                    type = GL_UNSIGNED_INT_2_10_10_10_REV;
                    goto proceed;
                }
            }
        }
    }
    return false;

  proceed:
    GLint alignment = 1; // No special row alignment
    bool includes_alpha = channel_conf.has_alpha;
    bool uses_srgb = true;
    format_2 = {
        format_3,
        type,
        alignment,
        includes_alpha,
        uses_srgb,
    };
    return true;
}


template<class F>
bool try_match_integer_based_format(const F& format, std::optional<bool> want_srgb, bool no_alpha, bool pres_prec,
                                    gl_format& format_2)
{
    ARCHON_ASSERT(!want_srgb.has_value() || !pres_prec);
    bool want_srgb_2 = want_srgb.value_or(true);
    if (!want_srgb_2)
        return false;

    image::BufferFormat::IntegerType word_type = format.word_type;
    bool is_byte = (word_type == image::BufferFormat::IntegerType::byte);
    {
        image::BufferFormat::IntegerFormat format_3 = {};
        if (format.try_cast_to(format_3, word_type)) {
            if (ARCHON_LIKELY(try_match_integer_format(format_3, no_alpha, pres_prec, format_2)))
                return true;
        }
        if (!is_byte && format.try_cast_to(format_3, image::BufferFormat::IntegerType::byte)) {
            if (ARCHON_LIKELY(try_match_integer_format(format_3, no_alpha, pres_prec, format_2)))
                return true;
        }
    } {
        image::BufferFormat::PackedFormat format_3 = {};
        if (format.try_cast_to(format_3, word_type)) {
            if (ARCHON_LIKELY(try_match_packed_format(format_3, no_alpha, pres_prec, format_2)))
                return true;
        }
        if (!is_byte && format.try_cast_to(format_3, image::BufferFormat::IntegerType::byte)) {
            if (ARCHON_LIKELY(try_match_packed_format(format_3, no_alpha, pres_prec, format_2)))
                return true;
        }
    }
    // Subword formats (GL_BITMAP) unavailable since OpenGL 3.1
    return false;
}


bool try_match_float_format(const image::BufferFormat::FloatFormat& format, std::optional<bool> want_srgb,
                            bool no_alpha, gl_format& format_2)
{
    bool want_srgb_2 = want_srgb.value_or(false);
    if (ARCHON_UNLIKELY(want_srgb_2))
        return false;

    const image::BufferFormat::ChannelConf& channel_conf = format.channel_conf;
    if (ARCHON_UNLIKELY(!channel_conf.color_space->is_rgb()))
        return false;

    GLenum type = {};
    constexpr std::optional<image::BufferFormat::FloatType> word_type_float = map_float_type<GLfloat>();
    if constexpr (word_type_float.has_value()) {
        if (format.word_type == word_type_float.value()) {
            type = GL_FLOAT;
            goto proceed_1;
        }
    }
    return false;

  proceed_1:
    GLenum format_3 = {};
    if (!channel_conf.has_alpha) {
        format_3 = (channel_conf.reverse_order ? GL_BGR : GL_RGB);
        goto proceed_2;
    }
    if (!no_alpha && channel_conf.reverse_order == channel_conf.alpha_first) {
        format_3 = (channel_conf.reverse_order ? GL_BGRA : GL_RGBA);
        goto proceed_2;
    }
    return false;

  proceed_2:
    GLint alignment = 1; // No special row alignment
    bool includes_alpha = channel_conf.has_alpha;
    bool uses_srgb = false;
    format_2 = {
        format_3,
        type,
        alignment,
        includes_alpha,
        uses_srgb,
    };
    return true;
}


bool try_match_format(const image::BufferFormat& format, std::optional<bool> want_srgb, bool no_alpha, bool pres_prec,
                      gl_format& format_2)
{
    switch (format.type) {
        case image::BufferFormat::Type::integer:
            return try_match_integer_based_format(format.integer, want_srgb, no_alpha, pres_prec, format_2); // Throws
        case image::BufferFormat::Type::packed:
            return try_match_integer_based_format(format.packed, want_srgb, no_alpha, pres_prec, format_2); // Throws
        case image::BufferFormat::Type::subword:
            return try_match_integer_based_format(format.packed, want_srgb, no_alpha, pres_prec, format_2); // Throws
        case image::BufferFormat::Type::float_:
            return try_match_float_format(format.float_, want_srgb, no_alpha, format_2); // Throws
        case image::BufferFormat::Type::indexed:
            // Indexed color formats unavailable since OpenGL 3.1
            return false;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


template<class F> auto create_image(image::Size size, const void*& buffer) -> std::unique_ptr<image::WritableImage>
{
    using format_type = F;
    using image_type = image::BufferedImage<format_type>;
    std::unique_ptr<image_type> image = std::make_unique<image_type>(size); // Throws
    buffer = image->get_buffer().data();
    return image;
}


template<class W, int B>
auto create_integer_image(image::Size size, bool has_alpha,
                          const void*& buffer) -> std::unique_ptr<image::WritableImage>
{
    using word_type = W;
    static constexpr int bits_per_word = B;
    if (!has_alpha) {
        using format_type = image::IntegerPixelFormat_RGB<word_type, bits_per_word>;
        return create_image<format_type>(size, buffer); // Throws
    }
    {
        using format_type = image::IntegerPixelFormat_RGBA<word_type, bits_per_word>;
        return create_image<format_type>(size, buffer); // Throws
    }
}


template<class W>
auto create_float_image(image::Size size, bool has_alpha, const void*& buffer) -> std::unique_ptr<image::WritableImage>
{
    using word_type = W;
    if (!has_alpha) {
        using format_type = image::FloatPixelFormat_RGB<word_type>;
        return create_image<format_type>(size, buffer); // Throws
    }
    {
        using format_type = image::FloatPixelFormat_RGBA<word_type>;
        return create_image<format_type>(size, buffer); // Throws
    }
}


auto copy_image(const image::Image& image, std::optional<bool> want_srgb, bool no_alpha, bool pres_prec,
                const void*& buffer, gl_format& format) -> std::unique_ptr<image::Image>
{
    ARCHON_ASSERT(!want_srgb.has_value() || !pres_prec);

    image::TransferInfo info = image.get_transfer_info();
    bool incl_alpha = (info.has_alpha && !no_alpha);

    const void* buffer_2 = {};
    std::unique_ptr<image::WritableImage> image_2;
    GLenum type = {};

    image::Size size = image.get_size();
    bool want_srgb_2 = want_srgb.value_or(!pres_prec || info.bit_depth <= 8);
    if (ARCHON_LIKELY(want_srgb_2)) {
        static_assert(sizeof (GLubyte) == 1);
        image_2 = create_integer_image<char, 8>(size, incl_alpha, buffer_2); // Throws
        type = GL_UNSIGNED_BYTE;
    }
    else {
        image_2 = create_float_image<GLfloat>(size, incl_alpha, buffer_2); // Throws
        type = GL_FLOAT;
    }

    image::Pos pos = { 0, 0 };
    image_2->put_image(pos, image); // Throws

    GLenum format_2 = (incl_alpha ? GL_RGBA : GL_RGB);
    GLint alignment = 1;

    buffer = buffer_2;
    format = {
        format_2,
        type,
        alignment,
        incl_alpha,
        want_srgb_2,
    };
    return image_2;
}


} // unnamed namespace



void render::load_and_configure_texture(const image::Image& image, bool require_format_match, bool preserve_precision,
                                        bool no_interp, bool no_mipmap)
{
    render::load_texture(image, require_format_match, preserve_precision); // Throws

    if (ARCHON_LIKELY(!no_mipmap))
        glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (no_interp ? GL_NEAREST : GL_LINEAR));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (no_mipmap ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR));
}


void render::load_texture(const image::Image& image, bool require_format_match, bool preserve_precision)
{
    if (ARCHON_LIKELY(render::try_load_texture(image, require_format_match, preserve_precision))) // Throws
        return;
    throw std::runtime_error("Failed to match image format to supported OpenGL client-side format");
}


bool render::try_load_texture(const image::Image& image, bool require_format_match, bool preserve_precision)
{
    std::optional<bool> want_srgb; // Choose based on incoming image
    bool no_alpha = false; // Keep alpha channel if present
    image::BufferFormat format;
    const void* buffer = {};
    gl_format format_2 = {};
    bool have_buffer = (image.try_get_buffer(format, buffer) &&
                        try_match_format(format, want_srgb, no_alpha, preserve_precision, format_2)); // Throws
    std::unique_ptr<image::Image> image_2;
    if (ARCHON_UNLIKELY(!have_buffer)) {
        if (require_format_match)
            return false;
        image_2 = copy_image(image, want_srgb, no_alpha, preserve_precision, buffer, format_2); // Throws
    }

    GLint internal_format = (format_2.uses_srgb ?
                             (format_2.includes_alpha ? GL_SRGB8_ALPHA8 : GL_SRGB8) :
                             (format_2.includes_alpha ? GL_RGBA32F : GL_RGB32F));

    image::Size image_size = image.get_size();
    GLsizei width = {};
    GLsizei height = {};
    core::int_cast(image_size.width, width); // Throws
    core::int_cast(image_size.height, height); // Throws

    GLint orig_unpack_alignment = {};
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &orig_unpack_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, format_2.alignment);

    GLint level = 0;
    GLint border = 0;
    glTexImage2D(GL_TEXTURE_2D, level, internal_format, width, height, border, format_2.format, format_2.type, buffer);

    glPixelStorei(GL_UNPACK_ALIGNMENT, orig_unpack_alignment);

    return true;
}


bool render::try_load_texture_layer(const image::Image& image, int layer, bool require_format_match)
{
    image::Size image_size = image.get_size();
    GLsizei width = {};
    GLsizei height = {};
    core::int_cast(image_size.width, width); // Throws
    core::int_cast(image_size.height, height); // Throws

    GLint zoffset = {};
    core::int_cast(layer, zoffset); // Throws

    GLint level = 0;
    GLint width_2 = {};
    GLint height_2 = {};
    GLint depth = {};
    GLint internal_format = {};
    glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, level, GL_TEXTURE_WIDTH, &width_2);
    glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, level, GL_TEXTURE_HEIGHT, &height_2);
    glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, level, GL_TEXTURE_DEPTH, &depth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D_ARRAY, level, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

    if (ARCHON_UNLIKELY(width != width_2 || height != height_2))
        throw std::invalid_argument("Wrong image size");
    if (ARCHON_UNLIKELY(zoffset >= depth))
        throw std::invalid_argument("Layer index out of range");

    bool want_srgb = false;
    bool no_alpha = false;
    switch (internal_format) {
        case GL_SRGB8:
            want_srgb = true;
            no_alpha = true;
            break;
        case GL_SRGB8_ALPHA8:
            want_srgb = true;
            break;
        case GL_RGB32F:
            no_alpha = true;
            break;
        case GL_RGBA32F:
            break;
        default:
            throw std::invalid_argument("Unsupported internal format");
    }

    bool pres_prec = false;
    image::BufferFormat format;
    const void* buffer = {};
    gl_format format_2 = {};
    bool have_buffer = (image.try_get_buffer(format, buffer) &&
                        try_match_format(format, want_srgb, no_alpha, pres_prec, format_2)); // Throws
    std::unique_ptr<image::Image> image_2;
    if (ARCHON_UNLIKELY(!have_buffer)) {
        if (require_format_match)
            return false;
        image_2 = copy_image(image, want_srgb, no_alpha, pres_prec, buffer, format_2); // Throws
    }

    GLint orig_unpack_alignment = {};
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &orig_unpack_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, format_2.alignment);

    GLint xoffset = 0;
    GLint yoffset = 0;
    GLsizei depth_2 = 1;
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth_2,
                    format_2.format, format_2.type, buffer);

    glPixelStorei(GL_UNPACK_ALIGNMENT, orig_unpack_alignment);

    return true;
}


#endif // ARCHON_DISPLAY_HAVE_OPENGL
