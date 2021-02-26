#include <streambuf>
#include <iostream>

#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/format_enc.hpp>
#include <archon/base/file.hpp>
#include <archon/unit_test.hpp>



// Consideer: make its such that codec decode only returns true if eof arg is true and conversion is complete (in order to deal with quirks of std::codecvt in libstdc++ and libc++)              


// Consider: Put Impl in detail namespace and source files in impl (or detail) subdir


// Consider: BufferedFile, BasicBufferedPrimTextFile, BasicBufferedTextFile


// FIXME: Introduce new error codes for invalid multi-byte character, and for invalid wide character at level of TextFileImpl.                    






// Consider introducing BasicTextFile<C, T, F>, and then build BasicTextFileStreambuf<C, T, F> on top of it. Here F is the underlying file type, which is base::PrimTextFile by default. base::PrimTextFile is an alias for base::PrimPosixTextFile on POSIX, and an alias for base::PrimWindowsTextFile on Windows. base::PrimWindowsTextFile is buffered. base::PrimPosixTextFile is unbuffered.                                 




    // WHY IS IT, THAT I COULD NOT JUST CHANGE CODEC TO ABSORBE ANY PARTIAL CHARACTER?

    // PROBLEM: libstdc++ (GCC) reports decode completion (result is ok) even after consuming a partial character. Fortunately, std::mbsinit() returns false in this case (useful only under assumption that encoding is no stateful).

    // PROBLEM: libstdc++ (GCC) inconsistency: With input "\xC3x", GCC consumes nothing and reports error, but with intput "\xC3" followed by input "x", GCC consumes first byte and report same error. Fortunately, the two situations can be distinguished by whether the state is initial (std::mbsinit()).

    // PROBLEM: libc++ (Clang) reports decode completion (result is ok + in initial state according to std::mbsinit()) even after consuming a partial character. Fortunately, state is not equal to default initialized state in this case (useful only under assumption that encoding is no stateful).



    // NEW IDEA TO SOLVE OTHERWISE INTRACTABLE PROBLEM IN THIS FUNCTION                             
    //
    // Needs to be better explained        
    //
    // Primitive:
    //   advance()
    //     - move revert reference to just after the last byte returned by try_read()
    //   advance(n)
    //     - move revert reference to just after the first n bytes returned by try_read() since since advance() was last called, or since read mode was engaged if advance() has not called since read mode was engaged
    //       PrimWinImpl will implement this by compareing n to known last position of newline conversion. If n is higher, no repeated decoding (simulation) is necessary.
    //     - n is relative to current revert reference
    //   try_revert(n)
    //     - n is relative to current revert reference
    //
    // Non-primitive:
    //   advance()
    //     - move revert reference to just after the last character returned by try_read()
    //   try_revert(n)
    //     - n is relative to current revert reference

    
    




// ============================================================================================ newline_crlf.hpp ============================================================================================


namespace archon::base::newline_crlf {


// FIXME: Add non-incremental convenience functions and use them in unit test root file / STDERR logger     


/// \brief Advance incremental newline decoding process.
///
/// \param clear_offset, clear If any newline decoding takes place, \p clear is
/// set to the sum of \p clear_offset and the position in the buffer following
/// the last decoded newline character. Otherwise, \p clear is left untouched.
///
void inc_decode(base::Span<const char> data, std::size_t& data_offset, base::Span<char> buffer,
                std::size_t& buffer_offset, std::size_t clear_offset, std::size_t& clear,
                bool end_of_data) noexcept;


/// \brief Advance incremental newline encoding process.
///
void inc_encode(base::Span<const char> data, std::size_t& data_offset,
                base::Span<char> buffer, std::size_t& buffer_offset) noexcept;


bool simulate_inc_decode(base::Span<const char> data, std::size_t& data_offset,
                         std::size_t buffer_size) noexcept;


} // namespace archon::base::newline_crlf





// ============================================================================================ newline_crlf.cpp ============================================================================================


namespace archon::base::newline_crlf {


void inc_decode(base::Span<const char> data, std::size_t& data_offset, base::Span<char> buffer,
                std::size_t& buffer_offset, std::size_t clear_offset, std::size_t& clear,
                bool end_of_data) noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(buffer_offset <= buffer.size());

    // CR+LF -> NL
    std::size_t i = data_offset, i_2;
    std::size_t j = buffer_offset;
    std::size_t k = clear;
    while (ARCHON_LIKELY(i < data.size() && j < buffer.size())) {
        char ch = data[i];
        if (ARCHON_LIKELY(ch != '\r')) {
          cr:
            buffer[j++] = ch;
            ++i;
            continue;
        }
        i_2 = i + 1;
        if (ARCHON_LIKELY(i_2 < data.size())) {
            char ch_2 = data[i_2];
            if (ARCHON_LIKELY(ch_2 == '\n')) {
                buffer[j++] = '\n';
                k = clear_offset + j;
                i = i_2 + 1;
                continue;
            }
            goto cr;
        }
        if (!end_of_data)
            break;
        goto cr;
    }
    data_offset   = i;
    buffer_offset = j;
    clear         = k;
}


void inc_encode(base::Span<const char> data, std::size_t& data_offset,
                base::Span<char> buffer, std::size_t& buffer_offset) noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(buffer_offset <= buffer.size());

    // NL -> CR+LF
    std::size_t i = data_offset;
    std::size_t j = buffer_offset;
    while (ARCHON_LIKELY(i < data.size())) {
        char ch = data[i];
        if (ARCHON_LIKELY(ch != '\n')) {
            if (ARCHON_UNLIKELY(j == buffer.size()))
                break;
            buffer[j++] = ch;
        }
        else {
            if (ARCHON_UNLIKELY(buffer.size() - j < 2))
                break;
            buffer[j++] = '\r';
            buffer[j++] = '\n';
        }
        ++i;
    }
    data_offset   = i;
    buffer_offset = j;
}


bool simulate_inc_decode(base::Span<const char> data, std::size_t& data_offset,
                         std::size_t buffer_size) noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());

    // CR+LF -> NL
    std::size_t i = data_offset, i_2;
    std::size_t j = 0;
    while (ARCHON_LIKELY(j < buffer_size)) {
        if (ARCHON_LIKELY(i < data.size())) {
            char ch = data[i];
            if (ARCHON_LIKELY(ch != '\r')) {
              regular:
                ++j;
                ++i;
                continue;
            }
            i_2 = i + 1;
            if (ARCHON_LIKELY(i_2 < data.size())) {
                char ch_2 = data[i_2];
                if (ARCHON_LIKELY(ch_2 == '\n')) {
                    ++j;
                    i = i_2 + 1;
                    continue;
                }
            }
            goto regular;
        }
        return false;
    }
    data_offset = i;
    return true;
}


} // namespace archon::base::newline_crlf





