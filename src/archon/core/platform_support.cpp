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
#include <string>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/platform_support.hpp>


using namespace archon;
namespace impl = core::impl;


#if ARCHON_WINDOWS


auto impl::WindowsSystemErrorCategory::name() const noexcept -> const char*
{
    return std::system_category().name();
}


auto impl::WindowsSystemErrorCategory::default_error_condition(int err) const noexcept -> std::error_condition
{
    return std::system_category().default_error_condition(err);
}


bool impl::WindowsSystemErrorCategory::equivalent(int err, const std::error_condition& cond) const noexcept
{
    return std::system_category().equivalent(err, cond);
}


bool impl::WindowsSystemErrorCategory::equivalent(const std::error_code& ec, int err) const noexcept
{
    return std::system_category().equivalent(ec, err);
}


auto impl::WindowsSystemErrorCategory::message(int err) const -> std::string
{
    DWORD flags = DWORD(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS);
    LPCVOID message = nullptr; // Not applicable
    LPSTR buffer = nullptr;
    DWORD language_id = 0; // Be flexible
    DWORD min_size = 0; // Do not allocate more than is needed
    va_list* args = nullptr; // Not applicable
    DWORD len = FormatMessageA(flags, message, DWORD(err), language_id, reinterpret_cast<LPSTR>(&buffer),
                               min_size, args);
    if (ARCHON_LIKELY(len > 0)) {
        // Trim away trailing line terminators
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
            --len;
        // Trim away trailing full stop
        if (len > 0 && buffer[len - 1] == '.')
            --len;
        std::size_t size = 0;
        core::int_cast(len, size); // Throws
        std::string message_2(static_cast<const char*>(buffer), size); // Throws
        HLOCAL handle = LocalFree(buffer);
        ARCHON_ASSERT(!handle);
        return message_2;
    }
    return "Unknown error"; // Throws
}


auto impl::get_dll_func(const char* dll_name, const char* func_name) -> FARPROC
{
    HMODULE module = GetModuleHandleA(dll_name);
    if (ARCHON_UNLIKELY(!module)) {
        DWORD err = GetLastError(); // Eliminate any risk of clobbering
        core::throw_system_error(int(err), "GetModuleHandle() failed"); // Throws
    }
    return GetProcAddress(module, func_name);
}

#endif // ARCHON_WINDOWS


void impl::platform_support_dummy()
{
}
