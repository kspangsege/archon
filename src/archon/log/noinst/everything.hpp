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
// which is a topological dependency ordering of all the header files of the Archon Logging
// Library, including any that must never be included by applications.
#error "Do not include this header file"


// Foundation
#include <archon/log/log_namespace.hpp>
#include <archon/log/log_level.hpp>

// Support
#include <archon/log/limit.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/sink.hpp>
#include <archon/log/channel.hpp>
#include <archon/log/channel_map.hpp>

// Logger types
#include <archon/log/logger.hpp>
#include <archon/log/stream_logger.hpp>
#include <archon/log/timestamp_logger.hpp>
#include <archon/log/prefix_logger.hpp>
#include <archon/log/limit_logger.hpp>
#include <archon/log/channel_logger.hpp>
#include <archon/log/duplicating_logger.hpp>
#include <archon/log/jail_logger.hpp>
#include <archon/log/impl/encoding_logger_impl.hpp>
#include <archon/log/encoding_logger.hpp>

// Miscellaneous
#include <archon/log/log.hpp>
