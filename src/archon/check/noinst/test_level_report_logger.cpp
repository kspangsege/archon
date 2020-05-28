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

#include <archon/core/as_int.hpp>
#include <archon/log/logger.hpp>
#include <archon/check/noinst/test_level_report_logger.hpp>


using namespace archon;
namespace impl = check::impl;
using impl::TestLevelReportLogger;


TestLevelReportLogger::TestLevelReportLogger(log::Logger& base_logger) noexcept
    : log::Logger(*this, base_logger.get_channel(), base_logger.get_channel_map())
    , m_parent_prefix(base_logger.get_prefix())
{
}


void TestLevelReportLogger::format_prefix(ostream_type& out) const
{
    m_parent_prefix.format_prefix(out); // Throws
    std::string_view test_name = test_context->test_details.name;
    out << file_path << ":" << core::as_int(line_number) << ": " << test_name; // Throws
    if (test_context->thread_context.root_context.num_repetitions > 1)
        out << "#" << core::as_int(test_context->repetition_no); // Throws
    out << ": "; // Throws
}
