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


#include <archon/core/buffer.hpp>
#include <archon/core/hex_dump.hpp>
#include <archon/core/base64.hpp>
#include <archon/check.hpp>


using namespace archon;
namespace base64 = core::base64;


ARCHON_TEST(Core_Base64_EncodeBufferSize)
{
    base64::EncodeConfig config;
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 0, config),  0);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 1, config),  2);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 2, config),  3);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 3, config),  4);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 4, config),  6);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 5, config),  7);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 6, config),  8);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 7, config), 10);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 8, config), 11);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 9, config), 12);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size(10, config), 14);

    // With padding
    config.use_padding = true;
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 0, config),  0);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 1, config),  4);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 2, config),  4);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 3, config),  4);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 4, config),  8);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 5, config),  8);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 6, config),  8);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 7, config), 12);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 8, config), 12);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 9, config), 12);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size(10, config), 16);

    // With line breaking
    config.use_padding = false;
    config.line_size = 8;
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 0, config),  0);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 1, config),  3);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 2, config),  4);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 3, config),  5);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 4, config),  7);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 5, config),  8);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 6, config),  9);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 7, config), 12);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 8, config), 13);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 9, config), 14);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size(10, config), 16);

    // With padding and line breaking
    config.use_padding = true;
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 0, config),  0);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 1, config),  5);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 2, config),  5);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 3, config),  5);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 4, config),  9);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 5, config),  9);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 6, config),  9);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 7, config), 14);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 8, config), 14);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size( 9, config), 14);
    ARCHON_CHECK_EQUAL(base64::encode_buffer_size(10, config), 18);
}


ARCHON_TEST(Core_Base64_DecodeBufferSize)
{
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 0), 0);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 1), 0);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 2), 1);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 3), 2);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 4), 3);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 5), 3);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 6), 4);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 7), 5);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 8), 6);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size( 9), 6);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size(10), 7);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size(11), 8);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size(12), 9);
    ARCHON_CHECK_EQUAL(base64::decode_buffer_size(13), 9);
}


ARCHON_TEST(Core_Base64_Encode)
{
    const char data[] {
        32, 43, 23, 121, 63, 38, 34, 92, 7, 26, 113, 29, 91, 32, 43, 123,
        11, 63, 38, 34, 10, 7, 26, 113, 23, 81
    };
    core::Buffer<char> buffer;
    base64::EncodeConfig config;
    config.use_padding = false;
    config.line_size = 0;
    ARCHON_CHECK_EQUAL(base64::encode(data, buffer, test_context.locale, config),
                       "ICsXeT8mIlwHGnEdWyArews/JiIKBxpxF1E");
}


ARCHON_TEST(Core_Base64_Decode)
{
    const char* data = "ICsXeT8mIlwHGnEdWyArews/JiIKBxpxF1E";
    core::Buffer<char> buffer;
    base64::DecodeConfig config;
    config.padding = base64::DecodeConfig::Padding::reject;
    config.allow_whitespace = false;
    const char expected[] {
        32, 43, 23, 121, 63, 38, 34, 92, 7, 26, 113, 29, 91, 32, 43, 123,
        11, 63, 38, 34, 10, 7, 26, 113, 23, 81
    };
    ARCHON_CHECK_EQUAL_SEQ(base64::decode(std::string_view(data), buffer, config), expected);
}


ARCHON_TEST(Core_Base64_IncrementalEncode)
{
    char data[] { 33 };
    std::array<char, 256> buffer;
    base64::IncrementalEncoder encoder;
    {
        const char* data_begin = data + 0;
        const char* data_end   = data + 1;
        bool end_of_input = false;
        char* buffer_begin = buffer.data();
        char* buffer_end   = buffer_begin + buffer.size();
        bool done = encoder.encode(data_begin, data_end, end_of_input, buffer_begin, buffer_end);
        ARCHON_CHECK_NOT(done);
        ARCHON_CHECK_EQUAL(data_begin, data + 1);
        ARCHON_CHECK_EQUAL(data_end,   data + 1);
        ARCHON_CHECK_EQUAL(buffer_begin, buffer.data());
        ARCHON_CHECK_EQUAL(buffer_end,   buffer.data() + buffer.size());
    }
    {
        const char* data_begin = data + 1;
        const char* data_end   = data + 1;
        bool end_of_input = true;
        char* buffer_begin = buffer.data();
        char* buffer_end   = buffer_begin + buffer.size();
        bool done = encoder.encode(data_begin, data_end, end_of_input, buffer_begin, buffer_end);
        ARCHON_CHECK(done);
        ARCHON_CHECK_EQUAL(data_begin, data + 1);
        ARCHON_CHECK_EQUAL(data_end,   data + 1);
        ARCHON_CHECK_EQUAL(buffer_begin, buffer.data() + 2);
        ARCHON_CHECK_EQUAL(buffer_end,   buffer.data() + buffer.size());
    }
    ARCHON_CHECK_EQUAL(std::string_view(buffer.data(), 2), "IQ");
}
