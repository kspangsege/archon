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


using namespace std;
using namespace archon::core;
using namespace archon::util;
using namespace archon::image;


namespace
{
  void readTray(Image::ConstRefArg image, int left, int bottom, int width, int height)
  {
    WordType componentType = image->get_word_type();
    int numberOfChannels = image->get_num_channels();
    int pixelSize = numberOfChannels * get_bytes_per_word(componentType);
    size_t numberOfPixels = height * size_t(width); 
    size_t numberOfComponents = numberOfPixels * numberOfChannels;
    MemoryBuffer tray(numberOfPixels * pixelSize);
    Array<float> floats(numberOfComponents);

    for(size_t i=0; i<numberOfComponents; ++i) floats[i] = 0.25;
    (*get_word_type_frac_converter(word_type_Float, componentType))(floats.get(), tray.get(),
                                                                    numberOfComponents);

    Image::CodecConstPtr codec(image->acquire_codec().release());
    codec->decode(TupleGrid(tray.get(), pixelSize, width * ssize_t(pixelSize)),
                  width, height, left, bottom);
    (*get_word_type_frac_converter(componentType, word_type_Float))(tray.get(), floats.get(),
                                                                    numberOfComponents);

    Text::Table table;
    for(int i=0; i<height; ++i)
    {
      for(int j=0; j<width; ++j)
      {
        string v;
        for(int k=0; k<numberOfChannels; ++k)
        {
          if(!v.empty()) v += ", ";
          v += Text::print(floats[(i*size_t(width)+j) * numberOfChannels + k]);
        }
        table.get_cell(i,j).set_text("("+v+")");
      }
    }
    cout << table.print() << endl;
  }


  void bit_ordering()
  {
    ColorSpace::ConstRef  colorSpace = ColorSpace::get_Lum();
    bool hasAlpha = false;

    IntegerBufferFormat::ChannelLayout channels(2);
    channels.bits_per_pixel = 8;

    vector<bool> endianness;

    BufferFormat::ConstRef bufferFormat =
      IntegerBufferFormat::get_format(word_type_UChar,
                                      channels,
                                      false,                       // mostSignificantBitsFirst
                                      true,                        // wordAlignedStrips
                                      endianness);

    unsigned char buffer[] = { 32, 64, 128 };

    BufferedImage::Ref image =
      BufferedImage::new_image(buffer, 3, 1, colorSpace, hasAlpha, bufferFormat);

    readTray(image, 0, 0, 3, 1);
  }


