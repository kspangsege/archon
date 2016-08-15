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

#ifndef ARCHON_DOM_IMPL_HTML_HPP
#define ARCHON_DOM_IMPL_HTML_HPP

#include <map>

#include <archon/dom/html.hpp>
#include <archon/dom/impl/ls.hpp>
#include <archon/dom/impl/render.hpp>


namespace archon
{
  namespace dom_impl
  {
    class HTMLImplementation;



    struct HTMLElement: virtual dom::html::HTMLElement, RenderElement
    {
      virtual dom::DOMString getId() const throw ();

      virtual void setId(dom::DOMString const &id) throw (dom::DOMException);

      HTMLElement(StyledElemType *t): RenderElement(t) {}
    };




    struct HtmlElemType: StyledElemType
    {
      // Overriding method in ElemType
      virtual HTMLElement *create_element();


      HtmlElemType(StyledDocument *d, bool read_only, ElemKey const &k, ElemQual const &q):
        StyledElemType(d, read_only, k, q) {}
    };




    struct HTMLDocument: dom::html::HTMLDocument, RenderDocument
    {
      enum Mode
      {
        /**
         * Document was served with MIME type 'application/xml'.
         */
        mode_XML = 0,

        /**
         * Document was served with MIME type 'application/xhtml+xml'.
         */
        mode_XHTML = 1,

        /**
         * Document was served with MIME type 'text/html' and a DTD
         * that selects strict mode.
         */
        mode_HTML_Strict = 2,

        /**
         * Document was served with MIME type 'text/html' and a DTD
         * that selects almost strict mode.
         */
        mode_HTML_AlmostStrict = 3,

        /**
         * Document was served with MIME type 'text/html' and no DTD,
         * or a DTD that selects quirks mode.
         */
        mode_HTML_Quirks = 4
      };

      Mode const mode;



      virtual dom::ref<dom::html::HTMLElement> getBody() const throw ();

      virtual void setBody(dom::ref<dom::html::HTMLElement> const &) throw (dom::DOMException);


      HTMLImplementation *get_impl() const;

      bool is_xml() const { return mode <= mode_XHTML; }


      // Overriding method in RenderDocument
      virtual ElemTypeRef create_elem_type(bool read_only, ElemKey const &key,
                                           dom::DOMString const &prefix,
                                           dom::DOMString const &local_name) const;

      // Overriding method in RenderDocument
      virtual void before_children_change() throw ()
      {
        RenderDocument::before_children_change();
        valid_body = false;
      }

      void find_body() const;

      HTMLDocument(HTMLImplementation *i, Mode m);

    private:
      mutable bool valid_body;
      mutable HTMLElement *body;
    };




    struct HTMLImplementation: StyledImplementation
    {
      dom::DOMString const str_feat_html;
      dom::DOMString const str_ns_xhtml;

      bool const quirk_dom1_api_sets_ns_and_local_name;

      // Overriding method in DOMImplementationLS
      virtual bool has_feature(dom::DOMString const &, dom::DOMString const &) const throw ();


      // Overriding method in DOMImplementation
      virtual dom::ref<Document> create_document(dom::DocumentType const *doctype) const;


      struct Config
      {
        // Gecko, Trident, WebKit, and Presto say TRUE. The specification says FALSE.
        bool dom1_api_sets_ns_and_local_name;

        CssLevel css_level;

        Config(): dom1_api_sets_ns_and_local_name(false), css_level(css3) {}
      };


      HTMLImplementation(Config const &c = Config());

      virtual ~HTMLImplementation() throw () {}


    private:
      friend class HTMLDocument;
      typedef ElemType *(*HtmlElemTypeCtor)(HTMLDocument *doc, bool read_only,
                                            ElemKey const &k, ElemQual const &q);
      typedef std::map<dom::DOMString, HtmlElemTypeCtor> HtmlElemTypeCtors;
      HtmlElemTypeCtors htmlElemTypeCtorsByName;
      HtmlElemTypeCtors htmlElemTypeCtorsByCfName; // By case folded name

      template<class T> void add_html_elem_type_ctor(dom::DOMString const &n)
      {
        dom::DOMString n2 = n;
        HtmlElemTypeCtor const ctor = &html_elem_type_ctor<T>;
        htmlElemTypeCtorsByName[n2] = ctor;
        case_fold_ascii(n2);
        htmlElemTypeCtorsByCfName[n2] = ctor;
      }

