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


#include <archon/log/encoding_logger.hpp>
#include <archon/cli.hpp>
#include <archon/cli/logging_error_handler.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(proc_variants,
                     ARCHON_TEST_TYPE(cli::Processor,     Processor),
                     ARCHON_TEST_TYPE(cli::WideProcessor, WideProcessor));


} // unnamed namespace


ARCHON_TEST_BATCH(Cli_Processor_FromCommandLine, proc_variants)
{
    using proc_type = test_type;

    using char_type         = typename proc_type::char_type;
    using command_line_type = typename proc_type::command_line_type;
    using spec_type         = typename proc_type::spec_type;

    std::array args = { "prog", "7" };
    int argc = int(args.size());
    const char* const* argv = args.data();
    log::BasicEncodingLogger<char_type> logger(test_context.logger);
    cli::BasicLoggingErrorHandler error_handler(logger);
    typename command_line_type::Config config;
    config.error_handler = &error_handler;
    command_line_type command_line(argc, argv, test_context.locale, std::move(config));
    spec_type spec;
    int val = -1;
    pat("<val>", cli::no_attributes, spec, "", std::tie(val));
    proc_type proc(command_line, spec);

    int exit_status; // Dummy
    ARCHON_CHECK_NOT(proc.process(exit_status));
    ARCHON_CHECK_EQUAL(val, 7);
}


ARCHON_TEST_BATCH(Cli_Processor_FromArgcArgv, proc_variants)
{
    using proc_type = test_type;

    using char_type         = typename proc_type::char_type;
    using command_line_type = typename proc_type::command_line_type;
    using spec_type         = typename proc_type::spec_type;

    std::array args = { "prog", "7" };
    int argc = int(args.size());
    const char* const* argv = args.data();
    spec_type spec;
    int val = -1;
    pat("<val>", cli::no_attributes, spec, "", std::tie(val));
    log::BasicEncodingLogger<char_type> logger(test_context.logger);
    cli::BasicLoggingErrorHandler error_handler(logger);
    typename command_line_type::Config config;
    config.error_handler = &error_handler;
    proc_type proc(argc, argv, spec, test_context.locale, std::move(config));

    int exit_status; // Dummy
    ARCHON_CHECK_NOT(proc.process(exit_status));
    ARCHON_CHECK_EQUAL(val, 7);
}
