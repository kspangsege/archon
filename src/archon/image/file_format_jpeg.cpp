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


#include <cstddef>
#include <csetjmp>
#include <type_traits>
#include <exception>
#include <iterator>
#include <utility>
#include <algorithm>
#include <memory>
#include <string_view>
#include <system_error>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/misc_error.hpp>
#include <archon/core/source.hpp>
#include <archon/core/sink.hpp>
#include <archon/log.hpp>
#include <archon/image/impl/config.h>
#include <archon/image/image.hpp>
#include <archon/image/writable_image.hpp>
#include <archon/image/error.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_jpeg.hpp>

#if ARCHON_IMAGE_HAVE_JPEG
#  include <jpeglib.h>
#  include <jerror.h>
#  if JPEG_LIB_VERSION < 62 // Need version 6b or newer
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


constexpr std::string_view g_file_format_ident = "jpeg"sv;

constexpr std::string_view g_file_format_descr = "JPEG (Joint Photographic Experts Group)"sv;

constexpr std::string_view g_mime_types[] = {
    "image/jpeg"sv,
};

constexpr std::string_view g_filename_extensions[] = {
    ".jpg"sv,
    ".jpeg"sv,
};



#if ARCHON_IMAGE_HAVE_JPEG


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


class Context {
public:
    Context(log::Logger& logger);

protected:
    jpeg_error_mgr m_error_mgr = {};
    int m_num_warnings = 0;
    bool m_have_libjpeg_error = false;
    bool m_have_ec = false;
    std::exception_ptr m_exception;
    std::error_code m_ec;

    log::Logger& m_logger;
    log::PrefixLogger m_libjpeg_logger;

    std::jmp_buf m_jmp_buf = {};

    void libjpeg_log(j_common_ptr info, log::LogLevel level);

private:
    static void error_callback(j_common_ptr info) noexcept;
    static void message_callback(j_common_ptr info, int msg_level) noexcept;

    static auto get_context(j_common_ptr info) noexcept -> Context&;

    void message(j_common_ptr info, int msg_level);
};


Context::Context(log::Logger& logger)
    : m_logger(logger)
    , m_libjpeg_logger(m_logger, "libjpeg: ") // Throws
{
    jpeg_std_error(&m_error_mgr);
    m_error_mgr.error_exit   = &error_callback;
    m_error_mgr.emit_message = &message_callback;

    // progress_monitor = progress_callback;                 
}


void Context::libjpeg_log(j_common_ptr info, log::LogLevel level)
{
    if (ARCHON_LIKELY(!m_logger.will_log(level)))
        return;
    char buffer[JMSG_LENGTH_MAX];
    (*m_error_mgr.format_message)(info, buffer);
    m_libjpeg_logger.log(level, "%s", std::string_view(buffer)); // Throws
}


void Context::error_callback(j_common_ptr info) noexcept
{
    Context& ctx = get_context(info);
    ctx.m_have_libjpeg_error = true;
    std::longjmp(ctx.m_jmp_buf, 1);
}


void Context::message_callback(j_common_ptr info, int msg_level) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() may be called or through with stack unwinding may pass (see notes on
    // long jump safety above).

    Context& ctx = get_context(info);
    try {
        ctx.message(info, msg_level); // throws
        return;
    }
    catch (...) {
        ctx.m_exception = std::current_exception();
    }

    std::longjmp(ctx.m_jmp_buf, 1);
}


inline auto Context::get_context(j_common_ptr info) noexcept -> Context&
{
    // Note: To make this safe, the pointer stored in `client_data` must be of type
    // `Context*` rather than `LoadContext*` or `SaveContext*`. See LoadContext and
    // SaveContext constructors.
    return *static_cast<Context*>(info->client_data);
}


void Context::message(j_common_ptr info, int msg_level)
{
    log::LogLevel log_level = log::LogLevel::warn;
    if (ARCHON_LIKELY(msg_level >= 0)) {
        log_level = (msg_level == 0 ? log::LogLevel::detail : log::LogLevel::trace);
    }
    else {
        constexpr int max_warnings = 1; // Emulate behavior of jpeg_std_error()
        if (m_num_warnings >= max_warnings)
            return;
        m_num_warnings += 1;
    }
    libjpeg_log(info, log_level); // Throws
}



