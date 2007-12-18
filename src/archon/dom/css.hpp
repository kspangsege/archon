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

#ifndef ARCHON_DOM_CSS_HPP
#define ARCHON_DOM_CSS_HPP

#include <archon/dom/core.hpp>


namespace Archon
{
  namespace dom
  {
    namespace css
    {
      typedef dom::DOMString DOMString;



      struct CSSStyleDeclaration: virtual DOMObject
      {
        virtual DOMString getCssText() const throw () = 0;

        virtual DOMString getPropertyValue(DOMString const &propertyName) const throw () = 0;

        virtual void setProperty(DOMString const &propertyName, DOMString const &value,
                                 DOMString const &priority) throw (DOMException) = 0;
      };



      struct ElementCSSInlineStyle: virtual DOMObject
      {
        virtual ref<CSSStyleDeclaration> getStyle() const throw () = 0;
      };
    }
  }
}


#endif // ARCHON_DOM_CSS_HPP
