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
#include <system_error>

#include <archon/core/file.hpp>
#include <archon/core/file_source.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_FileSource_EndOfFileOnRead_Unbuffered)
{
    ARCHON_TEST_FILE(path);
    core::File::touch(path);
    core::File file(path);
    core::FileSource source(file);
    std::array<char, 1> buffer;
    std::size_t n;
    std::error_code ec;
    if (ARCHON_CHECK_NO_ERROR(source.try_read(buffer, n, ec), ec))
        ARCHON_CHECK_EQUAL(n, 0);
}


ARCHON_TEST(Core_FileSource_EndOfFileOnRead_Buffered)
{
    ARCHON_TEST_FILE(path);
    core::File::touch(path);
    core::File file(path);
    std::array<char, 1> buffer_1;
    core::BufferedFileSource source(file, buffer_1);
    std::array<char, 1> buffer_2;
    std::size_t n;
    std::error_code ec;
    if (ARCHON_CHECK_NO_ERROR(source.try_read(buffer_2, n, ec), ec))
        ARCHON_CHECK_EQUAL(n, 0);
}
