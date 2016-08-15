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

#ifndef ARCHON_CORE_ASSERT_HPP
#define ARCHON_CORE_ASSERT_HPP

/// \file
///
/// \author Kristian Spangsege

#include <archon/features.h>

#if ARCHON_ENABLE_ASSERTIONS || !defined(ARCHON_NDEBUG)
#  define ARCHON_ASSERTIONS_ENABLED 1
#else
#  define ARCHON_ASSERTIONS_ENABLED 0
#endif

#if ARCHON_ASSERTIONS_ENABLED
#  include <archon/core/terminate.hpp>
#  define ARCHON_ASSERT(condition) \
    (ARCHON_LIKELY(condition) ? static_cast<void>(0) : \
     archon::core::_Impl::terminate("Assertion failed: " #condition, __FILE__, __LINE__))
#  define ARCHON_ASSERT_1(condition, message)    \
    (ARCHON_LIKELY(condition) ? static_cast<void>(0) : \
     archon::core::_Impl::terminate(message, __FILE__, __LINE__))
#else
#  define ARCHON_ASSERT(condition) \
    static_cast<void>(sizeof bool(condition))
#  define ARCHON_ASSERT_1(condition, message) \
    static_cast<void>(sizeof bool(condition) + sizeof (message))
#endif


#ifdef ARCHON_HAVE_CXX11_STATIC_ASSERT
#  define ARCHON_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#  define ARCHON_STATIC_ASSERT(condition, message) typedef \
    archon::core::_impl::static_assert_dummy<sizeof(archon::core::_impl:: \
        ARCHON_STATIC_ASSERTION_FAILURE<bool(condition)>)> \
    ARCHON_JOIN(_archon_static_assert_, __LINE__) ARCHON_UNUSED
namespace archon {
namespace core {
namespace _impl {
    template<bool> struct ARCHON_STATIC_ASSERTION_FAILURE;
    template<> struct ARCHON_STATIC_ASSERTION_FAILURE<true> {};
    template<int> struct static_assert_dummy {};
} // namespace _impl
} // namespace core
} // namespace archon
#endif

#endif // ARCHON_CORE_ASSERT_HPP
