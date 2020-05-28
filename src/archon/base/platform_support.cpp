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


#include <cstddef>

#include <archon/base/assert.hpp>
#include <archon/base/platform_support.hpp>


using namespace archon;
using namespace archon::base;


#if ARCHON_WINDOWS

std::wstring base::to_windows_wstring(std::string_view string)
{
    UINT code_page = CP_UTF8;
    DWORD flags = 0;
    const char* data = string.data();
    std::size_t size = string.size();
    if (ARCHON_LIKELY(size > 0)) {
        int size_2 = MultiByteToWideChar(code_page, flags, data, int(size), nullptr, 0);
        ARCHON_ASSERT(size_2 >= 0);
        if (ARCHON_UNLIKELY(size_2 == 0))
            on_system_error("MultiByteToWideChar() (1st) failed"); // Throws
        std::wstring string_2;
        string_2.resize(std::size_t(size_2)); // Throws
        int size_3 = MultiByteToWideChar(code_page, flags, data, int(size),
                                         string_2.data(), size_2);
        ARCHON_ASSERT(size_3 >= 0);
        if (ARCHON_UNLIKELY(size_3 == 0))
            on_system_error("MultiByteToWideChar() (2nd) failed"); // Throws
        return string_2.substr(0, std::size_t(size_3)); // Throws
    }
    return {};
}


std::string base::from_windows_wstring(std::wstring_view string)
{
    UINT code_page = CP_UTF8;
    DWORD flags = 0;
    const wchar_t* data = string.data();
    std::size_t size = string.size();
    if (ARCHON_LIKELY(size > 0)) {
        int size_2 = WideCharToMultiByte(code_page, flags, data, int(size), nullptr, 0,
                                         nullptr, nullptr);
        ARCHON_ASSERT(size_2 >= 0);
        if (ARCHON_UNLIKELY(size_2 == 0))
            on_system_error("WideCharToMultiByte() (1st) failed"); // Throws
        std::string string_2;
        string_2.resize(std::size_t(size_2)); // Throws
        int size_3 = WideCharToMultiByte(code_page, flags, data, int(size),
                                         string_2.data(), size_2, nullptr, nullptr);
        ARCHON_ASSERT(size_3 >= 0);
        if (ARCHON_UNLIKELY(size_3 == 0))
            on_system_error("WideCharToMultiByte() (2nd) failed"); // Throws
        return string_2.substr(0, std::size_t(size_3)); // Throws
    }
    return {};
}


FARPROC detail::get_dll_func(const char* dll_name, const char* func_name)
{
    HMODULE module = GetModuleHandleA(dll_name);
    if (ARCHON_UNLIKELY(!module))
        base::on_system_error("GetModuleHandle() failed"); // Throws
    return GetProcAddress(module, func_name);
}

#endif // ARCHON_WINDOWS
