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
 * Testing the regular expressions.
 */

#include <string>
#include <iostream>

#include <archon/core/codec.hpp>
#include <archon/parser/regex.hpp>
#include <archon/parser/regex_print.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::parser;

int main() throw()
{
  WideRegex::Exp e = WideRegex::alt(WideRegex::jux(WideRegex::alt(WideRegex::rep(WideRegex::alt(WideRegex::plus(WideRegex::str(L"Kim Possible")),
                                                                                                WideRegex::str(L"Ron Stoppable")), 2),
                                                                  WideRegex::bol()),
                                                   WideRegex::str(L"\x62c\x627\x645\x020\x64a\x64a\x647\x020\x628\x644\x648\x631\x645\x020\x628\x6ad\x627\x020\x636\x631\x631\x649\x020\x637\x648\x642\x648\x646\x645\x632")),
                                    WideRegex::jux(WideRegex::range(WideRegex::CharRange(L'a', L'z'), true), WideRegex::cla(WideRegex::digit, true)));

  WideRegexPrinter p;

  cout << env_encode(p.print(e)) << endl;

  return 0;
}