// ============================================================================================ char_codec.hpp ============================================================================================


namespace archon::base {


// FIXME: Add no-op specialization for char<->char case                                                       
template<class C, class T = std::char_traits<C>> class BasicCharCodec {
public:
    using char_type   = C;
    using traits_type = T;

    BasicCharCodec(const std::locale&);

    bool stateful() const noexcept;

    // If this function returns true, the decoding operation completed succesfully. If it returns false, it did not.
    //
    // When false is returned, \p error is set to true if the reason for the incompleteness is invalid input, and to false otherwise.
    //
    // When false is returned, and \p error is set to false, and buffer_offset is equal to the size of the specified buffer upon return, additional buffer space is required in order for decoding to proceed.
    //
    // When false is returned, and \p error is set to false, and buffer_offset is less than the size of the specified buffer upon return, additional input (\p data) is required in order for decoding to proceed.
    //
    // In any case, data_offset and buffer_offset are updated to reflect the how far decoding has progressed.
    //
    // If an exception is thrown, more data may have been written to the buffer than indicated by the final value of buffer_offset.
    //
    bool inc_decode(std::mbstate_t&, base::Span<const char> data, std::size_t& data_offset,
                    base::Span<C> buffer, std::size_t& buffer_offset, bool& error);

    auto inc_encode(std::mbstate_t&, base::Span<const C> data, std::size_t& data_offset,
                    base::Span<char> buffer, std::size_t& buffer_offset) ->
        std::codecvt_base::result;

    /// \brief   
    ///
    /// It is an error if the difference between `data.size()` and \p
    /// data_offset (prior to invocation) is more than \ref
    /// max_simulate_decode_size().
    ///
    /// It is an error if \p buffer_size is greater than the increase in
    /// `buffer_offset` that would be cuased by an invocation of \ref
    /// inc_decode() given the same state and span of data.
    ///
    void simulate_inc_decode(std::mbstate_t&, base::Span<const char> data,
                             std::size_t& data_offset, std::size_t buffer_size) noexcept;

    /// \brief   
    ///
    /// See \ref simulate_inc_decode().
    ///
    static constexpr std::size_t max_simulate_decode_size() noexcept;

private:
    using codecvt_type = std::codecvt<C, char, std::mbstate_t>;

    const std::locale m_locale;
    const codecvt_type& m_codecvt;
    const bool m_stateful = (m_codecvt.encoding() == -1);

    BasicCharCodec(const std::locale&, const codecvt_type&, bool lenient) noexcept;
};


using CharCodec     = BasicCharCodec<char>;
using WideCharCodec = BasicCharCodec<wchar_t>;








// Implementation


template<class C, class T> inline BasicCharCodec<C, T>::BasicCharCodec(const std::locale& locale) :
    m_locale(locale),
    m_codecvt(std::use_facet<codecvt_type>(m_locale)) // Throws
{
}


template<class C, class T> inline bool BasicCharCodec<C, T>::stateful() const noexcept
{
    return m_stateful;
}


template<class C, class T>
bool BasicCharCodec<C, T>::inc_decode(std::mbstate_t& state, base::Span<const char> data,
                                      std::size_t& data_offset, base::Span<C> buffer,
                                      std::size_t& buffer_offset, bool& error)
{
    // Deal with bug in GNU libstdc++ causing m_codecvt.in() to return "ok"
    // when the buffer size is zero. The bug is present in GCC 10.2.0. See also
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37475.
#if ARCHON_GNU_LIBCXX
    bool has_buffer_space = (buffer_offset < buffer.size());
    if (ARCHON_UNLIKELY(!has_buffer_space))
        return std::codecvt_base::partial;
#endif

    // FIXME: IS THIS RIGHT (CHECK PLATFORMS)???                                                                                                                         
    const char* from      = data.data() + data_offset;
    const char* from_end  = data.data() + data.size();
    const char* from_next = nullptr;
    C* to      = buffer.data() + buffer_offset;
    C* to_end  = buffer.data() + buffer.size();
    C* to_next = nullptr;
    std::codecvt_base::result result =
        m_codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
    data_offset   = std::size_t(from_next - data.data());
    buffer_offset = std::size_t(to_next - buffer.data());
    // Cases:
    //    Everything is consumed, and all output is produced
    //       result:   ok
    //       reaction: success if buffer_offset > 0 || buffer.size() == 0, else try to read more
    //       platform: ?  
    //
    //    Everything is consumed, but more input is needed to complete the partially consumed byte sequence
    //       result:   partial ---------------> NO, not with libstdc++ and libc++                                                              
    //       reaction: success if buffer_offset > 0 || buffer.size() == 0, else try to read more
    //       platform: ?  
    //
    //    Everything is consumed, but there is not enough space in output buffer to complete conversion
    //       result:   partial
    //       reaction: success if buffer_offset > 0 || buffer.size() == 0, else try to read more
    //       platform: ?  
    //
    //    Not everything is consumed, because the next unconsumed byte is the first byte of an incomplete character
    //       result:   partial (, ok)  
    //
    //    Not everything is consumed, because there is not enough space in output buffer to convert the next unconsumed character
    //       result:   partial (, ok)
    //       reaction: 
    //
    //    Not everything is consumed, because there is not enough space in output buffer to convert the last consumed character
    //
    //    Not everything is consumed, because the next byte is the first byte of a byte sequence that does not correspond to a character (error)
    //
    //    Not everything is consumed, because the next byte is not a valid continuation of the parially consumed byte sequence (error)
    //
    switch (result) {
        case std::codecvt_base::ok:
            if (ARCHON_LIKELY(data_offset == data.size()))
                return true;
            [[fallthrough]];
        case std::codecvt_base::partial:
            error = false;
            return false;
        case std::codecvt_base::error:
            error = true;
            return false;
        case std::codecvt_base::noconv:
            break;
    }
    ARCHON_ASSERT_UNREACHABLE;
}


// FIXME: Update to match inc_decode() in style                                                                                         
template<class C, class T>
auto BasicCharCodec<C, T>::inc_encode(std::mbstate_t& state, base::Span<const C> data,
                                      std::size_t& data_offset, base::Span<char> buffer,
                                      std::size_t& buffer_offset) -> std::codecvt_base::result
{
    // Deal with bug in GNU libstdc++ causing m_codecvt.out() to return "ok"
    // when the buffer size is zero. The bug is present in GCC 10.2.0. See also
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37475.
#if ARCHON_GNU_LIBCXX
    bool has_buffer_space = (buffer_offset < buffer.size());
    if (ARCHON_UNLIKELY(!has_buffer_space))
        return std::codecvt_base::partial;
#endif

    // FIXME: What about unshifting on end of input?          

    // FIXME: Tend to details here            
    const C* from      = data.data() + data_offset;
    const C* from_end  = data.data() + data.size();
    const C* from_next = nullptr;
    char* to      = buffer.data() + buffer_offset;
    char* to_end  = buffer.data() + buffer.size();
    char* to_next = nullptr;
    std::codecvt_base::result result =
        m_codecvt.out(state, from, from_end, from_next, to, to_end, to_next);
    data_offset   = std::size_t(from_next - data.data());
    buffer_offset = std::size_t(to_next - buffer.data());
    return result;
}


template<class C, class T>
void  BasicCharCodec<C, T>::simulate_inc_decode(std::mbstate_t& state, base::Span<const char> data,
                                                std::size_t& data_offset,
                                                std::size_t buffer_size) noexcept
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(std::size_t(data.size() - data_offset) <= max_simulate_decode_size());
    const char* begin = data.data() + data_offset;
    const char* end   = data.data() + data.size();
    int n = m_codecvt.length(state, begin, end, buffer_size);
    ARCHON_ASSERT(n >= 0);
    data_offset += std::size_t(n);
}


