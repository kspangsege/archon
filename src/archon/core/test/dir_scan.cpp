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
/// Testing the directory scanner.

#include <iostream>

#include <archon/core/dir_scan.hpp>


using namespace archon::core;

int main(int argc, const char* argv[])
{
    if (argc != 2)
        throw std::runtime_error("Wrong number of command line arguments");

    std::unique_ptr<DirScanner> s = DirScanner::new_dir_scanner(argv[1]);

    for (;;) {
        std::string e = s->next_entry();
        if (e.empty())
            break;
        std::cout << e << std::endl;
    }
}
