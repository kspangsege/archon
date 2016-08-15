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

#include <string>
#include <iostream>

#include <archon/core/config.hpp>


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

    Alpha(Cfg const &cfg)
    {
      cerr << "Alpha(lorem:"<<cfg.lorem<<", ipsum:"<<cfg.ipsum<<")" << endl;
    }
  };

  struct Beta: Alpha
  {
    struct Cfg
    {
      Alpha::Cfg alpha;
      bool flag;

      Cfg(): flag(false) {}

      void populate(ConfigBuilder &cfg);
    };

    Beta(Cfg const &cfg): Alpha(cfg.alpha)
    {
      cerr << "Beta(flag:"<<cfg.flag<<")" << endl;
    }
  };


  void Alpha::Cfg::populate(ConfigBuilder &cfg)
  {
    cfg.add_param("l", "lorem", lorem, "The quick brown fox jumps over the lazy dog");
    cfg.add_param("i", "ipsum", ipsum, "Do Androids Dream of Electric Sheep?");
  }

  void Beta::Cfg::populate(ConfigBuilder &cfg)
  {
    cfg.add_group(alpha, "alpha");
    cfg.add_param("f", "flag", flag, "Set to true if robots will inherit the earth?");
  }
}

int main() throw()
{
  Config cfg;

  Beta::Cfg beta_cfg;
  cfg.add_group(beta_cfg);

  wcout.imbue(locale(""));

  int const n = cfg.get_num_params();
  wcout << L"num_params = " << n << endl;
  for(int i=0; i<n; ++i)
  {
    wcout << " "<<i<<") "
      L"-"<<cfg.get_param_short_name(i)<<L" "
      L"--"<<cfg.get_param_long_name(i)<<L" = "
      L"'"<<cfg.get_param_val(i)<<L"' "
      L"type="<<cfg.get_param_type(i)<<L" "
      L"bool="<<(cfg.is_param_bool(i) ? L"Yes" : L"No") << endl;
    wcout << L"   " << cfg.get_param_description(i) << endl;
  }

  return 0;
}
