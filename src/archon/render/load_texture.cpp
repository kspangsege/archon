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

#include <archon/core/features.h>
#include <archon/core/type_traits.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/image.hpp>
#include <archon/display/opengl.hpp>
#include <archon/render/load_texture.hpp>


#if ARCHON_DISPLAY_HAVE_OPENGL

using namespace archon;


namespace {


// ASSUMPTION: When using GL_INT, GL_UNSIGNED_INT, or GL_UNSIGNED_INT_* as `type` argument
// for glTexImage2D(), it always means exactly 4 bytes per component / bit compound and
// OpenGL will access the passed memory as a sequence of bytes rather than as a sequence of
// objects of type GLint or GLuint. Similarly, when using GL_SHORT, GL_UNSIGNED_SHORT, or
// GL_UNSIGNED_SHORT_*, it always means exactly 2 bytes per component / bit compound and
// memory will be accessed as a sequence of bytes rather than as a sequence of objects of
// type GLshort or GLushort. This assumption is motivated by the fact that it is possible to
// specify that bytes occur in reverse order (GL_UNPACK_SWAP_BYTES). The point is that
// swapped byte order strongly suggests that OpenGL has a bytes point of view of the passed
// data, and it would be strange / unsafe for OpenGL to access memory as a sequence of GLint
// objects in a case where the byte order is reversed relative to the native byte order of
// GLint. It would also be unreasonable application behavior to supply memory that is
// actually a sequence of GLint objects, but have component values scrambled due to reversed
// byte order.
//
// ASSUMPTION: If a byte has more than 8 bits, OpenGL considers only the bits at the 8 least
// significant bit positions.
//
// ASSUMPTION: The default byte order, that is, when GL_UNPACK_SWAP_BYTES is false, is the
// naitive byte order on the target platform for the type than corresponds to the specified
// `type` argument (GLint for GL_INT, etc.).
//


struct gl_format {
    GLint internal_format;
    GLenum format;
    GLenum type;
    GLint alignment;
};


bool try_map_integer_format(const image::BufferFormat::IntegerFormat& format, std::optional<bool> want_alpha,
                            gl_format& format_2)
{
/*
    // FIXME: In OpenGL ES 3.2, only GL_UNSIGNED_BYTE is supported in combination with one
    // of the sRGB internal formats (GL_SRGB8 or GL_SRGB8_ALPHA8)                      
    //
    if (format.bits_per_word == 8) {
        GLenum type = {};
        core::Endianness byte_order = core::Endianness::big;
        if (ARCHON_LIKELY(format.words_per_channel == 1)) {
            type = GL_UNSIGNED_BYTE;
            goto proceed_1;
        }
        else if (ARCHON_LIKELY(format.words_per_channel == 2)) {
            if (sizeof (GLushort) == 2 && core::try_get_byte_order<GLushort>(byte_order)) {
                type = GL_UNSIGNED_SHORT;
                goto proceed_1;
            }
        }
        else if (ARCHON_LIKELY(format.words_per_channel == 4)) {
            if (sizeof (GLuint) == 4 && core::try_get_byte_order<GLuint>(byte_order)) {
                type = GL_UNSIGNED_INT;
                goto proceed_1;
            }
        }
        return false;

      proceed_1:
        GLint internal_format = {};
        GLenum format_3 = {};
        const image::BufferFormat::ChannelConf& channel_conf = format.channel_conf;
        // Since OpenGL 3.1, luminance (grayscale) has been deprecated as an external color
        // space, so only RGB is supported now.
        if (ARCHON_LIKELY(channel_conf.color_space->is_rgb())) {
            // FIXME: Would it be better to use the sized internal formats, GL_SRGB8 and
            // GL_SRGB8_ALPHA8, here?
            if (!channel_conf.has_alpha) {
                internal_format =  GL_SRGB;
                format_3 = (channel_conf.reverse_order ? GL_BGR : GL_RGB);
                goto proceed_2;
            }
            // OpenGL only supports formats where the alpha channel occurs last
            bool alpha_is_last = (channel_conf.alpha_first == channel_conf.reverse_order);
            if (ARCHON_LIKELY(alpha_is_last)) {
                internal_format =  GL_SRGB_ALPHA;
                format_3 = (channel_conf.reverse_order ? GL_BGRA : GL_RGBA);
                goto proceed_2;
            }
        }
        return false;

      proceed_2:
        bool swap_bytes = (format.words_per_channel != 1 && format.word_order != byte_order);
        format_2.internal_format = internal_format;
        format_2.format = format_3;
        format_2.type = type;
        format_2.swap_bytes = GLint(swap_bytes ? 1 : 0);
        format_2.lsb_first = 0;
        format_2.alignment = GLint(format.words_per_channel);
        return true;
    }
    return false;
*/

    static_cast<void>(format);    
    static_cast<void>(want_alpha);    
    static_cast<void>(format_2);    
    return false;
}


bool try_map_packed_format(const image::BufferFormat::PackedFormat& format, std::optional<bool> want_alpha,
                           gl_format& format_2)
{
/*    
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
    //
    // GL_UNSIGNED_BYTE_3_3_2
    // GL_UNSIGNED_BYTE_2_3_3_REV
    // GL_UNSIGNED_SHORT_5_6_5
    // GL_UNSIGNED_SHORT_5_6_5_REV
    //
    // GL_UNSIGNED_SHORT_4_4_4_4
    // GL_UNSIGNED_SHORT_4_4_4_4_REV
    // GL_UNSIGNED_SHORT_5_5_5_1
    // GL_UNSIGNED_SHORT_1_5_5_5_REV
    // GL_UNSIGNED_INT_8_8_8_8
    // GL_UNSIGNED_INT_8_8_8_8_REV
    // GL_UNSIGNED_INT_10_10_10_2
    // GL_UNSIGNED_INT_2_10_10_10_REV
    //
    // FIXME: In OpenGL ES 3.2, no packed formats are supported in combination with any of
    // the sRGB internal formats (GL_SRGB8 or GL_SRGB8_ALPHA8)                       
    //
    if (format.bits_per_word == 8) {
        GLenum type = {};
        core::Endianness byte_order = core::Endianness::big;
        if (ARCHON_LIKELY(format.words_per_pixel == 1)) {
            if ()
            type = GL_UNSIGNED_BYTE;
            goto proceed_1;
        }
        else if (ARCHON_LIKELY(format.words_per_channel == 2)) {
            if (sizeof (GLushort) == 2 && core::try_get_byte_order<GLushort>(byte_order)) {
                type = GL_UNSIGNED_SHORT;
                goto proceed_1;
            }
        }
        else if (ARCHON_LIKELY(format.words_per_channel == 4)) {
            if (sizeof (GLuint) == 4 && core::try_get_byte_order<GLuint>(byte_order)) {
                type = GL_UNSIGNED_INT;
                goto proceed_1;
            }
        }
        return false;

    
*/

    static_cast<void>(format);    
    static_cast<void>(want_alpha);    
    static_cast<void>(format_2);    
    return false;
}


template<class F>
bool try_map_integer_based_format(const F& format, std::optional<bool> want_alpha, gl_format& format_2)
{
    image::BufferFormat::IntegerType word_type = format.word_type;
    bool is_byte = (word_type == image::BufferFormat::IntegerType::byte);
    {
        image::BufferFormat::IntegerFormat format_3 = {};
        if (format.try_cast_to(format_3, word_type)) {
            if (ARCHON_LIKELY(try_map_integer_format(format_3, want_alpha, format_2)))
                return true;
        }
        if (!is_byte && format.try_cast_to(format_3, image::BufferFormat::IntegerType::byte)) {
            if (ARCHON_LIKELY(try_map_integer_format(format_3, want_alpha, format_2)))
                return true;
        }
    }
    {
        image::BufferFormat::PackedFormat format_3 = {};
        if (format.try_cast_to(format_3, word_type)) {
            if (ARCHON_LIKELY(try_map_packed_format(format_3, want_alpha, format_2)))
                return true;
        }
        if (!is_byte && format.try_cast_to(format_3, image::BufferFormat::IntegerType::byte)) {
            if (ARCHON_LIKELY(try_map_packed_format(format_3, want_alpha, format_2)))
                return true;
        }
    }
    // Subword formats (GL_BITMAP) unavailable since OpenGL 3.1
    return false;
}


bool try_map_float_format(const image::BufferFormat::FloatFormat& format, std::optional<bool> want_alpha,
                          gl_format& format_2)
{
    static_cast<void>(format);    
    static_cast<void>(want_alpha);    
    static_cast<void>(format_2);    
    return false;
}


bool try_map_format(const image::BufferFormat& format, std::optional<bool> want_alpha, gl_format& format_2)
{
    switch (format.type) {
        case image::BufferFormat::Type::integer:
            return try_map_integer_based_format(format.integer, want_alpha, format_2); // Throws
        case image::BufferFormat::Type::packed:
            return try_map_integer_based_format(format.packed, want_alpha, format_2); // Throws
        case image::BufferFormat::Type::subword:
            return try_map_integer_based_format(format.packed, want_alpha, format_2); // Throws
        case image::BufferFormat::Type::float_:
            return try_map_float_format(format.float_, want_alpha, format_2); // Throws
        case image::BufferFormat::Type::indexed:
            // Indexed color formats unavailable since OpenGL 3.1
            return false;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


template<class F> auto create_image_1(image::Size size, const void*& buffer) -> std::unique_ptr<image::WritableImage>
{
    using format_type = F;
    using image_type = image::BufferedImage<format_type>;
    std::unique_ptr<image_type> image = std::make_unique<image_type>(size); // Throws
    buffer = image->get_buffer().data();
    return image;
}


template<class W, int B>
auto create_image_2(image::Size size, bool has_alpha, const void*& buffer) -> std::unique_ptr<image::WritableImage>
{
    using word_type = W;
    static constexpr int bits_per_word = B;
    if (!has_alpha) {
        using format_type = image::IntegerPixelFormat_RGB<word_type, bits_per_word>;
        return create_image_1<format_type>(size, buffer); // Throws
    }
    {
        using format_type = image::IntegerPixelFormat_RGBA<word_type, bits_per_word>;
        return create_image_1<format_type>(size, buffer); // Throws
    }
}


auto copy_image(const image::Image& image, std::optional<bool> want_alpha, const void*& buffer,
                gl_format& format) -> std::unique_ptr<image::Image>
{
    // Since OpenGL 3.1, luminance (grayscale) has been deprecated as an external color
    // space, so only RGB is supported now.

    image::TransferInfo info = image.get_transfer_info();
    bool want_alpha_2 = want_alpha.value_or(info.has_alpha);
    GLint internal_format = (want_alpha_2 ? GL_SRGB8_ALPHA8 : GL_SRGB8);
    GLenum format_2 = (want_alpha_2 ? GL_RGBA : GL_RGB);

    constexpr bool can_use_short = (sizeof (GLushort) == 2);
    constexpr bool can_use_int   = (sizeof (GLuint) == 4);

    int min_short_depth = 8 + 1;
    int min_int_depth = (can_use_short ? 16 + 1 : min_short_depth);

    GLenum type = {};
    GLint alignment = {};
    int depth = info.bit_depth;
    image::Size size = image.get_size();
    const void* buffer_2 = {};
    std::unique_ptr<image::WritableImage> image_2;
    if constexpr (can_use_int) {
        if (ARCHON_UNLIKELY(depth >= min_int_depth)) {
            image_2 = create_image_2<GLuint, 32>(size, want_alpha_2, buffer_2); // Throws
            type = GL_UNSIGNED_INT;
            alignment = 4;
            goto proceed;
        }
    }
    if constexpr (can_use_short) {
        if (ARCHON_UNLIKELY(depth >= min_short_depth)) {
            image_2 = create_image_2<GLushort, 16>(size, want_alpha_2, buffer_2); // Throws
            type = GL_UNSIGNED_SHORT;
            alignment = 2;
            goto proceed;
        }
    }
    {
        image_2 = create_image_2<char, 8>(size, want_alpha_2, buffer_2); // Throws
        type = GL_UNSIGNED_BYTE;
        alignment = 1;
    }

  proceed:
    image::Pos pos = { 0, 0 };
    image_2->put_image(pos, image); // Throws
    buffer = buffer_2;
    format = {
        internal_format,
        format_2,
        type,
        alignment,
    };
    return image_2;
}


} // unnamed namespace



void render::load_and_configure_texture(const image::Image& image, bool no_interp, bool no_mipmap)
{
    render::load_texture(image); // Throws

    if (ARCHON_LIKELY(!no_mipmap))
        glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (no_interp ? GL_NEAREST : GL_LINEAR));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (no_mipmap ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR));
}


