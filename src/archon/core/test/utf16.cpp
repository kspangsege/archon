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

#include <stdexcept>
#include <sstream>
#include <iostream>

#include <archon/core/utf16.hpp>

/*
#define TEST(assertion)              if(!(assertion)) throw std::runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw std::runtime_error(message)
*/

using namespace archon::core;


int main()
{
    // FIXME: Code fails to compile with LLVM libc++ seemingly due to a bug in libc++.
/*
    std::basic_ostringstream<CharUtf16> out;

    CharUtf16 c;
    c.val = 10;

    out << 77.9 << c;

    std::wcout << out.str().size() << std::endl;

    std::wcout << utf16_to_wide(out.str(), std::locale::classic()) << std::endl;
*/
}
