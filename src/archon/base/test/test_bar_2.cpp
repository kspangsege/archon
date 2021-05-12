#include <streambuf>
#include <iostream>

#include <archon/base/demangle.hpp>
#include <archon/base/locale.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/format_enc.hpp>
#include <archon/base/file.hpp>
#include <archon/unit_test.hpp>

#include <locale.h>



template<class P, class D> class Base {
};

template<class P, class D> class Impl {
};


/*               





Basic incremental decoding (BasicCharCodec):


General questions about std::codecvt::in():
- What is the meaning of `ok`? (just all input consumed, or all input consumed and all output produced)
- What is the meaning of `partial`?
- What is the meaning of `error`?
- What is the meaning of `noconv`?
- If a bad byte is identified, will from_next point to beginning of byte sequence, or to the bad byte (will state have already absorbed good bytes)?
- 


List of quirks in implementations of std::codecvt::in():
- LLVM libc++ std::codecvt.in() never reports decoding errors?????                
- GNU libstdc++ std::codecvt.in(),  std::codecvt.out() return "ok" when the buffer size is zero. This bug is present in GCC 10.2.0. See also https://gcc.gnu.org/bugzilla/show_bug.cgi?id=37475
-


Decoding cases:
- Complete cases:
  - no input, no output space (expect ok)
  - no input, plenty of output space (expect ok)
  - end of input after one valid single-byte char, no output space (expect ??)        
  - end of input after one valid single-byte char, only enough output space for one character (expect ok)
  - end of input after one valid single-byte char, plenty of output space (expect ok)
  - end of input after one valid multi-byte char, no output space (UTF-8) (expect ??)        
  - end of input after one valid multi-byte char, only enough output space for one character (UTF-8) (expect ok)
  - end of input after one valid multi-byte char, plenty of output space (UTF-8) (expect ok)
- Partial cases:
  - end of input after 1st byte of multi-byte char, no output space (UTF-8)
  - end of input after 1st byte of multi-byte char, plenty of output space (UTF-8)
  - end of input after complete char followed by 1st byte of multi-byte char, no output space (UTF-8)
  - end of input after complete char followed by 1st byte of multi-byte char, only enough output space for one character (UTF-8)
  - end of input after complete char followed by 1st byte of multi-byte char, plenty of output space (UTF-8)
- Bad byte cases:
  - 1st byte of 1st character is a bad byte, no output space (UTF-8)
  - 1st byte of 1st character is a bad byte, plenty of output space (UTF-8)
  - 2nd byte of 1st character is a bad byte, no output space (UTF-8)
  - 2nd byte of 1st character is a bad byte, plenty of output space (UTF-8)
  - 1st byte of 2nd character is a bad byte, no output space (UTF-8)
  - 1st byte of 2nd character is a bad byte, only enough output space for one character (UTF-8)
  - 1st byte of 2nd character is a bad byte, plenty of output space (UTF-8)
  - 2nd byte of 2nd character is a bad byte, no output space (UTF-8)
  - 2nd byte of 2nd character is a bad byte, only enough output space for one character (UTF-8)
  - 2nd byte of 2nd character is a bad byte, plenty of output space (UTF-8)




bool inc_decode(std::mbstate_t&, base::Span<const char> data, std::size_t& data_offset,
                base::Span<C> buffer, std::size_t& buffer_offset, bool& error);



If this function returns true, the decoding operation completed succesfully. If it returns false, it did not.

When false is returned, \p error is set to true if the reason for the incompleteness is invalid input, and to false otherwise. BUT WHERE IS THE INVALID INPUT? AND HOW TO RESUME?

When false is returned, and \p error is set to false, and buffer_offset is equal to the size of the specified buffer upon return, additional buffer space is required in order for decoding to proceed.

When false is returned, and \p error is set to false, and buffer_offset is less than the size of the specified buffer upon return, additional input (\p data) is required in order for decoding to proceed.

In any case, data_offset and buffer_offset are updated to reflect the how far decoding has progressed.

If an exception is thrown, more data may have been written to the buffer than indicated by the final value of buffer_offset.



How to test:
    



















Finish implementation of BasicTextCodec:
- Implement decoding and encoding involving char codec.
- Verify that it is possible to plug in a custom char codec.
- Find a way to express short-circuiting API (e.g., std::string_view encode_sc(D&& data, base::SeedMemoryBuffer<C>&)).
- Add BasicTextEncoder and BasicTextDecoder classes as wrappers around BasicTextCodec. These will contain the target buffer when, and only when the codec is nontrivial, and only offer short-circuiting API variant.










Current plan:

- While searching for decode error locale, be prepared to add an arbitrary tail of 'x'es based on what codecvt.max_length() returns.




Linux ---> C.UTF-8



add encode test

VARY BUFFER SIZES RANDOMLY, especially in Base_TextFile_EncodeError
hmmmmmmm    a bit of a problem that unit test repetition does not try many different random seeds
---> Solution: Always compute a new seed sequence when a unit test ask for one, and do it in a way that combines the command line specified seed as well as the index into the repetition sequence. Then also provide a command line option to override the index into the repetition sequence as it is used for the random seed.


GENERAL RULE ALL THE WAY FROM basic text file down for base::File is that if write()/try_write() fails, then n is strictly less than data.size()                  
----> Similarly, if read() fails, then n is strictly less than buffer.size()


OOOOOOOPS, apparently we cannot assume that when write() fails, `n` is the position of the failure (buffering destroys this) Be sure that the documentation does not promise too much   
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


// Consider adding `write(const C* c_str)` and `try_write(const C* c_str, ...)` overloads to non-impl text files.
// ----------> RATHER: Use base::span_from_string() as in BasicTextCodec::encode() <----------                       


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

    static constexpr bool is_nontrivial = !std::is_same_v<C, char>;

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
                             std::size_t& data_offset, std::size_t buffer_size);

    /// \brief   
    ///
    /// See \ref simulate_inc_decode().
    ///
    static constexpr std::size_t max_simulate_decode_size() noexcept;

    /// \{
    ///
    /// Copyability    
    BasicCharCodec(const BasicCharCodec&) noexcept = default;
    BasicCharCodec& operator=(const BasicCharCodec&) noexcept = default;
    /// \}

private:
    using codecvt_type = std::codecvt<C, char, std::mbstate_t>;

    std::locale m_locale;
    const codecvt_type* m_codecvt;
    bool m_stateful = (m_codecvt->encoding() == -1);
};


using CharCodec     = BasicCharCodec<char>;
using WideCharCodec = BasicCharCodec<wchar_t>;








// Implementation


template<class C, class T> inline BasicCharCodec<C, T>::BasicCharCodec(const std::locale& locale) :
    m_locale(locale),
    m_codecvt(&std::use_facet<codecvt_type>(m_locale)) // Throws
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
    // Deal with bug in GNU libstdc++ causing m_codecvt.in() to return "ok" when
    // the buffer size is zero. This bug is present in GCC 10.2.0. See also
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
        m_codecvt->in(state, from, from_end, from_next, to, to_end, to_next);
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
    // Deal with bug in GNU libstdc++ causing m_codecvt->out() to return "ok"
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
        m_codecvt->out(state, from, from_end, from_next, to, to_end, to_next);
    data_offset   = std::size_t(from_next - data.data());
    buffer_offset = std::size_t(to_next - buffer.data());
    return result;
}


template<class C, class T>
void  BasicCharCodec<C, T>::simulate_inc_decode(std::mbstate_t& state, base::Span<const char> data,
                                                std::size_t& data_offset, std::size_t buffer_size)
{
    ARCHON_ASSERT(data_offset <= data.size());
    ARCHON_ASSERT(std::size_t(data.size() - data_offset) <= max_simulate_decode_size());
    const char* begin = data.data() + data_offset;
    const char* end   = data.data() + data.size();
    int n = m_codecvt->length(state, begin, end, buffer_size);
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
    using char_type   = C;
    using traits_type = T;

};


using ExtendedCharCodec     = BasicExtendedCharCodec<char>;
using WideExtendedCharCodec = BasicExtendedCharCodec<wchar_t>;


} // namespace archon::base
*/




// ============================================================================================ detail/text_codec_impl.hpp ============================================================================================


