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


// Dependencies
#include <archon/core/noinst/everything.hpp>
#include <archon/log/noinst/everything.hpp>

// Foundation
#include <archon/cli/cli_namespace.hpp>
#include <archon/cli/impl/call.hpp>
#include <archon/cli/string_holder.hpp>
#include <archon/cli/impl/value_formatter.hpp>
#include <archon/cli/impl/value_parser.hpp>

// Support
#include <archon/cli/proc_error.hpp>
#include <archon/cli/error_handler.hpp>
#include <archon/cli/logging_error_handler.hpp>
#include <archon/cli/config.hpp>
#include <archon/cli/spec_error.hpp>
#include <archon/cli/help_spec_error.hpp>
#include <archon/cli/exception.hpp>
#include <archon/cli/option_actions.hpp>
#include <archon/cli/attributes.hpp>
#include <archon/cli/spec_support.hpp>
#include <archon/cli/help_config.hpp>

// Main
#include <archon/cli/impl/root_state.hpp>
#include <archon/cli/impl/option_action.hpp>
#include <archon/cli/impl/error_accum.hpp>
#include <archon/cli/processor_fwd.hpp>
#include <archon/cli/impl/option_occurrence.hpp>
#include <archon/cli/impl/option_invocation.hpp>
#include <archon/cli/command_line.hpp>
#include <archon/cli/impl/pattern_symbol.hpp>
#include <archon/cli/impl/pattern_structure.hpp>
#include <archon/cli/impl/pattern_args_parser.hpp>
#include <archon/cli/impl/pattern_func_checker.hpp>
#include <archon/cli/impl/pattern_action.hpp>
#include <archon/cli/impl/spec.hpp>
#include <archon/cli/impl/nfa.hpp>
#include <archon/cli/impl/nfa_builder.hpp>
#include <archon/cli/impl/pattern_matcher.hpp>
#include <archon/cli/spec.hpp>
#include <archon/cli/impl/processor.hpp>
#include <archon/cli/impl/help_formatter.hpp>
#include <archon/cli/impl/spec_parser.hpp>
#include <archon/cli/processor.hpp>
#include <archon/cli/process.hpp>
