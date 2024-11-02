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

#ifndef ARCHON_X_CORE_X_FEATURES_H
#define ARCHON_X_CORE_X_FEATURES_H

/// \file


#include <version>
#include <cstdint>


#define ARCHON_STRINGIFY(x) ARCHON_STRINGIFY_IMPL(x)
#define ARCHON_STRINGIFY_IMPL(x) #x

#define ARCHON_CONCAT(a, b) ARCHON_CONCAT_IMPL(a, b)
#define ARCHON_CONCAT_IMPL(a, b) a ## b

#define ARCHON_CONCAT_3(a, b, c) ARCHON_CONCAT_3_IMPL(a, b, c)
#define ARCHON_CONCAT_3_IMPL(a, b, c) a ## b ## c

#define ARCHON_CONCAT_4(a, b, c, d) ARCHON_CONCAT_4_IMPL(a, b, c, d)
#define ARCHON_CONCAT_4_IMPL(a, b, c, d) a ## b ## c ## d


/// \def ARCHON_DEBUG
///
/// \brief Debug mode compilation for Archon libraries.
///
/// When this macro is nonzero, the Archon libraries are compiled in debug mode (as opposed
/// to release mode).
///
#if !defined ARCHON_DEBUG
#  if !defined NDEBUG
#    define ARCHON_DEBUG 1
#  else
#    define ARCHON_DEBUG 0
#  endif
#endif


// Compiler is GCC or GCC compatible (covers Clang)
//
#if defined __GNUC__ && __GNUC__
#  define ARCHON_GCC_OR_GCC_EMUL 1
#else
#  define ARCHON_GCC_OR_GCC_EMUL 0
#endif


// Compiler is Clang
//
#if defined __clang__ && __clang__
#  define ARCHON_CLANG 1
#else
#  define ARCHON_CLANG 0
#endif


// Compiler is Microsoft Visual Studio
//
#if defined _MSC_VER
#  define ARCHON_MSVC 1
#else
#  define ARCHON_MSVC 0
#endif


// Sanitizers
//
#if !defined ARCHON_ASAN
#  define ARCHON_ASAN 0
#endif
#if !defined ARCHON_TSAN
#  define ARCHON_TSAN 0
#endif
#if !defined ARCHON_UBSAN
#  define ARCHON_UBSAN 0
#endif


// Unix platform
//
#if !defined ARCHON_UNIX
#  if defined __unix__ && __unix__
#    define ARCHON_UNIX 1
#  else
#    define ARCHON_UNIX 0
#  endif
#endif


// Linux platform
//
#if !defined ARCHON_LINUX
#  if defined __linux__ && __linux__
#    define ARCHON_LINUX 1
#  else
#    define ARCHON_LINUX 0
#  endif
#endif


// Windows platform (MSVC or MinGW)
//
#if !defined ARCHON_WINDOWS
#  if defined WIN32 || defined _WIN32 || defined __WIN32__
#    define ARCHON_WINDOWS 1
#  else
#    define ARCHON_WINDOWS 0
#  endif
#endif


// MinGW platform (GCC on Windows)
//
#if !defined ARCHON_MINGW
#  if defined __MINGW32__ && __MINGW32__
#    define ARCHON_MINGW 1
#  else
#    define ARCHON_MINGW 0
#  endif
#endif


// Cygwin platform (POSIX emulation on Windows)
//
#if !defined ARCHON_CYGWIN
#  if defined __CYGWIN__ && __CYGWIN__
#    define ARCHON_CYGWIN 1
#  else
#    define ARCHON_CYGWIN 0
#  endif
#endif


// Android platform
//
#if !defined ARCHON_ANDROID
#  if defined ANDROID
#    define ARCHON_ANDROID 1
#  else
#    define ARCHON_ANDROID 0
#  endif
#endif


// Apple platform
//
// Some documentation of the defines provided by Apple:
// http://developer.apple.com/library/mac/documentation/Porting/Conceptual/PortingUnix/compiling/compiling.html#//apple_ref/doc/uid/TP40002850-SW13
//
#if defined __APPLE__ && defined __MACH__
#  define ARCHON_APPLE 1
#  include <Availability.h>
#  include <TargetConditionals.h>
#  if TARGET_OS_MAC == 1 // macOS.
#    define ARCHON_MACOS 1
#  else
#    define ARCHON_MACOS 0
#  endif
#  if TARGET_OS_IPHONE == 1 // Device (iPhone or iPad) or simulator.
#    define ARCHON_IOS 1
#  else
#    define ARCHON_IOS 0
#  endif
#  if TARGET_OS_WATCH == 1 // Device (Apple Watch) or simulator.
#    define ARCHON_WATCHOS 1
#  else
#    define ARCHON_WATCHOS 0
#  endif
#  if TARGET_OS_TV // Device (Apple TV) or simulator.
#    define ARCHON_TVOS 1
#  else
#    define ARCHON_TVOS 0
#  endif
#else
#  define ARCHON_APPLE 0
#  define ARCHON_MACOS 0
#  define ARCHON_IOS 0
#  define ARCHON_WATCHOS 0
#  define ARCHON_TVOS 0
#endif