namespace archon::base::detail {



template<class C, class T, class P, class I> class TextCodecImpl1 :
        private P {
public:
    static constexpr bool is_nontrivial = P::is_nontrivial;

    using decoder_type = typename P::decoder_type;
    using encoder_type = typename P::encoder_type;

    TextCodecImpl1(const std::locale&) noexcept;

    template<class J> static const J& up_cast(const I&) noexcept;
};



template<class C, class T, class P, class D, class I> class TextCodecImpl2 :
        private P {
private:
    class Decoder;
    class Encoder;
    template<class E> class CompoundEncoder;

public:
    static constexpr bool is_nontrivial = true;

    using decoder_type = typename P::template build_decoder_type<Decoder>;
    using encoder_type = typename P::template build_encoder_type<Encoder, CompoundEncoder>;

    TextCodecImpl2(const std::locale&);

    template<class J> static const J& up_cast(const I&) noexcept;
    template<class J> static const I& down_cast(const J&) noexcept;

private:
    D m_char_codec;
};



template<class C, class T, class P, class D, class I, bool N> class TextCodecImpl3 :
        public TextCodecImpl1<C, T, P, I> {
public:
    using TextCodecImpl1<C, T, P, I>::TextCodecImpl1;
};

template<class C, class T, class P, class D, class I> class TextCodecImpl3<C, T, P, D, I, true> :
        public TextCodecImpl2<C, T, P, D, I> {
public:
    using TextCodecImpl2<C, T, P, D, I>::TextCodecImpl2;
};



template<class C, class T, class P, class D> class TextCodecImpl :
        public TextCodecImpl3<C, T, P, D, TextCodecImpl<C, T, P, D>, D::is_nontrivial> {
public:
    using TextCodecImpl3<C, T, P, D, TextCodecImpl, D::is_nontrivial>::TextCodecImpl3;
};



class PrimPosixTextCodecImpl {
private:
    class Copier;
    class Decoder;
    class Encoder;

public:
    static constexpr bool is_nontrivial = false;

    using decoder_type = Decoder;
    using encoder_type = Encoder;

    template<class E> using build_decoder_type = E;
    template<class E, template<class> class> using build_encoder_type = E;

    template<class I> static const I& up_cast(const PrimPosixTextCodecImpl&) noexcept;
};



class PrimWindowsTextCodecImpl {
private:
    class Decoder;
    class Encoder;
    template<class E> class CompoundDecoder;

public:
    static constexpr bool is_nontrivial = true;

    using decoder_type = Decoder;
    using encoder_type = Encoder;

    template<class E> using build_decoder_type = CompoundDecoder<E>;
    template<class, template<class> class E> using build_encoder_type = E<Encoder>;

    template<class I> static const I& up_cast(const PrimWindowsTextCodecImpl&) noexcept;
    template<class I> static const PrimWindowsTextCodecImpl& down_cast(const I&) noexcept;
};



#if ARCHON_WINDOWS
using PrimTextCodecImpl = PrimWindowsTextCodecImpl;
#else
using PrimTextCodecImpl = PrimPosixTextCodecImpl;
#endif








// Implementation


// ============================ TextCodecImpl1 ============================


template<class C, class T, class P, class I>
inline TextCodecImpl1<C, T, P, I>::TextCodecImpl1(const std::locale&) noexcept
{
}


template<class C, class T, class P, class I> template<class J>
inline auto TextCodecImpl1<C, T, P, I>::up_cast(const I& impl) noexcept -> const J&
{
    if constexpr (std::is_same_v<J, I>) {
        return impl;
    }
    else {
        return P::template up_cast<J>(impl);
    }
}



// ============================ TextCodecImpl2 ============================


template<class C, class T, class P, class D, class I>
class TextCodecImpl2<C, T, P, D, I>::Decoder {
public:
    using impl_type = I;

    Decoder(const I& impl, base::SeedMemoryBuffer<C>& buffer,
            std::size_t& buffer_offset) noexcept :
        m_char_codec(impl.m_char_codec),
        m_buffer(buffer),
        m_buffer_offset(buffer_offset)
    {
    }

    bool decode(base::Span<const char> data, std::size_t& data_offset, bool end_of_data)
    {
/*
        for (;;) {
            // FIXME: Understanding: `ok` means: All input, possibly with the exception of a final incomplete, but valid byte sequence, has been consumed, and all the corresponding output has been produced.                                                  
            // -----------> Hmm, probably not possible to uphold this due to baroque quirk in libc++ (LLVM) of never reporting decoding errors.
            std::codecvt_base::result result =
                m_char_codec.inc_decode(data, data_offset, end_of_data,
                                        m_buffer, m_buffer_offset); // Throws
            switch (result) {
                case std::codecvt_base::ok:
                    return true; // Success
                case std::codecvt_base::partial:
                    ARCHON_ASSERT(m_buffer_offset == m_buffer.size());
                    m_buffer.reserve_extra(1, m_buffer.size()); // Throws
                    continue;
                case std::codecvt_base::error:
                    return false; // Failure
                case std::codecvt_base::noconv:
                    // Not possible, because trivial case is handled by TextCodecImpl1
                    break;
            }
            ARCHON_ASSERT_UNREACHABLE;
        }
*/

        static_cast<void>(data);                                                     
        static_cast<void>(data_offset);                                                     
        static_cast<void>(end_of_data);                                                     
        return false;
    }

private:
    const D& m_char_codec;
    base::SeedMemoryBuffer<C>& m_buffer;
    std::size_t& m_buffer_offset;
};


template<class C, class T, class P, class D, class I>
class TextCodecImpl2<C, T, P, D, I>::Encoder {
public:
    using impl_type = I;

    Encoder(const I& impl, base::SeedMemoryBuffer<char>& buffer,
            std::size_t& buffer_offset) noexcept :
        m_char_codec(impl.m_char_codec),
        m_buffer(buffer),
        m_buffer_offset(buffer_offset)
    {
    }

    bool encode(base::Span<const C> data)
    {
        static_cast<void>(data);                                                                                                                                                                
        return false;
    }

private:
    const D& m_char_codec;
    base::SeedMemoryBuffer<char>& m_buffer;
    std::size_t& m_buffer_offset;
};


template<class C, class T, class P, class D, class I>
template<class E> class TextCodecImpl2<C, T, P, D, I>::CompoundEncoder :
        private E {
public:
    using impl_type = I;

    CompoundEncoder(const I& impl, base::SeedMemoryBuffer<char>& buffer,
                    std::size_t& buffer_offset) :
        E(I::template up_cast<typename E::impl_type>(impl), buffer, buffer_offset), // Throws
        m_char_codec(impl.m_char_codec)
    {
    }

    bool encode(base::Span<const C> data)
    {
        static_cast<void>(data);                                                     
        return false;
    }

private:
    const D& m_char_codec;
};


template<class C, class T, class P, class D, class I>
inline TextCodecImpl2<C, T, P, D, I>::TextCodecImpl2(const std::locale& locale) :
    m_char_codec(locale) // Throws
{
}


template<class C, class T, class P, class D, class I> template<class J>
inline auto TextCodecImpl2<C, T, P, D, I>::up_cast(const I& impl) noexcept -> const J&
{
    if constexpr (std::is_same_v<J, I>) {
        return impl;
    }
    else {
        return P::template up_cast<J>(impl);
    }
}


template<class C, class T, class P, class D, class I> template<class J>
inline auto TextCodecImpl2<C, T, P, D, I>::down_cast(const J& impl) noexcept -> const I&
{
    if constexpr (std::is_same_v<J, I>) {
        return impl;
    }
    else {
        return static_cast<const I&>(P::down_cast(impl));
    }
}


/*
template<class C, class T>
bool TextCodecImpl<C, T>::decode(base::Span<const char> data, base::SeedMemoryBuffer<C>& buffer,
                                 std::size_t& buffer_offset)
{
        std::array<C, 512> buffer_2;
        state_type state = {};
        for (;;) {
            std::size_t buffer_offset_2 = 0;
            bool end_of_data = true;
            std::codecvt_base::result result =
                m_char_codec.inc_decode(state, data, data_offset, buffer_2, buffer_offset_2,
                                        end_of_data); // Throws
            base::Span<const char> data_2 = { buffer_2.data(), buffer_offset_2 };
            m_subimpl.encode(data_2, buffer, buffer_offset); // Throws
            switch (result) {
                case std::codecvt_base::ok:
                    return true; // Success
                case std::codecvt_base::partial:
                    continue;
                case std::codecvt_base::error:
                    return false; // Failure
                case std::codecvt_base::noconv:
                    // Not possible, because of specialization for `char`       
                    break;
            }
            ARCHON_ASSERT_UNREACHABLE;
        }
}


template<class C, class T>
bool TextCodecImpl<C, T>::encode(base::Span<const C> data, bool end_of_data,
                                 base::SeedMemoryBuffer<char>& buffer, std::size_t& buffer_offset)
{
    state_type state = {};
    if constexpr (P::is_trivial) {
        for (;;) {
            std::codecvt_base::result result =
                m_char_codec.inc_encode(state, data, data_offset, buffer, buffer_offset,
                                        end_of_data); // Throws
            switch (result) {
                case std::codecvt_base::ok:
                    return true; // Success
                case std::codecvt_base::partial:
                    buffer.expand(1, buffer_offset); // Throws
                    continue;
                case std::codecvt_base::error:
                    return false; // Failure
                case std::codecvt_base::noconv:
                    // Not possible, because of specialization for `char`
                    break;
            }
            ARCHON_ASSERT_UNREACHABLE;
        }
    }
    else {
        std::array<char, 512> buffer_2;
        for (;;) {
            std::size_t buffer_offset_2 = 0;
            std::codecvt_base::result result =
                m_char_codec.inc_encode(state, data, data_offset, buffer_2, buffer_offset_2,
                                        end_of_data); // Throws
            base::Span<const char> data_2 = { buffer_2.data(), buffer_offset_2 };
            m_prim_impl.encode(data_2, buffer, buffer_offset); // Throws
            switch (result) {
                case std::codecvt_base::ok:
                    return true; // Success
                case std::codecvt_base::partial:
                    continue;
                case std::codecvt_base::error:
                    return false; // Failure
                case std::codecvt_base::noconv:
                    // Not possible, because of specialization for `char`
                    break;
            }
            ARCHON_ASSERT_UNREACHABLE;
        }
    }
}
*/



// ============================ PrimPosixTextCodecImpl ============================


class PrimPosixTextCodecImpl::Copier {
public:
    void copy(base::Span<const char> data)
    {
        m_buffer.reserve_extra(data.size(), m_buffer_offset); // Throws
        char* base = m_buffer.data();
        char* i = std::copy_n(data.data(), data.size(), base + m_buffer_offset);
        m_buffer_offset = std::size_t(i - base);
    }

protected:
    Copier(base::SeedMemoryBuffer<char>& buffer, std::size_t& buffer_offset) noexcept :
        m_buffer(buffer),
        m_buffer_offset(buffer_offset)
    {
    }

