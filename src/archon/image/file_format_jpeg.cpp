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
#include <exception>
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
// A secondary problem is that an automatic variable in the scope from which setjmp() is
// called has indeterminate state upon a long jump if it was modified after the invocation
// of setjmp().
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
//  * Keep the "try"-scope completely free of automatic variables of nontrivial type
//    (std::is_trivial). This also applies to any sub-scope from which a long jump may be
//    directly or indirectly initiated. A sub-scope from which a long jump can not be
//    initiated, may safly have automatic variable of nontrivial type.
//
//  * Keep statements, that directly or indirectly may initiate a long jump, completely free
//    of temporaries of nontrivial type.
//
//  * Keep the scope from which setjmp() is called completely free of automatic variables.
//
//  * Never long jump out of a C++ try block or catch block.
//


class Context {
public:
    Context();

protected:
    // Must be first non-static data member of Context (see LoadContext::get_context() and
    // SaveContext::get_context())
    jpeg_error_mgr m_err = {};

    std::jmp_buf m_jmp_buf = {};

    // Error handling
    std::error_code m_ec;
    std::exception_ptr m_exception;

    static auto get_context(jpeg_error_mgr& err) noexcept -> Context&;
};


Context::Context()
{
    jpeg_std_error(&m_err);
/*
    progress_monitor = progress_callback;    
    error_exit       = error_callback;    
    output_message   = warning_callback;    
*/
}


inline auto Context::get_context(jpeg_error_mgr& err) noexcept -> Context&
{
    static_assert(offsetof(Context, m_err) == 0);
    return *reinterpret_cast<Context*>(&err);
}



// The size of the buffers used for input/output streaming
constexpr std::size_t g_buffer_size = 4096;


class LoadContext : public Context {
public:
    LoadContext(core::Source&);
    ~LoadContext() noexcept;

    bool recognize();

private:
    core::Source& m_source;
    std::unique_ptr<char[]> m_buffer;
    jpeg_source_mgr m_src = {};
    jpeg_decompress_struct m_info = {};

    static void init_callback(j_decompress_ptr info) noexcept;
    static int read_callback(j_decompress_ptr info) noexcept;
    static void skip_callback(j_decompress_ptr info, long num_bytes) noexcept;
    static void term_callback(j_decompress_ptr info) noexcept;

    static auto get_context(j_decompress_ptr info) noexcept -> LoadContext&;

    bool read(std::error_code& ec);
    bool skip(long num_bytes, std::error_code& ec);
};


LoadContext::LoadContext(core::Source& source)
    : m_source(source)
{
    m_buffer = std::make_unique<char[]>(g_buffer_size); // Throws

    m_src.init_source       = &init_callback;
    m_src.fill_input_buffer = &read_callback;
    m_src.skip_input_data   = &skip_callback;
    m_src.resync_to_restart = jpeg_resync_to_restart; // Default implementation
    m_src.term_source       = &term_callback;
    m_src.bytes_in_buffer   = 0;
    m_src.next_input_byte   = nullptr;
}


LoadContext::~LoadContext() noexcept
{
    m_info.err = {};
    m_info.src = {};
    jpeg_destroy_decompress(&m_info);
}


bool LoadContext::recognize()
{
    // Long jump safety: No non-volatile automatic variables in the scope from which
    // setjmp() is called (see notes on long jump safety above).

    // Catch long jumps from one of the callback functions.
    if (setjmp(m_jmp_buf) == 0) {
        // Long jump safety: No automatic variables of nontrivial type in the "try"-scope,
        // or in any subscope that may directly or indirectly initiate a long jump (see
        // notes on long jump safety above).

        m_info.err = jpeg_std_error(&m_err);
        jpeg_create_decompress(&m_info);

        m_info.src = &m_src;
        jpeg_read_header(&m_info, 1);
    }
    else {
        // Long jumps from the callback functions land here.

        if (ARCHON_LIKELY(!m_exception)) {
            // Certain error codes (m_ec) should be interpreted as "unrecognized" while other should lead to the throwing of a system error exception                                                               
            ARCHON_STEADY_ASSERT_UNREACHABLE();          
        }
        std::rethrow_exception(std::move(m_exception)); // Throws
    }

    return true; // Success
}


void LoadContext::init_callback(j_decompress_ptr) noexcept
{
    // No-op
}


int LoadContext::read_callback(j_decompress_ptr info) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() is called (see notes on long jump safety above).

    LoadContext& ctx = get_context(info);
    try {
        std::error_code ec;
        if (ARCHON_LIKELY(ctx.read(ec))) // Throws
            return 1;
        ctx.m_ec = ec;
    }
    catch (...) {
        ctx.m_exception = std::current_exception();
    }

    std::longjmp(ctx.m_jmp_buf, 1);
}


void LoadContext::skip_callback(j_decompress_ptr info, long num_bytes) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() is called (see notes on long jump safety above).

    LoadContext& ctx = get_context(info);
    try {
        std::error_code ec;
        if (ARCHON_LIKELY(ctx.skip(num_bytes, ec))) // Throws
            return;
        ctx.m_ec = ec;
    }
    catch (...) {
        ctx.m_exception = std::current_exception();
    }

    std::longjmp(ctx.m_jmp_buf, 1);
}


void LoadContext::term_callback(j_decompress_ptr) noexcept
{
    // No-op
}


inline auto LoadContext::get_context(j_decompress_ptr info) noexcept -> LoadContext&
{
    return static_cast<LoadContext&>(Context::get_context(*info->err));
}


bool LoadContext::read(std::error_code& ec)
{
    core::Span buffer = { m_buffer.get(), g_buffer_size };
    std::size_t n = {};
    if (ARCHON_LIKELY(m_source.try_read_some(buffer, n, ec))) { // Throws
        static_assert(std::is_same_v<JOCTET, char> || std::is_same_v<JOCTET, unsigned char>);
        m_src.bytes_in_buffer = n;
        m_src.next_input_byte = reinterpret_cast<JOCTET*>(buffer.data());
        return true;
    }
    return false;
}


bool LoadContext::skip(long num_bytes, std::error_code& ec)
{
    static_cast<void>(num_bytes);    
    static_cast<void>(ec);    
    ARCHON_STEADY_ASSERT_UNREACHABLE();     
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
        LoadContext context(source);
        return context.recognize();
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