// Detect GNU libstdc++, the GNU implementation of the C++ standard library. See
// https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_macros.html.
//
#if defined __GLIBCXX__
#  define ARCHON_GNU_LIBCXX 1
#else
#  define ARCHON_GNU_LIBCXX 0
#endif


// Detect LLVM libc++, the LLVM implementation of the C++ standard library. See
// https://libcxx.llvm.org/docs.
//
#if defined _LIBCPP_VERSION && _LIBCPP_VERSION
#  define ARCHON_LLVM_LIBCXX 1
#else
#  define ARCHON_LLVM_LIBCXX 0
#endif


// Detect Microsoft's implementation of the C++ Standard Library as used in Visual
// Studio. See https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros.
//
#if defined _MSVC_LANG
#  define ARCHON_MSVC_LIBCXX 1
#else
#  define ARCHON_MSVC_LIBCXX 0
#endif


// Branch condition likelihood
//
#if ARCHON_GCC_OR_GCC_EMUL
#  define ARCHON_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#  define ARCHON_LIKELY(expr)   __builtin_expect(!!(expr), 1)
#else
#  define ARCHON_UNLIKELY(expr) (expr)
#  define ARCHON_LIKELY(expr)   (expr)
#endif


// Try to force inlining
//
#if ARCHON_GCC_OR_GCC_EMUL
#  define ARCHON_FORCEINLINE inline __attribute__((always_inline))
#elif ARCHON_MSVC
#  define ARCHON_FORCEINLINE __forceinline
#else
#  define ARCHON_FORCEINLINE inline
#endif


// Try to prevent inlining
//
#if ARCHON_GCC_OR_GCC_EMUL
#  define ARCHON_NOINLINE __attribute__((noinline))
#elif ARCHON_MSVC
#  define ARCHON_NOINLINE __declspec(noinline)
#else
#  define ARCHON_NOINLINE
#endif


// If `ARCHON_WCHAR_IS_UNICODE` is true, it means that we can assume that the character
// encoding for `wchar_t` is UCS in all locales.
//
// See also \ref ARCHON_ASSUME_UCS_LOCALE and \ref ARCHON_ASSUME_UTF8_LOCALE.
//
#if defined __STDC_ISO_10646__
#  define ARCHON_WCHAR_IS_UNICODE 1
#else
#  define ARCHON_WCHAR_IS_UNICODE 0
#endif


// Availability of optional integer types
//
#if defined UINTPTR_MAX
#  define ARCHON_HAVE_UINTPTR_T 1
#else
#  define ARCHON_HAVE_UINTPTR_T 0
#endif


/// \def ARCHON_NO_UNIQUE_ADDRESS
///
/// \brief Substitute for 'no unique address' attribute.
///
/// This macro expands to `[[no_unique_address]]` except when compiling with Microsoft
/// Visual Studio where it expands to `[[msvc::no_unique_address]]` instead.
///
/// When used before the declaration of a member variable of empty class type
/// (`std::is_empty`), it allows the compiler to reduce the footprint of the member to zero
/// bytes. This will generally have the same effect as if that member was turned into a base
/// class subobject and the compiler applied the effect of "empty base optimization".
///
/// Here is an demonstration of how it can be used:
///
/// \code{.cpp}
///
///    template<class A> class Foo {
///    public:
///        Foo(A alloc) : m_alloc(std::move(alloc)) {}
///
///    private:
///        ARCHON_NO_UNIQUE_ADDRESS A m_alloc;
///    };
///
/// \endcode
///
/// It is up to the compiler whether and how to apply the optimization opportunity, but on
/// most compilers, when `A` is an empty class type (`std::is_empty`), the size of `Foo`
/// will be as if `m_alloc` had not been there. Moreover, on most compilers, if `Foo` has no
/// members other than `m_alloc`, `Foo` will be an empty class type when `A` is an empty
/// class type.
///
#if ARCHON_MSVC
#  define ARCHON_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#  define ARCHON_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif


// Control enablement of platform optimizations
//
#if !defined ARCHON_DISABLE_PLATFORM_OPTIMIZATIONS
#  define ARCHON_ENABLE_PLATFORM_OPTIMIZATIONS 1
#else
#  define ARCHON_ENABLE_PLATFORM_OPTIMIZATIONS 0
#endif


#endif // ARCHON_X_CORE_X_FEATURES_H
