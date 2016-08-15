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

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_DISPLAY_GEOMETRY_HPP
#define ARCHON_DISPLAY_GEOMETRY_HPP

#include <vector>
#include <string>

#include <archon/core/time.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/display/window.hpp>


namespace archon {
namespace display {

struct Point {
    int x = 0;
    int y = 0;
};

struct Box {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

} // namespace display
} // namespace archon

#endif // ARCHON_DISPLAY_GEOMETRY_HPP
