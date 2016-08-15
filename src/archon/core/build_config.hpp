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
 * Access to Archon library configuration paramters whose values are
 * fixed at build time.
 */

#ifndef ARCHON_CORE_BUILD_CONFIG_HPP
#define ARCHON_CORE_BUILD_CONFIG_HPP

#include <string>


namespace archon
{
  namespace core
  {
    enum BuildConfigParam
    {
      /**
       * The intended path to the directory holding idiosyncratic
       * read-only architecture-independent data objects. Typically
       * "/usr/share/archon/". You may assume that it always has a
       * final slash.
       */
      build_config_param_DataDir
    };

    std::string get_value_of(BuildConfigParam);


    /**
     * Automatically detect when a program is executed from whithin
     * the source tree, and possibly before installation, and in that
     * case, update the value of 'DataDir' to reflect this fact.
     *
     * If the calling program selects a new directory as the
     * current working directory, make sure it calls this function
     * first, otherwise the result is unreliable.
     *
     * \param argv0 The value of \c argv[0] where \c argv is the
     * second argument passed to main().
     *
     * \param subdir The relative path within the source tree to the
     * subdirectory holding the executing program. It must be
     * specified relative to the root of the source tree. The root of
     * the source tree is the directory from which
     * 'core/build_config.hpp' can be resolved. The path must either
     * be empty (which means './') or contain a final slash. The path
     * must never contain segments equal to '.' or '..'.
     *
     * \note This function is thread-safe, that is, it is safe to have
     * some threads call this function while other threads call
     * get_value_of().
     */
    void try_fix_preinstall_datadir(std::string argv0, std::string subdir);
  }
}

#endif // ARCHON_CORE_BUILD_CONFIG_HPP
