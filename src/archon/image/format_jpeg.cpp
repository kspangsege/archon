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

#include <cstdio>
#include <csetjmp>
#include <limits>
#include <string>
#include <vector>
#include <iostream>

#include <jpeglib.h>
#include <jerror.h>

#include <archon/core/memory.hpp>
#include <archon/core/text.hpp>
#include <archon/core/blocking.hpp>
#include <archon/util/transcode.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/integer_buffer_format.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Util;
using namespace archon::Imaging;


namespace {

struct Context: jpeg_progress_mgr, jpeg_error_mgr {
    FileFormat::ProgressTracker* tracker;
    Logger* logger;        // For warnings
    jmp_buf setjmp_buffer; // For aborting from the LIBJPEG.
    // 0 for none, 1 for InterruptException, 2 for IOException,
    // 3 for other exceptions from the stream, 4 for libjpeg initiated
    // error or invalid format detected.
    int error_type;
    string error_message;  // For fatal errors
    string error_location; // Source file location from exception
    Context(FileFormat::ProgressTracker* tracker, Logger* logger):
        tracker(tracker),
        logger(logger),
        error_type(0)
    {
        // Start by defaulting everything, then override
        jpeg_std_error(this);
        progress_monitor = progress_callback;
        error_exit       = error_callback;
        output_message   = warning_callback;
    }

    void warning(string message)
    {
        if (logger)
            logger->log(message);
    }

    static void progress_callback(j_common_ptr cinfo) throw()
    {
        Context* c = static_cast<Context*>(cinfo->progress);
        double p = c->completed_passes + double(c->pass_counter)/c->pass_limit;
        c->tracker->progress(p/c->total_passes);
    }

    static void error_callback(j_common_ptr cinfo) throw()
    {
        Context* c = static_cast<Context*>(cinfo->err);
        {
            char buffer[JMSG_LENGTH_MAX];
            (*cinfo->err->format_message)(cinfo, buffer);
            c->error_type    = 4;
            c->error_message = buffer;
        }
        longjmp(c->setjmp_buffer, 1);
    }

    static void warning_callback(j_common_ptr cinfo) throw()
    {
        Context* c = static_cast<Context*>(cinfo->err);
        char buffer[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, buffer);
        c->warning(buffer);
    }
};

/**
 * The size of the buffers used for input/output streaming.
 */
const int buffer_size = 4096;

struct LoadContext: jpeg_source_mgr, Context {
    Array<JOCTET> buffer;
    bool start_of_file;        // have we gotten any data yet?
    InputStream &in;
    LoadContext(InputStream& i, FileFormat::ProgressTracker* tracker, Logger* logger):
        Context(tracker, logger),
        buffer(buffer_size),
        start_of_file(true),
        in(i)
    {
        init_source       = init_callback;
        fill_input_buffer = read_callback;
        skip_input_data   = skip_callback;
        resync_to_restart = jpeg_resync_to_restart; // use default method
        term_source       = term_callback;
        bytes_in_buffer   = 0; // forces fill_input_buffer on first read
        next_input_byte   = 0;
    }

    /**
     * From LIBJPEG/example.c:
     *
     * Initialize source --- called by jpeg_read_header before any
     * data is actually read.
     */
    static void init_callback(j_decompress_ptr /* cinfo */) throw()
    {
    }

