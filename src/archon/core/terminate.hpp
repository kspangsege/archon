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

#ifndef ARCHON_CORE_TERMINATE_HPP
#define ARCHON_CORE_TERMINATE_HPP

/// \file
///
/// \author Kristian Spangsege

#include <cstdlib>
#include <string>

#include <archon/features.h>

#ifndef ARCHON_NDEBUG
#  define ARCHON_TERMINATE(message) archon::core::_impl::terminate((message), __FILE__, __LINE__)
#else
#  define ARCHON_TERMINATE(message) (static_cast<void>(sizeof (message)), std::abort())
#endif

namespace archon {
namespace core {
namespace _impl {

ARCHON_NORETURN void terminate(const char* message, const char* file, long line) ARCHON_NOEXCEPT;

} // namespace _impl
} // namespace core
} // namespace archon

#endif // ARCHON_CORE_TERMINATE_HPP
