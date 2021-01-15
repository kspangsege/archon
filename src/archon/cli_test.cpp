#include <iostream>

#include <archon/cli.hpp>


using namespace archon;


int main(int argc, char* argv[])
{
    std::locale locale = std::locale("");

    cli::CommandLine command_line(argc, argv); // Throws

    long width = 80;

    cli::WideSpec spec(locale); // Throws

    // FIXME: Explain implicit option specifications and the the associated rules                  

    // FIXME: Make sure it is impossible for a pattern to refer to an option form that is also mentioned in a short circuit option specification.                  

    opt("-h --help", "", cli::short_circuit, spec,
        "Show command synopsis and the list of available options.",
        [&] {
            spec.show_help(command_line, std::wcout, width); // Throws
        }); // Throws
    opt("-w --width", "<num>", cli::no_attributes, spec,
        "Format command-line help to a line length of @N (default is @V).",
        cli::assign(width)); // Throws

    pat("<file>...", spec,
        "Files.",
        []() {
            std::cerr << "Files\n"; // Throws
        }); // Throws
}