    ~Copier() noexcept = default;

private:
    base::SeedMemoryBuffer<char>& m_buffer;
    std::size_t& m_buffer_offset;
};


class PrimPosixTextCodecImpl::Decoder :
    private Copier {
public:
    using impl_type = PrimPosixTextCodecImpl;

    Decoder(const PrimPosixTextCodecImpl&, base::SeedMemoryBuffer<char>& buffer,
            std::size_t& buffer_offset) noexcept :
        Copier(buffer, buffer_offset)
    {
    }

    bool decode(base::Span<const char> data, std::size_t& data_offset, bool)
    {
        copy(data); // Throws
        data_offset = data.size();
        return true;
    }
};


class PrimPosixTextCodecImpl::Encoder :
    private Copier {
public:
    using impl_type = PrimPosixTextCodecImpl;

    Encoder(const PrimPosixTextCodecImpl&, base::SeedMemoryBuffer<char>& buffer,
            std::size_t& buffer_offset) noexcept :
        Copier(buffer, buffer_offset)
    {
    }

    bool encode(base::Span<const char> data)
    {
        copy(data); // Throws
        return true;
    }
};


template<class I>
inline auto PrimPosixTextCodecImpl::up_cast(const PrimPosixTextCodecImpl& impl) noexcept ->
    const I&
{
    return impl;
}



// ============================ PrimWindowsTextCodecImpl ============================


class PrimWindowsTextCodecImpl::Decoder {
public:
    using impl_type = PrimWindowsTextCodecImpl;

    Decoder(const PrimWindowsTextCodecImpl&, base::SeedMemoryBuffer<char>& buffer,
            std::size_t& buffer_offset) noexcept :
        m_buffer(buffer),
        m_buffer_offset(buffer_offset)
    {
    }

    bool decode(base::Span<const char> data, std::size_t& data_offset, bool end_of_data)
    {
        std::size_t clear_offset = 0; // Dummy
        std::size_t clear; // Dummy
        for (;;) {
            newline_crlf::inc_decode(data, data_offset, m_buffer, m_buffer_offset,
                                     clear_offset, clear, end_of_data);
            if (ARCHON_LIKELY(data_offset == data.size() || m_buffer_offset < m_buffer.size()))
                return true;
            m_buffer.reserve_extra(1, m_buffer.size()); // Throws
        }
    }

private:
    base::SeedMemoryBuffer<char>& m_buffer;
    std::size_t& m_buffer_offset;
};


class PrimWindowsTextCodecImpl::Encoder {
public:
    using impl_type = PrimWindowsTextCodecImpl;

    Encoder(const PrimWindowsTextCodecImpl&, base::SeedMemoryBuffer<char>& buffer,
            std::size_t& buffer_offset) noexcept :
        m_buffer(buffer),
        m_buffer_offset(buffer_offset)
    {
    }

    bool encode(base::Span<const char> data)
    {
        std::size_t data_offset = 0;
        for (;;) {
            newline_crlf::inc_encode(data, data_offset, m_buffer, m_buffer_offset);
            if (ARCHON_LIKELY(data_offset == data.size()))
                return true;
            m_buffer.expand(1, m_buffer_offset); // Throws
        }
    }

private:
    base::SeedMemoryBuffer<char>& m_buffer;
    std::size_t& m_buffer_offset;
};


template<class E> class PrimWindowsTextCodecImpl::CompoundDecoder :
     private E {
public:
    using impl_type = PrimWindowsTextCodecImpl;

    template<class C>
    CompoundDecoder(const PrimWindowsTextCodecImpl& impl, base::SeedMemoryBuffer<C>& buffer,
                    std::size_t& buffer_offset) :
        E(E::impl_type::down_cast(impl), buffer, buffer_offset) // Throws
    {
    }

    bool decode(base::Span<const char> data, std::size_t& data_offset, bool end_of_data)
    {
        std::size_t clear; // Dummy
        for (;;) {
            std::size_t clear_offset = 0; // Dummy
            newline_crlf::inc_decode(data, data_offset, m_buffer, m_buffer_offset,
                                     clear_offset, clear, end_of_data);
            base::Span<const char> data_2(m_buffer.data(), m_buffer_offset);
            std::size_t data_offset_2 = 0;
            bool end_of_data_2 = (end_of_data && data_offset == data.size());
            bool success = E::decode(data_2, data_offset_2, end_of_data_2); // Throws
            if (ARCHON_LIKELY(success)) {
                // Move any remaining data to start of buffer
                char* base = m_buffer.data();
                char* i = std::copy(base + data_offset_2, base + m_buffer_offset, base);
                bool done = (data_offset == data.size() || m_buffer_offset < m_buffer.size());
                m_buffer_offset = std::size_t(i - base);
                if (ARCHON_LIKELY(done))
                    return true;
                m_buffer.reserve_extra(1, m_buffer_offset); // Throws
                continue;
            }
            return false;
        }
    }

private:
    base::ArraySeededBuffer<char, 512> m_buffer;
    std::size_t m_buffer_offset = 0;
};


template<class I>
inline auto PrimWindowsTextCodecImpl::up_cast(const PrimWindowsTextCodecImpl& impl) noexcept ->
    const I&
{
    return impl;
}


template<class I> inline auto PrimWindowsTextCodecImpl::down_cast(const I& impl) noexcept ->
    const PrimWindowsTextCodecImpl&
{
    return impl;
}


} // namespace archon::base::detail





// ============================================================================================ text_codec.hpp ============================================================================================


namespace archon::base {


/// \{
///
/// \brief 
///
/// Explain requirements for character codec type `D`.             
///
/// All three types are empty classes unless D::is_nontrivial is true.
///
template<class C, class T, class D = base::BasicCharCodec<C, T>> using TextCodecImpl =
    detail::TextCodecImpl<C, T, detail::PrimTextCodecImpl, D>;
template<class C, class T, class D = base::BasicCharCodec<C, T>> using PosixTextCodecImpl =
    detail::TextCodecImpl<C, T, detail::PrimPosixTextCodecImpl, D>;
template<class C, class T, class D = base::BasicCharCodec<C, T>> using WindowsTextCodecImpl =
    detail::TextCodecImpl<C, T, detail::PrimWindowsTextCodecImpl, D>;
/// \}



/// \brief 
///
/// Explain requirements for text codec implementation type `I`.             
///
/// This class is an empty class if I is an empty class.
///
template<class C, class T = std::char_traits<C>, class I = base::TextCodecImpl<C, T>>
class BasicTextCodec : private I {
public:
    using char_type   = C;
    using traits_type = T;
    using impl_type   = I;

    using string_view_type = std::basic_string_view<C, T>;

    static constexpr bool is_nontrivial = I::is_nontrivial;

    BasicTextCodec(const std::locale&);

    template<class D> string_view_type decode(D&& data, base::SeedMemoryBuffer<C>&) const;
    template<class D> std::string_view encode(D&& data, base::SeedMemoryBuffer<char>&) const;

    template<class D> bool try_decode(D&& data, base::SeedMemoryBuffer<C>&,
                                      std::size_t& buffer_offset) const;
    template<class D> bool try_encode(D&& data, base::SeedMemoryBuffer<char>&,
                                      std::size_t& buffer_offset) const;

private:
    string_view_type do_decode(base::Span<const char> data, base::SeedMemoryBuffer<C>&) const;
    bool do_try_decode(base::Span<const char> data, base::SeedMemoryBuffer<C>&,
                       std::size_t& buffer_offset) const;

