// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include <archon/base/file.hpp>
#include <archon/unit_test.hpp>

#include <archon/base/string_codec.hpp>            
#include <archon/base/hex_dump.hpp>            


using namespace archon;


namespace {

constexpr std::string_view g_test_dir_path = "archon/base/test";

} // unnamed namespace



ARCHON_TEST(Base_File_NonblockingLockExclusive)
{
    ARCHON_TEST_FILE(path);
    base::File::touch(path);
    base::File file_1(path);
    base::File file_2(path);
    ARCHON_CHECK(file_1.nb_lock_exclusive());
    ARCHON_CHECK_NOT(file_2.nb_lock_exclusive());
    file_1.unlock();
    ARCHON_CHECK(file_2.nb_lock_exclusive());
}


ARCHON_TEST(Base_File_NonblockingLockShared)
{
    ARCHON_TEST_FILE(path);
    base::File::touch(path);
    base::File file_1(path);
    base::File file_2(path);
    base::File file_3(path);
    ARCHON_CHECK(file_1.nb_lock_shared());
    ARCHON_CHECK(file_2.nb_lock_shared());
    ARCHON_CHECK_NOT(file_3.nb_lock_exclusive());
    file_1.unlock();
    ARCHON_CHECK_NOT(file_3.nb_lock_exclusive());
    file_2.unlock();
    ARCHON_CHECK(file_3.nb_lock_exclusive());
}


ARCHON_TEST(Base_File_Read)
{
    namespace fs = std::filesystem;
    fs::path path = test_context.get_data_path(g_test_dir_path, "test_file_foo.txt");
    base::File file(path);
    std::array<char, 16> buffer;
    std::size_t n = file.read(buffer);
    std::string_view string(buffer.data(), n);
    ARCHON_CHECK_EQUAL(string, "foo\n");
}


ARCHON_TEST(Base_File_Write)
{
    ARCHON_TEST_FILE(path);
    std::string_view string = "bar\n";
    {
        base::File file(path, base::File::Mode::write);
        file.write(string);
    }
    {
        base::File file(path);
        std::array<char, 16> buffer;
        std::size_t n = file.read(buffer);
        std::string_view string_2(buffer.data(), n);
        ARCHON_CHECK_EQUAL(string_2, string);
    }
}


ARCHON_TEST(Base_File_Load)
{
    namespace fs = std::filesystem;
    fs::path path = test_context.get_data_path(g_test_dir_path, "test_file_foo.txt");
    std::string string = base::File::load(path);
    ARCHON_CHECK_EQUAL(string, "foo\n");
}


ARCHON_TEST(Base_File_LoadAndChomp)
{
    namespace fs = std::filesystem;
    fs::path path = test_context.get_data_path(g_test_dir_path, "test_file_foo.txt");
    std::string string = base::File::load_and_chomp(path);
    ARCHON_CHECK_EQUAL(string, "foo");
}


ARCHON_TEST(Base_File_Save)
{
    ARCHON_TEST_FILE(path);
    std::string_view string = "bar\n";
    base::File::save(path, string);
    std::string string_2 = base::File::load(path);
    ARCHON_CHECK_EQUAL(string_2, string);
}


ARCHON_TEST(Base_File_GetOffset)
{
    ARCHON_TEST_FILE(path);
    {
        base::File file(path, base::File::Mode::write);
        ARCHON_CHECK_EQUAL(file.get_offset(), 0);
        file.write(std::string_view("foo"));
        ARCHON_CHECK_EQUAL(file.get_offset(), 3);
        file.write(std::string_view("bar"));
        ARCHON_CHECK_EQUAL(file.get_offset(), 6);
    }
    {
        base::File file(path);
        ARCHON_CHECK_EQUAL(file.get_offset(), 0);
        std::array<char, 3> buffer;
        file.read(buffer);
        ARCHON_CHECK_EQUAL(file.get_offset(), 3);
        file.read(buffer);
        ARCHON_CHECK_EQUAL(file.get_offset(), 6);
    }
}


