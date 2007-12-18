/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_FEATURES_H
#define ARCHON_FEATURES_H


#define ARCHON_TO_STRING(x) #x
#define ARCHON_JOIN(x,y) ARCHON_JOIN2(x,y)
#define ARCHON_JOIN2(x,y) x ## y


#if __cplusplus >= 201103 || __GXX_EXPERIMENTAL_CXX0X__
#  define ARCHON_HAVE_CXX11 1
#endif


/* See these links for information about feature check macroes in GCC,
 * Clang, and MSVC:
 *
 * http://gcc.gnu.org/projects/cxx0x.html
 * http://clang.llvm.org/cxx_status.html
 * http://clang.llvm.org/docs/LanguageExtensions.html#checks-for-standard-language-features
 * http://msdn.microsoft.com/en-us/library/vstudio/hh567368.aspx
 * http://sourceforge.net/p/predef/wiki/Compilers
 */


/* Compiler is GCC and version is greater than or equal to the
 * specified version */
#define ARCHON_HAVE_AT_LEAST_GCC(maj, min) \
    (__GNUC__ > (maj) || __GNUC__ == (maj) && __GNUC_MINOR__ >= (min))

#if __clang__
#  define ARCHON_HAVE_CLANG_FEATURE(feature) __has_feature(feature)
#else
#  define ARCHON_HAVE_CLANG_FEATURE(feature) 0
#endif

/* Compiler is MSVC (Microsoft Visual C++) */
#if _MSC_VER >= 1600
#  define ARCHON_HAVE_AT_LEAST_MSVC_10_2010 1
#endif
#if _MSC_VER >= 1700
#  define ARCHON_HAVE_AT_LEAST_MSVC_11_2012 1
#endif
#if _MSC_VER >= 1800
#  define ARCHON_HAVE_AT_LEAST_MSVC_12_2013 1
#endif


/* Support for C++11 <type_traits>. */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 3) || \
    ARCHON_HAVE_CXX11 && _LIBCPP_VERSION >= 1001 || \
    ARCHON_HAVE_AT_LEAST_MSVC_10_2010
#  define ARCHON_HAVE_CXX11_TYPE_TRAITS 1
#endif


/* Support for C++11 <atomic>. */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 4) || \
    ARCHON_HAVE_CXX11 && _LIBCPP_VERSION >= 1001 || \
    ARCHON_HAVE_AT_LEAST_MSVC_11_2012
#  define ARCHON_HAVE_CXX11_ATOMIC 1
#endif


/* Support for C++11 variadic templates. */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 3) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_variadic_templates) || \
    ARCHON_HAVE_AT_LEAST_MSVC_12_2013
#  define ARCHON_HAVE_CXX11_VARIADIC_TEMPLATES 1
#endif


/* Support for C++11 static_assert(). */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 3) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_static_assert) || \
    ARCHON_HAVE_AT_LEAST_MSVC_10_2010
#  define ARCHON_HAVE_CXX11_STATIC_ASSERT 1
#endif


/* Support for C++11 r-value references and std::move().
 *
 * NOTE: Not yet fully supported in MSVC++ 12 (2013). */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC_4_3 || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_rvalue_references) && _LIBCPP_VERSION >= 1001
#  define ARCHON_HAVE_CXX11_RVALUE_REFERENCE 1
#endif


/* Support for the C++11 'decltype' keyword. */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 3) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_decltype) || \
    ARCHON_HAVE_AT_LEAST_MSVC_12_2013
#  define ARCHON_HAVE_CXX11_DECLTYPE 1
#endif


/* Support for C++11 initializer lists. */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 4) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_generalized_initializers) || \
    ARCHON_HAVE_AT_LEAST_MSVC_12_2013
#  define ARCHON_HAVE_CXX11_INITIALIZER_LISTS 1
#endif


/* Support for C++11 explicit conversion operators. */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 5) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_explicit_conversions) || \
    ARCHON_HAVE_AT_LEAST_MSVC_12_2013
#  define ARCHON_HAVE_CXX11_EXPLICIT_CONV_OPERATORS 1
#endif


/* Support for the C++11 'constexpr' keyword.
 *
 * NOTE: Not yet fully supported in MSVC++ 12 (2013). */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 6) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_constexpr)
#  define ARCHON_HAVE_CXX11_CONSTEXPR 1
#endif
#if ARCHON_HAVE_CXX11_CONSTEXPR
#  define ARCHON_CONSTEXPR constexpr
#else
#  define ARCHON_CONSTEXPR
#endif


/* Support for the C++11 'noexcept' specifier.
 *
 * NOTE: Not yet fully supported in MSVC++ 12 (2013). */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 6) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_noexcept)
#  define ARCHON_HAVE_CXX11_NOEXCEPT 1
#endif
#if ARCHON_HAVE_CXX11_NOEXCEPT
#  define ARCHON_NOEXCEPT noexcept
#elif defined ARCHON_NDEBUG
#  define ARCHON_NOEXCEPT
#else
#  define ARCHON_NOEXCEPT throw()
#endif
#if ARCHON_HAVE_CXX11_NOEXCEPT
#  define ARCHON_NOEXCEPT_IF(cond) noexcept(cond)
#else
#  define ARCHON_NOEXCEPT_IF(cond)
#endif
#if ARCHON_HAVE_CXX11_NOEXCEPT
#  define ARCHON_NOEXCEPT_OR_NOTHROW noexcept
#else
#  define ARCHON_NOEXCEPT_OR_NOTHROW throw()
#endif


/* Support for C++11 explicit virtual overrides */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 7) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_override_control) || \
    ARCHON_HAVE_AT_LEAST_MSVC_11_2012
#  define ARCHON_OVERRIDE override
#else
#  define ARCHON_OVERRIDE
#endif


/* The way to specify that a function never returns.
 *
 * NOTE: C++11 generalized attributes are not yet fully supported in
 * MSVC++ 12 (2013). */
#if ARCHON_HAVE_CXX11 && ARCHON_HAVE_AT_LEAST_GCC(4, 8) || \
    ARCHON_HAVE_CLANG_FEATURE(cxx_attributes)
#  define ARCHON_NORETURN [[noreturn]]
#elif __GNUC__
#  define ARCHON_NORETURN __attribute__((noreturn))
#elif _MSC_VER
#  define ARCHON_NORETURN __declspec(noreturn)
#else
#  define ARCHON_NORETURN
#endif


/* The way to specify that a variable or type is intended to possibly
 * not be used. Use it to suppress a warning from the compiler. */
#if __GNUC__
#  define ARCHON_UNUSED __attribute__((unused))
#else
#  define ARCHON_UNUSED
#endif


#if __GNUC__
#  define ARCHON_UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#  define ARCHON_LIKELY(expr)   __builtin_expect(!!(expr), 1)
#else
#  define ARCHON_UNLIKELY(expr) (expr)
#  define ARCHON_LIKELY(expr)   (expr)
#endif


#if __GNUC__
    #define ARCHON_FORCEINLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
    #define ARCHON_FORCEINLINE __forceinline
#else
    #define ARCHON_FORCEINLINE inline
#endif


#endif /* ARCHON_FEATURES_H */
