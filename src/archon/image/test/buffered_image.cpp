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

#include <archon/core/memory.hpp>
#include <archon/core/text.hpp>
#include <archon/core/text_table.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/image/integer_buffer_format.hpp>

/*
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <X11/extensions/Xrender.h>
*/

#include <iostream>


using namespace archon::core;
using namespace archon::util;
using namespace archon::image;


namespace {

void read_tray(Image::ConstRefArg image, int left, int bottom, int width, int height)
{
    WordType component_type = image->get_word_type();
    int num_channels = image->get_num_channels();
    int pixel_size = num_channels * get_bytes_per_word(component_type);
    size_t num_pixels = height * size_t(width);
    size_t num_components = num_pixels * num_channels;
    std::unique_ptr<char[]> tray = std::make_unique<char[]>(num_pixels * pixel_size);
    std::unique_ptr<float[]> floats = std::make_unique<float[]>(num_components);

    for (std::size_t i = 0; i < num_components; ++i)
        floats[i] = 0.25;
    (*get_word_type_frac_converter(word_type_Float, component_type))(floats.get(), tray.get(),
                                                                     num_components);

    Image::CodecConstPtr codec(image->acquire_codec().release());
    codec->decode(TupleGrid(tray.get(), pixel_size, width * ssize_t(pixel_size)),
                  width, height, left, bottom);
    (*get_word_type_frac_converter(component_type, word_type_Float))(tray.get(), floats.get(),
                                                                     num_components);

    Text::Table table;
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            std::string v;
            for (int k = 0; k < num_channels; ++k) {
                if (!v.empty())
                    v += ", ";
                v += Text::print(floats[(i*size_t(width)+j) * num_channels + k]);
            }
            table.get_cell(i,j).set_text("("+v+")");
        }
    }
    std::cout << table.print()<<std::endl;
}


void bit_ordering()
{
    ColorSpace::ConstRef color_space = ColorSpace::get_Lum();
    bool has_alpha = false;

    IntegerBufferFormat::ChannelLayout channels(2);
    channels.bits_per_pixel = 8;

    std::vector<bool> endianness;

    BufferFormat::ConstRef buffer_format =
        IntegerBufferFormat::get_format(word_type_UChar,
                                        channels,
                                        false,                       // mostSignificantBitsFirst
                                        true,                        // wordAlignedStrips
                                        endianness);

    unsigned char buffer[] = { 32, 64, 128 };

    BufferedImage::Ref image =
        BufferedImage::new_image(buffer, 3, 1, color_space, has_alpha, buffer_format);

    read_tray(image, 0, 0, 3, 1);
}


unsigned char buffer[] = {
    // -    128 64 32    -    16 8 4      -    2 1 # 128    -    64 32 16      -    8 4 2      -    1 # 128 64      -    32 16 8      -    4 2 1
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,
    /* 0 */           /* 1 */      4 + /* 2 */   1 ,     /* 3 */    32+16 + /* 4 */ 8     + /* 5 */ 1 ,     64 + /* 6 */ 32+16   + /* 7 */ 4+2+1,

    // 1 8 64 - 2 16 128 - 4 32
    //       8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 8+128, 6+64+128, 2+24+32+64+128, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000,
    000, 000, 000, 000, 000, 000, 000, 000
};


void test_1()
{
    std::size_t words_in_buffer = sizeof buffer / sizeof *buffer;
    std::cerr << "Buffer: " << static_cast<void*>(buffer) << " -> " << static_cast<void*>(buffer + words_in_buffer) << " (size: " << words_in_buffer << " x " << sizeof *buffer << " = " << sizeof buffer << ")\n";
    ColorSpace::ConstRef color_space = ColorSpace::get_Lum();
    bool has_alpha = false;
/*
//    buffer_format.top_to_bottom = true;
    buffer_format.format_type = BufferFormat::tight;
//    buffer_format.word_type = BufferFormat::std_double;
//    buffer_format.bits_per_word = 128;
    buffer_format.pixel_size = 3;
    buffer_format.most_significant_bits_first = true;
//    std::cerr << "Buffer format: " << buffer_format.to_string()<<"\n";
*/

    IntegerBufferFormat::ChannelLayout channels(3);

    std::vector<bool> endianness(3, 0);
    endianness[2] = false;
    endianness[1] = false;
    endianness[0] = true;

    BufferFormat::ConstRef buffer_format =
        IntegerBufferFormat::get_format(word_type_UChar,
                                        channels,
                                        true,
                                        true,
                                        endianness); // , buffer_format, 0, 0, 0, 0, endianness
    BufferedImage::Ref image =
        BufferedImage::new_image(buffer, 2, 2, color_space, has_alpha, buffer_format);

    read_tray(image, 0, 0, 2, 2);
}

