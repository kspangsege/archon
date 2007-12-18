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

#ifndef ARCHON_DOM_BOOTSTRAP_HPP
#define ARCHON_DOM_BOOTSTRAP_HPP

#include <vector>

#include <archon/dom/core.hpp>


namespace Archon
{
  namespace dom
  {
    namespace bootstrap
    {
      /**
       * A factory that enables applications to obtain instances of
       * DOMImplementation.
       */
      struct DOMImplementationRegistry: DOMObject
      {
        /**
         * Obtain a new instance of a DOMImplementationRegistry.
         */
        static ref<DOMImplementationRegistry> newInstance();


        /**
         * Return the first implementation that has the desired features, or
         * null if none are found.
         */
        virtual ref<DOMImplementation>
        getDOMImplementation(DOMString const &features) const throw ();


        /**
         * Return a list of implementations that support the desired
         * features.
         */
        virtual ref<DOMImplementationList>
        getDOMImplementationList(DOMString const &features) const throw ();


        /**
         * Register an implementation.
         */
        virtual void addSource(ref<DOMImplementationSource> const &s);


        virtual ~DOMImplementationRegistry() throw () {}

      private:
        typedef ref<DOMImplementationSource> Source;
        typedef std::vector<Source> Sources;
        Sources sources;

        DOMImplementationRegistry();
      };
    }
  }
}


#endif // ARCHON_DOM_BOOTSTRAP_HPP
