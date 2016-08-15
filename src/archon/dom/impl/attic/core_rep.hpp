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

/*
 * This file is part of ArchonJS, the Archon JavaScript library.
 *
 * Copyright (C) 2008  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * ArchonJS is free software: You can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ArchonJS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ArchonJS.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_DOM_IMPL_CORE_REP_HPP
#define ARCHON_DOM_IMPL_CORE_REP_HPP

#include <iostream>

#include <archon/core/unique_ptr.hpp>
#include <archon/util/hashing.hpp>
#include <archon/util/hash_map.hpp>
#include <archon/dom/core.hpp>


/*


getElementsByTagName
--------------------

NodeList->item(): Brute force traversal, however, the pair
(index,node) is stored for the last invocation of NodeList->index(),
and if it is still valid at the next invocation of index(), traversal
will be relative to the previous index. The cached pair must be
invalidated if a matching element is added or removed from the tree
(or changes type from or to a mathcing namespace and name), or a
parent of such an element is added or removed to the tree. The tree is
the subtree rooted at the target of the getElementsByTagName()
invocation. Matching can in some cases simply be a match on the
ElemType pointer.

NodeList->length(): More or less like NodeList.item(), however, in
this case we must traverse the entire subtree whenever the length
cache has been invalidated. If might be possible to keep the length
cache valid by edjusting it required.

This requires direct access to previousSibling.



Required and very common properties with a small set of possible values:
------------------------------------------------------------------------

These should be handed as distinct element types and therfore refer to
distinct instances of ElemType. These attributes should not invoke the
Atributes instance in ElemAux.



"In tree" or "out of tree":
---------------------------

Consider having two instances for every NodeType, one for "in tree"
and one for "out of tree", this way we can quickly detect whether we
are "in tree" or not, and for example, we can avoid clearing tree
caches when "out of tree" nodes are manipulated.


Element attributes:
-------------------

AttribsRep:
  AttrRep *id;
  RareAttribs *rare;

RareAttribs:
  

CssAttribsRep: AttribsRep:
  AttrRep *class;
  StyleAttrRep *style;

HtmlAnchorAttribsRep: CssAttribsRep: (only when 'href' is specified) (questionable whether this is worth the while)
  AttrRep *href;


*/


namespace archon
{
  namespace DomImpl
  {
    template<class T> struct CreatRef
    {
      T *operator->() throw ()
      {
        if (!p) p = new T;
        return p;
      }

      void destroy() throw ()
      {
        delete p;
        p = 0;
      }

      CreatRef() throw (): p(0) {}
      ~CreatRef() throw () { delete p; }

    private:
      T *p;

      // Forbid copying
      CreatRef(CreatRef const &);
      CreatRef &operator=(CreatRef const &) const;
    };





    struct TreeNodeRep;
    struct ParentRep;
    struct ElemRep;
    struct CharDataRep;
    struct DocRep;
    struct ExternTreeNodeRefFactory;



    struct ExternTreeNodeRefFactory
    {
      virtual dom::Node *make_extern_element_ref(ElemRep *) throw () = 0;
      virtual dom::Node *make_extern_text_ref(CharDataRep *) throw () = 0;
      virtual dom::Node *make_extern_document_ref(DocRep *) throw () = 0;

      virtual ~ExternTreeNodeRefFactory() {}
    };



    struct TreeNodeType
    {
      DocRep *const doc;

      TreeNodeType(DocRep *d) throw (): doc(d) {}

      // FIXME: No need for this virtual call, callers can branch on type_id
      virtual dom::Node *
      recover_extern_ref(TreeNodeRep *r, ExternTreeNodeRefFactory *f) const throw () = 0;

      virtual ~TreeNodeType() throw () {}
    };



    // Ownership is shared among the element representations using
    // this type.
    struct ElemType: TreeNodeType
    {
      dom::DOMString const tag_name; // Never null
      dom::DOMString const namespace_uri;
      dom::DOMString const prefix; // Is null if 'namespace_uri' is null
      dom::DOMString const local_name; // Is null for, and only for elements created by DOM Level 1 methods.

