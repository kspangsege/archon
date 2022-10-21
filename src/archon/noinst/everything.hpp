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


// Do not include this header file. It exists only to specify the canonical header order,
// which is a topological dependency ordering of all the header files of the Archon Core
// Library, including any that must never be included by applications.
#error "Do not include this header file"


#include <archon/impl/config.h>
#include <archon/core/noinst/everything.hpp>
#include <archon/version.hpp>
#include <archon/log.hpp>
#include <archon/log/noinst/everything.hpp>
#include <archon/cli.hpp>
#include <archon/cli/noinst/everything.hpp>
#include <archon/check.hpp>
#include <archon/check/noinst/everything.hpp>
#include <archon/core/test/everything.hpp>

#include <archon/math/noinst/everything.hpp>
#include <archon/util/noinst/everything.hpp>
#include <archon/image.hpp>
#include <archon/image/noinst/everything.hpp>
#include <archon/font.hpp>
#include <archon/font/noinst/everything.hpp>
#include <archon/display.hpp>
#include <archon/display/noinst/everything.hpp>
