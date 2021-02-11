#include <streambuf>

#include <archon/base/file.hpp>
#include <archon/unit_test.hpp>


using namespace archon;




struct CharCodecResult {
    std::size_t data_advance;
    std::size_t buffer_advance;
    std::codecvt_base::result status;
};


template<class C, class T = std::char_traits<C>> class BasicCharCodec {
public:
    using char_type   = C;
    using traits_type = T;
    using string_view_type = std::basic_string_view<C, T>;

    // Throws on unencodable input when not in lenient mode
    base::Span<char> encode(string_view_type data, base::SeedMemoryBuffer<char>&);

    // Returns false on unencodable input when not in lenient mode
    bool try_encode(string_view_type data, base::SeedMemoryBuffer<char>&, std::size_t& size);

    auto inc_encode(std::mbstate_t&, const C*& data_begin, const C* data_end, bool flush,
                    char*& buffer_begin, char* buffer_end) -> std::codecvt_base::result;

private:
    const bool m_lenient;
};







template<class C, class T = std::char_traits<C>> class BasicTextCodec {
public:
    using char_type   = C;
    using traits_type = T;
    using string_view_type = std::basic_string_view<C, T>;

    // Empty class except on Windows
    template<std::size_t size> class InternalBuffer {
    };

    base::Span<char> encode(string_view_type data, base::Span<char> internal_buffer,  base::SeedMemoryBuffer<char>& target_buffer);

private:
    BasicCharCodec<C, T> m_char_codec;
};





/*
template<class C, class T> base::Span<char> BasicTextCodec<C, T>::encode(string_view_type data, base::Span<char> internal_buffer,  base::SeedMemoryBuffer<char>& target_buffer)
{
    std::mbstate_t state {};
    const C* data_begin = data.data();
    const C* data_end   = data_begin + data.size();
    std::codecvt_base::result result = inc_encode(state, data_begin, data_end, buffer_begin, buffer_end, ...);
}
*/



namespace detail {

class TextFile {
public:
    using offset_type = base::File::offset_type;

    TextFile(base::File& file, base::Span<char> seed_memory, std::size_t default_buffer_size = 1024) :
        m_file(file),
        m_buffer(seed_memory)
    {
        if (m_buffer.size() > 0) {
            // Implementation requires that the buffer size is at least 2.
            m_buffer.reserve(2); // Throws
        }
        else {
            // Allocate default buffer
            m_buffer.reserve(default_buffer_size); // Throws
        }
    }

    std::size_t read(base::Span<char> buffer) // no inline
    {
        if (ARCHON_UNLIKELY(m_writing))
            stop_writing(); // Throws

        // CR+LF -> NL
        char* buffer_end = m_buffer.data() + m_buffer.size();
        auto end_of_input = [&] {
            return (m_end != buffer_end);
        };
        char* begin = buffer.data();
        char* end   = begin + buffer.size();
      again_1:
        if (ARCHON_LIKELY(begin != end)) {
          again_2:
            if (ARCHON_LIKELY(m_begin != m_end)) {
                char ch = *m_begin++;
                if (ARCHON_LIKELY(ch != '\r')) {
                    *begin++ = ch;
                    goto again_1;
                }
                if (ARCHON_LIKELY(m_begin != m_end)) {
                    char ch_2 = *m_begin;
                    if (ARCHON_LIKELY(ch_2 == '\n')) {
                        ++m_begin;
                        *begin++ = '\n';
                        goto again_1;
                    }
                    *begin++ = '\r';
                    goto again_1;
                }
                if (!end_of_input()) {
                    m_begin = m_buffer.data();
                    m_end = m_begin;
                    *m_end++ = '\r';
                    goto read;
                }
                *begin++ = '\r';
                return std::size_t(begin - buffer.data());
            }
            if (!end_of_input()) {
                m_begin = m_buffer.data();
                m_end = m_begin;
              read:
                std::size_t n = m_file.read({ m_end, buffer_end }); // Throws
                m_end += n;
                goto again_2;
            }
            return std::size_t(begin - buffer.data());
        }
        return buffer.size();
    }

    void write(base::Span<const char> data) // no inline
    {
        if (ARCHON_UNLIKELY(!m_writing))
            start_writing(); // Throws

        // NL -> CR+LF
        const char* begin = data.data();
        const char* end = begin + data.size();
        while (ARCHON_LIKELY(begin != end)) {
            if (ARCHON_UNLIKELY(std::size_t(m_end - m_begin) < 2)) {
                m_file.write({ m_buffer.data(), m_begin }); // Throws
                m_begin = m_buffer.data();
            }
            char ch = *begin++;
            if (ARCHON_LIKELY(ch != '\n')) {
                *m_begin++ = ch;
                continue;
            }
            *m_begin++ = '\r';
            *m_begin++ = '\n';
        }
    }

