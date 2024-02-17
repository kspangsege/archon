// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <ios>

#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/log/stream_logger.hpp>
#include <archon/log/prefix_logger.hpp>
#include <archon/log/encoding_logger.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST_BATCH(Log_EncodingLogger_Basics, variants)
{
    using char_type = test_type;
    using encoding_logger_type = log::BasicEncodingLogger<char_type>;
    using prefix_logger_type = log::BasicPrefixLogger<char_type>;

    core::StringFormatter string_formatter(test_context.locale);

    auto test = [&](const char* prefix) {
        core::SeedMemoryOutputStream out;
        out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
        out.imbue(test_context.locale); // Throws
        log::StreamLogger logger_1(out);
        encoding_logger_type logger_2(logger_1);
        prefix_logger_type logger_3(logger_2, prefix);
        logger_3.info("Click");
        ARCHON_CHECK_EQUAL(out.view(), string_formatter.format("%sClick\n", prefix));
    };

    test("--: ");
    test("----------------------------------------------------------------"
         "----------------------------------------------------------------"
         "----------------------------------------------------------------"
         "----------------------------------------------------------------"
         "--------------------------------------------------------------: ");
}
