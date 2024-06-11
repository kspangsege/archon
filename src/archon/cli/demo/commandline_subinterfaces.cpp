// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <tuple>
#include <string>
#include <locale>
#include <iostream>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/locale.hpp>
#include <archon/cli.hpp>


using namespace archon;


namespace {


int copy_command(const cli::CommandLine& commandline)
{
    namespace fs = std::filesystem;
    fs::path origin_path, target_path;
    cli::Spec spec;
    pat("<origin path>  <target path>", cli::no_attributes, spec, "",
        std::tie(origin_path, target_path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(commandline, spec, exit_status))) // Throws
        return exit_status;

    fs::copy_file(origin_path, target_path); // Throws
    return true; // Success
}


int move_command(const cli::CommandLine& commandline)
{
    namespace fs = std::filesystem;
    fs::path origin_path, target_path;
    cli::Spec spec;

    pat("<origin path>  <target path>", cli::no_attributes, spec, "",
        std::tie(origin_path, target_path)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(commandline, spec, exit_status))) // Throws
        return exit_status;

    fs::rename(origin_path, target_path); // Throws
    return true; // Success
}


int file_command(const cli::CommandLine& commandline)
{
    cli::Spec spec;

    pat("copy", cli::no_attributes, spec,
        "Copy a file.",
        &copy_command); // Throws

    pat("move", cli::no_attributes, spec,
        "Move a file.",
        &move_command); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    return cli::process(commandline, spec); // Throws
}


int echo_command(const cli::CommandLine& commandline)
{
    std::string text;
    cli::Spec spec;

    pat("<text>", cli::no_attributes, spec, "",
        std::tie(text)); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(commandline, spec, exit_status))) // Throws
        return exit_status;

    std::cout << text << "\n"; // Throws
    return true; // Success
}


int main_command(const cli::CommandLine& commandline)
{
    cli::Spec spec;

    pat("file", cli::no_attributes, spec,
        "File operations.",
        &file_command); // Throws

    pat("echo", cli::no_attributes, spec,
        "Echo service.",
        &echo_command); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    return cli::process(commandline, spec); // Throws
}


} // unnamed namespace


int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws
    cli::CommandLine commandline(argc, argv, locale); // Throws
    return main_command(commandline); // Throws
}
