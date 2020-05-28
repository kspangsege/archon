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


#include <archon/core/newline_codec.hpp>
#include <archon/check.hpp>


using namespace archon;
namespace codec = core::newline_codec;


ARCHON_TEST(Core_NewlineCodec_Decode)
{
    // Enough buffer space
    {
        std::string_view data = "foo\r\nbar\r\nbaz";
        std::size_t data_offset = 0;
        bool end_of_data = false;
        std::array<char, 16> buffer;
        std::size_t buffer_offset = 0;
        std::size_t clear_offset = 100;
        std::size_t clear = 0;
        codec::decode(data, data_offset, end_of_data, buffer, buffer_offset, clear_offset, clear);
        ARCHON_CHECK_EQUAL(data_offset, 13);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer_offset), "foo\nbar\nbaz");
        ARCHON_CHECK_EQUAL(clear, 108);
    }

    // Not enough buffer space
    {
        std::string_view data = "foo\r\nbar\r\nbaz";
        std::size_t data_offset = 0;
        bool end_of_data = false;
        std::array<char, 7> buffer;
        std::size_t buffer_offset = 0;
        std::size_t clear_offset = 100;
        std::size_t clear = 0;
        codec::decode(data, data_offset, end_of_data, buffer, buffer_offset, clear_offset, clear);
        ARCHON_CHECK_EQUAL(data_offset, 8);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer_offset), "foo\nbar");
        ARCHON_CHECK_EQUAL(clear, 104);
    }

    // CR at end of data, end_of_data = false
    {
        std::string_view data = "foo\r\nbar\r";
        std::size_t data_offset = 0;
        bool end_of_data = false;
        std::array<char, 16> buffer;
        std::size_t buffer_offset = 0;
        std::size_t clear_offset = 100;
        std::size_t clear = 0;
        codec::decode(data, data_offset, end_of_data, buffer, buffer_offset, clear_offset, clear);
        ARCHON_CHECK_EQUAL(data_offset, 8);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer_offset), "foo\nbar");
        ARCHON_CHECK_EQUAL(clear, 104);
    }

    // CR at end of data, end_of_data = true
    {
        std::string_view data = "foo\r\nbar\r";
        std::size_t data_offset = 0;
        bool end_of_data = true;
        std::array<char, 16> buffer;
        std::size_t buffer_offset = 0;
        std::size_t clear_offset = 100;
        std::size_t clear = 0;
        codec::decode(data, data_offset, end_of_data, buffer, buffer_offset, clear_offset, clear);
        ARCHON_CHECK_EQUAL(data_offset, 9);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer_offset), "foo\nbar\r");
        ARCHON_CHECK_EQUAL(clear, 104);
    }

    // Whacky input
    {
        std::string_view data = "foo\nbar\r\n\rbaz\n\rfoo\n\r\nbar\rbaz";
        std::size_t data_offset = 0;
        bool end_of_data = true;
        std::array<char, 32> buffer;
        std::size_t buffer_offset = 0;
        std::size_t clear_offset = 100;
        std::size_t clear = 0;
        codec::decode(data, data_offset, end_of_data, buffer, buffer_offset, clear_offset, clear);
        ARCHON_CHECK_EQUAL(data_offset, 28);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer_offset), "foo\nbar\n\rbaz\n\rfoo\n\nbar\rbaz");
        ARCHON_CHECK_EQUAL(clear, 119);
    }
}


ARCHON_TEST(Core_NewlineCodec_Encode)
{
    // Enough buffer space
    {
        std::string_view data = "foo\nbar\nbaz";
        std::size_t data_offset = 0;
        std::array<char, 16> buffer;
        std::size_t buffer_offset = 0;
        codec::encode(data, data_offset, buffer, buffer_offset);
        ARCHON_CHECK_EQUAL(data_offset, 11);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer_offset), "foo\r\nbar\r\nbaz");
    }

    // Not enough buffer space
    {
        std::string_view data = "foo\nbar\nbaz";
        std::size_t data_offset = 0;
        std::array<char, 8> buffer;
        std::size_t buffer_offset = 0;
        codec::encode(data, data_offset, buffer, buffer_offset);
        ARCHON_CHECK_EQUAL(data_offset, 7);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer_offset), "foo\r\nbar");
    }

    // Not enough buffer space for LF
    {
        std::string_view data = "foo\nbar\nbaz";
        std::size_t data_offset = 0;
        std::array<char, 9> buffer;
        std::size_t buffer_offset = 0;
        codec::encode(data, data_offset, buffer, buffer_offset);
        ARCHON_CHECK_EQUAL(data_offset, 7);
        ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), buffer_offset), "foo\r\nbar");
    }
}


ARCHON_TEST(Core_NewlineCodec_SimulDecode)
{
    // Not too much buffer space
    {
        std::string_view data = "foo\r\nbar\r\nbaz";
        std::size_t data_offset = 0;
        std::size_t buffer_size = 7;
        ARCHON_CHECK(codec::simul_decode(data, data_offset, buffer_size));
        ARCHON_CHECK_EQUAL(data_offset, 8);
    }
    {
        std::string_view data = "foo\r\nbar\r\nbaz";
        std::size_t data_offset = 0;
        std::size_t buffer_size = 8;
        ARCHON_CHECK(codec::simul_decode(data, data_offset, buffer_size));
        ARCHON_CHECK_EQUAL(data_offset, 10);
    }
    {
        std::string_view data = "foo\r\nbar\r\nbaz";
        std::size_t data_offset = 0;
        std::size_t buffer_size = 11;
        ARCHON_CHECK(codec::simul_decode(data, data_offset, buffer_size));
        ARCHON_CHECK_EQUAL(data_offset, 13);
    }

    // Too much buffer space
    {
        std::string_view data = "foo\r\nbar\r\nbaz";
        std::size_t data_offset = 0;
        std::size_t buffer_size = 12;
        ARCHON_CHECK_NOT(codec::simul_decode(data, data_offset, buffer_size));
        ARCHON_CHECK_EQUAL(data_offset, 0);
    }

    // CR at end of data
    {
        std::string_view data = "foo\r\nbar\r";
        std::size_t data_offset = 0;
        std::size_t buffer_size = 8;
        ARCHON_CHECK(codec::simul_decode(data, data_offset, buffer_size));
        ARCHON_CHECK_EQUAL(data_offset, 9);
    }

    // Whacky input
    {
        std::string_view data = "foo\nbar\r\n\rbaz\n\rfoo\n\r\nbar\rbaz";
        std::size_t data_offset = 0;
        std::size_t buffer_size = 24;
        ARCHON_CHECK(codec::simul_decode(data, data_offset, buffer_size));
        ARCHON_CHECK_EQUAL(data_offset, 26);
    }
}
