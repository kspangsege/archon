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


// Do not include this header file. It exists only to specify the canonical header order,
// which is a topological dependency ordering of all the header files of the Archon Core
// Library, including any that must never be included by applications.
#error "Do not include this header file"


// Foundation
#include <archon/check/check_namespace.hpp>
#include <archon/check/seed_seq.hpp>
#include <archon/check/random_seed.hpp>

// Support
#include <archon/check/test_details.hpp>
#include <archon/check/check_arg.hpp>

// Main
#include <archon/check/root_context.hpp>
#include <archon/check/thread_context.hpp>
#include <archon/check/test_context.hpp>
#include <archon/check/test_list.hpp>
#include <archon/check/fail_context.hpp>
#include <archon/check/reporter.hpp>
#include <archon/check/test_config.hpp>
#include <archon/check/noinst/test_level_report_logger.hpp>
#include <archon/check/noinst/root_context_impl.hpp>
#include <archon/check/noinst/thread_context_impl.hpp>
#include <archon/check/test_runner.hpp>
#include <archon/check/run.hpp>
#include <archon/check/command.hpp>

// Test and check macros
#include <archon/check/test_macros.hpp>
#include <archon/check/test_batch_macros.hpp>
#include <archon/check/test_trail.hpp>
#include <archon/check/test_path.hpp>
#include <archon/check/check_macros.hpp>
#include <archon/check/check_no_error.hpp>

// Utilities
#include <archon/check/standard_path_mapper.hpp>
#include <archon/check/pattern_based_test_order.hpp>
#include <archon/check/wildcard_filter.hpp>
#include <archon/check/simple_reporter.hpp>
#include <archon/check/duplicating_reporter.hpp>
#include <archon/check/xml_reporter.hpp>
