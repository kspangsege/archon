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


#include <archon/display/event_handler.hpp>


using namespace archon;
using display::WindowEventHandler;
using display::ConnectionEventHandler;


bool WindowEventHandler::on_keydown(const display::KeyEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_keyup(const display::KeyEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_mousedown(const display::MouseButtonEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_mouseup(const display::MouseButtonEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_mousemove(const display::MouseEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_scroll(const display::ScrollEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_mouseover(const display::TimedWindowEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_mouseout(const display::TimedWindowEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_focus(const display::TimedWindowEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_expose(const display::WindowEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_resize(const display::WindowSizeEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_blur(const display::TimedWindowEvent&)
{
    return true; // Do not interrupt event processing
}


bool WindowEventHandler::on_close(const display::TimedWindowEvent&)
{
    return true; // Do not interrupt event processing
}


bool ConnectionEventHandler::on_display_change(int)
{
    return true; // Do not interrupt event processing
}


bool ConnectionEventHandler::before_sleep()
{
    return true; // Do not interrupt event processing
}


bool ConnectionEventHandler::on_quit()
{
    return false; // Interrupt event processing
}
