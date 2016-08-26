/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#include <stdexcept>
#include <string>
#include <iostream>

#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/image/image.hpp>
#include <archon/display/implementation.hpp>


using namespace archon::core;
using namespace archon::image;
using namespace archon::display;


namespace {

class CloseException: public std::exception {
public:
    const char* what() const noexcept override
    {
        return "Close";
    }
};

class EventHandlerImpl: public EventHandler {
public:
    void on_close(const Event&) override
    {
        throw CloseException{};
    }

    void on_keydown(const KeyEvent& e) override
    {
        if (e.key_sym == KeySym_Escape)
            m_win->enable_relative_mouse_motion(false);
    }

    void on_mousedown(const MouseButtonEvent& e) override
    {
        if (e.button == 1)
            m_win->enable_relative_mouse_motion(true);
    }

    void on_mousemove(const MouseEvent& e) override
    {
        std::cerr << '('<<e.x<<','<<e.y<<')';
    }

    EventHandlerImpl(Window::Arg win):
        m_win{win}
    {
    }

private:
    const Window::Ptr m_win;
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    try_fix_preinstall_datadir(argv[0], "display/test/");

    CommandlineOptions opts;
    opts.add_help("archon::display::RelativeMouseMotion");
    opts.check_num_args(0,1);
    opts.add_stop_opts();
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    Implementation::Ptr impl = archon::display::get_default_implementation();
    Connection::Ptr conn = impl->new_connection();
    Window::Ptr win = conn->new_window(256, 256);
    win->set_title("archon::display::RelativeMouseMotion");
    win->set_bg_color(0xDFBFCF);
    win->show();

    EventHandlerImpl event_handler{win};
    EventProcessor::Ptr event_proc = conn->new_event_processor(&event_handler);
    event_proc->register_window(win);
    event_proc->process();
}