  unsigned char buffer[] =
    {
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
    size_t wordsInBuffer = sizeof(buffer)/sizeof(*buffer);
    cerr << "Buffer: " << static_cast<void *>(buffer) << " -> " << static_cast<void *>(buffer + wordsInBuffer) << " (size: " << wordsInBuffer << " x " << sizeof(*buffer) << " = " << sizeof(buffer) << ")" << endl;
    ColorSpace::ConstRef  colorSpace = ColorSpace::get_Lum();
    bool hasAlpha = false;
/*
    //bufferFormat.topToBottom = true;
    bufferFormat.formatType = BufferFormat::tight;
    //bufferFormat.wordType = BufferFormat::std_double;
    //bufferFormat.bitsPerWord = 128;
    bufferFormat.pixelSize = 3;
    bufferFormat.mostSignificantBitsFirst = true;
    //cerr << "Buffer format: " << bufferFormat.toString() << endl;
*/

    IntegerBufferFormat::ChannelLayout channels(3);

    vector<bool> endianness(3, 0);
    endianness[2] = false;
    endianness[1] = false;
    endianness[0] = true;

    BufferFormat::ConstRef bufferFormat =
      IntegerBufferFormat::get_format(word_type_UChar,
                                      channels,
                                      true,
                                      true,
                                      endianness); // , bufferFormat, 0, 0, 0, 0, endianness
    BufferedImage::Ref image =
      BufferedImage::new_image(buffer, 2, 2, colorSpace, hasAlpha, bufferFormat);

    readTray(image, 0, 0, 2, 2);
  }

/*
  ImageData getFormatOfImage(Image &image, bool assumeHsv = false)
  {
    PixelFormat pixelFormat;
    pixelFormat.formatType = PixelFormat::tight;
    pixelFormat.wordType = PixelFormat::custom_int;
    pixelFormat.bitsPerWord = 8;
    int w = image.getBitsPerComponent();
    for(int i=0; i<image.getComponentSpecifier(); ++i) pixelFormat.channelLayout.push_back(PixelFormat::Channel(w*i, w));
    if(assumeHsv && (image.getComponentSpecifier() == 3 || image.getComponentSpecifier() == 4)) pixelFormat.colorScheme = PixelFormat::hsv;
    ImageData::BufferFormat bufferFormat;
    cerr << "Pixel format: " << pixelFormat.toString() << endl;
    return ImageData(image.getPixelBuffer(), image.getWidth(), image.getHeight(), pixelFormat, bufferFormat, 0, 0, 0, 0);
  }

  void test_2()
  {
    cerr << "\ntest_2:\n\n";
    Image image("../../../../scenes/img/alley_baggett.jpg");
    ImageData data = getFormatOfImage(image);
    size_t n = 3;
    size_t m = image.getComponentSpecifier();
    Array<double> tray(n*m);
    for(size_t i=0; i<n; ++i) for(size_t j=0; j<m; ++j) tray[i*m+j] = j/(m-1.0);
    data.putPixels(0, 0, tray.get(), n, 1);
    image.save("out2.png");
  }

  void test_3()
  {
    cerr << "\ntest_3:\n\n";
    Image image1("../../../../scenes/img/pattern.png");
    ImageData data1 = getFormatOfImage(image1);
    Image image2("../../../../scenes/img/alley_baggett.jpg");
    ImageData data2 = getFormatOfImage(image2);

    size_t w = 128;
    size_t h = 128;
    size_t n = w*h;
    size_t m = image1.getComponentSpecifier();
    Array<double> tray(n*m);

    data1.getPixels(-32, -32, tray.get(), w, h, 1, 1);
    data2.putPixels(128, 128, tray.get(), w, h);

    image2.save("out3.png");
  }

  void test_4()
  {
    cerr << "\ntest_4:\n\n";
    Image image1("../../../../scenes/img/alley_baggett.jpg");
    ImageData data1 = getFormatOfImage(image1);
    Image image2("../../../../scenes/img/a.png");
    ImageData data2 = getFormatOfImage(image2);

    data2.putImage(&data1);

    image2.save("out4.png");
  }

  void test_5()
  {
    cerr << "\ntest_5:\n\n";
    Image image1("../../../../scenes/img/pattern_gray_alpha.png");
    ImageData data1 = getFormatOfImage(image1);
    Image image2("../../../../scenes/img/alley_baggett.jpg");
    ImageData data2 = getFormatOfImage(image2);
    
    data2.putImage(&data1,
		   32, 32,
		   32, 32,
		   0, 0,
		   1, 1,
		   0, 0);

    image2.save("out5.png");
  }

  void test_6()
  {
    cerr << "\ntest_6:\n\n";
    Image image1("../../../../scenes/img/alley_baggett.jpg");
    ImageData data1 = getFormatOfImage(image1);
    Image image2("../../../../scenes/img/alley_baggett.jpg");
    ImageData data2 = getFormatOfImage(image2, true);
    
    data2.putImage(&data1);
    data1.putImage(&data2);

    image2.save("out6-1.png");
    image1.save("out6-2.png");
  }

  void test_7()
  {
    cerr << "\ntest_7:\n\n";
    Image image1("alley_baggett.png");
    ImageData data1 = getFormatOfImage(image1);

    PixelFormat pixelFormat;
    pixelFormat.wordType = PixelFormat::std_int;
    pixelFormat.formatType = PixelFormat::packed;
    pixelFormat.pixelSize = 1;
    pixelFormat.colorScheme = PixelFormat::rgb;
    pixelFormat.hasAlphaChannel = true;
    ImageData::BufferFormat bufferFormat;
    cerr << "Transient 1 pixel format: " << pixelFormat.toString() << endl;
    ImageData data2(0, image1.getWidth(), image1.getHeight(), pixelFormat, bufferFormat); // No buffer yet
    size_t bufferSize = data2.getMinimumBufferSizeInWords();
    cerr << "Buffer size: " << bufferSize << " words (" << bufferSize * pixelFormat.getExpandedFormat().bitsPerWord / 8 << " bytes)" << endl;
    Array<long> buffer(data2.getMinimumBufferSizeInWords());
    data2.setBuffer(buffer.get());

    pixelFormat.mostSignificantBitsFirst = true;
    cerr << "Transient 2 pixel format: " << pixelFormat.toString() << endl;
    ImageData data3(buffer.get(), image1.getWidth(), image1.getHeight(), pixelFormat, bufferFormat);
    if(bufferSize < data3.getMinimumBufferSizeInWords())
      throw "Buffer size mismatch";

    data2.putImage(&data1);

    data1.putImage(&data2);
    image1.save("out7-1.png");

    data1.putImage(&data3);
    image1.save("out7-2.png");
  }

  void test_8()
  {
    cerr << "\ntest_8:\n\n";
    Image image1("interp-tester.png");
    ImageData data1 = getFormatOfImage(image1);

    int width = 64;
    int height = 64;
    Image image2(width, height, image1.getComponentSpecifier());
    ImageData data2 = getFormatOfImage(image2);

    Array<double> pixel(image1.getComponentSpecifier());

    for(int y=0; y<height; ++y) for(int x=0; x<width; ++x)
    {
      data1.interpolateLinear((x+0.5) / width, (y+0.5) / height, pixel.get());
      data2.putPixels(x, y, pixel.get());
    }
    image2.save("out8-1.png");

    for(int y=0; y<height; ++y) for(int x=0; x<width; ++x)
    {
      data1.interpolateCubic((x+0.5) / width, (y+0.5) / height, pixel.get());
      data2.putPixels(x, y, pixel.get());
    }
    image2.save("out8-2.png");
  }

  void test_9()
  {
    cerr << "\ntest_9:\n\n";

    Display *dpy = XOpenDisplay(0);

    Visual *vis = DefaultVisual(dpy, 0);

    XSetWindowAttributes swa;
    swa.event_mask = FocusChangeMask | EnterWindowMask | LeaveWindowMask |
      ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
      KeyPressMask | KeyRelease | StructureNotifyMask;
    swa.background_pixmap = None;
    swa.background_pixel  = 0;
    swa.border_pixel = 0;
    swa.colormap = XCreateColormap(dpy, RootWindow(dpy, 0), vis, AllocNone);

cerr << "Default window depth: " << DefaultDepth(dpy, 0) << endl;

Window win = XCreateWindow(dpy, RootWindow(dpy, 0), 0, 0, 512, 512, 0, DefaultDepth(dpy, 0) *//* depth *//*,
			       InputOutput, vis, CWEventMask|CWBackPixmap|
			       CWBorderPixel|CWColormap, &swa);

cerr << "Window created: " << win << endl;
                                                                                                         */
    /*
    XSizeHints sizeHints;
    sizeHints.flags = 0;
    if(true) sizeHints.flags |= USPosition; /// \todo What condition should control this?
    if(true) sizeHints.flags |= USSize;     /// \todo What condition should control this?
    sizeHints.x      = left;
    sizeHints.y      = top;
    sizeHints.width  = width;
    sizeHints.height = height;

    XWMHints wmHints;
    wmHints.flags = StateHint;
    wmHints.initial_state = false ? IconicState : NormalState; /// \todo What condition should control this?

    XTextProperty textProperty;
    const char *t = title.c_str();
    XStringListToTextProperty(const_cast<char **>(&t), 1, &textProperty);

    XSetWMProperties(dpy, win,
		     &textProperty, // Window title
		     &textProperty, // Icon title
		     0,             // argv (of application)
		     0,             // argc (of application)
		     &sizeHints,
		     &wmHints,
		     0);            // Class hints
    XFree(textProperty.value);
    */