template<class C, class T> constexpr std::size_t BasicCharCodec<C, T>::max_simulate_decode_size() noexcept
{
    std::size_t max_1 = std::numeric_limits<std::size_t>::max();
    int max_2 = std::numeric_limits<int>::max();
    using uint = unsigned int;
    if (max_1 >= uint(max_2))
        return max_1;
    return std::size_t(max_2);
}


} // namespace archon::base




// ============================================================================================ extended_char_codec.hpp ============================================================================================


/*
namespace archon::base {


// Adds support for leniency mode                     
template<class C, class T = std::char_traits<C>> class BasicExtendedCharCodec {
public:
};


using ExtendedCharCodec     = BasicExtendedCharCodec<char>;
using WideExtendedCharCodec = BasicExtendedCharCodec<wchar_t>;


} // namespace archon::base
*/




// ============================================================================================ prim_text_file_impl.hpp ============================================================================================


namespace archon::base {


/// \{
///
/// \brief Files with support for newline translation on select platforms.
///
/// Concept: PrimTextFileImpl
/// -------------------------
///
/// A file is in one of three modes, neutral, reading, or writing. Initially, it
/// is in neutral mode.
///
/// A precondition for calling read(), is that the file is not in writing
/// mode. After an invocation of read(), the file is in reading mode, even if
/// the operation fails.
///
/// A precondition for calling write(), is that the file is not in reading
/// mode. After an invocation of write(), the file is in writing mode, even if
/// the operation fails.
///
/// A precondition of calling advance() is that the file is not in writing
/// mode. After an invocation of advance(), the mode is unchanged.
///
/// A precondition of calling discard() is that the file is not in writing
/// mode. After a successful invocation of discard(), the file is in neutral
/// mode. After a failed invocation of discard(), the mode is unchanged.
///
/// A precondition of calling revert() is that the file is not in writing
/// mode. After a successful invocation of revert(), the file is in neutral
/// mode. After a failed invocation of revert(), the mode is unchanged.
///
/// A precondition of calling flush() is that the file is not in reading
/// mode. After a successful invocation of flush(), the file is in neutral
/// mode. After a failed invocation of flush(), the mode is unchanged.
///
/// A precondition for calling seek(), is that the file is not in writing
/// mode. After a successful invocation of seek(), the file is in neutral
/// mode. After a failed invocation of seek(), the mode is unchanged.
///
class PrimPosixTextFileImpl;
class PrimWindowsTextFileImpl;
/// \}


#if ARCHON_WINDOWS
using PrimTextFileImpl = PrimWindowsTextFileImpl;
#else
using PrimTextFileImpl = PrimPosixTextFileImpl;
#endif



struct PrimTextFileImplConfig {
    static constexpr std::size_t default_newline_codec_buffer_size = 1024;

    // GENERIC:
    //
    // Buffer will be automatically expanded if necessary.              
    //
    std::size_t newline_codec_buffer_size = default_newline_codec_buffer_size;
    base::Span<char> newline_codec_buffer_memory;
};



class PrimPosixTextFileImpl {
public:
    using Config = PrimTextFileImplConfig;
    using offset_type = base::File::offset_type;

    PrimPosixTextFileImpl(base::File&, Config) noexcept;

    [[nodiscard]] bool read(base::Span<char> buffer, bool dynamic_eof, std::size_t& n,
                            std::error_code&);

    [[nodiscard]] bool write(base::Span<const char> data, std::size_t& n,
                             std::error_code&) noexcept;

    void advance() noexcept;
    void advance(std::size_t n) noexcept;

    [[nodiscard]] bool discard(std::error_code&) noexcept;

    [[nodiscard]] bool revert(std::size_t offset, std::error_code&) noexcept;

    [[nodiscard]] bool flush(std::error_code&) noexcept;

    [[nodiscard]] bool seek(offset_type, std::error_code&) noexcept;

private:
    base::File& m_file;
    std::size_t m_retain_size = 0; // Zero in neutral and writing modes

#if ARCHON_DEBUG
    bool m_reading = false;
    bool m_writing = false;
#endif
};



class PrimWindowsTextFileImpl {
public:
    using Config = PrimTextFileImplConfig;
    using offset_type = base::File::offset_type;

    PrimWindowsTextFileImpl(base::File&, Config);

    // GENERIC:
    //
    // "Read some" with retaining semantics.
    //
    // On success, if \p n is zero and the size of the specified buffer is
    // greater than zero, it means that the end of input has been reached.
    //
    // On success, \p ec is left untouched. On failure \p n is left untouched,
    // and no characters will have been read from the stream.
    //
    [[nodiscard]] bool read(base::Span<char> buffer, bool dynamic_eof, std::size_t& n,
                            std::error_code&);

    // GENERIC:
    //
    // On success, \p n will always be equal to `data.size()`. On failure, \p n
    // will always be less than `data.size()` (provided that `data.size()` is
    // greater than zero), and it will indicate how many characters were written
    // before the failure occurred.
    //
    // On success, \p ec is left untouched.
    //
    [[nodiscard]] bool write(base::Span<const char> data, std::size_t& n, std::error_code&);

