#include <cstdlib>
#include <optional>
#include <locale>
#include <iostream>

#include <archon/base/value_parser.hpp>
#include <archon/base/build_mode.hpp>
#include <archon/base/build_environment.hpp>
#include <archon/base/platform_info.hpp>
#include <archon/base/filesystem.hpp>
#include <archon/unit_test/run.hpp>
#include <archon/unit_test/wildcard_filter.hpp>
#include <archon/unit_test/pattern_based_test_order.hpp>
#include <archon/unit_test/simple_reporter.hpp>
#include <archon/unit_test/duplicating_reporter.hpp>
#include <archon/unit_test/xml_reporter.hpp>
#include <archon/unit_test/standard_path_mapper.hpp>


using namespace archon;


namespace {


std::string_view test_order[] = {
    // When choosing order, please try to follow these guidelines:
    //
    //  - If feature A depends on feature B, test feature B first.
    //
    //  - If feature A has a more central role than feature B, test feature A
    //    first.
    //
    "*",
};


} // unnamed namespace


int main(int argc, char* argv[])
{
//    std::locale::global(std::locale(""));

    std::string_view argv0 = argv[0];
    base::BuildEnvironment build_env;
    {
        base::BuildEnvironment::Config config;
        config.argv0     = argv0;
        config.file_path = __FILE__;
        config.bin_path  = "archon/test"; // Relative to build reflection of source root
        config.src_path  = "archon/test.cpp"; // Relative to source root
        config.src_root  = "src"; // Relative to project root
        build_env = base::BuildEnvironment(config); // Throws
    }

    bool report_progress = false;
    unit_test::TestConfig test_config;
    test_config.data_root_dir = build_env.get_relative_source_root(); // Throws
    test_config.log_file_base_dir = build_env.get_relative_project_root(); // Throws
    test_config.test_file_base_dir = build_env.get_relative_project_root(); // Throws
    std::string_view filter;
    bool xml = false;
    std::string_view suite_name = "default";

    // Process command line
    --argc;
    ++argv;
    {
        bool error   = false;
        bool help    = false;
        int argc_2 = 0;
        int i = 0;
        char* arg = nullptr;
        base::ValueParser parser;
        auto get_value_c = [&](auto& var, auto check_val) {
            if (ARCHON_LIKELY(argc > i)) {
                std::string_view str = argv[i++];
                using T = base::RemoveOptional<std::decay_t<decltype(var)>>;
                T val {};
                bool good_val = parser.parse(str, val) && check_val(val); // Throws
                if (ARCHON_LIKELY(good_val)) {
                    var = val;
                    return true;
                }
            }
            return false;
        };
        auto get_value = [&](auto& var) {
            return get_value_c(var, [](auto) { return true; }); // Throws
        };
        while (i < argc) {
            arg = argv[i++];
            if (arg[0] != '-') {
                argv[argc_2++] = arg;
                continue;
            }
            if (std::strcmp(arg, "-h") == 0 || std::strcmp(arg, "--help") == 0) {
                help = true;
                continue;
            }
            else if (std::strcmp(arg, "-p") == 0 || std::strcmp(arg, "--progress") == 0) {
                report_progress = true;
                continue;
            }
            else if (std::strcmp(arg, "-n") == 0 || std::strcmp(arg, "--num-repetitions") == 0) {
                if (get_value(test_config.num_repetitions)) // Throws
                    continue;
            }
            else if (std::strcmp(arg, "-t") == 0 || std::strcmp(arg, "--num-threads") == 0) {
                if (get_value(test_config.num_threads)) // Throws
                    continue;
            }
            else if (std::strcmp(arg, "-f") == 0 || std::strcmp(arg, "--filter") == 0) {
                if (get_value(filter)) // Throws
                    continue;
            }
            else if (std::strcmp(arg, "-l") == 0 || std::strcmp(arg, "--log-level") == 0) {
                if (get_value(test_config.inner_log_level_limit)) // Throws
                    continue;
            }
            else if (std::strcmp(arg, "-s") == 0 || std::strcmp(arg, "--seed") == 0) {
                if (get_value(test_config.random_seed)) // Throws
                    continue;
            }
            else if (std::strcmp(arg, "-r") == 0 || std::strcmp(arg, "--random") == 0) {
                std::size_t num_blocks = 4; // The default number of 4 is chosen to make it managable as log output and as a command-line parameter.                  
                if (argc == i || get_value(num_blocks)) { // Throws
                    test_config.random_seed = unit_test::RandomSeed::random(num_blocks); // Throws
                    continue;
                }
            }
            else if (std::strcmp(arg, "-S") == 0 || std::strcmp(arg, "--shuffle") == 0) {
                test_config.shuffle = true;
                continue;
            }
            else if (std::strcmp(arg, "-k") == 0 || std::strcmp(arg, "--keep-test-files") == 0) {
                test_config.keep_test_files = true;
                continue;
            }
            else if (std::strcmp(arg, "-T") == 0 || std::strcmp(arg, "--log-timestamps") == 0) {
                test_config.log_timestamps = true;
                continue;
            }
            else if (std::strcmp(arg, "-F") == 0 || std::strcmp(arg, "--log-to-files") == 0) {
                test_config.log_to_files = true;
                continue;
            }
            else if (std::strcmp(arg, "-a") == 0 || std::strcmp(arg, "--abort-on-failure") == 0) {
                test_config.abort_on_failure = true;
                continue;
            }
            else if (std::strcmp(arg, "-d") == 0 || std::strcmp(arg, "--data-dir") == 0) {
                std::string_view path;
                if (get_value(path)) { // Throws
                    test_config.data_root_dir =
                        base::make_fs_path_native(path, test_config.locale); // Throws
                    continue;
                }
            }
            else if (std::strcmp(arg, "-p") == 0 || std::strcmp(arg, "--log-path-template") == 0) {
                if (get_value(test_config.log_path_template)) // Throws
                    continue;
            }
            else if (std::strcmp(arg, "-L") == 0 || std::strcmp(arg, "--log-file-base-dir") == 0) {
                std::string_view path;
                if (get_value(path)) { // Throws
                    test_config.log_file_base_dir =
                        base::make_fs_path_native(path, test_config.locale); // Throws
                    continue;
                }
            }
            else if (std::strcmp(arg, "-e") == 0 || std::strcmp(arg, "--test-file-subdir") == 0) {
                if (get_value(test_config.test_file_subdir)) // Throws
                    continue;
            }
            else if (std::strcmp(arg, "-E") == 0 || std::strcmp(arg, "--test-file-base-dir") == 0) {
                std::string_view path;
                if (get_value(path)) { // Throws
                    test_config.test_file_base_dir =
                        base::make_fs_path_native(path, test_config.locale); // Throws
                    continue;
                }
            }
            else if (std::strcmp(arg, "-u") == 0 || std::strcmp(arg, "--suite-name") == 0) {
                if (get_value(suite_name)) // Throws
                    continue;
            }
            else {
                std::cerr <<
                    "ERROR: Unknown option `" << arg << "`\n"; // Throws
                error = true;
                continue;
            }
            std::cerr <<
                "ERROR: Bad or missing value for option `" << arg << "`\n"; // Throws
            error = true;
        }
        argc = argc_2;

        i = 0;
        if (i != argc) {
            std::cerr <<
                "ERROR: Too many command line arguments\n";
            error = true;
        }

        if (help) {
            std::cerr <<
                "Synopsis: " << argv0 << "\n"
                "\n"
                "Options:\n"
                "  -h, --help           Display command-line synopsis followed by the list of\n"
                "                       available options.\n"
                "  -p, --progress\n"
                "  -n, --num-repetitions INTEGER\n"
                "  -t, --num-threads INTEGER\n"
                "  -f, --filter STRING\n"
                "  -l, --log-level STRING\n"
                "  -s, --seed STRING\n"
                "  -r, --random [INTEGER]\n"
                "  -S, --shuffle\n"
                "  -k, --keep-test-files\n"
                "  -T, --log-timestamps\n"
                "  -F, --log-to-files\n"
                "  -a, --abort-on-failure\n"
                "  -d, --data-dir PATH\n"
                "  -p, --log-path-template STRING\n"
                "  -L, --log-file-base-dir PATH\n"
                "  -e, --test-file-subdir PATH\n"
                "  -E, --test-file-base-dir PATH\n"
                "  -u, --suite-name STRING\n";
            return EXIT_SUCCESS;
        }

        if (error) {
            std::cerr <<
                "ERROR: Bad command line.\n"
                "Try `" << argv0 << " --help`\n";
            return EXIT_FAILURE;
        }
    }

    std::cout << "Build mode: " ARCHON_BUILD_MODE_EX "\n";
    std::cout << "Platform: " << base::get_platform_description() << "\n";
    std::cout << "Random seed: " << test_config.random_seed << "\n";

    unit_test::WildcardFilter filter_2(filter);
    test_config.filter = &filter_2;

    unit_test::PatternBasedTestOrder test_order_2(test_order);
    test_config.test_order = &test_order_2;

    unit_test::SimpleReporter reporter(report_progress);
    test_config.reporter = &reporter;

    struct XmlExtras {
        unit_test::XmlReporter xml_reporter;
        unit_test::DuplicatingReporter duplicating_reporter;
        XmlExtras(unit_test::Reporter& reporter, std::string_view suite_name) :
            // FIXME: Need a FileStream here                                          
            xml_reporter(std::cout, suite_name), // Throws       
            duplicating_reporter(reporter, xml_reporter) // Throws
        {
        }
    };
    std::optional<XmlExtras> xml_extras;
    if (xml) {
        xml_extras.emplace(reporter, suite_name); // Throws
        test_config.reporter = &xml_extras->duplicating_reporter;
    }

    unit_test::StandardPathMapper source_path_mapper(build_env);
    test_config.source_path_mapper = &source_path_mapper;

    bool success = unit_test::run(std::move(test_config)); // Throws
    if (success)
        return EXIT_SUCCESS;
    return EXIT_FAILURE;
}
