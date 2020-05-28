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
int test(check::TestContext& test_context, core::Span<const char* const> args, S setup)
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
    return cli::process(argc, argv, spec, test_context.locale, std::move(config));
}



ARCHON_TEST_VARIANTS(proc_variants,
                     ARCHON_TEST_TYPE(cli::Processor,     Processor),
                     ARCHON_TEST_TYPE(cli::WideProcessor, WideProcessor));


} // unnamed namespace


ARCHON_TEST_BATCH(Cli_PatternForms_Optionality, proc_variants)
{
    int n_1, n_2;
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [&](spec_type& spec) {
        pat("[<val>]", cli::no_attributes, spec,
            "Lorem ipsum.",
            [&](std::optional<int> opt) {
                ++n_1;
                if (opt.has_value()) {
                    ARCHON_CHECK_EQUAL(*opt, 7);
                    ++n_2;
                }
            });
    };
    {
        const char* args[] = { "prog" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 1);
        ARCHON_CHECK_EQUAL(n_2, 0);
    }
    {
        const char* args[] = { "prog", "7" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 1);
        ARCHON_CHECK_EQUAL(n_2, 1);
    }
    {
        const char* args[] = { "prog", "7", "8" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_NOT_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 0);
    }
    {
        const char* args[] = { "prog", "x" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_NOT_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 0);
    }
}


ARCHON_TEST_BATCH(Cli_PatternForms_Repetition, proc_variants)
{
    int n_1, n_2;
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [&](spec_type& spec) {
        pat("<val>...", cli::no_attributes, spec,
            "Lorem ipsum.",
            [&](std::vector<int> vec) {
                ++n_1;
                if (ARCHON_LIKELY(ARCHON_CHECK_BETWEEN(vec.size(), 1, 2))) {
                    ARCHON_CHECK_EQUAL(vec[0], 7);
                    if (vec.size() == 2) {
                        ARCHON_CHECK_EQUAL(vec[1], 8);
                        ++n_2;
                    }
                }
            });
    };
    {
        const char* args[] = { "prog", "7" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 1);
        ARCHON_CHECK_EQUAL(n_2, 0);
    }
    {
        const char* args[] = { "prog", "7", "8" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 1);
        ARCHON_CHECK_EQUAL(n_2, 1);
    }
    {
        const char* args[] = { "prog" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_NOT_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 0);
    }
    {
        const char* args[] = { "prog", "x" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_NOT_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 0);
    }
}


ARCHON_TEST_BATCH(Cli_PatternForms_Alternatives, proc_variants)
{
    int n_1, n_2;
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [&](spec_type& spec) {
        pat("(-x <val> | -y <val>)", cli::no_attributes, spec,
            "Lorem ipsum.",
            [&](std::variant<int, int> var) {
                ++n_1;
                if (var.index() == 0) {
                    ARCHON_CHECK_EQUAL(std::get<0>(var), 7);
                }
                else {
                    ARCHON_CHECK_EQUAL(std::get<1>(var), 8);
                    ++n_2;
                }
            });
    };
    {
        const char* args[] = { "prog", "-x", "7" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 1);
        ARCHON_CHECK_EQUAL(n_2, 0);
    }
    {
        const char* args[] = { "prog", "-y", "8" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 1);
        ARCHON_CHECK_EQUAL(n_2, 1);
    }
    {
        const char* args[] = { "prog", "7" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_NOT_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 0);
    }
    {
        const char* args[] = { "prog", "-x", "x" };
        n_1 = n_2 = 0;
        int exit_status = test<proc_type>(test_context, args, setup);
        ARCHON_CHECK_NOT_EQUAL(exit_status, 0);
        ARCHON_CHECK_EQUAL(n_1, 0);
    }
}
