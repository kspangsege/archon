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

#ifndef ARCHON_CORE_CXX_HPP
#define ARCHON_CORE_CXX_HPP

#include <typeinfo>
#include <string>

namespace archon
{
  namespace core
  {
    /**
     * Functions for demangling C++ type names.
     */
    namespace Cxx
    {
      /**
       * Tries to demangle the argument. If this is not possible then
       * the argument is returned unchanged.
       *
       * \param mangledName The mangled name.
       *
       * \return The demangled name.
       */
      std::string demangle(std::string mangledName);

      template<typename T> inline std::string type()
      {
        return demangle(typeid(T).name());
      }

      template<typename T> inline std::string type(T const &v)
      {
        return demangle(typeid(v).name());
      }

      void terminate_handler();
    }
  }
}

#endif // ARCHON_CORE_CXX_HPP