    // GENERIC:
    //
    // `noexcept` is important here   
    //
    void advance() noexcept;
    void advance(std::size_t n) noexcept;

    [[nodiscard]] bool discard(std::error_code&) noexcept;

    // GENERIC:
    //
    // BRIEF: Revert file offset and discard pre-loaded data.
    //
    // The explanation below is no longer correct               
    //
    // On success, the file offset is reverted to where it would have been if
    // the size of the caller's buffer during the last invocation of read() had
    // been `offset`, and the read operation had resulted in a situation where
    // no data was pre-loaded.
    //
    // When in reading mode, `offset` must be less than, or equal to the size of
    // the extracted chunk during the last invocation of read(), which is zero
    // if that read operation failed. When in neutral mode, `offset` must be
    // zero.
    //
    [[nodiscard]] bool revert(std::size_t offset, std::error_code&) noexcept;

    [[nodiscard]] bool flush(std::error_code&) noexcept;

    [[nodiscard]] bool seek(offset_type, std::error_code&) noexcept;

private:
    base::File& m_file;
    base::SeedMemoryBuffer<char> m_buffer;
    std::size_t m_retain_begin = 0; // Zero in neutral and writing modes
    std::size_t m_begin = 0; // Zero in neutral mode
    std::size_t m_end   = 0; // Zero in neutral mode
    std::size_t m_retain_size  = 0; // Zero in neutral and writing modes
    std::size_t m_retain_clear = 0; // Zero in neutral and writing modes

#if ARCHON_DEBUG
    bool m_reading = false;
    bool m_writing = false;
#endif

    bool do_discard(std::size_t n, std::error_code& ec) noexcept;
    void expand_buffer();
};








// Implementation


// ============================ PrimPosixTextFileImpl ============================


inline PrimPosixTextFileImpl::PrimPosixTextFileImpl(base::File& file, Config) noexcept :
    m_file(file)
{
}


inline bool PrimPosixTextFileImpl::read(base::Span<char> buffer, bool, std::size_t& n,
                                        std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    std::size_t buffer_size = buffer.size();
    std::size_t max = std::size_t(std::numeric_limits<std::size_t>::max() - m_retain_size);
    if (ARCHON_UNLIKELY(buffer_size > max)) {
        if (ARCHON_UNLIKELY(max == 0))
            throw std::length_error("Revert distance");
        buffer_size = max;
    }

    if (ARCHON_LIKELY(m_file.try_read_some({ buffer.data(), buffer_size }, n, ec))) {
        m_retain_size += n;
        return true;
    }
    return false;
}


inline bool PrimPosixTextFileImpl::write(base::Span<const char> data, std::size_t& n,
                                         std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    return m_file.try_write(data, n, ec);
}


inline void PrimPosixTextFileImpl::advance() noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    m_retain_size = 0;
}


inline void PrimPosixTextFileImpl::advance(std::size_t n) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(n <= m_retain_size);
    m_retain_size -= n;
}


inline bool PrimPosixTextFileImpl::discard(std::error_code&) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = false;
#endif

    m_retain_size = 0;
    return true;
}


inline bool PrimPosixTextFileImpl::revert(std::size_t offset, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(offset <= m_retain_size);
    std::size_t n = std::size_t(m_retain_size - offset);
    auto whence = base::File::Whence::cur;
    offset_type result; // Dummy
    // Avoid actual seeking when n is zero, such that writing can be enabled for
    // an underlying file that does not support seeking, e.g., STDOUT.
    if (ARCHON_LIKELY(n == 0 || m_file.try_seek(-offset_type(n), whence, result, ec))) {
        m_retain_size = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


inline bool PrimPosixTextFileImpl::flush(std::error_code&) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = false;
#endif
    return true;
}


inline bool PrimPosixTextFileImpl::seek(offset_type offset, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    offset_type result; // Dummy
    if (ARCHON_LIKELY(m_file.try_seek(offset, base::File::Whence::set, result, ec))) {
        m_retain_size = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}



// ============================ PrimWindowsTextFileImpl ============================


inline PrimWindowsTextFileImpl::PrimWindowsTextFileImpl(base::File& file, Config config) :
    m_file(file),
    m_buffer(config.newline_codec_buffer_memory, config.newline_codec_buffer_size) // Throws
{
}


inline bool PrimWindowsTextFileImpl::discard(std::error_code& ec) noexcept
{
    ARCHON_ASSERT(m_begin <= m_end);
    std::size_t n = std::size_t(m_end - m_begin);
    return do_discard(n, ec);
}


inline bool PrimWindowsTextFileImpl::revert(std::size_t offset, std::error_code& ec) noexcept
{
    ARCHON_ASSERT(m_retain_begin <= m_begin);
    ARCHON_ASSERT(m_begin <= m_end);
    base::Span data = base::Span(m_buffer).first(m_begin);
    std::size_t data_offset = m_retain_begin;
    bool success = base::newline_crlf::simulate_inc_decode(data, data_offset, offset);
    ARCHON_ASSERT(success);
    ARCHON_ASSERT(data_offset <= m_begin);
    std::size_t n = std::size_t(m_end - data_offset);
    return do_discard(n, ec);
}


inline bool PrimWindowsTextFileImpl::seek(offset_type offset, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    offset_type result; // Dummy
    if (ARCHON_LIKELY(m_file.try_seek(offset, base::File::Whence::set, result, ec))) {
        m_retain_begin = 0;
        m_begin = 0;
        m_end   = 0;
        m_retain_size  = 0;
        m_retain_clear = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


inline void PrimWindowsTextFileImpl::expand_buffer()
{
    m_buffer.expand(1, m_end); // Throws
}


} // namespace archon::base




// ============================================================================================ prim_text_file_impl.cpp ============================================================================================


namespace archon::base {


bool PrimWindowsTextFileImpl::read(base::Span<char> buffer, bool dynamic_eof, std::size_t& n,
                                   std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    bool end_of_file = false;
    for (;;) {
        base::Span data = base::Span(m_buffer).first(m_end);
        std::size_t buffer_offset = 0;
        std::size_t clear_offset = m_retain_size;
        base::newline_crlf::inc_decode(data, m_begin, buffer, buffer_offset,
                                       clear_offset, m_retain_clear, end_of_file);
        m_retain_size += buffer_offset;
        if (ARCHON_LIKELY(buffer_offset > 0 || buffer.size() == 0)) {
            n = buffer_offset;
            return true;
        }
        ARCHON_ASSERT(!end_of_file);
        // Move retained data to start of buffer
        ARCHON_ASSERT(m_retain_begin <= m_begin);
        char* base = m_buffer.data();
        std::copy(base + m_retain_begin, base + m_end, base);
        m_begin -= m_retain_begin;
        m_end   -= m_retain_begin;
        m_retain_begin = 0;
        if (ARCHON_UNLIKELY(m_end == m_buffer.size()))
            expand_buffer(); // Throws
        std::size_t n_2 = 0;
        if (ARCHON_LIKELY(m_file.try_read_some(base::Span(m_buffer).subspan(m_end), n_2, ec))) {
            if (ARCHON_LIKELY(n_2 > 0)) {
                m_end += n_2;
                continue;
            }
            if (ARCHON_LIKELY(m_end == m_begin || dynamic_eof)) {
                // Signal end of file
                n = 0;
                return true;
            }
            end_of_file = true;
            continue;
        }
        return false;
    }
}


bool PrimWindowsTextFileImpl::write(base::Span<const char> data, std::size_t& n,
                                    std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    std::size_t data_offset = 0;
    for (;;) {
        base::newline_crlf::inc_encode(data, data_offset, m_buffer, m_end);
        if (ARCHON_LIKELY(data_offset == data.size())) {
            n = data.size();
            return true;
        }
        if (ARCHON_LIKELY(m_end > 0)) {
            if (ARCHON_LIKELY(flush(ec)))
                continue;
            return false;
        }
        expand_buffer(); // Throws
    }
}


void PrimWindowsTextFileImpl::advance() noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_retain_begin <= m_begin);
    m_retain_begin = m_begin;
    m_retain_size  = 0;
    m_retain_clear = 0;
}


void PrimWindowsTextFileImpl::advance(std::size_t n) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(n <= m_retain_size);
    ARCHON_ASSERT(m_retain_begin <= m_begin);
    ARCHON_ASSERT(m_begin <= m_end);
    if (ARCHON_LIKELY(n >= m_retain_clear)) {
        m_retain_begin = std::size_t(m_begin - (m_retain_size - n));
        m_retain_size -= n;
        m_retain_clear = 0;
        return;
    }
    base::Span data = base::Span(m_buffer).first(m_begin);
    bool success = base::newline_crlf::simulate_inc_decode(data, m_retain_begin, n);
    ARCHON_ASSERT(success);
    ARCHON_ASSERT(m_retain_begin <= m_begin);
    m_retain_size  -= n;
    m_retain_clear -= n;
}


bool PrimWindowsTextFileImpl::flush(std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    ARCHON_ASSERT(m_retain_begin == 0);
    ARCHON_ASSERT(m_begin <= m_end);
    ARCHON_ASSERT(m_retain_size == 0);
    ARCHON_ASSERT(m_retain_clear == 0);
    base::Span data = base::Span(m_buffer).first(m_end).subspan(m_begin);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_file.try_write(data, n, ec))) {
        m_begin = 0;
        m_end = 0;
#if ARCHON_DEBUG
        m_writing = false;
#endif
        return true;
    }
    m_begin += n;
    return false;
}


