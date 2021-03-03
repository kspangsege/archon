#include <streambuf>
#include <iostream>

#include <archon/base/demangle.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/format_enc.hpp>
#include <archon/base/file.hpp>
#include <archon/unit_test.hpp>


/*               




Linux ---> C.UTF-8



add encode test

VARY BUFFER SIZES RANDOMLY, especially in Base_TextFile_EncodeError
hmmmmmmm    a bit of a problem that unit test repetition does not try many different random seeds
---> Solution: Always compute a new seed sequence when a unit test ask for one, and do it in a way that combines the command line specified seed as well as the index into the repetition sequence. Then also provide a command line option to override the index into the repetition sequence as it is used for the random seed.


GENERAL RULE ALL THE WAY FROM basic text file down for base::File is that if write()/try_write() fails, then n is strictly less than data.size()                  
----> Similarly, if read() fails, then n is strictly less than buffer.size()


OOOOOOOPS, apparently we cannot assume that when write() fails, `n` is the position of the failure (buffering destroys this) Be sure that the documentation doe not promise too much   
--> generic guarantee: On failure, at least all the characters that preceed the position of the error will have been written, so the position of the error is less than, or equal to `n`.   




Want: ARCHON_CHECK_IN_RANGE(var, min, max)
Want: ARCHON_CHECK_IN_SET(var, a, b, c, ...)

*/



// Make note about flush(), that when it fails and, the reason is that a character could not be encoded, it still promises to flush everything up to that character.



// Consider: Char codec decode never error if input consumed or output produced, and return error flag, not completion flag ---> hmm






// FIXME: Figure out whether ARCHON_C_LOCALE_IS_ASCII should be true on other platforms, such as macOS and Windows. AND FIX FIXME IN feature.h                   




// Next:

// - Add BasicTextFileStreambuf<C, T, I> based on unnbuffered impl, but adding its own buffer.
// - NO: Hideaway Impl in detail namespace.
// - Tend to load and save --> there should probably not be a load_and_chomp on base::File.





// General rule: read_some() will only block if there is nothing more that can be read at any level without blocking.


// Generic rule for non-primitive impl: Must be in reading mode if the subfile is in reading mode.
// Generic rule for non-primitive impl: Must be in writing mode if the subfile is in writing mode.


// BasicPrimTextFile will probably end up being completely redundant, as it will be effecively identidcal to BasicTextFile using `char` as character type.


// Consider adding constructor to BasicTextFile::BasicTextFile(base::File&, std::mbstate_t initial_state = {})


// Consider adding `write(const C* c_str)` and `try_write(const C* c_str, ...)` overloads to non-impl test files.


// Max possible file size seems to be 17592186040320 on my Linux box (11111111111111111111111111111111000000000000 in binary) (44 bits) (~16TiB)


/*

Idea:

change
- There are three file positions to keep track of, the logical read/write position, the read ahead position, and the actual read/write position.
- The logical read/write position is the start position for the next write operation (\ref write()) and the initial position for the read ahead position after switching to read mode.
- The read ahead position is the start position for the next read ahead operation (\ref read_ahead()).
- The actual read/write position is the read/write position recognized by \ref base::File. POSIX calls this, the file offset.
- While in netral mode, all three positions coincide.
- While in reading mode, the read ahead position is always greater than, or equal to the logical read/write position, and the actual read/write position is always greater than, or equal to the read ahead position.
- While in writing mode, the read ahead position is undefined, and the logical read/write position is always greater than, or equal to the actual read/write position.

- Introduce Impl::tell_read() which determines logical read/write position while in neutral, or in reading mode.
- Introduce Impl::tell_write() which determines logical read/write position while in writing mode. This one effectively flushes all the way to, but not including the lowest level.

- Impl::read_some() and Impl::write() advance the logical read/write position.
- Impl::read_ahead() advances the read ahead position, not the logical read/write position.
- Impl::advance(), without arguments, moves the logical read/write position to the current read ahead position.
- Impl::advance(), with an argument, moves the logical read/write position to the direction of the current read ahead position, but it cannot move it beyond the current read ahead position.
- Impl::flush() causes the actual read/write position to be moved to the current logical read/write position (by way of flushing).

- WRONG: Impl::discard() makes the actual read/write position coincide with the logical read/write position.                                                                                                                                                                                     

- Impl::discard() ???  
- Impl::revert() ???  

*/






// Consideer: make its such that codec decode only returns true if eof arg is true and conversion is complete (in order to deal with quirks of std::codecvt in libstdc++ and libc++)              


// Consider: NO: Put Impl in detail namespace and source files in impl (or detail) subdir


// Consider: BufferedFile for binary stuff








// This started the whole thing: Consider introducing BasicTextFile<C, T, F>, and then build BasicTextFileStreambuf<C, T, F> on top of it. Here F is the underlying file type, which is base::PrimTextFile by default. base::PrimTextFile is an alias for base::PrimPosixTextFile on POSIX, and an alias for base::PrimWindowsTextFile on Windows. base::PrimWindowsTextFile is buffered. base::PrimPosixTextFile is unbuffered.                                 




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
    return false;
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
/// A precondition for calling read_ahead(), is that the file is not in writing
/// mode. After an invocation of read_ahead(), the file is in reading mode, even
/// if the operation fails.
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
/// A precondition of calling flush() is that the file is not in reading
/// mode. After a successful invocation of flush(), the file is in neutral
/// mode. After a failed invocation of flush(), the mode is unchanged.
///
/// A precondition for calling tell_read(), is that the file is not in writing
/// mode. After an invocation of tell_read() the mode is unchanged.
///
/// A precondition for calling tell_write(), is that the file is not in reading
/// mode. After an invocation of tell_write() the mode is unchanged.
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
    using Config   = PrimTextFileImplConfig;
    using pos_type = base::File::offset_type;

    static constexpr bool has_windows_newline_codec = false;

    PrimPosixTextFileImpl(base::File&, Config) noexcept;

    [[nodiscard]] bool read_ahead(base::Span<char> buffer, bool dynamic_eof, std::size_t& n,
                                  std::error_code&);

    [[nodiscard]] bool write(base::Span<const char> data, std::size_t& n,
                             std::error_code&) noexcept;

    void advance() noexcept;
    void advance(std::size_t n) noexcept;

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&) noexcept;

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&) noexcept;
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&) noexcept;

    [[nodiscard]] bool seek(pos_type, std::error_code&) noexcept;

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
    using Config   = PrimTextFileImplConfig;
    using pos_type = base::File::offset_type;

    static constexpr bool has_windows_newline_codec = true;

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
    [[nodiscard]] bool read_ahead(base::Span<char> buffer, bool dynamic_eof, std::size_t& n,
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
    // `noexcept` is important here, but only for the zero-arg version (all other functions must be considered throwing from a generic point of view)                                                 
    //
    void advance() noexcept;
    void advance(std::size_t n) noexcept;

    // GENERIC:                 
    //
    // Revert read-ahead position, and actual read/write position to coincide with the current logical read/write position.
    //
    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&) noexcept;

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&) noexcept;
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&) noexcept;

    [[nodiscard]] bool seek(pos_type, std::error_code&) noexcept;

private:
    base::File& m_file;
    base::SeedMemoryBuffer<char> m_buffer;

    // Beginning and end of the current contents of the buffer. In neutral mode,
    // both are zero. In reading mode, `m_begin` corresponds to the logical
    // read/write position, and `m_end` corresponds to the actual read/write
    // position. In writing mode, it is the other way around.
    std::size_t m_begin = 0;
    std::size_t m_end   = 0;

    // Position in buffer corresponding to the read ahead position. In neutral
    // mode, and in writing mode, it is always zero.
    std::size_t m_curr = 0;

    // In reading mode, `m_retain_size` is the number of decoded characters
    // between the logical read/write position and the read ahead position, and
    // `m_retain_clear` is the number of decoded characters that needs to be
    // advanced by in order to clear all newline conversions in the retained
    // part. Both are zero in neutral mode, and in writing mode.
    std::size_t m_retain_size  = 0;
    std::size_t m_retain_clear = 0;

#if ARCHON_DEBUG
    bool m_reading = false;
    bool m_writing = false;
#endif

    bool do_flush(std::error_code&) noexcept;
    void expand_buffer();
};








// Implementation


// ============================ PrimPosixTextFileImpl ============================


inline PrimPosixTextFileImpl::PrimPosixTextFileImpl(base::File& file, Config) noexcept :
    m_file(file)
{
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


inline bool PrimPosixTextFileImpl::flush(std::error_code&) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = false;
#endif
    return true;
}


