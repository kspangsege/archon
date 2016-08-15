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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <stdexcept>
#include <string>
#include <iostream>

#include <archon/image/image.hpp>
#include <archon/display/implementation.hpp>


using namespace std;
using namespace archon::Imaging;
using namespace archon::Display;

namespace
{
  struct EventHandlerImpl: EventHandler
  {
    void on_close(Event const &) { throw exception(); }
    void on_mousedown(MouseButtonEvent const &e)
    {
      if(e.button == 1) win->set_cursor(cursor2);
    }
    void on_mouseup(MouseButtonEvent const &e)
    {
      if(e.button == 1) win->set_cursor(cursor1);
    }
    EventHandlerImpl(Window::Arg w, Cursor::Arg c1, Cursor::Arg c2):
      win(w), cursor1(c1), cursor2(c2) {}
    Window::Ptr const win;
    Cursor::Ptr const cursor1, cursor2;
  };
}

int main(int argc, char const *argv[]) throw()
{
  if(argc != 3) throw runtime_error("Please specify two images on the commandline");
  string const path1 = argv[1], path2 = argv[2];

  Implementation::Ptr const impl = archon::Display::get_default_implementation();
  Connection::Ptr const conn = impl->new_connection();
  Cursor::Ptr const cursor1 = conn->new_cursor(Image::load(path1));
  Cursor::Ptr const cursor2 = conn->new_cursor(Image::load(path2));
  Window::Ptr const win = conn->new_window(256, 256);
  win->set_title("archon::Display::Cursor");
  win->set_bg_color(0xCFDFBF);
  win->set_cursor(cursor1);
  win->show();

  EventHandlerImpl event_handler(win, cursor1, cursor2);
  EventProcessor::Ptr event_proc = conn->new_event_processor(&event_handler);
  event_proc->register_window(win);
  event_proc->process();

  return 0;
}
