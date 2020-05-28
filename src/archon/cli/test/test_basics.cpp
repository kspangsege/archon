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


ARCHON_TEST_BATCH(Cli_Basics_NoPattern, proc_variants)
{
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [](spec_type&) {};
    {
        const char* args[] = { "prog" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK_NOT(complete);
    }
    {
        const char* args[] = { "prog", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
    }
}


ARCHON_TEST_BATCH(Cli_Basics_EmptyNoncompletingPattern, proc_variants)
{
    int n = 0;
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [&](spec_type& spec) {
        pat("", cli::no_attributes, spec,
            "Lorem ipsum.",
            [&] {
                ++n;
            });
    };
    {
        n = 0;
        const char* args[] = { "prog" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK_NOT(complete);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        n = 0;
        const char* args[] = { "prog", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
        ARCHON_CHECK_EQUAL(n, 0);
    }
}


ARCHON_TEST_BATCH(Cli_Basics_EmptyCompletingPattern, proc_variants)
{
    int n = 0;
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [&](spec_type& spec) {
        pat("", cli::completing, spec,
            "Lorem ipsum.",
            [&] {
                ++n;
            });
    };
    {
        n = 0;
        const char* args[] = { "prog" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        n = 0;
        const char* args[] = { "prog", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
        ARCHON_CHECK_EQUAL(n, 0);
    }
}


ARCHON_TEST_BATCH(Cli_Basics_SimplPattern, proc_variants)
{
    int n = 0;
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [&](spec_type& spec) {
        pat("<val>", cli::no_attributes, spec,
            "Lorem ipsum.",
            [&](int foo) {
                ++n;
                ARCHON_CHECK_EQUAL(foo, 7);
            });
    };
    {
        const char* args[] = { "prog", "7" };
        n = 0;
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK_NOT(complete);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        n = 0;
        const char* args[] = { "prog", "7", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
        ARCHON_CHECK_EQUAL(n, 0);
    }
    {
        n = 0;
        const char* args[] = { "prog", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
        ARCHON_CHECK_EQUAL(n, 0);
    }
}


ARCHON_TEST_BATCH(Cli_Basics_KeywordPattern, proc_variants)
{
    int n = 0;
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [&](spec_type& spec) {
        pat("foo <val>", cli::no_attributes, spec,
            "Lorem ipsum.",
            [&](int foo) {
                ++n;
                ARCHON_CHECK_EQUAL(foo, 7);
            });
    };
    {
        n = 0;
        const char* args[] = { "prog", "foo", "7" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK_NOT(complete);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        n = 0;
        const char* args[] = { "prog", "foo", "7", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
        ARCHON_CHECK_EQUAL(n, 0);
    }
    {
        n = 0;
        const char* args[] = { "prog", "foo", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
        ARCHON_CHECK_EQUAL(n, 0);
    }
}


ARCHON_TEST_BATCH(Cli_Basics_OptionPattern, proc_variants)
{
    int n = 0;
    using proc_type = test_type;
    using spec_type = typename proc_type::spec_type;
    auto setup = [&](spec_type& spec) {
        pat("-x <val>", cli::no_attributes, spec,
            "Lorem ipsum.",
            [&](int foo) {
                ++n;
                ARCHON_CHECK_EQUAL(foo, 7);
            });
    };
    {
        n = 0;
        const char* args[] = { "prog", "-x", "7" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK_NOT(complete);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        n = 0;
        const char* args[] = { "prog", "-x", "7", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
        ARCHON_CHECK_EQUAL(n, 0);
    }
    {
        n = 0;
        const char* args[] = { "prog", "-x", "x" };
        int exit_status = -1;
        bool complete = test<proc_type>(test_context, args, setup, exit_status);
        ARCHON_CHECK(complete);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_FAILURE);
        ARCHON_CHECK_EQUAL(n, 0);
    }
}


// FIXME: Add Cli_Basics_MultiplePatterns                                                  


/*
ARCHON_TEST(Cli_Foo)
{
    Processor proc;

    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Sym>("<a>"), std::make_unique<Sym>("<b>")), [&](int a, int b) {
        log("CLICK (%s, %s)", a, b);
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Rep>(std::make_unique<Sym>("<a>")), std::make_unique<Rep>(std::make_unique<Sym>("<b>"))), [&](std::vector<int>, std::vector<int>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("<a>"), std::make_unique<Sym>("<b>")), [&](std::variant<int, float>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("-a")), [&]() {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("<a>")), [&](int i) {
        log("A<%s>", i);
        return EXIT_SUCCESS;
    }, test_context.logger);
    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Sym>("-b"), std::make_unique<Sym>("<b>")), [&](int i) {
        log("B<%s>", i);
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Alt>(std::make_unique<Cat>(std::make_unique<Sym>("<a1>"), std::make_unique<Cat>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("<a2>"))),
                                           std::make_unique<Cat>(std::make_unique<Sym>("<b1>"), std::make_unique<Cat>(std::make_unique<Sym>("-b"), std::make_unique<Sym>("<b2>")))), [&](std::variant<std::pair<int, int>, std::pair<int, int>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Sym>("<a>"), std::make_unique<Rep>(std::make_unique<Sym>("<b>"))), [&](int, std::vector<int>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Opt>(std::make_unique<Sym>("<a>"))), [&]() {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Sym>("<a>")), [&](std::vector<int> vec) {
        log("CLICK %s", core::as_sbr_list(vec));
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Opt>(std::make_unique<Sym>("<b>")), [&](std::optional<int>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Sym>("<a>"), [&](int) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Cat>(std::make_unique<Opt>(std::make_unique<Sym>("-a")),
                                           std::make_unique<Opt>(std::make_unique<Sym>("-b"))), [&]() {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Opt>(std::make_unique<Sym>("<a>"))), [&]() {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);


    // Test all element types: T, std::optional<T>, std::vector<T>, std::variant<T...>

    // Test all tuple-like types: std::monostate, T, std::pair<T, U>, std::tuple<T...>, std::array<T, N>

    // Test collapsing optionality with repetition inside


    // Test case (collapsed param): foo [-x]
    // Test case (collapsed param): foo -x...
    // Test case (collapsed param): foo [-x...]
    // Test case (collapsed param): foo (-a | -b | -c)

    // Test case (collapsed param): <val> [-x]
    // Test case (collapsed param): <val> -x...
    // Test case (collapsed param): <val> [-x...]
    // Test case (collapsed param): <val> (-a | -b | -c)

    // Test case (collapsed param): [foo [-x]]
    // Test case (collapsed param): [foo -x...]
    // Test case (collapsed param): [foo [-x...]]
    // Test case (collapsed param): [foo (-a | -b | -c)]

    // Test case (collapsed param): [<val> [-x]]
    // Test case (collapsed param): [<val> -x...]
    // Test case (collapsed param): [<val> [-x...]]
    // Test case (collapsed param): [<val> (-a | -b | -c)]

    // Test case (collapsed param): (foo [-x])...
    // Test case (collapsed param): (foo -x...])...
    // Test case (collapsed param): (foo [-x...])...
    // Test case (collapsed param): (foo (-a | -b | -c))...

    // Test case (collapsed param): (<val> [-x])...
    // Test case (collapsed param): (<val> -x...)...
    // Test case (collapsed param): (<val> [-x...])...
    // Test case (collapsed param): (<val> (-a | -b | -c))...


    // Test case (collapsed param): (<foo> [-x])...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Opt>(std::make_unique<Sym>("-x")))), [&](std::vector<std::pair<int, bool>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    // Test case (collapsed param): (<foo> -x...)...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Rep>(std::make_unique<Sym>("-x")))), [&](std::vector<std::pair<int, std::size_t>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    // Test case (collapsed param): (<foo> [-a...])...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Opt>(std::make_unique<Rep>(std::make_unique<Sym>("-x"))))), [&](std::vector<std::pair<int, std::size_t>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    // Test case (collapsed param): (<foo> (-a | -b | -c))...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Alt>(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("-b")), std::make_unique<Sym>("-c")))), [&](std::vector<std::pair<int, std::size_t>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    // Test case (collapsed param): (<foo> [-a | -b | -c])...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Opt>(std::make_unique<Alt>(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("-b")), std::make_unique<Sym>("-c"))))), [&](std::vector<std::pair<int, std::optional<std::size_t>>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);

    // Test case (collapsed param): (<foo> (-a | -b | -c)...)...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Sym>("<foo>"), std::make_unique<Rep>(std::make_unique<Alt>(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Sym>("-b")), std::make_unique<Sym>("-c"))))), [&](std::vector<std::pair<int, std::vector<std::size_t>>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);



    // Test case: ([<a>] -x [<b>])...
    proc.add_pattern(std::make_unique<Rep>(std::make_unique<Cat>(std::make_unique<Cat>(std::make_unique<Opt>(std::make_unique<Sym>("<a>")), std::make_unique<Sym>("-x")), std::make_unique<Opt>(std::make_unique<Sym>("<b>")))), [&](std::vector<std::pair<std::optional<int>, std::optional<int>>>) {
        log("CLICK");
        return EXIT_SUCCESS;
    }, test_context.logger);


    // Test case: -a | [-b]
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Opt>(std::make_unique<Sym>("-b"))), [&](std::variant<std::monostate, bool> var) {
            ++n;
            const bool* ptr = std::get_if<1>(&var);
            ARCHON_CHECK(ptr && !*ptr);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({}, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Opt>(std::make_unique<Sym>("-b"))), [&](std::variant<std::monostate, bool> var) {
            ++n;
            const std::monostate* ptr = std::get_if<0>(&var);
            ARCHON_CHECK(ptr);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({ "-a" }, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Alt>(std::make_unique<Sym>("-a"), std::make_unique<Opt>(std::make_unique<Sym>("-b"))), [&](std::variant<std::monostate, bool> var) {
            ++n;
            const bool* ptr = std::get_if<1>(&var);
            ARCHON_CHECK(ptr && *ptr);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({ "-b" }, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }

    // Test case: -x...
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Rep>(std::make_unique<Sym>("-x")), [&](std::size_t m) {
            ++n;
            ARCHON_CHECK_EQUAL(m, 3);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({ "-x", "-x", "-x" }, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
    {
        std::size_t n = 0;
        Processor proc;
        proc.add_pattern(std::make_unique<Rep>(std::make_unique<Sym>("-x")), [&](std::vector<std::monostate> v) {
            ++n;
            ARCHON_CHECK_EQUAL(v.size(), 3);
            return EXIT_SUCCESS;
        }, test_context.logger);
        int exit_status = proc.process({ "-x", "-x", "-x" }, test_context.logger);
        ARCHON_CHECK_EQUAL(exit_status, EXIT_SUCCESS);
        ARCHON_CHECK_EQUAL(n, 1);
    }
}
*/
