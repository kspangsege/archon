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

#include <archon/platform.hpp> // Never include in other header files

#include <cstdio> // Needed on Darwin
#ifdef HAVE_LIBREADLINE
#  include <cstdlib>
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else
char *readline(char const *prompt);
#    ifdef HAVE_READLINE_BIND_KEY
int rl_bind_key(int key, rl_command_func_t *function);
#    endif
#  endif
#  ifdef HAVE_READLINE_HISTORY
#    if defined(HAVE_READLINE_HISTORY_H)
#      include <readline/history.h>
#    elif defined(HAVE_HISTORY_H)
#      include <history.h>
#    else
void add_history(char const *);
#    endif
#  endif
#else
#  include <iostream>
#endif

#include <archon/core/mutex.hpp>
#include <archon/util/readline.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::Util;

namespace
{
  Mutex mutex;
  string prompt;            // Protected by 'mutex'
  bool occupied = false;    // Protected by 'mutex'
  bool read_called = false; // Protected by 'mutex'

  struct Acquisition
  {
    Acquisition()
    {
      Mutex::Lock l(mutex);
      if(occupied) throw Readline::OccupiedException();
      occupied = true;
    }

    ~Acquisition()
    {
      Mutex::Lock l(mutex);
      occupied = false;
    }
  };
}


namespace Archon
{
  namespace Util
  {
    namespace Readline
    {
      bool read(string &s)
      {
        Acquisition a;

        read_called = true;

#ifdef HAVE_LIBREADLINE
        char *const line = readline(prompt.c_str());
        if(!line) return false;

#  ifdef HAVE_READLINE_HISTORY
        // If the line has any text in it, save it on the history
        if(*line) add_history(line);
#  endif

        s.assign(line);
        free(line);
#else
        cout << prompt;
        getline(cin, s);
        if(cin.eof()) return false;
#endif
        return true;
      }


      void set_prompt(string text)
      {
        Acquisition a;
        prompt = text;
      }


      void disable_file_completion()
      {
        Acquisition a;
        if(read_called) throw runtime_error("attempt to disable file completion after "
                                            "the first call to the read function");
#ifdef HAVE_LIBREADLINE
#  ifdef HAVE_READLINE_BIND_KEY
        rl_bind_key('\t', rl_insert);
#  endif
#endif
      }
    }
  }
}
