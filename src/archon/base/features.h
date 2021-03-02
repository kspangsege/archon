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

#ifndef ARCHON_X_BASE_X_FEATURES_H
#define ARCHON_X_BASE_X_FEATURES_H

/// \file


// Obtaining definitions of implementation-specific STL library version macros.
//
#if __has_include(<version>) // C++20
#  include <version>
#else
#  include <ciso646>
#endif


#define ARCHON_STRINGIFY(x) ARCHON_STRINGIFY_IMPL(x)
#define ARCHON_STRINGIFY_IMPL(x) #x

#define ARCHON_CONCAT(a, b) ARCHON_CONCAT_IMPL(a, b)
#define ARCHON_CONCAT_IMPL(a, b) a ## b

#define ARCHON_CONCAT_3(a, b, c) ARCHON_CONCAT_3_IMPL(a, b, c)
#define ARCHON_CONCAT_3_IMPL(a, b, c) a ## b ## c

#define ARCHON_CONCAT_4(a, b, c, d) ARCHON_CONCAT_4_IMPL(a, b, c, d)
#define ARCHON_CONCAT_4_IMPL(a, b, c, d) a ## b ## c ## d


// Debug mode
//
#if !defined ARCHON_DEBUG
#  if !defined NDEBUG
#    define ARCHON_DEBUG 1
#  else
#    define ARCHON_DEBUG 0
#  endif
#endif


// Sanitizers
//
#if !defined ARCHON_ASAN
#  define ARCHON_ASAN 0
#endif
#if !defined ARCHON_TSAN
#  define ARCHON_TSAN 0
#endif


// Windows platform
//
#if !defined ARCHON_WINDOWS
#  if defined WIN32 || defined _WIN32 || defined __WIN32__
#    define ARCHON_WINDOWS 1
#  else
#    define ARCHON_WINDOWS 0
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


#if __GNUC__
#  define ARCHON_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#  define ARCHON_LIKELY(expr)   __builtin_expect(!!(expr), 1)
#else
#  define ARCHON_UNLIKELY(expr) (expr)
#  define ARCHON_LIKELY(expr)   (expr)
#endif


// Try to force inlining
//
#if __GNUC__
#  define ARCHON_FORCEINLINE inline __attribute__((always_inline))
#elif defined _MSC_VER
#  define ARCHON_FORCEINLINE __forceinline
#else
#  define ARCHON_FORCEINLINE inline
#endif


// Try to prevent inlining
//
#if __GNUC__
#  define ARCHON_NOINLINE __attribute__((noinline))
#elif defined _MSC_VER
#  define ARCHON_NOINLINE __declspec(noinline)
#else
#  define ARCHON_NOINLINE
#endif


// If `ARCHON_WCHAR_IS_UNICODE` is true, it means that we should assume that the
// STL locale facilities assume that the character encoding for `wchar_t` is UCS
// in all locales.
//
#if defined __STDC_ISO_10646__
#  define ARCHON_WCHAR_IS_UNICODE 1
#else
#  define ARCHON_WCHAR_IS_UNICODE 0
#endif


#endif // ARCHON_X_BASE_X_FEATURES_H
