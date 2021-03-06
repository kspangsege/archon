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
///
/// Testing the time representation.

#include <iostream>

#include <archon/core/time.hpp>


using namespace archon::core;


int main()
{
    Time t{7};

    std::cout << "bool(Time(7)) == " << bool(t) << std::endl;
    std::cout << "7 < 6 == " << (t < Time(6)) << std::endl;
    std::cout << "7 < 8 == " << (t < Time(8)) << std::endl;

    std::cout << "now = " << Time::now().format_rfc_1123() << std::endl;
}