bool PrimWindowsTextFileImpl::do_discard(std::size_t n, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    auto whence = base::File::Whence::cur;
    offset_type result; // Dummy
    // Avoid actual seeking when n is zero, such that writing can be enabled for
    // an underlying file that does not support seeking, e.g., STDOUT.
    if (ARCHON_LIKELY(n == 0 || m_file.try_seek(-offset_type(n), whence, result, ec))) {
        m_retain_begin = 0;
        m_begin = 0;
        m_end   = 0;
        m_retain_size  = 0;
        m_retain_clear = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


} // namespace archon::base





// ============================================================================================ prim_text_file.hpp ============================================================================================


namespace archon::base {


struct PrimTextFileConfig;


template<class I> class BasicPrimTextFile {
public:
    using Config = PrimTextFileConfig;
    using Mode        = base::File::Mode;
    using offset_type = base::File::offset_type;

    BasicPrimTextFile(base::FilesystemPathRef, Mode = Mode::read);
    BasicPrimTextFile(base::FilesystemPathRef, Mode, Config);

    std::size_t read_some(base::Span<char> buffer);

    std::size_t read(base::Span<char> buffer);

    void write(base::Span<const char> data);

    void flush();

    void seek(offset_type);

    [[nodiscard]] bool try_read_some(base::Span<char> buffer, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_read(base::Span<char> buffer, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_write(base::Span<const char> data, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_flush(std::error_code&);

    [[nodiscard]] bool try_seek(offset_type, std::error_code&);

private:
    base::File m_file;
    I m_impl;
    bool m_dynamic_eof;
    bool m_reading = false;
    bool m_writing = false;

    bool stop_reading(std::error_code&);
    bool stop_writing(std::error_code&);
    bool do_read_some(base::Span<char> buffer, std::size_t& n, std::error_code&);
};


using PrimTextFile        = BasicPrimTextFile<base::PrimTextFileImpl>;
using PrimPosixTextFile   = BasicPrimTextFile<base::PrimPosixTextFileImpl>;
using PrimWindowsTextFile = BasicPrimTextFile<base::PrimWindowsTextFileImpl>;


struct PrimTextFileConfig : public base::PrimTextFileImplConfig {
    bool dynamic_eof = false;
};







// Implementation


template<class I>
inline BasicPrimTextFile<I>::BasicPrimTextFile(base::FilesystemPathRef path, Mode mode) :
    BasicPrimTextFile(path, mode, {}) // Throws
{
}


template<class I>
inline BasicPrimTextFile<I>::BasicPrimTextFile(base::FilesystemPathRef path, Mode mode,
                                               Config config) :
    m_file(path, mode), // Throws
    m_impl(m_file, std::move(config)), // Throws
    m_dynamic_eof(config.dynamic_eof)
{
}


template<class I> inline std::size_t BasicPrimTextFile<I>::read_some(base::Span<char> buffer)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_some(buffer, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


template<class I> inline std::size_t BasicPrimTextFile<I>::read(base::Span<char> buffer)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read(buffer, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


template<class I> inline void BasicPrimTextFile<I>::write(base::Span<const char> data)
{
    std::size_t n; // Dummy
    std::error_code ec;
    if (ARCHON_LIKELY(try_write(data, n, ec)))
        return; // Success
    throw std::system_error(ec, "Failed to write to file");
}


template<class I> inline void BasicPrimTextFile<I>::flush()
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_flush(ec)))
        return; // Success
    throw std::system_error(ec, "Failed to flush");
}


template<class I> inline void BasicPrimTextFile<I>::seek(offset_type offset)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_seek(offset, ec)))
        return; // Success
    throw std::system_error(ec, "Failed to seek");
}


template<class I>
inline bool BasicPrimTextFile<I>::try_read_some(base::Span<char> buffer, std::size_t& n,
                                                std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_writing || stop_writing(ec))) { // Throws
        m_reading = true;
        return do_read_some(buffer, n, ec); // Throws
    }
    return false;
}


