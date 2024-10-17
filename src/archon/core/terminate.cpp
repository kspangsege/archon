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


#include <string_view>

#include <archon/core/terminate.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/file.hpp>


using namespace archon;
namespace impl = core::impl;


namespace {

void do_write_message(std::string_view message) noexcept
{
    std::size_t n; // Dummy
    std::error_code ec; // Dummy
    bool ret = core::File::get_stderr().try_write(message, n, ec);
    static_cast<void>(ret); // There is nothing more we can do
}

void write_message(const char* message, const char* file, long line, core::Span<const impl::TerminateVal2> values)
{
    core::SeedMemoryOutputStream out;
    out << file << ":" << line << ": " << message << "\n"; // Throws
    for (const impl::TerminateVal2& value : values)
        out << value.text << " = " << value.value << "\n"; // Throws
    do_write_message(out.view());
}

} // unnamed namespace


[[noreturn]] void impl::do_terminate(const char* message, const char* file, long line,
                                     core::Span<const impl::TerminateVal2> values) noexcept
{
    try {
        write_message(message, file, line, values); // Throws
    }
    catch (...) {
        do_write_message("Terminating: Failed to format termination message (this is not the message you were "
                         "intended to see)\n");
    }
    std::abort();
}