inline bool PrimPosixTextFileImpl::tell_read(pos_type& pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    pos_type result;
    if (ARCHON_LIKELY(m_file.try_seek(0, base::File::Whence::cur, result, ec))) {
        if (ARCHON_LIKELY(base::try_int_sub(result, m_retain_size))) {
            pos = result;
            return true;
        }
        // We only get here if the actual read/write position was manipulated
        // outside the control of this text file object.
        pos = 0;
        return true;
    }
    return false;
}


inline bool PrimPosixTextFileImpl::tell_write(pos_type& pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    return m_file.try_seek(0, base::File::Whence::cur, pos, ec);
}


inline bool PrimPosixTextFileImpl::seek(pos_type pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    pos_type result; // Dummy
    if (ARCHON_LIKELY(m_file.try_seek(pos, base::File::Whence::set, result, ec))) {
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


inline bool PrimWindowsTextFileImpl::flush(std::error_code& ec) noexcept
{
    ARCHON_ASSERT(m_curr == 0);
    ARCHON_ASSERT(m_retain_size  == 0);
    ARCHON_ASSERT(m_retain_clear == 0);
    if (ARCHON_LIKELY(do_flush(ec))) {
#if ARCHON_DEBUG
        m_writing = false;
#endif
        return true;
    }
    return false;
}


inline bool PrimWindowsTextFileImpl::seek(pos_type pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    pos_type result; // Dummy
    if (ARCHON_LIKELY(m_file.try_seek(pos, base::File::Whence::set, result, ec))) {
        m_begin = 0;
        m_end   = 0;
        m_curr  = 0;
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


// ============================ PrimPosixTextFileImpl ============================


bool PrimPosixTextFileImpl::read_ahead(base::Span<char> buffer, bool, std::size_t& n,
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
            throw std::length_error("Retain size");
        buffer_size = max;
    }

    if (ARCHON_LIKELY(m_file.try_read_some({ buffer.data(), buffer_size }, n, ec))) {
        m_retain_size += n;
        return true;
    }
    return false;
}


bool PrimPosixTextFileImpl::discard(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    pos_type n = 0;
    if (ARCHON_LIKELY(base::try_int_cast(m_retain_size, n))) {
        auto whence = base::File::Whence::cur;
        pos_type result; // Dummy
        if (ARCHON_LIKELY(n == 0 || m_file.try_seek(-n, whence, result, ec))) {
            m_retain_size = 0;
#if ARCHON_DEBUG
            m_reading = false;
#endif
            return true;
        }
        return false;
    }
    throw std::length_error("Retain size");
}



// ============================ PrimWindowsTextFileImpl ============================


bool PrimWindowsTextFileImpl::read_ahead(base::Span<char> buffer, bool dynamic_eof, std::size_t& n,
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
        base::newline_crlf::inc_decode(data, m_curr, buffer, buffer_offset,
                                       clear_offset, m_retain_clear, end_of_file);
        m_retain_size += buffer_offset;
        if (ARCHON_LIKELY(buffer_offset > 0 || buffer.size() == 0)) {
            n = buffer_offset;
            return true;
        }
        ARCHON_ASSERT(!end_of_file);
        // Move retained data to start of buffer
        ARCHON_ASSERT(m_begin <= m_curr);
        char* base = m_buffer.data();
        std::copy(base + m_begin, base + m_end, base);
        m_curr -= m_begin;
        m_end  -= m_begin;
        m_begin = 0;
        if (ARCHON_UNLIKELY(m_end == m_buffer.size()))
            expand_buffer(); // Throws
        std::size_t n_2 = 0;
        base::Span buffer_2 = base::Span(m_buffer).subspan(m_end);
        if (ARCHON_LIKELY(m_file.try_read_some(buffer_2, n_2, ec))) {
            if (ARCHON_LIKELY(n_2 > 0)) {
                m_end += n_2;
                continue;
            }
            if (ARCHON_LIKELY(m_end == m_curr || dynamic_eof)) {
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
            if (ARCHON_LIKELY(do_flush(ec)))
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

    ARCHON_ASSERT(m_begin <= m_curr);
    m_begin = m_curr;
    m_retain_size  = 0;
    m_retain_clear = 0;
}


void PrimWindowsTextFileImpl::advance(std::size_t n) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(n <= m_retain_size);
    ARCHON_ASSERT(m_begin <= m_curr);
    ARCHON_ASSERT(m_curr <= m_end);
    if (ARCHON_LIKELY(n >= m_retain_clear)) {
        m_begin = std::size_t(m_curr - (m_retain_size - n));
        m_retain_size -= n;
        m_retain_clear = 0;
        return;
    }
    base::Span data = base::Span(m_buffer).first(m_curr);
    bool success = base::newline_crlf::simulate_inc_decode(data, m_begin, n);
    ARCHON_ASSERT(success);
    ARCHON_ASSERT(m_begin <= m_curr);
    m_retain_size  -= n;
    m_retain_clear -= n;
}


bool PrimWindowsTextFileImpl::discard(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    pos_type n = 0;
    if (ARCHON_LIKELY(base::try_int_cast(m_end - m_begin, n))) {
        auto whence = base::File::Whence::cur;
        pos_type result; // Dummy
        if (ARCHON_LIKELY(n == 0 || m_file.try_seek(-n, whence, result, ec))) {
            m_begin = 0;
            m_end   = 0;
            m_curr  = 0;
            m_retain_size  = 0;
            m_retain_clear = 0;
#if ARCHON_DEBUG
            m_reading = false;
#endif
            return true;
        }
        return false;
    }
    throw std::length_error("Retain size");
}


bool PrimWindowsTextFileImpl::tell_read(pos_type& pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    pos_type result;
    if (ARCHON_LIKELY(m_file.try_seek(0, base::File::Whence::cur, result, ec))) {
        if (ARCHON_LIKELY(base::try_int_sub(result, m_end - m_begin))) {
            pos = result;
            return true;
        }
        // We only get here if the actual read/write position was manipulated
        // outside the control of this text file object.
        pos = 0;
        return true;
    }
    return false;
}


bool PrimWindowsTextFileImpl::tell_write(pos_type& pos, std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    pos_type result;
    if (ARCHON_LIKELY(m_file.try_seek(0, base::File::Whence::cur, result, ec))) {
        if (ARCHON_LIKELY(base::try_int_add(result, m_end - m_begin))) {
            pos = result;
            return true;
        }
        ec = make_error_code(std::errc::file_too_large);
    }
    return false;
}


bool PrimWindowsTextFileImpl::do_flush(std::error_code& ec) noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    base::Span data = base::Span(m_buffer).first(m_end).subspan(m_begin);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_file.try_write(data, n, ec))) {
        m_begin = 0;
        m_end   = 0;
        return true;
    }
    m_begin += n;
    return false;
}


} // namespace archon::base





// ============================================================================================ prim_text_file.hpp ============================================================================================