void render::load_texture(const image::Image& image)
{
    std::optional<bool> want_alpha; // Let it be up to the image
    image::BufferFormat format;
    const void* buffer = {};
    gl_format format_2 = {};
    bool have_buffer = (image.try_get_buffer(format, buffer) &&
                        try_map_format(format, want_alpha, format_2)); // Throws
    std::unique_ptr<image::Image> image_2;
    if (ARCHON_UNLIKELY(!have_buffer))
        image_2 = copy_image(image, want_alpha, buffer, format_2); // Throws

    image::Size image_size = image.get_size();
    GLsizei width = {};
    GLsizei height = {};
    core::int_cast(image_size.width, width); // Throws
    core::int_cast(image_size.height, height); // Throws

    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, format_2.alignment);

    GLint level = 0;
    GLint border = 0;
    glTexImage2D(GL_TEXTURE_2D, level, format_2.internal_format, width, height, border,
                 format_2.format, format_2.type, buffer);

    glPopClientAttrib();
}


void render::load_texture_layer(const image::Image& image, int layer, bool texture_has_alpha)
{
    std::optional<bool> want_alpha = texture_has_alpha;
    image::BufferFormat format;
    const void* buffer = {};
    gl_format format_2 = {};
    bool have_buffer = (image.try_get_buffer(format, buffer) &&
                        try_map_format(format, want_alpha, format_2)); // Throws
    std::unique_ptr<image::Image> image_2;
    if (ARCHON_UNLIKELY(!have_buffer))
        image_2 = copy_image(image, want_alpha, buffer, format_2); // Throws

    image::Size image_size = image.get_size();
    GLsizei width = {};
    GLsizei height = {};
    core::int_cast(image_size.width, width); // Throws
    core::int_cast(image_size.height, height); // Throws

    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, format_2.alignment);

    GLint zoffset = {};
    core::int_cast(layer, zoffset); // Throws

    GLint level = 0;
    GLint xoffset = 0;
    GLint yoffset = 0;
    GLsizei depth = 1;
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth,
                    format_2.format, format_2.type, buffer);

    glPopClientAttrib();
}


#endif // ARCHON_DISPLAY_HAVE_OPENGL