    std::string_view do_encode(base::Span<const C> data, base::SeedMemoryBuffer<char>&) const;
    bool do_try_encode(base::Span<const C> data, base::SeedMemoryBuffer<char>&,
                       std::size_t& buffer_offset) const;
};


template<class C, class T = std::char_traits<C>> using BasicPosixTextCodec =
    BasicTextCodec<C, T, PosixTextCodecImpl<C, T>>;
template<class C, class T = std::char_traits<C>> using BasicWindowsTextCodec =
    BasicTextCodec<C, T, WindowsTextCodecImpl<C, T>>;

using TextCodec        = BasicTextCodec<char>;
using PosixTextCodec   = BasicPosixTextCodec<char>;
using WindowsTextCodec = BasicWindowsTextCodec<char>;

using WideTextCodec        = BasicTextCodec<wchar_t>;
using WidePosixTextCodec   = BasicPosixTextCodec<wchar_t>;
using WideWindowsTextCodec = BasicWindowsTextCodec<wchar_t>;








// Implementation


template<class C, class T, class I>
inline BasicTextCodec<C, T, I>::BasicTextCodec(const std::locale& locale) :
    I(locale) // Throws
{
}


template<class C, class T, class I> template<class D>
inline auto BasicTextCodec<C, T, I>::decode(D&& data,
                                            base::SeedMemoryBuffer<C>& buffer) const ->
    string_view_type
{
    return do_decode(base::span_from_string<char>(std::forward<D>(data)), buffer); // Throws
}


template<class C, class T, class I> template<class D>
inline auto BasicTextCodec<C, T, I>::encode(D&& data,
                                            base::SeedMemoryBuffer<char>& buffer) const ->
    std::string_view
{
    return do_encode(base::span_from_string<C>(std::forward<D>(data)), buffer); // Throws
}


template<class C, class T, class I> template<class D>
inline bool BasicTextCodec<C, T, I>::try_decode(D&& data, base::SeedMemoryBuffer<C>& buffer,
                                                std::size_t& buffer_offset) const
{
    return do_try_decode(base::span_from_string<char>(std::forward<D>(data)),
                         buffer, buffer_offset); // Throws
}


template<class C, class T, class I> template<class D>
inline bool BasicTextCodec<C, T, I>::try_encode(D&& data, base::SeedMemoryBuffer<char>& buffer,
                                                std::size_t& buffer_offset) const
{
    return do_try_encode(base::span_from_string<C>(std::forward<D>(data)),
                         buffer, buffer_offset); // Throws
}


template<class C, class T, class I>
inline auto BasicTextCodec<C, T, I>::do_decode(base::Span<const char> data,
                                               base::SeedMemoryBuffer<C>& buffer) const ->
    string_view_type
{
    std::size_t buffer_offset = 0;
    if (ARCHON_LIKELY(do_try_decode(data, buffer, buffer_offset))) // Throws
        return { buffer.data(), buffer_offset };
    throw std::runtime_error("Decoding failed");
}


template<class C, class T, class I>
bool BasicTextCodec<C, T, I>::do_try_decode(base::Span<const char> data,
                                            base::SeedMemoryBuffer<C>& buffer,
                                            std::size_t& buffer_offset) const
{
    using decoder_type = typename I::decoder_type;
    using decoder_impl_type = typename decoder_type::impl_type;
    const decoder_impl_type& impl = I::template up_cast<decoder_impl_type>(*this);
    decoder_type decoder(impl, buffer, buffer_offset); // Throws
    std::size_t data_offset = 0;
    bool end_of_data = true;
    bool success = decoder.decode(data, data_offset, end_of_data); // Throws
    ARCHON_ASSERT(!success || data_offset == data.size());
    return success;
}


template<class C, class T, class I>
inline auto BasicTextCodec<C, T, I>::do_encode(base::Span<const C> data,
                                               base::SeedMemoryBuffer<char>& buffer) const ->
    std::string_view
{
    std::size_t buffer_offset = 0;
    if (ARCHON_LIKELY(do_try_encode(data, buffer, buffer_offset))) // Throws
        return { buffer.data(), buffer_offset };
    throw std::runtime_error("Encoding failed");
}


template<class C, class T, class I>
bool BasicTextCodec<C, T, I>::do_try_encode(base::Span<const C> data,
                                            base::SeedMemoryBuffer<char>& buffer,
                                            std::size_t& buffer_offset) const
{
    using encoder_type = typename I::encoder_type;
    using encoder_impl_type = typename encoder_type::impl_type;
    const encoder_impl_type& impl = I::template up_cast<encoder_impl_type>(*this);
    encoder_type encoder(impl, buffer, buffer_offset); // Throws
    return encoder.encode(data); // Throws
}


} // namespace archon::base





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
    static constexpr std::size_t default_newline_codec_buffer_size = 4096;

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
        if (ARCHON_LIKELY(n > 0)) {
            if (ARCHON_LIKELY(m_file.try_seek(-n, whence, result, ec))) {
                m_retain_size = 0;
                goto proceed;
            }
            return false; // Failure
        }
      proceed:
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true; // Success
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
        if (ARCHON_LIKELY(n > 0)) {
            if (ARCHON_LIKELY(m_file.try_seek(-n, whence, result, ec)))
                goto proceed;
            return false; // Failure
        }
      proceed:
        m_begin = 0;
        m_end   = 0;
        m_curr  = 0;
        m_retain_size  = 0;
        m_retain_clear = 0;
#if ARCHON_DEBUG
        m_reading = false;
#endif
        return true; // Success
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

    TextFileImpl(base::File&, std::locale, Config);

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

    /// The file implementation must be in neutral mode when this function is called.
    void imbue(const std::locale&, std::mbstate_t);

private:
    prim_impl_type m_prim_impl;
    base::BasicCharCodec<C, T> m_codec;
    base::SeedMemoryBuffer<char> m_buffer;

    // `m_state` is always the state at the logical read/write position. In
    // neutral mode, `m_state_2` is equal to `m_state`. In reading mode,
    // `m_state_2` is the state at the read ahead position.  In writing mode,
    // the value of `m_state_2` in undefined.
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

    TextFileImpl(base::File&, std::locale, Config);

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

    void imbue(const std::locale&, std::mbstate_t);

private:
    prim_impl_type m_prim_impl;
};



template<class C, class T = std::char_traits<C>>
using PosixTextFileImpl = TextFileImpl<C, T, base::PrimPosixTextFileImpl>;

template<class C, class T = std::char_traits<C>>
using WindowsTextFileImpl = TextFileImpl<C, T, base::PrimWindowsTextFileImpl>;



struct TextFileImplConfig :
        base::PrimTextFileImplConfig {
    static constexpr std::size_t default_char_codec_buffer_size = 4096;

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
inline TextFileImpl<C, T, P>::TextFileImpl(base::File& file, std::locale locale, Config config) :
    m_prim_impl(file, std::move(config)), // Throws
    m_codec(locale), // Throws
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
            m_codec.inc_encode(m_state, data, data_offset, m_buffer, m_end); // Throws
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


template<class C, class T, class P>
inline void TextFileImpl<C, T, P>::imbue(const std::locale& locale, std::mbstate_t state)
{
#if ARCHON_DEBUG
    ARCHON_ASSERT(!m_reading);
    ARCHON_ASSERT(!m_writing);
#endif

    m_codec = base::BasicCharCodec<C, T>(locale); // Throws
    m_state   = state;
    m_state_2 = state;
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
inline TextFileImpl<char, T, P>::TextFileImpl(base::File& file, std::locale, Config config) :
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


template<class T, class P>
inline void TextFileImpl<char, T, P>::imbue(const std::locale&, std::mbstate_t)
{
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

    BufferedTextFileImpl(base::File&, std::locale, Config);

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
    static constexpr std::size_t default_buffer_size = 4096;

    // GENERIC:
    //
    // Buffer will be automatically expanded if necessary.              
    //
    std::size_t buffer_size = default_buffer_size;
    base::Span<C> buffer_memory;
};








// Implementation


template<class C, class T, class I>
inline BufferedTextFileImpl<C, T, I>::BufferedTextFileImpl(base::File& file, std::locale locale,
                                                           Config config) :
    m_subimpl(file, std::move(locale), std::move(config)), // Throws
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


///  
///
/// FIXME: Consider replacing flush() with sync() which will also revert file position and discard buffered data in reading mode.          
///
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
    std::locale locale;
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
    m_impl(m_file, std::move(config.locale), std::move(config)), // Throws
    m_dynamic_eof(config.dynamic_eof)
{
}


template<class C, class T, class I>
inline std::size_t BasicTextFile<C, T, I>::read_some(base::Span<C> buffer)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_some(buffer, n, ec))) // Throws
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


template<class C, class T, class I>
inline std::size_t BasicTextFile<C, T, I>::read(base::Span<C> buffer)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read(buffer, n, ec))) // Throws
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