namespace archon::base {


struct PrimTextFileConfig;


template<class I> class BasicPrimTextFile {
public:
    using Config   = PrimTextFileConfig;
    using Mode     = base::File::Mode;
    using pos_type = base::File::offset_type;

    BasicPrimTextFile(base::FilesystemPathRef, Mode = Mode::read);
    BasicPrimTextFile(base::FilesystemPathRef, Mode, Config);

    std::size_t read_some(base::Span<char> buffer);

    std::size_t read(base::Span<char> buffer);

    void write(base::Span<const char> data);

    void flush();

    // Bad phrasing: Get position of logical file pointer in terms of actual bytes in the underlying file (base::File). The logical file pointer corresponds to the end of the last read or write operation, and this will not always agree with the actual file pointer because of buffering. Hmm                         
    pos_type tell();

    // Bad phrasing: Set position of logical file pointer in terms of actual bytes in the underlying file (base::File). Hmm                        
    void seek(pos_type);

    [[nodiscard]] bool try_read_some(base::Span<char> buffer, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_read(base::Span<char> buffer, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_write(base::Span<const char> data, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_flush(std::error_code&);

    [[nodiscard]] bool try_tell(pos_type&, std::error_code&);

    [[nodiscard]] bool try_seek(pos_type, std::error_code&);

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


template<class I> inline auto BasicPrimTextFile<I>::tell() -> pos_type
{
    pos_type pos = {};
    std::error_code ec;
    if (ARCHON_LIKELY(try_tell(pos, ec)))
        return pos; // Success
    throw std::system_error(ec, "Failed to determine read/write position");
}


template<class I> inline void BasicPrimTextFile<I>::seek(pos_type pos)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_seek(pos, ec)))
        return; // Success
    throw std::system_error(ec, "Failed to update read/write position");
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
            if (ARCHON_LIKELY(n_2 > 0 && n_2 < buffer_2.size())) {
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
inline bool BasicPrimTextFile<I>::try_tell(pos_type& pos, std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_reading))
        return m_impl.tell_write(pos, ec); // Throws
    return m_impl.tell_read(pos, ec); // Throws
}


template<class I>
inline bool BasicPrimTextFile<I>::try_seek(pos_type pos, std::error_code& ec)
{
    bool success = ((!m_writing || stop_writing(ec)) && m_impl.seek(pos, ec)); // Throws
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
    if (ARCHON_LIKELY(m_impl.read_ahead(buffer, m_dynamic_eof, n, ec))) { // Throws
        m_impl.advance();
        return true;
    }
    return false;
}


} // namespace archon::base





// ============================================================================================ text_file_impl.hpp ============================================================================================


namespace archon::base {


struct TextFileImplConfig;


template<class C, class T, class P = base::PrimTextFileImpl> class TextFileImpl {
public:
    using Config = TextFileImplConfig;

    using char_type      = C;
    using traits_type    = T;
    using prim_impl_type = P;
    using pos_type       = typename T::pos_type;

    static constexpr bool is_buffered = false;
    static constexpr bool has_windows_newline_codec = prim_impl_type::has_windows_newline_codec;

    TextFileImpl(base::File&, Config);

    [[nodiscard]] bool read_ahead(base::Span<C> buffer, bool dynamic_eof, std::size_t& n,
                                  std::error_code&);

    [[nodiscard]] bool write(base::Span<const C> data, std::size_t& n, std::error_code&);

    void advance() noexcept;
    void advance(std::size_t n);

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&);

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&);
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&);

    [[nodiscard]] bool seek(pos_type, std::error_code&);

private:
    prim_impl_type m_prim_impl;
    base::BasicCharCodec<C, T> m_codec;
    base::SeedMemoryBuffer<char> m_buffer;

    // `m_state` is always the state at the logical read/write position.
    // In reading mode, `m_state_2` is the state at the read ahead position.
    // In writing mode, the value of `m_state_2` in undefined.
    std::mbstate_t m_state = {};
    std::mbstate_t m_state_2 = {};

    // Beginning and end of the current contents of the buffer. In neutral mode,
    // both are zero. In reading mode, `m_begin` corresponds to the logical
    // read/write position, and `m_end` corresponds to the actual read/write
    // position. In writing mode, it is the other way around.
    std::size_t m_begin = 0;
    std::size_t m_end   = 0;

    // Position in buffer corresponding to the logical read/write position from
    // point of view of the primitive file implementation (`m_prim_impl`). In
    // neutral mode, and in writing mode, it is always zero.
    std::size_t m_offset = 0;

    // Position in buffer corresponding to the read ahead position. In neutral
    // mode, and in writing mode, it is always zero.
    std::size_t m_curr = 0;

    // In reading mode, this is the number of decoded characters between the
    // logical read/write position and the read ahead position. It is always
    // zero in neutral mode, and in writing mode.
    std::size_t m_retain_size  = 0;

#if ARCHON_DEBUG
    bool m_reading = false;
    bool m_writing = false;
#endif

    static base::SeedMemoryBuffer<char> make_buffer(Config&);
    static constexpr std::size_t max_buffer_size() noexcept;

    bool shallow_flush(std::error_code&);
    void expand_buffer();
};



template<class T, class P> class TextFileImpl<char, T, P> {
public:
    using Config = TextFileImplConfig;

    using char_type      = char;
    using traits_type    = T;
    using prim_impl_type = P;
    using pos_type       = typename T::pos_type;

    static constexpr bool is_buffered = false;
    static constexpr bool has_windows_newline_codec = prim_impl_type::has_windows_newline_codec;

    TextFileImpl(base::File&, Config);

    [[nodiscard]] bool read_ahead(base::Span<char> buffer, bool dynamic_eof, std::size_t& n,
                                  std::error_code&);

    [[nodiscard]] bool write(base::Span<const char> data, std::size_t& n, std::error_code&);

    void advance() noexcept;
    void advance(std::size_t n);

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&);

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&);
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&);

    [[nodiscard]] bool seek(pos_type, std::error_code&);

private:
    prim_impl_type m_prim_impl;
};



template<class C, class T = std::char_traits<C>>
using PosixTextFileImpl = TextFileImpl<C, T, base::PrimPosixTextFileImpl>;

template<class C, class T = std::char_traits<C>>
using WindowsTextFileImpl = TextFileImpl<C, T, base::PrimWindowsTextFileImpl>;



struct TextFileImplConfig :
        base::PrimTextFileImplConfig {
    static constexpr std::size_t default_char_codec_buffer_size = 1024;

    std::locale locale;

    // GENERIC:
    //
    // Buffer will be automatically expanded if necessary.              
    //
    std::size_t char_codec_buffer_size = default_char_codec_buffer_size;
    base::Span<char> char_codec_buffer_memory;
};



/// \brief Errors that can be generated through the use of text files.
///
/// These are errors that can be generated through use of \ref TextFileImpl.
///
enum class TextFileError {
    /// \brief Invalid byte sequence while trying to decode character.
    ///
    /// A character could not be decoded, because the presented byte sequence
    /// was not a valid encoding of any character.
    ///
    invalid_byte_seq = 1,

    /// \brief Invalid character value while trying to encode character.
    ///
    /// A character could not be encoded, because its value was outside the
    /// range of valid character values.
    ///
    invalid_char = 2,
};



/// \brief Text file error category.
///
/// This is the error category associated with \ref TextFileError. The name of
/// this category is `archon:base:text_file`.
///
class TextFileErrorCategory :
        public std::error_category {
public:
    const char* name() const noexcept override final;
    std::string message(int) const override final;
};


/// \brief Text file error category singleton.
///
/// This is the error category object that must be used when constructing error
/// code objects associated with the text file error category.
///
extern TextFileErrorCategory text_file_error_category;


/// \brief Make text file error code object.
///
/// This function makes a text file error code object. Together with the
/// specialization of `std::is_error_code_enum<T>` for \ref TextFileError, this
/// allows for implicit conversion from an enum value to an error code object.
///
inline std::error_code make_error_code(TextFileError err)
{
    return std::error_code(int(err), text_file_error_category);
}


} // namespace archon::base

namespace std {


template<> class is_error_code_enum<archon::base::TextFileError> {
public:
    static constexpr bool value = true;
};


} // namespace std

namespace archon::base {








// Implementation


// ============================ TextFileImpl<C, T, P> ============================


template<class C, class T, class P>
inline TextFileImpl<C, T, P>::TextFileImpl(base::File& file, Config config) :
    m_prim_impl(file, std::move(config)), // Throws
    m_codec(std::move(config.locale)), // Throws    
    m_buffer(make_buffer(config)) // Throws
{
}


template<class C, class T, class P>
bool TextFileImpl<C, T, P>::read_ahead(base::Span<C> buffer, bool dynamic_eof, std::size_t& n,
                                       std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    bool end_of_file = false;
    for (;;) {
        base::Span data = base::Span<char>(m_buffer).first(m_end);
        std::size_t buffer_offset = 0;
        bool error = false;
        if (end_of_file)
            error = true;                   
        bool complete =
            m_codec.inc_decode(m_state_2, data, m_curr, buffer, buffer_offset, error);
        m_retain_size += buffer_offset;
        if (ARCHON_LIKELY(buffer_offset > 0 || buffer.size() == 0)) {
            n = buffer_offset;
            return true;
        }
        if (!error) {
            ARCHON_ASSERT(!end_of_file);
            // Move any retained data to start of buffer
            ARCHON_ASSERT(m_offset <= m_begin);
            m_prim_impl.advance(std::size_t(m_begin - m_offset)); // Throws
            char* base = m_buffer.data();
            std::copy(base + m_begin, base + m_end, base);
            m_end  -= m_begin;
            m_curr -= m_begin;
            m_begin  = 0;
            m_offset = 0;
            if (ARCHON_UNLIKELY(m_end == m_buffer.size()))
                expand_buffer(); // Throws
            std::size_t n_2 = 0;
            base::Span buffer_2 = base::Span<char>(m_buffer).subspan(m_end);
            if (ARCHON_LIKELY(m_prim_impl.read_ahead(buffer_2, dynamic_eof, n_2, ec))) { // Throws
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
        // FIXME: Only when not in lenient mode            
        ec = TextFileError::invalid_byte_seq;
        return false;
    }
}


template<class C, class T, class P>
bool TextFileImpl<C, T, P>::write(base::Span<const C> data, std::size_t& n, std::error_code& ec)
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
                // FIXME: Only when not in lenient mode            
                n = data_offset;
                ec = TextFileError::invalid_char;
                return false;
            case std::codecvt_base::noconv:
                break;
        }
        ARCHON_ASSERT_UNREACHABLE;
        n = data.size();
        return true;
    }
}


