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

#include <stdexcept>
#include <iostream>

#include <archon/util/packed_trgb.hpp>


#define TEST(assertion)              if(!(assertion)) throw std::runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw std::runtime_error(message)


using namespace archon::core;
using namespace archon::util;


int main() throw()
{
//   {
//     PackedTRGB trgb;
//     std::ostringstream out;
//     out << s1;
//     std::istringstream in(out.str());
//     Series<3, int> s2(-1, -2, -3);
//     in >> s2;
//     TEST_MSG(!in.fail() && !in.bad() && in.eof(), "Parse");
//     TEST_MSG(s1[0] == s2[0], "First component");
//     TEST_MSG(s1[1] == s2[1], "Second component");
//     TEST_MSG(s1[2] == s2[2], "Third component");
//   }
//   {
//     Series<3, int> s1(12, 31, 471);
//     std::wostringstream out;
//     out << s1;
//     std::wistringstream in(out.str());
//     Series<3, int> s2(-1, -2, -3);
//     in >> s2;
//     TEST_MSG(!in.fail() && !in.bad() && in.eof(), "Parse, wide");
//     TEST_MSG(s1[0] == s2[0], "First component, wide");
//     TEST_MSG(s1[1] == s2[1], "Second component, wide");
//     TEST_MSG(s1[2] == s2[2], "Third component, wide");
//   }


  PackedTRGB trgb;

  std::cout << "TRGB: ";
  std::cin >> trgb;
  TEST_MSG(!std::cin.fail() && !std::cin.bad(), "Parse");

  std::cout << "IS REALLY: " << static_cast<PackedTRGB::value_type>(trgb) << "\n";

  std::cout << trgb;
  std::cout << "\n";
}
