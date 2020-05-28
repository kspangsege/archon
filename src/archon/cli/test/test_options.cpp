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


template<class P, class S>
bool test(check::TestContext& test_context, core::Span<const char* const> args, S setup, int& exit_status)
{
    using proc_type = P;
    typename proc_type::spec_type spec;
    setup(spec);
    using char_type = typename proc_type::char_type;
    int argc = int(args.size());
    const char* const* argv = args.data();
    log::BasicEncodingLogger<char_type> logger(test_context.logger);
    cli::BasicLoggingErrorHandler error_handler(logger);
    typename proc_type::command_line_type::Config config;
    config.error_handler = &error_handler;
    return cli::process(argc, argv, spec, exit_status, test_context.locale, std::move(config));
}



ARCHON_TEST_VARIANTS(proc_variants,
                     ARCHON_TEST_TYPE(cli::Processor,     Processor),
                     ARCHON_TEST_TYPE(cli::WideProcessor, WideProcessor));


} // unnamed namespace


ARCHON_TEST_BATCH(Cli_Options_Basics, proc_variants)
{
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    int x = 0;
    auto setup = [&](spec_type& spec) {
        opt("-x", "<x>", cli::no_attributes, spec, "", cli::assign(x));
    };
    {
        const char* args[] = { "prog", "-x", "101" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(!complete);
        ARCHON_CHECK_EQUAL(x, 101);
    }
    {
        const char* args[] = { "prog", "-x", "y" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
    }
    {
        const char* args[] = { "prog", "-x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
    }
}
