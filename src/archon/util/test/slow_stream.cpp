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
 * Testing the slow stream.
 */

#include <iostream>

#include <archon/core/options.hpp>
#include <archon/util/stream.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::util;

int main(int argc, char const *argv[]) throw()
{
  double opt_rate  = 3.3;
  double opt_size  = 3.3;

  CommandlineOptions opts;
  opts.add_help("Test application for slow streams");
  opts.check_num_args();
  opts.add_param("r", "rate", opt_rate,
                 "The average transfer rate in bytes per second");
  opts.add_param("s", "size", opt_size,
                 "Set average chunk size");
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;

  UniquePtr<InputStream>  in(make_stdin_stream().release());
  UniquePtr<OutputStream> out(make_stdout_stream().release());

  out->write(*make_slow_stream(*in, opt_rate, opt_size));

  return 0;
}