    void flush()
    {
        if (ARCHON_LIKELY(m_writing)) {
            m_file.write({ m_buffer.data(), m_begin }); // Throws
            m_begin = m_buffer.data();
        }
    }

//    seek()                        

private:
    base::File& m_file;
    base::SeedMemoryBuffer<char> m_buffer;
    char* m_begin = nullptr;
    char* m_end   = nullptr;
    bool m_writing = false;

    void start_writing() // no inline
    {
        std::size_t n = std::size_t(m_end - m_begin);
        offset_type max = std::numeric_limits<offset_type>::max();
        // FIXME: Makes no sense to seek multiple times, because a file cannot be larger than max
        while (ARCHON_UNLIKELY(n > base::to_unsigned(max))) {
            m_file.seek(-max, base::File::Whence::cur); // Throws
            n -= std::size_t(max);
        }
        // FIXME: Avoid calling seek when n is zero, such that this is usable for output only on STDOUT       
        m_file.seek(-offset_type(n), base::File::Whence::cur); // Throws
        m_begin = m_buffer.data();
        m_end   = m_begin + m_buffer.size();
        m_writing = true;
    }

    void stop_writing() // no inline
    {
        m_file.write({ m_buffer.data(), m_begin }); // Throws
        m_begin = nullptr;
        m_end   = nullptr;
        m_writing = false;
    }
};

} // namespace detail




// When reading from file, use try_read_some()       


// Consider introducing BasicTextFile<C, T, F>, and then build BasicTextFileStreambuf<C, T, F> on top of it. Here F is the underlying file type, which is base::PrimitiveTextFile by default. base::PrimitiveTextFile is an alias for base::PrimitivePosixTextFile on POSIX, and an alias for base::PrimitiveWindowsTextFile on Windows. base::PrimitiveWindowsTextFile is buffered. base::PrimitivePosixTextFile is unbuffered.                                 



template<class C, class T = std::char_traits<C>> class BasicTextFileStreambuf : std::basic_streambuf<C, T> {
public:
    using char_type   = C;
    using traits_type = T;
    using int_type    = typename T::int_type;

    int_type overflow(int_type ch) override final
    {
        if (ARCHON_LIKELY(encode())) { // Throws
            if (!T::eq_int_type(ch, T::eof())) {
                *this->pptr() = T::to_char_type(ch); // Throws
                this->pbump(1); // Throws
            }
            return T::not_eof(ch); // Success
        }
        return T::eof(); // Failure to encode
    }

    // Do unshift as part of flush  

private:
    std::locale m_locale;
    base::File& m_file;
    base::SeedMemoryBuffer<C> m_int_buf;
    base::SeedMemoryBuffer<char> m_ext_buf;
    C* m_int_begin = this->pptr();
    char* m_ext_curr  = nullptr;
    BasicCharCodec<C, T> m_char_codec;
    std::mbstate_t m_state;
    bool m_writing = false;

    void encode()
    {
        const C* data_begin = m_int_begin;
        const C* data_end   = this->pptr();
        while (data_begin != data_end) {
            bool flush = false;
            char* buffer_begin = m_ext_curr;
            char* buffer_end   = m_ext_buf.data() + m_ext_buf.size();
            std::codecvt_base::result result =
                m_char_codec.inc_encode(m_state, data_begin, data_end, flush,
                                        buffer_begin, buffer_end); // Throws
            switch (result) {
                case std::codecvt_base::ok:      // All specified input has been converted, unshift is still required
                    m_int_begin = m_int_buf.data();
                    m_ext_curr = buffer_begin;
                    setp(m_int_begin, m_int_buf.data() + m_int_buf.size()); // Throws
                    return;
                case std::codecvt_base::partial: // Need more space in output buffer
                    m_int_begin += data_begin - m_int_begin;
                    m_ext_curr = buffer_begin;
                    write(); // Throws
                    continue;
                case std::codecvt_base::error:
                    break;             
                case std::codecvt_base::noconv:
                    break;             
            }
            ARCHON_ASSERT_UNREACHABLE;
        }
    }

    void write()
    {
        ARCHON_ASSERT_UNREACHABLE;                  
    }
};




template<class C, class T = std::char_traits<C>> class BasicTextFileStream :
        public std::basic_iostream<C, T> {
public:
    using char_type   = C;
    using traits_type = T;

    BasicTextFileStream(base::File);

private:
    base::File m_file;
    BasicTextFileStreambuf<C, T> m_streambuf;
};


using TextFileStream     = BasicTextFileStream<char>;
using WideTextFileStream = BasicTextFileStream<wchar_t>;



template<class C, class T> inline BasicTextFileStream<C, T>::BasicTextFileStream(base::File file) :
    std::basic_iostream<C, T>(nullptr), // Throws
    m_file(std::move(file)),
    m_streambuf(m_file) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}






ARCHON_TEST(Bar)
{
    base::File file;
//    WideTextFileStream stream(file);
}
