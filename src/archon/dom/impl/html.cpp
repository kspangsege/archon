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

#include <archon/dom/impl/html.hpp>


using namespace std;


namespace archon
{
  namespace dom_impl
  {
    dom::DOMString HTMLElement::getId() const throw ()
    {
//      return get_attr_value(get_doc()->impl->attr_type_id);
      return dom::DOMString();
    }



    void HTMLElement::setId(dom::DOMString const &/*v*/) throw (dom::DOMException)
    {
//      set_attr_value(get_doc()->impl->attr_type_id, v);
    }



    dom::ref<dom::html::HTMLElement> HTMLDocument::getBody() const throw ()
    {
      if (!valid_body) find_body();
      return dom::ref<dom::html::HTMLElement>(body);
    }



    void HTMLDocument::setBody(dom::ref<dom::html::HTMLElement> const &b) throw (dom::DOMException)
    {
      HTMLBodyElement *b2 = dynamic_cast<HTMLBodyElement *>(b.get());
      if (!b2) throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR,
                                       "Specified element is no a valid body");
      if (!valid_body) find_body();
      Element *const root = get_root();
      if (body) root->add_child<add_mode_Replace>(b2, body);
      else {
        if (!root || !dynamic_cast<HTMLHtmlElement *>(root))
          throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "Root is not a valid HTML element");
        root->add_child<add_mode_Append>(b2);
      }
      body = b2;
    }



    // Overriding method in RenderDocument
    ElemTypeRef HTMLDocument::create_elem_type(bool read_only, ElemKey const &key,
                                               dom::DOMString const &prefix,
                                               dom::DOMString const &local_name) const
    {
      ElemQual qual;
      qual.tag_name = key.tag_name;
      dom::DOMString html_name;
      bool case_insensitive = false;
      bool const no_xml = mode != mode_XML && mode != mode_XHTML;
      if (key.dom1) {
        bool set_xhtml_ns = false;
        if (get_impl()->quirk_dom1_api_sets_ns_and_local_name) {
          if (mode != mode_XML) set_xhtml_ns = true;
          qual.local_name = key.tag_name;
          if (no_xml) to_lower_case_ascii(qual.local_name);
        }
        if (no_xml || set_xhtml_ns) {
          html_name = key.tag_name;
          qual.ns_uri = get_impl()->str_ns_xhtml;
          if (no_xml) {
            to_upper_case_ascii(qual.tag_name);
            case_fold_ascii(html_name);
            case_insensitive = true;
          }
        }
      }
      else {
        qual.ns_uri     = key.ns_uri;
        qual.prefix     = prefix;
        qual.local_name = local_name;
        bool const is_xhtml_ns = key.ns_uri == get_impl()->str_ns_xhtml;
        if (no_xml && is_xhtml_ns) to_upper_case_ascii(qual.tag_name);
        if (is_xhtml_ns) html_name = local_name;
      }

      HTMLDocument *const doc = const_cast<HTMLDocument *>(this);
      if (!html_name.empty()) {
        typedef HTMLImplementation::HtmlElemTypeCtors Ctors;
        Ctors const &ctors = case_insensitive ? get_impl()->htmlElemTypeCtorsByCfName :
          get_impl()->htmlElemTypeCtorsByName;
        Ctors::const_iterator const i = ctors.find(html_name);
        if (i != ctors.end()) {
          HTMLImplementation::HtmlElemTypeCtor const ctor = i->second;
          return ElemTypeRef((*ctor)(doc, read_only, key, qual));
        }

        return ElemTypeRef(new HtmlElemType(doc, read_only, key, qual));
      }

      return ElemTypeRef(new ElemType(doc, read_only, key, qual));
    }



    HTMLDocument::HTMLDocument(HTMLImplementation *i, Mode m):
      RenderDocument(i), mode(m), valid_body(false) {}



    // Overriding method in DOMImplementation
    bool HTMLImplementation::has_feature(dom::DOMString const &f,
                                         dom::DOMString const &v) const throw ()
    {
      if (f == str_feat_html) {
        return v.empty() || v == str_ver_1_0 || v == str_ver_2_0;
      }
      return DOMImplementationLS::has_feature(f,v);
    }



    // Overriding method in DOMImplementation
    dom::ref<Document> HTMLImplementation::create_document(dom::DocumentType const *) const
    {
      // COMPATIBILITY NOTE: Gecko creates an HTML mode document if
      // the doctype is specified and it has a public ID that
      // indicates HTML 4.0 or HTML 4.01. WebKit and Presto does not.

      HTMLImplementation *const i = const_cast<HTMLImplementation *>(this);
      return dom::ref<Document>(new HTMLDocument(i, HTMLDocument::mode_XML));
    }



    HTMLImplementation::HTMLImplementation(Config const &c):
      StyledImplementation(c.css_level),
      str_feat_html(dom::str_from_cloc(L"HTML")),
      str_ns_xhtml(dom::str_from_cloc(L"http://www.w3.org/1999/xhtml")),
      quirk_dom1_api_sets_ns_and_local_name(c.dom1_api_sets_ns_and_local_name)
    {
      add_html_elem_type_ctor<HTMLHtmlElement>(dom::str_from_cloc(L"html"));
      add_html_elem_type_ctor<HTMLBodyElement>(dom::str_from_cloc(L"body"));
      add_html_elem_type_ctor<HTMLDivElement>(dom::str_from_cloc(L"div"));
      add_html_elem_type_ctor<HTMLParagraphElement>(dom::str_from_cloc(L"p"));
      add_html_elem_type_ctor<HTMLUListElement>(dom::str_from_cloc(L"ul"));
      add_html_elem_type_ctor<HTMLLIElement>(dom::str_from_cloc(L"li"));
    }
  }
}
