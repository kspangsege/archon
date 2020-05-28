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


/// \file

#ifndef ARCHON__BASE__PLATFORM_SUPPORT_HPP
#define ARCHON__BASE__PLATFORM_SUPPORT_HPP

#include <cerrno>
#include <type_traits>
#include <utility>
#include <string_view>
#include <string>
#include <stdexcept>
#include <system_error>

#include <archon/base/features.h>

#if ARCHON_WINDOWS
#  define NOMINMAX
#  include <windows.h>
#else
#  include <unistd.h>
#endif


namespace archon::base {


/// \brief Throw exception due to system error.
///
/// This function reads the current system error code (`errno` on POSIX
/// systems), constructs an `std::error_code` from it, and throws an exception
/// of type std::system_error.
///
/// CAUTION: Be careful not to clobber the system error code before calling this
/// function.
///
[[noreturn]] void on_system_error(const char* message);



#if ARCHON_WINDOWS

template<class F, class R, class... A>
bool try_call_dll_func(const char* dll_name, const char* func_name, R& result, A&&... args);

template<class F, class... A>
auto call_dll_func(const char* dll_name, const char* func_name, A&&... args);

std::wstring to_windows_wstring(std::string_view);

std::string from_windows_wstring(std::wstring_view);

#endif // ARCHON_WINDOWS








// Implementation


[[noreturn]] inline void on_system_error(const char* message)
{
#if ARCHON_WINDOWS
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
#else
    int err = errno; // Eliminate any risk of clobbering
#endif
    std::error_code ec = { int(err), std::system_category() };
    throw std::system_error(ec, message);
}


#if ARCHON_WINDOWS


namespace detail {
FARPROC get_dll_func(const char* dll_name, const char* func_name);
} // namespace detail


template<class F, class R, class... A>
inline bool try_call_dll_func(const char* dll_name, const char* func_name, R& result, A&&... args)
{
    if (FARPROC func = detail::get_dll_func(dll_name, func_name)) { // Throws
        auto func_2 = reinterpret_cast<F*>(func);
        result = (*func_2)(std::forward<A>(args)...);
        return true;
    }
    return false;
}


template<class F, class... A>
inline auto call_dll_func(const char* dll_name, const char* func_name, A&&... args)
{
    if (FARPROC func = detail::get_dll_func(dll_name, func_name)) { // Throws
        auto func_2 = reinterpret_cast<F*>(func);
        return (*func_2)(std::forward<A>(args)...);
    }
    throw std::runtime_error("No such function in DLL");
}


#endif // ARCHON_WINDOWS


} // namespace archon::base

#endif // ARCHON__BASE__PLATFORM_SUPPORT_HPP
