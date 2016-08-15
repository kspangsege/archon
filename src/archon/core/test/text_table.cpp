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
 * Testing the text table renderer.
 */

#include <iostream>

#include <archon/core/text_table.hpp>


using namespace std;
using namespace archon::Core;

int main() throw()
{
  Text::Table table;

  // Make a visual difference between odd and even rows and between
  // odd and even columns
  table.get_odd_row_attr().set_bg_color(Term::color_White);
  table.get_odd_col_attr().set_bold();

  // Make the first row a header
  table.get_row(0).set_bg_color(Term::color_Default).set_reverse().set_bold();

  table.get_cell(0,0).set_text("Alpha");
  table.get_cell(0,1).set_text("Beta");
  table.get_cell(0,2).set_text("Gamma");
  table.get_cell(1,0).set_val(1);
  table.get_cell(1,1).set_val(2);
  table.get_cell(2,2).set_val(3);
  table.get_cell(3,0).set_val(4);
  table.get_cell(3,1).set_val(5);
  table.get_cell(3,2).set_val(6);
  table.get_cell(4,1).set_val(7);

  cout << table.print() << endl;

  return 0;
}