      ElemType(DocRep *d, dom::DOMString const &t, dom::DOMString const &n,
               dom::DOMString const &p, dom::DOMString const &l) throw ():
        TreeNodeType(d), tag_name(t), namespace_uri(n), prefix(p), local_name(l), num_refs(0) {}

      ~ElemType() throw () { unregister(); }

      void bind() const throw () { ++num_refs; }
      void unbind() const throw () { if (--num_refs == 0) delete this; }

      virtual dom::Node *
      recover_extern_ref(TreeNodeRep *r, ExternTreeNodeRefFactory *f) const throw ();

      void unregister() throw ();

    private:
      mutable int num_refs;
    };



    // Owned by the document.
    struct CharDataType: TreeNodeType
    {
      dom::DOMString const node_name;

      CharDataType(DocRep *d, dom::DOMString const &n) throw (): TreeNodeType(d), node_name(n) {}
      ~CharDataType() throw () {}

      virtual dom::Node *
      recover_extern_ref(TreeNodeRep *r, ExternTreeNodeRefFactory *f) const throw ();
    };






    /**
     * Common base for represenations of nodes that can be returned by
     * Node::getFirstChild() or Node::getParentNode(): Document,
     * DocumentFragment, DocumentType, EntityReference, Element,
     * ProcessingInstruction, Comment, Text, CDATASection.
     *
     * The ownership is shared between its parent node representation
     * and external references to this node representation.
     */
    struct TreeNodeRep
    {
      TreeNodeType const *get_type() const { return type; }

      ParentRep *get_parent() const { return parent; }

    protected:
      TreeNodeRep(TreeNodeType const *t) throw (): type(t), parent(0), prev(0), next(0) {}
      ~TreeNodeRep() throw () {}

    private:
      TreeNodeType const *type;
      ParentRep *parent;
      TreeNodeRep *prev, *next;
    };



    /**
     * Common base for represenations of nodes that can be returned by
     * Node::getFirstChild() or Node::getParentNode(): Document,
     * DocumentFragment, DocumentType, EntityReference, Element,
     * ProcessingInstruction, Comment, Text, CDATASection.
     */
    struct ChildRep: TreeNodeRep
    {
    protected:
      template<class Rep>
      static dom::Node *child_recover_extern_ref(Rep *r, ExternTreeNodeRefFactory *f) throw ();

      template<class Rep> static void child_extern_unbind(Rep *);

      // Ownership of the memory allocated to 'child' is passed from
      // the caller to the callee. 'child' must not already be owned
      // by any list. Returns the appended node.
//       void append_as_child_unchecked(ElemRef *new_parent) throw ()
//       {
//         ChildRep *const last  = p->get_last_child();
//         parent = new_parent;
//         prev   = last;
//         next   = 0;
//         (last ? last->next : first_child) = child;
//       }

      // Remove this child from its parents list of children. This
      // must only be done when it is guaranteed that there is an
      // external reference to the child.
//       void remove_from_parent_unchecked()
//       {
//         // FIXME: Assert that there is an external reference
//         if (parent->get_type()->type_id == dom::Node::ELEMENT_NODE) {
//           ElemRep *p = static_cast<ElemRep *>(parent);
//           (prev ? prev->next : p->get_first_child()) = next;
//           (next ? next->prev : p->get_last_child())  = prev;
//         }
//         else {
//           // Document needs special handling.
//           throw std::runtime_error("FIXME");
//         }
//         parent = prev = next = 0;
//       }



    struct ElemAux
    {
//      Attributes attribs;
//      Style style;
      dom::Node *ext_ref;

      ElemAux(): ext_ref(0) {}
    };



    struct ElemRep: ChildRep
    {
      ElemRep(ElemType const *t, dom::Node *n=0) throw (): ChildRep(t), first_child(0)
      {
        t->bind();
        if (n) aux->ext_ref = n;
      }

      ~ElemRep() throw ()
      {
        static_cast<ElemType const *>(get_type())->unbind();
      }

      ElemType const *get_type() const { return static_cast<ElemType const *>(get_type()); }

      bool has_children() const { return first_child; }

