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
 * Testing the features of <tt>char_enc.hpp</tt>.
 */

#include <stdexcept>
#include <string>
#include <iostream>

#include <archon/core/char_enc.hpp>

using namespace std;
using namespace archon::core;

int main() throw()
{
  ios_base::sync_with_stdio(false);
  locale loc(locale::classic(), locale("en_US.UTF-8"), locale::ctype);
  wcin.imbue(loc);
  wcout.imbue(loc);

  wstring s;
  wchar_t buf[4096];
  while (wcin) {
    wcin.read(buf, sizeof(buf)/sizeof(*buf));
    streamsize const n = wcin.gcount();
    s.append(buf, n);
  }

  CharEnc<CharUtf16> codec(loc);

  StringUtf16 t;
  if (!codec.encode(s, t)) throw runtime_error("encode");

  wstring s2;
  if (!codec.decode(t, s2)) throw runtime_error("decode");

  wcout << s2;

  return 0;
}