    /**
     * From LIBJPEG/example.c:
     *
     * Fill the input buffer --- called whenever buffer is emptied.
     *
     * In typical applications, this should read fresh data into the buffer
     * (ignoring the current state of next_input_byte & bytes_in_buffer), reset
     * the pointer & count to the start of the buffer, and return TRUE
     * indicating that the buffer has been reloaded.  It is not necessary to
     * fill the buffer entirely, only to obtain at least one more byte.
     *
     * There is no such thing as an EOF return.  If the end of the file has been
     * reached, the routine has a choice of ERREXIT() or inserting fake data
     * into the buffer.  In most cases, generating a warning message and
     * inserting a fake EOI marker is the best course of action --- this will
     * allow the decompressor to output however much of the image is there.
     * However, the resulting error message is misleading if the real problem is
     * an empty input file, so we handle that case specially.
     *
     * In applications that need to be able to suspend compression due to input
     * not being available yet, a FALSE return indicates that no more data can
     * be obtained right now, but more may be forthcoming later.  In this
     * situation, the decompressor will return to its caller (with an indication
     * of the number of scanlines it has read, if any).  The application should
     * resume decompression after it has loaded more data into the input buffer.
     * Note that there are substantial restrictions on the use of suspension ---
     * see the documentation.
     *
     * When suspending, the decompressor will back up to a convenient restart
     * point (typically the start of the current MCU). next_input_byte &
     * bytes_in_buffer indicate where the restart point will be if the current
     * call returns FALSE.  Data beyond this point must be rescanned after
     * resumption, so move it to the front of the buffer rather than discarding
     * it.
     */
    static int read_callback(j_decompress_ptr cinfo) throw()
    {
        LoadContext* c = static_cast<LoadContext*>(cinfo->src);
        int n = 0;
        try {
            n = c->in.read(reinterpret_cast<char*>(c->buffer.get()),
                           buffer_size);
        }
        catch (InterruptException& e) {
            c->error_type = 1;
        }
        catch (ReadException& e) {
            c->error_type    = 2;
            c->error_message = e.what();
        }
        catch (exception& e) {
            c->error_type    = 3;
            c->error_message = e.what();
        }

        if (c->error_type == 1 || c->error_type == 2 && c->start_of_file || c->error_type == 3)
            longjmp(c->setjmp_buffer, 1);

        if (n < 1) {
            if (c->start_of_file)
                ERREXIT(cinfo, JERR_INPUT_EMPTY);

            if (c->error_type == 2) {
                c->warning(c->error_message);
            }
            else {
                WARNMS(cinfo, JWRN_JPEG_EOF);
            }

            // Insert a fake EOI marker
            c->buffer[0] = static_cast<JOCTET>(0xFF);
            c->buffer[1] = static_cast<JOCTET>(JPEG_EOI);
            n = 2;
        }

        c->next_input_byte = c->buffer.get();
        c->bytes_in_buffer = n;
        c->start_of_file = false;

        return 1;
    }

    /**
     * From LIBJPEG/example.c:
     *
     * Skip data --- used to skip over a potentially large amount of
     * uninteresting data (such as an APPn marker).
     *
     * Writers of suspendable-input applications must note that skip_input_data
     * is not granted the right to give a suspension return.  If the skip
     * extends beyond the data currently in the buffer, the buffer can be marked
     * empty so that the next read will cause a fill_input_buffer call that can
     * suspend.  Arranging for additional bytes to be discarded before reloading
     * the input buffer is the application writer's problem.
     */
    static void skip_callback(j_decompress_ptr cinfo, long num_bytes) throw()
    {
        LoadContext* c = static_cast<LoadContext*>(cinfo->src);

        /* Just a dumb implementation for now.  Could use fseek() except it
         * doesn't work on pipes.  Not clear that being smart is worth any
         * trouble anyway --- large skips are infrequent.
         */
        if (num_bytes > 0) {
            while (num_bytes > long(c->bytes_in_buffer)) {
                num_bytes -= long(c->bytes_in_buffer);
                read_callback(cinfo);
                /* note we assume that fill_input_buffer will never return
                 * FALSE, so suspension need not be handled.
                 */
            }
            c->next_input_byte += size_t(num_bytes);
            c->bytes_in_buffer -= size_t(num_bytes);
        }
    }

    /*
     * From LIBJPEG/example.c:
     *
     * An additional method that can be provided by data source modules is the
     * resync_to_restart method for error recovery in the presence of RST
     * markers.  For the moment, this source module just uses the default resync
     * method provided by the JPEG library.  That method assumes that no
     * backtracking is possible.
     */

    /**
     * From LIBJPEG/example.c:
     *
     * Terminate source --- called by jpeg_finish_decompress after all data has
     * been read. Often a no-op.
     *
     * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding application
     * must deal with any cleanup that should happen even for error exit.
     */
    static void term_callback(j_decompress_ptr /* cinfo */) throw() {}
};


struct SaveContext: jpeg_destination_mgr, Context {
    Array<JOCTET> buffer;
    OutputStream& out;
    SaveContext(OutputStream& o, FileFormat::ProgressTracker* tracker, Logger* logger):
        Context(tracker, logger),
        buffer(buffer_size),
        out(o)
    {
        init_destination    = init_callback;
        empty_output_buffer = write_callback;
        term_destination    = term_callback;
        next_output_byte    = buffer.get();
        free_in_buffer      = buffer_size;
    }