ARCHON_TEST(Base_File_SetOffset)
{
    ARCHON_TEST_FILE(path);
    base::File::save(path, std::string_view("alpha gamma eta beta delta epsilon"));
    {
        base::File file(path, base::File::Mode::update);
        file.set_offset(16);
        file.write(std::string_view("theta"));
        file.write(std::string_view(" zeta"));
        file.set_offset(6);
        file.write(std::string_view("kappa"));
    }
    std::string string = base::File::load(path);
    ARCHON_CHECK_EQUAL(string, "alpha kappa eta theta zeta epsilon");
    {
        base::File file(path);
        file.set_offset(16);
        std::array<char, 5> buffer;
        file.read(buffer);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer.size()), "theta");
        file.read(buffer);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer.size()), " zeta");
        file.set_offset(6);
        file.read(buffer);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer.size()), "kappa");
    }
}


ARCHON_TEST(Base_File_OffsetIndependance)
{
    ARCHON_TEST_FILE(path);
    base::File::save(path, std::string_view("alpha beta gamma delta delta epsilon zeta"));
    {
        base::File file_1(path, base::File::Mode::update);
        base::File file_2(path, base::File::Mode::update);
        file_1.set_offset(6);
        file_2.set_offset(23);
        file_1.write(std::string_view("kappa"));
        file_2.write(std::string_view("omicron"));
        file_1.write(std::string_view(" iota"));
        file_2.write(std::string_view(" theta"));
    }
    std::string string = base::File::load(path);
    ARCHON_CHECK_EQUAL(string, "alpha kappa iota delta omicron theta zeta");
}


/*
ARCHON_TEST(Base_File_StreamInput)
{
    ARCHON_TEST_FILE(path);
    base::File::save(path, std::string_view("6723 6364 7735"));
    base::File file(path);
    base::File::Streambuf::Buffers<3> buffers;
    base::File::Streambuf streambuf(std::move(file), buffers);
    std::istream in(&streambuf);
    int a = 0, b = 0, c = 0;
    ARCHON_CHECK(in >> a >> b >> c);
    ARCHON_CHECK(in.eof());
    ARCHON_CHECK_EQUAL(a, 6723);
    ARCHON_CHECK_EQUAL(b, 6364);
    ARCHON_CHECK_EQUAL(c, 7735);
}
*/


ARCHON_TEST(Base_File_Foo)
{
    log_info("Global  locale: %s", std::locale().name());
    log_info("Classic locale: %s", std::locale::classic().name());
    log_info("Environ locale: %s", std::locale("").name());
//    log_info("English locale: %s", std::locale("en_US").name());
    log_info("English locale: %s", std::locale("en_US.utf8").name());
    log_info("1: <<<\U00000041>>>");
    log_info("1: <<<\U000000C5>>>");
    log_info("1: <<<\U00002022>>>");
    log_info("1: <<<\U0001F64F>>>");
    log_info("Width of wchar_t is %s", base::get_int_width<wchar_t>());
    // Windows -> UTF-16LE
    // UTF-16 -> high ten bits first: 0xD800–0xDBFF
    // UTF-16 -> low  ten bits last:  0xDC00–0xDFFF
    // Example: D83D DE4F
    std::locale locale;
    base::WideStringEncoder encoder(locale);
    log_info("2: <<<%s>>>", encoder.encode(L"\U00000041"));
    log_info("2: <<<%s>>>", encoder.encode(L"\U000000C5"));
    log_info("2: <<<%s>>>", encoder.encode(L"\U00002022"));
    log_info("2: <<<%s>>>", encoder.encode(L"\U0001F64F"));
    using codecvt_type = std::codecvt<wchar_t, char, std::mbstate_t>;
    const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);
    {
        std::array<wchar_t, 2> data { 0xD83D, 0xDE4F };
        std::mbstate_t state = {};
        const wchar_t* from_next = nullptr;
        std::array<char, 256> buffer;
        char* to_next = nullptr;
        codecvt_type::result result =
            codecvt.out(state, data.data(), data.data() + data.size(), from_next,
                        buffer.data(), buffer.data() + buffer.size(), to_next);
        ARCHON_CHECK_EQUAL(result, codecvt_type::ok);
        ARCHON_CHECK_EQUAL(from_next, data.data() + data.size());
        ARCHON_CHECK_EQUAL(to_next, buffer.data() + 4);
        std::array<char, 4> expected { char(0xF0), char(0x9F), char(0x99), char(0x8F) };
        ARCHON_CHECK_EQUAL_SEQ(base::Span(buffer).subspan(0, 4), expected);
    }
}
