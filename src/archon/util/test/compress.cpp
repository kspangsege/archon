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
/// Testing compression codecs.

#include <string>

#include <archon/core/options.hpp>
#include <archon/util/compress.hpp>

using namespace archon::core;
using namespace archon::util;

int main(int argc, const char* argv[])
{
    std::string opt_codec = "lzw";
    bool opt_decode = false;
    bool opt_input = false;

    CommandlineOptions opts;
    opts.add_help("Test application for utility codecs");
    opts.check_num_args();
    opts.add_param("c", "codec", opt_codec, "The codec to use.");
    opts.add_param("d", "decode", opt_decode, "Decode rather than encode.");
    opts.add_param("i", "input", opt_input,
                   "Encode/decode using an input stream rather than an output stream.");
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    std::unique_ptr<const Codec> codec;
    if (opt_codec == "lzw") {
        codec = get_lempel_ziv_welch_codec();
    }
    else {
        throw std::invalid_argument("Unknown codec '"+opt_codec+"'");
    }

    std::shared_ptr<InputStream> in = make_stdin_stream();
    std::shared_ptr<OutputStream> out = make_stdout_stream();

    if (opt_input) {
        in = (opt_decode ? codec->get_dec_in_stream(in) : codec->get_enc_in_stream(in));
    }
    else {
        out = (opt_decode ? codec->get_dec_out_stream(out) : codec->get_enc_out_stream(out));
    }

    out->write(*in);
    out->flush();
}