template<class C, class T, class I>
inline void BasicTextFile<C, T, I>::write(base::Span<const C> data)
{
    std::size_t n; // Dummy
    std::error_code ec;
    if (ARCHON_LIKELY(try_write(data, n, ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to write to file");
}


template<class C, class T, class I> inline void BasicTextFile<C, T, I>::flush()
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_flush(ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to flush");
}


template<class C, class T, class I> inline auto BasicTextFile<C, T, I>::tell() -> pos_type
{
    pos_type pos;
    std::error_code ec;
    if (ARCHON_LIKELY(try_tell(pos, ec))) // Throws
        return pos; // Success
    throw std::system_error(ec, "Failed to determine read/write position");
}


template<class C, class T, class I> inline void BasicTextFile<C, T, I>::seek(pos_type pos)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_seek(pos, ec))) // Throws
        return; // Success
    throw std::system_error(ec, "Failed to update read/write position");
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_read_some(base::Span<C> buffer, std::size_t& n,
                                                  std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_writing)) {
      proceed:
        m_reading = true;
        return do_read_some(buffer, n, ec); // Throws
    }
    if (ARCHON_LIKELY(stop_writing(ec))) // Throws
        goto proceed;
    return false; // Failure to stop writing
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_read(base::Span<C> buffer, std::size_t& n,
                                             std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_writing)) {
      proceed:
        m_reading = true;
        base::Span<C> buffer_2 = buffer;
        std::size_t n_2 = 0;
        while (ARCHON_LIKELY(do_read_some(buffer_2, n_2, ec))) {
            ARCHON_ASSERT(n_2 <= buffer_2.size());
            if (ARCHON_LIKELY(n_2 > 0 && n_2 < buffer_2.size())) {
                buffer_2 = buffer_2.subspan(n_2);
                continue;
            }
            n = std::size_t(buffer_2.data() + n_2 - buffer.data());
            return true; // Success
        }
        n = std::size_t(buffer_2.data() - buffer.data());
        return false; // Failure to read
    }
    if (ARCHON_LIKELY(stop_writing(ec))) // Throws
        goto proceed;
    n = 0;
    return false; // Failure to stop writing
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_write(base::Span<const C> data, std::size_t& n,
                                              std::error_code& ec)
{
    if (ARCHON_LIKELY(!m_reading)) {
      proceed:
        m_writing = true;
        return m_impl.write(data, n, ec); // Throws
    }
    if (ARCHON_LIKELY(stop_reading(ec))) // Throws
        goto proceed;
    n = 0;
    return false; // Failure to stop reading
}


template<class C, class T, class I>
inline bool BasicTextFile<C, T, I>::try_flush(std::error_code& ec)
{
    if (ARCHON_LIKELY(m_writing)) {
        if (ARCHON_LIKELY(m_impl.flush(ec))) { // Throws
            m_writing = false;
            return true; // Success
        }
        return false; // Failure
    }
    return true; // Trivial success
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
    if (ARCHON_LIKELY(m_writing)) {
        if (ARCHON_LIKELY(stop_writing(ec))) // Throws
            goto proceed;
        return false; // Failure to stop writing
    }
  proceed:
    if (ARCHON_LIKELY(m_impl.seek(pos, ec))) { // Throws
        m_reading = false;
        return true; // Success
    }
    return false; // Failure
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





// ============================================================================================ text_file_stream.hpp ============================================================================================


namespace archon::base {


template<class C, class T, class I> class BasicTextFileStreambuf;



template<class C, class T = std::char_traits<C>, class I = base::TextFileImpl<C, T>>
class BasicTextFileStream :
    public std::basic_iostream<C, T> {
public:
    using char_type   = C;
    using traits_type = T;
    using Mode        = base::File::Mode;
    using Config      = typename BasicTextFileStreambuf<C, T, I>::Config;

    BasicTextFileStream(base::FilesystemPathRef, Mode = Mode::read);
    BasicTextFileStream(base::FilesystemPathRef, Mode, Config);

private:
    base::File m_file;
    BasicTextFileStreambuf<C, T, I> m_streambuf;
};


template<class C, class T = std::char_traits<C>>
using BasicPosixTextFileStream = BasicTextFileStream<C, T, base::PosixTextFileImpl<C, T>>;

template<class C, class T = std::char_traits<C>>
using BasicWindowsTextFileStream = BasicTextFileStream<C, T, base::WindowsTextFileImpl<C, T>>;

using TextFileStream        = BasicTextFileStream<char>;
using PosixTextFileStream   = BasicPosixTextFileStream<char>;
using WindowsTextFileStream = BasicWindowsTextFileStream<char>;

using WideTextFileStream        = BasicTextFileStream<wchar_t>;
using WidePosixTextFileStream   = BasicPosixTextFileStream<wchar_t>;
using WideWindowsTextFileStream = BasicWindowsTextFileStream<wchar_t>;




/// \brief Stream buffer for text file streams.
///
///   
///
/// FIXME: Implement xsputn() for improved efficiency (avoid call from virtual to vitual).    
/// FIXME: Implement xsgetn() for improved efficiency (avoid call from virtual to vitual).    
/// FIXME: Implement uflow() for improved efficiency (avoid call from virtual to vitual).    
///
/// With this implementation, `seekoff()` fails unless the spcified offset is
/// zero and the specified direction is `std::ios_base::cur`. This means that
/// relative seeking is unsupported, and that `seekoff()` can only be used for
/// the purpose of telling the current read/write position.
///
/// With this implementation, `setbuf()` has no effect. To use a custom buffer,
/// specify it through \ref Config::buffer_memory.
///
/// With this implementation, `showmanyc()` always returns 0, which means that
/// `in_avail()`, and, in turn, `std::basic_istream<C, T>::readsome()` will
/// generally not work in a useful way, and it is not clear that there is any
/// way to remedy the situation. See \ref BasicTextFile<C, T>::read_some() for a
/// working alternative.
///
/// With this implementation, `pbackfail()` always fails (returns `T::eof()`),
/// so `sungetc()` and `sputbackc()` (`unget()` and `putback()` in
/// `std::basic_istream`) can only be relied on immediately after advancing the
/// read position, such as through `sbumpc()`.
///
/// This implementation assumes that `setg()` and `setp()` in
/// `std::basic_streambuf` never throw. While this is not currently guaranteed
/// by the standard (C++17), it is deemed very likely to be the case in
/// practice. This assumption is necessary in order to uphold critical
/// invariants.
///
template<class C, class T = std::char_traits<C>, class I = base::TextFileImpl<C, T>>
class BasicTextFileStreambuf :
    public std::basic_streambuf<C, T> {
public:
    struct Config;

    using char_type   = C;
    using traits_type = T;
    using int_type    = typename T::int_type;
    using off_type    = typename T::off_type;
    using pos_type    = typename T::pos_type;

    BasicTextFileStreambuf(base::File&);
    BasicTextFileStreambuf(base::File&, std::locale, Config);

protected:
    void imbue(const std::locale&) override;
    int_type underflow() override;
    int_type overflow(int_type) override;
    int sync() override;
    pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override;
    pos_type seekpos(pos_type, std::ios_base::openmode) override;

private:
    I m_text_file_impl;
    base::SeedMemoryBuffer<C> m_buffer;
    C* m_base;
    bool m_dynamic_eof;
    bool m_reading = false;
    bool m_writing = false;

    bool do_sync(std::error_code&);
    bool discard(std::error_code&);
    void advance();
    bool flush(std::error_code&);
    bool shallow_flush(std::error_code&);
};


template<class C, class T = std::char_traits<C>>
using BasicPosixTextFileStreambuf = BasicTextFileStreambuf<C, T, base::PosixTextFileImpl<C, T>>;

template<class C, class T = std::char_traits<C>>
using BasicWindowsTextFileStreambuf = BasicTextFileStreambuf<C, T, base::WindowsTextFileImpl<C, T>>;

using TextFileStreambuf        = BasicTextFileStreambuf<char>;
using PosixTextFileStreambuf   = BasicPosixTextFileStreambuf<char>;
using WindowsTextFileStreambuf = BasicWindowsTextFileStreambuf<char>;

using WideTextFileStreambuf        = BasicTextFileStreambuf<wchar_t>;
using WidePosixTextFileStreambuf   = BasicPosixTextFileStreambuf<wchar_t>;
using WideWindowsTextFileStreambuf = BasicWindowsTextFileStreambuf<wchar_t>;


template<class C, class T, class I> struct BasicTextFileStreambuf<C, T, I>::Config :
        public I::Config {
    static constexpr std::size_t default_buffer_size = 4096;

    bool dynamic_eof = false;

    // GENERIC:
    //
    // Buffer will be automatically expanded if necessary.              
    //
    std::size_t buffer_size = default_buffer_size;
    base::Span<C> buffer_memory;
};








// Implementation


// ============================ BasicTextFileStream ============================


template<class C, class T, class I>
inline BasicTextFileStream<C, T, I>::BasicTextFileStream(base::FilesystemPathRef path, Mode mode) :
    BasicTextFileStream(path, mode, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicTextFileStream<C, T, I>::BasicTextFileStream(base::FilesystemPathRef path, Mode mode,
                                                         Config config) :
    std::basic_iostream<C, T>(nullptr), // Throws
    m_file(path, mode), // Throws
    m_streambuf(m_file, this->getloc(), std::move(config)) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}



// ============================ BasicTextFileStreambuf ============================


template<class C, class T, class I>
inline BasicTextFileStreambuf<C, T, I>::BasicTextFileStreambuf(base::File& file) :
    BasicTextFileStreambuf(file, {}, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicTextFileStreambuf<C, T, I>::BasicTextFileStreambuf(base::File& file,
                                                               std::locale locale, Config config) :
    m_text_file_impl(file, std::move(locale), std::move(config)), // Throws
    m_buffer(config.buffer_memory, config.buffer_size), // Throws
    m_dynamic_eof(config.dynamic_eof)
{
    // Buffer must not be empty
    m_buffer.reserve(1); // Throws

    // Start out in neutral mode
    m_base = m_buffer.data();
    this->setg(m_base, m_base, m_base);
    this->setp(m_base, m_base);
}


template<class C, class T, class I>
void BasicTextFileStreambuf<C, T, I>::imbue(const std::locale& loc)
{
    // Ideally, this function should not have had any failure modes, because it
    // is called from `std::basic_ios<C, T>::imbue()`, and after that function
    // has made changes that are not guaranteed to be reversible. But,
    // eliminating all failure modes from this function is essentially
    // impossible. Ignoring practical limitations, one resolution would be to
    // modify the C++ standard by introducing a two-phase commit protocol for
    // changing the locale in a stream.

    std::error_code ec;
    if (ARCHON_LIKELY(do_sync(ec))) {
        ARCHON_ASSERT(!m_reading);
        ARCHON_ASSERT(!m_writing);
        std::mbstate_t state = {}; // Best guess
        m_text_file_impl.imbue(loc, state); // Throws
        return;
    }
    throw std::system_error(ec, "Failed to synchronize");
}


template<class C, class T, class I> auto BasicTextFileStreambuf<C, T, I>::underflow() -> int_type
{
    ARCHON_ASSERT(this->gptr() == this->egptr());

    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(!m_writing)) {
        m_text_file_impl.advance();
        m_base = m_buffer.data();
        this->setg(m_base, m_base, m_base);

      read:
        // Enter into reading mode
        m_reading = true;

        std::size_t n = 0;
        if (m_text_file_impl.read_ahead(m_buffer, m_dynamic_eof, n, ec)) { // Throws
            if (ARCHON_LIKELY(n > 0)) {
                this->setg(m_base, m_base, m_base + n);
                C ch = *this->gptr(); // Throws
                return T::to_int_type(ch);
            }
            return T::eof(); // End of file
        }
        return T::eof(); // Failure to read
    }

    if (ARCHON_LIKELY(flush(ec))) // Throws
        goto read;

    return T::eof(); // Failure to stop writing
}


template<class C, class T, class I>
auto BasicTextFileStreambuf<C, T, I>::overflow(int_type ch) -> int_type
{
    ARCHON_ASSERT(this->pptr() == this->epptr());

    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(m_writing)) {
        if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
          write_1:
            if (!T::eq_int_type(ch, T::eof())) {
                ARCHON_ASSERT(this->pptr() < this->epptr());
                *this->pptr() = T::to_char_type(ch); // Throws
                this->pbump(1); // Throws
            }
            return T::not_eof(ch); // Success
        }
        return T::eof(); // Failure to flush
    }

    if (ARCHON_LIKELY(!m_reading)) {
      write_2:
        // Enter into writing mode
        ARCHON_ASSERT(m_buffer.size() > 0);
        this->setp(m_base, m_base + m_buffer.size());
        m_writing = true;

        goto write_1;
    }

    if (ARCHON_LIKELY(discard(ec))) // Throws
        goto write_2;

    return T::eof(); // Failure to stop reading
}