template<class C, class T, class P> inline void TextFileImpl<C, T, P>::advance() noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_curr);
    m_state = m_state_2;
    m_begin = m_curr;
    m_retain_size = 0;
}


template<class C, class T, class P> inline void TextFileImpl<C, T, P>::advance(std::size_t n)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(n <= m_retain_size);
    ARCHON_ASSERT(m_begin <= m_curr);
    if (ARCHON_LIKELY(n == m_retain_size)) {
        m_state = m_state_2;
        m_begin = m_curr;
        m_retain_size = 0;
        return;
    }
    base::Span data = base::Span<char>(m_buffer).first(m_curr);
    // The difference between `m_begin` and `data.size()` cannot be greater
    // than the size of `m_buffer`, and the buffer is not allowed to grow larger
    // than `m_codec.max_simulate_decode_size()`
    m_codec.simulate_inc_decode(m_state, data, m_begin, n); // Throws
    ARCHON_ASSERT(m_begin <= m_curr);
    m_retain_size -= n;
}


template<class C, class T, class P> bool TextFileImpl<C, T, P>::discard(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_offset <= m_begin);
    std::size_t n = std::size_t(m_begin - m_offset);
    m_prim_impl.advance(n);
    m_offset = m_begin;
    if (ARCHON_LIKELY(m_prim_impl.discard(ec))) { // Throws
        m_state_2     = m_state;
        m_begin       = 0;
        m_end         = 0;
        m_offset      = 0;
        m_curr        = 0;
        m_retain_size = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class C, class T, class P> inline bool TextFileImpl<C, T, P>::flush(std::error_code& ec)
{
    ARCHON_ASSERT(m_offset       == 0);
    ARCHON_ASSERT(m_curr         == 0);
    ARCHON_ASSERT(m_retain_size  == 0);
    if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
        if (ARCHON_LIKELY(m_prim_impl.flush(ec))) { // Throws
            m_state_2 = m_state; // Part of transitioning to neutral mode
#if ARCHON_DEBUG
            m_writing = false;
#endif
            return true;
        }
        return false;
    }

    // Even when everything in the local buffer could not be written, an attempt
    // to recursively flush the part, that could be writen, must still be
    // made. If the recursive flush also fails, it doesn't really matter which
    // error code is passed back. The choice here is to let the lowest level
    // error win.
    static_cast<void>(m_prim_impl.flush(ec));
    return false;
}


template<class C, class T, class P>
bool TextFileImpl<C, T, P>::tell_read(pos_type& pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_offset <= m_begin);
    m_prim_impl.advance(std::size_t(m_begin - m_offset)); // Throw
    m_offset = m_begin;
    typename prim_impl_type::pos_type pos_2 = {};
    if (ARCHON_LIKELY(m_prim_impl.tell_read(pos_2, ec))) { // Throws
        pos = pos_type(pos_2);
        pos.state(m_state);
        return true;
    }
    return false;
}


template<class C, class T, class P>
bool TextFileImpl<C, T, P>::tell_write(pos_type& pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    typename prim_impl_type::pos_type pos_2 = {};
    // Take care to not invoke write() on `m_prim_impl` unless there is actually
    // somethig to write. This is necesary to avoid ending up in a situation
    // where `m_prim_impl` is in writing mode, but this file implementation
    // object is in neutral mode.
    bool success = ((m_begin == m_end || shallow_flush(ec)) &&
                    m_prim_impl.tell_write(pos_2, ec)); // Throws
    if (ARCHON_LIKELY(success)) {
        pos = pos_type(pos_2);
        pos.state(m_state);
        return true;
    }
    return false;
}


template<class C, class T, class P>
inline bool TextFileImpl<C, T, P>::seek(pos_type pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    auto pos_2 = typename prim_impl_type::pos_type(pos);
    if (ARCHON_LIKELY(m_prim_impl.seek(pos_2, ec))) { // Throws
        m_state       = pos.state();
        m_state_2     = m_state;
        m_begin       = 0;
        m_end         = 0;
        m_offset      = 0;
        m_curr        = 0;
        m_retain_size = 0;
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

    ARCHON_ASSERT(m_begin <= m_end);
    base::Span data = base::Span<char>(m_buffer).first(m_end).subspan(m_begin);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_prim_impl.write(data, n, ec))) { // Throws
        m_begin = 0;
        m_end   = 0;
        return true;
    }
    m_begin += n;
    return false;
}


template<class C, class T, class P> inline void TextFileImpl<C, T, P>::expand_buffer()
{
    m_buffer.expand(1, m_end, max_buffer_size()); // Throws
}



// ============================ TextFileImpl<char, T, P> ============================


template<class T, class P>
inline TextFileImpl<char, T, P>::TextFileImpl(base::File& file, Config config) :
    m_prim_impl(file, std::move(config)) // Throws
{
}


template<class T, class P>
inline bool TextFileImpl<char, T, P>::read_ahead(base::Span<char> buffer, bool dynamic_eof,
                                                 std::size_t& n, std::error_code& ec)
{
    return m_prim_impl.read_ahead(buffer, dynamic_eof, n, ec); // Throws
}


template<class T, class P>
inline bool TextFileImpl<char, T, P>::write(base::Span<const char> data, std::size_t& n,
                                            std::error_code& ec)
{
    return m_prim_impl.write(data, n, ec); // Throws
}


template<class T, class P> inline void TextFileImpl<char, T, P>::advance() noexcept
{
    m_prim_impl.advance();
}


template<class T, class P> inline void TextFileImpl<char, T, P>::advance(std::size_t n)
{
    m_prim_impl.advance(n); // Throws
}


template<class T, class P> inline bool TextFileImpl<char, T, P>::discard(std::error_code& ec)
{
    return m_prim_impl.discard(ec); // Throws
}


template<class T, class P> inline bool TextFileImpl<char, T, P>::flush(std::error_code& ec)
{
    return m_prim_impl.flush(ec); // Throws
}


template<class T, class P>
inline bool TextFileImpl<char, T, P>::tell_read(pos_type& pos, std::error_code& ec)
{
    typename prim_impl_type::pos_type pos_2 = {};
    if (ARCHON_LIKELY(m_prim_impl.tell_read(pos_2, ec))) { // Throws
        pos = pos_type(pos_2);
        return true;
    }
    return false;
}


template<class T, class P>
inline bool TextFileImpl<char, T, P>::tell_write(pos_type& pos, std::error_code& ec)
{
    typename prim_impl_type::pos_type pos_2 = {};
    if (ARCHON_LIKELY(m_prim_impl.tell_write(pos_2, ec))) { // Throws
        pos = pos_type(pos_2);
        return true;
    }
    return false;
}


template<class T, class P>
inline bool TextFileImpl<char, T, P>::seek(pos_type pos, std::error_code& ec)
{
    auto pos_2 = typename prim_impl_type::pos_type(pos);
    return m_prim_impl.seek(pos_2, ec); // Throws
}


} // namespace archon::base





// ============================================================================================ text_file_impl.cpp ============================================================================================


using namespace archon;

namespace {


const char* get_error_message(base::TextFileError err) noexcept
{
    switch (err) {
        case base::TextFileError::invalid_byte_seq:
            return "Invalid byte sequence while trying to decode character";
        case base::TextFileError::invalid_char:
            return "Invalid character value while trying to encode character";
    }
    ARCHON_ASSERT_UNREACHABLE;
    return nullptr;
}


} // unnamed namespace



base::TextFileErrorCategory base::text_file_error_category;


const char* base::TextFileErrorCategory::name() const noexcept
{
    return "archon:base:text_file";
}


std::string base::TextFileErrorCategory::message(int err) const
{
    return std::string(get_error_message(TextFileError(err))); // Throws
}





// ============================================================================================ buffered_text_file_impl.hpp ============================================================================================


