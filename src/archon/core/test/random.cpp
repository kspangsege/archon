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
 * Testing random number generator.
 */

#include <cmath>
#include <iostream>

#include <archon/core/term.hpp>
#include <archon/core/random.hpp>
#include <archon/core/text.hpp>
#include <archon/core/text_hist.hpp>
#include <archon/core/series.hpp>
#include <archon/core/options.hpp>
#include <archon/core/memory.hpp>


using namespace std;
using namespace Archon::Core;

namespace
{
  bool check_distrib(string name, string opt, vector<double> &params)
  {
    if(!Text::is_prefix(name, opt)) return false;
    if(name.size() == opt.size()) return true;
    if(opt[name.size()] != ',') throw invalid_argument("Syntax error in distribution params");
    istringstream i(opt.substr(name.size()+1));
    Text::SimpleTokenizer<char> tokenizer(i, ",", Text::SimpleTokenizer<char>::incl_empty);
    string s;
    while(tokenizer.generate(s)) params.push_back(Text::parse<double>(s));
    if(params.empty()) throw invalid_argument("Syntax error in distribution params");
    return true;
  }
}


int main(int argc, char const *argv[]) throw()
{
  pair<int, int> term_size(80, 25);
  try { term_size = Term::get_terminal_size(); }
  catch(Term::NoTerminalException &) {}

  string         opt_distribution = "uniform";
  Series<2, int> opt_size(term_size.first, max(term_size.second-2, 1));
  int            opt_iterations   = 128*65536;

  CommandlineOptions o;
  o.add_help("Test Application for the random number generator");
  o.check_num_args();

  o.add_param("d", "distribution", opt_distribution,
              "Choose from:\n"
              "uniform[,a[,b]]\n"
              "normal[,mean[,deviation]]\n"
              "poisson,lambda\n"
              "finite[,prob1[,prob2[...]]]");
  o.add_param("s", "size",       opt_size,       "Set size of rendered histogram");
  o.add_param("i", "iterations", opt_iterations, "Number of extracted random values");

  if(int stop = o.process(argc, argv)) return stop == 2 ? 0 : 1;

  double a, b;
  vector<double> params;
  Random random;
  UniquePtr<Random::Distribution> distribution;
  bool discrete = false;
  if(check_distrib("uniform", opt_distribution, params))
  {
    if(2 < params.size()) throw invalid_argument("Too many distribution params");
    a = params.size() < 1 ? 0 : params[0];
    b = params.size() < 2 ? 1 : params[1];
    distribution.reset(new Random::UniformDistrib(&random, a, b));
  }
  else if(check_distrib("normal", opt_distribution, params))
  {
    if(2 < params.size()) throw invalid_argument("Too many distribution params");
    double mean      = params.size() < 1 ? 0 : params[0];
    double deviation = params.size() < 2 ? 1 : params[1];
    a = mean - 3.3 * deviation;
    b = mean + 3.3 * deviation;
    distribution.reset(Random::get_normal_distrib(mean, deviation).release());
  }
  else if(check_distrib("poisson", opt_distribution, params))
  {
    if(1 != params.size()) throw invalid_argument("Wrong number of distribution params");
    double lambda = params[0];
    a = 0;
    b = ceil(lambda + 4.5 * sqrt(lambda));
    distribution.reset(Random::get_poisson_distrib(lambda).release());
    discrete = true;
  }
  else if(check_distrib("finite", opt_distribution, params))
  {
    if(params.size() < 1) throw invalid_argument("Too few distribution params");
    a = 0;
    b = params.size();
    distribution.reset(Random::get_finite_distrib(params).release());
    discrete = true;
  }
  else throw invalid_argument("No such distribution '"+opt_distribution+"'");


  int const n = discrete ? min<int>(opt_size[1], b-a) : opt_size[1];
  Histogram hist(a,b,n);
  for(int i=0; i<opt_iterations; ++i) hist.add(distribution->get());
  hist.print(cout, false, opt_size[0]);

  return 0;
}
