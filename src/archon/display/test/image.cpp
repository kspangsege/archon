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

#include <archon/image/image.hpp>
#include <archon/display/implementation.hpp>


using namespace std;
using namespace archon::image;
using namespace archon::display;

namespace {

class EventHandlerImpl: public EventHandler {
public:
    void on_close(const Event&)
    {
        throw exception();
    }

    void on_mousedown(const MouseButtonEvent&)
    {
    }

    void on_damage(const AreaEvent& ev)
    {
        std::cerr << ev.x<<","<<ev.y<<", "<<ev.width<<","<<ev.height<<"\n";
        Box clip;
        clip.x = ev.x;
        clip.y = ev.y;
        clip.width = ev.width;
        clip.height = ev.height;
        win->put_image(img, clip);
    }

    EventHandlerImpl(Window::Ptr w, Image::Ref i):
        win(w),
        img(i)
    {
    }

    const Window::Ptr win;
    const Image::Ref img;
};

} // unnamed namespace


int main(int argc, const char* argv[]) throw()
{
    if (argc != 2)
        throw runtime_error("Please specify an image on the commandline");
    string path = argv[1];

    Implementation::Ptr impl = archon::display::get_default_implementation();
    Connection::Ptr conn = impl->new_connection();
    Image::Ref img = Image::load(path);
    Window::Ptr win = conn->new_window(img->get_width(), img->get_height());
    win->set_title("archon::display::Image");

    EventHandlerImpl event_handler(win, img);
    EventProcessor::Ptr event_proc = conn->new_event_processor(&event_handler);
    event_proc->register_window(win);

    win->show();
    event_proc->process();
}