    /**
     * From LIBJPEG/example.c:
     *
     * Initialize destination --- called by jpeg_start_compress before any data
     * is actually written.
     */
    static void init_callback(j_compress_ptr /* cinfo */) throw()
    {
    }

    /**
     * From LIBJPEG/example.c:
     *
     * Empty the output buffer --- called whenever buffer fills up.
     *
     * In typical applications, this should write the entire output buffer
     * (ignoring the current state of next_output_byte & free_in_buffer), reset
     * the pointer & count to the start of the buffer, and return TRUE
     * indicating that the buffer has been dumped.
     *
     * In applications that need to be able to suspend compression due to output
     * overrun, a FALSE return indicates that the buffer cannot be emptied now.
     * In this situation, the compressor will return to its caller (possibly
     * with an indication that it has not accepted all the supplied scanlines).
     * The application should resume compression after it has made more room in
     * the output buffer.  Note that there are substantial restrictions on the
     * use of suspension --- see the documentation.
     *
     * When suspending, the compressor will back up to a convenient restart
     * point (typically the start of the current MCU). next_output_byte &
     * free_in_buffer indicate where the restart point will be if the current
     * call returns FALSE.  Data beyond this point will be regenerated after
     * resumption, so do not write it out when emptying the buffer externally.
     */
    static int write_callback(j_compress_ptr cinfo) throw()
    {
        SaveContext* c = static_cast<SaveContext*>(cinfo->dest);

        try {
            c->out.write(reinterpret_cast<const char*>(c->buffer.get()), buffer_size);
            c->next_output_byte = c->buffer.get();
            c->free_in_buffer   = buffer_size;
            return 1;
        }
        catch (InterruptException& e) {
            c->error_type = 1;
        }
        catch (WriteException& e) {
            c->error_type    = 2;
            c->error_message = e.what();
        }
        catch (exception& e) {
            c->error_type    = 3;
            c->error_message = e.what();
        }

        longjmp(c->setjmp_buffer, 1);
    }