namespace archon::base {


template<class C, class T, class I> class BufferedTextFileImpl {
public:
    struct Config;

    using char_type    = C;
    using traits_type  = T;
    using subimpl_type = I;
    using pos_type     = typename I::pos_type;

    static constexpr bool is_buffered = true;
    static constexpr bool has_windows_newline_codec = subimpl_type::has_windows_newline_codec;

    BufferedTextFileImpl(base::File&, Config);

    [[nodiscard]] bool read_ahead(base::Span<C> buffer, bool dynamic_eof, std::size_t& n,
                                  std::error_code&);

    [[nodiscard]] bool write(base::Span<const C> data, std::size_t& n, std::error_code&);

    void advance() noexcept;
    void advance(std::size_t n);

    [[nodiscard]] bool discard(std::error_code&);

    [[nodiscard]] bool flush(std::error_code&);

    [[nodiscard]] bool tell_read(pos_type&, std::error_code&);
    [[nodiscard]] bool tell_write(pos_type&, std::error_code&);

    [[nodiscard]] bool seek(pos_type, std::error_code&);

private:
    subimpl_type m_subimpl;
    base::SeedMemoryBuffer<C> m_buffer;

    // Beginning and end of the current contents of the buffer. In neutral mode,
    // both are zero. In reading mode, `m_begin` corresponds to the logical
    // read/write position, and `m_end` corresponds to the subimplementation's
    // read ahead position (`m_subimpl`). In writing mode, `m_end` corresponds
    // to the logical read/write position, and `m_begin` corresponds to the
    // subimplementation's logical read/write position.    FIXME: Adopt this explanation CRIT FIX at lower levels (regular and primitive)                                                          
    std::size_t m_begin = 0;
    std::size_t m_end   = 0;

    // In neutral mode, and in reading mode, this is the position in the buffer
    // that corresponds to the subimplementation's logical read/write position
    // (`m_subimpl`). In writing mode, it has no meaning. It is always zero in
    // neutral mode, and in writing mode.    FIXME: Adopt this explanation fix at lower levels (regular and primitive)                                                          
    std::size_t m_offset = 0;

    // In neutral mode, and in reading mode, this is the position in the buffer
    // that corresponds to the read ahead position. In writing mode, it has no
    // meaning. It is always zero in neutral mode, and in writing mode.    FIXME: Adopt this explanation fix at lower levels (regular and primitive)                                                          
    std::size_t m_curr = 0;

#if ARCHON_DEBUG
    bool m_reading = false;
    bool m_writing = false;
#endif

    bool shallow_flush(std::error_code&);
    void expand_buffer();
};


template<class C, class T, class I> struct BufferedTextFileImpl<C, T, I>::Config :
        base::TextFileImplConfig {
    static constexpr std::size_t default_buffer_size = 1024;

    // GENERIC:
    //
    // Buffer will be automatically expanded if necessary.              
    //
    std::size_t buffer_size = default_buffer_size;
    base::Span<C> buffer_memory;
};








// Implementation


template<class C, class T, class I>
inline BufferedTextFileImpl<C, T, I>::BufferedTextFileImpl(base::File& file, Config config) :
    m_subimpl(file, std::move(config)), // Throws
    m_buffer(config.buffer_memory, config.buffer_size) // Throws
{
    // Buffer must not be empty
    m_buffer.reserve(1); // Throws
}


template<class C, class T, class I>
bool BufferedTextFileImpl<C, T, I>::read_ahead(base::Span<C> buffer, bool dynamic_eof,
                                               std::size_t& n, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
    m_reading = true;
#endif

    if (ARCHON_LIKELY(buffer.size() > 0)) {
        if (ARCHON_LIKELY(m_curr < m_end)) {
          copy:
            std::size_t n_2 = std::min(buffer.size(), std::size_t(m_end - m_curr));
            std::copy_n(m_buffer.data() + m_curr, n_2, buffer.data());
            m_curr += n_2;
            n = n_2;
            ARCHON_ASSERT(n > 0);
            return true;
        }
        {
            // Move any retained data to start of buffer
            ARCHON_ASSERT(m_offset <= m_begin);
            m_subimpl.advance(std::size_t(m_begin - m_offset)); // Throws
            C* base = m_buffer.data();
            std::copy(base + m_begin, base + m_end, base);
            m_end  -= m_begin;
            m_curr -= m_begin;
            m_begin  = 0;
            m_offset = 0;
            if (ARCHON_UNLIKELY(m_end == m_buffer.size()))
                expand_buffer(); // Throws
            std::size_t n_2 = 0;
            base::Span buffer_2 = base::Span<C>(m_buffer).subspan(m_end);
            if (ARCHON_LIKELY(m_subimpl.read_ahead(buffer_2, dynamic_eof, n_2, ec))) { // Throws
                if (ARCHON_LIKELY(n_2 > 0)) {
                    m_end += n_2;
                    goto copy;
                }
                // Signal end of file
            }
            else {
                return false;
            }
        }
    }
    n = 0;
    return true;
}


template<class C, class T, class I>
bool BufferedTextFileImpl<C, T, I>::write(base::Span<const C> data, std::size_t& n,
                                          std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    m_writing = true;
#endif

    ARCHON_ASSERT(m_buffer.size() > 0);
    base::Span data_2 = data;
    for (;;) {
        std::size_t capacity = std::size_t(m_buffer.size() - m_end);
        std::size_t n_2 = std::min(data_2.size(), capacity);
        std::copy_n(data_2.data(), n_2, m_buffer.data() + m_end);
        m_end += n_2;
        if (ARCHON_LIKELY(data_2.size() <= capacity)) {
            n = data.size();
            return true;
        }
        data_2 = data_2.subspan(n_2);
        if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
            ARCHON_ASSERT(m_end == 0);
            continue;
        }
        n = std::size_t(data.size() - data_2.size());
        return false;
    }
}


template<class C, class T, class I> inline void BufferedTextFileImpl<C, T, I>::advance() noexcept
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_curr);
    m_begin = m_curr;
}


template<class C, class T, class I>
inline void BufferedTextFileImpl<C, T, I>::advance(std::size_t n)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_begin <= m_curr);
    ARCHON_ASSERT(n <= std::size_t(m_curr - m_begin));
    m_begin += n;
}


template<class C, class T, class I>
bool BufferedTextFileImpl<C, T, I>::discard(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_offset <= m_begin);
    std::size_t n = std::size_t(m_begin - m_offset);
    m_subimpl.advance(n);
    m_offset = m_begin;
    if (ARCHON_LIKELY(m_subimpl.discard(ec))) { // Throws
        m_begin   = 0;
        m_end     = 0;
        m_offset  = 0;
        m_curr    = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class C, class T, class I>
inline bool BufferedTextFileImpl<C, T, I>::flush(std::error_code& ec)
{
    ARCHON_ASSERT(m_offset == 0);
    ARCHON_ASSERT(m_curr   == 0);

    if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
        if (ARCHON_LIKELY(m_subimpl.flush(ec))) { // Throws
#if ARCHON_DEBUG
            m_writing = false;
#endif
            return true;
        }
        return false;
    }

    // Even when everything in the local buffer could not be written, an attempt
    // to recursively flush the part, that could be writen, must still be
    // made. If the recursive flush also fails, it doesn't really matter which
    // error code is passed back. The choice here is to let the lowest level
    // error win.
    static_cast<void>(m_subimpl.flush(ec));
    return false;
}


template<class C, class T, class I>
bool BufferedTextFileImpl<C, T, I>::tell_read(pos_type& pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    ARCHON_ASSERT(m_offset <= m_begin);
    m_subimpl.advance(std::size_t(m_begin - m_offset)); // Throw
    m_offset = m_begin;
    return m_subimpl.tell_read(pos, ec); // Throws
}


template<class C, class T, class I>
bool BufferedTextFileImpl<C, T, I>::tell_write(pos_type& pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    // Take care to not invoke write() on `m_subimpl` unless there is actually
    // somethig to write. This is necesary to avoid ending up in a situation
    // where `m_subimpl` is in writing mode, but this file implementation object
    // is in neutral mode.
    return ((m_begin == m_end || shallow_flush(ec)) && m_subimpl.tell_write(pos, ec)); // Throws
}


template<class C, class T, class I>
inline bool BufferedTextFileImpl<C, T, I>::seek(pos_type pos, std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_writing);
#endif

    if (ARCHON_LIKELY(m_subimpl.seek(pos, ec))) { // Throws
        m_begin   = 0;
        m_end     = 0;
        m_offset  = 0;
        m_curr    = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true;
    }
    return false;
}


template<class C, class T, class I> bool BufferedTextFileImpl<C, T, I>::shallow_flush(std::error_code& ec)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
#endif

    ARCHON_ASSERT(m_begin <= m_end);
    base::Span data = base::Span<C>(m_buffer).first(m_end).subspan(m_begin);
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_subimpl.write(data, n, ec))) { // Throws
        m_begin = 0;
        m_end   = 0;
        return true;
    }
    m_begin += n;
    return false;
}


