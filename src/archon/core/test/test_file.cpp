// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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
#include <array>
#include <string_view>
#include <string>
#include <system_error>
#include <filesystem>

#include <archon/core/file.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {

constexpr std::string_view g_test_dir_path = "archon/core/test";

} // unnamed namespace



ARCHON_TEST(Core_File_TryOpenRejectsDirPath)
{
    ARCHON_TEST_DIR(path);
    core::File file;
    std::error_code ec;
    ARCHON_CHECK_NOT(file.try_open(path, core::File::Mode::read, ec));
}


ARCHON_TEST(Core_File_NonblockingLockExclusive)
{
    ARCHON_TEST_FILE(path);
    core::File::touch(path);
    core::File file_1(path);
    core::File file_2(path);
    ARCHON_CHECK(file_1.nb_lock_exclusive());
    ARCHON_CHECK_NOT(file_2.nb_lock_exclusive());
    file_1.unlock();
    ARCHON_CHECK(file_2.nb_lock_exclusive());
}


ARCHON_TEST(Core_File_NonblockingLockShared)
{
    ARCHON_TEST_FILE(path);
    core::File::touch(path);
    core::File file_1(path);
    core::File file_2(path);
    core::File file_3(path);
    ARCHON_CHECK(file_1.nb_lock_shared());
    ARCHON_CHECK(file_2.nb_lock_shared());
    ARCHON_CHECK_NOT(file_3.nb_lock_exclusive());
    file_1.unlock();
    ARCHON_CHECK_NOT(file_3.nb_lock_exclusive());
    file_2.unlock();
    ARCHON_CHECK(file_3.nb_lock_exclusive());
}


ARCHON_TEST(Core_File_Read)
{
    namespace fs = std::filesystem;
    fs::path path = test_context.get_data_path(g_test_dir_path, "test_file_data.txt");
    core::File file(path);
    std::array<char, 16> buffer;
    std::size_t n = file.read(buffer);
    std::string_view string(buffer.data(), n);
    ARCHON_CHECK_EQUAL(string, "foo\nbar\n");
}


ARCHON_TEST(Core_File_Write)
{
    ARCHON_TEST_FILE(path);
    std::string_view string = "foo\nbar\n";
    {
        core::File file(path, core::File::Mode::write);
        file.write(string);
    }
    {
        core::File file(path);
        std::array<char, 16> buffer;
        std::size_t n = file.read(buffer);
        std::string_view string_2(buffer.data(), n);
        ARCHON_CHECK_EQUAL(string_2, string);
    }
}


ARCHON_TEST(Core_File_Load)
{
    namespace fs = std::filesystem;
    fs::path path = test_context.get_data_path(g_test_dir_path, "test_file_data.txt");
    std::string string = core::File::load(path);
    ARCHON_CHECK_EQUAL(string, "foo\nbar\n");
}


ARCHON_TEST(Core_File_Save)
{
    ARCHON_TEST_FILE(path);
    std::string_view string = "foo\nbar\n";
    core::File::save(path, string);
    std::string string_2 = core::File::load(path);
    ARCHON_CHECK_EQUAL(string_2, string);
}


ARCHON_TEST(Core_File_Tell)
{
    ARCHON_TEST_FILE(path);
    {
        core::File file(path, core::File::Mode::write);
        ARCHON_CHECK_EQUAL(file.tell(), 0);
        file.write(std::string_view("foo"));
        ARCHON_CHECK_EQUAL(file.tell(), 3);
        file.write(std::string_view("bar"));
        ARCHON_CHECK_EQUAL(file.tell(), 6);
    }
    {
        core::File file(path);
        ARCHON_CHECK_EQUAL(file.tell(), 0);
        std::array<char, 3> buffer;
        file.read(buffer);
        ARCHON_CHECK_EQUAL(file.tell(), 3);
        file.read(buffer);
        ARCHON_CHECK_EQUAL(file.tell(), 6);
    }
}


ARCHON_TEST(Core_File_Seek)
{
    ARCHON_TEST_FILE(path);
    core::File::save(path, std::string_view("alpha gamma eta beta delta epsilon"));
    {
        core::File file(path, core::File::Mode::update);
        file.seek(16);
        file.write(std::string_view("theta"));
        file.write(std::string_view(" zeta"));
        file.seek(6);
        file.write(std::string_view("kappa"));
    }
    std::string string = core::File::load(path);
    ARCHON_CHECK_EQUAL(string, "alpha kappa eta theta zeta epsilon");
    {
        core::File file(path);
        file.seek(16);
        std::array<char, 5> buffer;
        file.read(buffer);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer.size()), "theta");
        file.read(buffer);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer.size()), " zeta");
        file.seek(6);
        file.read(buffer);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer.size()), "kappa");
    }
}


ARCHON_TEST(Core_File_FilePointerIndependence)
{
    ARCHON_TEST_FILE(path);
    core::File::save(path, std::string_view("alpha beta gamma delta delta epsilon zeta"));
    {
        core::File file_1(path, core::File::Mode::update);
        core::File file_2(path, core::File::Mode::update);
        file_1.seek(6);
        file_2.seek(23);
        file_1.write(std::string_view("kappa"));
        file_2.write(std::string_view("omicron"));
        file_1.write(std::string_view(" iota"));
        file_2.write(std::string_view(" theta"));
    }
    std::string string = core::File::load(path);
    ARCHON_CHECK_EQUAL(string, "alpha kappa iota delta omicron theta zeta");
}
