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

    void on_mousedown(const MouseButtonEvent& e) override
    {
        if (e.button == 1)
            m_win->set_cursor(*m_cursor);
    }

    void on_mouseup(const MouseButtonEvent& e) override
    {
        if (e.button == 1)
            m_win->reset_cursor();
    }

    EventHandlerImpl(Window::Arg win, std::unique_ptr<Cursor> cursor):
        m_win{win},
        m_cursor{std::move(cursor)}
    {
    }

private:
    const Window::Ptr m_win;
    const std::unique_ptr<Cursor> m_cursor;
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    try_fix_preinstall_datadir(argv[0], "display/test/");

    std::string path = get_value_of(build_config_param_DataDir) +
        "/display/test/ring_cursor.png";
    Series<2, int> opt_hotspot{16,16};

    CommandlineOptions opts;
    opts.add_help("archon::display::Cursor", "IMAGE");
    opts.check_num_args(0,1);
    opts.add_stop_opts();
    opts.add_param("H", "hotspot", opt_hotspot, "Set the cursor hotspot relative to the upper "
                   "right corner");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;
    if (argc >= 2)
        path = argv[1];

    Implementation::Ptr impl = archon::display::get_default_implementation();
    Connection::Ptr conn = impl->new_connection();
    std::unique_ptr<Cursor> cursor =
        conn->new_cursor(Image::load(path), opt_hotspot[0], opt_hotspot[1]);
    Window::Ptr win = conn->new_window(256, 256);
    win->set_title("archon::display::Cursor");
    win->set_bg_color(0xCFDFBF);
    win->show();

    EventHandlerImpl event_handler{win, std::move(cursor)};
    EventProcessor::Ptr event_proc = conn->new_event_processor(&event_handler);
    event_proc->register_window(win);
    event_proc->process();
}
