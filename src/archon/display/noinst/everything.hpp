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
// which is a topological dependency ordering of all the header files of the Archon Core
// Library, including any that must never be included by applications.
#error "Do not include this header file"


// Dependencies
#include <archon/noinst/everything_base.hpp>
#include <archon/noinst/everything_math.hpp>
#include <archon/noinst/everything_util.hpp>
#include <archon/noinst/everything_image.hpp>

// Local headers
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
#include <archon/display/screen.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/texture.hpp>
#include <archon/display/window.hpp>
#include <archon/display/connection.hpp>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_sdl.hpp>
#include <archon/display/as_key_name.hpp>
