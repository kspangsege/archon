#include <fstream>

#include <archon/unit_test.hpp>


using namespace archon;


ARCHON_TEST(Baz)
{
/*
    std::wstreambuf& streambuf = *std::wcout.rdbuf();
    streambuf.sputc(wchar_t(35265));
    streambuf.pubsync();
*/
    std::wofstream out("/tmp/out");
    out.imbue(std::locale("C"));
/*
    std::wstreambuf& streambuf = *out.rdbuf();
    streambuf.sputc(wchar_t(35265));
    streambuf.pubsync();
*/
    out << wchar_t(35265) << std::flush;
}