      void append_child(ChildRep *) throw (dom::DOMException)
      {
//         // FIXME: Fail if this element is 'read only' - implement by flag in Aux or on TreeNodeType.
//         TreeNodeRep *const childs_parent = c->get_parent();
//         if (childs_parent) {
//           if (childs_parent == this) goto release;
//             // FIXME: Fail if childs parent is 'read only' - implement by flag in Aux or on TreeNodeType.
//         }

//         // Prevent creation of a cycle
//         {
//           dom::uint16 type_id = c->get_type()->type_id;
//           if (type_id == dom::Node::ELEMENT_NODE) {
//             if (c == this) {
//             cycle:
//               dom::DOMException(dom::DOMException::HIERARCHY_REQUEST_ERR,
//                                 "Attempt to create cycle");
//             }
//             if (static_cast<ElemRep *>(c)->has_children) {
//               TreeNodeRep *p = get_parent();
//               while (p->get_type()->node_id == dom::Node::ELEMENT_NODE) {
//                 if (p == c) goto cycle;
//                 p = static_cast<ElemRep *>(p)->get_parent();
//               }
//             }
//           }
//         }

//         if (childs_parent) {
//         release:
//           if (childs_parent->get_type()->type_id == dom::Node::ELEMENT_NODE) {
//             throw std::runtime_error("FIXME");
//            static_cast<ElemRep *>(childs_parent)->remove_child_unchecked(c);
//           }
//           else {
//             throw std::runtime_error("FIXME");
//           }
//         }
        throw std::runtime_error("FIXME");
//        append_child(child2);
      }

      // Must only be called for a newly created element
      // representation where there is no chance that it already has
      // an external reference.
      void set_extern_ref_of_new_rep(dom::Node *n)
      {
        aux->ext_ref = n;
      }

      dom::Node *recover_extern_ref(ExternTreeNodeRefFactory *f) throw ()
      {
        return child_recover_extern_ref(this, f);
      }

      void extern_unbind() throw ()
      {
        child_extern_unbind(this);
      }

    private:
      template<class Rep> friend
      dom::Node *ChildRep::child_recover_extern_ref(Rep *, ExternTreeNodeRefFactory *) throw ();

      template<class Rep> friend void ChildRep::child_extern_unbind(Rep *);

      ChildRep *first_child; // How about number of children?
      CreatRef<ElemAux> aux;
//      LayoutBox *box; // Includes ComputedStyle

      void clear_extern_ref() throw ()
      {
        if (true) { // FIXME: Because there is nothing else in 'aux' !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          aux.destroy();
        }
      }
    };



    struct TextAux
    {
      dom::Node *ext_ref;

      TextAux(): ext_ref(0) {}
    };



    /**
     * Common representation for CharacterData nodes, that is: Text,
     * Comment, CDATASection
     */
    struct CharDataRep: ChildRep
    {
      CharDataRep(CharDataType const *t, dom::Node *n=0) throw (): ChildRep(t)
      {
        if (n) aux->ext_ref = n;
      }

      CharDataType const *get_type() const { return static_cast<CharDataType const *>(get_type()); }

      // Must only be called for a newly created text representation
      // where there is no chance that it already has an external
      // reference.
      void set_extern_ref_of_new_rep(dom::Node *n)
      {
        aux->ext_ref = n;
      }

      dom::Node *recover_extern_ref(ExternTreeNodeRefFactory *f) throw ()
      {
        return child_recover_extern_ref(this, f);
      }

      void extern_unbind() throw ()
      {
        child_extern_unbind(this);
      }

      dom::DOMString get_text() const throw () { return text; }
      void set_text(dom::DOMString const &t) throw () { text = t; }

    private:
      template<class Rep> friend
      dom::Node *ChildRep::child_recover_extern_ref(Rep *, ExternTreeNodeRefFactory *) throw ();

      template<class Rep> friend void ChildRep::child_extern_unbind(Rep *);

      dom::DOMString text;
      CreatRef<TextAux> aux;

      void clear_extern_ref() throw ()
      {
        if (true) { // FIXME: Because there is nothing else in 'aux' !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
          aux.destroy();
        }
      }
    };




