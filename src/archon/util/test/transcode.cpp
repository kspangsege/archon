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
 * Testing text transcoding.
 */

#include <string>
//#include <iostream>

#include <archon/util/transcode.hpp>

using namespace std;
using namespace archon::Core;
using namespace archon::Util;

int main() throw()
{
  //cout << transcode("Kim Possible æøå - nice\xC3", "UTF-8", "ASCII", false) << endl;
//  get_transcoding_output_stream(make_stdout_stream(), "UTF-8", "ISO-8859-1")->write(get_transcoding_input_stream(make_stdin_stream(), "ISO-8859-1", "UTF-8"));
  get_transcoding_output_stream(*make_stdout_stream(), "UTF-8", "ISO-8859-1")->write(*make_stdin_stream());
  //make_stdout_stream()->write(get_transcoding_input_stream(make_stdin_stream(), "UTF-8", "ISO-8859-1"));
  return 0;
}
