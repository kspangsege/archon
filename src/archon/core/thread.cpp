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


#include <cstddef>
#include <cstring>
#include <cerrno>
#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/string_codec.hpp>
#include <archon/core/platform_support.hpp>
#include <archon/core/thread.hpp>


#if (defined _GNU_SOURCE && !ARCHON_ANDROID) || ARCHON_APPLE
#  include <pthread.h>
#endif

#if ARCHON_WINDOWS
#  include <processthreadsapi.h>
#endif


using namespace archon;


bool core::set_thread_name(std::string_view name, const std::locale& locale)
{
#if defined _GNU_SOURCE && !ARCHON_ANDROID && !ARCHON_APPLE
    static_cast<void>(locale);
    const std::size_t max = 16;
    std::size_t n = name.size();
    if (n > max - 1)
        n = max - 1;
    char name_2[max];
    std::copy_n(name.data(), n, name_2);
    name_2[n] = '\0';
    pthread_t id = pthread_self();
    int err = pthread_setname_np(id, name_2);
    if (ARCHON_LIKELY(err == 0))
        return true;
    core::throw_system_error(err, "pthread_setname_np() failed"); // Throws
#elif ARCHON_APPLE
    static_cast<void>(locale);
    std::string name_2(name); // Throws
    int err = pthread_setname_np(name_2.c_str());
    if (ARCHON_LIKELY(err == 0))
        return true;
    core::throw_system_error(err, "pthread_setname_np() failed"); // Throws
#elif ARCHON_WINDOWS
    HANDLE self = GetCurrentThread();
    std::wstring name_2 = core::decode_string<wchar_t>(name, locale); // Throws
    HRESULT result = SetThreadDescription(self, name_2.c_str());
    if (ARCHON_LIKELY(!FAILED(result)))
        return true;
    throw std::runtime_error("SetThreadDescription() failed");
#else
    static_cast<void>(name);
    static_cast<void>(locale);
    return false;
#endif
}


bool core::get_thread_name(std::string& name, const std::locale& locale)
{
#if (defined _GNU_SOURCE && !ARCHON_ANDROID) || ARCHON_APPLE
    static_cast<void>(locale);
    const std::size_t max = 64;
    char name_2[max];
    pthread_t id = pthread_self();
    int err = pthread_getname_np(id, name_2, max);
    if (ARCHON_LIKELY(err == 0)) {
        name_2[max - 1] = '\0'; // Eliminate any risk of buffer overrun in strlen().
        name.assign(name_2, std::strlen(name_2)); // Throws
        return true;
    }
    core::throw_system_error(err, "pthread_getname_np() failed"); // Throws
#elif ARCHON_WINDOWS
    HANDLE self = GetCurrentThread();
    PWSTR name_2 = nullptr;
    HRESULT result = GetThreadDescription(self, &name_2);
    if (ARCHON_LIKELY(!FAILED(result))) {
        name = core::encode_string(std::wstring_view(name_2), locale); // Throws
        return true;
    }
    throw std::runtime_error("GetThreadDescription() failed");
#else
    static_cast<void>(name);
    static_cast<void>(locale);
    return false;
#endif
}
