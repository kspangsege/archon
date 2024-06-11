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


#include <cerrno>
#include <string_view>
#include <system_error>

#include <locale.h>

#include <archon/core/features.h>

#include <archon/core/locale.hpp>


#if !ARCHON_WINDOWS
#  include <unistd.h>
#endif

#if ARCHON_MACOS
#  include <xlocale.h>
#endif


using namespace archon;


bool core::has_locale(const char* name)
{
#if ARCHON_CYGWIN || ARCHON_MINGW
    // Both Cygwin and MingGW use libstdc++ in a way that supports only the C locale.
    return (std::string_view(name) == "C");
#elif ARCHON_WINDOWS
    _locale_t loc = ::_create_locale(LC_ALL, name);
    if (loc) {
        ::_free_locale(loc);
        return true;
    }
#else
    // Unfortunately, with GLIBC, newlocale() does not always set `errno` when failing (see
    // https://sourceware.org/bugzilla/show_bug.cgi?id=14247). The following is a
    // work-around.
    errno = ENOENT;
    locale_t loc = ::newlocale(LC_ALL_MASK, name, locale_t(0));
    if (loc != locale_t(0)) {
        ::freelocale(loc);
        return true;
    }
#  if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L // POSIX.1-2008
    int err = errno; // Eliminate any risk of clobbering
    if (err != ENOENT) {
        std::error_code ec = { int(err), std::system_category() };
        throw std::system_error(ec, "newlocale() failed");
    }
#  endif
#endif
    return false;
}
