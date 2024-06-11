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


#include <cstdlib>
#include <utility>
#include <optional>
#include <filesystem>
#include <iostream>

#include <archon/core/filesystem.hpp>
#include <archon/core/build_mode.hpp>
#include <archon/core/platform_info.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text_file_stream.hpp>
#include <archon/cli.hpp>
#include <archon/check/test_config.hpp>
#include <archon/check/test_runner.hpp>
#include <archon/check/command.hpp>
#include <archon/check/standard_path_mapper.hpp>
#include <archon/check/pattern_based_test_order.hpp>
#include <archon/check/wildcard_filter.hpp>
#include <archon/check/simple_reporter.hpp>
#include <archon/check/duplicating_reporter.hpp>
#include <archon/check/xml_reporter.hpp>


using namespace archon;


int check::command(std::string_view label, int argc, char* argv[],
                   const core::BuildEnvironment::Params& build_env_params,
                   core::Span<const std::string_view> test_order, const std::locale& locale)
{
    std::cerr << "----> CLICK 2\n";
    cli::WideStringHolder string_holder;

    bool report_progress = false;
    std::string_view filter;
    check::TestConfig test_config;
    bool xml = false;
    std::string_view xml_path = "test_results.xml";
    std::string_view suite_name = "default";
    bool describe_build_env = false;

    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws
    test_config.data_file_base_dir = build_env.get_relative_source_root(); // Throws
    test_config.log_file_base_dir  = build_env.get_relative_project_root(); // Throws
    test_config.test_file_base_dir = build_env.get_relative_project_root(); // Throws

    cli::WideSpec spec;
    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws
    opt("-p, --progress", "", cli::no_attributes, spec,
        "Log a message for each test case that starts to execute.",
        cli::raise_flag(report_progress)); // Throws
    opt("-f, --filter", "<string>", cli::no_attributes, spec,
        "Only the test cases that are matched by the specified filter (@A) will be executed. A filter specification "
        "consists of zero or more \"words\" separated by one or more spaces. Leading and trailing spaces are ignored. "
        "Except for the special word, \"-\"; each word is a wildcard pattern where the special character, \"*\", is a "
        "widcard that matches any substring of any size. A pattern may contain any number of wildcards. A test case "
        "is matched by a pattern if the pattern matches the complete name of that test case (a match on a substring "
        "is not enough). Patterns occurring before \"-\", or in a filter without \"-\" are *positive* patterns. "
        "Patterns occurring after \"-\" are *negative* patterns. At most one \"-\" is allowed in a filter "
        "specification. As a special rule, if the filter does not explicitly specify any positive patterns, it is "
        "understood as having one positive pattern matching everything, in addition to any negative patterns that it "
        "may have. Finally, a particular test case is matched by the filter if, and only if it is matched by at least "
        "one of the positive patterns but not by any of the negative patterns. For example, \"Util_*\" matches all "
        "test cases whose names start with \"Util_\", and \"- Foo Bar\" matches all test cases except the two named "
        "\"Foo\" and \"Bar\". The default filter is @Q.",
        cli::assign(filter)); // Throws
    opt("-n, --num-repetitions", "<num>", cli::no_attributes, spec,
        "The number of times to repeat the execution of each of the selected and enabled test cases (see --filter). "
        "The default number is @V.",
        cli::assign_c(test_config.num_repetitions, [](auto num) {
            return num >= 0;
        })); // Throws
    opt("-t, --num-threads", "<num>", cli::no_attributes, spec,
        "The maximum number of threads that will be used to execute test cases. This is therefore also the maximum "
        "number of test cases that will be able to execute in parallel. If the specified value is zero, a platform "
        "dependent number of threads is used. The default value is @V.",
        cli::assign_c(test_config.num_threads, [](auto num) {
            return num >= 0;
        })); // Throws
    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "The log level limit to apply to logging coming from inside test cases. The possible levels are \"off\", "
        "\"fatal\", \"error\", \"warn\", \"info\", \"detail\", \"debug\", \"trace\", and \"all\". Set it to \"off\" "
        "to completely disable logging from inside test cases. The default limit is \"@V\".",
        cli::assign(test_config.inner_log_level_limit)); // Throws
    opt("-s, --shuffle", "", cli::no_attributes, spec,
        "Randomize the order in which test cases are started. This randomization is also controlled by the seed "
        "specified though --seed or --random.",
        cli::raise_flag(test_config.shuffle)); // Throws
    opt("-r, --random", "[<num blocks>]", cli::no_attributes, spec,
        "Generate a nondeterministically randomized base random seed (see also --seed). The size of the generated "
        "seed is controlled by specifying the number of 192-bit blocks (through @A). If the number of blocks is not "
        "specified, it defaults to @W. An optimal seeding requires 104 blocks (19968 bits). This is due to the large "
        "state of the Mersenne Twister generator. Each occurrence of --random overrides any earlier occurrences of "
        "both --random and --seed.",
        // The current default value of 2 for the number of blocks is chosen to
        // make it managable to work with seeds as log output and as a
        // command-line parameter. A two-block seed is 67 characters long.
        cli::exec([&](std::size_t num_blocks) {
            test_config.random_seed = check::RandomSeed::random(num_blocks); // Throws
        }, 2)); // Throws
    opt("-S, --seed", "<string>", cli::no_attributes, spec,
        "The base random seed that controls the seeds offered to test cases. The seed offered to a particular "
        "execution of a test case is a function of two things; the base seed as specified here and the repetition "
        "ordinal number, i.e., a number between 1 and the N, where N is the requested number of repetitions "
        "(--num-repetitions). Thus, each repetition of a test case generally gets a different seed. To execute a test "
        "case with the same random seed as a particular execution of that test case during an earlier execution of "
        "the test suite, be sure to set both the correct base seed (--seed) and the correct repetition ordinal number "
        "(--seed-rep-no). The base seed is logged during each test suite execution. If the requested number of "
        "repetitions is greater than 1, the repetition number of a particular execution is indicated by the number "
        "following the hash mark in the test case execution identifiers (e.g., \"Foo#723\") logged during test suite "
        "execution. To run with a nondeterministically randomized seed, use --random. Each occurrence of "
        "--seed overrides any earlier occurrences of both --random and --seed. The default base seed is @Q.",
        cli::assign(test_config.random_seed)); // Throws
    opt("-R, --seed-rep-no", "<num>", cli::no_attributes, spec,
        "Set to a nonzero value in order to override the actual repetition ordinal number as used in the "
        "determination of the random seed to be offered to test cases (see --seed). When zero, the actual repetition "
        "ordinal number is used. When nonzero, all repetitions of a test case will be offered the same random seed "
        "based on the specified value. The default value is @V.",
        cli::assign_c(test_config.rseed_rep_no_override, [](auto num) {
            return num > 0;
        })); // Throws
    opt("-x, --xml", "", cli::no_attributes, spec,
        "Save the test results in an XML file (see also --xml-path and --suite-name). The XML schema is compatible "
        "with UnitTest++.",
        cli::raise_flag(xml)); // Throws
    opt("-m, --xml-path", "<path>", cli::no_attributes, spec,
        "The filesystem path to where the generated XML file must be placed (see --xml). If the specified path is not "
        "absolute, it will be understood as relative to the current working directory. The path must refer to a file "
        "in a directory that already exists. The default path is @Q.",
        cli::assign(xml_path)); // Throws
    opt("-u, --suite-name", "<name>", cli::no_attributes, spec,
        "The name of the executed test suite as used in the generated XML file (see --xml). The default name is @Q.",
        cli::assign(suite_name)); // Throws
    opt("-k, --keep-test-files", "", cli::no_attributes, spec,
        "Disable automatic removal of test files. When not disabled, all test files created during a test case "
        "execution will be automatically removed unless the execution of the test case ends abnormally (an exception "
        "is thrown). When disabled, the files will be left behind inside the test file directory (see "
        "--test-file-base-dir).",
        cli::raise_flag(test_config.keep_test_files)); // Throws
    opt("-T, --log-timestamps", "", cli::no_attributes, spec,
        "Add timestamps to logged messages. If file logging is enabled (--log-to-files), timestamps will be added "
        "only to the messages that are logged to files. Otherwise, timestamps will be added to the messages that are "
        "logged to STDOUT.",
        cli::raise_flag(test_config.log_timestamps)); // Throws
    opt("-F, --log-to-files", "", cli::no_attributes, spec,
        "By default, log messages are sent to STDOUT. With this option, log messages are instead sent to a file. Each "
        "thread will send messages to a separate log file. See --num-threads, --log-path-template, and "
        "--log-file-base-dir. Log messages that are not specific to a particular thread will still be sent to STDOUT. "
        "The files will be opened in \"append\" mode.",
        cli::raise_flag(test_config.log_to_files)); // Throws
    opt("-a, --abort-on-failure", "", cli::no_attributes, spec,
        "Abort the testing process as soon as a check fails or an unexpected exception is thrown in a test case.",
        cli::raise_flag(test_config.abort_on_failure)); // Throws
    opt("-d, --data-file-base-dir", "<path>", cli::no_attributes, spec,
        "The base directory for data files. These are the data files that are supposed to be available to test cases "
        "as immutable fixtures. This base directory must be the root of the source file directory structure, or the "
        "root of a reflection of the source file directory structure in which all relevant data files are present. "
        "Test cases can locate the data files by specifying relative paths. Those relative paths will be resolved "
        "against this base directory. If the specified base directory path is not absolute, it will be understood as "
        "relative to the current working directory. Therefore, if the specified path is empty, it effectively refers "
        "to the current working directory. It makes no difference whether the specified path has a final directory "
        "separator (\"/\") as long as the path would be nonempty without one. The default path is @Q.",
        cli::assign(test_config.data_file_base_dir)); // Throws
    opt("-P, --log-path-template", "<string>", cli::no_attributes, spec,
        "The template used to generate filesystem paths for log files (see --log-to-files). It will be separately "
        "expanded for each test thread. The path must be on relative form, and will be resolved against the log file "
        "base directory (--log-file-base-dir). Any directories explicitely mentioned in the specified path will be "
        "created, if they do not already exist. The path must be specified in platform independent form, i.e., using "
        "slashes as directory separator. The following parameters are recognized: \"@@t\" will be replaced by a "
        "seconds-precision timestamp (\"<date>_<time>\"); \"@@T\" will be replaced by a microseconds-precision "
        "timestamp (\"<date>_<time>_<micro seconds>\"); \"@@i\" will be replaced by the test thread index, i.e., a "
        "number in the range 0 -> N-1 where N is the number of test threads (--num-threads); \"@@I\" is like \"@@i\" "
        "but with leading zeroes included; \"@@n\" will be replaced by the test thread number, i.e., a number in the "
        "range 1 -> N where N is the number of test threads (--num-threads); \"@@N\" is like \"@@n\" but with leading "
        "zeroes included; and finally, \"@@@@\" is replaced by \"@@\". The default template is @Q.",
        cli::assign(test_config.log_path_template)); // Throws
    opt("-L, --log-file-base-dir", "<path>", cli::no_attributes, spec,
        "The base directory for log files (see --log-to-files and --log-path-template). If the specified path is not "
        "absolute, it will be understood as relative to the current working directory. Therefore, if the specified "
        "path is empty, it effectively refers to the current working directory. It makes no difference whether the "
        "specified path has a final directory separator (\"/\") as long as the path would be nonempty without one. "
        "The directory referred to by the specified path must already exist. The default path is @Q.",
        cli::assign(test_config.log_file_base_dir)); // Throws
    opt("-e, --test-file-subdir", "<path>", cli::no_attributes, spec,
        "The subdirectory in which to place the files and directories created by test cases. The path must be on "
        "relative form, and will be resolved against the test file base directory (--test-file-base-dir). Specifying "
        "an empty path causes test files and directories to be placed in the base directory. Any directories "
        "explicitely mentioned in the specified path will be created, if they do not already exist. The path must be "
        "specified in platform independent form, i.e., using slashes as directory separator. It makes no difference "
        "whether the specified path has a final directory separator as long as the path would be nonempty without "
        "one. The default subdirectory path is @Q.",
        cli::assign(test_config.test_file_subdir)); // Throws
    opt("-E, --test-file-base-dir", "<path>", cli::no_attributes, spec,
        "The base directory for test files (see also --test-file-subdir). If the specified path is not absolute, it "
        "will be understood as relative to the current working directory. Therefore, if the specified path is empty, "
        "it effectively refers to the current working directory. It makes no difference whether the specified path "
        "has a final directory separator (\"/\") as long as the path would be nonempty without one. The directory "
        "referred to by the specified path must already exist. The default path is @Q.",
        cli::assign(test_config.test_file_base_dir)); // Throws
    opt("-b, --describe-build-env", "", cli::no_attributes, spec,
        "Describe detected build environment.",
        cli::raise_flag(describe_build_env)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, string_holder, locale))) // Throws
        return exit_status;

    check::WildcardFilter filter_2(filter, locale); // Throws
    test_config.filter = &filter_2;

    check::PatternBasedTestOrder test_order_2(test_order, locale); // Throws
    test_config.test_order = &test_order_2;

    check::SimpleReporter reporter(report_progress);
    test_config.reporter = &reporter;

    struct XmlExtras {
        core::TextFileStream stream;
        check::XmlReporter xml_reporter;
        check::DuplicatingReporter duplicating_reporter;
        XmlExtras(core::FilesystemPathRef path, check::Reporter& reporter, std::string_view suite_name)
            : stream(path, core::File::Mode::write) // Throws
            , xml_reporter(stream, suite_name) // Throws
            , duplicating_reporter(reporter, xml_reporter) // Throws
        {
        }
    };
    std::optional<XmlExtras> xml_extras;
    if (xml) {
        namespace fs = std::filesystem;
        fs::path path = core::make_fs_path_native(xml_path, {}); // Throws
        xml_extras.emplace(path, reporter, suite_name); // Throws
        xml_extras->stream.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
        xml_extras->stream.imbue(locale); // Throws
        test_config.reporter = &xml_extras->duplicating_reporter;
    }

    check::StandardPathMapper source_path_mapper(build_env);
    test_config.source_path_mapper = &source_path_mapper;

    check::TestRunner runner(locale, std::move(test_config)); // Throws
    log::Logger& logger = runner.get_logger();

    logger.info("Testing: %s", label); // Throws
    logger.info("Build mode: " ARCHON_BUILD_MODE_EX); // Throws
    if (describe_build_env)
        logger.info("Build environment: %s", build_env); // Throws
    logger.info("Platform: %s", core::get_platform_description()); // Throws
    logger.info("Random seed: %s", runner.get_config().random_seed); // Throws

    std::cerr << "----> CLICK 3\n";
    bool success = runner.run(); // Throws

    if (xml) {
        xml_extras->stream.flush(); // Throws
        logger.info("Test results saved as: %s", xml_path); // Throws
    }

    if (success)
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}
