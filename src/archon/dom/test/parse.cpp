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

#include <stdexcept>
#include <iostream>

#include <archon/core/cxx.hpp>
#include <archon/core/text.hpp>
#include <archon/dom/bootstrap.hpp>
#include <archon/dom/ls.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::Core;
namespace dom = archon::dom;
namespace bootstrap = archon::dom::bootstrap;
namespace ls = archon::dom::ls;
using dom::dynamic_pointer_cast;


namespace
{
  Text::WideOptionalWordQuoter quoter;


  inline wstring fmt(dom::DOMString const &s)
  {
    return quoter.print(dom::str_to_wide(s));
  }


  wstring const ind_step = Text::widen_port<wchar_t>("  ", locale());


  void dump(dom::ref<dom::Node> const &n, wstring ind)
  {
    if (dom::DocumentType *doctype = dynamic_cast<dom::DocumentType *>(n.get())) {

      wcout << ind << "Doctype: " << fmt(doctype->getName()) << "\n";
      wcout << ind << "  Public ID = " << fmt(doctype->getPublicId()) << "\n";
      wcout << ind << "  System ID = " << fmt(doctype->getSystemId()) << "\n";

      wcout << ind << "  Entities:\n";
      dom::ref<dom::NamedNodeMap> entities = doctype->getEntities();
      for (dom::uint32 i=0; i<entities->getLength(); ++i) {
        dom::ref<dom::Entity> e = dynamic_pointer_cast<dom::Entity>(entities->item(i));
        dump(e, ind + ind_step + ind_step);
      }

      wcout << ind << "  Notations:\n";
      dom::ref<dom::NamedNodeMap> notations = doctype->getNotations();
      for (dom::uint32 i=0; i<notations->getLength(); ++i) {
        dom::ref<dom::Notation> e = dynamic_pointer_cast<dom::Notation>(notations->item(i));
        dump(e, ind + ind_step + ind_step);
      }

      wcout << ind << "  Internal subset = " << fmt(doctype->getInternalSubset()) << "\n";
      return;
    }

    wcout << ind << "Node: " << fmt(n->getNodeName()) << "  (" << Cxx::type(*n).c_str() << ")\n";
    if (!n->getNodeValue().empty()) {
      wcout << ind << "  Value: " << fmt(n->getNodeValue()) << "\n";
    }
    if (!n->getNamespaceURI().empty()) {
      wcout << ind << "  NS: " << fmt(n->getNamespaceURI()) << "\n";
    }
//     if (ref<NamedNodeMap> const attrs = n->getAttributes()) {
//       for (int i=0; i<attrs->getLength(); ++i) {
//         ref<Attr> const a = dynamic_pointer_cast<Attr>(attrs->item(i));
//         wcout << ind << "  Attr: " << a->getName() << " = " << fmt(a->getValue()) << "\n";
//         if (!a->getNamespaceURI().empty()) {
//           wcout << ind << "    NS: " << fmt(a->getNamespaceURI()) << "\n";
//         }
//       }
//     }
    dom::ref<dom::Node> c = n->getFirstChild();
    while (c) {
      dump(c, ind + ind_step);
      c = c->getNextSibling();
    }
  }
}



int main() throw()
{
  set_terminate(&Cxx::terminate_handler);

  dom::ref<bootstrap::DOMImplementationRegistry> const registry =
    bootstrap::DOMImplementationRegistry::newInstance();

  dom::ref<dom::DOMImplementation> const impl =
    registry->getDOMImplementation(dom::str_from_cloc(L"CORE 3.0 XML LS"));
  TEST_MSG(impl, "No such implementation");

  dom::ref<ls::DOMImplementationLS> const ls = dynamic_pointer_cast<ls::DOMImplementationLS>(impl);
  TEST_MSG(ls, "Wrong implementation");

  dom::ref<ls::LSInput> const input = ls->createLSInput();
  input->setByteStream(&cin);
  input->setSystemId(dom::str_from_cloc(L"/home/kristian/public_html/tests/funny.xml"));

  dom::ref<ls::LSParser> const parser =
    ls->createLSParser(ls::DOMImplementationLS::MODE_SYNCHRONOUS, dom::DOMString());

  dom::ref<dom::Document> const doc = parser->parse(input);

  wcout << "Document URI   = " << fmt(doc->getDocumentURI()) << "\n";
  wcout << "Input encoding = " << fmt(doc->getInputEncoding()) << "\n";
  wcout << "XML version    = " << fmt(doc->getXmlVersion()) << "\n";
  wcout << "XML encoding   = " << fmt(doc->getXmlEncoding()) << "\n";
  wcout << "XML standalone = " << (doc->getXmlStandalone()?"Yes":"No") << "\n";
  dump(doc, L"");

  return 0;
}
