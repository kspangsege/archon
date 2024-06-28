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


#include <utility>
#include <memory>
#include <stdexcept>
#include <string>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/string.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/connection.hpp>
#include <archon/display/implementation.hpp>


using namespace archon;
using display::Connection;


auto Connection::new_window(std::string_view title, display::Size size,
                            const display::Window::Config& config) -> std::unique_ptr<display::Window>
{
    std::unique_ptr<display::Window> win;
    std::string error;
    if (ARCHON_LIKELY(try_new_window(title, size, config, win, error))) // Throws
        return win;
    std::string message = core::concat("Failed to create window: ", error); // Throws
    throw std::runtime_error(message);
}


auto display::new_connection(const std::locale& locale, const display::Guarantees& guarantees,
                             const display::Connection::Config& config) -> std::unique_ptr<display::Connection>
{
    std::unique_ptr<display::Connection> conn;
    std::string error;
    if (ARCHON_LIKELY(display::try_new_connection(locale, guarantees, config, conn, error))) // Throws
        return conn;
    std::string message = core::concat("Failed to open display connection: ", error); // Throws
    throw std::runtime_error(message);
}


bool display::try_new_connection(const std::locale& locale, const display::Guarantees& guarantees,
                                 const display::Connection::Config& config,
                                 std::unique_ptr<display::Connection>& conn, std::string& error)
{
    std::unique_ptr<display::Connection> conn_2;
    if (ARCHON_LIKELY(display::try_new_connection_a(locale, guarantees, config, conn_2, error))) { // Throws
        if (ARCHON_LIKELY(conn_2)) {
            conn = std::move(conn_2);
            return true;
        }
        error = "No available display implementations"; // Throws
    }
    return false;
}


bool display::try_new_connection_a(const std::locale& locale, const display::Guarantees& guarantees,
                                   const display::Connection::Config& config,
                                   std::unique_ptr<display::Connection>& conn, std::string& error)
{
    const display::Implementation* impl = display::get_default_implementation_a(guarantees);
    if (ARCHON_LIKELY(impl))
        return impl->try_new_connection(locale, config, conn, error); // Throws
    conn = {};
    return true;
}
