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

#include <iostream>

#include <archon/core/archon_version.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/text_file_stream.hpp>    
#include <archon/log.hpp>    
#include <archon/check/command.hpp>


using namespace archon;


namespace {


// When choosing order, follow these rough guidelines:
//
//  - If feature A depends on feature B, then test feature B first.
//
//  - If feature A has a more central role than feature B, then test feature A first.
//
/*
constexpr std::string_view test_order[] = {
    "Core_*",
    "Log_*",
    "Cli_*",
    "Check_*",
};
*/


} // unnamed namespace


int main()
{
/*
    core::BuildEnvironment::Params params;
    params.file_path = __FILE__;
    params.bin_path  = "test"; // Relative to build reflection of source root
    params.src_path  = "test.cpp"; // Relative to source root
    params.src_root  = "src"; // Relative to project root
    params.source_from_build_path = core::archon_source_from_build_path;
*/

//    std::locale locale = core::get_default_locale(); // Throws

//    log::info("GLOBAL LOGGER");                          

/*
    core::File::get_cout().write(std::string_view("=======================>>>> RAW BYTE-LEVEL WRITE\n"));                  

    core::TextFileStream stream(&core::File::get_cout());   
    stream << "=======================>>>> TEXT-LEVEL WRITE" << std::endl;     
*/

    std::locale loc = std::locale::classic();
    log::FileLogger logger(core::File::get_cout(), loc);      

/*
    {
        std::cerr << "----> X - 1\n";   
        std::lock_guard(logger.m_mutex);  
        std::cerr << "----> X - 2\n";   
    }
    std::cerr << "----> X - 3\n";   
*/

//    logger.m_channel.channel_log(log::LogLevel::info, *logger.m_prefix, "FILE LOGGER");                          
//    logger.m_channel.m_sink.sink_log(log::LogLevel::info, logger.m_channel.m_prefix, *logger.m_prefix, "FILE LOGGER");                          

    std::array<char, 2048> seed_memory;
    core::StringFormatter formatter(seed_memory, loc);
/*
    std::string_view message = formatter.format("FILE LOGGER 2");
    static_cast<void>(message);
*/
    static_cast<void>(formatter);
    static_cast<void>(seed_memory);
/*
    {
        std::cerr << "repl - 1 ("<<static_cast<void*>(&m_mutex)<<")\n";    
        std::lock_guard lock(logger.m_channel.m_sink.m_mutex);    
        std::cerr << "repl - 2\n";    
    }
    std::cerr << "repl - 3\n";    
*/
    {
//        std::cerr << "repl - 1\n";    
        std::string_view message = "FILE LOGGER 4";
        log::Channel& channel = logger.m_channel;
        log::RootLogger& sink = dynamic_cast<log::RootLogger&>(channel.m_sink);
//        std::cerr << "repl - 2\n";    
        std::lock_guard lock(sink.m_mutex);
        sink.m_out.full_clear();
        logger.m_channel.m_prefix.format_prefix(sink.m_out);
//        std::cerr << "repl - 3\n";    
        logger.m_prefix->format_prefix(sink.m_out);
        sink.format_log_level(log::LogLevel::info, sink.m_out);
        char newline = sink.m_newline;
//        std::cerr << "repl - 4\n";    
        std::string_view message_2;
        std::size_t j = message.find(newline);
//        std::cerr << "repl - 5\n";    
        if (ARCHON_LIKELY(j == std::string_view::npos)) {
            std::cerr << "repl - 6\n";    
        }
    }
//    logger.m_channel.m_sink.sink_log(log::LogLevel::info, logger.m_channel.m_prefix, *logger.m_prefix, "FILE LOGGER 3");

/*
    log::FileLogger logger_2(core::File::get_cout(), locale);      
    logger_2.info("FILE LOGGER 2");                          
*/

//    return check::command(ARCHON_VERSION_EX, argc, argv, params, test_order, locale); // Throws
}
