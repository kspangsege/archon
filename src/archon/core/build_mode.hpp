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

#ifndef ARCHON_X_CORE_X_BUILD_MODE_HPP
#define ARCHON_X_CORE_X_BUILD_MODE_HPP

/// \file


#include <archon/core/features.h>


#if ARCHON_DEBUG
#  define ARCHON_BUILD_MODE "Debug"
#else
#  define ARCHON_BUILD_MODE "Release"
#endif


#if ARCHON_ASAN
#  define ARCHON_BUILD_MODE_EX ARCHON_BUILD_MODE " (AddressSanitizer)"
#elif ARCHON_TSAN
#  define ARCHON_BUILD_MODE_EX ARCHON_BUILD_MODE " (ThreadSanitizer)"
#elif ARCHON_UBSAN
#  define ARCHON_BUILD_MODE_EX ARCHON_BUILD_MODE " (UndefinedBehaviorSanitizer)"
#else
#  define ARCHON_BUILD_MODE_EX ARCHON_BUILD_MODE
#endif


#endif // ARCHON_X_CORE_X_BUILD_MODE_HPP