template<class C, class T, class I> inline void BufferedTextFileImpl<C, T, I>::expand_buffer()
{
    m_buffer.expand(1, m_end); // Throws
}


} // namespace archon::base





// ============================================================================================ text_file.hpp ============================================================================================


namespace archon::base {




template<class C, class T = std::char_traits<C>, class I = base::TextFileImpl<C, T>>
class BasicTextFile {
public:
    struct Config;

    using char_type   = C;
    using traits_type = T;
    using impl_type   = I;
    using pos_type    = typename T::pos_type;
    using Mode        = base::File::Mode;

    BasicTextFile(base::FilesystemPathRef, Mode = Mode::read);
    BasicTextFile(base::FilesystemPathRef, Mode, Config);

    std::size_t read_some(base::Span<C> buffer);

    std::size_t read(base::Span<C> buffer);

    void write(base::Span<const C> data);

    void flush();

    pos_type tell();

    void seek(pos_type);

    [[nodiscard]] bool try_read_some(base::Span<C> buffer, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_read(base::Span<C> buffer, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_write(base::Span<const C> data, std::size_t& n, std::error_code&);

    [[nodiscard]] bool try_flush(std::error_code&);

    [[nodiscard]] bool try_tell(pos_type&, std::error_code&);

    // FIXME: State somewhere that "state" part of pos arg is ignored when char type is `char`                                                                
    [[nodiscard]] bool try_seek(pos_type, std::error_code&);

private:
    base::File m_file;
    impl_type m_impl;
    bool m_dynamic_eof;
    bool m_reading = false;
    bool m_writing = false;

    bool stop_reading(std::error_code&);
    bool stop_writing(std::error_code&);
    bool do_read_some(base::Span<C> buffer, std::size_t& n, std::error_code&);
};


template<class C, class T = std::char_traits<C>>
using BasicPosixTextFile = BasicTextFile<C, T, base::PosixTextFileImpl<C, T>>;

template<class C, class T = std::char_traits<C>>
using BasicWindowsTextFile = BasicTextFile<C, T, base::WindowsTextFileImpl<C, T>>;

template<class C, class T = std::char_traits<C>, class I = base::TextFileImpl<C, T>>
using BasicBufferedTextFile = BasicTextFile<C, T, base::BufferedTextFileImpl<C, T, I>>;

template<class C, class T = std::char_traits<C>>
using BasicBufferedPosixTextFile = BasicBufferedTextFile<C, T, base::PosixTextFileImpl<C, T>>;

template<class C, class T = std::char_traits<C>>
using BasicBufferedWindowsTextFile = BasicBufferedTextFile<C, T, base::WindowsTextFileImpl<C, T>>;


using TextFile        = BasicTextFile<char>;
using PosixTextFile   = BasicPosixTextFile<char>;
using WindowsTextFile = BasicWindowsTextFile<char>;

using WideTextFile        = BasicTextFile<wchar_t>;
using WidePosixTextFile   = BasicPosixTextFile<wchar_t>;
using WideWindowsTextFile = BasicWindowsTextFile<wchar_t>;

using BufferedTextFile        = BasicBufferedTextFile<char>;
using BufferedPosixTextFile   = BasicBufferedPosixTextFile<char>;
using BufferedWindowsTextFile = BasicBufferedWindowsTextFile<char>;

using WideBufferedTextFile        = BasicBufferedTextFile<wchar_t>;
using WideBufferedPosixTextFile   = BasicBufferedPosixTextFile<wchar_t>;
using WideBufferedWindowsTextFile = BasicBufferedWindowsTextFile<wchar_t>;


template<class C, class T, class I> struct BasicTextFile<C, T, I>::Config :
        public I::Config {
    bool dynamic_eof = false;
};








// Implementation


template<class C, class T, class I>
inline BasicTextFile<C, T, I>::BasicTextFile(base::FilesystemPathRef path, Mode mode) :
    BasicTextFile(path, mode, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicTextFile<C, T, I>::BasicTextFile(base::FilesystemPathRef path, Mode mode,
                                             Config config) :
    m_file(path, mode), // Throws
    m_impl(m_file, std::move(config)), // Throws
    m_dynamic_eof(config.dynamic_eof)
{
}


template<class C, class T, class I>
inline std::size_t BasicTextFile<C, T, I>::read_some(base::Span<C> buffer)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_some(buffer, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


template<class C, class T, class I>
inline std::size_t BasicTextFile<C, T, I>::read(base::Span<C> buffer)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read(buffer, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


template<class C, class T, class I>
inline void BasicTextFile<C, T, I>::write(base::Span<const C> data)
{
    std::size_t n; // Dummy
    std::error_code ec;
    if (ARCHON_LIKELY(try_write(data, n, ec)))
        return; // Success
    throw std::system_error(ec, "Failed to write to file");
}


template<class C, class T, class I> inline void BasicTextFile<C, T, I>::flush()
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_flush(ec)))
        return; // Success
    throw std::system_error(ec, "Failed to flush");
}


template<class C, class T, class I> inline auto BasicTextFile<C, T, I>::tell() -> pos_type
{
    pos_type pos;
    std::error_code ec;
    if (ARCHON_LIKELY(try_tell(pos, ec)))
        return pos; // Success
    throw std::system_error(ec, "Failed to determine read/write position");
}


template<class C, class T, class I> inline void BasicTextFile<C, T, I>::seek(pos_type pos)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_seek(pos, ec)))
        return; // Success
    throw std::system_error(ec, "Failed to update read/write position");
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_read_some(base::Span<C> buffer, std::size_t& n,
                                                  std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_writing || stop_writing(ec))) { // Throws
        m_reading = true;
        return do_read_some(buffer, n, ec); // Throws
    }
    return false;
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_read(base::Span<C> buffer, std::size_t& n,
                                             std::error_code& ec)
{
    base::Span<C> buffer_2 = buffer;
    if (ARCHON_LIKELY(!m_writing || stop_writing(ec))) { // Throws
        m_reading = true;
        std::size_t n_2 = 0;
      again:
        if (ARCHON_LIKELY(do_read_some(buffer_2, n_2, ec))) {
            ARCHON_ASSERT(n_2 <= buffer_2.size());
            if (ARCHON_LIKELY(n_2 > 0 && n_2 < buffer_2.size())) {
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


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_write(base::Span<const C> data, std::size_t& n,
                                              std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_reading || stop_reading(ec))) { // Throws
        m_writing = true;
        return m_impl.write(data, n, ec); // Throws
    }
    n = 0;
    return false;
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_flush(std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_writing || m_impl.flush(ec))) { // Throws
        m_writing = false;
        return true;
    }
    return false;
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_tell(pos_type& pos, std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_reading))
        return m_impl.tell_write(pos, ec); // Throws
    return m_impl.tell_read(pos, ec); // Throws
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_seek(pos_type pos, std::error_code& ec)
{
    bool success = ((!m_writing || stop_writing(ec)) && m_impl.seek(pos, ec)); // Throws
    if (ARCHON_LIKELY(success)) {
        m_reading = false;
        return true;
    }
    return false;
}


template<class C, class T, class I> bool BasicTextFile<C, T, I>::stop_reading(std::error_code& ec)
{
    ARCHON_ASSERT(m_reading);
    ARCHON_ASSERT(!m_writing);
    if (ARCHON_LIKELY(m_impl.discard(ec))) { // Throws
        m_reading = false;
        return true;
    }
    return false;
}


template<class C, class T, class I> bool BasicTextFile<C, T, I>::stop_writing(std::error_code& ec)
{
    ARCHON_ASSERT(!m_reading);
    ARCHON_ASSERT(m_writing);
    if (ARCHON_LIKELY(m_impl.flush(ec))) { // Throws
        m_writing = false;
        return true;
    }
    return false;
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::do_read_some(base::Span<C> buffer, std::size_t& n,
                                                 std::error_code& ec)
{
    ARCHON_ASSERT(!m_writing);
    if (ARCHON_LIKELY(m_impl.read_ahead(buffer, m_dynamic_eof, n, ec))) { // Throws
        m_impl.advance();
        return true;
    }
    return false;
}


} // namespace archon::base
















using namespace archon;





// ============================================================================================ test_prim_text_file_impl.cpp ============================================================================================


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
    success = text_file_impl.read_ahead(buffer, dynamic_eof, n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);
    if (success)
        log("contents = %s", base::quoted(std::string_view(buffer.data(), n)));

    log("------ advance_and_discard ------");
    text_file_impl.advance(5);
    success = text_file_impl.discard(ec);
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
    success = text_file_impl.read_ahead(buffer, dynamic_eof, n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);
    if (success)
        log("contents = %s", base::quoted(std::string_view(buffer.data(), n)));

    // FIXME: Also check dynamic_eof   
}





