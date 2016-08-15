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

#ifndef ARCHON_DOM_UTIL_OBJECT_HPP
#define ARCHON_DOM_UTIL_OBJECT_HPP

#include <archon/core/assert.hpp>
#include <archon/core/bind_ref.hpp>


namespace archon
{
  namespace dom
  {
    struct DOMObject
    {
      virtual ~DOMObject() throw () { ARCHON_ASSERT(ref_count == 0); }

//      int get_reference_count() const throw () { return ref_count; }

    protected:
      DOMObject() throw (): ref_count(0) {}

      bool is_referenced() const throw () { return ref_count != 0; }

      friend struct Core::DefaultBindTraits;
      void bind_ref()   const throw () { if (ref_count++ == 0) on_referenced();   }
      void unbind_ref() const throw () { if (--ref_count == 0) on_unreferenced(); }

      void bind_ref_n(int n) const throw ()
      {
        bool const z = ref_count == 0;
        ref_count += n;
        if (z) on_referenced();
      }

      void unbind_ref_n(int n) const throw ()
      {
        ref_count -= n;
        if (ref_count == 0) on_unreferenced();
      }

      virtual void on_referenced()   const throw () {}
      virtual void on_unreferenced() const throw () { delete this; }

    private:
      mutable int ref_count;
    };
  }
}


#endif // ARCHON_DOM_UTIL_OBJECT_HPP
