// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include <cerrno>
#include <stdexcept>
#include <system_error>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/resource_limits.hpp>

#if !ARCHON_WINDOWS
#  define HAS_POSIX_RLIMIT 1
#endif

#if HAS_POSIX_RLIMIT
#  include <sys/resource.h>
#endif


using namespace archon;
using namespace archon::base;


#if HAS_POSIX_RLIMIT

namespace {

std::error_code make_system_error_code(int err) noexcept
{
    return { err, std::system_category() };
}

int map_resource_ident(Resource resource) noexcept
{
    switch (resource) {
        case Resource::core_dump_size:
            return RLIMIT_CORE;
        case Resource::cpu_time:
            return RLIMIT_CPU;
        case Resource::data_segment_size:
            return RLIMIT_DATA;
        case Resource::file_size:
            return RLIMIT_FSIZE;
        case Resource::num_open_files:
            return RLIMIT_NOFILE;
        case Resource::stack_size:
            return RLIMIT_STACK;
        case Resource::virtual_memory_size:
            return RLIMIT_AS;
    }
    ARCHON_ASSERT(false);
    return 0;
}

long get_rlimit(Resource resource, bool hard)
{
    int resource_2 = map_resource_ident(resource);
    rlimit rlimit;
    int status = getrlimit(resource_2, &rlimit);
    if (ARCHON_UNLIKELY(status < 0)) {
        int err = errno; // Eliminate any risk of clobbering
        std::error_code ec = make_system_error_code(err);
        throw std::system_error(ec, "getrlimit() failed");
    }
    rlim_t value = (hard ? rlimit.rlim_max : rlimit.rlim_cur);
    return value == RLIM_INFINITY ? -1 : long(value);
}

void set_rlimit(Resource resource, long value, bool hard)
{
    int resource_2 = map_resource_ident(resource);
    rlimit rlimit;
    int status = getrlimit(resource_2, &rlimit);
    if (ARCHON_UNLIKELY(status < 0)) {
        int err = errno; // Eliminate any risk of clobbering
        std::error_code ec = make_system_error_code(err);
        throw std::system_error(ec, "getrlimit() failed");
    }
    rlim_t value_2 = (value < 0 ? RLIM_INFINITY : rlim_t(value));
    (hard ? rlimit.rlim_max : rlimit.rlim_cur) = value_2;
    status = setrlimit(resource_2, &rlimit);
    if (ARCHON_UNLIKELY(status < 0)) {
        int err = errno; // Eliminate any risk of clobbering
        std::error_code ec = make_system_error_code(err);
        throw std::system_error(ec, "setrlimit() failed");
    }
}

} // anonymous namespace

#endif // HAS_POSIX_RLIMIT


#if HAS_POSIX_RLIMIT

bool base::system_has_rlimit(Resource) noexcept
{
    return true;
}

long base::get_hard_rlimit(Resource resource)
{
    bool hard = true;
    return get_rlimit(resource, hard);
}

long base::get_soft_rlimit(Resource resource)
{
    bool hard = false;
    return get_rlimit(resource, hard);
}

void base::set_soft_rlimit(Resource resource, long value)
{
    bool hard = false;
    set_rlimit(resource, value, hard);
}

#else // ! HAS_POSIX_RLIMIT

bool base::system_has_rlimit(Resource) noexcept
{
    return false;
}

long base::get_hard_rlimit(Resource)
{
    throw std::runtime_error("Not supported");
}

long base::get_soft_rlimit(Resource)
{
    throw std::runtime_error("Not supported");
}

void base::set_soft_rlimit(Resource, long)
{
    throw std::runtime_error("Not supported");
}

#endif // ! HAS_POSIX_RLIMIT