// ============================================================================================ test_prim_text_file.cpp ============================================================================================


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


ARCHON_TEST(Base_PrimTextFile_PosixTellAndSeek)
{
    ARCHON_TEST_FILE(path);
    base::PrimPosixTextFile text_file(path, base::File::Mode::write);
    ARCHON_CHECK_EQUAL(text_file.tell(), 0);
    text_file.write(std::string_view("foo\nbar"));
    auto pos = text_file.tell();
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    ARCHON_CHECK_EQUAL(pos, 7);
    text_file.write(std::string_view("\nbaz\n"));
    ARCHON_CHECK_EQUAL(text_file.tell(), 12);
    text_file.seek(pos);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    text_file.seek(0);
    ARCHON_CHECK_EQUAL(text_file.tell(), 0);
    text_file.seek(pos);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    text_file.seek(0);
    text_file.seek(pos);
    std::array<char, 64> buffer;
    std::size_t n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "\nbaz\n");
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


ARCHON_TEST(Base_PrimTextFile_WindowsTellAndSeek)
{
    ARCHON_TEST_FILE(path);
    base::PrimWindowsTextFile text_file(path, base::File::Mode::write);
    ARCHON_CHECK_EQUAL(text_file.tell(), 0);
    text_file.write(std::string_view("foo\nbar"));
    auto pos = text_file.tell();
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    ARCHON_CHECK_EQUAL(pos, 8);
    text_file.write(std::string_view("\nbaz\n"));
    ARCHON_CHECK_EQUAL(text_file.tell(), 15);
    text_file.seek(pos);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    text_file.seek(0);
    ARCHON_CHECK_EQUAL(text_file.tell(), 0);
    text_file.seek(pos);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    text_file.seek(0);
    text_file.seek(pos);
    std::array<char, 64> buffer;
    std::size_t n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), n), "\nbaz\n");
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


// Impossible: Do this genuinely at the impl level by introducing method for seeing the current size of the buffer                                                                                                                                                    
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





// ============================================================================================ test_text_file_impl.cpp ============================================================================================


ARCHON_TEST(Base_TextFileImpl_Windows)
{
    // FIXME: Make similar test for POSIX variant  

    // FIXME: Do this for both narrow and wide character types  

    ARCHON_TEST_FILE(path);
    base::File file(path, base::File::Mode::write);
    base::TextFileImplConfig config;
    config.locale = test_context.get_locale();
    config.char_codec_buffer_size = 16;
    config.newline_codec_buffer_size = 16;
    base::WindowsTextFileImpl<wchar_t> text_file_impl(file, std::move(config));
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
    success = text_file_impl.read_ahead(buffer, dynamic_eof, n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);
    if (success)
        log("contents = %s", base::format_enc<wchar_t>(test_context.get_locale(), "%s", base::quoted(std::wstring_view(buffer.data(), n))));

    log("------ advance_and_discard ------");
    text_file_impl.advance(5);
    success = text_file_impl.discard(ec);
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
    success = text_file_impl.read_ahead(buffer, dynamic_eof, n, ec);
    log(" success = %s", success);
    log("       n = %s", n);
    log("      ec = %s", ec);
    if (success)
        log("contents = %s", base::format_enc<wchar_t>(test_context.get_locale(), "%s", base::quoted(std::wstring_view(buffer.data(), n))));

    // FIXME: Also check dynamic_eof   
}





// ============================================================================================ test_text_file.cpp ============================================================================================


ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(base::PosixTextFile,               Posix),
                     ARCHON_TEST_TYPE(base::WindowsTextFile,             Windows),
                     ARCHON_TEST_TYPE(base::WidePosixTextFile,           WidePosix),
                     ARCHON_TEST_TYPE(base::WideWindowsTextFile,         WideWindows),
                     ARCHON_TEST_TYPE(base::BufferedPosixTextFile,       BufferedPosix),
                     ARCHON_TEST_TYPE(base::BufferedWindowsTextFile,     BufferedWindows),
                     ARCHON_TEST_TYPE(base::WideBufferedPosixTextFile,   WideBufferedPosix),
                     ARCHON_TEST_TYPE(base::WideBufferedWindowsTextFile, WideBufferedWindows));


ARCHON_TEST_BATCH(Base_TextFile_Read, variants)
{
    ARCHON_TEST_FILE(path);
    {
        base::File file(path, base::File::Mode::write);
        file.write(std::string_view("foo\r\nbar\r\nbaz\r\n"));
    }
    const std::locale& locale = test_context.get_locale();
    using text_file_type = test_type;
    using char_type = typename text_file_type::char_type;
    using string_view_type = std::basic_string_view<char_type>;
    typename text_file_type::Config config;
    config.locale = locale;
    config.char_codec_buffer_size = 3;
    config.newline_codec_buffer_size = 3;
    if constexpr (text_file_type::impl_type::is_buffered)
        config.buffer_size = 3;
    text_file_type text_file(path, base::File::Mode::read, std::move(config));
    std::array<char_type, 64> buffer;
    std::size_t n = text_file.read(buffer);
    string_view_type data(buffer.data(), n);
    std::array<char_type, 64> seed_memory;
    base::BasicStringWidener<char_type> widener(locale, seed_memory);
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(data, widener.widen("foo\nbar\nbaz\n"));
    }
    else {
        ARCHON_CHECK_EQUAL(data, widener.widen("foo\r\nbar\r\nbaz\r\n"));
    }
}


ARCHON_TEST_BATCH(Base_TextFile_WriteAndFlush, variants)
{
    using text_file_type = test_type;
    ARCHON_TEST_FILE(path);
    {
        const std::locale& locale = test_context.get_locale();
        using char_type = typename text_file_type::char_type;
        typename text_file_type::Config config;
        config.locale = locale;
        config.char_codec_buffer_size = 3;
        config.newline_codec_buffer_size = 3;
        if constexpr (text_file_type::impl_type::is_buffered)
            config.buffer_size = 3;
        text_file_type text_file(path, base::File::Mode::write, std::move(config));
        std::array<char_type, 64> seed_memory;
        base::BasicStringWidener<char_type> widener(locale, seed_memory);
        text_file.write(widener.widen("foo\nbar\nbaz\n"));
        text_file.flush();
    }
    base::File file(path);
    std::array<char, 64> buffer;
    std::size_t n = file.read(buffer);
    std::string_view data(buffer.data(), n);
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(data, "foo\r\nbar\r\nbaz\r\n");
    }
    else {
        ARCHON_CHECK_EQUAL(data, "foo\nbar\nbaz\n");
    }
}


ARCHON_TEST_BATCH(Base_TextFile_TellAndSeek, variants)
{
    ARCHON_TEST_FILE(path);
    const std::locale& locale = test_context.get_locale();
    using text_file_type = test_type;
    using char_type = typename text_file_type::char_type;
    using string_view_type = std::basic_string_view<char_type>;
    typename text_file_type::Config config;
    config.locale = locale;
    config.char_codec_buffer_size = 3;
    config.newline_codec_buffer_size = 3;
    if constexpr (text_file_type::impl_type::is_buffered)
        config.buffer_size = 3;
    text_file_type text_file(path, base::File::Mode::write, std::move(config));
    std::array<char_type, 64> seed_memory;
    base::BasicStringWidener<char_type> widener(locale, seed_memory);
    using pos_type = typename text_file_type::pos_type;
    ARCHON_CHECK_EQUAL(text_file.tell(), pos_type(0));
    text_file.write(string_view_type(widener.widen("foo\nbar")));
    pos_type pos = text_file.tell();
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(pos, pos_type(8));
    }
    else {
        ARCHON_CHECK_EQUAL(pos, pos_type(7));
    }
    text_file.write(widener.widen("\nbaz\n"));
    if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
        ARCHON_CHECK_EQUAL(text_file.tell(), pos_type(15));
    }
    else {
        ARCHON_CHECK_EQUAL(text_file.tell(), pos_type(12));
    }
    text_file.seek(pos);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    text_file.seek(0);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos_type(0));
    text_file.seek(pos);
    ARCHON_CHECK_EQUAL(text_file.tell(), pos);
    text_file.seek(0);
    text_file.seek(pos);
    std::array<char_type, 64> buffer;
    std::size_t n = text_file.read(buffer);
    ARCHON_CHECK_EQUAL(string_view_type(buffer.data(), n), widener.widen("\nbaz\n"));
}