/*

ImageData getFormatOfImage(Image& image, bool assume_hsv = false)
{
    PixelFormat pixel_format;
    pixel_format.format_type = PixelFormat::tight;
    pixel_format.word_type = PixelFormat::custom_int;
    pixel_format.bits_per_word = 8;
    int w = image.get_bits_per_component();
    for (int i = 0; i < image.get_component_specifier(); ++i)
        pixel_format.channel_layout.push_back(PixelFormat::Channel(w*i, w));
    if (assume_hsv && (image.get_component_specifier() == 3 || image.get_component_specifier() == 4))
        pixel_format.color_scheme = PixelFormat::hsv;
    ImageData::BufferFormat buffer_format;
    std::cerr << "Pixel format: "<<pixel_format.to_string()<<"\n";
    return ImageData(image.get_pixel_buffer(), image.get_width(), image.get_height(), pixel_format, buffer_format, 0, 0, 0, 0);
}

void test_2()
{
    std::cerr << "\ntest_2:\n\n";
    Image image("../../../../scenes/img/alley_baggett.jpg");
    ImageData data = get_format_of_image(image);
    std::size_t n = 3;
    std::size_t m = image.get_component_specifier();
    std::unique_ptr<double[]> tray = std::make_unique<double[]>(n*m);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < m; ++j)
            tray[i*m+j] = j/(m-1.0);
    }
    data.put_pixels(0, 0, tray.get(), n, 1);
    image.save("out2.png");
}

void test_3()
{
    std::cerr << "\ntest_3:\n\n";
    Image image1("../../../../scenes/img/pattern.png");
    ImageData data1 = get_format_of_image(image1);
    Image image2("../../../../scenes/img/alley_baggett.jpg");
    ImageData data2 = get_format_of_image(image2);

    size_t w = 128;
    size_t h = 128;
    size_t n = w*h;
    size_t m = image1.get_component_specifier();
    std::unique_ptr<double[]> tray = std::make_unique<double[]>(n*m);

    data1.get_pixels(-32, -32, tray.get(), w, h, 1, 1);
    data2.put_pixels(128, 128, tray.get(), w, h);

    image2.save("out3.png");
}

void test_4()
{
    std::cerr << "\ntest_4:\n\n";
    Image image1("../../../../scenes/img/alley_baggett.jpg");
    ImageData data1 = get_format_of_image(image1);
    Image image2("../../../../scenes/img/a.png");
    ImageData data2 = get_format_of_omage(image2);

    data2.put_image(&data1);

    image2.save("out4.png");
}

void test_5()
{
    std::cerr << "\ntest_5:\n\n";
    Image image1("../../../../scenes/img/pattern_gray_alpha.png");
    ImageData data1 = get_format_of_image(image1);
    Image image2("../../../../scenes/img/alley_baggett.jpg");
    ImageData data2 = get_format_of_image(image2);

    data2.put_image(&data1,
                   32, 32,
                   32, 32,
                   0, 0,
                   1, 1,
                   0, 0);

    image2.save("out5.png");
}

void test_6()
{
    std::cerr << "\ntest_6:\n\n";
    Image image1("../../../../scenes/img/alley_baggett.jpg");
    ImageData data1 = get_format_of_image(image1);
    Image image2("../../../../scenes/img/alley_baggett.jpg");
    ImageData data2 = get_format_of_image(image2, true);

    data2.put_image(&data1);
    data1.put_image(&data2);

    image2.save("out6-1.png");
    image1.save("out6-2.png");
}

void test_7()
{
    std::cerr << "\ntest_7:\n\n";
    Image image1("alley_baggett.png");
    ImageData data1 = get_format_of_image(image1);

    PixelFormat pixel_format;
    pixel_format.word_wype = PixelFormat::std_int;
    pixel_format.format_type = PixelFormat::packed;
    pixel_format.pixel_size = 1;
    pixel_format.color_scheme = PixelFormat::rgb;
    pixel_format.has_alpha_channel = true;
    ImageData::BufferFormat buffer_format;
    std::cerr << "Transient 1 pixel format: "<<pixel_format.to_string()<<"\n";
    ImageData data2(0, image1.get_width(), image1.get_height(), pixel_format, buffer_format); // No buffer yet
    std::size_t bufferSize = data2.get_minimum_buffer_size_in_words();
    std::cerr << "Buffer size: "<<buffer_size<<" words ("<<bufferSize * pixel_format.get_expanded_format().bits_per_word / 8<<" bytes)\n";
    std::unique_ptr<long[]> buffer = std::make_unique<long[]>(data2.get_minimum_buffer_size_in_words());
    data2.set_buffer(buffer.get());

    pixel_format.most_significant_bits_first = true;
    std::cerr << "Transient 2 pixel format: "<<pixel_format.to_string()<<"\n";
    ImageData data3(buffer.get(), image1.get_width(), image1.get_height(), pixel_format, buffer_format);
    if (buffer_size < data3.get_minimum_buffer_size_in_words())
        throw "Buffer size mismatch";

    data2.put_image(&data1);

    data1.put_image(&data2);
    image1.save("out7-1.png");

    data1.put_image(&data3);
    image1.save("out7-2.png");
}

void test_8()
{
    std::cerr << "\ntest_8:\n\n";
    Image image1("interp-tester.png");
    ImageData data1 = get_format_of_image(image1);

    int width = 64;
    int height = 64;
    Image image2(width, height, image1.get_component_specifier());
    ImageData data2 = get_format_of_image(image2);

    std::unique_ptr<double[]> pixel = std::make_unique<double[]>(image1.get_component_specifier());

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            data1.interpolate_linear((x+0.5) / width, (y+0.5) / height, pixel.get());
            data2.put_pixels(x, y, pixel.get());
        }
    }
    image2.save("out8-1.png");

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            data1.interpolate_cubic((x+0.5) / width, (y+0.5) / height, pixel.get());
            data2.put_pixels(x, y, pixel.get());
        }
    }
    image2.save("out8-2.png");
}

void test_9()
{
    std::cerr << "\ntest_9:\n\n";

    Display* dpy = XOpenDisplay(0);

    Visual* vis = DefaultVisual(dpy, 0);

    XSetWindowAttributes swa;
    swa.event_mask = FocusChangeMask | EnterWindowMask | LeaveWindowMask |
        ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
        KeyPressMask | KeyRelease | StructureNotifyMask;
    swa.background_pixmap = None;
    swa.background_pixel  = 0;
    swa.border_pixel = 0;
    swa.colormap = XCreateColormap(dpy, RootWindow(dpy, 0), vis, AllocNone);

    std::cerr << "Default window depth: "<<DefaultDepth(dpy, 0)<<"\n";

    Window win = XCreateWindow(dpy, RootWindow(dpy, 0), 0, 0, 512, 512, 0, DefaultDepth(dpy, 0) *//* depth *//*,
                               InputOutput, vis, CWEventMask|CWBackPixmap|
                               CWBorderPixel|CWColormap, &swa);

    std::cerr << "Window created: "<<win<<"\n";
*/
/*
    XSizeHints size_hints;
    size_hints.flags = 0;
    if (true)
        size_hints.flags |= USPosition; /// \todo What condition should control this?
    if (true)
        size_hints.flags |= USSize;     /// \todo What condition should control this?
    size_hints.x      = left;
    size_hints.y      = top;
    size_hints.width  = width;
    size_hints.height = height;

    XWMHints wm_hints;
    wm_hints.flags = StateHint;
    wm_hints.initial_state = (false ? IconicState : NormalState); /// \todo What condition should control this?

    XTextProperty text_property;
    const char* t = title.c_str();
    XStringListToTextProperty(const_cast<char**>(&t), 1, &text_property);

    XSetWMProperties(dpy, win,
                     &text_property, // Window title
                     &text_property, // Icon title
                     0,             // argv (of application)
                     0,             // argc (of application)
                     &size_hints,
                     &wm_hints,
                     0);            // Class hints
    XFree(text_property.value);
*/