    /*
    // Ask X to notify rather than kill us when user attempts to close the window.
    Atom deleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &deleteWindow, 1);
    */

/*
    XMapWindow(dpy, win);
cerr << "Window mapped" << endl;
cerr << "Default byte order = " << (ImageByteOrder(dpy) == MSBFirst ? "MSBFirst" : ImageByteOrder(dpy) == LSBFirst ? "LSBFirst" : "Unknown") << endl;
cerr << "Default visualid = " << vis->visualid << endl;
    XFlush(dpy); // It seems that a flush must be performed before
		 // putting images into the window. OUCH - that is not
		 // enough either - it improves the chance of a good
		 // result but is is not perfect - sometimes the
		 // window ends up only partially filled with the
		 // image.

    Image image("a6007b-tester.png");
    XImage ximage;
    ximage.width = image.getWidth();
    ximage.height = image.getHeight();		// size of image
    ximage.xoffset = 0;		// number of pixels offset in X direction
    ximage.format = ZPixmap;		// XYBitmap, XYPixmap, ZPixmap
    ximage.data = image.getPixelBuffer(); // pointer to image data
    ximage.byte_order = MSBFirst;	// data byte order, LSBFirst, MSBFirst
    ximage.bitmap_unit = 8;		// quant. of scanline 8, 16, 32
    ximage.bitmap_bit_order = LSBFirst; // LSBFirst, MSBFirst
    ximage.bitmap_pad = 32;		// 8, 16, 32 either XY or ZPixmap
    ximage.depth = DefaultDepth(dpy, 0);// depth of image
    ximage.bytes_per_line = image.getCharsPerRow(); // accelerator to next scanline
    ximage.bits_per_pixel = 24; // bits per pixel (ZPixmap)
    ximage.red_mask   = 0x3 << 0;	// bits in z arrangement
    ximage.green_mask = 0x3 << 2;
    ximage.blue_mask  = 0x3 << 4;
    XInitImage(&ximage);
*/
    /*

    Have only been able to test 24 bits depth mode. I do not know if
    the propriatary driver from ATI has any impact on this.

    "depth" Must match the dpet of the visual of the window on which
    the image is put.

    Masks are totally ignored.

    bits-per-pixel determines the bit-level advance in the image data
    buffer for each pixel, but regardless of this, bytes are always
    read and used in their entirety. No bit shifting. Say the
    accumulated bit-level position for some pixel is N, then the first
    used byte for this pixel is at buffer offset N/8 (integer
    division) and the number of bytes read for this pixel is
    bits-per-pixel/8 (integer division).

    byte_order is ignored unless bits-per-pixel is 16, 24 or 32. In
    which case it determines the order in which the bytes are assmbled
    into an RGB triplet. If byteorder is LSBFirst then the first byte
    becomes blue, the second one (if present) becomes green and the
    third one (if present) becomes red. If the byteorder is MSBFirst
    then the last byte becomes blue, the previous byte (if present)
    becomes green and the second previous byte (if present) becomes
    red.

    In conclusion, only bits-per-pixel values of 8, 16, 24 and 32 are
    meaningfull.

    */

/*
cerr << "Image initialized" << endl;

    XPutImage(dpy, win, DefaultGC(dpy, 0), &ximage, 0, 0, 0, 0, image.getWidth(), image.getHeight());
cerr << "Image written" << endl;

    sleep(100);
  }
*/
}

int main() throw()
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

  return 0;
}
