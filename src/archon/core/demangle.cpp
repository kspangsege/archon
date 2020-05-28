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


#include <archon/core/features.h>

#include <archon/core/demangle.hpp>


#if __has_include(<cxxabi.h>)
#  define HAS_CXXABI_H 1
#  include <cstdlib>
#  include <stdexcept>
#  include <cxxabi.h>
#else
#  define HAS_CXXABI_H 0
#endif


using namespace archon;


auto core::demangle(const char* mangled_name) -> std::string
{
#if HAS_CXXABI_H
    class Mem {
    public:
        char* ptr = nullptr;
        ~Mem() noexcept
        {
            std::free(ptr);
        }
    };
    int status = 0;
    Mem mem;
    mem.ptr = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
    if (ARCHON_LIKELY(status == 0))
        return std::string(mem.ptr); // Throws
    if (ARCHON_UNLIKELY(status == -1))
        throw std::bad_alloc();
#endif // HAS_CXXABI_H
    return std::string(mangled_name); // Throws
}