      template<class T>
      static ElemType *html_elem_type_ctor(HTMLDocument *doc, bool read_only,
                                           ElemKey const &k, ElemQual const &q)
      {
        return new typename T::ElemType(doc, read_only, k, q);
      }
    };




    struct HTMLHtmlElement: virtual dom::html::HTMLHtmlElement, HTMLElement
    {
      struct ElemType: HtmlElemType
      {
        // Overriding method in ElemType
        virtual HTMLElement *create_element()
        {
          return new HTMLHtmlElement(this);
        }

        ElemType(HTMLDocument *d, bool read_only, ElemKey const &k, ElemQual const &q):
          HtmlElemType(d, read_only, k, q) {}
      };


    protected:
      HTMLHtmlElement(StyledElemType *t): HTMLElement(t) {}
    };




    struct HTMLBodyElement: virtual dom::html::HTMLBodyElement, HTMLElement
    {
      struct ElemType: HtmlElemType
      {
        // Overriding method in ElemType
        virtual HTMLElement *create_element()
        {
          return new HTMLBodyElement(this);
        }

        ElemType(HTMLDocument *d, bool read_only, ElemKey const &k, ElemQual const &q):
          HtmlElemType(d, read_only, k, q) {}
      };


    protected:
      HTMLBodyElement(ElemType *t): HTMLElement(t) {}
    };




    struct HTMLDivElement: virtual dom::html::HTMLDivElement, HTMLElement
    {
      struct ElemType: HtmlElemType
      {
        // Overriding method in ElemType
        virtual HTMLElement *create_element()
        {
          return new HTMLDivElement(this);
        }

        ElemType(HTMLDocument *d, bool read_only, ElemKey const &k, ElemQual const &q):
          HtmlElemType(d, read_only, k, q) {}
      };


    protected:
      HTMLDivElement(ElemType *t): HTMLElement(t) {}
    };




    struct HTMLParagraphElement: virtual dom::html::HTMLParagraphElement, HTMLElement
    {
      struct ElemType: HtmlElemType
      {
        // Overriding method in ElemType
        virtual HTMLElement *create_element()
        {
          return new HTMLParagraphElement(this);
        }

        ElemType(HTMLDocument *d, bool read_only, ElemKey const &k, ElemQual const &q):
          HtmlElemType(d, read_only, k, q) {}
      };


    protected:
      HTMLParagraphElement(ElemType *t): HTMLElement(t) {}
    };




    struct HTMLUListElement: virtual dom::html::HTMLUListElement, HTMLElement
    {
      struct ElemType: HtmlElemType
      {
        // Overriding method in ElemType
        virtual HTMLElement *create_element()
        {
          return new HTMLUListElement(this);
        }

        virtual bool is_element_content() const { return true; }

        ElemType(HTMLDocument *d, bool read_only, ElemKey const &k, ElemQual const &q):
          HtmlElemType(d, read_only, k, q) {}
      };


    protected:
      HTMLUListElement(ElemType *t): HTMLElement(t) {}
    };




    struct HTMLLIElement: virtual dom::html::HTMLLIElement, HTMLElement
    {
      struct ElemType: HtmlElemType
      {
        // Overriding method in ElemType
        virtual HTMLElement *create_element()
        {
          return new HTMLLIElement(this);
        }

        ElemType(HTMLDocument *d, bool read_only, ElemKey const &k, ElemQual const &q):
          HtmlElemType(d, read_only, k, q) {}
      };


    protected:
      HTMLLIElement(ElemType *t): HTMLElement(t) {}
    };










    // Implementation:

    inline HTMLElement *HtmlElemType::create_element()
    {
      return new HTMLElement(this);
    }



    inline HTMLImplementation *HTMLDocument::get_impl() const
    {
      return static_cast<HTMLImplementation *>(impl.get());
    }



    inline void HTMLDocument::find_body() const
    {
      HTMLElement *b = 0;
      Element *const root = get_root();
      if (root && dynamic_cast<HTMLHtmlElement *>(root)) {
        Node *c = root->get_first_child();
        while (c) {
          if (c->get_type()->id == ELEMENT_NODE) {
            b = dynamic_cast<HTMLBodyElement *>(c);
            if (b) break;
          }
          c = c->get_next_sibling();
        }
      }
      body = b;
      valid_body = true;
    }
  }
}


#endif // ARCHON_DOM_IMPL_HTML_HPP