    /**
     * From LIBJPEG/example.c:
     *
     * Terminate destination --- called by jpeg_finish_compress after all data
     * has been written.  Usually needs to flush buffer.
     *
     * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding application
     * must deal with any cleanup that should happen even for error exit.
     */
    static void term_callback(j_compress_ptr cinfo) throw()
    {
        SaveContext* c = static_cast<SaveContext*>(cinfo->dest);

        try {
            c->out.write(reinterpret_cast<const char*>(c->buffer.get()),
                         buffer_size - c->free_in_buffer);
            return;
        }
        catch (InterruptException& e) {
            c->error_type = 1;
        }
        catch (WriteException& e) {
            c->error_type    = 2;
            c->error_message = e.what();
        }
        catch (exception& e) {
            c->error_type    = 3;
            c->error_message = e.what();
        }

        longjmp(c->setjmp_buffer, 1);
    }
};


/**
 * The jpeg_decompress_struct constructor \c jpeg_create_decompress is not
 * called here because it may fail and be aborted via a long jump. Consequently
 * the call to jpeg_create_decompress has to occur after setting up the jump
 * buffer. On the other hand, no stack object with a destructor may be defined
 * in a scope where a long jump might pass through, so we cannot delay the
 * construction of this object to the longjump catching (try) scope.
 *
 * It is crucial that the \c need_destruction flag is raised immediately before
 * \c jpeg_create_decompress is called in the longjump catching scope.
 */
struct LoadWrapper {
    LoadWrapper():
        need_destruction(false)
    {
    }
    ~LoadWrapper()
    {
        cinfo.err = 0;
        cinfo.src = 0;
        if (need_destruction)
            jpeg_destroy_decompress(&cinfo);
    }
    bool need_destruction;
    jpeg_decompress_struct cinfo;
};


/**
 * See LoadWrapper. Same cautions apply.
 */
struct SaveWrapper {
    SaveWrapper():
        need_destruction(false)
    {
    }
    ~SaveWrapper()
    {
        cinfo.err = 0;
        cinfo.dest = 0;
        if (need_destruction)
            jpeg_destroy_compress(&cinfo);
    }
    bool need_destruction;
    jpeg_compress_struct cinfo;
};






/**
 * An adaptor that allows the Archon image library to use the JFIF file format
 * by using <tt><b>libjpeg</b></tt>.
 *
 * \sa  http://www.jpeg.org (ISO JPEG standards committee)
 * \sa  http://www.ijg.org (Independent JPEG Group)
 *
 * Rewritten and tested against <tt><b>libjpeg</b></tt> version 6b.
 *
 * \todo How do we handle progressive display during loading? The issue is that
 * of preventing concurrent reading and writing of the pixel data in a single
 * image. It is trivial for applications that does not need to load several
 * images concurrently, in this case we just read pixel data during the call to
 * to the ProgressTracker. The reading operation could eg. be a texture creation
 * function with the pixel buffer as input.
 *
 * \todo Loading multiple images concurrently with progressive display needs
 * more care. With the right thread safe API we might still be able to succeed
 * with only the ProgressTracker callback function. In most cases though, we
 * will have to bother about concurrency and re-entrance. There are at least two
 * different approaches one should considder. Either you would have each of the
 * progreass tracker callbacks compete for a critical region in which the image
 * data can be transfered safely to the destination buffer. This could be a call
 * to an OpenGL texture creation function, and in this case the GLXContext would
 * be the entity to protect.
 *
 * \todo A different approach to displaying multiple concurrently loading images
 * in a progressive manner is to have an extra thread that on regular intervals
 * consults each image and transfers its pixel data onto the display device. For
 * this to work, we need a way to synchronize access to the pixel data, since
 * the loader thread will need to access the pixel data too. An idea would be to
 * add a way for the image loading process to query the ProgressTracker object
 * for a mutex. The ProgressTracker object may report that there is no mutex, in
 * which case the loading process should not bother with synchronization. On the
 * other hand, if the ProgressTracker object does return a mutex reference, the
 * loading process shall acquire a lock on that mutex before accessing the image
 * data.
 *
 * \todo This appraoch will fit well into the common paradigme of having a frame
 * based rending thread with exclusive access to the rendering target
 * (GLXContext).
 *
 * \todo Considder adding the progressive flag to the Image API. This goes for
 * both loading and saving. In saving we have: If possible change the
 * compression strategy/file-layout to improve the perceived progressiveness
 * during the loading process potentally at the cost of system resources. For
 * loading we have: If possible use extra resources to improve the perceived
 * progressiveness of the loading process.
 *
 * Notes on using setjump/longjump in C++:
 *
 * Great care must be taken when using setjump/longjump in C++ context. The main
 * problem is that stack objects will not have their destructors called during a
 * long jump stack unwinding process.
 *
 * In this case we set up the long jump handler in a C++ function and then call
 * a number of C (not C++) functions, and it is only these C functions that can
 * potentially issue a long jump. This fact makes things a bit easier to
 * control.
 *
 * We should be safe as long as we stick to the following rules:
 *
 * <ol>
 *
 * <li> Keep the setjump/longjump construction on the following canonical form:
 *
 * <pre>
 *
 *   if(!setjmp(jmpbuf)) { // try
 *       // code that may potentially issue a long jump (throw)
 *   }
 *   else { // catch
 *       // code that handles the long jump (exception)
 *   }
 *
 * </pre>
 *
 * <li> Keep the try-scope completely free of declarations of variables whos
 * type have either an explicit or an implied destructor. This also applies to
 * any sub-scope through which a long jump may pass. A sub-scope through which a
 * long jump cannot pass, may safly define stack objects with destructors.
 *
 * <li> For each C function call that may lead to a long jump, make sure that
 * the surrounding expression does not involve creation of any temporary objects
 * of a type that has either an explicit or an implied destructor.
 *
 * <li> If any of the C function calls that may lead to a long jump calls a C++
 * function (eg. as a call back function) then make sure that the called C++
 * function does not issue a long jump or lead to one being issued in a place
 * where stack variables with destructors are "live".
 *
 * <li> Never long jump out of a C++ try block or catch block.
 *
 * </ul>
 */
class FormatJpeg: public FileFormat {
public:
    string get_name() const override
    {
        return "jpeg";
    }

