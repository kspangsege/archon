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
 * Testing the integer division and modulo functions of
 * <tt>functions.hpp</tt>.
 *
 * \todo FIXME: This file is not currently testing anything.
 */

#include <string>
#include <iostream>

#include <archon/core/functions.hpp>
#include <archon/core/text_table.hpp>

using namespace std;
using namespace Archon::Core;

int main() throw()
{
  Text::Table table;

  // Make a visual difference between odd and even rows and between
  // odd and even columns
  table.get_odd_row_attr().set_bg_color(Term::color_White);
  table.get_odd_col_attr().set_bold();

  // Make the first row a header
  table.get_row(0).set_bg_color(Term::color_Default).set_reverse().set_bold();
  table.get_col(0).set_bg_color(Term::color_Default).set_reverse().set_bold();

  int a_from = -6, a_to = 6;
  int b_from = -6, b_to = 6;

  for(int a = a_from; a <= a_to; ++a) table.get_cell(a-a_from+1, 0).set_val(a);
  for(int b = b_from; b <= b_to; ++b) table.get_cell(0, b-b_from+1).set_val(b);

  for(int a = a_from; a <= a_to; ++a)
    for(int b = b_from; b <= b_to; ++b)
      if(b) table.get_cell(a-a_from+1, b-b_from+1).set_val(a%b);

  cout << table.print() << endl;

  return 0;
}