    struct DocRep: virtual dom::Document, TreeNodeType, TreeNodeRep
    {
      virtual ElemRep *create_elem_rep(ElemType *type) { return new ElemRep(type); }

      dom::ref<dom::DOMImplementation> const impl;

      DocRep(dom::DOMImplementation *i) throw (): TreeNodeType(this), TreeNodeRep(this), impl(i) {}
      ~DocRep() throw () { std::cerr << "~DocRep()" << std::endl; }

      void on_unreferenced() throw ()
      {
        // FIXME: DELETE ALL CHILDREN BEFORE DELTING OURSELVES SUCH THAT ELEMENT TYPES DO NOT ATTEMPT TO ACCESS US AFTER DELETION HAS STARTED
        delete this;
      }

      virtual dom::Node *
      recover_extern_ref(TreeNodeRep *, ExternTreeNodeRefFactory *) const throw () { return const_cast<DocRep *>(this); }

    protected:
      ElemRep *create_elem_rep(dom::DOMString const &ns_uri, dom::DOMString const &qname)
      {
        dom::DOMString::size_type const i = qname.find(0x3A); // Find colon
        bool const has_prefix = i != dom::DOMString::npos;
        bool const has_local_name = true;
        ElemTypeKey key(ns_uri, qname, has_prefix, has_local_name);
        ElemType *&type_ref = elem_types[key];
        ElemType *type = type_ref;
        if (type) return create_elem_rep(type);
        dom::DOMString const prefix = has_prefix ? qname.substr(0, i) : dom::DOMString();
        dom::DOMString const local_name = has_prefix ? qname.substr(i+1) : qname;
        Core::UniquePtr<ElemType> owned_type(new ElemType(this, qname, ns_uri,
                                                          prefix, local_name));
        type_ref = owned_type.get();
        ElemRep *const rep = create_elem_rep(owned_type.get());
        owned_type.release();
        return rep;
      }


    private:
      friend void ElemType::unregister(); // May call unregister_elem_type()

      struct ElemTypeKey
      {
        dom::DOMString namespace_uri;
        dom::DOMString tag_name;
        int flags; // bit 0 = has prefix, bit 1 = has local name
        ElemTypeKey(dom::DOMString const &n, dom::DOMString const &t,
                    bool has_prefix, bool has_local_name):
          namespace_uri(n), tag_name(t), flags((has_prefix?1:0)|(has_local_name?2:0)) {}
        bool operator==(ElemTypeKey const &k) const
        {
          return tag_name == k.tag_name && flags == k.flags && namespace_uri == k.namespace_uri;
        }
      };

      struct ElemTypeHashFunc
      {
        static int hash(ElemTypeKey const &k, int n)
        {
          Util::Hash_FNV_1a_32 h;
          h.add_sequence(k.tag_name.begin(), k.tag_name.end());
          return h.get_hash(n);
        }
      };

      Util::HashMap<ElemTypeKey, ElemType *, ElemTypeHashFunc> elem_types;

      void unregister_elem_type(ElemTypeKey const &key)
      {
        elem_types.erase(key);
      }
    };




    struct ImplRep: virtual dom::DOMImplementation
    {
      ~ImplRep() throw () { std::cerr << "~ImplRep()" << std::endl; }
    };







    // Implementation:

    inline dom::Node *
    ElemType::recover_extern_ref(TreeNodeRep *r, ExternTreeNodeRefFactory *f) const throw ()
    {
      return static_cast<ElemRep *>(r)->recover_extern_ref(f);
    }


    inline void ElemType::unregister() throw ()
    {
      DocRep::ElemTypeKey const key(namespace_uri, tag_name, prefix, local_name);
      doc->unregister_elem_type(key);
    }


    inline dom::Node *
    CharDataType::recover_extern_ref(TreeNodeRep *r, ExternTreeNodeRefFactory *f) const throw ()
    {
      return static_cast<CharDataRep *>(r)->recover_extern_ref(f);
    }