template<class I>
inline bool BasicPrimTextFile<I>::try_read(base::Span<char> buffer, std::size_t& n,
                                           std::error_code& ec)
{
    base::Span<char> buffer_2 = buffer;
    if (ARCHON_LIKELY(!m_writing || stop_writing(ec))) { // Throws
        m_reading = true;
        std::size_t n_2 = 0;
      again:
        if (ARCHON_LIKELY(do_read_some(buffer_2, n_2, ec))) {
            ARCHON_ASSERT(n_2 <= buffer_2.size());
            if (ARCHON_LIKELY(n_2 != 0 && n_2 < buffer_2.size())) {
                buffer_2 = buffer_2.subspan(n_2);
                goto again;
            }
            n = std::size_t(buffer_2.data() + n_2 - buffer.data());
            return true;
        }
    }
    n = std::size_t(buffer_2.data() - buffer.data());
    return false;
}


template<class I>
inline bool BasicPrimTextFile<I>::try_write(base::Span<const char> data, std::size_t& n,
                                            std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_reading || stop_reading(ec))) { // Throws
        m_writing = true;
        return m_impl.write(data, n, ec); // Throws
    }
    n = 0;
    return false;
}


template<class I>
inline bool BasicPrimTextFile<I>::try_flush(std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_writing || m_impl.flush(ec))) { // Throws
        m_writing = false;
        return true;
    }
    return false;
}


template<class I>
inline bool BasicPrimTextFile<I>::try_seek(offset_type offset, std::error_code& ec)
{
    bool success = ((!m_writing || stop_writing(ec)) && m_impl.seek(offset, ec)); // Throws
    if (ARCHON_LIKELY(success)) {
        m_reading = false;
        return true;
    }
    return false;
}


template<class I> bool BasicPrimTextFile<I>::stop_reading(std::error_code& ec)
{
    ARCHON_ASSERT(m_reading);
    ARCHON_ASSERT(!m_writing);
    if (ARCHON_LIKELY(m_impl.discard(ec))) { // Throws
        m_reading = false;
        return true;
    }
    return false;
}


template<class I> bool BasicPrimTextFile<I>::stop_writing(std::error_code& ec)
{
    ARCHON_ASSERT(!m_reading);
    ARCHON_ASSERT(m_writing);
    if (ARCHON_LIKELY(m_impl.flush(ec))) { // Throws
        m_writing = false;
        return true;
    }
    return false;
}


template<class I>
inline bool BasicPrimTextFile<I>::do_read_some(base::Span<char> buffer, std::size_t& n,
                                               std::error_code& ec)
{
    ARCHON_ASSERT(!m_writing);
    if (ARCHON_LIKELY(m_impl.read(buffer, m_dynamic_eof, n, ec))) { // Throws
        m_impl.advance();
        return true;
    }
    return false;
}


} // namespace archon::base





// ============================================================================================ text_file_impl.hpp ============================================================================================


