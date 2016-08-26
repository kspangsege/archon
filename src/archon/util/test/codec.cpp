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
///
/// Testing utility codecs.

#include <stdexcept>
#include <string>

#include <archon/util/codec.hpp>

using namespace archon::core;
using namespace archon::util;

int main(int argc, const char* argv[])
{
    if (argc != 2)
        throw std::invalid_argument("Wrong number of command line arguments");
    std::string type = argv[1];

    std::shared_ptr<InputStream> in = make_stdin_stream();
    std::shared_ptr<OutputStream> out = make_stdout_stream();

    if (type == "block-encode") {
        out = get_block_codec()->get_enc_out_stream(out);
    }
    else if(type == "block-decode")
    {
        in = get_block_codec()->get_dec_in_stream(in);
    }
    else {
        throw std::invalid_argument("Unrecognized encoding/decoding '"+type+"'");
    }

    out->write(std::string("Kristian"));

    out->flush();

    out->write(*in);
}
