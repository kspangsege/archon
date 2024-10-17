// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <csetjmp>
#include <memory>
#include <string_view>
#include <system_error>
#include <locale>

#include <archon/core/span.hpp>
#include <archon/core/source.hpp>
#include <archon/core/sink.hpp>
#include <archon/image/impl/config.h>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_jpeg.hpp>

#if ARCHON_IMAGE_HAVE_JPEG
#  include <jpeglib.h>
#  include <jerror.h>
#  if JPEG_LIB_VERSION < 60 // Need version 6 or newer
#    error "PNG library is too old"
#  endif
#endif


// For libjpeg API documentation, see libjpeg.txt and example.c from the libjpeg-turbo
// project:
//
//  * https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/main/doc/libjpeg.txt
//
//  * https://raw.githubusercontent.com/libjpeg-turbo/libjpeg-turbo/main/src/example.c
//
// See also https://libjpeg-turbo.org/Documentation/Documentation


using namespace archon;
using namespace std::literals;


namespace {


#if ARCHON_IMAGE_HAVE_JPEG


// Notes on using setjump/longjump in C++:
//
// Great care must be taken when using setjump/longjump in C++ context. The main problem is
// that stack objects will not have their destructors called during a long jump stack
// unwinding process.
//
// A secondary problem is that a local variable in the scope from which setjmp() is called
// has indeterminate state upon a long jump if it was modified after the invocation of
// setjmp().
//
// In order to be on the safe side, the following restrictions should be adhered to:
//
//  * Keep the setjump/longjump construction on the following canonical form:
//
//      if (setjmp(jmpbuf) == 0) { // try
//         // code that may potentially issue a long jump (throw)
//      }
//      else { // catch
//          // code that handles the long jump (exception)
//      }
//
//  * Keep the "try"-scope completely free of automatic variables of non-trivial type
//    (std::is_trivial). This also applies to any sub-scope from which a long jump may be
//    directly or indirectly initiated. A sub-scope from which a long jump can not be
//    initiated, may safly have automatic variable of non-trivial type.
//
//  * Keep statements, that directly or indirectly may initiate a long jump, completely free
//    of temporaries of non-trivial type.
//
//  * Keep the scope from which setjmp() is called completely free of automatic variables.
//
//  * Never long jump out of a C++ try block or catch block.
//


struct Context {
    jpeg_error_mgr jerr = {};

    std::jmp_buf jmp_buf;

    Context() noexcept;
};


Context::Context() noexcept
{
    jpeg_std_error(&jerr);
/*
    progress_monitor = progress_callback;    
    error_exit       = error_callback;    
    output_message   = warning_callback;    
*/
}


struct LoadContext : Context {
    jpeg_source_mgr jsrc = {};
    jpeg_decompress_struct cinfo = {};

    bool recognize();

    LoadContext() noexcept;
    ~LoadContext() noexcept;
};


bool LoadContext::recognize()
{
    // Long jump safety: No non-volatile local variables in the scope from which setjmp() is
    // called (see notes on long jump safety above).

    // Catch long jumps from one of the callback functions.
    if (setjmp(jmp_buf) == 0) {
        // Long jump safety: No local variables of nontrivial type in the "try"-scope, or in
        // any subscope that may directly or indirectly initiate a long jump (see notes on
        // long jump safety above).

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);

        cinfo.src = &jsrc;
        jpeg_read_header(&cinfo, 1);
    }
    else {
        // Long jumps from the callback functions land here.

                                    
        return false;
    }

    return true;
}


LoadContext::LoadContext() noexcept
{
/*    
    jsrc.init_source       = &init_callback;
    jsrc.fill_input_buffer = &read_callback;
    jsrc.skip_input_data   = &skip_callback;
    jsrc.resync_to_restart = jpeg_resync_to_restart; // use default method    
    jsrc.term_source       = &term_callback;
    jsrc.bytes_in_buffer   = 0; // forces fill_input_buffer on first read    
    jsrc.next_input_byte   = 0;    
*/
}


LoadContext::~LoadContext() noexcept
{
    cinfo.err = {};
    cinfo.src = {};
    jpeg_destroy_decompress(&cinfo);
}


constexpr std::string_view g_mime_types[] = {
    "image/jpeg"sv,
};

constexpr std::string_view g_filename_extensions[] = {
    ".jpg"sv,
    ".jpeg"sv,
};


class FileFormatImpl final
    : public image::FileFormat {
public:
    auto get_ident() const noexcept -> std::string_view override
    {
        return "jpeg"sv;
    }

    auto get_descr() const -> std::string_view override
    {
        return "JPEG (Joint Photographic Experts Group)"sv;
    }

    auto get_mime_types() const noexcept -> core::Span<const std::string_view> override
    {
        return g_mime_types;
    }

    auto get_filename_extensions() const noexcept -> core::Span<const std::string_view> override
    {
        return g_filename_extensions;
    }

    bool recognize(core::Source& source) const override
    {
        static_cast<void>(source);                 
        LoadContext ctx;
        return ctx.recognize();
/*    
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
*/
    }

    bool do_try_load(core::Source& source, std::unique_ptr<image::WritableImage>& image, const std::locale& loc,
                     log::Logger& logger, const LoadConfig& config, std::error_code& ec) const override
    {
        static_cast<void>(source);    
        static_cast<void>(image);    
        static_cast<void>(loc);    
        static_cast<void>(logger);    
        static_cast<void>(config);    
        static_cast<void>(ec);    
        return false;               
    }

    bool do_try_save(const image::Image& image, core::Sink& sink, const std::locale& loc, log::Logger& logger,
                     const SaveConfig& config, std::error_code& ec) const override
    {
        static_cast<void>(image);    
        static_cast<void>(sink);    
        static_cast<void>(loc);    
        static_cast<void>(logger);    
        static_cast<void>(config);    
        static_cast<void>(ec);    
        return false;               
    }
};


#endif // ARCHON_IMAGE_HAVE_JPEG


} // unnamed namespace


auto image::get_file_format_jpeg() noexcept -> const image::FileFormat*
{
#if ARCHON_IMAGE_HAVE_JPEG
    static FileFormatImpl impl;
    return &impl;
#else
    return nullptr;
#endif
}
