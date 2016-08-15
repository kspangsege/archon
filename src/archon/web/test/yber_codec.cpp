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
 * Testing the Yber codec.
 */

#include <iostream>

#include <archon/web/yber_codec.hpp>

using namespace std;
using namespace archon::Web;
using namespace archon::core;

int main(int argc, char const *argv[]) throw()
{
  string s = argc < 2 ? "hest*eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee" : argv[1];
  UniquePtr<Codec const> yber(get_yber_codec().release());
  cout << yber->decode(s) << endl;

  return 0;
}