    bool check_signature(InputStream &in) const override
    {
        LoadContext c(in, 0, 0); // No logging since we are only probing.
        LoadWrapper w;
        // Catch errors occuring in any of the following JPEG-functions:
        if (!setjmp(c.setjmp_buffer)) { // try
            w.cinfo.err = &c; // Downcast to jpeg_err_mgr
            jpeg_create_decompress(&w.cinfo);
            w.cinfo.src = &c; // Downcast to jpeg_source_mgr
            return jpeg_read_header(&w.cinfo, 1) == JPEG_HEADER_OK;
        }
        else { // catch
            return false;
        }
    }

    bool check_suffix(string s) const override
    {
        return s == "jpg" || s == "jpeg";
    }

    BufferedImage::Ref load(InputStream& in, Logger* logger,
                            ProgressTracker* tracker) const override
    {
        // Notes about Libjpeg decompression:
        //
        // Libjpeg always returns data in direct color mode (as opposed to color
        // mapped mode) by default. It also by default uses the best (Floyd
        // Steinberg) dithering method to produce the quantized color output
        // (color quantization is always a necessity here). Prescaling is
        // possible and may produce higher quality output than first
        // decompressing then rescaling, but since it only supports a small set
        // of fixed ratios, it's not really usefull in this context.
        //
        // Scanlines are returned from top to bottom, and each scanline holds
        // pixels from left to right side of the image. Each pixel consists of
        // 'output_components' color components, and each component is of type
        // JSAMPLE and uses the BITS_IN_JSAMPLE least significant bits.

        // If possible use extra resources to improve the perceived
        // progressiveness of the loading process.
        bool progressive = false; // Not yet propperly implemented

        LoadContext c(in, tracker, logger);

        LoadWrapper w;
        w.cinfo.err = &c; // Downcast to jpeg_err_mgr

        string comment;
        Array<JSAMPROW> row_pointers;

        ColorSpace::ConstRef color_space;
        IntegerBufferFormat::ChannelLayout channels;
        IntegerBufferFormat::ConstRef buf_fmt;
        BufferedImage::Ref image;

        // Handle termination and catch errors occuring in any of the following
        // JPEG-functions.
        if (!setjmp(c.setjmp_buffer)) { // try
            // CAUTION: No stack variables involving destructors may be defined
            // in this scope, or any sub-scope where a longjump might pass
            // through.

            // Must come immediately before jpeg_create_decompress
            w.need_destruction = true;
            jpeg_create_decompress(&w.cinfo);
            w.cinfo.src = &c; // Downcast to jpeg_source_mgr
            if (tracker)
                w.cinfo.progress = &c; // Downcast to jpeg_progress_mgr

            // Load comments from stream
            jpeg_save_markers(&w.cinfo, JPEG_COM, 0xFFFF);

            if (jpeg_read_header(&w.cinfo, 1) != JPEG_HEADER_OK)
                throw InvalidFormatException("Not a JPEG header");

            // We don't want Libjpeg to do color space transformations except
            // for the case of YCCK which we need converted to CMYK.
            J_COLOR_SPACE color_type = w.cinfo.jpeg_color_space;
            w.cinfo.out_color_space = color_type == JCS_YCCK ? JCS_CMYK : color_type;

            // Turn off progressive mode for files/streams with single-scan
            // images.
            progressive = progressive && jpeg_has_multiple_scans(&w.cinfo);

            if (progressive) {
                // select buffered-image mode
                w.cinfo.buffered_image = 1;

                // Run the initial passes with single-pass fixed color
                // quantization
                w.cinfo.two_pass_quantize = 0;
                w.cinfo.colormap          = 0;

                // Prepare for shifting to two-pass optimum color quantization.
                w.cinfo.enable_2pass_quant = 1;
            }

            jpeg_start_decompress(&w.cinfo);


            // Map the Libjpeg color space to a corresponding Archon color
            // space.
            switch (color_type) {
                case JCS_UNKNOWN:
                    color_space = ColorSpace::new_custom(w.cinfo.output_components);
                    break;
                case JCS_GRAYSCALE:
                    color_space = ColorSpace::get_Lum();
                    break;
                case JCS_RGB:
                    color_space = ColorSpace::get_RGB();
                    break;
                case JCS_YCbCr:
                    color_space = ColorSpace::get_YCbCr();
                    break;
                case JCS_CMYK:
                    color_space = ColorSpace::get_CMYK();
                    break;
                default:
                    throw runtime_error("Unexpected color space for JPEG data "
                                        "("+Text::print(static_cast<int>(color_type))+")");
            }

            // We now create the buffer format where channels are always evenly
            // spaced in terms of number of buffer pixels.
            int bits_per_byte = numeric_limits<unsigned char>::digits;
            int channel_pitch = sizeof(JSAMPLE)*bits_per_byte;
            WordType const word_type = get_word_type_by_bit_width(channel_pitch);
            int num_channels = w.cinfo.output_components;
            channels.bits_per_pixel = num_channels * channel_pitch;
            for (int i = 0; i < num_channels; ++i)
                channels.add(IntegerBufferFormat::Channel(i*channel_pitch, BITS_IN_JSAMPLE));
            // Libjpeg stores components in the low order bits of a JSAMPLE.
            bool most_sig_bit_first = false;
            // Since Libjpeg always uses an integer number of JSAMPLEs per
            // pixel, strips are necessarily word aligned, so the following
            // setting actually has no effect (is arbitarry).
            bool word_align_strip = true;

            buf_fmt = IntegerBufferFormat::get_format(word_type, channels,
                                                      most_sig_bit_first, word_align_strip);

            int height = w.cinfo.output_height;
            int width  = w.cinfo.output_width;
            int bytes_per_strip = buf_fmt->get_bytes_per_strip(width);

//cerr << "---- LOAD JPEG: "<<width<<"x"<<height<<" "<<buf_fmt->print(color_space, false)<< endl;

            // Construct an image with an uninitialized pixel buffer
            image = BufferedImage::new_image(width, height, color_space, false, buf_fmt);
            if (tracker) {
                image->clear();
                tracker->defined(image);
            }

            // Make row array
            JSAMPROW pixel_buffer = reinterpret_cast<JSAMPROW>(image->get_buffer_ptr());
            row_pointers.reset(height);
            for (int i = 0; i < height; ++i)
                row_pointers[i] = pixel_buffer + (height-i-1)*bytes_per_strip;

            if (progressive) {
                for (;;) {
                    // Absorb any waiting input by calling jpeg_consume_input().
                    //
                    // It seems to me there are two important aspectes of
                    // this. First of all it can be used to advance the input
                    // consumption to the point where a blocking read would
                    // block. This is important in progressive visualization
                    // mode, since it may allow us to skip display iterations if
                    // the data arrives fast enough. The challenge here is that
                    // it requires support for a non-blocking read on the
                    // stream.
                    //
                    // The obvious procedure would then be to start by switching
                    // the stream to non-blocking mode, then calling
                    // jpeg_consume_input() repeatedly until it returns either
                    // JPEG_SUSPENDED (stream reader would block) or
                    // JPEG_REACHED_EOI.
                    //
                    // At this point we must....
                    switch (jpeg_consume_input(&w.cinfo)) {
                        case JPEG_REACHED_SOS:
                            cerr << "JPEG_REACHED_SOS\n";
                        case JPEG_REACHED_EOI:
                            cerr << "JPEG_REACHED_EOI\n";
                        case JPEG_ROW_COMPLETED:
                            cerr << "JPEG_ROW_COMPLETED\n";
                        case JPEG_SCAN_COMPLETED:
                            cerr << "JPEG_SCAN_COMPLETED\n";
                        case JPEG_SUSPENDED:
                            cerr << "JPEG_SUSPENDED\n";
                    }

                    bool final_pass = jpeg_input_complete(&w.cinfo);

                    // Run final pass with two-pass optimum color quantization.
                    if (final_pass) {
                        // FIXME: Probably invalid for non-RGB color spaces.
                        w.cinfo.two_pass_quantize = 1;
                        w.cinfo.colormap          = 0;
                    }

                    jpeg_start_output(&w.cinfo, w.cinfo.input_scan_number);
                    while (w.cinfo.output_scanline < static_cast<JDIMENSION>(height))
                        jpeg_read_scanlines(&w.cinfo,
                                            row_pointers.get()+w.cinfo.output_scanline,
                                            height-w.cinfo.output_scanline);
                    jpeg_finish_output(&w.cinfo);

                    if (final_pass)
                        break;
                }
            }
            else {
                while (w.cinfo.output_scanline < static_cast<JDIMENSION>(height))
                    jpeg_read_scanlines(&w.cinfo,
                                        row_pointers.get()+w.cinfo.output_scanline,
                                        height-w.cinfo.output_scanline);
            }

            jpeg_saved_marker_ptr text = w.cinfo.marker_list;
            while (text) {
                string c =
                    Text::line_trim_ascii(string(reinterpret_cast<char*>(text->data),
                                                 text->data_length));
                if (!c.empty()) {
                    if (!comment.empty())
                        comment += "\n\n";
                    comment += c;
                }
                text = text->next;
            }

            // According to the JFIF standard, comments can only 7-bit ASCII. We
            // wranscode to 'clamp' potentail invalid 8-bit data.
            if (!comment.empty())
                comment = transcode(comment, transcode_US_ASCII, transcode_UTF_8);

//        cerr << "Comment: '"<<comment<<"'" << endl;

            jpeg_finish_decompress(&w.cinfo);
        }
        else { // catch
            switch (c.error_type) {
                case 1:
                    throw InterruptException();
                case 2:
                    throw ReadException(c.error_message);
                case 3:
                    throw runtime_error(c.error_message);
            }
            throw InvalidFormatException(c.error_message);
        }

        return image;
    }