template<class C, class T, class I> int BasicTextFileStreambuf<C, T, I>::sync()
{
    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(do_sync(ec))) // Throws
        return 0; // Success
    return -1; // Failure
}


template<class C, class T, class I>
auto BasicTextFileStreambuf<C, T, I>::seekoff(off_type off, std::ios_base::seekdir dir,
                                              std::ios_base::openmode) -> pos_type
{
    if (ARCHON_LIKELY(off == 0 && dir == std::ios_base::cur)) {
        pos_type pos = off_type(0);
        std::error_code ec; // Dummy
        if (ARCHON_LIKELY(m_writing)) {
            if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
                if (ARCHON_LIKELY(m_text_file_impl.tell_write(pos, ec))) // Throws
                    return pos; // Success
            }
        }
        else {
            advance(); // Throws
            if (ARCHON_LIKELY(m_text_file_impl.tell_read(pos, ec))) // Throws
                return pos; // Success
        }
    }
    return pos_type(off_type(-1)); // Failure
}


template<class C, class T, class I>
auto BasicTextFileStreambuf<C, T, I>::seekpos(pos_type pos, std::ios_base::openmode) -> pos_type
{
    std::error_code ec; // Dummy
    if (ARCHON_LIKELY(!m_writing)) {
      seek:
        if (ARCHON_LIKELY(m_text_file_impl.seek(pos, ec))) { // Throws
            // Enter into neutral mode (or stay there)
            m_base = m_buffer.data();
            this->setg(m_base, m_base, m_base);
            m_reading = false;
            return pos; // Success
        }
    }
    else {
        if (ARCHON_LIKELY(flush(ec))) // Throws
            goto seek;
    }
    return pos_type(off_type(-1)); // Failure
}


template<class C, class T, class I>
inline bool BasicTextFileStreambuf<C, T, I>::do_sync(std::error_code& ec)
{
    if (ARCHON_LIKELY(m_writing))
        return flush(ec); // Throws
    if (ARCHON_LIKELY(!m_reading))
        return true;
    return discard(ec); // Throws
}


template<class C, class T, class I> bool BasicTextFileStreambuf<C, T, I>::discard(std::error_code& ec)
{
    advance(); // Throws
    if (ARCHON_LIKELY(m_text_file_impl.discard(ec))) { // Throws
        // Enter into neutral mode (or stay there)
        m_base = m_buffer.data();
        this->setg(m_base, m_base, m_base);
        m_reading = false;
        return true;
    }
    return false;
}


template<class C, class T, class I> inline void BasicTextFileStreambuf<C, T, I>::advance()
{
    ARCHON_ASSERT(!m_writing);
    ARCHON_ASSERT(this->gptr() >= m_base);
    std::size_t n = std::size_t(this->gptr() - m_base);
    m_text_file_impl.advance(n); // Throws
    m_base = this->gptr();
}


template<class C, class T, class I>
inline bool BasicTextFileStreambuf<C, T, I>::flush(std::error_code& ec)
{
    if (ARCHON_LIKELY(shallow_flush(ec))) { // Throws
        if (ARCHON_LIKELY(m_text_file_impl.flush(ec))) { // Throws
            // Enter into neutral mode
            m_base = m_buffer.data();
            this->setp(m_base, m_base);
            m_writing = false;
            return true;
        }
    }
    return false; // Failure
}


template<class C, class T, class I>
inline bool BasicTextFileStreambuf<C, T, I>::shallow_flush(std::error_code& ec)
{
    ARCHON_ASSERT(m_writing);
    ARCHON_ASSERT(this->pptr() >= m_base);
    base::Span data = base::Span<C>(m_base, this->pptr());
    std::size_t n = 0;
    if (ARCHON_LIKELY(m_text_file_impl.write(data, n, ec))) { // Throws
        ARCHON_ASSERT(m_buffer.size() > 0);
        m_base = m_buffer.data();
        this->setp(m_base, m_base + m_buffer.size());
        return true;
    }
    m_base += n;
    return false;
}


} // namespace archon::base













using namespace archon;





// ============================================================================================ test_text_codec.cpp ============================================================================================


static_assert(std::is_empty_v<base::TextCodec>);
static_assert(std::is_empty_v<base::PosixTextCodec>);
static_assert(std::is_empty_v<base::WindowsTextCodec>);

static_assert(!std::is_empty_v<base::WideTextCodec>);
static_assert(!std::is_empty_v<base::WidePosixTextCodec>);
static_assert(!std::is_empty_v<base::WideWindowsTextCodec>);

static_assert(!base::PosixTextCodec::is_nontrivial);
static_assert(base::WindowsTextCodec::is_nontrivial);
static_assert(base::WidePosixTextCodec::is_nontrivial);
static_assert(base::WideWindowsTextCodec::is_nontrivial);


ARCHON_TEST_VARIANTS(codec_variants,
                     ARCHON_TEST_TYPE(base::PosixTextCodec,       Posix),
                     ARCHON_TEST_TYPE(base::WindowsTextCodec,     Windows),
                     ARCHON_TEST_TYPE(base::WidePosixTextCodec,   WidePosix),
                     ARCHON_TEST_TYPE(base::WideWindowsTextCodec, WideWindows));


