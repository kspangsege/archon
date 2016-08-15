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
 * Testing utility codecs.
 */

#include <stdexcept>
#include <string>

#include <archon/util/codec.hpp>

using namespace std;
using namespace archon::core;
using namespace archon::Util;

int main(int argc, char const *argv[]) throw()
{
  if(argc != 2)
    throw invalid_argument("Wrong number of command line arguments");
  string type = argv[1];

  UniquePtr<InputStream> in(make_stdin_stream().release());
  UniquePtr<OutputStream> out(make_stdout_stream().release());

  if(type == "block-encode")
  {
    UniquePtr<OutputStream> out2(get_block_codec()->get_enc_out_stream(*out).release());
    out.release();
    out = out2;
  }
  else if(type == "block-decode")
  {
    UniquePtr<InputStream> in2(get_block_codec()->get_dec_in_stream(*in).release());
    in.release();
    in = in2;
  }
  else throw invalid_argument("Unrecognized encoding/decoding '"+type+"'");

  out->write(string("Kristian"));

  out->flush();

  out->write(*in);

  return 0;
}
