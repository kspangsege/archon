#include <tuple>

namespace cli {



} // namespace cli



// How to:
// - Allow addition of standard help option.
// - Allow compile-time computation of static help text.
// - Allow run-time computation of dynamic help text.
// - Allow wide characters mode.
// - Allow non-C++20 mode.
// - Deal with option groups


int main_static()
{
    auto deleg = [&](const cli::CommandLine&) {
        std::cout << "Deleg\n";
    };

    using cli::str;
    std::tuple spec = {
        pat(str<"foo <val>">,
            str<"Lorem ipsum.">,
            [&](float val) {
                std::cout << "Foo " << val << "\n";
            }),
        pat(str<"bar <val>">,
            str<"Lorem ipsum.">,
            [&](float val) {
                std::cout << "Bar " << val << "\n";
            }),
        pat(str<"deleg">,
            str<"Lorem ipsum.">,
            deleg),
        opt(cli::help_static<"prog", 80>),
        opt(cli::help_dynamic),
        opt(str<"-h, --help">, spec,
            str<"Lorem ipsum.">,
            [](const cli::Processor& proc) {
                proc.show_help();
            });
        opt(str<"-f, --foo">,
            str<"Lorem ipsum.">),
        opt(str<"-b, --bar">,
            str<"Lorem ipsum.">),
        cli::allow_cross_pattern_ambiguity,
        cli::allow_pattern_internal_positional_ambiguity,
    };
    return cli::process(argc, argv, spec);
}


int main_dynamic()
{
    auto deleg = [&](const cli::CommandLine&) {
        std::cout << "Deleg\n";
    };

    cli::SpecConfig config;
    config.allow_cross_pattern_ambiguity = true;
    config.allow_pattern_internal_positional_ambiguity = true;
    cli::Spec spec(config);

    pat("foo <val>", spec,
        "Lorem ipsum.",
        [&](float val) {
            std::cout << "Foo " << val << "\n";
        });
    pat("bar <val>", spec,
        "Lorem ipsum.",
        [&](float val) {
            std::cout << "Bar " << val << "\n";
        });
    pat("deleg", spec,
        "Lorem ipsum.",
        deleg);

    opt(cli::help_dynamic, spec);
    opt("-h, --help", spec,
        "Lorem ipsum.",
        [](const cli::Processor& proc) {
            proc.show_help();
        });
    opt("-f, --foo", spec,
        "Lorem ipsum.");
    opt("-b, --bar", spec,
        "Lorem ipsum.");

    return cli::process(argc, argv, spec);
}
