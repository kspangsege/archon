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


// Do not include this header file. It exists only to specify the canonical header order,
// which is a topological dependency ordering of all the header files of the Archon Display
// Library, including any that must never be included by applications.
#error "Do not include this header file"


#include <archon/display/display_namespace.hpp>
#include <archon/display/impl/config.h>
#include <archon/display/implementation_fwd.hpp>
#include <archon/display/geometry.hpp>
#include <archon/display/key.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/mouse_button.hpp>
#include <archon/display/event.hpp>
#include <archon/display/event_handler.hpp>
#include <archon/display/resolution.hpp>
#include <archon/display/viewport.hpp>
#include <archon/display/noinst/timestamp_unwrapper.hpp>
#include <archon/display/noinst/edid.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>
#include <archon/display/sdl_connection_config.hpp>
#include <archon/display/texture.hpp>
#include <archon/display/window.hpp>
#include <archon/display/connection.hpp>
#include <archon/display/implementation.hpp>
#include <archon/display/x11_implementation.hpp>
#include <archon/display/sdl_implementation.hpp>
#include <archon/display/as_key_name.hpp>
#include <archon/display/list_implementations.hpp>
#include <archon/display/noinst/mult_pixel_format.hpp>
#include <archon/display/noinst/palette_map.hpp>
#include <archon/display/noinst/impl_util.hpp>
#include <archon/display/noinst/x11/support.hpp>