// It is possible to have a locale where there is no such thing as an invalid
// byte sequence (e.g., ISO-8859-1). Requiring ASCII is one way of making sure
// that at least one invalid byte sequence exists (any byte with a value outside
// the range 0 -> 127).
//
ARCHON_TEST_BATCH_IF(Base_TextFile_DecodeError, variants, ARCHON_C_LOCALE_IS_ASCII)
{
    std::string string_1 = "foo\nbar\nbaz\n";
    string_1[9] = char(-1); // Invalid byte at offset 9
    std::string string_2 = "foo\r\nbar\r\nbaz\r\n";
    string_2[11] = char(-1); // Invalid byte at offset 11
    ARCHON_TEST_FILE(path);
    {
        base::File file(path, base::File::Mode::write);
        file.write(string_2);
    }
    const std::locale& locale = std::locale::classic();
    using text_file_type = test_type;
    using char_type = typename text_file_type::char_type;
    using string_view_type = std::basic_string_view<char_type>;
    typename text_file_type::Config config;
    config.locale = locale;
    config.char_codec_buffer_size = 3;
    config.newline_codec_buffer_size = 3;
    if constexpr (text_file_type::impl_type::is_buffered)
        config.buffer_size = 3;
    text_file_type text_file(path, base::File::Mode::read, std::move(config));
    std::array<char_type, 64> buffer;
    std::size_t n = 0;
    std::error_code ec;
    if constexpr (std::is_same_v<char_type, char>) {
        if (ARCHON_CHECK(text_file.try_read(buffer, n, ec))) {
            if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
                if (ARCHON_CHECK_EQUAL(n, 12)) {
                    std::string_view data(buffer.data(), n);
                    ARCHON_CHECK_EQUAL(data, string_1);
                }
            }
            else {
                if (ARCHON_CHECK_EQUAL(n, 15)) {
                    std::string_view data(buffer.data(), n);
                    ARCHON_CHECK_EQUAL(data, string_2);
                }
            }
            ARCHON_CHECK_NOT(ec);
        }
    }
    else {
        std::array<char_type, 64> seed_memory;
        base::BasicStringWidener<char_type> widener(locale, seed_memory);
        if (ARCHON_CHECK_NOT(text_file.try_read(buffer, n, ec))) {
            if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
                if (ARCHON_CHECK_EQUAL(n, 9)) {
                    string_view_type data(buffer.data(), n);
                    ARCHON_CHECK_EQUAL(data, widener.widen("foo\nbar\nb"));
                }
            }
            else {
                if (ARCHON_CHECK_EQUAL(n, 11)) {
                    string_view_type data(buffer.data(), n);
                    ARCHON_CHECK_EQUAL(data, widener.widen("foo\r\nbar\r\nb"));
                }
            }
            ARCHON_CHECK_EQUAL(ec, base::TextFileError::invalid_byte_seq);
        }
    }
}


// It is possible to have a locale where there is no such thing as a character
// with an invalid value. Requiring ASCII is one way of making sure that at
// least one invalid character value exists (any value outside the range 0 ->
// 127).
//
ARCHON_TEST_BATCH_IF(Base_TextFile_EncodeError, variants, ARCHON_C_LOCALE_IS_ASCII)
{
    ARCHON_TEST_FILE(path);
    const std::locale& locale = std::locale::classic();
    using text_file_type = test_type;
    using char_type = typename text_file_type::char_type;
    using string_type = std::basic_string<char_type>;
    std::array<char_type, 64> seed_memory;
    base::BasicStringWidener<char_type> widener(locale, seed_memory);
    string_type string_1(widener.widen("foo\nbar\nbaz\n"));
    string_1[9] = char_type(-1); // Invalid byte at offset 9
    string_type string_2(widener.widen("foo\r\nbar\r\nbaz\r\n"));
    string_2[11] = char_type(-1); // Invalid byte at offset 11
    {
        typename text_file_type::Config config;
        config.locale = locale;
        config.char_codec_buffer_size = 3;
        config.newline_codec_buffer_size = 3;
        if constexpr (text_file_type::impl_type::is_buffered)
            config.buffer_size = 3;
        text_file_type text_file(path, base::File::Mode::write, std::move(config));
        std::size_t n = 0;
        std::error_code ec, ec_2;
        if constexpr (std::is_same_v<char_type, char>) {
            bool good = (ARCHON_CHECK(text_file.try_write(string_1, n, ec)) &&
                         ARCHON_CHECK_EQUAL(n, 12) &&
                         ARCHON_CHECK_NOT(ec) &&
                         ARCHON_CHECK(text_file.try_flush(ec)) &&
                         ARCHON_CHECK_NOT(ec));
            if (!good)
                return;
        }
        else {
            if (text_file.try_write(string_1, n, ec)) {
                bool good = (ARCHON_CHECK_EQUAL(n, 12) &&
                             ARCHON_CHECK_NOT(ec) &&
                             ARCHON_CHECK_NOT(text_file.try_flush(ec)) &&
                             ARCHON_CHECK_EQUAL(ec, base::TextFileError::invalid_char));
                if (!good)
                    return;
            }
            else if (n > 9) {
                bool good = (ARCHON_CHECK_GREATER_EQUAL(n, 10) &&
                             ARCHON_CHECK_LESS_EQUAL(n, 11) &&
                             ARCHON_CHECK_EQUAL(ec, base::TextFileError::invalid_char) &&
                             ARCHON_CHECK_NOT(text_file.try_flush(ec_2)) &&
                             ARCHON_CHECK_EQUAL(ec_2, base::TextFileError::invalid_char));
                if (!good)
                    return;
            }
            else {
                bool good = (ARCHON_CHECK_EQUAL(n, 9) &&
                             ARCHON_CHECK_EQUAL(ec, base::TextFileError::invalid_char) &&
                             ARCHON_CHECK(text_file.try_flush(ec_2)) &&
                             ARCHON_CHECK_NOT(ec_2));
                if (!good)
                    return;
            }
        }
    }
    base::File file(path);
    std::array<char, 64> buffer;
    std::size_t n = file.read(buffer);
    std::string_view data(buffer.data(), n);
    if constexpr (std::is_same_v<char_type, char>) {
        if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
            ARCHON_CHECK_EQUAL(data, string_2);
        }
        else {
            ARCHON_CHECK_EQUAL(data, string_1);
        }
    }
    else {
        if constexpr (text_file_type::impl_type::has_windows_newline_codec) {
            ARCHON_CHECK_EQUAL(data, "foo\r\nbar\r\nb");
        }
        else {
            ARCHON_CHECK_EQUAL(data, "foo\nbar\nb");
        }
    }
}


bool has_locale(const char* name)
{
    locale_t loc = ::newlocale(LC_ALL_MASK, name, 0);
    if (loc != 0) {
        ::freelocale(loc);
        return true;
    }
    return false;
}


const char* candidate_locales[] = { "C", "C.UTF-8", ".UTF8", "en_US", "en_US.UTF-8", "" };


ARCHON_TEST(Base_TextFile_AsciiCodecError_CHECK)                       
{
    using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
    auto test_decode = [](const std::locale& locale) {
        const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);
        char ch = -1;
        std::array<wchar_t, 1> buffer;
        std::mbstate_t state = {};
        const char* from     = &ch;
        const char* from_end = from + 1;
        const char* from_next;
        wchar_t* to     = buffer.data();
        wchar_t* to_end = to + buffer.size();
        wchar_t* to_next;
        auto result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        return (result == std::codecvt_base::error);
    };
    auto test_encode = [](const std::locale& locale) {
        const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);
        wchar_t ch = -1;
        std::array<char, 8> buffer;
        std::mbstate_t state = {};
        const wchar_t* from     = &ch;
        const wchar_t* from_end = from + 1;
        const wchar_t* from_next;
        char* to     = buffer.data();
        char* to_end = to + buffer.size();
        char* to_next;
        auto result = codecvt.out(state, from, from_end, from_next, to, to_end, to_next);
        return (result == std::codecvt_base::error);
    };

    for (const char* name : candidate_locales) {
        std::locale locale;
        bool has_1 = false;
        try {
            locale = std::locale(name);
            has_1 = true;
        }
        catch (std::runtime_error&) {}
        bool has_2 = has_locale(name);
        if (has_1) {
            bool decode = test_decode(locale);
            bool encode = test_encode(locale);
            log("has %s %s     decode %s     encode %s    %s", has_1, has_2, decode, encode, locale.name());
        }
        else {
            log("has %s %s                              %s", has_1, has_2, locale.name());
        }
    }
}
