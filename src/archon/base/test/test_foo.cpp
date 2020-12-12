#include <string>
#include <locale>
#include <ios>
#include <iostream>

#include <archon/base/span.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/file.hpp>
#include <archon/base/file_stream.hpp>

#include <archon/unit_test.hpp>


using namespace archon;


/*
template<class C, class T = std::char_traits<C>> class BasicCharCodec {
public:
    using char_type   = C;
    using traits_type = T;
    using string_view_type = std::basic_string_view<C, T>;
    using codecvt_type     = std::codecvt<C, char, std::mbstate_t>;

    BasicCharCodec() :
        m_locale(), // I.e., copy of global locale
        m_codecvt(std::use_facet<codecvt_type>(m_locale)) // Throws
    {
    }
    BasicCharCodec(const codecvt_type& codecvt) noexcept :
        m_locale(), // Unused in this case
        m_codecvt(codecvt)
    {
    }
    BasicCharCodec(const std::locale& locale) :
        m_locale(locale),
        m_codecvt(std::use_facet<codecvt_type>(m_locale)) // Throws
    {
    }
    BasicCharCodec(const std::ios_base& ios) :
        m_locale(ios.getloc()),
        m_codecvt(std::use_facet<codecvt_type>(m_locale)) // Throws
    {
    }

    base::Span<const char> encode(string_view_type string, base::SeedMemoryBuffer<char>&);

    bool try_encode(string_view_type string, base::Span<const char>, base::SeedMemoryBuffer<char>&);

    class State;

    bool inc_encode(State&, const C*& data_begin, const C* data_end, bool flush,
                    char*& buffer_begin, char* buffer_end) noexcept;

private:
    const std::locale m_locale;
    const codecvt_type& m_codecvt;
};


using CharCodec     = BasicCharCodec<char>;
using WideCharCodec = BasicCharCodec<wchar_t>;




template<class C, class T = std::char_traits<C>> class BasicFileOutputStreambuf :
    public std::basic_streambuf<C, T> {
public:
    using char_type    = C;
    using traits_type  = T;
    using int_type     = typename T::int_type;

    BasicFileOutputStreambuf(base::File& file) noexcept :
        m_file(file)
    {
    }

protected:
    int_type overflow(int_type ch) override
    {
        encode();
        static_cast<void>(ch);
        std::cout << "***CLICK***\n";
        return T::eof();
    }

private:
    base::File& m_file;

    void encode()
    {
        for (;;) {
            // encode
            // If  out of input, break
            // Flush
        }
    }
};


using FileOutputStreambuf     = BasicFileOutputStreambuf<char>;
using WideFileOutputStreambuf = BasicFileOutputStreambuf<wchar_t>;
*/






// QUESTION: If I open a file stream for both reading and writing, and I read just one character, what is the file position then?



// Streambuf must maintain buffer for reading from external source (512 chars).
// Streambuf can use single buffer of internal character type for both reading and writing. When reading, first flush any remaing unflushed data from prior write. When writing, discard any unread data that was decoded during an earlier read (256 wchar_t).
// Streambuf must contain dedicated buffer for output from encoding during writing (512 chars).


// Newline translation?    
//   Output: 
//   Input: 

// Shortcuts for trivial encoding?   

// Replacement character?   



// BasicFileInputStreambuf
//   - BasicCharCodec
//   - BasicCharCodec::State
// BasicFileOutputStreambuf
//   - BasicCharCodec
//   - BasicCharCodec::State

// BasicFileInputStream
// BasicFileOutputStream


/*
ARCHON_TEST(Foo)
{
    base::File file;
    FileOutputStreambuf streambuf(file);
    std::ostream out(&streambuf);
    std::cout << out.good() << ", " << out.eof() << ", " << out.fail() << ", " << out.bad() << "\n";
    out << "Hej\n";
    std::cout << out.good() << ", " << out.eof() << ", " << out.fail() << ", " << out.bad() << "\n";
}
*/


ARCHON_TEST(Foo)
{
    std::locale locale("");
    base::WideFileStream stream(base::make_fs_path_generic("/tmp/foo", locale));
//    std::fstream stream(base::make_fs_path_generic("/tmp/foo", locale));
    stream.imbue(locale);
    log_info("bool = %s", bool(stream));
//    log_info("read pos = %s", stream.tellg());

    int a = 0, b = 0;
    stream >> a >> b;
    log_info("bool = %s", bool(stream));
    log_info("fail = %s", stream.fail());
    log_info("bad = %s", stream.bad());
    log_info("eof = %s", stream.eof());
    log_info("good = %s", stream.good());
    log_info("n = %s", stream.gcount());
//    log_info("read pos = %s", stream.tellg());
    log_info("a = %s, b = %s", a, b);

    wchar_t c = 0;
    stream >> c;
    log_info("bool = %s", bool(stream));
    log_info("fail = %s", stream.fail());
    log_info("bad = %s", stream.bad());
    log_info("eof = %s", stream.eof());
    log_info("good = %s", stream.good());
    log_info("n = %s", stream.gcount());
//    log_info("read pos = %s", stream.tellg());
    log_info("c = %s", std::char_traits<wchar_t>::to_int_type(c));

/*
    std::array<char, 1024> buffer;
    stream.read(buffer.data(), buffer.size());
    log_info("bool = %s", bool(stream));
    log_info("fail = %s", stream.fail());
    log_info("bad = %s", stream.bad());
    log_info("eof = %s", stream.eof());
    log_info("good = %s", stream.good());
    log_info("n = %s", stream.gcount());
*/

/*
    stream << "foo";
    log_info("bool = %s", bool(stream));
*/
}