inline bool is_due_to_invalid_file_contents(int msg_code) noexcept
{
    // These are errors that are not caused by invalid file contents
    //
    // FIXME: Other libjpeg error codes may need to be listed here
    //
    int actual_errors[] = {
        JERR_OUT_OF_MEMORY,
    };
    int* begin = actual_errors;
    int* end = begin + std::size(actual_errors);
    int* i = std::find(begin, end, msg_code);
    bool found = (i != end);
    return !found;
}


// The size of the buffers used for input/output streaming
constexpr std::size_t g_buffer_size = 4096;


class LoadContext : public Context {
public:
    LoadContext(log::Logger&, core::Source&);
    ~LoadContext() noexcept;

    bool recognize(bool& recognized, std::error_code& ec);
    bool load(std::error_code& ec);

private:
    core::Source& m_source;
    std::unique_ptr<char[]> m_buffer;
    jpeg_source_mgr m_source_mgr = {};
    jpeg_decompress_struct m_info = {};

    static void noop_message_callback(j_common_ptr info, int msg_level) noexcept;

    static void init_callback(j_decompress_ptr info) noexcept;
    static auto read_callback(j_decompress_ptr info) noexcept -> boolean;
    static void skip_callback(j_decompress_ptr info, long num_bytes) noexcept;
    static void term_callback(j_decompress_ptr info) noexcept;

    static auto get_context(j_decompress_ptr info) noexcept -> LoadContext&;

    bool read(std::error_code& ec);
    bool skip(long num_bytes, std::error_code& ec);
};


LoadContext::LoadContext(log::Logger& logger, core::Source& source)
    : Context(logger) // Throws
    , m_source(source)
{
    m_buffer = std::make_unique<char[]>(g_buffer_size); // Throws

    m_source_mgr.init_source       = &init_callback;
    m_source_mgr.fill_input_buffer = &read_callback;
    m_source_mgr.skip_input_data   = &skip_callback;
    m_source_mgr.resync_to_restart = jpeg_resync_to_restart; // Default implementation
    m_source_mgr.term_source       = &term_callback;
    m_source_mgr.bytes_in_buffer   = 0;
    m_source_mgr.next_input_byte   = nullptr;

    // Must store point of type `Context*` (see Context::get_context())
    m_info.client_data = static_cast<Context*>(this);
}


LoadContext::~LoadContext() noexcept
{
    m_info.err = {};
    m_info.src = {};
    jpeg_destroy_decompress(&m_info);
}


bool LoadContext::recognize(bool& recognized, std::error_code& ec)
{
    // Long jump safety: No non-volatile automatic variables in the scope from which
    // setjmp() is called (see notes on long jump safety above).

    // Don't log non-error messages
    m_error_mgr.emit_message = &noop_message_callback;

    // Catch long jumps from one of the callback functions.
    if (setjmp(m_jmp_buf) == 0) {
        // Long jump safety: No automatic variables of nontrivial type in the "try"-scope,
        // or in any subscope that may directly or indirectly initiate a long jump (see
        // notes on long jump safety above).

        m_info.err = &m_error_mgr;
        jpeg_create_decompress(&m_info);

        m_info.src = &m_source_mgr;
        jpeg_read_header(&m_info, 1);
    }
    else {
        // Long jumps from the callback functions land here.

        if (ARCHON_LIKELY(m_have_libjpeg_error)) {
            if (ARCHON_LIKELY(is_due_to_invalid_file_contents(m_error_mgr.msg_code))) {
                recognized = false;
                return true; // Success
            }
            libjpeg_log(reinterpret_cast<j_common_ptr>(&m_info), log::LogLevel::error); // Throws
            ec = image::Error::loading_process_failed;
            return false; // Failure
        }

        if (ARCHON_LIKELY(m_have_ec)) {
            if (ARCHON_LIKELY(m_ec == core::MiscError::premature_end_of_input)) {
                recognized = false;
                return true; // Success
            }
            ec = m_ec;
            return false; // Failure
        }

        ARCHON_ASSERT(m_exception);
        std::rethrow_exception(std::move(m_exception)); // Throws
    }

    recognized = true;
    return true; // Success
}