ARCHON_TEST_BATCH(Base_TextCodec_Decode, codec_variants)
{
    const std::locale& locale = test_context.get_locale();
    using text_codec_type = test_type;
    text_codec_type codec(locale);
    using char_type = typename text_codec_type::char_type;
    base::SeedMemoryBuffer<char_type> buffer;
    std::basic_string_view<char_type> result = codec.decode("foo", buffer);
    std::array<char_type, 64> seed_memory;
    base::BasicStringWidener<char_type> widener(locale, seed_memory);
    ARCHON_CHECK_EQUAL(result, widener.widen("foo"));
}


ARCHON_TEST_BATCH(Base_TextCodec_Encode, codec_variants)
{
    const std::locale& locale = test_context.get_locale();
    using text_codec_type = test_type;
    text_codec_type codec(locale);
    base::SeedMemoryBuffer<char> buffer;
    using char_type = typename text_codec_type::char_type;
    std::array<char_type, 64> seed_memory;
    base::BasicStringWidener<char_type> widener(locale, seed_memory);
    std::string_view result = codec.encode(widener.widen("foo"), buffer);
    ARCHON_CHECK_EQUAL(result, "foo");
}




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
    const std::locale& locale = test_context.get_locale();
    base::TextFileImplConfig config;
    config.char_codec_buffer_size = 16;
    config.newline_codec_buffer_size = 16;
    base::WindowsTextFileImpl<wchar_t> text_file_impl(file, locale, std::move(config));
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


/*
namespace {


class FindCodecErrorLocale {
public:
    const char* decode_error_locale = nullptr;
    const char* encode_error_locale = nullptr;

    FindCodecErrorLocale()
    {
        // Find decode error locale
        for (const char* name : candidates) {
            if (base::has_locale(name)) {
                std::locale locale(name);
                
            }
    }

private:
    const char* candidates[] = { "C", "C.UTF-8", ".UTF8", "en_US", "en_US.UTF-8", "" };
};

const FindCodecErrorLocale g_find_codec_error_locale;

inline bool has_decode_error_locale()
{
    retirn (g_find_codec_error_locale.decode_error_locale != nullptr);
}

inline bool has_encode_error_locale()
{
    retirn (g_find_codec_error_locale.encode_error_locale != nullptr);
}


// unnamed namespace
*/


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


const char* candidate_locales[] = { "C", "C.UTF-8", ".UTF8", "en_US", "en_US.UTF-8", "" };


ARCHON_TEST(Base_TextFile_AsciiCodecError_CHECK)                       
{
    // Both macOS and Windows accept byte values outside 0 -> 127 when decoding in C locale.
    // In UTF-8 locale, on bad byte while decoding, macOS returns partial and leaves `from_next` to point at the bad char.
    //  -----> Is this behavior also borne out in `codecvt-test-cases` branch? YES                    
    //  -----> What are the consequenses of this quirk from the point of view of the codec implementation above?      

    // libstdc++ has quirk in that it reports errors differently depending on whether it has enough context to see the error. If it sees only one byte, and it is good, it consumes it even though the next byte might be bad. On the other hand, if it sees both bytes at once, it does not consume anything.

    // No quirks in behavior std::codecvt found on Windows

    // Still need to check behavior on macOS when amount of input starting from beginning of bad byte seq is greater than, or equal to max_length(). Done. Nothing changes.

    // It seams that, at least on macOS, when decoding result is partial, it is necessary to check whether the remaining amount of presented input was greater than, or equal to max_length(), and if it was, treat the situation as an error.           

    // STRATEGY: Individually for decode and encode, run through list of candicate locales and look for one where char(-1) cannot be decoded or wchar_t(-1) cannot be encoded.      



    using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
    auto test_decode = [](const std::locale& locale) {
        const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);
        char data[] = { 'x', char(-1), 'x' };
        base::Span data_2(data);
        std::array<wchar_t, 8> buffer;
        std::mbstate_t state = {};
        const char* from     = data_2.data();
        const char* from_end = from + data_2.size();
        const char* from_next;
        wchar_t* to     = buffer.data();
        wchar_t* to_end = to + buffer.size();
        wchar_t* to_next;
        auto result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        if (result == std::codecvt_base::error)
            return 'e';
        if (result == std::codecvt_base::ok && from_next == from + 1)
            return 'K';
        if (result == std::codecvt_base::ok && from_next > from + 1)
            return 'L';
        if (result == std::codecvt_base::partial && from_next == from + 1)
            return 'p';
        if (result == std::codecvt_base::partial && from_next == from + 2)
            return 'q';
        if (result == std::codecvt_base::partial && from_next > from + 2)
            return 'r';
        return 'W';
    };
    auto test_encode = [](const std::locale& locale) {
        const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);
        wchar_t data[] = { L'x', wchar_t(-1), L'x' };
        base::Span data_2(data);
        std::array<char, 8> buffer;
        std::mbstate_t state = {};
        const wchar_t* from     = data_2.data();
        const wchar_t* from_end = from + data_2.size();
        const wchar_t* from_next = nullptr;
        char* to     = buffer.data();
        char* to_end = to + buffer.size();
        char* to_next = nullptr;
        auto result = codecvt.out(state, from, from_end, from_next, to, to_end, to_next);
        if (result == std::codecvt_base::error)
            return 'e';
        if (result == std::codecvt_base::ok && from_next == from + 1)
            return 'K';
        if (result == std::codecvt_base::ok && from_next > from + 1)
            return 'L';
        if (result == std::codecvt_base::partial && from_next == from + 1)
            return 'p';
        if (result == std::codecvt_base::partial && from_next > from + 1)
            return 'q';
        return 'W';
    };

    for (const char* name : candidate_locales) {
        if (base::has_locale(name)) {
            std::locale locale(name);
            char decode = test_decode(locale);
            char encode = test_encode(locale);
            log("has 1     decode %s     encode %s    %s", decode, encode, locale.name());
        }
        else {
            log("has 0                              %s", name);
        }
    }
}





// ============================================================================================ test_text_file_stream.cpp ============================================================================================


ARCHON_TEST_VARIANTS(stream_variants,
                     ARCHON_TEST_TYPE(base::PosixTextFileStream,       Posix),
                     ARCHON_TEST_TYPE(base::WindowsTextFileStream,     Windows),
                     ARCHON_TEST_TYPE(base::WidePosixTextFileStream,   WidePosix),
                     ARCHON_TEST_TYPE(base::WideWindowsTextFileStream, WideWindows));


ARCHON_TEST_BATCH(Base_TextFileStream_Read, stream_variants)
{
    ARCHON_TEST_FILE(path);
    {
        base::TextFile text_file(path, base::File::Mode::write);
        text_file.write(std::string_view("4689"));
        text_file.flush();
    }
    using stream_type = test_type;
    typename stream_type::Config config;
    config.buffer_size = 3;
    config.char_codec_buffer_size = 3;
    config.newline_codec_buffer_size = 3;
    stream_type stream(path, base::File::Mode::read, std::move(config));
    stream.imbue(std::locale::classic());
    ARCHON_CHECK(stream);
    int value = 0;
    stream >> value;
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(value, 4689);
}


ARCHON_TEST_BATCH(Base_TextFileStream_WriteAndFlush, stream_variants)
{
    ARCHON_TEST_FILE(path);
    {
        using stream_type = test_type;
        typename stream_type::Config config;
        config.buffer_size = 3;
        config.char_codec_buffer_size = 3;
        config.newline_codec_buffer_size = 3;
        stream_type stream(path, base::File::Mode::write, std::move(config));
        stream.imbue(std::locale::classic());
        ARCHON_CHECK(stream);
        stream << 4689;
        ARCHON_CHECK(stream);
        stream.flush();
        ARCHON_CHECK(stream);
    }
    base::TextFile text_file(path, base::File::Mode::read);
    std::array<char, 64> buffer;
    std::size_t n = text_file.read(buffer);
    std::string_view data(buffer.data(), n);
    ARCHON_CHECK_EQUAL(data, "4689");
}


ARCHON_TEST_BATCH(Base_TextFileStream_TellAndSeek, stream_variants)
{
    ARCHON_TEST_FILE(path);
    using stream_type = test_type;
    typename stream_type::Config config;
    config.buffer_size = 3;
    config.char_codec_buffer_size = 3;
    config.newline_codec_buffer_size = 3;
    stream_type stream(path, base::File::Mode::write, std::move(config));
    stream.imbue(std::locale::classic());
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 0);
    stream << 4689;
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 4);
    stream.flush();
    ARCHON_CHECK(stream);
    stream.seekg(0);
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(stream.tellp(), 0);
    int value = 0;
    stream >> value;
    ARCHON_CHECK(stream);
    ARCHON_CHECK_EQUAL(value, 4689);
    ARCHON_CHECK_EQUAL(stream.tellp(), 4);
}


