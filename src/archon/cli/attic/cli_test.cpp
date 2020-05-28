#include <locale>
#include <iostream>

#include <archon/core/features.h>
#include <archon/cli.hpp>


using namespace archon;


namespace {

int move(const cli::WideCommandLine& command_line)
{
    cli::WideProcessor proc(command_line); // Throws

    opt("-h --help", "", cli::short_circuit, proc,
        "Show command synopsis and the list of available options.",
        [&] {
            proc.show_help(std::wcout, 80); // Throws
        }); // Throws

    {
        int exit_status = 0;
        if (ARCHON_UNLIKELY(proc.process(exit_status))) // Throws
            return exit_status;
    }

    std::wcout << "MOVE\n";

    return EXIT_SUCCESS;
}

} // unnamed namespace


int main(int argc, char* argv[])
{
    std::locale::global(std::locale("")); // Throws

    long width = 80;

    cli::WideProcessor proc(argc, argv); // Throws

    pat("[<file>...]", proc,
        "Files.",
        [] {
            std::cerr << "Files\n"; // Throws
        }); // Throws
    pat("(-m | --move)", proc,
        "Move 1.", &move); // Throws
    pat("-k <foo> <bar>", proc,
        "K."); // Throws

    opt("--", "", cli::further_args_are_values, proc,
        "Do not interpret subsequent command-line arguments as options (or keywords), "
        "even if they look like options (or keywords).",
        [&] {
            std::wcout << "SEP\n"; // Throws
        }); // Throws
    opt("-h --help", "", cli::short_circuit, proc,
        "Show command synopsis and the list of available options.",
        [&] {
            proc.show_help(std::wcout, width); // Throws
        }); // Throws
    opt("-w --width", "<num>", cli::no_attributes, proc,
        "Format command-line help to a line length of @A (default is @V).",
        cli::assign(width)); // Throws
    opt("-d --debug", "", cli::no_attributes, proc,
        "Debug."); // Throws

    {
        int exit_status = 0;
        if (ARCHON_UNLIKELY(!proc.process(exit_status))) // Throws
            return exit_status;
    }

    std::wcout << "GOOD ("<<width<<")\n";
}
