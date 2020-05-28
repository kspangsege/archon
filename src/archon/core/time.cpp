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


#include <time.h>
#include <stdexcept>

#include <archon/core/features.h>

#include <archon/core/time.hpp>


using namespace archon;


auto core::time_breakdown_local(std::time_t time) -> std::tm
{
#if ARCHON_WINDOWS
    std::tm tm;
    if (ARCHON_UNLIKELY(::localtime_s(&tm, &time) != 0))
        throw std::invalid_argument("localtime_s() failed");
    return tm;
#else
    // Assuming POSIX.1-2008 for now.
    std::tm tm;
    if (ARCHON_UNLIKELY(!::localtime_r(&time, &tm)))
        throw std::invalid_argument("localtime_r() failed");
    return tm;
#endif
}


auto core::time_breakdown_utc(std::time_t time) -> std::tm
{
#if ARCHON_WINDOWS
    std::tm tm;
    if (ARCHON_UNLIKELY(::gmtime_s(&tm, &time) != 0))
        throw std::invalid_argument("gmtime_s() failed");
    return tm;
#else
    // Assuming POSIX.1-2008 for now.
    std::tm tm;
    if (ARCHON_UNLIKELY(!::gmtime_r(&time, &tm)))
        throw std::invalid_argument("gmtime_r() failed");
    return tm;
#endif
}
