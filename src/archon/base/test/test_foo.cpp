#include <locale>
#include <iostream>
#include <fstream>

#include <archon/unit_test.hpp>


namespace {

template<class T, std::size_t n> class Array {
public:
    T* data()
    {
        return m_array;
    }
    const T* data() const
    {
        return m_array;
    }
    static std::size_t size()
    {
        return n;
    }
private:
    T m_array[n == 0 ? 1: n];
};

template<class C, class T = std::char_traits<C> > class BasicStringView {
public:
    BasicStringView(const C* c_str) :
        m_data(c_str),
        m_size(T::length(c_str))
    {
    }
    const C* data() const
    {
        return m_data;
    }
    std::size_t size() const
    {
        return m_size;
    }
private:
    const C* m_data;
    std::size_t m_size;
};

typedef BasicStringView<char> StringView;
typedef BasicStringView<wchar_t> WideStringView;

bool is_initial_2(const std::mbstate_t& state)
{
    std::mbstate_t initial = std::mbstate_t();
    return (std::memcmp(&state, &initial, sizeof (std::mbstate_t)) == 0);
}


} // unnamed namespace


ARCHON_TEST(Foo)
{
    std::locale locale("");
    std::cout << locale.name() << "\n";
    typedef std::codecvt<wchar_t, char, std::mbstate_t> codecvt_type;
    typedef std::codecvt_base::result result_type;
    const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);
    std::cout << "encoding   = " << codecvt.encoding() << "\n";
    std::cout << "max_length = " << codecvt.max_length() << "\n";
    std::cout << "---------------------------------- A ----------------------------------\n";
    // Trivial decoding with and without enough output buffer space
    {
        StringView string = "xx";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        bool was_ok_1, was_ok_2;
        std::cout << "------------ A01 ------------\n";
        {
            std::mbstate_t state = std::mbstate_t();
            Array<wchar_t, 256> buffer;
            wchar_t* to      = buffer.data();
            wchar_t* to_end  = buffer.data() + buffer.size();
            wchar_t* to_next = 0;
            result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
            std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
            std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
            std::cout << "input consumed  = " << (from_next - from) << "\n";
            std::cout << "output produced = " << (to_next - to) << "\n";
            std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
            std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
            was_ok_1 = (result == std::codecvt_base::ok);
        }
        std::cout << "------------ A02 ------------\n";
        {
            std::mbstate_t state = std::mbstate_t();
            Array<wchar_t, 1> buffer;
            wchar_t* to      = buffer.data();
            wchar_t* to_end  = buffer.data() + buffer.size();
            wchar_t* to_next = 0;
            result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
            std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
            std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
            std::cout << "input consumed  = " << (from_next - from) << "\n";
            std::cout << "output produced = " << (to_next - to) << "\n";
            std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
            std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
            was_ok_2 = (result == std::codecvt_base::ok);
        }
        std::cout << "AGREE WITH STANDARD: " << (was_ok_1 == was_ok_2 ? "YES" : "NO") << "\n";
    }
    std::cout << "---------------------------------- B ----------------------------------\n";
    // Incomplete decoding
    std::cout << "------------ B01 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "\xC3";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ B02 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\xC3";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ B03 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "\xC3\x86";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ B04 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\xC3\x86";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 1> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "---------------------------------- C ----------------------------------\n";
    // Invalid decoding
    std::cout << "------------ C01 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "\x86";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C02 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\x86";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C03 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "\x86x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C04 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\x86x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C05 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "\x86\x87";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C06 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\x86\x87";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C07 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "\x86\x87x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C08 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\x86\x87x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C09 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "\xC3x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C10 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\xC3x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C11 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "\x86\xC3x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C12 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\x86\xC3x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ C13 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\xC3x";
        const char* from      = string.data();
        const char* from_end  = string.data() + 2;
        const char* from_next = 0;
        Array<wchar_t, 256> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result_1 = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        from      = from_next;
        from_end  = string.data() + string.size();
        to        = to_next;
        result_type result_2 = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok_1      = " << (result_1 == std::codecvt_base::ok) << "\n";
        std::cout << "is ok_2      = " << (result_2 == std::codecvt_base::ok) << "\n";
        std::cout << "is partial_1 = " << (result_1 == std::codecvt_base::partial) << "\n";
        std::cout << "is partial_2 = " << (result_2 == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - string.data()) << "\n";
        std::cout << "output produced = " << (to_next - buffer.data()) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "---------------------------------- D ----------------------------------\n";
    // Encoding
    std::cout << "------------ D01 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        WideStringView string = L"\xC6";
        const wchar_t* from      = string.data();
        const wchar_t* from_end  = string.data() + string.size();
        const wchar_t* from_next = 0;
        Array<char, 256> buffer;
        char* to      = buffer.data();
        char* to_end  = buffer.data() + buffer.size();
        char* to_next = 0;
        result_type result = codecvt.out(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ D02 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        WideStringView string = L"\xC6";
        const wchar_t* from      = string.data();
        const wchar_t* from_end  = string.data() + string.size();
        const wchar_t* from_next = 0;
        Array<char, 1> buffer;
        char* to      = buffer.data();
        char* to_end  = buffer.data() + buffer.size();
        char* to_next = 0;
        result_type result = codecvt.out(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ D03 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        WideStringView string = L"\xC6";
        const wchar_t* from      = string.data();
        const wchar_t* from_end  = string.data() + string.size();
        const wchar_t* from_next = 0;
        Array<char, 0> buffer;
        char* to      = buffer.data();
        char* to_end  = buffer.data() + buffer.size();
        char* to_next = 0;
        result_type result = codecvt.out(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "---------------------------------- E ----------------------------------\n";
    std::cout << "------------ E01 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 1> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
        bool sane = (result == std::codecvt_base::ok);
        std::cout << "SANE = " << (sane ? "YES" : "NO") << "\n";
    }
    std::cout << "------------ E02 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\xC3";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 1> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
    }
    std::cout << "------------ E03 ------------\n";
    {
        std::mbstate_t state = std::mbstate_t();
        StringView string = "x\xC3";
        const char* from      = string.data();
        const char* from_end  = string.data() + string.size();
        const char* from_next = 0;
        Array<wchar_t, 2> buffer;
        wchar_t* to      = buffer.data();
        wchar_t* to_end  = buffer.data() + buffer.size();
        wchar_t* to_next = 0;
        result_type result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        std::cout << "is ok      = " << (result == std::codecvt_base::ok) << "\n";
        std::cout << "is partial = " << (result == std::codecvt_base::partial) << "\n";
        std::cout << "input consumed  = " << (from_next - from) << "\n";
        std::cout << "output produced = " << (to_next - to) << "\n";
        std::cout << "in initial state = " << std::mbsinit(&state) << "\n";
        std::cout << "in initial state 2 = " << is_initial_2(state) << "\n";
        bool sane = (result == std::codecvt_base::ok);
        std::cout << "SANE = " << (sane ? "YES" : "NO") << "\n";
    }
    std::cout << "---------------------------------- F ----------------------------------\n";
    {
        {
            std::ofstream out("/tmp/x");
            StringView string = "682\xC3yfoo";
            out.write(string.data(), string.size());
            out.flush();
            std::cout << "bool out stream = " << bool(out) << "\n";
        }
        {
            std::wifstream in("/tmp/x");
            int i;
            in >> i;
            std::cout << "gcount in stream = " << in.gcount() << "\n";
            std::cout << "bool in stream   = " << bool(in) << "\n";
            std::cout << "good in stream   = " << in.good() << "\n";
            std::cout << "bad in stream    = " << in.bad() << "\n";
            std::cout << "fail in stream   = " << in.fail() << "\n";
            std::cout << "eof in stream    = " << in.eof() << "\n";
        }
    }
}
