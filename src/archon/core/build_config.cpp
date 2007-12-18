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
 */

#include <stdexcept>

#include <archon/core/file.hpp>
#include <archon/core/text.hpp>
#include <archon/core/mutex.hpp>
#include <archon/core/build_config.hpp>
#include <archon/core/build_config.dyn.h>


using namespace std;
using namespace Archon::Core;


namespace
{
  Mutex mutex;
  string data_dir = ARCHON_BUILD_CONFIG_DATA_DIR; // Protected by 'mutex'

  string get_data_dir()
  {
    Mutex::Lock lock(mutex);
    return data_dir;
  }
}


namespace Archon
{
  namespace Core
  {
    string get_value_of(BuildConfigParam p)
    {
      switch(p) {
      case build_config_param_DataDir: return get_data_dir();
      }
      throw runtime_error("Unexpected parameter");
    }


    void try_fix_preinstall_datadir(string argv0, string subdir)
    {
      string dir = File::dir_of(argv0);
      if (dir.empty()) return;
      dir = File::canonicalize_path(File::resolve_path(dir, File::get_cwd()));

      // A special hook to recognize and handle the case where the
      // executing program is invoked through a Libtool wrapper.
      string const libtool_dir = ".libs/";
      if (Text::is_suffix("/"+libtool_dir, dir)) {
        // The 'lt-' prefix appears to not always be used
//        if (Text::is_prefix("lt-", File::name_of(argv0))) {
          dir = Text::get_prefix(libtool_dir, dir, true);
//        }
      }

      if (!subdir.empty()) {
        if (!Text::is_suffix("/"+subdir, dir)) return;
        dir = Text::get_prefix(subdir, dir, true);
      }

      if(File::exists(dir+"core/build_config.hpp")) {
        Mutex::Lock lock(mutex);
        data_dir = dir;
      }
    }
  }
}

