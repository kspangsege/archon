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

#ifndef ARCHON_X_CORE_X_PLATFORM_SUPPORT_HPP
#define ARCHON_X_CORE_X_PLATFORM_SUPPORT_HPP

/// \file


#include <utility>
#include <string>
#include <stdexcept>
#include <system_error>

#include <archon/core/features.h>

#if ARCHON_WINDOWS
#  if !defined NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <unistd.h>
#endif


namespace archon::core {


/// \brief Get error code object for system error.
///
/// This function constructs an error code object from the specified "bare" system error
/// code (`errno` on POSIX systems, `GetLastError()` on Windows).
///
auto make_system_error(int err) noexcept -> std::error_code;


/// \brief Throw exception due to system error.
///
/// This function throws a system error (`std::system_error`) for the specified "bare"
/// system error code (`errno` on POSIX systems, `GetLastError()` on Windows).
///
[[noreturn]] void throw_system_error(int err, const char* message);



#if ARCHON_WINDOWS

template<class F, class R, class... A> bool try_call_dll_func(const char* dll_name, const char* func_name, R& result,
                                                              A&&... args);

template<class F, class... A> auto call_dll_func(const char* dll_name, const char* func_name, A&&... args);

#endif // ARCHON_WINDOWS








// Implementation


namespace impl {

#if ARCHON_WINDOWS


// On Windows, at least with some version of Visual Studio, and with some system level
// configurations (system-level locale), `std::system_category().message(err)` returns
// "Unknown error" for all error codes.
//
// The solution pursued here is to manually call FormatMessageA() and pass 0 for
// `language_id` rather than a value derived from the configured system-level locale.
//
// The idea is to forward to std::system_category() for everything other than retrieval of
// error messages.
//
class WindowsSystemErrorCategory final
    : public std::error_category {
public:
    auto name() const noexcept -> const char* override;
    auto default_error_condition(int) const noexcept -> std::error_condition override;
    bool equivalent(int, const std::error_condition&) const noexcept override;
    bool equivalent(const std::error_code&, int) const noexcept override;
    auto message(int) const -> std::string override;
};

inline constinit const impl::WindowsSystemErrorCategory g_windows_system_error_category;


#endif // ARCHON_WINDOWS


// Work around "no symbols" warning from Apple linker
void platform_support_dummy();


} // namespace impl


inline auto make_system_error(int err) noexcept -> std::error_code
{
#if ARCHON_WINDOWS
    const std::error_category& cat = impl::g_windows_system_error_category;
#else
    const std::error_category& cat = std::system_category();
#endif
    return { err, cat };
}


[[noreturn]] inline void throw_system_error(int err, const char* message)
{
    std::error_code ec = core::make_system_error(err);
    throw std::system_error(ec, message);
}


#if ARCHON_WINDOWS

namespace impl {

auto get_dll_func(const char* dll_name, const char* func_name) -> FARPROC;

} // namespace impl


template<class F, class R, class... A> inline bool try_call_dll_func(const char* dll_name, const char* func_name,
                                                                     R& result, A&&... args)
{
    if (FARPROC func = impl::get_dll_func(dll_name, func_name)) { // Throws
        auto func_2 = reinterpret_cast<F*>(func);
        result = (*func_2)(std::forward<A>(args)...);
        return true;
    }
    return false;
}


template<class F, class... A> inline auto call_dll_func(const char* dll_name, const char* func_name, A&&... args)
{
    if (FARPROC func = impl::get_dll_func(dll_name, func_name)) { // Throws
        auto func_2 = reinterpret_cast<F*>(func);
        return (*func_2)(std::forward<A>(args)...);
    }
    throw std::runtime_error("No such function in DLL");
}


#endif // ARCHON_WINDOWS


} // namespace archon::core

#endif // ARCHON_X_CORE_X_PLATFORM_SUPPORT_HPP