/*
// Ask X to notify rather than kill us when user attempts to close the window.
Atom delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
XSetWMProtocols(dpy, win, &delete_window, 1);
*/

/*
    XMapWindow(dpy, win);
    std::cerr << "Window mapped\n";
    std::cerr << "Default byte order = "<<(ImageByteOrder(dpy)==MSBFirst?"MSBFirst":(ImageByteOrder(dpy)==LSBFirst?"LSBFirst":"Unknown"))<<"\n";
    std::cerr << "Default visualid = "<<vis->visualid<<"\n";
    XFlush(dpy); // It seems that a flush must be performed before
    // putting images into the window. OUCH - that is not
    // enough either - it improves the chance of a good
    // result but is is not perfect - sometimes the
    // window ends up only partially filled with the
    // image.

    Image image("a6007b-tester.png");
    XImage ximage;
    ximage.width = image.getWidth();
    ximage.height = image.getHeight();          // size of image
    ximage.xoffset = 0;         // number of pixels offset in X direction
    ximage.format = ZPixmap;            // XYBitmap, XYPixmap, ZPixmap
    ximage.data = image.getPixelBuffer(); // pointer to image data
    ximage.byte_order = MSBFirst;       // data byte order, LSBFirst, MSBFirst
    ximage.bitmap_unit = 8;             // quant. of scanline 8, 16, 32
    ximage.bitmap_bit_order = LSBFirst; // LSBFirst, MSBFirst
    ximage.bitmap_pad = 32;             // 8, 16, 32 either XY or ZPixmap
    ximage.depth = DefaultDepth(dpy, 0);// depth of image
    ximage.bytes_per_line = image.getCharsPerRow(); // accelerator to next scanline
    ximage.bits_per_pixel = 24; // bits per pixel (ZPixmap)
    ximage.red_mask   = 0x3 << 0;       // bits in z arrangement
    ximage.green_mask = 0x3 << 2;
    ximage.blue_mask  = 0x3 << 4;
    XInitImage(&ximage);
*/
/*

Have only been able to test 24 bits depth mode. I do not know if the propriatary
driver from ATI has any impact on this.

"depth" Must match the dpet of the visual of the window on which the image is
put.

Masks are totally ignored.

bits-per-pixel determines the bit-level advance in the image data buffer for
each pixel, but regardless of this, bytes are always read and used in their
entirety. No bit shifting. Say the accumulated bit-level position for some pixel
is N, then the first used byte for this pixel is at buffer offset N/8 (integer
division) and the number of bytes read for this pixel is bits-per-pixel/8
(integer division).

byte_order is ignored unless bits-per-pixel is 16, 24 or 32. In which case it
determines the order in which the bytes are assmbled into an RGB triplet. If
byteorder is LSBFirst then the first byte becomes blue, the second one (if
present) becomes green and the third one (if present) becomes red. If the
byteorder is MSBFirst then the last byte becomes blue, the previous byte (if
present) becomes green and the second previous byte (if present) becomes red.

In conclusion, only bits-per-pixel values of 8, 16, 24 and 32 are meaningfull.

*/

/*
    std::cerr << "Image initialized\n";

    XPutImage(dpy, win, DefaultGC(dpy, 0), &ximage, 0, 0, 0, 0, image.get_width(), image.get_height());
    std::cerr << "Image written\n";

    sleep(100);
}
*/

} // unnamed namespace


int main()
{
    bit_ordering();

    test_1();

    //test_2();

    //test_3();

    //test_4();

    //test_5();

    //test_6();

    //test_7();

    //test_8();

    //test_9();
}