    template<class Rep>
    dom::Node *ChildRep::child_recover_extern_ref(Rep *r, ExternTreeNodeRefFactory *) throw ()
    {
      dom::Node *&n = r->aux->ext_ref;
      if (!n) {
//        n = f->make_extern_element_ref(r); // FIXME: Caching!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  FIXME: REENABLE
        // Bind the external reference to the document
        r->get_type()->doc->bind_ref();
      }
      return n;
    }


    template<class Rep> void ChildRep::child_extern_unbind(Rep *r)
    {
      // The external ref must unbind from the document after this
      // node representation is destroyed (if it must be), because
      // otherwise the node representation would potentially be kept
      // alive after the death of the document. On the other hand,
      // we must make a copy of the document reference before we
      // destroy the node representation (if we have to), because
      // otherwise the path to it will have ceased to exist.
      DocRep *const doc = r->get_type()->doc;
      if (!r->parent) {
        delete r;
      }
      else {
        r->clear_extern_ref();
      }
      doc->unbind_ref();
    }


//     struct Document: virtual dom::Document
//     {
//       Core::SharedPtr<Implementation> const impl;

//       dom::DOMString getNodeName() const throw () { return impl->doc_node_name; }
//       dom::DOMString getNodeValue() const throw () { return dom::DOMString(); }
//       Core::UIntMin16 getNodeType() const throw () { return dom::Node::DOCUMENT_NODE; }
//       Core::SharedPtr<dom::Document> getOwnerDocument() const throw () { return 0; }
//       dom::DOMString getNamespaceURI() const throw () { return dom::DOMString(); }
//       Core::SharedPtr<Implementation> getImplementation() const throw () { return impl; }

//       Document(Core::SharedPtr<Implementation> const &i): impl(i), num_external_refs(0) {}

//       ~Document() { std::cerr << "DomImpl::~Document()" << std::endl; }

//       void external_bind()
//       {
//         ++num_external_refs;
//       }

//       void external_unbind()
//       {
//         if (--num_external_refs == 0) REMOVE ALL CHILD NODES, THIS MUST LEAD TO A CASCADING DESTRUCTION OF ALL NODES ENDING WITH THIS DOCUMENT;
//       }


//     private:

//       int num_external_refs;

//     public:
//       Core::SharedPtr<com::Element>
//       createElementNS(dom::DOMString const &namespace_uri, dom::DOMString const &qualified_name)
//         throw (DOMException)
//       {
//         dom::DOMString::size_type const i = qualified_name.find(0x3A); // Find colon
//         bool const has_prefix = i != dom::DOMString::npos; // Colon
//         bool const has_local_name = true;
//         ElemTypeKey key(qualified_name, namespace_uri, has_prefix, has_local_name);
//         ElemType *&type = elem_types[key];
//         if (!type) {
//           dom::DOMString const prefix =
//             has_prefix ? qualified_name.substr(0, i) : dom::DOMString();
//           dom::DOMString const local_name =
//             has_prefix ? qualified_name.substr(i+1) : qualified_name;
//           new ElemType(get_shared_self(), qualified_name, namespace_uri, prefix, local_name);
//         }
//       }
//     };



//     // Ownership: Shared between all that hold a std::shared_ptr<>.
//     struct Implementation: virtual dom::Implementation
//     {
//       dom::DOMString const doc_node_name;

//       Implementation(): doc_node_name(FIXME) {}

//       ~Implementation() { std::cerr << "DomImpl::~Implementation()" << std::endl; }
//     };





// //HMM - How to handle multiple simultaneous bindings? JavaScript + C++ + Java  -  efficiency! hash_map?


//   struct Document: virtual dom::Document, DomImpl::ExtDocRef
//   {
//     static Core::SharedPtr<Document> get(DomImpl::Doc *rep)
//     {
//       if (doc->ext_ref) {
//         return doc->ext_ref->self();
//       }
//       shared_ptr<Document> doc = new Document(d);
//       doc->self = doc;
//       return doc;
//     }

//     ~Document()
//     {
//     }

//   private:
//     Document(DomImpl::Doc *d): rep(d) {}

//     Core::WeakPtr<Document> self;
//     DomImpl::Doc *const rep;
//   };


  }
}


#endif // ARCHON_DOM_IMPL_CORE_REP_HPP