bool LoadContext::load(std::error_code& ec)
{
    // Long jump safety: No non-volatile automatic variables in the scope from which
    // setjmp() is called (see notes on long jump safety above).

    // Catch long jumps from one of the callback functions.
    if (setjmp(m_jmp_buf) == 0) {
        // Long jump safety: No automatic variables of nontrivial type in the "try"-scope,
        // or in any subscope that may directly or indirectly initiate a long jump (see
        // notes on long jump safety above).

        m_info.err = &m_error_mgr;
        jpeg_create_decompress(&m_info);

        m_info.src = &m_source_mgr;
        jpeg_read_header(&m_info, 1);

        ARCHON_STEADY_ASSERT_UNREACHABLE();     
    }
    else {
        // Long jumps from the callback functions land here.

        if (ARCHON_LIKELY(m_have_libjpeg_error)) {
            libjpeg_log(reinterpret_cast<j_common_ptr>(&m_info), log::LogLevel::error); // Throws
            ec = (is_due_to_invalid_file_contents(m_error_mgr.msg_code) ? image::Error::bad_file :
                  image::Error::loading_process_failed);
            return false; // Failure
        }

        if (ARCHON_LIKELY(m_have_ec)) {
            ec = m_ec;
            return false; // Failure
        }

        ARCHON_ASSERT(m_exception);
        std::rethrow_exception(std::move(m_exception)); // Throws
    }

    return true; // Success
}


void LoadContext::noop_message_callback(j_common_ptr, int) noexcept
{
    // No-op
}


void LoadContext::init_callback(j_decompress_ptr) noexcept
{
    // No-op
}


auto LoadContext::read_callback(j_decompress_ptr info) noexcept -> boolean
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() may be called or through with stack unwinding may pass (see notes on
    // long jump safety above).

    LoadContext& ctx = get_context(info);
    try {
        std::error_code ec;
        if (ARCHON_LIKELY(ctx.read(ec))) // Throws
            return 1;
        ctx.m_ec = ec;
        ctx.m_have_ec = true;
    }
    catch (...) {
        ctx.m_exception = std::current_exception();
    }

    std::longjmp(ctx.m_jmp_buf, 1);
}


void LoadContext::skip_callback(j_decompress_ptr info, long num_bytes) noexcept
{
    // Long jump safety: No automatic variables of nontrivial type in a scope from which
    // std::longjmp() may be called or through with stack unwinding may pass (see notes on
    // long jump safety above).

    LoadContext& ctx = get_context(info);
    try {
        std::error_code ec;
        if (ARCHON_LIKELY(ctx.skip(num_bytes, ec))) // Throws
            return;
        ctx.m_ec = ec;
        ctx.m_have_ec = true;
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
    // Stored pointer is always of type `Context*` (see Context::get_context())
    Context& context = *static_cast<Context*>(info->client_data);
    return static_cast<LoadContext&>(context);
}


bool LoadContext::read(std::error_code& ec)
{
    core::Span buffer = { m_buffer.get(), g_buffer_size };
    std::size_t n = {};
    if (ARCHON_LIKELY(m_source.try_read_some(buffer, n, ec))) { // Throws
        if (ARCHON_LIKELY(n > 0)) {
            static_assert(std::is_same_v<JOCTET, char> || std::is_same_v<JOCTET, unsigned char>);
            m_source_mgr.bytes_in_buffer = n;
            m_source_mgr.next_input_byte = reinterpret_cast<JOCTET*>(buffer.data());
            return true; // Success
        }
        ec = core::MiscError::premature_end_of_input;
    }
    return false; // Failure
}


bool LoadContext::skip(long num_bytes, std::error_code& ec)
{
    static_cast<void>(num_bytes);    
    static_cast<void>(ec);    
    ARCHON_STEADY_ASSERT_UNREACHABLE();     
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

    bool try_recognize(core::Source& source, bool& recognized, const std::locale&, log::Logger& logger,
                       std::error_code& ec) const override
    {
        LoadContext context(logger, source);
        return context.recognize(recognized, ec);
    }

    bool do_try_load(core::Source& source, std::unique_ptr<image::WritableImage>& image, const std::locale& loc,
                     log::Logger& logger, const LoadConfig& config, std::error_code& ec) const override
    {
        static_cast<void>(image);    
        static_cast<void>(loc);    
        static_cast<void>(config);    
        LoadContext context(logger, source);
        return context.load(ec);
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
        ARCHON_STEADY_ASSERT_UNREACHABLE();     
    }
};


#else // !ARCHON_IMAGE_HAVE_JPEG


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


#endif // !ARCHON_IMAGE_HAVE_JPEG


} // unnamed namespace


auto image::get_file_format_jpeg() noexcept -> const image::FileFormat&
{
    static FileFormatImpl impl;
    return impl;
}