/*
Decoding cases:
- Complete cases:
  - no input, no output space (expect ok)
  - no input, plenty of output space (expect ok)
  - end of input after one valid single-byte char, no output space (expect ??)        
  - end of input after one valid single-byte char, only enough output space for one character (expect ok)
  - end of input after one valid single-byte char, plenty of output space (expect ok)
  - end of input after one valid multi-byte char, no output space (UTF-8) (expect ??)        
  - end of input after one valid multi-byte char, only enough output space for one character (UTF-8) (expect ok)
  - end of input after one valid multi-byte char, plenty of output space (UTF-8) (expect ok)
- Partial cases:
  - end of input after 1st byte of multi-byte char, no output space (UTF-8)
  - end of input after 1st byte of multi-byte char, plenty of output space (UTF-8)
  - end of input after complete char followed by 1st byte of multi-byte char, no output space (UTF-8)
  - end of input after complete char followed by 1st byte of multi-byte char, only enough output space for one character (UTF-8)
  - end of input after complete char followed by 1st byte of multi-byte char, plenty of output space (UTF-8)
- Bad byte cases:
  - 1st byte of 1st character is a bad byte, no output space (UTF-8)
  - 1st byte of 1st character is a bad byte, plenty of output space (UTF-8)
  - 2nd byte of 1st character is a bad byte, no output space (UTF-8)
  - 2nd byte of 1st character is a bad byte, plenty of output space (UTF-8)
  - 1st byte of 2nd character is a bad byte, no output space (UTF-8)
  - 1st byte of 2nd character is a bad byte, only enough output space for one character (UTF-8)
  - 1st byte of 2nd character is a bad byte, plenty of output space (UTF-8)
  - 2nd byte of 2nd character is a bad byte, no output space (UTF-8)
  - 2nd byte of 2nd character is a bad byte, only enough output space for one character (UTF-8)
  - 2nd byte of 2nd character is a bad byte, plenty of output space (UTF-8)
*/


#include <archon/base/quote.hpp>

namespace archon::base::detail {

#if ARCHON_GNU_LIBCXX
inline constexpr bool codecvt_quirk_ok_on_empty_buffer = true;
#else
inline constexpr bool codecvt_quirk_ok_on_empty_buffer = false;
#endif

#if ARCHON_GNU_LIBCXX || ARCHON_LLVM_LIBCXX
inline constexpr bool codecvt_quirk_ok_on_partial_char = true;
#else
inline constexpr bool codecvt_quirk_ok_on_partial_char = false;
#endif

#if ARCHON_GNU_LIBCXX || ARCHON_LLVM_LIBCXX
inline constexpr bool codecvt_quirk_consume_partial_char = true;
#else
inline constexpr bool codecvt_quirk_consume_partial_char = false;
#endif

#if ARCHON_GNU_LIBCXX
inline constexpr bool codecvt_quirk_consume_partial_char_makes_noninit_state = true;
#else
inline constexpr bool codecvt_quirk_consume_partial_char_makes_noninit_state = false;
#endif

} // namespace archon::base::detail


ARCHON_TEST(CodecvtDecodeBaseline)
{
    std::array<char, 16> seed_memory;
    base::StringFormatter formatter(seed_memory, test_context.get_locale());
    base::ArraySeededBuffer<wchar_t, 10> buffer;
    auto subtest = [&, &parent_test_context = test_context](const std::locale& locale) {
        ARCHON_TEST_TRAIL(parent_test_context,
                          formatter.format("%s", base::quoted(std::string_view(locale.name()))));
        bool is_utf8 = base::assume_utf8_locale(locale);
        using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
        const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);
        auto subtest = [&, &parent_test_context =
                        test_context](std::string_view data, std::size_t buffer_size,
                                      std::size_t expected_from_advance,
                                      std::size_t expected_to_advance,
                                      std::codecvt_base::result expected_result,
                                      bool expect_final_empty_state) {
            ARCHON_TEST_TRAIL(parent_test_context,
                              formatter.format("%s, %s", base::quoted(data), buffer_size));
            buffer.reserve(buffer_size);
            std::mbstate_t state = {};
            const char* from = data.data();
            const char* from_end = from + data.size();
            const char* from_next = nullptr;
            wchar_t* to = buffer.data();
            wchar_t* to_end = to + buffer_size;
            wchar_t* to_next = nullptr;
            std::codecvt_base::result result =
                codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
            ARCHON_CHECK_EQUAL(result, expected_result);
            ARCHON_CHECK_EQUAL(from_next - from, expected_from_advance);
            ARCHON_CHECK_EQUAL(to_next - to,     expected_to_advance);
            ARCHON_CHECK_EQUAL(std::mbsinit(&state) != 0, expect_final_empty_state);
        };

        bool quirk1 = base::detail::codecvt_quirk_ok_on_empty_buffer;
        bool quirk2 = base::detail::codecvt_quirk_ok_on_partial_char;
        bool quirk3 = base::detail::codecvt_quirk_consume_partial_char;
        bool quirk4 = base::detail::codecvt_quirk_consume_partial_char_makes_noninit_state;

        std::codecvt_base::result ok      = std::codecvt_base::ok;
        std::codecvt_base::result partial = std::codecvt_base::partial;
        std::codecvt_base::result error   = std::codecvt_base::error;
        std::codecvt_base::result quirk_1_result = (quirk1 ? ok : partial);
        std::codecvt_base::result quirk_2_result = (quirk2 ? ok : partial);

        subtest("",    0, 0, 0, ok, true);
        subtest("",   10, 0, 0, ok, true);

        subtest("x",   0, 0, 0, quirk_1_result, true);
        subtest("x",   1, 1, 1, ok,             true);
        subtest("x",  10, 1, 1, ok,             true);

        subtest("xx",  0, 0, 0, quirk_1_result, true);
        subtest("xx",  1, 1, 1, partial,        true);
        subtest("xx",  2, 2, 2, ok,             true);
        subtest("xx", 10, 2, 2, ok,             true);

        if (is_utf8) {
            subtest("\xC3\xA6",   0, 0, 0, quirk_1_result, true);
            subtest("\xC3\xA6",   1, 2, 1, ok,             true);
            subtest("\xC3\xA6",  10, 2, 1, ok,             true);

            subtest("x\xC3\xA6",  0, 0, 0, quirk_1_result, true);
            subtest("x\xC3\xA6",  1, 1, 1, partial,        true);
            subtest("x\xC3\xA6",  2, 3, 2, ok,             true);
            subtest("x\xC3\xA6", 10, 3, 2, ok,             true);

            subtest("\xC3",   0, 0,                0, quirk_1_result, true);
            subtest("\xC3",   1, (quirk3 ? 1 : 0), 0, quirk_2_result, !quirk4);
            subtest("\xC3",  10, (quirk3 ? 1 : 0), 0, quirk_2_result, !quirk4);

            subtest("x\xC3",  0, 0,                0, quirk_1_result, true);
            subtest("x\xC3",  1, 1,                1, partial,        true);
            subtest("x\xC3",  2, (quirk3 ? 2 : 1), 1, quirk_2_result, !quirk4);
            subtest("x\xC3", 10, (quirk3 ? 2 : 1), 1, quirk_2_result, !quirk4);

            // 1st byte of 1st char is bad
            subtest("\xA6",   0, 0, 0, quirk_1_result, true);
            subtest("\xA6",   1, 0, 0, error,          true);
            subtest("\xA6",  10, 0, 0, error,          true);

            // 2nd byte of 1st char is bad
            subtest("\xC3x",  0, 0, 0, quirk_1_result, true);
            subtest("\xC3x",  1, 0, 0, error,          true);
            subtest("\xC3x", 10, 0, 0, error,          true);

            // 1st byte of 2nd char is bad
            subtest("x\xA6",  0, 0, 0, quirk_1_result, true);
            subtest("x\xA6",  1, 1, 1, partial,        true);
            subtest("x\xA6",  2, 1, 1, error,          true);
            subtest("x\xA6", 10, 1, 1, error,          true);

            // 2nd byte of 2nd char is bad
            subtest("x\xC3x",  0, 0, 0, quirk_1_result, true);
            subtest("x\xC3x",  1, 1, 1, partial,        true);
            subtest("x\xC3x",  2, 1, 1, error,          true);
            subtest("x\xC3x", 10, 1, 1, error,          true);

            // Extra quirk with libstdc++: Leading valid bytes of invalid byte sequence are not consumed, but only when the invalid part is part of the presented input. This in spite of the fact that libstdc++ normally does consume partial byte sequences.
            // What about case where one valid leading byte is good and already consumed during previous invocation of std::codecvt::in(), but in the input presented to second invocation of std::codecvt::in(), there is one more good byte, and then a bad byte. Will the good byte be consumed? Probably not.

/*            
            subtest("\xC3", "x",  0, 0, 0, quirk_1_result, true);
            subtest("\xC3", "x",  1, 0, 0, error,          true);
            subtest("\xC3", "x", 10, 0, 0, error,          true);
*/            
        }
    };
    for (const char* name : candidate_locales) {
        if (base::has_locale(name)) {
            std::locale locale(name);
            subtest(locale);
        }
    }
}
