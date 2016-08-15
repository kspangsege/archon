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

#ifndef ARCHON_CORE_GENERATOR_HPP
#define ARCHON_CORE_GENERATOR_HPP

namespace archon
{
  namespace core
  {
    template<typename T> struct Generator
    {
      /**
       * Request generation of the next item or an indication that
       * no more items are available. This method shall return true
       * to indicate that an item was generated. When no more items
       * are available it shall return false, and the passed
       * variable shall not be modified. It is legal to call this
       * method again after it has returned false, but it must then
       * return false again and leave the argument untouched.
       *
       * \param i The generated item will be assigned to this variable.
       *
       * \return True if an item was generated, otherwise false.
       */
      virtual bool generate(T &i) = 0;

      virtual ~Generator() {}
    };
  }
}

#endif // ARCHON_CORE_GENERATOR_HPP
