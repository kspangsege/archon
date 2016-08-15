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

#include <iostream>

#include <archon/core/series.hpp>
#include <archon/core/options.hpp>


using namespace std;
using namespace archon::core;


namespace
{
  struct Alpha
  {
    struct Cfg
    {
      int lorem;
      float ipsum;
      Cfg(): lorem(2), ipsum(9.9) {}
      void populate(ConfigBuilder &cfg);
    };
  };

  struct Beta: Alpha
  {
    struct Cfg
    {
      Alpha::Cfg alpha;
      bool flag;
      long ipsum;
      string ida;
      string gulf;
      Series<4,int> series;
      Cfg(): flag(false), ipsum('\0'), ida("fido"), gulf("Per seus"), series(2,3,7,8) {}
      void populate(ConfigBuilder &cfg);
    };
  };


  struct Gamma
  {
    struct Cfg
    {
      char goblin;
      Cfg(): goblin('%') {}
      void populate(ConfigBuilder &cfg);
    };
  };

  struct Delta: Gamma
  {
    struct Cfg
    {
      Gamma::Cfg gamma;
      void populate(ConfigBuilder &cfg);
    };
  };


  void Alpha::Cfg::populate(ConfigBuilder &cfg)
  {
    cfg.add_param("l", "lorem", lorem, "The quick brown fox jumps over the lazy dog");
    cfg.add_param("i", "ipsum", ipsum, "The number of androids that dream of electric sheep");
  }

  void Beta::Cfg::populate(ConfigBuilder &cfg)
  {
    cfg.add_group(alpha, "alpha");
    cfg.add_param("f", "flag", flag, "Set to true if robots rule the world");
    cfg.add_param("x", "ipsum", ipsum, "Ipsum");
    cfg.add_param("i", "ida",   ida,    "Halfway through their journey, they come across a witch who is to be burned at the stake. Jöns is sympathetic to the girl and contemplates killing her executioners, but decides against it as she is almost dead. Block asks her both at their first encounter in a village and as she is at the actual stake to summon Satan for him; he wants to ask the Devil about God.");
    cfg.add_param("G", "gulf",  gulf,   "Beat the horse");
    cfg.add_param("S", "series", series,   "Outcast");
  }


  void Gamma::Cfg::populate(ConfigBuilder &cfg)
  {
    cfg.add_param("G", "goblin", goblin, "Gamma Goblins");
  }

  void Delta::Cfg::populate(ConfigBuilder &cfg)
  {
    cfg.add_group(gamma, "gamma");
  }
}

int main(int argc, char const *argv[]) throw()
{
  Beta::Cfg beta_cfg;
  Delta::Cfg delta_cfg;
  int opt_seven = 7777777;

  CommandlineOptions opts;
  opts.add_help("Test application for the command-line interpreter "
                "of the archon::Core library.\n"
                "By Kristian Spangsege.", "  WIDTH  HEIGHT  ");
  opts.check_num_args(0,-1);
  opts.add_version("5.5");
  opts.add_stop_opts();
  opts.handle_config_file("/tmp/archon-core-test-options.conf");
  opts.add_group(beta_cfg, "beta");
  opts.add_group(delta_cfg, "delta");
  opts.add_param("7", "seven", opt_seven, "The Seventh Seal");

/*
  char const *argv[] = { "/tuba/rilo", "-l", "7", "" };
  int argc = sizeof(argv)/sizeof(argv[0]);
*/
  if(int stop = opts.process(argc, argv)) return stop == 2 ? 0 : 1;

  cerr << "argc = " << argc << endl;

  return 0;
}