namespace archon::base {


template<class C, class T, class P> class TextFileImpl {
public:
    struct Config;

    using char_type   = C;
    using traits_type = T;
    using string_view_type = std::basic_string_view<C, T>;
    using offset_type = typename P::offset_type;

    TextFileImpl(base::File&, const std::locale&, Config);

    [[nodiscard]] bool read(base::Span<C> buffer, bool dynamic_eof, std::size_t& n,
                            std::error_code&);

    [[nodiscard]] bool write(string_view_type data, std::size_t& n, std::error_code&);

    // GENERIC:
    //
    // BRIEF: Revert file offset and discard pre-loaded data.
    //
    // Maybe change description below like at primitive level                   
    //
    // On success, the byte-level file offset is reverted to where it would have
    // been if the size of the caller's buffer during the last invocation of
    // read() had been `offset`, and the read operation had resulted in a
    // situation where no data was pre-loaded.
    //
    // When in reading mode, `offset` must be less than, or equal to the size of
    // the extracted chunk during the last invocation of read(), which is zero
    // if that read operation failed. When in neutral mode, `offset` must be
    // zero.
    //
    [[nodiscard]] bool revert(std::size_t offset, std::error_code&);

    [[nodiscard]] bool flush(std::error_code&);

    [[nodiscard]] bool seek(offset_type, std::error_code&);

private:
    P m_prim_impl;
    base::BasicCharCodec<C, T> m_codec;
    base::SeedMemoryBuffer<char> m_buffer;
    std::mbstate_t m_prev_state = {};
    std::mbstate_t m_state = {};
    std::size_t m_prev_begin = 0;
    std::size_t m_begin = 0;
    std::size_t m_end   = 0;

#if ARCHON_DEBUG
    bool m_reading = false;
    bool m_writing = false;
#endif

    static base::SeedMemoryBuffer<char> make_buffer(Config&);
    static constexpr std::size_t max_buffer_size() noexcept;

    bool shallow_flush(std::error_code&);
    void expand_buffer();
};


template<class C, class T, class P> struct TextFileImpl<C, T, P>::Config :
        base::PrimTextFileImplConfig {
    static constexpr std::size_t default_char_codec_buffer_size = 1024;

    // GENERIC:
    //
    // Buffer will be automatically expanded if necessary.              
    //
    std::size_t char_codec_buffer_size = default_char_codec_buffer_size;
    base::Span<char> char_codec_buffer_memory;
};








// Implementation


template<class C, class T, class P>
inline TextFileImpl<C, T, P>::TextFileImpl(base::File& file, const std::locale& locale,
                                           Config config) :
    m_prim_impl(file, std::move(config)), // Throws
    m_codec(locale), // Throws
    m_buffer(make_buffer(config)) // Throws
{
}


template<class C, class T, class P>
bool TextFileImpl<C, T, P>::read(base::Span<C> buffer, bool dynamic_eof, std::size_t& n,
                                 std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    // Tend to m_state management    

    bool end_of_file = false;
    m_prev_state = m_state;
    m_prev_begin = m_begin;
    for (;;) {
        base::Span data = base::Span(m_buffer).first(m_end);
        std::size_t buffer_offset = 0;
        bool error = false;
        if (end_of_file)
            error = true;                   
        bool complete =
            m_codec.inc_decode(m_state, data, m_begin, buffer, buffer_offset, error);
        if (ARCHON_LIKELY(buffer_offset > 0 || buffer.size() == 0)) {
            n = buffer_offset;
            return true;
        }
        if (!error) {
            ARCHON_ASSERT(!end_of_file);
            // Move any retained data to start of buffer
            m_prim_impl.advance(m_begin);
            char* base = m_buffer.data();
            std::copy(base + m_begin, base + m_end, base);
            m_end -= m_begin;
            m_begin = 0;
            m_prev_state = m_state;
            m_prev_begin = 0;
            if (ARCHON_UNLIKELY(m_end == m_buffer.size()))
                expand_buffer(); // Throws
            std::size_t n_2 = 0;
            base::Span<char> buffer_2 = base::Span(m_buffer).subspan(m_end);
            if (ARCHON_LIKELY(m_prim_impl.read(buffer_2, dynamic_eof, n_2, ec))) { // Throws
                if (ARCHON_LIKELY(n_2 > 0)) {
                    m_end += n_2;
                    continue;
                }
                if (ARCHON_LIKELY(complete || dynamic_eof)) {
                    // Signal end of file
                    n = 0;
                    return true;
                }
                end_of_file = true;
                continue;
            }
            return false;
        }
        // FIXME: Should introduce new error codes.  
        // FIXME: Report EILSEQ / std::errc::illegal_byte_sequence when not in lenient mode       
        ec = std::make_error_code(std::errc::illegal_byte_sequence);                  
        return false;
    }
}


template<class C, class T, class P>
bool TextFileImpl<C, T, P>::write(string_view_type data, std::size_t& n, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    std::size_t data_offset = 0;
    for (;;) {
        std::codecvt_base::result result =
            m_codec.inc_encode(m_state, data, data_offset, m_buffer, m_end);
        switch (result) {
            case std::codecvt_base::ok:
                // All specified input has been converted, unshift is still
                // required eventually
                ARCHON_ASSERT(data_offset == data.size());
                n = data.size();
                return true;
            case std::codecvt_base::partial:
                // Need more space in output buffer
                if (ARCHON_LIKELY(m_end > 0)) {
                    if (ARCHON_LIKELY(shallow_flush(ec))) // Throws
                        continue;
                    return false;
                }
                expand_buffer(); // Throws
                continue;
            case std::codecvt_base::error:
                // FIXME: Should introduce new error codes.  
                // FIXME: Report EDOM / std::errc::argument_out_of_domain when not in lenient mode       
                n = data_offset;
                ec = std::make_error_code(std::errc::argument_out_of_domain);                  
                return false;
            case std::codecvt_base::noconv:
                break;
        }
        ARCHON_ASSERT_UNREACHABLE;
        n = data.size();
        return true;
    }
}


template<class C, class T, class P>
bool TextFileImpl<C, T, P>::revert(std::size_t offset, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_prev_begin <= m_begin);
    ARCHON_ASSERT(m_begin <= m_end);
    base::Span data = base::Span(m_buffer).first(m_begin);
    std::mbstate_t state = m_prev_state;
    std::size_t data_offset = m_prev_begin;
    // The difference between `data_offset` and `data.size()` cannot be greater
    // than the size of `m_buffer`, and the buffer cannot grow larger than
    // `m_codec.max_simulate_decode_size()`
    m_codec.simulate_inc_decode(state, data, data_offset, offset); // Throws
    ARCHON_ASSERT(data_offset <= m_begin);
    if (ARCHON_LIKELY(m_prim_impl.revert(data_offset, ec))) { // Throws
        m_prev_begin = 0;
        m_begin = 0;
        m_end   = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class C, class T, class P>
inline bool TextFileImpl<C, T, P>::flush(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    if (ARCHON_LIKELY(shallow_flush(ec) && m_prim_impl.flush(ec))) { // Throws
#if ARCHON_DEBUG
        m_writing = false;
#endif
        return true;
    }
    return false;
}


template<class C, class T, class P>
inline bool TextFileImpl<C, T, P>::seek(offset_type offset, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    if (ARCHON_LIKELY(m_prim_impl.seek(offset, ec))) { // Throws
        m_prev_begin = 0;
        m_begin = 0;
        m_end   = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class C, class T, class P> auto TextFileImpl<C, T, P>::make_buffer(Config& config) ->
    base::SeedMemoryBuffer<char>
{
    base::Span<char> seed_memory = config.char_codec_buffer_memory;
    if (ARCHON_UNLIKELY(seed_memory.size() > max_buffer_size()))
        seed_memory = seed_memory.subspan(0, max_buffer_size());
    std::size_t size = config.char_codec_buffer_size;
    if (ARCHON_UNLIKELY(size > max_buffer_size()))
        size = max_buffer_size();
    return base::SeedMemoryBuffer<char>(seed_memory, size); // Throws
}


template<class C, class T, class P>
constexpr std::size_t TextFileImpl<C, T, P>::max_buffer_size() noexcept
{
    return decltype(m_codec)::max_simulate_decode_size();
}


template<class C, class T, class P> bool TextFileImpl<C, T, P>::shallow_flush(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    ARCHON_ASSERT(m_prev_begin == 0);
    ARCHON_ASSERT(m_begin <= m_end);
    base::Span data = base::Span(m_buffer).first(m_end).subspan(m_begin);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_prim_impl.write(data, n, ec))) { // Throws
        m_begin = 0;
        m_end = 0;
        return true;
    }
    m_begin += n;
    return false;
}


template<class C, class T, class P> inline void TextFileImpl<C, T, P>::expand_buffer()
{
    m_buffer.expand(1, m_end, max_buffer_size()); // Throws
}


} // namespace archon::base
















using namespace archon;


ARCHON_TEST(Base_PrimTextFileImpl_Windows)
{
    // FIXME: Make similar test for POSIX variant  

    ARCHON_TEST_FILE(path);
    base::File file(path, base::File::Mode::write);
    base::PrimTextFileImplConfig config;
    config.newline_codec_buffer_size = 16;
    base::PrimWindowsTextFileImpl text_file_impl(file, std::move(config));
    std::array<char, 64> buffer;
    bool dynamic_eof = false;
    std::size_t n = 0;
    bool success;
    std::error_code ec;

    log("------ write ------");
    success = text_file_impl.write(std::string_view("foo\nbar\nbaz\n"), n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);

    log("------ flush ------");
    success = text_file_impl.flush(ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ seek ------");
    success = text_file_impl.seek(5, ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ read ------");
    n = 0;
    success = text_file_impl.read(buffer, dynamic_eof, n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);
    if (success)
        log("contents = %s", base::quoted(std::string_view(buffer.data(), n)));

    log("------ revert ------");
    success = text_file_impl.revert(5, ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ write ------");
    success = text_file_impl.write(std::string_view("o"), n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);

    log("------ flush ------");
    success = text_file_impl.flush(ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ seek ------");
    success = text_file_impl.seek(0, ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ read ------");
    n = 0;
    success = text_file_impl.read(buffer, dynamic_eof, n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);
    if (success)
        log("contents = %s", base::quoted(std::string_view(buffer.data(), n)));

    // FIXME: Also check dynamic_eof   
}


ARCHON_TEST(Base_PrimTextFile_PosixRead)
{
    ARCHON_TEST_FILE(path);
    {
        base::File file(path, base::File::Mode::write);
        file.write(std::string_view("foo\r\nbar\r\nbaz\r\n"));
    }
    base::PrimPosixTextFile text_file(path);
    std::array<char, 64> buffer;
    std::size_t n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "foo\r\nbar\r\nbaz\r\n");
}


ARCHON_TEST(Base_PrimTextFile_PosixWriteAndFlush)
{
    ARCHON_TEST_FILE(path);
    {
        base::PrimPosixTextFile text_file(path, base::File::Mode::write);
        text_file.write(std::string_view("foo\nbar\nbaz\n"));
        text_file.flush();
    }
    base::File file(path);
    std::array<char, 64> buffer;
    std::size_t n = file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "foo\nbar\nbaz\n");
}


ARCHON_TEST(Base_PrimTextFile_WindowsRead)
{
    ARCHON_TEST_FILE(path);
    {
        base::File file(path, base::File::Mode::write);
        file.write(std::string_view("foo\r\nbar\r\nbaz\r\n"));
    }
    base::PrimTextFileConfig config;
    config.newline_codec_buffer_size = 3;
    base::PrimWindowsTextFile text_file(path, base::File::Mode::read, std::move(config));
    std::array<char, 64> buffer;
    std::size_t n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "foo\nbar\nbaz\n");
}


ARCHON_TEST(Base_PrimTextFile_WindowsWriteAndFlush)
{
    ARCHON_TEST_FILE(path);
    {
        base::PrimTextFileConfig config;
        config.newline_codec_buffer_size = 3;
        base::PrimWindowsTextFile text_file(path, base::File::Mode::write, std::move(config));
        text_file.write(std::string_view("foo\nbar\nbaz\n"));
        text_file.flush();
    }
    base::File file(path);
    std::array<char, 64> buffer;
    std::size_t n = file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "foo\r\nbar\r\nbaz\r\n");
}


ARCHON_TEST(Base_PrimTextFile_Windows)
{
    // FIXME: Make similar test for POSIX variant  

    ARCHON_TEST_FILE(path);
    base::PrimTextFileConfig config;
    config.newline_codec_buffer_size = 16;
    base::PrimWindowsTextFile text_file(path, base::File::Mode::write, std::move(config));

    std::array<char, 64> buffer;
    std::size_t n;
    text_file.write(std::string_view("foo\nbar\nbaz\n"));
    text_file.seek(5);
    n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "bar\nbaz\n");
    text_file.seek(11);
    text_file.write(std::string_view("o"));
    text_file.seek(0);
    n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "foo\nbar\nboz\n");