    void save(Image::ConstRefArg image, OutputStream& out,
              Logger* logger, ProgressTracker* tracker) const override
    {
        // The idea is to first construct a pixel format that is supported by
        // Libpng, and at the same time, as closely as possible, matches the
        // pixel format of the incoming image, preferrably without loosing
        // precision. Then, if the constructed format turns out to be memory
        // compatible with the one used by the incoming image, we can use its
        // buffer directly, otherwise we need to copy the input image into a new
        // buffered image of the desired format.

        // Choose a suitable color space
        ColorSpace::ConstRef orig_color_space = image->get_color_space();
        ColorSpace::ConstRef color_space = orig_color_space;
        J_COLOR_SPACE color_type;
        switch (orig_color_space->get_type()) {
            case ColorSpace::type_Lum:
                color_type = JCS_GRAYSCALE;
                break;
            case ColorSpace::type_RGB:
                color_type = JCS_RGB;
                break;
            case ColorSpace::type_YCbCr:
                color_type = JCS_YCbCr;
                break;
            case ColorSpace::type_CMYK:
                color_type = JCS_CMYK;
                break;
            default:
                if (1 < orig_color_space->get_num_primaries()) {
                    color_space = ColorSpace::get_RGB();
                    color_type = JCS_RGB;
                }
                else {
                    color_space = ColorSpace::get_Lum();
                    color_type = JCS_GRAYSCALE;
                }
        }

        // Libjpeg support only one channel width
        int channel_width = BITS_IN_JSAMPLE;
        int bits_per_byte = numeric_limits<unsigned char>::digits;
        int channel_pitch = sizeof (JSAMPLE) * bits_per_byte;
        WordType word_type = get_word_type_by_bit_width(channel_pitch);
        int num_channels = color_space->get_num_primaries();
        IntegerBufferFormat::ChannelLayout channels;
        channels.bits_per_pixel = num_channels * channel_pitch;
        for (int i = 0; i < num_channels; ++i)
            channels.add(IntegerBufferFormat::Channel(i*channel_pitch, channel_width));
        // Libjpeg stores components in the low order bits of a JSAMPLE.
        bool  most_sig_bit_first = false;
        // Since Libjpeg always uses an integer number of JSAMPLEs per pixel,
        // strips are necessarily word aligned, so the following setting
        // actually has no effect (is arbitarry).
        bool word_align_strip = true;

        IntegerBufferFormat::ConstRef buf_fmt =
            IntegerBufferFormat::get_format(word_type, channels, most_sig_bit_first, word_align_strip);

        int width  = image->get_width();
        int height = image->get_height();
        int bytes_per_strip = buf_fmt->get_bytes_per_strip(width);

        BufferedImage::ConstRef buf_img;
        if (const BufferedImage* i = dynamic_cast<const BufferedImage*>(image.get())) {
            if (orig_color_space == color_space && i->has_equiv_buffer_format(buf_fmt))
                buf_img.reset(i);
        }
        if (!buf_img) {
            BufferedImage::Ref i =
                BufferedImage::new_image(width, height, color_space, false, buf_fmt);
            i->put_image(image, 0, 0, false); // Copy / transcode, no blending
            buf_img = i;
        }

//cerr << "---- SAVE JPEG: "<<width<<"x"<<height<<" "<<buf_fmt->print(color_space, false)<<" "<<
//  (buf_img == image ? "Can use incoming buffer" : "Can not use incoming buffer") << endl;

        // Make row array
        JSAMPROW pix_buf = reinterpret_cast<JSAMPROW>(const_cast<void*>(buf_img->get_buffer_ptr()));
        Array<JSAMPROW> row_pointers(height);
        for (int i = 0; i < height; ++i)
            row_pointers[i] = pix_buf + (height-i-1)*bytes_per_strip;

        string comment = "Created by the Archon image library";
        // Transcode comment to enforce 7-bit ASCII, which is all that the JFIF
        // file format allows
        comment = transcode(comment, transcode_UTF_8, transcode_US_ASCII);

        SaveContext c(out, tracker, logger);
        SaveWrapper w;
        w.cinfo.err = &c; // Downcast to jpeg_err_mgr

        save(c, w, width, height, num_channels, color_type, row_pointers.get(), comment,
             tracker); // Throws
    }

private:
    void save(SaveContext& c, SaveWrapper& w, int width, int height, int num_channels,
              J_COLOR_SPACE color_type, JSAMPROW* row_pointers, const std::string& comment,
              bool has_tracker) const
    {
        // Handle termination and catch errors occuring in any of the following
        // JPEG-functions.
        if (!setjmp(c.setjmp_buffer)) { // try
            // CAUTION: No stack variables involving destructors may be defined
            // in this scope, or any sub-scope where a longjump might pass
            // through.

            // Must come immediately before jpeg_create_compress
            w.need_destruction = true;
            jpeg_create_compress(&w.cinfo);
            w.cinfo.dest = &c; // Downcast to jpeg_destination_mgr
            if (has_tracker)
                w.cinfo.progress = &c; // Downcast to jpeg_progress_mgr

            // Set header info
            w.cinfo.image_width  = width;
            w.cinfo.image_height = height;
            w.cinfo.input_components = num_channels;
            w.cinfo.in_color_space = color_type;
            jpeg_set_defaults(&w.cinfo);

            jpeg_start_compress(&w.cinfo, true);

            // Save comment
            jpeg_write_marker(&w.cinfo, JPEG_COM,
                              reinterpret_cast<const JOCTET*>(comment.data()),
                              comment.size());

            // This loop is expected to have one iteration only, since we
            // are not using a 'suspending data destination manager'.
            while (w.cinfo.next_scanline < static_cast<JDIMENSION>(height)) {
                jpeg_write_scanlines(&w.cinfo,
                                     row_pointers+w.cinfo.next_scanline,
                                     height-w.cinfo.next_scanline);
            }

            jpeg_finish_compress(&w.cinfo);
        }
        else { // catch
            switch (c.error_type) {
                case 1:
                    throw InterruptException();
                case 2:
                    throw WriteException(c.error_message);
                case 3:
                    throw runtime_error(c.error_message);
            }
            throw InvalidFormatException(c.error_message);
        }
    }
};

} // unnamed namespace


namespace archon {
namespace Imaging {

FileFormat::ConstRef get_default_jpeg_file_format()
{
    static FileFormat::ConstRef f(new FormatJpeg());
    return f;
}

} // namespace Imaging
} // namespace archon
