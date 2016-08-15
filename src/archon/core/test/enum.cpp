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
 *
 * Testing the endowed enums.
 */

#include <iostream>

#include <archon/core/enum.hpp>


// Module header

namespace Module
{
  enum Color
  {
    orange, purple, brown
  };

  struct ColorSpec { static archon::Core::EnumAssoc map[]; };
  typedef archon::Core::Enum<Color, ColorSpec> ColorEnum;
}


// Module implementation

using namespace std;
using namespace archon::Core;

namespace Module
{
  archon::Core::EnumAssoc ColorSpec::map[] =
  {
    { orange, "orange" },
    { purple, "purple" },
    { brown,  "brown"  },
    { 0, 0 }
  };
}



// Module application

using namespace Module;

int main() throw()
{
  ColorEnum a(orange), b(purple), c(brown);

  cout << a<<", "<<b<<", "<<c << endl;

  cout << "Color: ";
  cin >> a;
  if(!cin) throw runtime_error("Bad read");
  cout << "Was: " << a << endl;

  return 0;
}
