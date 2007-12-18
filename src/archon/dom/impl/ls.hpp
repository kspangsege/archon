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

#ifndef ARCHON_DOM_IMPL_LS_HPP
#define ARCHON_DOM_IMPL_LS_HPP

#include <archon/dom/ls.hpp>
#include <archon/dom/impl/core.hpp>


namespace Archon
{
  namespace DomImpl
  {
    struct DOMImplementationLS: virtual dom::ls::DOMImplementationLS, DOMImplementation
    {
      virtual dom::ref<dom::ls::LSParser>
      createLSParser(dom::uint16 mode, dom::DOMString const &schemaType) const
        throw (dom::DOMException);

      virtual dom::ref<dom::ls::LSInput> createLSInput() const throw ();


      dom::DOMString const str_feat_ls;

      // Overriding method in DOMImplementation
      virtual bool has_feature(dom::DOMString const &, dom::DOMString const &) const throw ();


      DOMImplementationLS();

      virtual ~DOMImplementationLS() throw () {}
    };
  }
}


#endif // ARCHON_DOM_IMPL_LS_HPP