    // Must test some alternating read and write, especially read to write      

    // Must test flush  

    // One time: Check that (crlf<->nl) buffer does not grow if initial size is 2 and whole file (size > 2 bytes) is read        
}


// TEMPORARY                                                                           
ARCHON_TEST(Base_PrimTextFile_WindowsFoo)
{
    ARCHON_TEST_FILE(path);
    {
        base::File file(path, base::File::Mode::write);
        file.write(std::string_view("abcdefghijklmnopqrstuvwxyz"));
    }
    {
        base::PrimTextFileConfig config;
        config.newline_codec_buffer_size = 1;
        base::PrimWindowsTextFile text_file(path, base::File::Mode::read, std::move(config));
        std::array<char, 64> buffer;
        std::size_t n = text_file.read(buffer);
        log("contents = %s", base::quoted(std::string_view(buffer.data(), n)));
    }
}


ARCHON_TEST(Base_TextFileImpl_Windows)
{
    // FIXME: Make similar test for POSIX variant  

    // FIXME: Do this for both narrow and wide character types  

    ARCHON_TEST_FILE(path);
    base::File file(path, base::File::Mode::write);
    using text_file_impl_type = base::TextFileImpl<wchar_t, std::char_traits<wchar_t>, base::PrimWindowsTextFileImpl>;
    const std::locale& locale = test_context.get_locale();         
    text_file_impl_type::Config config;
    config.char_codec_buffer_size = 16;
    text_file_impl_type text_file_impl(file, locale, std::move(config));
    std::array<wchar_t, 64> buffer;
    bool dynamic_eof = false;
    std::size_t n = 0;
    bool success;
    std::error_code ec;

    log("------ write ------");
    success = text_file_impl.write(std::wstring_view(L"foo\nbar\nbaz\n"), n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);

    log("------ flush ------");
    success = text_file_impl.flush(ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ seek ------");
    success = text_file_impl.seek(5, ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ read ------");
    n = 0;
    success = text_file_impl.read(buffer, dynamic_eof, n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);
    if (success)
        log("contents = %s", base::format_enc<wchar_t>(locale, "%s", base::quoted(std::wstring_view(buffer.data(), n))));

    log("------ revert ------");
    success = text_file_impl.revert(5, ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ write ------");
    success = text_file_impl.write(std::wstring_view(L"o"), n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);

    log("------ flush ------");
    success = text_file_impl.flush(ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ seek ------");
    success = text_file_impl.seek(0, ec);
    log(" success = %s", success);
    log("      ec = %s", ec);

    log("------ read ------");
    n = 0;
    success = text_file_impl.read(buffer, dynamic_eof, n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);
    if (success)
        log("contents = %s", base::format_enc<wchar_t>(locale, "%s", base::quoted(std::wstring_view(buffer.data(), n))));

    // FIXME: Also check dynamic_eof   
}
