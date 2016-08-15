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
 * A small subset of the "HTML" extension to the "Document Object
 * Model Level 2" as defined by the World Wide Web Consortium (W3C).
 *
 * \sa http://www.w3.org/DOM/
 */

#ifndef ARCHON_DOM_HTML_HPP
#define ARCHON_DOM_HTML_HPP

#include <archon/dom/core.hpp>


namespace archon
{
  namespace dom
  {
    namespace html
    {
      typedef dom::DOMString DOMString;
      typedef dom::Node Node;
      typedef dom::Document Document;
      typedef dom::NodeList NodeList;
      typedef dom::Element Element;


      class HTMLElement;



      struct HTMLDocument: virtual Document
      {
        virtual ref<HTMLElement> getBody() const throw () = 0;

        virtual void setBody(ref<HTMLElement> const &body) throw (DOMException) = 0;
      };



      struct HTMLElement: virtual Element
      {
        virtual DOMString getId() const throw () = 0;

        virtual void setId(DOMString const &id) throw (DOMException) = 0;
      };



      struct HTMLHtmlElement: virtual HTMLElement
      {
      };



      struct HTMLBodyElement: virtual HTMLElement
      {
      };



      struct HTMLDivElement: virtual HTMLElement
      {
      };



      struct HTMLParagraphElement: virtual HTMLElement
      {
      };



      struct HTMLUListElement: virtual HTMLElement
      {
//        virtual bool getCompact() const throw () = 0;

//        virtual void setCompact(bool compact) throw () = 0;

//        virtual DOMString getType() const throw () = 0;

//        virtual void setType(DOMString const &type) throw () = 0;
      };



      struct HTMLLIElement: virtual HTMLElement
      {
//        virtual DOMString getType() const throw () = 0;

//        virtual void setType(DOMString const &type) throw () = 0;

//        virtual int32 getValue() const throw () = 0;

//        virtual void setValue(int32 value) throw () = 0;
      };
    }
  }
}


#endif // ARCHON_DOM_HTML_HPP
