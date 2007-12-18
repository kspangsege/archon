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

#ifndef ARCHON_DOM_LS_HPP
#define ARCHON_DOM_LS_HPP

#include <iosfwd>

#include <archon/dom/core.hpp>


namespace Archon
{
  namespace dom
  {
    namespace ls
    {
      typedef dom::DOMString DOMString;
//      typedef dom::DOMConfiguration DOMConfiguration;
      typedef dom::Node Node;
      typedef dom::Document Document;
      typedef dom::Element Element;

      class LSParser;
      class LSSerializer;
      class LSInput;
      class LSOutput;
      class LSParserFilter;
      class LSSerializerFilter;



      struct LSException: std::runtime_error
      {
        LSException(int16 c, std::string msg): std::runtime_error(msg), code(c) {}

        int16 const code;
      };

      int16 const PARSE_ERR     = 81;
      int16 const SERIALIZE_ERR = 82;



      struct DOMImplementationLS: virtual DOMObject
      {
        static uint16 const MODE_SYNCHRONOUS  = 1;
        static uint16 const MODE_ASYNCHRONOUS = 2;

        virtual ref<LSParser> createLSParser(uint16 mode, DOMString const &schemaType) const
          throw (DOMException) = 0;

//        virtual ref<LSSerializer> createLSSerializer() const throw () = 0;
        virtual ref<LSInput> createLSInput() const throw () = 0;
//        virtual ref<LSOutput> createLSOutput() const throw () = 0;
      };



      struct LSParser: virtual DOMObject
      {
//        virtual ref<DOMConfiguration> getDomConfig() const throw () = 0;
//        virtual ref<LSParserFilter> getFilter() const throw () = 0;
//        virtual void setFilter(ref<LSParserFilter> const &filter) throw () = 0;
//        virtual bool getAsync() const throw () = 0;
//        virtual bool getBusy() const throw () = 0;

        virtual ref<Document> parse(ref<LSInput> const &input)
          throw (dom::DOMException, LSException) = 0;

//        virtual ref<Document> parseURI(DOMString const &uri)
//          throw (dom::DOMException, LSException) = 0;

//        static uint16 const ACTION_APPEND_AS_CHILDREN = 1;
//        static uint16 const ACTION_REPLACE_CHILDREN   = 2;
//        static uint16 const ACTION_INSERT_BEFORE      = 3;
//        static uint16 const ACTION_INSERT_AFTER       = 4;
//        static uint16 const ACTION_REPLACE            = 5;

//        virtual ref<Node> parseWithContext(ref<LSInput> const &input,
//                                           ref<Node> const &contextArg,
//                                           uint16 action)
//          throw (dom::DOMException, LSException) = 0;

//        virtual void abort() throw () = 0;
      };



      struct LSInput: virtual DOMObject
      {
        virtual std::istream *getByteStream() const throw () = 0;

        /**
         * Ownership of the std::istream instance remains with the
         * caller.
         */
        virtual void setByteStream(std::istream *byteStream) throw () = 0;

//        virtual DOMString getStringData() const throw () = 0;
//        virtual void setStringData(DOMString const &stringData) throw () = 0;

        virtual DOMString getSystemId() const throw () = 0;
        virtual void setSystemId(DOMString const &systemId) throw () = 0;

//        virtual DOMString getPublicId() const throw () = 0;
//        virtual void setPublicId(DOMString const &publicId) throw () = 0;

//        virtual DOMString getBaseURI() const throw () = 0;
//        virtual void setBaseURI(DOMString const &baseURI) throw () = 0;

        virtual DOMString getEncoding() const throw () = 0;
        virtual void setEncoding(DOMString const &encoding) throw () = 0;

//        virtual bool getCertifiedText() const throw () = 0;
//        virtual void setCertifiedText(bool certifiedText) throw () = 0;
      };
    }
  }
}


#endif // ARCHON_DOM_LS_HPP
