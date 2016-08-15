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

#include <cstdlib>
#include <cmath>
#include <new>
#include <list>
#include <map>
#include <stack>
#include <iomanip>
#include <sstream>
#include <iostream>

#include <archon/platform.hpp> // Never include this one in header files

#include <GL/gl.h>
#ifdef ARCHON_HAVE_GLU
#include <GL/glu.h>
#endif

#include <archon/core/functions.hpp>
#include <archon/core/weak_ptr.hpp>
#include <archon/core/sys.hpp>
#include <archon/core/build_config.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/text.hpp>
#include <archon/util/ticker.hpp>
#include <archon/image/image.hpp>
#include <archon/display/implementation.hpp>
#include <archon/render/dialog.hpp>
#include <archon/render/css.hpp>
#include <archon/render/html.hpp>
#include <archon/render/app.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::font;
using namespace archon::display;
using namespace archon::render;


namespace
{
  typedef archon::render::TextFormatter TextFormatter; // Resolving ambiguity




//       Color color;
//       // URI image;
//       // repeat;
//       // attachment;
//       // position;

//     template<class T> struct Alloc
//     {
//       T *alloc()
//       {
//         if(unused.empty()) return new T;
//         T *p = unused.top();
//         unused.pop();
//         return p;
//       }

//       void free(T *p)
//       {
//         unused.push_back(p);
//       }

//       ~Alloc()
//       {
//         typedef typename stack<T *>::const_iterator iter;
//         iter const end = unused.end();
//         for (iter i=unused.begin(); i!=end; ++i) delete *i;
//       }

//     private:
//       stack<T *> unused;
//     };




/*

archon/dom/core.hpp
archon/dom/html.hpp
archon/dom/css.hpp
archon/dom/impl/
archon/dom/bind/

dom/browser

browser/browser.h

namespace archon::dom
{
  typedef util::NullableString<core::CharUtf16> DOMString;


  struct Document;
  struct Implementation;


  struct Node
  {
    virtual DOMString getNodeName() const throw () = 0;
    virtual DOMString getNodeValue() const throw () = 0;
    virtual core::UIntMin16 getNodeType() const throw () = 0;
    virtual core::SharedPtr<Document> getOwnerDocument() const throw () = 0;
    virtual DOMString getNamespaceURI() const throw () = 0;
    virtial bool isSameNode(core::SharedPtr<Node> const &other) = 0;

    virtual ~Node() {}
  };


  struct Element: virtual Node
  {
    virtual DOMString getTagName() const throw () = 0;
  };


  struct Document: virtual Node
  {
    virtual core::SharedPtr<Implementation> getImplementation() const throw () = 0;

    virtual core::SharedPtr<Element>
    createElementNS(DOMString const &namespaceURI, DOMString const &qualifiedName)
      throw (DOMException) = 0;
  };


  struct Implementation
  {
    virtual ~Implementation() {}
  };
}


THE DOCUMENT OWNS THE ROOT NODE. AN EXT REF KKEPS THE DOCUMENT ALIVE. EACH EXT REF IS COUNTED IN THE DOCUMENT.

Resolution:

Make the ExtNodeRef be an integral part of the API of the implementation.








New strategy:


Computed style is cached for every box (or node, or element) in the
following way: In each box there is a counted reference to a structure
which holds the computed style. When a child has the same computed
style as the parent, they refer to the same structure of computed
style. Also, a hash table is used to discover reuse of the same
computed style for boxes that are far apart on the tree. The structure
is itself a set of counted references to substructures, and the same
reuse pattern applies recursively.

Uninherited properties are groupd together in a substructure, and when
none of the properties, that are covered by a strucure, have
non-default values, the reference points to a statically provided
structure of default values.

A box also stores its structural layout (left, top, width,
height). The combination of this information and the computed style is
always enough to efficiently compute the used style for any element.

Rebuild the whole thing from scratch in new indpendant location.




Strategy:

DOM nodes must contain a reference count such that the application can share ownership with the implementation.

The entire DOM is rendered into a display list such that repeated renderings in each frame is as fast as possible when there are no modifications of the DOM (including viewport size).

Any change in the DOM must invalidate the display list.

Even in the face of DOM modification, enough information must be cached from the previous rendering, that the new rendering is reasonable fast when the modification are small (outline change, for example).

This cached information must also make it reasonable fast to query for used values via window.defaultView.getComputedStyle().

A rendering or a query through window.defaultView.getComputedStyle() might have to recompute some or all of the cahced information.

In particular, the sizes of each box must be cached because it generally requires a complete DOM traversal to determine them. Sizes always refer to the border edge of the boxes.




The offsetParent of a block-level element inside an inline element with position=relative is that inline element, even though it is as if the block level element "breaks out" of the inline element. This agrees with the proposed standard 'CSSOM View Module', and with Gecko and Opera, but disagrees with KHTML/WebKit and IE.

Position=relative on an inline element has effect on a contained block-level element even though it is as if the block level element "breaks out" of the inline element. This is in agreement with the CSS specification, and in agreement with Gecko and IE, but in disagreement with KHTML/WebKit and Opera.




Make a shadow hierarchy of boxes. When an element generates a principal box (display!=none) a reference to that box is stored in the element.

DOM elements do not own a referenced box object. It is the responsibility of the owner of the box object to clear the elements box reference before the box object is destroyed.

A box object has a render() method.

Various kinds of style changes on an element, causes various kinds of invalidation of the block hierarchy.

Any invalidation due to a change of an element shuould be handled either by calling a method on som block object, either the lements own block object, or some other block object by delegation to an immediate ancestor, immediate siblings, and/or an immediate descendants.

Invalidations must be registered in a way that makes the cost of repeated invalidations only marginally higher than for a single invalidation.


There are two kinds of changes that are interesting:

  1) structural changes (children)

  2) style changes

If the list of children is changed for an element, then the element receives a child-changed event.

A child-changed event is sent to an element, if its list of children has change, or a child has changed in a way that may affect the parent.


On a child-changed event, if an element has fixed sized (fixed height and no shrink-to-fit) and is the root of a block formatting context (no margin collapsing)


When an element is removed from its parent, all box references in the removed subtree are cleared.


If any style sheet has changed, the the entire box tree is invalidated.



Simplified first go:

An element has a generate_box() method that generates a single box object.

struct BlockContainerBox;

struch BlockContainerBox: BlockContainerBox;


generate_box()



Traverse DOM:


  Each block level element generates a block box


  struct Box
  {
    /// \patam w Width of containing block.
    void adjust_to_width(int w) {
      if (0 <= width) return;
      
    }

    // A negative value of 'width' means that 'left' and 'width' are not yet calculated.
    int left;   // Rightward displacement of left content edge relative to left content edge of parent box.
    int top;    // Downward displacement of top content edge relative to top content edge of parent box.
    int width;  // Distance between left and right content edges.
    int height; // Distance between top and bottom content edges.

    Box *parent; // Containing block
    Box *first_child;
    Box *next_sibling;
  }


  block_set = { inline, table-row-group, table-column, table-column-group, table-header-group, table-footer-group, table-row, table-cell, table-caption, inline-block };

  // FIXME: Canvas background: http://www.w3.org/TR/CSS2/colors.html#background
  // FIXME: Must inform caller whether bottom margin collapses?
  function gen(elem, x, y, avail_width, avail_height, adjoining_top_margin, bottom_margin_out, shrink_to_fit, is_root) {
    var display = elem.style.display;
    if (display == 'none') return null;
    var is_abs_pos = elem.style.position == 'absolute' || elem.style.position == 'fixed';
    var is_float = !is_abs_pos && elem.style.float != 'none';
    if (is_root || is_abs_pos || is_float) {
      if (display == 'inline-table') display = 'table';
      else if (display in block_set) display = 'block';
    }
    var is_block_level_box = display=='block' || display=='list_item' || display=='table';
    var is_block_container_box = display=='block' || display=='inline-block' || display=='list-item' || display=='table-cell' || display=='table-caption';
    var is_root_of_block_formatting_context = is_abs_pos || is_float || is_block_container_box && display!='block' || display=='inline-block' || display=='block' && elem.style.overflow!='visible';
    var shrink_to_fit = !defined elem.style.width && (shrink_to_fit || is_abs_pos || is_float || is_table_cell_in_some_cases);
    var top    = elem.style.borderTopWidth    + elem.style.paddingTop;
    var right  = elem.style.borderRightWidth  + elem.style.paddingRight;
    var bottom = elem.style.borderBottomWidth + elem.style.paddingBottom;
    var left   = elem.style.borderLeftWidth   + elem.style.paddingLeft;

    if (adjoining_top_margin && !is_root_of_block_formatting_context && top == 0) {
      if (adjoining_top_margin[0] < elem.style.marginTop) {
        adjoining_top_margin[0] = elem.style.marginTop;
      }
    }
    else {
      adjoining_top_margin = [ elem.style.marginTop ];
    }

    var child_y = top;
    var vert_child_margin = top==0 ? adjoining_top_margin : undefined;
    var max_width = elem.style.width // avail_width - left - right;
    var child_boxes = [];
    for (var child in elem.children) {
      StyleApplication child_style_appl(elem, style_appl);
      StyleImpl::EvaluatedStyle const *child_style = child_style_appl.get_style();
      var horiz_margin = child.style.marginLeft + child.style.marginRight
      var child_bottom_margin_out = [ undefined ];
      var box = gen(child, left, child_y, max_width - horiz_margin, hhhhhhhh, vert_child_margin, child_bottom_margin_out, shrink_to_fit);
      if (!box) continue;
      child_boxes.push(box);
      if (defined child_bottom_margin_out[0]) {
        child_y = box.top + box.width;
        vert_child_margin = child_bottom_margin_out;
      }
      if (shrink_to_fit) {
        var child_width = box.width + horiz_margin;
        max_child_width = child_width if max_child_width < child_width;
      }
    }

    var width = shrink_to_fit ? max_child_width : avail_contents_width;

    // Adjust horizontal placement of children
    for (var box in child_boxes) {
      box->adjust_to_width(width);
    }

    box = new BlockBox;
    box.children = child_boxes;
    box.width  = shrink_to_fit ? max_child_width : max_width;
    box.top    = y + adjoining_top_margin[0];
    if (elem.style.height == 'auto') {                                 // FIXME: Consider clearance
      if (!is_root_of_block_formatting_context && bottom == 0) {
        box.height = child_y; // FIXME: Unfinished business here - must take care of the case where top and bottom margins collapse
        bottom_margin_out[0] = vert_child_margin ? max(vert_child_margin[0], elem.style.marginBottom) : elem.style.marginBottom;
      }
      else {
        box.height = child_y + (vert_child_margin ? vert_child_margin[0] : 0) + bottom;
        bottom_margin_out[0] = elem.style.marginBottom;
      }
    }
    else {
      box.height = elem.style.height + top + bottom; // FIXME: Consider overflow and min-height and max-height
      bottom_margin_out[0] = elem.style.marginBottom;
    }
  }


  var shrink_to_fit = false;
  var is_root = true;
  var root_box = gen(root, 0, 0, viewport_width, viewport_height, shrink_to_fit, is_root);






  Specifically, the horizontal placement of children in the face of shrink-to-fit:

    The exact placement cannot be determined until all children have been processed.

    The exact widths of children can also not be determined until all children have been processed.



    For these reasons,





    var display = elem.style.display;
    if (display == 'none') return null;
    var is_abs_pos = elem.style.position == 'absolute' || elem.style.position == 'fixed';
    var is_float = !is_abs_pos && elem.style.float != 'none';
    if (is_root || is_abs_pos || is_float) {
      if (display == 'inline-table') display = 'table';
      else if (display in block_set) display = 'block';
    }
    var is_block_level_box = display=='block' || display=='list_item' || display=='table';

    


 */







  namespace DomImpl
  {
    struct NodeImpl;
    struct NodeListImpl;
    struct ElementImpl;
    struct DocumentImplBase;



    struct NullNodeList: virtual dom::NodeList
    {
      dom::Node *item(UIntMin32) throw() { return 0; }
      UIntMin32 getLength()      throw() { return 0; }
    };

    NullNodeList null_node_list;



    // Note that the fields are left uninitialized by the constructor.
    struct Sibling: virtual dom::Node
    {
      friend struct SimpleNodeList;
      friend struct ElementChildren;
      friend struct DocumentChildren;

      dom::Node *getPreviousSibling() throw () { return parent ? prev : 0; }
      dom::Node *getNextSibling()     throw () { return parent ? next : 0; }

      NodeImpl *get_parent() const throw () { return parent; }

    protected:
      // If this method has not been called since a specific point in
      // time, then the parent of this node has not changed since that
      // point in time. Overriding methods must call this one.
      virtual void on_parent_changed() throw () {}

    private:
      NodeImpl *parent;
      Sibling *prev, *next;
    };



    struct SimpleNodeList
    {
      template<class T> struct Forward { T operator()(T s) const { return s->next; } };
      typedef IncIter<Sibling const *, Forward<Sibling const *> > const_iterator;
      typedef IncIter<Sibling       *, Forward<Sibling       *> > iterator;
      const_iterator begin() const { return const_iterator(first); }
      const_iterator end()   const { return const_iterator(0);     }
      iterator       begin()       { return iterator(first);       }
      iterator       end()         { return iterator(0);           }

      Sibling *get_first() const { return first; }
      Sibling *get_last()  const { return last;  }

      // Ownership of the memory allocated to 'child' is passed from
      // the caller to the callee. 'child' must not already be owned
      // by any list. Returns the appended node.
      Sibling *append(Sibling *child, NodeImpl *parent) throw ()
      {
        child->parent = parent;
        child->prev   = last;
        child->next   = 0;
        last = (last ? last->next : first) = child;
        child->on_parent_changed();
        return child;
      }

      // 'anchor_child' must not be null, and it must be in this
      // list. Ownership of the memory allocated to 'child' is passed
      // from the caller to the callee. 'child' must not already be
      // owned by any list. Returns the inserted node.
      Sibling *insert_before(Sibling *child, Sibling *anchor_child, NodeImpl *parent) throw ()
      {
        child->parent = parent;
        child->prev   = anchor_child->prev;
        child->next   = anchor_child;
        anchor_child->prev = (anchor_child->prev ? anchor_child->prev->next : first) = child;
        child->on_parent_changed();
        return child;
      }

      // Ownership of the memory is passed from the callee to the
      // caller. Before the call, 'child' must be owned by this
      // list. The 'parent', 'next', and 'prev' fields are left in an
      // ill-defined state, and it must therefore be properly
      // reinitialized immediately upon return. Returns the removed
      // child.
      Sibling *remove(Sibling *child) throw ()
      {
        (child->prev ? child->prev->next : first) = child->next;
        (child->next ? child->next->prev : last)  = child->prev;
        return child;
      }

      // Ownership of memory of 'new_child' is passed from caller to
      // callee. Ownership of memory of 'old_child' is passed from
      // callee to caller. Before the call 'old_child' must be a child
      // of this element. The old child is returned.
      Sibling *replace(Sibling *new_child, Sibling *old_child) throw ()
      {
        new_child->parent = old_child->parent;
        new_child->prev   = old_child->prev;
        new_child->next   = old_child->next;
        (old_child->next ? old_child->next->prev : last) =
          (old_child->prev ? old_child->prev->next : first) = new_child;
        new_child->on_parent_changed();
        return old_child;
      }

      SimpleNodeList(): first(0), last(0) {}

      ~SimpleNodeList()
      {
        Sibling *s = first;
        while (s) {
          Sibling *const n = s->next;
          delete s;
          s = n;
        }
      }

    protected:
      Sibling *first, *last;
    };



    struct ElementChildren: virtual dom::NodeList, SimpleNodeList
    {
      dom::Node *item(UIntMin32 i) throw ()
      {
        // FIXME: The performance of this can be improved
        // heuristically by remembering the previous index and the
        // identified sibling. Then the difference in index is the
        // number of links to traverse, often just one. It is
        // necessary that this remebered result is cleared if the list
        // is modified in any way.
        Sibling *s = first;
        if (!s || i==0) return s;
        for (;;) {
          s = s->next;
          if (!s || --i == 0) return s;
        }
      }

      UIntMin32 getLength() throw () { return length; }

      bool empty() const { return length == 0; }

      Sibling *append(Sibling *child, NodeImpl *parent) throw ()
      {
        ++length;
        return SimpleNodeList::append(child, parent);
      }

      Sibling *insert_before(Sibling *child, Sibling *anchor_child, NodeImpl *parent) throw ()
      {
        ++length;
        return SimpleNodeList::insert_before(child, anchor_child, parent);
      }

      Sibling *remove(Sibling *child) throw ()
      {
        --length;
        return SimpleNodeList::remove(child);
      }

      ElementChildren(): length(0) {}

    private:
      UIntMin32 length;
    };



    struct DetachedNodes: private SimpleNodeList
    {
      // Ownership of the memory allocated to 'child' is passed from the
      // caller to the callee. 'child' must not already be owned by any
      // list. Returns the added node.
      Sibling *add(Sibling *child) throw ()
      {
        return SimpleNodeList::append(child,0);
      }

      // Ownership of the memory is passed from the callee to the
      // caller. Before the call, 'child' must be owned by this list. The
      // 'parent', 'next', and 'prev' fields are left in an
      // ill-defined state, and it must therefore be properly
      // reinitialized immediately upon return. REturns the removed
      // node.
      Sibling *remove(Sibling *child) throw ()
      {
        return SimpleNodeList::remove(child);
      }
    };



    struct NodeImpl: Sibling
    {
      dom::Node *getParentNode()     throw () { return get_parent();    }
      dom::NodeList *getChildNodes() throw () { return &null_node_list; }
      dom::Node *getFirstChild()     throw () { return 0;               }
      dom::Node *getLastChild()      throw () { return 0;               }

      dom::Document *getOwnerDocument() throw ();

      dom::Node *appendChild(dom::Node *) throw (dom::DOMException)
      {
        throw dom::DOMException("HIERARCHY_REQUEST_ERR");
      }

      dom::Node *replaceChild(Node *, Node *) throw (dom::DOMException)
      {
        throw dom::DOMException("HIERARCHY_REQUEST_ERR");
      }

      DocumentImplBase *get_document() const throw () { return document; }

      // It is assumed that this node is owned by some list. If this
      // node is currently attached to a parent, detach it. Otherwise
      // remove it from the list of detached nodes in the document.
      void release_from_owner() throw ();

    protected:
      // Must only be called if this node has a parent.
      virtual void detach_child(NodeImpl *child) throw () = 0;

      NodeImpl(DocumentImplBase *doc): document(doc) {}

    private:
      DocumentImplBase *const document;
    };



    struct DocumentImplBase: virtual dom::Document, NodeImpl
    {
      DocumentImplBase(): NodeImpl(0) {}

      DetachedNodes detached_nodes;
    };



    dom::Document *NodeImpl::getOwnerDocument() throw ()
    {
      return document;
    }

    void NodeImpl::release_from_owner() throw ()
    {
      NodeImpl *const parent = get_parent();
      if (parent) parent->detach_child(this);
      else document->detached_nodes.remove(this);
    }



    struct TextImpl: virtual dom::Text, NodeImpl
    {
      UIntMin16 getNodeType() throw () { return dom::Node::TEXT_NODE;      }

      void detach_child(NodeImpl *) throw () {} // Can have no children

      TextImpl(DocumentImplBase *doc, dom::DOMString const &str): NodeImpl(doc), data(str) {}

      dom::DOMString data;
    };



    struct ElementImpl: virtual dom::Element, NodeImpl
    {
      UIntMin16 getNodeType()        throw () { return dom::Node::ELEMENT_NODE; }
      dom::NodeList *getChildNodes() throw () { return &children;               }
      dom::Node *getFirstChild()     throw () { return children.get_first();    }
      dom::Node *getLastChild()      throw () { return children.get_last();     }

      dom::Node *appendChild(dom::Node *child) throw (dom::DOMException)
      {
        NodeImpl *const child2 = validate_new_child(child);
        child2->release_from_owner();
        return append_child(child2);
      }

      dom::Node *replaceChild(Node *new_child, Node *old_child) throw (dom::DOMException)
      {
        NodeImpl *const new_child2 = validate_new_child(new_child);
        NodeImpl *const old_child2 = validate_old_child(old_child);
        new_child2->release_from_owner();
        return replace_child(new_child2, old_child2);
      }

      // Verify that the specified node would be a valid new child of
      // this element.
      NodeImpl *validate_new_child(dom::Node *child) const
      {
        NodeImpl *const child2 = dynamic_cast<NodeImpl *>(child);

        if (!child2 || child2->get_document() != get_document())
          throw dom::DOMException("WRONG_DOCUMENT_ERR");

        // Prevent creation of a cycle
        if (ElementImpl *const elem = dynamic_cast<ElementImpl *>(child2)) {
          // Check if child is among ancestors of this element
          ElementImpl const *ancest = this;
          if (elem == ancest) {
          hierarchy_error:
            throw dom::DOMException("HIERARCHY_REQUEST_ERR");
          }
          if (!elem->children.empty()) {
            for (;;) {
              {
                NodeImpl const *const p = ancest->get_parent();
                if (p == get_document()) break;
                ancest = static_cast<ElementImpl const *>(p);
              }
              if (elem == ancest) goto hierarchy_error;
            }
          }
        }

        return child2;
      }

      // Verify that the specified node is currently a child of this
      // element.
      NodeImpl *validate_old_child(dom::Node *child) const
      {
        NodeImpl *const child2 = dynamic_cast<NodeImpl *>(child);
        if (child2 && child2->get_parent() == this) return child2;
        throw dom::DOMException("NOT_FOUND_ERR");
      }

      // Ownership of memory is passed from caller to callee, so
      // child->release_from_owner() must be called first unless the
      // child was not owned by any list to begin with. Returns the
      // appended child. Nothing is assumed about the state of the
      // 'parent', 'prev', and 'next' fields of the passed sibling.
      NodeImpl *append_child(NodeImpl *child) throw ()
      {
        children.append(child, this);
        on_children_changed();
        return child;
      }

      // Ownership of memory of 'new_child' is passed from caller to
      // callee, so child->release_from_owner() must be called first
      // unless the child was not owned by any list to begin
      // with. Ownership of memory of 'old_child' remains with the
      // callee. Before the call 'old_child' must be a child of this
      // element. The old child is returned.
      NodeImpl *replace_child(NodeImpl *new_child, NodeImpl *old_child) throw ()
      {
        get_document()->detached_nodes.add(children.replace(new_child, old_child));
        on_children_changed();
        return old_child;
      }

      void detach_child(NodeImpl *child) throw ()
      {
        children.remove(child);
        on_children_changed();
      }

      ElementImpl(DocumentImplBase *doc): NodeImpl(doc) {}

    protected:
      // If this method has not been called since a specific point in
      // time, then the children of this element have not changed
      // since that point in time. Overriding methods must call this
      // one.
      virtual void on_children_changed() throw () {}

    private:
      typedef ElementChildren Children;
      Children children;

    public:
      typedef Children::const_iterator const_child_iterator;
      typedef Children::iterator       child_iterator;
      const_child_iterator children_begin() const { return children.begin(); }
      const_child_iterator children_end()   const { return children.end();   }
      child_iterator       children_begin()       { return children.begin(); }
      child_iterator       children_end()         { return children.end();   }
    };



    struct DocumentChildren: virtual dom::NodeList
    {
      dom::Node *item(UIntMin32 i) throw ()
      {
        return i == 0 ? get_first() : 0;
      }

      UIntMin32 getLength() throw () { return root_elem ? 1 : 0; }

      Sibling *get_first() const { return root_elem ? root_elem.get() : 0; }
      Sibling *get_last()  const { return get_first();                     }

      // Ownership of the element remains with the callee.
      ElementImpl *get_root_elem() const throw ()
      {
        return root_elem.get();
      }

      // Ownership of the added element is passed from the caller to
      // the callee. Before the call, get_root_elem() must return
      // null. The added element is returned.
      ElementImpl *add_root_elem(ElementImpl *elem, NodeImpl *parent) throw ()
      {
        elem->parent = parent;
        elem->prev = elem->next = 0;
        root_elem.reset(elem);
        elem->on_parent_changed();
        return elem;
      }

      // Ownership of the removed element is passed from the callee to
      // the caller. Returns the removed element.
      ElementImpl *remove_root_elem() throw ()
      {
        return root_elem.release();
      }

    private:
      UniquePtr<ElementImpl> root_elem;
    };



    struct DocumentImpl: DocumentImplBase
    {
      UIntMin16 getNodeType()            throw () { return dom::Node::DOCUMENT_NODE; }
      dom::NodeList *getChildNodes()     throw () { return &children;                }
      dom::Node *getFirstChild()         throw () { return children.get_first();     }
      dom::Node *getLastChild()          throw () { return children.get_last();      }
      dom::Element *getDocumentElement() throw () { return children.get_root_elem(); }

      dom::Element *createElement(dom::DOMString const &tag_name) throw(dom::DOMException)
      {
        ElementImpl *const elem = create_element(tag_name);
        if (!elem) throw dom::DOMException("INVALID_CHARACTER_ERR");
        detached_nodes.add(elem);
        return elem;
      }

      dom::Text *createTextNode(dom::DOMString const &str) throw ()
      {
        TextImpl *const text = new TextImpl(this, str);
        detached_nodes.add(text);
        return text;
      }

      dom::Node *appendChild(dom::Node *child) throw (dom::DOMException)
      {
        NodeImpl *const child2 = dynamic_cast<NodeImpl *>(child);

        if (!child2 || child2->get_document() != this)
          throw dom::DOMException("WRONG_DOCUMENT_ERR");

        ElementImpl *const elem = dynamic_cast<ElementImpl *>(child2);
        if (!elem || children.get_root_elem()) throw dom::DOMException("HIERARCHY_REQUEST_ERR");

        elem->release_from_owner();
        return add_root_elem(elem);
      }

      ElementImpl *get_root_elem() const throw () { return children.get_root_elem(); }

      // Ownership of memory is passed from caller to callee, so
      // elem->release_from_owner() must be called first unless the
      // element has no owner to begin with. Before the call,
      // get_root_elem() must return null. The added element is
      // returned.
      ElementImpl *add_root_elem(ElementImpl *elem) throw ()
      {
        children.add_root_elem(elem, this);
        on_root_elem_changed();
        return elem;
      }

      // Ownership of the removed element is passed from the callee to
      // the caller. Returns the removed element.
      ElementImpl *remove_root_elem() throw ()
      {
        return children.remove_root_elem();
      }

      void detach_child(NodeImpl *child) throw ()
      {
        if (ElementImpl *const elem = dynamic_cast<ElementImpl *>(child)) {
          if (elem == children.get_root_elem()) {
            children.remove_root_elem();
            on_root_elem_changed();
            return;
          }
        }
        throw runtime_error("Unexpected absence of child to be removed");
      }

    protected:
      // Returns null if the tag name is invalid. Otherwise ownership
      // of the returned memory is passed from the callee to the
      // caller.
      virtual ElementImpl *create_element(dom::DOMString const &tag_name) = 0;

      // If this method has not been called since a specific point in
      // time, then the root element of this document have not changed
      // since that point in time. Overriding methods must call this
      // one.
      virtual void on_root_elem_changed() throw () {}

    private:
      DocumentChildren children;
    };
  }





  namespace StyleImpl
  {
    struct Element;
    struct ManipContext;


    enum Priority { priority_Normal, priority_Important };



    enum ValueType
    {
      value_Unspecified,
      value_Inherit,
      value_Auto,
      value_Transparent, // Deprecated in CSS3
      value_RGB_Number, value_RGB_Percent,
      value_HSL_Number, value_HSL_Percent,
      _value_End     ///< This one is just a marker
    };



    enum LengthUnit
    {
      lengthUnit_None, lengthUnit_Percent, lengthUnit_EM, lengthUnit_EX, lengthUnit_PX,
      lengthUnit_CM, lengthUnit_MM, lengthUnit_IN, lengthUnit_PT, lengthUnit_PC,
      _lengthUnit_End     ///< This one is just a marker
    };



    enum NamedBorderWidth
    {
      borderWidth_Thin, borderWidth_Medium, borderWidth_Thick,
      _borderWidth_End     ///< This one is just a marker
    };



    enum BorderStyle
    {
      borderStyle_None, borderStyle_Hidden, borderStyle_Dotted, borderStyle_Dashed,
      borderStyle_Solid, borderStyle_Double, borderStyle_Groove, borderStyle_Ridge,
      borderStyle_Inset, borderStyle_Outset
    };



    enum FontStyle
    {
      fontStyle_Normal, fontStyle_Italic, fontStyle_Oblique
    };



    enum FontVariant
    {
      fontVariant_Normal, fontVariant_SmallCaps
    };



    enum FontWeight
    {
      fontWeight_100, fontWeight_200, fontWeight_300, fontWeight_400, fontWeight_500,
      fontWeight_600, fontWeight_700, fontWeight_800, fontWeight_900
    };

    enum SpecialFontWeight
    {
      specialFontWeight_Normal, specialFontWeight_Bold,
      specialFontWeight_Bolder, specialFontWeight_Lighter,
      _specialFontWeight_End     ///< This one is just a marker
    };



    enum NamedFontSize
    {
      fontSize_XXSmall, fontSize_XSmall, fontSize_Small, fontSize_Medium,
      fontSize_Large, fontSize_XLarge, fontSize_XXLarge,
      fontSize_Larger, fontSize_Smaller,
      _fontSize_End     ///< This one is just a marker
    };



    enum SystemColor
    {
      sysColor_ActiveBorder, sysColor_ActiveCaption, sysColor_AppWorkspace,
      sysColor_Background, sysColor_ButtonFace, sysColor_ButtonHighlight,
      sysColor_ButtonShadow, sysColor_ButtonText, sysColor_CaptionText,
      sysColor_GrayText, sysColor_Highlight, sysColor_HighlightText,
      sysColor_InactiveBorder, sysColor_InactiveCaption, sysColor_InactiveCaptionText,
      sysColor_InfoBackground, sysColor_InfoText, sysColor_Menu, sysColor_MenuText,
      sysColor_Scrollbar, sysColor_ThreeDDarkShadow, sysColor_ThreeDFace,
      sysColor_ThreeDHighlight, sysColor_ThreeDLightShadow, sysColor_ThreeDShadow,
      sysColor_Window, sysColor_WindowFrame, sysColor_WindowText,
      _sysColor_End     ///< This one is just a marker
    };



    // FIXME: Document exactly how evaluated style is different from
    // computed style.
    struct EvaluatedStyle
    {
      // Properties are marked dirty and reported to the rendering
      // application in groups. The 'font' group must always be
      // applied first, such that properties in the other groups can
      // refer reliably to the current font size, as well as to the
      // current height of 'x'. The 'font' group consists precisely of
      // 'font-style', 'font-variant', 'font-weight', 'font-size', and
      // 'font-family'.
      // FIXME: Move this enum and GroupBits to Applyee.
      enum GroupId
      {
        group_Font, group_Text, group_Background, group_Border,
        group_Margin, group_Padding, group_Size
      };

      typedef unsigned long GroupBits;

      typedef double Length;
      struct AugmentedLength
      {
        enum State { state_Auto, state_Abs, state_Rel } state;
        Length value;
        AugmentedLength() {}
        AugmentedLength(State s, Length v): state(s), value(v) {}
        bool operator==(AugmentedLength const &l) const
        {
          switch (state) {
          default:         return l.state == state_Auto;
          case state_Abs:  return l.state == state_Abs && value == l.value;
          case state_Rel:  return l.state == state_Rel && value == l.value;
          }
        }
        bool operator!=(AugmentedLength const &l) const { return !operator==(l); }
      };
      typedef Vec4F Color;

      struct Font
      {
        FontStyle   style;
        FontVariant variant;
        FontWeight  weight;
        Length      size;
        void init()
        {
          style   = fontStyle_Normal;
          variant = fontVariant_Normal;
          weight  = fontWeight_400;
          size    = 0;
        }
      } font;

      struct Text
      {
        Color color;
        AugmentedLength line_height;
        void init()
        {
          color.set(1,1,1,1);
          line_height.state = AugmentedLength::state_Auto; // 'normal'
        }
      } text;

      struct Background
      {
        Color color;
        void init() { color.set(0,0,0,0); }
      } background;

      struct Border
      {
        struct Side
        {
          Length width; BorderStyle style; Color color; bool color_specified;
          void init() { width = 0; style = borderStyle_None; color_specified = false; }
        } top, right, bottom, left;
        void init() { top.init(); right.init(); bottom.init(); left.init(); }
        void set_width(Length w) { top.width = right.width = bottom.width = left.width = w; }
      } border;

      struct Margin
      {
        AugmentedLength top, right, bottom, left;
        void init()
        {
          top.state = right.state = bottom.state = left.state = AugmentedLength::state_Abs;
          top.value = right.value = bottom.value = left.value = 0;
        }
      } margin;

      struct Padding
      {
        AugmentedLength top, right, bottom, left;
        void init()
        {
          top.state = right.state = bottom.state = left.state = AugmentedLength::state_Abs;
          top.value = right.value = bottom.value = left.value = 0;
        }
      } padding;

      struct Size
      {
        AugmentedLength width, height;
        void init()
        {
          width.state = height.state = AugmentedLength::state_Auto;
        }
      } size;

      void init()
      {
        font.init();
        text.init();
        background.init();
        border.init();
        margin.init();
        padding.init();
        size.init();
      }
    };



    /**
     * Each distinct short or long hand CSS property has its
     * definition represented by a unique instance of this class.
     */
    struct PropDef
    {
      virtual dom::DOMString get(Element const &) const = 0;
      virtual void set(dom::DOMString const &value, Element &) const = 0;

      virtual ~PropDef() {}
    };



    struct StaticInfo
    {
      PropDef const *lookup_prop_def(string const &name) const
      {
        PropMap::const_iterator const i = prop_map.find(name);
        if (i == prop_map.end()) return 0;
        return i->second;
      }

      StaticInfo() { add_props(); }

    private:
      void add_props();

      template<class Prop, class Group> PropDef &add(Prop Group::*prop);

      template<class ForceGroup, class Prop, class Group> PropDef &add2(Prop Group::*prop);

      template<class P> P &add(string name, P *prop) { add2(name, prop); return *prop; }

      PropDef &add2(string name, PropDef *prop)
      {
        UniquePtr<PropDef> p(prop);
        prop_map.set_at(name, p);
        return *prop;
      }

      typedef DeletingMap<string, PropDef const> PropMap;
      PropMap prop_map;
    };



    struct ManipContext
    {
      StaticInfo const &get_static_info() const throw () { return static_info; }

      PackedTRGB::CssLevel get_css_level() const throw () { return css_level; }

      ctype<char> const &get_ctype_narrow() const throw () { return ctype_narrow; }

      locale const &get_locale() const throw () { return loc; }

      wostringstream &get_format_stream() throw ()
      {
        format_stream.str(wstring());
        return format_stream;
      }

      PropDef const *lookup_prop_def(dom::DOMString const &name) const
      {
        string name2;
        if (string_codec.decode_narrow(name, name2)) {
          PropDef const *const prop = static_info.lookup_prop_def(name2);
          if (prop) return prop;
        }
        return 0;
      }

      string toupper(string const &str) const { return Text::toupper(str, loc); }

      wstring widen(string const &str) const { return Text::widen<wchar_t>(str, loc); }

      dom::DOMString encode_narrow(string str)
      {
        dom::DOMString str2;
        string_codec.encode_narrow(str, str2);
        return str2;
      }

      dom::DOMString encode_wide(wstring str)
      {
        dom::DOMString str2;
        if (string_codec.encode(str, str2)) return str2;
        throw runtime_error("DOM string encoding failed"); // FIXME: How to handle this?
      }

      template<class Prop> dom::DOMString format_prop(Prop const &prop)
      {
        format_stream.str(wstring());
        prop.format_value(format_stream, *this);
        return encode_wide(format_stream.str());
      }

      template<class Prop> void parse_narrow_prop(dom::DOMString const &str, Prop &prop)
      {
        string str2;
        if (decode_narrow(str, str2)) {
          string::const_iterator i = str2.begin(), j = str2.end();
          for (;;) {
            if (i == j) {
              prop.parse_value(string(), *this); // Make it unspecified
              return;
            }
            if (!ctype_narrow.is(ctype_base::space, *i)) break;
            ++i;
          }
          for (;;) {
            string::const_iterator const k = j-1;
            if (!ctype_narrow.is(ctype_base::space, *k)) break;
            j = k;
          }
          if (prop.parse_value(string(i,j), *this)) return;
        }
        throw dom::DOMException("SYNTAX_ERR");
      }

      template<class Prop> void parse_wide_prop(dom::DOMString const &str, Prop &prop)
      {
        wstring str2;
        if (decode_wide(str, str2)) {
          wstring::const_iterator i = str2.begin(), j = str2.end();
          for (;;) {
            if (i == j) {
              prop.parse_value(wstring(), *this); // Make it unspecified
              return;
            }
            if (!ctype_wide.is(ctype_base::space, *i)) break;
            ++i;
          }
          for (;;) {
            wstring::const_iterator const k = j-1;
            if (!ctype_wide.is(ctype_base::space, *k)) break;
            j = k;
          }
          if (prop.parse_value(wstring(i,j), *this)) return;
        }
        throw dom::DOMException("SYNTAX_ERR");
      }

      bool decode_narrow(dom::DOMString const &str, string &str2) const
      {
        return string_codec.decode_narrow(str, str2);
      }

      bool decode_wide(dom::DOMString const &str, wstring &str2) const
      {
        return string_codec.decode(str, str2);
      }

      wstring decode_lenient(dom::DOMString const &str) const
      {
        return string_codec.decode(str, L'\uFFFD');
      }

      Priority parse_priority(dom::DOMString const &prio) const
      {
        string prio2;
        if (decode_narrow(prio, prio2)) {
          if (prio2.empty())        return priority_Normal;
          if (prio2 == "important") return priority_Important;
        }
        throw dom::DOMException("SYNTAX_ERR");
      }

      // FIXME: Do not allow numbers with no digits following the
      // decimal point.
      template<class T> bool parse_length(string str, T &value, string &unit)
      {
        T v;
        string u;
	parse_stream.clear();
        parse_stream.str(str);
        parse_stream >> v >> noskipws >> u;
        if (parse_stream.fail() || parse_stream.bad() ||
            parse_stream.get() != char_traits<char>::eof()) return false;
        value = v;
        unit = u;
        return true;
      }

      void deprecation_warning(string msg)
      {
        cerr << "WARNING: " << msg << endl;
      }

      ManipContext(StaticInfo const &i, locale const &l, PackedTRGB::CssLevel v):
        static_info(i), loc(l), css_level(v), ctype_narrow(use_facet<ctype<char> >(loc)),
        ctype_wide(use_facet<ctype<wchar_t> >(loc)), string_codec(l)
      {
        locale const loc2(locale::classic(), l, locale::ctype);
        format_stream.imbue(loc2);
        parse_stream.imbue(loc2);
      }

    private:
      StaticInfo const &static_info;
      locale const loc;
      PackedTRGB::CssLevel const css_level;
      ctype<char> const &ctype_narrow;
      ctype<wchar_t> const &ctype_wide;
      CharEnc<CharUtf16> const string_codec;
      wostringstream format_stream;
      istringstream parse_stream;
    };



    struct Document: ManipContext, DomImpl::DocumentImpl
    {
      double get_dpcm() const { return dpcm; };

      EvaluatedStyle const &get_default_style()
      {
        if (!default_style) {
          default_style.reset(new EvaluatedStyle);
          default_style->init();
          default_style->border.set_width(get_std_border_width(borderWidth_Medium));
          default_style->font.size = get_std_font_size(0);
        }
        return *default_style;
      }

      double get_std_border_width(NamedBorderWidth w)
      {
        switch (w) {
        case borderWidth_Thin:  return 1;
        default:                return 3;
        case borderWidth_Thick: return 5;
        }
      }

      double get_std_font_size(int i)
      {
        return 18 * pow(get_font_size_scale_factor(), i);
      }

      double increase_font_size(double s)
      {
        return get_font_size_scale_factor() * s;
      }

      double decrease_font_size(double s)
      {
        return (1/get_font_size_scale_factor()) * s;
      }

      Document(StaticInfo const &i, locale const &l, double d, PackedTRGB::CssLevel v):
        ManipContext(i,l,v), dpcm(d) {}

    private:
      double const dpcm;
      UniquePtr<EvaluatedStyle> default_style;

      static double get_font_size_scale_factor() { return 7.0/6; }
    };



    struct ComputeContext
    {
      Document &get_document() throw () { return document; }

      EvaluatedStyle &get_current_style() { return *current_style; }

      virtual double determine_current_height_of_x() = 0;

      // FIXME: Provide an optimized allocator of EvaluatedStyle. One may already be available in core/memory.hpp in code that is commented out.
      ComputeContext(Document &d):
        document(d), current_style(new EvaluatedStyle(d.get_default_style())) {}

      virtual ~ComputeContext() {}

    private:
      Document &document;
      UniquePtr<EvaluatedStyle> const current_style;
    };



    // The purpose of this class is to inform the application whenever
    // non-default style aplies.
    struct Applyee
    {
      double get_width_of_containing_block()
      {
        return 256; // FIXME: FIX THIS IMMEDIATELY!!!! FIX THIS IMMEDIATELY!!!! FIX THIS IMMEDIATELY!!!! FIX THIS IMMEDIATELY!!!! FIX THIS IMMEDIATELY!!!!
      }

      double get_current_font_size()
      {
        return current_font_size;
      }

      double get_current_height_of_x()
      {
        if (!has_current_height_of_x) {
          current_height_of_x = context.determine_current_height_of_x();
          has_current_height_of_x = true;
        }
        return current_height_of_x;
      }

      double get_dpcm()
      {
        return document.get_dpcm();
      }

      void get_system_color(SystemColor, EvaluatedStyle::Color &color)
      {
        color.set(0.5, 0.5, 0.5, 1); // FIXME: Implement this!
      }

      double get_std_border_width(NamedBorderWidth w)
      {
        return document.get_std_border_width(w);
      }

      double get_std_font_size(int i)
      {
        return document.get_std_font_size(i);
      }

      double increase_font_size(double s)
      {
        return document.increase_font_size(s);
      }

      double decrease_font_size(double s)
      {
        return document.decrease_font_size(s);
      }

      ComputeContext &get_context() const throw () { return context; }


      // Determine the value of the specified property that applies to
      // the parent element. For the root element, the default value
      // is returned.
      template<class PropSpec> typename PropSpec::value_get_type get_from_parent()
      {
        typename EvaluatedStyle::GroupId const group = PropSpec::eval_group;
        typename EvaluatedStyle::GroupBits const group_bit = 1ul<<group;
        EvaluatedStyle const &origin =
          PropSpec::is_default_inherited || parent && (parent->dirty & group_bit) ?
          (dirty & group_bit ? *backup_style : context.get_current_style()) :
          document.get_default_style();
        return PropSpec::get_value(origin);
      }


      template<class PropSpec> void inherit()
      {
        set<PropSpec>(get_from_parent<PropSpec>());
      }


      template<class PropSpec> void set(typename PropSpec::value_type const &value)
      {
        EvaluatedStyle &style = context.get_current_style();
        typename PropSpec::value_type &prop = PropSpec::get_access(style);
        bool const default_inherited = PropSpec::is_default_inherited;
        if (default_inherited && prop == value) return;
        typename EvaluatedStyle::GroupId const group = PropSpec::eval_group;
        typename EvaluatedStyle::GroupBits const group_bit = 1ul<<group;
        if (!(dirty & group_bit)) {
          backup_group<PropSpec>();
          dirty |= group_bit;
        }
        prop = value;
        PropSpec::on_value_specified(style);
      }


      void flush_font()
      {
        if (font_flushed) throw runtime_error("Repeated flush of font");
        if (other_flushed) throw runtime_error("Flush of font after flush of other");
        EvaluatedStyle const &current = context.get_current_style();
        if (dirty & 1ul<<EvaluatedStyle::group_Font)
        {
          set_font(current.font);
          current_font_size = current.font.size;
          has_current_height_of_x = false;
        }
        font_flushed = true;
      }


      void flush_other()
      {
        if (!font_flushed) throw runtime_error("Flush of other without flush of font");
        if (other_flushed) throw runtime_error("Repeated flush of other");
        EvaluatedStyle const &current = context.get_current_style();
        if (dirty & 1ul<<EvaluatedStyle::group_Text)       set_text(current.text);
        if (dirty & 1ul<<EvaluatedStyle::group_Background) set_background(current.background);
        if (dirty & 1ul<<EvaluatedStyle::group_Border)     set_border(current.border);
        if (dirty & 1ul<<EvaluatedStyle::group_Margin)     set_margin(current.margin);
        if (dirty & 1ul<<EvaluatedStyle::group_Padding)    set_padding(current.padding);
        if (dirty & 1ul<<EvaluatedStyle::group_Size)       set_size(current.size);
        other_flushed = true;
      }


      void revert()
      {
        if (!other_flushed) throw runtime_error("Revert without flush");

        // Only inform applyee when reverting properties that are default inherited
        EvaluatedStyle &current = context.get_current_style();
        if (dirty & 1ul<<EvaluatedStyle::group_Font) {
          current.font = backup_style->font;
          set_font(current.font);
        }
        if (dirty & 1ul<<EvaluatedStyle::group_Text) {
          current.text = backup_style->text;
          set_text(current.text);
        }
        if (dirty & 1ul<<EvaluatedStyle::group_Background) {
          current.background = backup_style->background;
        }
        if (dirty & 1ul<<EvaluatedStyle::group_Border) {
          current.border = backup_style->border;
        }
        if (dirty & 1ul<<EvaluatedStyle::group_Margin) {
          current.margin = backup_style->margin;
        }
        if (dirty & 1ul<<EvaluatedStyle::group_Padding) {
          current.padding = backup_style->padding;
        }
        if (dirty & 1ul<<EvaluatedStyle::group_Size) {
          current.size = backup_style->size;
        }
      }


      Applyee(ComputeContext &c, Applyee const *p):
        document(c.get_document()), context(c), parent(p), dirty(0),
        font_flushed(false), other_flushed(false), has_current_height_of_x(false),
        current_font_size(c.get_current_style().font.size) {}

      virtual ~Applyee() {}


    protected:
      virtual void set_font(EvaluatedStyle::Font const &) = 0;
      virtual void set_text(EvaluatedStyle::Text const &) = 0;
      virtual void set_background(EvaluatedStyle::Background const &) = 0;
      virtual void set_border(EvaluatedStyle::Border const &) = 0;
      virtual void set_margin(EvaluatedStyle::Margin const &) = 0;
      virtual void set_padding(EvaluatedStyle::Padding const &) = 0;
      virtual void set_size(EvaluatedStyle::Size const &) = 0;


    private:
      template<class PropSpec> void backup_group()
      {
        if (!backup_style) backup_style.reset(new EvaluatedStyle);
        EvaluatedStyle &current = context.get_current_style();
        bool const set_to_default = !PropSpec::is_default_inherited;
        switch(PropSpec::eval_group) {
        case EvaluatedStyle::group_Font:
          backup_style->font = current.font;
          if (set_to_default) current.font = document.get_default_style().font;
          break;
        case EvaluatedStyle::group_Text:
          backup_style->text = current.text;
          if (set_to_default) current.text = document.get_default_style().text;
          break;
        case EvaluatedStyle::group_Background:
          backup_style->background = current.background;
          if (set_to_default) current.background = document.get_default_style().background;
          break;
        case EvaluatedStyle::group_Border:
          backup_style->border = current.border;
          if (set_to_default) current.border = document.get_default_style().border;
          break;
        case EvaluatedStyle::group_Margin:
          backup_style->margin = current.margin;
          if (set_to_default) current.margin = document.get_default_style().margin;
          break;
        case EvaluatedStyle::group_Padding:
          backup_style->padding = current.padding;
          if (set_to_default) current.padding = document.get_default_style().padding;
          break;
        case EvaluatedStyle::group_Size:
          backup_style->size = current.size;
          if (set_to_default) current.size = document.get_default_style().size;
          break;
        }
      }


      Document &document;
      ComputeContext &context;
      Applyee const *const parent;
      EvaluatedStyle::GroupBits dirty;
      UniquePtr<EvaluatedStyle> backup_style;
      bool font_flushed, other_flushed;
      bool has_current_height_of_x;
      double current_height_of_x;

      // This is to hold its value fixed while the font style is applied.
      double current_font_size;
    };



    // Defininition of endowed enumerations


    struct EmptyEnumSpec { static EnumAssoc map[]; };
    EnumAssoc EmptyEnumSpec::map[] = { { 0, 0 } };
    enum EmptyBaseEnum {};
    typedef Enum<EmptyBaseEnum, EmptyEnumSpec> EmptyEnum;


    struct LengthUnitSpec { static EnumAssoc map[]; };
    EnumAssoc LengthUnitSpec::map[] =
    {
      { lengthUnit_None,    ""   },
      { lengthUnit_Percent, "%"  },
      { lengthUnit_EM,      "em" },
      { lengthUnit_EX,      "ex" },
      { lengthUnit_PX,      "px" },
      { lengthUnit_CM,      "cm" },
      { lengthUnit_MM,      "mm" },
      { lengthUnit_IN,      "in" },
      { lengthUnit_PT,      "pt" },
      { lengthUnit_PC,      "pc" },
      { 0, 0 }
    };
    typedef Enum<LengthUnit, LengthUnitSpec> LengthUnitEnum;



    struct NamedBorderWidthSpec { static EnumAssoc map[]; };
    EnumAssoc NamedBorderWidthSpec::map[] =
    {
      { borderWidth_Thin,   "thin"   },
      { borderWidth_Medium, "medium" },
      { borderWidth_Thick,  "thick"  },
      { 0, 0 }
    };
    typedef Enum<NamedBorderWidth, NamedBorderWidthSpec> NamedBorderWidthEnum;



    struct BorderStyleSpec { static EnumAssoc map[]; };
    EnumAssoc BorderStyleSpec::map[] =
    {
      { borderStyle_None,   "none"   },
      { borderStyle_Hidden, "hidden" },
      { borderStyle_Dotted, "dotted" },
      { borderStyle_Dashed, "dashed" },
      { borderStyle_Solid,  "solid"  },
      { borderStyle_Double, "double" },
      { borderStyle_Groove, "groove" },
      { borderStyle_Ridge,  "ridge"  },
      { borderStyle_Inset,  "inset"  },
      { borderStyle_Outset, "outset" },
      { 0, 0 }
    };
    typedef Enum<BorderStyle, BorderStyleSpec> BorderStyleEnum;



    struct FontStyleSpec { static EnumAssoc map[]; };
    EnumAssoc FontStyleSpec::map[] =
    {
      { fontStyle_Normal,  "normal"  },
      { fontStyle_Italic,  "italic"  },
      { fontStyle_Oblique, "oblique" },
      { 0, 0 }
    };
    typedef Enum<FontStyle, FontStyleSpec> FontStyleEnum;



    struct FontVariantSpec { static EnumAssoc map[]; };
    EnumAssoc FontVariantSpec::map[] =
    {
      { fontVariant_Normal,    "normal"     },
      { fontVariant_SmallCaps, "small-caps" },
      { 0, 0 }
    };
    typedef Enum<FontVariant, FontVariantSpec> FontVariantEnum;



    struct FontWeightSpec { static EnumAssoc map[]; };
    EnumAssoc FontWeightSpec::map[] =
    {
      { fontWeight_100,     "100"     },
      { fontWeight_200,     "200"     },
      { fontWeight_300,     "300"     },
      { fontWeight_400,     "400"     }, // Normal
      { fontWeight_500,     "500"     },
      { fontWeight_600,     "600"     },
      { fontWeight_700,     "700"     }, // Bold
      { fontWeight_800,     "800"     },
      { fontWeight_900,     "900"     },
      { 0, 0 }
    };
    typedef Enum<FontWeight, FontWeightSpec> FontWeightEnum;

    struct SpecialFontWeightSpec { static EnumAssoc map[]; };
    EnumAssoc SpecialFontWeightSpec::map[] =
    {
      { specialFontWeight_Normal,  "normal"  },
      { specialFontWeight_Bold,    "bold"    },
      { specialFontWeight_Bolder,  "bolder"  },
      { specialFontWeight_Lighter, "lighter" },
      { 0, 0 }
    };
    typedef Enum<SpecialFontWeight, SpecialFontWeightSpec> SpecialFontWeightEnum;



    struct NamedFontSizeSpec { static EnumAssoc map[]; };
    EnumAssoc NamedFontSizeSpec::map[] =
    {
      { fontSize_XXSmall, "xx-small" },
      { fontSize_XSmall,  "x-small"  },
      { fontSize_Small,   "small"    },
      { fontSize_Medium,  "medium"   },
      { fontSize_Large,   "large"    },
      { fontSize_XLarge,  "x-large"  },
      { fontSize_XXLarge, "xx-large" },
      { fontSize_Larger,  "larger"   },
      { fontSize_Smaller, "smaller"  },
      { 0, 0 }
    };
    typedef Enum<NamedFontSize, NamedFontSizeSpec> NamedFontSizeEnum;



    struct SystemColorSpec { static EnumAssoc map[]; };
    EnumAssoc SystemColorSpec::map[] =
    {
      { sysColor_ActiveBorder,        "ActiveBorder"        },
      { sysColor_ActiveCaption,       "ActiveCaption"       },
      { sysColor_AppWorkspace,        "AppWorkspace"        },
      { sysColor_Background,          "Background"          },
      { sysColor_ButtonFace,          "ButtonFace"          },
      { sysColor_ButtonHighlight,     "ButtonHighlight"     },
      { sysColor_ButtonShadow,        "ButtonShadow"        },
      { sysColor_ButtonText,          "ButtonText"          },
      { sysColor_CaptionText,         "CaptionText"         },
      { sysColor_GrayText,            "GrayText"            },
      { sysColor_Highlight,           "Highlight"           },
      { sysColor_HighlightText,       "HighlightText"       },
      { sysColor_InactiveBorder,      "InactiveBorder"      },
      { sysColor_InactiveCaption,     "InactiveCaption"     },
      { sysColor_InactiveCaptionText, "InactiveCaptionText" },
      { sysColor_InfoBackground,      "InfoBackground"      },
      { sysColor_InfoText,            "InfoText"            },
      { sysColor_Menu,                "Menu"                },
      { sysColor_MenuText,            "MenuText"            },
      { sysColor_Scrollbar,           "Scrollbar"           },
      { sysColor_ThreeDDarkShadow,    "ThreeDDarkShadow"    },
      { sysColor_ThreeDFace,          "ThreeDFace"          },
      { sysColor_ThreeDHighlight,     "ThreeDHighlight"     },
      { sysColor_ThreeDLightShadow,   "ThreeDLightShadow"   },
      { sysColor_ThreeDShadow,        "ThreeDShadow"        },
      { sysColor_Window,              "Window"              },
      { sysColor_WindowFrame,         "WindowFrame"         },
      { sysColor_WindowText,          "WindowText"          },
      { 0, 0 }
    };
    typedef Enum<SystemColor, SystemColorSpec> SystemColorEnum;



    struct PropBase
    {
      PropBase(): value_type(value_Unspecified) {}

    private:
      typedef int PropBase::*unspecified_bool_type;

    public:
      operator unspecified_bool_type() const throw()
      {
        return value_type != value_Unspecified ? &PropBase::value_type : 0;
      }

    protected:
      int value_type;
    };



    // The length unit with index I is represented as _value_End + I.
    // The named length with index I is represented as _value_End +
    // _lengthUnit_End + I.
    template<bool allow_bare_numbers, bool allow_percentages,
             bool allow_negative_values, bool has_keyword_auto,
             bool normal_instead_of_auto, class Names>
    struct LengthPropBase: PropBase
    {
      void format_value(wostream &out, ManipContext &) const
      {
        switch (value_type) {
        case value_Unspecified: return;
        case value_Inherit: out << "inherit"; return;
        case value_Auto:    out << (normal_instead_of_auto ? "normal" : "auto"); return;
        }
        int const i = value_type - _value_End;
        int const j = i - _lengthUnit_End;
        if (value_type < _value_End || Names::num_names <= j)
          throw runtime_error("Unexpected type of value for length property");
        if (j < 0) out << length << LengthUnitEnum(LengthUnit(i));
        else {
          typedef typename Names::endowed_enum_type NameEnum;
          typedef typename NameEnum::base_enum_type Name;
          out << NameEnum(Name(j));
        }
      }

      bool parse_value(string const &str, ManipContext &context)
      {
        if (str.empty()) {
          value_type = value_Unspecified;
          return true;
        }
        if (str == "inherit") {
          value_type = value_Inherit;
          return true;
        }
        float l;
        string u;
        if (context.parse_length(str, l, u)) {
          LengthUnitEnum unit;
          if (unit.parse(u)) {
            if (!allow_bare_numbers && unit == lengthUnit_None && l != 0) return false;
            if (!allow_percentages && unit == lengthUnit_Percent) return false;
            if (!allow_negative_values && l < 0) return false;
            value_type = _value_End + unit;
            length = l;
            return true;
          }
          return false;
        }
        if (has_keyword_auto && str == (normal_instead_of_auto ? "normal" : "auto")) {
          value_type = value_Auto;
          return true;
        }
        if (0 < Names::num_names) {
          typedef typename Names::endowed_enum_type NameEnum;
          typedef typename NameEnum::base_enum_type Name;
          NameEnum name;
          if (name.parse(str)) {
            value_type = _value_End + _lengthUnit_End + name;
            return true;
          }
        }
        return false;
      }

      bool operator==(LengthPropBase const &p) const
      {
        if (value_type != p.value_type) return false;
        int const i = this->value_type - _value_End;
        return 0 <= i && i < _lengthUnit_End ? length == p.length : true;
      }

    protected:
      float length;
    };



    template<class Spec>
    struct LengthProp:
      LengthPropBase<Spec::allow_bare_numbers, Spec::allow_percentages,
                     Spec::allow_negative_values, Spec::has_keyword_auto,
                     Spec::normal_instead_of_auto, typename Spec::Names>
    {
      typedef Spec spec_type;

      void apply_to(Applyee &applyee) const
      {
        if (this->value_type == value_Unspecified) return;
        int const i = this->value_type - _value_End;
        int const j = i - _lengthUnit_End;
        typedef typename Spec::Names Names;
        typename Spec::value_type value;
        if (this->value_type < _value_End || Names::num_names <= j) {
          switch (this->value_type) {
          case value_Inherit: applyee.inherit<Spec>(); return;
          case value_Auto:    set_auto(value); break;
          default: throw runtime_error("Unexpected type of value for length property");
          }
        }
        else if (j < 0) {
          double const l = this->length;
          switch (LengthUnit(i)) {
          case lengthUnit_None:    set_bare_number(applyee, value, l);           break;
          case lengthUnit_Percent: set_percentage(applyee, value, l);            break;
          case lengthUnit_EM:      set_abs(value, from_font_size(applyee, l));   break;
          case lengthUnit_EX:      set_abs(value, from_height_of_x(applyee, l)); break;
          case lengthUnit_PX:      set_abs(value, l);                            break;
          case lengthUnit_CM:      set_abs(value, from_centimeters(applyee, l)); break;
          case lengthUnit_MM:      set_abs(value, from_millimeters(applyee, l)); break;
          case lengthUnit_IN:      set_abs(value, from_inches(applyee, l));      break;
          case lengthUnit_PT:      set_abs(value, from_points(applyee, l));      break;
          case lengthUnit_PC:      set_abs(value, from_picas(applyee, l));       break;
          case _lengthUnit_End: return; // Never happens
          }
        }
        else {
          typedef typename Names::endowed_enum_type::base_enum_type Name;
          set_abs(value, Names::get_named_value(applyee, Name(j)));
        }
        applyee.set<Spec>(value);
      }

      static double from_font_size(Applyee &applyee, double font_sizes)
      {
        return applyee.get_current_font_size() * font_sizes;
      }

      static double from_height_of_x(Applyee &applyee, double heights_of_x)
      {
        return applyee.get_current_height_of_x() * heights_of_x;
      }

      static double from_centimeters(Applyee &applyee, double centimeters)
      {
        return applyee.get_dpcm() * centimeters;
      }

      static double from_millimeters(Applyee &applyee, double millimeters)
      {
        return 0.1 * applyee.get_dpcm() * millimeters;
      }

      static double from_inches(Applyee &applyee, double inches)
      {
        return 2.54 * applyee.get_dpcm() * inches;
      }

      static double from_points(Applyee &applyee, double points)
      {
        return 2.54/72 * applyee.get_dpcm() * points;
      }

      static double from_picas(Applyee &applyee, double picas)
      {
        return 12*2.54/72 * applyee.get_dpcm() * picas;
      }


      static void set_auto(EvaluatedStyle::Length &v)
      {
        v = 0;
      }

      static void set_auto(EvaluatedStyle::AugmentedLength &v)
      {
        v.state = EvaluatedStyle::AugmentedLength::state_Auto;
      }

      static void set_abs(EvaluatedStyle::Length &v, double w)
      {
        v = w;
      }

      static void set_abs(EvaluatedStyle::AugmentedLength &v, double w)
      {
        v.state = EvaluatedStyle::AugmentedLength::state_Abs;
        v.value = w;
      }

      static void set_percentage(Applyee &applyee, EvaluatedStyle::Length &v, double w)
      {
        v = 0.01 * w * Spec::get_relative_base(applyee);
      }

      static void set_percentage(Applyee &, EvaluatedStyle::AugmentedLength &v, double w)
      {
        if (Spec::force_percentage_eval) set_abs(v,w);
        else {
          v.state = EvaluatedStyle::AugmentedLength::state_Rel;
          v.value = 0.01 * w;
        }
      }

      static void set_bare_number(Applyee &applyee, EvaluatedStyle::Length &v, double w)
      {
        v = w == 0 ? 0 : w * Spec::get_relative_base(applyee);
      }

      static void set_bare_number(Applyee &, EvaluatedStyle::AugmentedLength &v, double w)
      {
        v.state = EvaluatedStyle::AugmentedLength::state_Rel;
        v.value = w;
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (this->value_type == value_Unspecified) return;
        out << context.widen(Spec::get_name()) << ": ";
        this->format_value(out, context);
        out << "; ";
      }

      using LengthPropBase<Spec::allow_bare_numbers, Spec::allow_percentages,
                           Spec::allow_negative_values, Spec::has_keyword_auto,
                           Spec::normal_instead_of_auto, typename Spec::Names>::operator=;
    };



    // The enumeration keyword with index I is represented as
    // _value_End + SpecialNames::num_names + I. The special keyword
    // with index I is represented as _value_End + I.
    template<class Enum, class SpecialNames> struct EnumPropBase: PropBase
    {
      void format_value(wostream &out, ManipContext &) const
      {
        switch (value_type) {
        case value_Unspecified: return;
        case value_Inherit: out << "inherit"; return;
        }
        if (value_type < _value_End)
          throw runtime_error("Unexpected type of value for enum property");
        int const i = value_type - _value_End;
        int const j = i - SpecialNames::num_names;
        if (j < 0) {
          typedef typename SpecialNames::endowed_enum_type NameEnum;
          typedef typename NameEnum::base_enum_type Name;
          out << NameEnum(Name(i));
        }
        else {
          typedef typename Enum::base_enum_type base_enum_type;
          out << Enum(base_enum_type(j));
        }
      }

      bool parse_value(string const &str, ManipContext &)
      {
        if (str.empty()) {
          value_type = value_Unspecified;
          return true;
        }
        if (str == "inherit") {
          value_type = value_Inherit;
          return true;
        }
        if (0 < SpecialNames::num_names) {
          typedef typename SpecialNames::endowed_enum_type NameEnum;
          typedef typename NameEnum::base_enum_type Name;
          NameEnum name;
          if (name.parse(str)) {
            value_type = _value_End + name;
            return true;
          }
        }
        Enum value;
        if (value.parse(str)) {
          value_type = _value_End + SpecialNames::num_names + value;
          return true;
        }
        return false;
      }

      bool operator==(EnumPropBase const &p) const { return value_type == p.value_type; }
    };



    template<class Spec> struct EnumProp: EnumPropBase<typename Spec::endowed_enum_type,
                                                       typename Spec::SpecialNames>
    {
      typedef Spec spec_type;

      void apply_to(Applyee &applyee) const
      {
        switch (this->value_type) {
        case value_Unspecified: return;
        case value_Inherit: applyee.inherit<Spec>(); return;
        }
        if (this->value_type < _value_End)
          throw runtime_error("Unexpected type of value for enum property");
        typedef typename Spec::endowed_enum_type::base_enum_type enum_type;
        enum_type value;
        typedef typename Spec::SpecialNames SpecialNames;
        int const i = this->value_type - _value_End;
        int const j = i - SpecialNames::num_names;
        if (j < 0) {
          typedef typename SpecialNames::endowed_enum_type::base_enum_type Name;
          value = SpecialNames::get_named_value(applyee, Name(i));
        }
        else value = static_cast<enum_type>(j);
        applyee.set<Spec>(value);
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (this->value_type == value_Unspecified) return;
        out << context.widen(Spec::get_name()) << ": ";
        this->format_value(out, context);
        out << "; ";
      }

      using EnumPropBase<typename Spec::endowed_enum_type,
                         typename Spec::SpecialNames>::operator=;
    };



    // In CSS2.1 'background-color' has a special 'transparent' value
    // that the other color properties do not. In CSS3 'transparent'
    // is a genuine named color and is availble to all color
    // properties. The color keyword with index I is represented as
    // _value_End + I.
    template<bool has_css21_transparent> struct ColorPropBase: PropBase
    {
      void format_value(wostream &out, ManipContext &context) const
      {
        int format;
        switch (value_type) {
        case value_Unspecified: return;
        case value_Inherit: out << "inherit"; return;
        case value_Transparent: out << "transparent"; return;
        case value_RGB_Number:  format = 3; break;
        case value_RGB_Percent: format = 4; break;
        case value_HSL_Number:  format = 5; break;
        case value_HSL_Percent: format = 6; break;
        default:
          {
            if (value_type < _value_End)
              throw runtime_error("Unexpected type of value for color property");
            int const i = value_type - _value_End;
            int const j = i - _sysColor_End;
            if (0 <= j) out << context.widen(PackedTRGB::get_color_name(j));
            else out << SystemColorEnum(SystemColor(i));
          }
          return;
        }
        string const str = PackedTRGB::format(format, Vec4F(red, green, blue, alpha),
                                              context.get_locale(), context.get_css_level());
        out << context.widen(str);
      }

      bool parse_value(string const &str, ManipContext &context)
      {
        PackedTRGB::CssLevel const css_level = context.get_css_level();
        if (str.empty()) {
          value_type = value_Unspecified;
          return true;
        }
        if (str == "inherit") {
          value_type = value_Inherit;
          return true;
        }
        if (has_css21_transparent && css_level == PackedTRGB::css21 && str == "transparent") {
          value_type = value_Transparent;
          return true;
        }
        int named_index;
        Vec4F rgba;
        int const res = PackedTRGB::parse(str, named_index, rgba, context.get_locale(), css_level);
        switch (res) {
        case 0:
          {
            SystemColorEnum sys_color;
            if(sys_color.parse(str)) {
              value_type = _value_End + sys_color;
              if (css_level != PackedTRGB::css21)
                context.deprecation_warning("System colors are deprecated in CSS3");
              return true;
            }
          }
          return false;
        case 1:
          value_type = _value_End + _sysColor_End + named_index;
          return true;
        case 2:
        case 3: value_type = value_RGB_Number;  break;
        case 4: value_type = value_RGB_Percent; break;
        case 5: value_type = value_HSL_Number;  break;
        case 6: value_type = value_HSL_Percent; break;
        default: throw runtime_error("Unexpected parsed color format");
        }
        red   = rgba[0];
        green = rgba[1];
        blue  = rgba[2];
        alpha = rgba[3];
        return true;
      }

      bool operator==(ColorPropBase const &p) const
      {
        if (value_type != p.value_type) return false;
        switch (value_type) {
        default: return true;
        case value_RGB_Number:
        case value_RGB_Percent:
        case value_HSL_Number:
        case value_HSL_Percent:
          return red == p.red && green = p.green && blue == p.blue && alpha == p.alpha;
        }
      }

    protected:
      float red, green, blue, alpha;
    };



    template<class Spec> struct ColorProp: ColorPropBase<Spec::has_css21_transparent>
    {
      typedef Spec spec_type;

      void apply_to(Applyee &applyee) const
      {
        float r,g,b,a;
        switch (this->value_type) {
        case value_Unspecified: return;
        case value_Inherit: applyee.inherit<Spec>(); return;
        case value_Transparent: r = g = b = a = 0; break;
        case value_RGB_Number:
          r = this->red/255; g = this->green/255; b = this->blue/255; a = this->alpha; break;
        case value_RGB_Percent:
          r = this->red/100; g = this->green/100; b = this->blue/100; a = this->alpha; break;
        case value_HSL_Number:
        case value_HSL_Percent:
          throw runtime_error("Unfortunately, the HSL color space is not yet available");
        default:
          {
            if (this->value_type < _value_End)
              throw runtime_error("Unexpected type of value for color property");
            EvaluatedStyle::Color color;
            int const i = this->value_type - _value_End;
            int const j = i - _sysColor_End;
            if (0 <= j) PackedTRGB::unpack(PackedTRGB::get_named_color(j), color);
            else applyee.get_system_color(SystemColor(i), color);
            applyee.set<Spec>(color);
            return;
          }
        }
        applyee.set<Spec>(EvaluatedStyle::Color(r,g,b,a));
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (this->value_type == value_Unspecified) return;
        out << context.widen(Spec::get_name()) << ": ";
        this->format_value(out, context);
        out << "; ";
      }

      using ColorPropBase<Spec::has_css21_transparent>::operator=;
    };




    // Compile-time specification of all CSS properties

    // A concrete version of this class must define the following members:
    //   static value_get_type get_value(EvaluatedStyle const &)
    //   static value_type &get_access(EvaluatedStyle &)
    //
    // get_value() must return the computed value as defined by the
    // CSS specification. get_access() must give direct access to the
    // property as it is stored in an instance of EvaluatedStyle. The
    // stored value is generally identical to the computed value, but
    // there are exceptions.
    template<class T, bool def_inherit, EvaluatedStyle::GroupId group> struct PropSpecBase
    {
      // This type may be overridden by derived classes.
      typedef T value_type;

      // This type may be overridden by derived classes, but it must
      // always be convertible to 'value_type'.
      typedef value_type const &value_get_type;

      static bool const is_default_inherited = def_inherit;

      static EvaluatedStyle::GroupId const eval_group = group;

      // Called if this property is set to any value after it has
      // received its default value.
      static void on_value_specified(EvaluatedStyle &) {}
    };


    template<class T> struct NoNamedValues
    {
      static int const num_names = 0;
      typedef EmptyEnum endowed_enum_type;
      static T get_named_value(Applyee &, EmptyBaseEnum) { return T(); }
    };

    template<bool def_inherit, EvaluatedStyle::GroupId group>
    struct LengthPropSpec:
      PropSpecBase<EvaluatedStyle::Length, def_inherit, group>
    {
      static bool const allow_bare_numbers = false;

      static bool const allow_percentages = false;

      // When the computed value is requested, a percentage is
      // converted to an absolute number of pixels if, and only if
      // this flag is true or 'value_type' is EvaluatedStyle::Length.
      static bool const force_percentage_eval = false;

      // This one is used to resolve bare numbers when they are
      // allowed and 'value_type' is EvaluatedStyle::Length. It is
      // also used to resolve percentages when they are allowed and
      // 'value_type' is EvaluatedStyle::Length or
      // force_percentage_eval is true.
      static double get_relative_base(Applyee &) { return 0; }

      static bool const allow_negative_values = false;

      static bool const has_keyword_auto = false;

      static bool const normal_instead_of_auto = false;

      typedef NoNamedValues<double> Names;
    };

    template<bool def_inherit, EvaluatedStyle::GroupId group>
    struct AugmentedLengthPropSpec: LengthPropSpec<def_inherit, group>
    {
      typedef EvaluatedStyle::AugmentedLength value_type;
      typedef value_type const &value_get_type;
      static bool const allow_percentages = true;
    };

    template<class Enum, bool def_inherit, EvaluatedStyle::GroupId group>
    struct EnumPropSpec:
      PropSpecBase<typename Enum::base_enum_type, def_inherit, group>
    {
      typedef Enum endowed_enum_type;
      typedef NoNamedValues<typename Enum::base_enum_type> SpecialNames;
    };

    template<bool def_inherit, EvaluatedStyle::GroupId group>
    struct ColorPropSpec:
      PropSpecBase<EvaluatedStyle::Color, def_inherit, group>
    {
      static bool const has_css21_transparent = false;
    };



    struct PropSpec_BackgroundColor: ColorPropSpec<false, EvaluatedStyle::group_Background>
    {
      static bool const has_css21_transparent = true;
      static string get_name() { return "background-color"; }

      static value_get_type get_value(EvaluatedStyle const &s) { return s.background.color; }
      static value_type &get_access(EvaluatedStyle &s)         { return s.background.color; }
    };



    struct BorderWidthPropSpecBase: LengthPropSpec<false, EvaluatedStyle::group_Border>
    {
      typedef value_type value_get_type;

      struct Names
      {
        static int const num_names = _borderWidth_End;
        typedef NamedBorderWidthEnum endowed_enum_type;
        static double get_named_value(Applyee &applyee, NamedBorderWidth w)
        {
          return applyee.get_std_border_width(w);
        }
      };
    };

    template<EvaluatedStyle::Border::Side EvaluatedStyle::Border::*side>
    struct BorderWidthPropSpec: BorderWidthPropSpecBase
    {
      static value_get_type get_value(EvaluatedStyle const &s)
      {
        EvaluatedStyle::Border::Side const &side2 = s.border.*side;
        return side2.style == borderStyle_None ||
          side2.style == borderStyle_Hidden ? 0 : side2.width;
      }

      static value_type &get_access(EvaluatedStyle &s)
      {
        return (s.border.*side).width;
      }
    };

    struct PropSpec_BorderTopWidth: BorderWidthPropSpec<&EvaluatedStyle::Border::top>
    {
      static string get_name() { return "border-top-width"; }
    };

    struct PropSpec_BorderRightWidth: BorderWidthPropSpec<&EvaluatedStyle::Border::right>
    {
      static string get_name() { return "border-right-width"; }
    };

    struct PropSpec_BorderBottomWidth: BorderWidthPropSpec<&EvaluatedStyle::Border::bottom>
    {
      static string get_name() { return "border-bottom-width"; }
    };

    struct PropSpec_BorderLeftWidth: BorderWidthPropSpec<&EvaluatedStyle::Border::left>
    {
      static string get_name() { return "border-left-width"; }
    };



    typedef EnumPropSpec<BorderStyleEnum, false,
                         EvaluatedStyle::group_Border> BorderStylePropSpecBase;

    template<EvaluatedStyle::Border::Side EvaluatedStyle::Border::*side>
    struct BorderStylePropSpec: BorderStylePropSpecBase
    {
      static value_get_type get_value(EvaluatedStyle const &s) { return (s.border.*side).style; }
      static value_type &get_access(EvaluatedStyle &s)         { return (s.border.*side).style; }
    };

    struct PropSpec_BorderTopStyle: BorderStylePropSpec<&EvaluatedStyle::Border::top>
    {
      static string get_name() { return "border-top-style"; }
    };

    struct PropSpec_BorderRightStyle: BorderStylePropSpec<&EvaluatedStyle::Border::right>
    {
      static string get_name() { return "border-right-style"; }
    };

    struct PropSpec_BorderBottomStyle: BorderStylePropSpec<&EvaluatedStyle::Border::bottom>
    {
      static string get_name() { return "border-bottom-style"; }
    };

    struct PropSpec_BorderLeftStyle: BorderStylePropSpec<&EvaluatedStyle::Border::left>
    {
      static string get_name() { return "border-left-style"; }
    };



    typedef ColorPropSpec<false, EvaluatedStyle::group_Border> BorderColorPropSpecBase;

    template<EvaluatedStyle::Border::Side EvaluatedStyle::Border::*side>
    struct BorderColorPropSpec: BorderColorPropSpecBase
    {
      static value_get_type get_value(EvaluatedStyle const &s)
      {
        EvaluatedStyle::Border::Side const &side2 = s.border.*side;
        return side2.color_specified ? side2.color : s.text.color;
      }

      static value_type &get_access(EvaluatedStyle &s)
      {
        return (s.border.*side).color;
      }

      static void on_value_specified(EvaluatedStyle &s)
      {
        (s.border.*side).color_specified = true;
      }
    };

    struct PropSpec_BorderTopColor: BorderColorPropSpec<&EvaluatedStyle::Border::top>
    {
      static string get_name() { return "border-top-color"; }
    };

    struct PropSpec_BorderRightColor: BorderColorPropSpec<&EvaluatedStyle::Border::right>
    {
      static string get_name() { return "border-right-color"; }
    };

    struct PropSpec_BorderBottomColor: BorderColorPropSpec<&EvaluatedStyle::Border::bottom>
    {
      static string get_name() { return "border-bottom-color"; }
    };

    struct PropSpec_BorderLeftColor: BorderColorPropSpec<&EvaluatedStyle::Border::left>
    {
      static string get_name() { return "border-left-color"; }
    };



    struct PropSpec_Color: ColorPropSpec<true, EvaluatedStyle::group_Text>
    {
      static string get_name() { return "color"; }

      static value_get_type get_value(EvaluatedStyle const &s) { return s.text.color; }
      static value_type &get_access(EvaluatedStyle &s)         { return s.text.color; }
    };



    struct PropSpec_FontStyle: EnumPropSpec<FontStyleEnum, true, EvaluatedStyle::group_Font>
    {
      static string get_name() { return "font-style"; }

      static value_get_type get_value(EvaluatedStyle const &s) { return s.font.style; }
      static value_type &get_access(EvaluatedStyle &s)         { return s.font.style; }
    };



    struct PropSpec_FontVariant: EnumPropSpec<FontVariantEnum, true, EvaluatedStyle::group_Font>
    {
      static string get_name() { return "font-variant"; }

      static value_get_type get_value(EvaluatedStyle const &s) { return s.font.variant; }
      static value_type &get_access(EvaluatedStyle &s)         { return s.font.variant; }
    };



    struct PropSpec_FontWeight: EnumPropSpec<FontWeightEnum, true, EvaluatedStyle::group_Font>
    {
      struct SpecialNames
      {
        static int const num_names = _specialFontWeight_End;
        typedef SpecialFontWeightEnum endowed_enum_type;
        static FontWeight get_named_value(Applyee &applyee, SpecialFontWeight w)
        {
          switch (w) {
          default:                     return fontWeight_400;
          case specialFontWeight_Bold: return fontWeight_700;

          case specialFontWeight_Bolder:
            switch (applyee.get_from_parent<PropSpec_FontWeight>()) {
            case fontWeight_100: return fontWeight_400;
            case fontWeight_200: return fontWeight_400;
            case fontWeight_300: return fontWeight_400;
            default:             return fontWeight_700;
            case fontWeight_500: return fontWeight_700;
            case fontWeight_600: return fontWeight_900;
            case fontWeight_700: return fontWeight_900;
            case fontWeight_800: return fontWeight_900;
            case fontWeight_900: return fontWeight_900;
            }

          case specialFontWeight_Lighter:
            switch (applyee.get_from_parent<PropSpec_FontWeight>()) {
            case fontWeight_100: return fontWeight_100;
            case fontWeight_200: return fontWeight_100;
            case fontWeight_300: return fontWeight_100;
            default:             return fontWeight_100;
            case fontWeight_500: return fontWeight_100;
            case fontWeight_600: return fontWeight_400;
            case fontWeight_700: return fontWeight_400;
            case fontWeight_800: return fontWeight_700;
            case fontWeight_900: return fontWeight_700;
            }
          }
        }
      };

      static string get_name() { return "font-weight"; }

      static value_get_type get_value(EvaluatedStyle const &s)
      {
        return s.font.weight;
      }

      static value_type &get_access(EvaluatedStyle &s)
      {
        return s.font.weight;
      }
    };



    struct PropSpec_FontSize: LengthPropSpec<true, EvaluatedStyle::group_Font>
    {
      static bool const allow_percentages = true;

      static double get_relative_base(Applyee &applyee)
      {
        return applyee.get_current_font_size(); // This is the inherited value
      }

      struct Names
      {
        static int const num_names = _fontSize_End;
        typedef NamedFontSizeEnum endowed_enum_type;
        static double get_named_value(Applyee &applyee, NamedFontSize s)
        {
          switch (s) {
          case fontSize_XXSmall: return applyee.get_std_font_size(-3);
          case fontSize_XSmall:  return applyee.get_std_font_size(-2);
          case fontSize_Small:   return applyee.get_std_font_size(-1);
          default:               return applyee.get_std_font_size(00);
          case fontSize_Large:   return applyee.get_std_font_size(+1);
          case fontSize_XLarge:  return applyee.get_std_font_size(+2);
          case fontSize_XXLarge: return applyee.get_std_font_size(+3);

          case fontSize_Larger:
            return applyee.increase_font_size(applyee.get_current_font_size());

          case fontSize_Smaller:
            return applyee.decrease_font_size(applyee.get_current_font_size());
          }
        }
      };

      static string get_name() { return "font-size"; }

      static value_get_type get_value(EvaluatedStyle const &s)
      {
        return s.font.size;
      }

      static value_type &get_access(EvaluatedStyle &s)
      {
        return s.font.size;
      }
    };



    struct PropSpec_LineHeight: AugmentedLengthPropSpec<true, EvaluatedStyle::group_Text>
    {
      static bool const allow_bare_numbers     = true;
      static bool const allow_percentages      = true;
      static bool const force_percentage_eval  = true;
      static bool const has_keyword_auto       = true;
      static bool const normal_instead_of_auto = true;

      static double get_relative_base(Applyee &applyee)
      {
        return applyee.get_current_font_size();
      }

      static string get_name() { return "line-height"; }

      static value_get_type get_value(EvaluatedStyle const &s)
      {
        return s.text.line_height;
      }

      static value_type &get_access(EvaluatedStyle &s)
      {
        return s.text.line_height;
      }
    };



    template<EvaluatedStyle::AugmentedLength EvaluatedStyle::Margin::*side>
    struct MarginPropSpec: AugmentedLengthPropSpec<false, EvaluatedStyle::group_Margin>
    {
      static bool const allow_percentages     = true;
      static bool const allow_negative_values = true;
      static bool const has_keyword_auto      = true;

      static value_get_type get_value(EvaluatedStyle const &s) { return s.margin.*side; }
      static value_type &get_access(EvaluatedStyle &s)         { return s.margin.*side; }
    };

    struct PropSpec_MarginTop: MarginPropSpec<&EvaluatedStyle::Margin::top>
    {
      static string get_name() { return "margin-top"; }
    };

    struct PropSpec_MarginRight: MarginPropSpec<&EvaluatedStyle::Margin::right>
    {
      static string get_name() { return "margin-right"; }
    };

    struct PropSpec_MarginBottom: MarginPropSpec<&EvaluatedStyle::Margin::bottom>
    {
      static string get_name() { return "margin-bottom"; }
    };

    struct PropSpec_MarginLeft: MarginPropSpec<&EvaluatedStyle::Margin::left>
    {
      static string get_name() { return "margin-left"; }
    };



    template<EvaluatedStyle::AugmentedLength EvaluatedStyle::Padding::*side>
    struct PaddingPropSpec: AugmentedLengthPropSpec<false, EvaluatedStyle::group_Padding>
    {
      static bool const allow_percentages = true;

      static value_get_type get_value(EvaluatedStyle const &s) { return s.padding.*side; }
      static value_type &get_access(EvaluatedStyle &s)         { return s.padding.*side; }
    };

    struct PropSpec_PaddingTop: PaddingPropSpec<&EvaluatedStyle::Padding::top>
    {
      static string get_name() { return "padding-top"; }
    };

    struct PropSpec_PaddingRight: PaddingPropSpec<&EvaluatedStyle::Padding::right>
    {
      static string get_name() { return "padding-right"; }
    };

    struct PropSpec_PaddingBottom: PaddingPropSpec<&EvaluatedStyle::Padding::bottom>
    {
      static string get_name() { return "padding-bottom"; }
    };

    struct PropSpec_PaddingLeft: PaddingPropSpec<&EvaluatedStyle::Padding::left>
    {
      static string get_name() { return "padding-left"; }
    };



    template<EvaluatedStyle::AugmentedLength EvaluatedStyle::Size::*which>
    struct SizePropSpec: AugmentedLengthPropSpec<false, EvaluatedStyle::group_Size>
    {
      static bool const allow_percentages = true;
      static bool const has_keyword_auto  = true;

      static value_get_type get_value(EvaluatedStyle const &s) { return s.size.*which; }
      static value_type &get_access(EvaluatedStyle &s)         { return s.size.*which; }
    };

    struct PropSpec_Width: SizePropSpec<&EvaluatedStyle::Size::width>
    {
      static string get_name() { return "width"; }
    };

    struct PropSpec_Height: SizePropSpec<&EvaluatedStyle::Size::height>
    {
      static string get_name() { return "height"; }
    };



    // Definition of a dynamic style declaration


    template<class Top, class Right, class Bottom, class Left> struct RectGroup
    {
      void apply_to(Applyee &applyee) const
      {
        top.apply_to(applyee);
        right.apply_to(applyee);
        bottom.apply_to(applyee);
        left.apply_to(applyee);
      }

      void format(wostream &out, ManipContext &context) const
      {
        top.format(out, context);
        right.format(out, context);
        bottom.format(out, context);
        left.format(out, context);
      }

      Top    top;
      Right  right;
      Bottom bottom;
      Left   left;
    };



    struct BackgroundGroup
    {
      void apply_to(Applyee &applyee) const
      {
        color.apply_to(applyee);
      }

      bool format_shorthand(wostream &, ManipContext &) const
      {
        return false; // FIXME: Implement this!
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (format_shorthand(out, context)) return;
        color.format(out, context);
      }

      ColorProp<PropSpec_BackgroundColor> color;
    };



    struct BorderGroup
    {
      void apply_to(Applyee &applyee) const
      {
        width.apply_to(applyee);
        style.apply_to(applyee);
        color.apply_to(applyee);
      }

      bool format_shorthand(wostream &out, ManipContext &context) const
      {
        TopWidth const w = width.top;
        if (w != width.right || w != width.bottom || w != width.left) return false;
        TopStyle const s = style.top;
        if (s != style.right || s != style.bottom || s != style.left) return false;
        TopColor const c = color.top;
        if (c != color.right || c != color.bottom || c != color.left) return false;
        if (!w && !s && !c) return false;
        out << "border:";
        if (w) {
          out << ' ';
          w.format_value(out, context);
        }
        if (s) {
          out << ' ';
          s.format_value(out, context);
        }
        if (c) {
          out << ' ';
          c.format_value(out, context);
        }
        out << "; ";
        return true;
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (format_shorthand(out, context)) return;
        width.format(out, context);
        style.format(out, context);
        color.format(out, context);
      }

      typedef LengthProp<PropSpec_BorderTopWidth>    TopWidth;
      typedef LengthProp<PropSpec_BorderRightWidth>  RightWidth;
      typedef LengthProp<PropSpec_BorderBottomWidth> BottomWidth;
      typedef LengthProp<PropSpec_BorderLeftWidth>   LeftWidth;
      typedef RectGroup<TopWidth, RightWidth, BottomWidth, LeftWidth> Width;
      Width width;

      typedef EnumProp<PropSpec_BorderTopStyle>    TopStyle;
      typedef EnumProp<PropSpec_BorderRightStyle>  RightStyle;
      typedef EnumProp<PropSpec_BorderBottomStyle> BottomStyle;
      typedef EnumProp<PropSpec_BorderLeftStyle>   LeftStyle;
      typedef RectGroup<TopStyle, RightStyle, BottomStyle, LeftStyle> Style;
      Style style;

      typedef ColorProp<PropSpec_BorderTopColor>    TopColor;
      typedef ColorProp<PropSpec_BorderRightColor>  RightColor;
      typedef ColorProp<PropSpec_BorderBottomColor> BottomColor;
      typedef ColorProp<PropSpec_BorderLeftColor>   LeftColor;
      typedef RectGroup<TopColor, RightColor, BottomColor, LeftColor> Color;
      Color color;
    };



    struct FontGroup
    {
      void apply_font_to(Applyee &applyee) const
      {
        style.apply_to(applyee);
        variant.apply_to(applyee);
        weight.apply_to(applyee);
        size.apply_to(applyee);
        // FIXME: This one must also apply 'font-family'
      }

      void apply_to(Applyee &applyee) const
      {
        line_height.apply_to(applyee);
      }

      bool format_shorthand(wostream &, ManipContext &) const
      {
        return false; // FIXME: Implement this!
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (format_shorthand(out, context)) return;
        size.format(out, context);
        line_height.format(out, context);
      }

      EnumProp<PropSpec_FontStyle>    style;
      EnumProp<PropSpec_FontVariant>  variant;
      EnumProp<PropSpec_FontWeight>   weight;
      LengthProp<PropSpec_FontSize>   size;  // FIXME: What about special font size keywords?
      LengthProp<PropSpec_LineHeight> line_height;
    };



    typedef LengthProp<PropSpec_MarginTop>    MarginTop;    // FIXME: What about keyword 'auto'?
    typedef LengthProp<PropSpec_MarginRight>  MarginRight;
    typedef LengthProp<PropSpec_MarginBottom> MarginBottom;
    typedef LengthProp<PropSpec_MarginLeft>   MarginLeft;
    typedef RectGroup<MarginTop, MarginRight, MarginBottom, MarginLeft> MarginRect;
    struct MarginGroup: MarginRect
    {
      bool format_shorthand(wostream &, ManipContext &) const
      {
        return false; // FIXME: Implement this!
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (format_shorthand(out, context)) return;
        MarginRect::format(out, context);
      }
    };



    typedef LengthProp<PropSpec_PaddingTop>    PaddingTop;
    typedef LengthProp<PropSpec_PaddingRight>  PaddingRight;
    typedef LengthProp<PropSpec_PaddingBottom> PaddingBottom;
    typedef LengthProp<PropSpec_PaddingLeft>   PaddingLeft;
    typedef RectGroup<PaddingTop, PaddingRight, PaddingBottom, PaddingLeft> PaddingRect;
    struct PaddingGroup: PaddingRect
    {
      bool format_shorthand(wostream &, ManipContext &) const
      {
        return false; // FIXME: Implement this!
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (format_shorthand(out, context)) return;
        PaddingRect::format(out, context);
      }
    };



    struct SizeGroup
    {
      void apply_to(Applyee &applyee) const
      {
        width.apply_to(applyee);
        height.apply_to(applyee);
      }

      void format(wostream &out, ManipContext &context) const
      {
        width.format(out, context);
        height.format(out, context);
      }

      LengthProp<PropSpec_Width>  width;
      LengthProp<PropSpec_Height> height;
    };



    struct TextGroup
    {
      void apply_font_to(Applyee &applyee) const
      {
        if (font) font->apply_font_to(applyee);
      }

      void apply_to(Applyee &applyee) const
      {
        color.apply_to(applyee);
        if (font) font->apply_to(applyee);
      }

      void format(wostream &out, ManipContext &context) const
      {
        color.format(out, context);
        if (font) font->format(out, context);
      }

      ColorProp<PropSpec_Color> color;
      UniquePtr<FontGroup>      font;
    };



    struct BoxGroup1
    {
      void apply_to(Applyee &applyee) const
      {
        if (background) background->apply_to(applyee);
        if (margin)         margin->apply_to(applyee);
        if (padding)       padding->apply_to(applyee);
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (background) background->format(out, context);
        if (margin)         margin->format(out, context);
        if (padding)       padding->format(out, context);
      }

      UniquePtr<BackgroundGroup> background;
      UniquePtr<MarginGroup>     margin;
      UniquePtr<PaddingGroup>    padding;
    };



    struct BoxGroup2
    {
      void apply_to(Applyee &applyee) const
      {
        if (size) size->apply_to(applyee);
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (size) size->format(out, context);
      }

      UniquePtr<SizeGroup> size;
    };



    struct StyleDecl
    {
      void apply_font_to(Applyee &applyee) const
      {
        if (text) text->apply_font_to(applyee);
      }

      void apply_to(Applyee &applyee) const
      {
        if (text)     text->apply_to(applyee);
        if (box1)     box1->apply_to(applyee);
        if (border) border->apply_to(applyee);
      }

      void format(wostream &out, ManipContext &context) const
      {
        if (text)     text->format(out, context);
        if (box1)     box1->format(out, context);
        if (border) border->format(out, context);
      }

      UniquePtr<TextGroup>   text;
      UniquePtr<BoxGroup1>   box1;
      UniquePtr<BoxGroup2>   box2;
      UniquePtr<BorderGroup> border;
    };



    struct Element: dom::css::ElementCSSInlineStyle, dom::css::CSSStyleDeclaration,
                    DomImpl::ElementImpl
    {
      dom::css::CSSStyleDeclaration *getStyle() throw ()
      {
        return this;
      }

      dom::DOMString getCssText() throw ()
      {
        ManipContext &context = get_manip_context();
        wostringstream &out = context.get_format_stream();
        if (style_decl) style_decl->format(out, context);
        wstring const str = out.str();
        return context.encode_wide(str.substr(0, str.size()-1)); // Chop off final space
      }

      dom::DOMString getPropertyValue(dom::DOMString const &name) throw ();

      void setProperty(dom::DOMString const &name, dom::DOMString const &value,
                       dom::DOMString const &prio) throw (dom::DOMException);

      StyleDecl const *get_style_decl_read_ptr() const
      {
        return style_decl ? style_decl.get() : 0;
      }

      StyleDecl &get_style_decl_write_ref()
      {
        if (!style_decl) style_decl.reset(new StyleDecl);
        return *style_decl;
      }

      Document *get_document() const throw ()
      {
        return static_cast<Document *>(DomImpl::ElementImpl::get_document());
      }

      ManipContext &get_manip_context() const { return *get_document(); }

      Element(Document *doc): DomImpl::ElementImpl(doc) {}

    protected:
      // The 'font' group must always be applied first, such that
      // properties in the other groups can refer reliably to the
      // current font size, as well as to the current height of
      // 'x'. The 'font' group consists precisely of 'font-style',
      // 'font-variant', 'font-weight', 'font-size', and
      // 'font-family'.
      void apply_style_to(Applyee &applyee) const
      {
        // We must first determine the font size, since other values may depend on it
        apply_default_font_to(applyee);
        // FIXME: Apply font styles from style sheets here
        if (style_decl) style_decl->apply_font_to(applyee);
        applyee.flush_font();
        apply_default_style_to(applyee);
        // FIXME: Apply other styles from style sheets here. The brute
        // force way is to evaluate each selector against this
        // element, and apply the style if ther is a match. This might
        // require access to ancestors, siblings, and descendants. How
        // about 'important'? Can the results be cached?
        if (style_decl) style_decl->apply_to(applyee);
        applyee.flush_other();
      }

      virtual void apply_default_font_to(Applyee &) const = 0;
      virtual void apply_default_style_to(Applyee &) const = 0;

      UniquePtr<StyleDecl> style_decl;
    };



    dom::DOMString Element::getPropertyValue(dom::DOMString const &name) throw ()
    {
      ManipContext &context = get_manip_context();
      PropDef const *const prop = context.lookup_prop_def(name);
      return prop ? prop->get(*this) : dom::DOMString();
    }



    void Element::setProperty(dom::DOMString const &name, dom::DOMString const &value,
                              dom::DOMString const &prio) throw (dom::DOMException)
    {
      ManipContext &context = get_manip_context();
      if (context.parse_priority(prio) != priority_Normal)
        throw runtime_error("Non-default priority is not yet implemented");
      PropDef const *const prop = context.lookup_prop_def(name);
      if (!prop) throw dom::DOMException("NO_MODIFICATION_ALLOWED_ERR");
      prop->set(value, *this);
    }



    template<class Group> struct PropGroupInfo;

    template<> struct PropGroupInfo<BackgroundGroup>
    {
      typedef BoxGroup1 Parent;
      static UniquePtr<BackgroundGroup> Parent::*get_self() { return &Parent::background; }
    };

    template<> struct PropGroupInfo<BorderGroup::Width>
    {
      typedef BorderGroup Parent;
      static BorderGroup::Width Parent::*get_self() { return &Parent::width; }
    };

    template<> struct PropGroupInfo<BorderGroup::Style>
    {
      typedef BorderGroup Parent;
      static BorderGroup::Style Parent::*get_self() { return &Parent::style; }
    };

    template<> struct PropGroupInfo<BorderGroup::Color>
    {
      typedef BorderGroup Parent;
      static BorderGroup::Color Parent::*get_self() { return &Parent::color; }
    };

    template<> struct PropGroupInfo<FontGroup>
    {
      typedef TextGroup Parent;
      static UniquePtr<FontGroup> Parent::*get_self() { return &Parent::font; }
    };

    template<> struct PropGroupInfo<MarginGroup>
    {
      typedef BoxGroup1 Parent;
      static UniquePtr<MarginGroup> Parent::*get_self() { return &Parent::margin; }
    };

    template<> struct PropGroupInfo<PaddingGroup>
    {
      typedef BoxGroup1 Parent;
      static UniquePtr<PaddingGroup> Parent::*get_self() { return &Parent::padding; }
    };

    template<> struct PropGroupInfo<SizeGroup>
    {
      typedef BoxGroup2 Parent;
      static UniquePtr<SizeGroup> Parent::*get_self() { return &Parent::size; }
    };

    template<> struct PropGroupInfo<TextGroup>
    {
      typedef StyleDecl Parent;
      static UniquePtr<TextGroup> Parent::*get_self() { return &Parent::text; }
    };

    template<> struct PropGroupInfo<BoxGroup1>
    {
      typedef StyleDecl Parent;
      static UniquePtr<BoxGroup1> Parent::*get_self() { return &Parent::box1; }
    };

    template<> struct PropGroupInfo<BoxGroup2>
    {
      typedef StyleDecl Parent;
      static UniquePtr<BoxGroup2> Parent::*get_self() { return &Parent::box2; }
    };

    template<> struct PropGroupInfo<BorderGroup>
    {
      typedef StyleDecl Parent;
      static UniquePtr<BorderGroup> Parent::*get_self() { return &Parent::border; }
    };



    template<class Group> struct PropGroupAccess
    {
      typedef PropGroupInfo<Group> Info;
      typedef typename Info::Parent Parent;

      static Group const *get_read_ptr(Element const &elem)
      {
        Parent const *const p = PropGroupAccess<Parent>::get_read_ptr(elem);
        return p ? (p->*Info::get_self()).get() : 0;
      }

      static Group &get_write_ref(Element &elem)
      {
        Parent &p = PropGroupAccess<Parent>::get_write_ref(elem);
        UniquePtr<Group> &ptr = p.*Info::get_self();
        if (!ptr) ptr.reset(new Group);
        return *ptr;
      }
    };

    template<> struct PropGroupAccess<StyleDecl>
    {
      static StyleDecl const *get_read_ptr(Element const &elem)
      {
        return elem.get_style_decl_read_ptr();
      }

      static StyleDecl &get_write_ref(Element &elem)
      {
        return elem.get_style_decl_write_ref();
      }
    };

    template<class Top, class Right, class Bottom, class Left>
    struct PropGroupAccess<RectGroup<Top, Right, Bottom, Left> >
    {
      typedef RectGroup<Top, Right, Bottom, Left> Group;
      typedef PropGroupInfo<Group> Info;
      typedef typename Info::Parent Parent;

      static Group const *get_read_ptr(Element const &elem)
      {
        Parent const *const p = PropGroupAccess<Parent>::get_read_ptr(elem);
        return p ? &(p->*Info::get_self()) : 0;
      }

      static Group &get_write_ref(Element &elem)
      {
        return PropGroupAccess<Parent>::get_write_ref(elem).*Info::get_self();
      }
    };



    template<class Prop, class Group> struct LonghandPropDefBase: PropDef
    {
      dom::DOMString get(Element const &elem) const
      {
        Group const *const group = PropGroupAccess<Group>::get_read_ptr(elem);
        if (!group) return dom::DOMString();
        return elem.get_manip_context().format_prop(group->*prop);
      }

      LonghandPropDefBase(Prop Group::*p): prop(p) {}

    protected:
      Prop Group::*const prop;
    };


    // If \a is_narrow is true, the parse_value() method for the
    // target property is assumed to take as argument a narrow string
    // rather than a wide one. This is supposed to be used for
    // properties whose values are always confined to the portable
    // character set.
    template<class Prop, class Group, bool is_narrow = true> struct LonghandPropDef;

    template<class Prop, class Group>
    struct LonghandPropDef<Prop, Group, true>: LonghandPropDefBase<Prop, Group>
    {
      void set(dom::DOMString const &str, Element &elem) const
      {
        Prop value;
        elem.get_manip_context().parse_narrow_prop(str, value);
        PropGroupAccess<Group>::get_write_ref(elem).*this->prop = value;
      }

      LonghandPropDef(Prop Group::*p): LonghandPropDefBase<Prop, Group>(p) {}
    };

    template<class Prop, class Group>
    struct LonghandPropDef<Prop, Group, false>: LonghandPropDefBase<Prop, Group>
    {
      void set(dom::DOMString const &str, Element &elem) const
      {
        Prop value;
        elem.get_manip_context().parse_wide_prop(str, value);
        PropGroupAccess<Group>::get_write_ref(elem).*this->prop = value;
      }

      LonghandPropDef(Prop Group::*p): LonghandPropDefBase<Prop, Group>(p) {}
    };



    template<class Prop, class Group> PropDef &StaticInfo::add(Prop Group::*prop)
    {
      return add(Prop::spec_type::get_name(), new LonghandPropDef<Prop, Group>(prop));
    }

    template<class ForceGroup, class Prop, class Group>
    PropDef &StaticInfo::add2(Prop Group::*prop) { return add<Prop, ForceGroup>(prop); }



    struct BorderWidthPropDef: PropDef
    {
      typedef LengthPropBase<BorderWidthPropSpecBase::allow_bare_numbers,
                             BorderWidthPropSpecBase::allow_percentages,
                             BorderWidthPropSpecBase::allow_negative_values,
                             BorderWidthPropSpecBase::has_keyword_auto,
                             BorderWidthPropSpecBase::normal_instead_of_auto,
                             BorderWidthPropSpecBase::Names> prop_type;

      dom::DOMString get(Element const &) const
      {
        throw runtime_error("Not yet implemented"); // FIXME: Implement this!
      }

      void set(dom::DOMString const &, Element &) const
      {
        throw runtime_error("Not yet implemented"); // FIXME: Implement this!
      }
    };



    struct BorderStylePropDef: PropDef
    {
      typedef EnumPropBase<BorderStylePropSpecBase::endowed_enum_type,
                           BorderStylePropSpecBase::SpecialNames> prop_type;

      dom::DOMString get(Element const &) const
      {
        throw runtime_error("Not yet implemented"); // FIXME: Implement this!
      }

      void set(dom::DOMString const &, Element &) const
      {
        throw runtime_error("Not yet implemented"); // FIXME: Implement this!
      }
    };



    struct BorderColorPropDef: PropDef
    {
      typedef ColorPropBase<BorderColorPropSpecBase::has_css21_transparent> prop_type;

      dom::DOMString get(Element const &) const
      {
        throw runtime_error("Not yet implemented"); // FIXME: Implement this!
      }

      void set(dom::DOMString const &, Element &) const
      {
        throw runtime_error("Not yet implemented"); // FIXME: Implement this!
      }
    };



    struct BorderPropDef: PropDef
    {
      dom::DOMString get(Element const &) const
      {
        throw runtime_error("Not yet implemented"); // FIXME: Implement this!
      }

      void set(dom::DOMString const &str, Element &elem) const
      {
        ManipContext &context = elem.get_manip_context();
        BorderWidthPropDef::prop_type width;
        BorderStylePropDef::prop_type style;
        BorderColorPropDef::prop_type color;
        string str2;
        if (context.decode_narrow(str, str2)) {
          ctype<char> const &ctype_narrow = context.get_ctype_narrow();
          string::const_iterator const end = str2.end();
          string::const_iterator i = str2.begin();
          for (;;) {
            for (;;) {
              if (i == end) {
                BorderGroup &border = PropGroupAccess<BorderGroup>::get_write_ref(elem);
                border.width.top    = width;
                border.width.right  = width;
                border.width.bottom = width;
                border.width.left   = width;
                border.style.top    = style;
                border.style.right  = style;
                border.style.bottom = style;
                border.style.left   = style;
                border.color.top    = color;
                border.color.right  = color;
                border.color.bottom = color;
                border.color.left   = color;
                return;
              }
              if (!ctype_narrow.is(ctype_base::space, *i)) break;
              ++i;
            }

            int parenth_level = 0;
            string::const_iterator j = i;
            char c = *j;
            for (;;) {
              if (c == '(') ++parenth_level;
              else if (c == ')') --parenth_level;
              if (++j == end) break;
              c = *j;
              if (parenth_level == 0 && ctype_narrow.is(ctype_base::space, c)) break;
            }

            string atom(i,j);
            if ((width || !width.parse_value(atom, context)) &&
                (style || !style.parse_value(atom, context)) &&
                (color || !color.parse_value(atom, context))) break;

            i = j;
          }
        }
        throw dom::DOMException("SYNTAX_ERR");
      }
    };



    void StaticInfo::add_props()
    {
      add(&BackgroundGroup::color);
      add(&BorderGroup::Width::top);
      add(&BorderGroup::Width::right);
      add(&BorderGroup::Width::bottom);
      add(&BorderGroup::Width::left);
      add(&BorderGroup::Style::top);
      add(&BorderGroup::Style::right);
      add(&BorderGroup::Style::bottom);
      add(&BorderGroup::Style::left);
      add(&BorderGroup::Color::top);
      add(&BorderGroup::Color::right);
      add(&BorderGroup::Color::bottom);
      add(&BorderGroup::Color::left);
      add("border-width", new BorderWidthPropDef());
      add("border-style", new BorderStylePropDef());
      add("border-color", new BorderColorPropDef());
      add("border",       new BorderPropDef());
      add(&TextGroup::color);
      add(&FontGroup::style);
      add(&FontGroup::variant);
      add(&FontGroup::weight);
      add(&FontGroup::size);
      add(&FontGroup::line_height);
      add2<MarginGroup>(&MarginGroup::top);
      add2<MarginGroup>(&MarginGroup::right);
      add2<MarginGroup>(&MarginGroup::bottom);
      add2<MarginGroup>(&MarginGroup::left);
      add2<PaddingGroup>(&PaddingGroup::top);
      add2<PaddingGroup>(&PaddingGroup::right);
      add2<PaddingGroup>(&PaddingGroup::bottom);
      add2<PaddingGroup>(&PaddingGroup::left);
      add(&SizeGroup::width);
      add(&SizeGroup::height);
    }
  }





  namespace HtmlImpl
  {
    typedef StyleImpl::EvaluatedStyle::Length length_type;
    typedef StyleImpl::EvaluatedStyle::Color color_type;


    /*

      An element must be re-flowed (have its contents layed out anew) if:
        1> Its css width value is changed
        2> If its list of children is changed
        3> If any child changes size.

      A child is condiered to change size, if:
        2> Any of the following CSS properties are changed: all visual properties except background-*, color, outline-*, visibility, z-index
        3> If the computed css width or height value is auto, and any of its children have their size changed.

      Levels of box reuse:
        1 - full reuse
        2 - rerender (no reflowing)
        3 - reflow (heighest cost)


      Element
        UniquePtr<Box> box;
        BlockBox *make_block_box()
        {
          BlockBox *box;
          For each child:
            If text: add text to formatter
            If inline:
              child->make_inline_box()
            If block:
              new_box = new BlockContainer
              if formatter is non-empty, flush it, and produce an InlineContainer and add if to new_box
              child_box = child->make_block_box()
              add child_box to new_box
          If formatter is non-empty:
            box = new InlineContainer produced from formatter
        }

        BlockBox *make_inline_box()
        {
          for each child:
            If text: add text to formatter
            If inline: child->make_inline_box()
            If block:
              new_box = new BlockContainer
              if formatter is non-empty, flush it, and produce an InlineContainer and add if to new_box
              child_box = child->make_block_box()
              add child_box to new_box
        }


      Box
        int get_offset_top()
        int get_offset_left()
        int get_offset_height()
        int get_offset_width()

      BlockBox: Box
        BlockBox *next_sibling
        render()

      BlockContainer: BlockBox
        BlockBox *first_child

      InlineContainer: BlockBox
        InlineBox *first_child


      Root box is always a BlockBox.

    */


    struct Box
    {
      virtual ~Box() {}
    };


    struct BlockBox: Box
    {
//      virtual int get_min_width()  const = 0;
//      virtual int get_min_height() const = 0;
      // (x,y) is top right corner of box
      virtual void render(int x, int y) const = 0;
    protected:
      BlockBox *next_sibling;
      int width, height;
    };


    struct BlockContainer: BlockBox
    {
      template<class T> void add_child(UniquePtr<T> &child)
      {
        int const w = child->get_min_width();
        int const h = child->get_min_height();
        children.push_back(child);
        if (min_width < w) min_width = w;
        min_height += h;
      }

      void render(int x, int y) const
      {
        if (background && 0 < background->color[3]) {
          int const x1 = x, x2 = x + width;
          int const y1 = y - height, y2 = y;
          color_type const &color = background->color;
          glColor4f(color[0], color[1], color[2], color[3]);
          glBegin(GL_QUADS);
          glVertex2i(x1, y1);
          glVertex2i(x2, y1);
          glVertex2i(x2, y2);
          glVertex2i(x1, y2);
          glEnd();
        }

        BlockBox *child = first_child;
        if (!child) return;
        for (;;) {
          child->render(x,y);
          BlockBox *const next_child = child->next_sibling;
          if (!next_child) break;
          y += child->height;
          child = next_child;
        }
      }

      ~BlockContainer()
      {
        BlockBox *child = first_child;
        while (child) {
          BlockBox *const next_child = child->next_sibling;
          delete child;
          child = next_child;
        }
      }

    private:
      BlockBox *first_child;
    };


/*
    struct Background
    {
      color_type const color;

      Background(color_type const &c): color(c) {}
    };



    struct Block: ParentBox
    {
      void render(int x, int y, int width) const
      {
        ParentBox::render(x, y, width);
      }

      UniquePtr<Background> background;
    };
*/



    struct FlowBox: Box
    {
      FlowBox(TextFormatter &formatter)
      {
        formatter.format(layout);
      }

      int get_min_width()  const { return ceil(layout.get_width());  }
      int get_min_height() const { return ceil(layout.get_height()); }

      void render(int x, int y, int) const
      {
        y -= ceil(layout.get_height());
        glPushMatrix();
        glTranslatef(x,y,0);
        layout.render();
        glPopMatrix();
      }

    private:
      TextLayout layout;
    };



    /*

      NOTE: window.defaultView.getComputedStyle() gives access to 'used' values, not computed values. Here a length is reported in pixels whenever possible.

      'display' needs special treatment - see http://www.w3.org/TR/CSS21/visuren.html#dis-pos-flo



      Start condition seems to be that we have an available width which is equal to the width of the view port. Within this width the root element must be rendered.

      Consider when there is an inline-level element at the root.
      Incorporate the fact that 'width-of-containg-block' is zero from the point of view of CSS if it is unspecified.

      max_width = width of viewport

      max_root_contents_width = calculate max contents width of root box
        if width is set, it counts
        otherwise width is max_parent_contents_width minus margins, borders, and paddings and any auto value of a margin counts as zero.

      set text rendering width = max_root_contents_width

      start rendering text nodes

      inline elements apply their style to the text formatter, but line boxes are also generated with background, border, marging, and padding.

      if a block level element is encountered, switch to block formatting context, which is no more complicated than simply ending the currently running line formatting context, and start a new one. In fact this involves a recursive call that calculates a new maximum contents width for the nested block.

      when all children of the root box have been rendered, the maximum used with among the child block-level boxes is the 'shrink-to-fit' width of the root box.

      the 'shrink-to-fit' height of the root box is simply the sum of the heights of the child block-level boxes.

      Only at the second traversal, the traversal of the generated boxes, is the actual widths of the block-level boxes known, that is, if they have no definite width, they must be expanded to fill the available width within their parent.


      When the actual width is known during second traversal:
        render background and border - background is beneath border where border is transparent, for root element, background is extended to fill entire viewport.
        properly position the boxes within it and then render them: alignment (eg. centering)


        Then:
         verify that line box stacking in text formatter is in agreement with CSS.
         somehow extent the rendering of layed out text such that boxes can be rendered independantly underneeth the individual glyph stretches.
         somehow extend the text rendering system to allow super and sub scripting, that is allow for the baseline to be shifted.
         somehow extend the text rendering system to be able to handle vertical alignment of adjacent inline boxes in the various  different ways that CSS allows.

    */



    struct BoxGenContext: StyleImpl::ComputeContext
    {
      TextFormatter &formatter;
      bool have_text;
      BoxGenContext(StyleImpl::Document &d, TextFormatter &f):
        StyleImpl::ComputeContext(d), formatter(f), have_text(false) {}

      double determine_current_height_of_x()
      {
        return 8; // FIXME: Implement this!
      }

      void flush_formatter(ParentBox &parent_box)
      {
        if (have_text) {
          UniquePtr<FlowBox> box(new FlowBox(formatter));
          formatter.clear();
          have_text = false;
          parent_box.add_child(box);
        }
      }

      void init_formatter(int max_avail_width)
      {
        StyleImpl::EvaluatedStyle const &style = get_current_style();
        set_font(style.font);
        set_text(style.text);
        formatter.set_page_width(Interval(0, max_avail_width));
      }

      void set_font(StyleImpl::EvaluatedStyle::Font const &font)
      {
cerr << "FONT(style="<<StyleImpl::FontStyleEnum(font.style)<<", weight="<<StyleImpl::FontWeightEnum(font.weight)<<", size="<<font.size<<")" << endl;
        int weight;
        switch (font.weight) {
        case StyleImpl::fontWeight_100: weight = 100; break;
        case StyleImpl::fontWeight_200: weight = 200; break;
        case StyleImpl::fontWeight_300: weight = 300; break;
        default:                        weight = 400; break;
        case StyleImpl::fontWeight_500: weight = 500; break;
        case StyleImpl::fontWeight_600: weight = 600; break;
        case StyleImpl::fontWeight_700: weight = 700; break;
        case StyleImpl::fontWeight_800: weight = 800; break;
        case StyleImpl::fontWeight_900: weight = 900; break;
        }
        formatter.set_font_boldness((weight - 400) / 300.0);
        if (font.variant != StyleImpl::fontVariant_Normal)
          throw runtime_error("Small-caps fonts are not yet supported");
        formatter.set_font_italicity(font.style == StyleImpl::fontStyle_Normal ? 0 : 1);
        formatter.set_font_size(font.size);
      }

      void set_text(StyleImpl::EvaluatedStyle::Text const &text)
      {
        formatter.set_text_color(text.color);
        // FIXME: Remember 'line-height'!
      }
    };



    struct BlockStyleApplyee: StyleImpl::Applyee
    {
      BlockStyleApplyee(Block &b, BoxGenContext &c, BlockStyleApplyee *parent):
        StyleImpl::Applyee(c, parent), block(b) {}

      BoxGenContext &get_context() const throw ()
      {
        return static_cast<BoxGenContext &>(StyleImpl::Applyee::get_context());
      }

      void set_font(StyleImpl::EvaluatedStyle::Font const &font)
      {
        get_context().set_font(font);
      }

      void set_text(StyleImpl::EvaluatedStyle::Text const &text)
      {
        get_context().set_text(text);
      }

      void set_background(StyleImpl::EvaluatedStyle::Background const &b)
      {
        cerr << "Background" << endl;
        if (b.color[3] <= 0) return;
        block.background.reset(new Background(b.color));
      }

      void set_border(StyleImpl::EvaluatedStyle::Border const &border)
      {
        cerr << "Border (left width = "<<border.left.width<<")" << endl; // FIXME: Implement this!
      }

      void set_margin(StyleImpl::EvaluatedStyle::Margin const &margin)
      {
        cerr << "Margin (left = "<<margin.left.state<<","<<margin.left.value<<")" << endl; // FIXME: Implement this!
      }

      void set_padding(StyleImpl::EvaluatedStyle::Padding const &padding)
      {
        cerr << "Padding (left = "<<padding.left.state<<","<<padding.left.value<<")" << endl; // FIXME: Implement this!
      }

      void set_size(StyleImpl::EvaluatedStyle::Size const &size)
      {
        cerr << "Size (width = "<<size.width.state<<","<<size.width.value<<")" << endl; // FIXME: Implement this!
      }

      Block &block;
    };



    struct ElementBase: virtual dom::html::HTMLHtmlElement, StyleImpl::Element
    {
      void apply_default_font_to(StyleImpl::Applyee &) const {}
      void apply_default_style_to(StyleImpl::Applyee &) const {}

      virtual void generate_elem_boxes(int max_avail_width, ParentBox &parent_box,
                                       BoxGenContext &context,
                                       BlockStyleApplyee *parent_applyee) const = 0;

      ElementBase(StyleImpl::Document *doc): StyleImpl::Element(doc) {}
    };



    struct DocumentBase: virtual dom::html::HTMLDocument, StyleImpl::Document
    {
      // If this method has not been called since a specific point in
      // time, then the body element of this document have not changed
      // since that point in time. Overriding methods must call this
      // one.
      virtual void on_body_elem_changed() throw () {}

      UniquePtr<ParentBox> generate_root_box(TextFormatter &formatter, int max_avail_width)
      {
        UniquePtr<ParentBox> root_block;
        if (ElementBase *const root_elem = dynamic_cast<ElementBase *>(get_root_elem())) {
          root_block.reset(new ParentBox());
          BoxGenContext context(*this, formatter);
          context.init_formatter(max_avail_width);
          root_elem->generate_elem_boxes(max_avail_width, *root_block, context, 0);
          context.flush_formatter(*root_block);
        }
        return root_block;
      }

      DocumentBase(StyleImpl::StaticInfo const &info, locale const &loc, double dpcm,
                   PackedTRGB::CssLevel css_level):
        StyleImpl::Document(info, loc, dpcm, css_level) {}
    };



    struct StaticInfo: StyleImpl::StaticInfo
    {
      static StaticInfo const &get()
      {
        static StaticInfo d;
        return d;
      }

      StaticInfo();

      typedef ElementBase *(*ElemCreator)(DocumentBase *);
      struct StandardElement
      {
        ElemCreator const creator;
        StandardElement(ElemCreator c): creator(c) {}
      };
      typedef std::map<string, StandardElement> StandardElements; // key is upper case tag name
      StandardElements standard_elements;
    };



    template<bool is_inline> struct Element: ElementBase
    {
      DocumentBase *get_document() const throw ()
      {
        return static_cast<DocumentBase *>(ElementBase::get_document());
      }

      Element(DocumentBase *doc): ElementBase(doc) {}

    private:
      void generate_elem_boxes(int max_avail_width, ParentBox &parent_box,
                               BoxGenContext &context, BlockStyleApplyee *parent_applyee) const
      {
//       Vec3F orig_fgcolor;
//       bool reset_fgcolor = false;
//       if (Vec4F const *c = get_fgcolor()) {
//         if ((*c)[3] < 1) throw runtime_error("Translucent text color is not yet supported");
//         orig_fgcolor = context.formatter->get_text_color();
//         context.formatter->set_text_color(c->slice<3>());
//         reset_fgcolor = true;
//       }

//      Vec4F bgcolor(1,1,1,0);
//      if (Vec4F const *c = get_bgcolor()) if (0 < (*c)[3]) bgcolor = *c;

        if (is_inline) {
          cerr << "Inline\n";
          generate_children_boxes(max_avail_width, parent_box, context, parent_applyee);
        }
        else {
          cerr << "Block\n";
          context.flush_formatter(parent_box);
          UniquePtr<Block> block(new Block);
          generate_children_boxes(max_avail_width, *block, context, parent_applyee);
          parent_box.add_child(block);
        }

//      if (reset_fgcolor) context.formatter->set_text_color(orig_fgcolor);
      }

      void generate_children_boxes(int max_avail_width, ParentBox &parent_box,
                                   BoxGenContext &context, BlockStyleApplyee *parent_applyee) const
      {
        BlockStyleApplyee applyee(parent_box, context, parent_applyee);
        apply_style_to(applyee);
        typedef Children::const_iterator iter;
        iter const end = children_end();
        for (iter i=children_begin(); i!=end; ++i) {
          if (DomImpl::TextImpl const *t = dynamic_cast<DomImpl::TextImpl const *>(&*i)) {
            context.formatter.write(get_document()->decode_lenient(t->data));
            context.have_text = true;
          }
          else if (ElementBase const *e = dynamic_cast<ElementBase const *>(&*i)) {
            e->generate_elem_boxes(max_avail_width, parent_box, context, parent_applyee);
          }
          else throw runtime_error("Unexpected type of child node");
        }
        context.flush_formatter(parent_box);
        applyee.revert();
      }

//       // Apply style to formatter if necessary
//       typedef Children::const_iterator iter;
//       iter const end = children.end();
//       for (iter i=children.begin(); i!=end; ++i)
//         if (TextNodeImpl *t = dynamic_cast<TextNodeImpl *>(*i)) {
//           if (!t->data.empty()) {
//             context.formatter->write(owner_document->string_codec.decode(t->data, L'\uFFFD'));
//             context.have_text = true;
//           }
//         }
//         else if (LayoutElement *e = dynamic_cast<LayoutElement *>(*i)) {
//           if (Vec4F const *c = e->get_bgcolor()) {
//             if (0 < (*c)[3]) {
//               orig_bgcolor = context.bgcolor;
//               context.bgcolor = *c;
//               have_orig_bgcolor = true;
//             }
//           }
//           if (e->is_inline) {
//             e->generate_boxes(max_avail_width, parent, context);
//           }
//           else {
//             flush_formatter(parent, context);
//             // Set new page width of formatter if necessary
//             UniquePtr<Block> block(new Block(context.bgcolor));
//             e->generate_boxes(max_avail_width, block.get(), context); // Use new width when necessary
//             flush_formatter(block.get(), context);
//             // Revert page width of formatter if necessary
//             parent->add_child(block);
//           }
//         }
//         else throw runtime_error("Unexpected type of child node");
//       // Revert style if necessary
    };


    typedef Element<true> InlineElement;
    typedef Element<false> BlockElement;


    // FIXME: Introduce an ElemSpec_XXX in the spirit of StyleImpl::PropSpec_YYY.

    struct Element_Bold: InlineElement
    {
      dom::DOMString getTagName() throw () { return get_manip_context().encode_narrow("B"); }

      void apply_default_font_to(StyleImpl::Applyee &applyee) const
      {
        applyee.set<StyleImpl::PropSpec_FontWeight>(StyleImpl::fontWeight_700);
      }

      Element_Bold(DocumentBase *d): InlineElement(d) {}
    };

    struct Element_Italic: InlineElement
    {
      dom::DOMString getTagName() throw () { return get_manip_context().encode_narrow("I"); }

      void apply_default_font_to(StyleImpl::Applyee &applyee) const
      {
        applyee.set<StyleImpl::PropSpec_FontStyle>(StyleImpl::fontStyle_Italic);
      }

      Element_Italic(DocumentBase *d): InlineElement(d) {}
    };

    struct Element_HTML: virtual dom::html::HTMLHtmlElement, BlockElement
    {
      dom::DOMString getTagName() throw () { return get_manip_context().encode_narrow("HTML"); }
      Element_HTML(DocumentBase *d): BlockElement(d) {}
    };

    struct Element_BODY: virtual dom::html::HTMLBodyElement, BlockElement
    {
      void on_parent_changed() throw ()
      {
        BlockElement::on_parent_changed();
        DocumentBase *const doc = get_document();
        DomImpl::NodeImpl *const parent = get_parent();
        if (parent && parent->get_parent() == doc) doc->on_body_elem_changed();
      }

      dom::DOMString getTagName() throw () { return get_manip_context().encode_narrow("BODY"); }
      Element_BODY(DocumentBase *d): BlockElement(d) {}
    };

    struct Element_DIV: virtual dom::html::HTMLDivElement, BlockElement
    {
      dom::DOMString getTagName() throw () { return get_manip_context().encode_narrow("DIV"); }
      Element_DIV(DocumentBase *d): BlockElement(d) {}
    };

    struct Element_Paragraph: virtual dom::html::HTMLParagraphElement, BlockElement
    {
      dom::DOMString getTagName() throw () { return get_manip_context().encode_narrow("P"); }

      void apply_default_style_to(StyleImpl::Applyee &applyee) const
      {
        double const value = 1.12 * applyee.get_current_font_size();
        typedef StyleImpl::EvaluatedStyle::AugmentedLength AugmentedLength;
        AugmentedLength const l(AugmentedLength::state_Abs, value);
        applyee.set<StyleImpl::PropSpec_MarginTop>(l);
        applyee.set<StyleImpl::PropSpec_MarginBottom>(l);
      }

      Element_Paragraph(DocumentBase *d): BlockElement(d) {}
    };

    struct UnknownElement: InlineElement
    {
      dom::DOMString getTagName() throw () { return get_manip_context().encode_narrow(tag_name); }
      UnknownElement(DocumentBase *d, string const &t): InlineElement(d), tag_name(t) {}

    private:
      string tag_name;
    };

    struct WideUnknownElement: InlineElement
    {
      dom::DOMString getTagName() throw () { return get_manip_context().encode_wide(tag_name); }
      WideUnknownElement(DocumentBase *d, wstring const &t):
        InlineElement(d), tag_name(t) {}

    private:
      wstring tag_name;
    };



    struct Document: DocumentBase
    {
      dom::html::HTMLElement *getBody() throw () { return get_body_elem(); }

      void setBody(dom::html::HTMLElement *new_body) throw ()
      {
        DomImpl::NodeImpl *new_body2;
        DomImpl::ElementImpl *root = get_root_elem();
        if (root) new_body2 = root->validate_new_child(new_body);
        else {
          root = add_root_elem(new Element_HTML(this));
          try {
            new_body2 = root->validate_new_child(new_body);
          }
          catch (...) {
            delete remove_root_elem();
            throw;
          }
        }
        new_body2->release_from_owner();
        if (Element_BODY *const old_body = get_body_elem())
          root->replace_child(new_body2, old_body);
        else root->append_child(new_body2);
      }

      Element_BODY *get_body_elem()
      {
        if (dirty_body_elem) find_body_elem();
        return body_elem;
      }

      void on_body_elem_changed() throw ()
      {
        DocumentBase::on_body_elem_changed();
        dirty_body_elem = true;
      }

      void on_root_elem_changed() throw ()
      {
        DocumentBase::on_root_elem_changed();
        dirty_body_elem = true;
      }

      StaticInfo const &get_static_info() const
      {
        return static_cast<StaticInfo const &>(DocumentBase::get_static_info());
      }

      ElementBase *create_element(dom::DOMString const &tag_name)
      {
        string tag_name2;
        if (decode_narrow(tag_name, tag_name2)) {
          StaticInfo::StandardElements const &standard_elements =
            get_static_info().standard_elements;
          StaticInfo::StandardElements::const_iterator const i =
            standard_elements.find(toupper(tag_name2));
          if (i != standard_elements.end()) return (*i->second.creator)(this);
          return new UnknownElement(this, tag_name2);
        }

        // Check name according to rules in XML1.0 specification
        typedef dom::DOMString::traits_type traits;
        typedef dom::DOMString::const_iterator iter;
        iter const begin = tag_name.begin(), end = tag_name.end();
        for (iter i=begin; i!=end; ++i) {
          unsigned const v = traits::to_int_type(*i);
          if (v < 0xC0) { // 0x0 <= v < 0xC0
            if (v < 0x5B) { // 0x0 <= v < 0x5B
              if (v < 0x41) { // 0x0 <= v < 0x41
                if (v < 0x30) { // 0x0 <= v < 0x30
                  if (v != 0x2D && v != 0x2E) return 0;
                  // '-' or '.'  -->  good unless first char!
                  if (i == begin) return 0;
                }
                else if (0x3A <= v) { // 0x3A <= v < 0x41
                  if (v != 0x3A) return 0;
                  // ':'  -->  good!
                }
                else { // '0' <= v <= '9'  -->  good unless first char!
                  if (i == begin) return 0;
                }
              }
              else {} // 'A' <= v <= 'Z'  -->  good!
            }
            else if (v < 0x7B) { // 0x5B <= v < 0x7B
              if (v < 0x61) { // 0x5B <= v < 0x61
                if (v != 0x5F) return 0;
                // '_'  -->  good!
              }
              else {} // 'a' <= v <= 'z'  -->  good!
            }
            else { // 0x7B <= v < 0xC0
              if (v != 0xB7) return 0;
              // 0xB7  -->  good unless first char!
              if (i == begin) return 0;
            }
          }
          else if (v <= 0x3000) { // 0xC0 <= v <= 0x3000
            if (v < 0x2000) { // 0xC0 <= v < 0x2000
              if (v <= 0x37E) { // 0xC0 <= v <= 0x37E
                if (v < 0x300) { // 0xC0 <= v < 0x300
                  if (v <= 0xF7) { // 0xC0 <= v <= 0xF7
                    if (v == 0xD7 || v == 0xF7) return 0;
                    // 0xC0 <= v < 0xD7  or  0xD7 < v < 0xF7  -->  good!
                  }
                  else {} // 0xF7 < v < 0x300  -->  good!
                }
                else { // 0x300 <= v <= 0x37E
                  if (v < 0x370) { // 0x300 <= v < 0x370  -->  good unless first char!
                    if (i == begin) return 0;
                  }
                  else { // 0x370 <= v <= 0x37E
                    if (v == 0x37E) return 0;
                    // 0x370 <= v < 0x37E  -->  good!
                  }
                }
              }
              else {} // 0x37E < v < 0x2000  -->  good!
            }
            else { // 0x2000 <= v <= 0x3000
              if (v < 0x2190) { // 0x2000 <= v < 0x2190
                if (v < 0x2070) { // 0x2000 <= v < 0x2070
                  if (v <= 0x203E) { // 0x2000 <= v <= 0x203E
                    if (v < 0x200E) { // 0x2000 <= v < 0x200E
                      if (v <= 0x200B) return 0; // 0x2000 <= v <= 0x200B  -->  bad!
                      // 0x200B < v < 0x200E  -->  good!
                    }
                    else return 0; // 0x200E <= v <= 0x203E  -->  bad!
                  }
                  else { // 0x203E < v < 0x2070
                    if (v <= 0x2040) { // 0x203E < v <= 0x2040  -->  good unless first char!
                      if (i == begin) return 0;
                    }
                    else return 0; // 0x2040 < v < 0x2070  -->  bad!
                  }
                }
                else {} // 0x2070 <= v < 0x2190  -->  good!
              }
              else { // 0x2190 <= v <= 0x3000
                if (v < 0x2C00) return 0; // 0x2190 <= v < 0x2C00  -->  bad!
                if (0x2FF0 <= v) return 0; // 0x2FF0 <= v <= 0x3000  -->  bad!
                // 0x2C00 <= v < 0x2FF0  -->  good!
              }
            }
          }
          else { // 0x3000 < v <= 0xFFFF
            if (0xD800 <= v) { // 0xD800 <= v <= 0xFFFF
              if (v < 0xDC00) { // 0xD800 <= v < 0xDC00
                // Combine UTF-16 surrogates
                if (++i == end) return 0; // Incomplete surrogate pair
                unsigned const v2 = traits::to_int_type(*i);
                if (v2 < 0xDC00 || 0xE000 <= v2) return 0; // Invalid high surrogate
                UIntFast32 w = 0x10000 + (UIntFast32(v-0xD800)<<10) + (v2-0xDC00);
                if (0xF0000 <= w) return 0; // 0xF0000 <= w  -->  bad!
                // 0x10000 <= w < 0xF0000  -->  good!
              }
              else { // 0xDC00 <= v <= 0xFFFF
                if (v < 0xFDD0) { // 0xDC00 <= v < 0xFDD0
                  if (v < 0xF900) return 0; // 0xDC00 <= v < 0xF900  -->  bad!
                  // 0xF900 <= v < 0xFDD0  -->  good!
                }
                else { // 0xFDD0 <= v <= 0xFFFF
                  if (v < 0xFDF0) return 0; // 0xFDD0 <= v < 0xFDF0  -->  bad!
                  if (0xFFFE <= v) return 0; // 0xFFFE or 0xFFFF  -->  bad!
                  // 0xFDF0 <= v < 0xFFFE  -->  good!
                }
              }
            }
            else {} // 0x3000 < v < 0xD800  -->  good!
          }
        }

        wstring tag_name3;
        if (!decode_wide(tag_name, tag_name3)) return 0;
        return new WideUnknownElement(this, tag_name3);
      }

      Document(locale const &loc, double dpcm, PackedTRGB::CssLevel css_level):
        DocumentBase(StaticInfo::get(), loc, dpcm, css_level), body_elem(0), dirty_body_elem(false)
      {
        DomImpl::ElementImpl *const root = add_root_elem(new Element_HTML(this));
        root->append_child(new Element_BODY(this));
      }

    private:
      void find_body_elem()
      {
        Element_BODY *body;
        if (DomImpl::ElementImpl *const root = get_root_elem()) {
          typedef DomImpl::ElementImpl::child_iterator iter;
          iter const end = root->children_end();
          for (iter i=root->children_begin(); i!=end; ++i) {
            if (Element_BODY *b = dynamic_cast<Element_BODY *>(&*i)) {
              body = b;
              break;
            }
          }
        }
        body_elem = body;
        dirty_body_elem = false;
      }

      Element_BODY *body_elem;
      bool dirty_body_elem;
    };



    template<class T> ElementBase *create_elem(DocumentBase *doc) { return new T(doc); }

    StaticInfo::StaticInfo()
    {
      typedef pair<string, StandardElement> Pair;
      standard_elements.insert(Pair("B",    &create_elem<Element_Bold>));
      standard_elements.insert(Pair("I",    &create_elem<Element_Italic>));
      standard_elements.insert(Pair("HTML", &create_elem<Element_HTML>));
      standard_elements.insert(Pair("BODY", &create_elem<Element_BODY>));
      standard_elements.insert(Pair("DIV",  &create_elem<Element_DIV>));
      standard_elements.insert(Pair("P",    &create_elem<Element_Paragraph>));
    }
  }











  double const zoom_step = pow(2, 1.0 / 8); // 8 steps to double
  double const zoom_min  = 0.1;
  double const zoom_max  = 32;

  double const camera_dist_step = pow(2, 1.0 / 8); // 8 steps to double

  long const status_hud_linger_millis = 1000;



  struct DialogImpl;


  struct PrivateApplicationState
  {
    void open_help_hud()
    {
      using namespace dom;
      Dialog::Ptr const dialog = new_modal_hud_dialog();
      html::HTMLDocument *const doc = dialog->get_dom();
      html::HTMLElement *const body = doc->getBody();
      css::CSSStyleDeclaration *const body_style =
        dynamic_cast<css::ElementCSSInlineStyle *>(body)->getStyle();
      body_style->setProperty(u16(L"background-color"), u16("white"), u16(""));
      body_style->setProperty(u16("color"), u16("red"), u16(""));
      body_style->setProperty(u16("border"), u16("1px solid lime"), u16(""));
      body->appendChild(doc->createTextNode(u16("Help me!")));
      body->appendChild(doc->createTextNode(u16(" Now!")));
      Element *const elem = doc->createElement(u16("B"));
      css::CSSStyleDeclaration *const elem_style =
        dynamic_cast<css::ElementCSSInlineStyle *>(elem)->getStyle();
      elem_style->setProperty(u16("color"), u16("lime"), u16(""));
      elem_style->setProperty(u16("font-weight"), u16("bolder"), u16(""));
      elem_style->setProperty(u16("font-size"), u16("smaller"), u16(""));
      elem->appendChild(doc->createTextNode(u16(" FISSE :-)")));
      body->appendChild(elem);
      Element *const elem2 = doc->createElement(u16("I"));
      css::CSSStyleDeclaration *const elem2_style =
        dynamic_cast<css::ElementCSSInlineStyle *>(elem2)->getStyle();
      elem2_style->setProperty(u16("color"), u16("purple"), u16(""));
      elem2_style->setProperty(u16("font-style"), u16("italic"), u16(""));
      elem2_style->setProperty(u16("font-size"), u16("larger"), u16(""));
      elem2_style->setProperty(u16("border-left-width"), u16("thick"), u16(""));
      elem2->appendChild(doc->createTextNode(u16("Barnach!?")));
      body->appendChild(elem2);
      dialog->show();
      cerr << "Body style #2: " << narrow_from_u16(body_style->getCssText()) << endl;
      cerr << "Elem style #2: " << narrow_from_u16(elem_style->getCssText()) << endl;
      cerr << "Elem2 style #2: " << narrow_from_u16(elem2_style->getCssText()) << endl;
    }

    Dialog::Ptr new_modal_hud_dialog();

    void open_dialog(SharedPtr<DialogImpl> const &);
    void close_dialog(SharedPtr<DialogImpl> const &);

    bool has_open_dialogs() const { return !open_dialogs.empty(); }

    void render_hud(int viewport_width, int viewport_height);

    void on_resize();

    /**
     * Can by called at any time, also when an OpenGL context is not
     * bound.
     */
    void recycle_display_list(GLuint disp_list)
    {
      available_display_lists.push_back(disp_list);
    }

    SharedPtr<TextureCache> get_texture_cache()
    {
      ensure_texture_cache();
      return texture_cache;
    }

    TextureDecl declare_texture(string image_path, bool repeat, bool mipmap)
    {
      ensure_texture_cache();
      UniquePtr<TextureSource> src(new TextureFileSource(image_path));
      GLenum const wrap = repeat ? GL_REPEAT : GL_CLAMP;
      return texture_cache->declare(src, wrap, wrap, mipmap);
    }

    FontProvider *get_font_provider()
    {
      if (!font_provider) {
        ensure_font_cache();
        ensure_texture_cache();
        font_provider.reset(new FontProvider(font_cache, texture_cache, glyph_resolution,
                                             glyph_mipmapping, save_glyph_textures));
      }
      return font_provider.get();
    }

    TextFormatter &get_text_formatter()
    {
      if (!text_formatter)
        text_formatter.reset(new TextFormatter(get_font_provider()));
      return *text_formatter;
    }

    // Called with bound OpenGL context
    void update()
    {
      typedef vector<GLuint>::iterator iter;
      iter const end = available_display_lists.end();
      for (iter i=available_display_lists.begin(); i!=end; ++i) {
cout << "*";
        glDeleteLists(*i, 1);
      }

      if(texture_cache) texture_cache->update();
    }

    TextLayout status_hud_text_layout;

    PrivateApplicationState(Application::Config const &cfg, locale const &l,
                            TextureCache::Arg tex, FontCache::Arg font):
      resource_dir(cfg.archon_datadir), loc(l), utf16_string_codec(loc), texture_cache(tex),
      font_cache(font), glyph_resolution(cfg.glyph_resol), glyph_mipmapping(cfg.glyph_mipmap),
      save_glyph_textures(cfg.glyph_save) {}

    WeakPtr<PrivateApplicationState> weak_self;

  private:
    typedef list<SharedPtr<DialogImpl> > OpenDialogs;
    OpenDialogs open_dialogs;

    vector<GLuint> available_display_lists;

    StringUtf16 u16(string const &s) const
    {
      StringUtf16 t;
      utf16_string_codec.encode_narrow(s,t);
      return t;
    }

    StringUtf16 u16(wstring const &s) const
    {
      StringUtf16 t;
      if (!utf16_string_codec.encode(s,t))
        throw runtime_error("UTF-16 encode");
      return t;
    }

    string narrow_from_u16(StringUtf16 const &s) const
    {
      string t;
      if (!utf16_string_codec.decode_narrow(s,t))
        throw runtime_error("UTF-16 decode");
      return t;
    }

    void ensure_font_cache()
    {
      if (!font_cache) {
        FontLoader::Ptr const loader = new_font_loader(resource_dir + "font/");
        FontList::Ptr const list = new_font_list(loader);
        font_cache = new_font_cache(list);
      }
    }

    void ensure_texture_cache()
    {
      if(!texture_cache) texture_cache = new_texture_cache();
    }

    string const resource_dir;
    locale const loc;
    CharEnc<CharUtf16> const utf16_string_codec;

    TextureCache::Ptr texture_cache;
    FontCache::Ptr font_cache;
    Vec2F const glyph_resolution;
    bool const glyph_mipmapping, save_glyph_textures;
    UniquePtr<FontProvider> font_provider;
    UniquePtr<TextFormatter> text_formatter;
  };



  struct DialogImpl: Dialog
  {
    DialogImpl(WeakPtr<PrivateApplicationState> const &s):
      state(s), is_open(false), dirty(true), disp_list(0) {}

    ~DialogImpl()
    {
      if (disp_list)
        if (SharedPtr<PrivateApplicationState> s = state.lock())
          s->recycle_display_list(disp_list);
    }

  protected:
    void mark_dirty() { dirty = true; }

    virtual void render(TextFormatter &, int viewport_width, int viewport_height) = 0;

    WeakPtr<PrivateApplicationState> const state;

  private:
    friend class PrivateApplicationState;

    bool is_open;
    bool dirty;

    // Name of the OpenGL display list that renders this HUD dialog,
    // or zero if no list has been created yet.
    GLuint disp_list;
  };



  void PrivateApplicationState::open_dialog(SharedPtr<DialogImpl> const &d)
  {
    if (d->is_open) return;
    open_dialogs.push_back(d);
    d->is_open = true;
  }

  void PrivateApplicationState::close_dialog(SharedPtr<DialogImpl> const &d)
  {
    if (!d->is_open) return;
    open_dialogs.remove(d);
    d->is_open = false;
  }


  void PrivateApplicationState::render_hud(int viewport_width, int viewport_height)
  {
    typedef OpenDialogs::iterator iter;
    iter const end = open_dialogs.end();
    for (iter i=open_dialogs.begin(); i!=end; ++i) {
      DialogImpl *const dlg = i->get();
      if (dlg->dirty) {
         if (!dlg->disp_list) {
           dlg->disp_list = glGenLists(1);
           if (!dlg->disp_list)
             throw runtime_error("Failed to create a new OpenGL display list");
         }

        glNewList(dlg->disp_list, GL_COMPILE_AND_EXECUTE);
        dlg->render(get_text_formatter(), viewport_width, viewport_height);
        glEndList();

        dlg->dirty = false;
      }
      else glCallList(dlg->disp_list);
    }
  }


  void PrivateApplicationState::on_resize()
  {
    typedef OpenDialogs::iterator iter;
    iter const end = open_dialogs.end();
    for (iter i=open_dialogs.begin(); i!=end; ++i) i->get()->dirty = true;
  }









  struct ModalHudDialogImpl: DialogImpl
  {
    void show()
    {
      SharedPtr<DialogImpl> const d(this->weak_self);
      if(SharedPtr<PrivateApplicationState> s = state.lock()) s->open_dialog(d);
    }

    void hide()
    {
      SharedPtr<DialogImpl> const d(this->weak_self);
      if(SharedPtr<PrivateApplicationState> s = state.lock()) s->close_dialog(d);
    }

    dom::html::HTMLDocument *get_dom() { return dom.get(); }

    static SharedPtr<ModalHudDialogImpl> create(WeakPtr<PrivateApplicationState> const &s,
                                                locale const &loc, double dpcm)
    {
      SharedPtr<ModalHudDialogImpl> const d(new ModalHudDialogImpl(s, loc, dpcm));
      d->weak_self = d;
      return d;
    }

  private:
    HtmlImpl::Document *create_dom(locale const &, double dpcm, PackedTRGB::CssLevel css_level);

    void render(TextFormatter &, int viewport_width, int viewport_height);

    ModalHudDialogImpl(WeakPtr<PrivateApplicationState> const &s, locale const &loc, double dpcm):
      DialogImpl(s), dom(create_dom(loc, dpcm, PackedTRGB::css3)) {}

    UniquePtr<HtmlImpl::Document> const dom;
    WeakPtr<ModalHudDialogImpl> weak_self;
  };



  HtmlImpl::Document *ModalHudDialogImpl::create_dom(locale const &loc, double dpcm,
                                                     PackedTRGB::CssLevel css_level)
  {
    return new HtmlImpl::Document(loc, dpcm, css_level);
  }



  void ModalHudDialogImpl::render(TextFormatter &formatter,
                                  int viewport_width, int viewport_height)
  {
    UniquePtr<HtmlImpl::ParentBox>
      root_block(dom->generate_root_box(formatter, viewport_width).release());
    if (root_block) {
      int const width  = root_block->get_min_width();
      int const height = root_block->get_min_height();
      int const x = (viewport_width  - width)      / 2;
      int const y = (viewport_height + height + 1) / 2;
      root_block->render(x, y, width);
    }
  }



  Dialog::Ptr PrivateApplicationState::new_modal_hud_dialog()
  {
    // FIXME: The calculation below is in accordance with CSS2.1, but
    // we should also support the true value which can be obtained
    // from the display connection.
    double ptpd  = 0.75; // Points per dot  (a dot is the same as a pixel)
    double ptpin = 72;   // Points per inch
    double cmpin = 2.54; // Centimeters per inch
    double dpcm = ptpin / cmpin / ptpd; // Dots per centimeter
    return ModalHudDialogImpl::create(this->weak_self, loc, dpcm);
  }
}





namespace archon
{
  namespace render
  {
    struct Application::PrivateState: PrivateApplicationState
    {
      static archon::core::SharedPtr<PrivateState>
      create(Config const &cfg, locale const &loc,
             TextureCache::Arg tex_cache, FontCache::Arg font_cache)
      {
        SharedPtr<PrivateState> const s(new PrivateState(cfg, loc, tex_cache, font_cache));
        s->weak_self = s;
        return s;
      }

    private:
      PrivateState(Config const &cfg, locale const &loc,
                   TextureCache::Arg tex_cache, FontCache::Arg font_cache):
        PrivateApplicationState(cfg, loc, tex_cache, font_cache) {}
    };


    void Application::set_window_size(int w, int h)
    {
      win->set_size(w,h);
    }


    void Application::set_window_pos(int x, int y)
    {
      win->set_position(x,y);
      win_x = x;
      win_y = y;
      win_pos_set = true;
    }


    void Application::set_fullscreen_enabled(bool enable)
    {
      fullscreen_mode = enable;
      win->set_fullscreen_enabled(enable);
    }


    void Application::set_headlight_enabled(bool enable)
    {
      headlight = enable;
    }


    void Application::set_frame_rate(double r)
    {
      frame_rate = r;
      time_per_frame.set_as_seconds_float(1/frame_rate);
//      cout << "Setting desired frame rate (f/s): " << frame_rate << endl;
    }


    void Application::set_scene_orientation(math::Rotation3 rot)
    {
      trackball.set_orientation(rot);
    }


    void Application::set_scene_spin(math::Rotation3 rot)
    {
      trackball.set_spin(rot, Time::now());
    }


    void Application::set_detail_level(double level)
    {
      detail_level = level;
    }


    void Application::set_interest_size(double diameter)
    {
      interest_size = diameter;
      projection_needs_update = true;
    }


    void Application::set_zoom_factor(double zoom)
    {
      proj.zoom_factor = clamp(zoom, zoom_min, zoom_max);
      projection_needs_update = true;
    }


    void Application::set_eye_screen_dist(double dist)
    {
      proj.view_dist = dist;
      projection_needs_update = true;
    }


    void Application::set_screen_dpcm(double horiz, double vert)
    {
      if(0 < horiz) proj.horiz_dot_pitch = 0.01 / horiz;
      if(0 < vert)  proj.vert_dot_pitch  = 0.01 / vert;
      if(0 < horiz || 0 < vert) projection_needs_update = true;
/*
      cout << "Horizontal dot pitch = " << proj.horiz_dot_pitch << " m/px  (" << proj.get_horiz_resol_dpi() << " dpi)" << endl;
      cout << "Vertical dot pitch   = " << proj.vert_dot_pitch  << " m/px  (" << proj.get_vert_resol_dpi()  << " dpi)" << endl;
*/
    }


    void Application::set_depth_of_field(double ratio)
    {
      proj.far_to_near_clip_ratio = ratio;
      projection_needs_update = true;
    }


    void Application::set_wireframe_enabled(bool enable)
    {
      wireframe_mode = enable;
    }


    void Application::set_axes_display_enabled(bool enable)
    {
      axes_display = enable;
    }


    void Application::set_global_ambience(double intencity)
    {
      global_ambience = intencity;
      need_misc_update = true;
    }


    void Application::set_background_color(Vec4 rgba)
    {
      background_color = rgba;
      need_misc_update = true;
    }



    void Application::run()
    {
      if(first_run)
      {
        initial_rotation = trackball.get_orientation(Time::now());
        initial_interest_size = interest_size;
        initial_zoom_factor = proj.zoom_factor;
        first_run = false;
      }

      win->show();
      if(win_pos_set || fullscreen_mode)
      {
        conn->flush_output();
        if(win_pos_set) win->set_position(win_x, win_y);
        if(fullscreen_mode) win->set_fullscreen_enabled(true);
      }

//      RateMeter rate_meter("Frame rate (f/s): ", 10000);
      bool lagging_frames = false;

      {
        Bind bind(ctx, win);
        Time time = Time::now();
        for(;;)
        {
//          rate_meter.tick();

          if(need_misc_update)
          {
            GLfloat params[] = { global_ambience, global_ambience, global_ambience, 1 };
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params);
            glClearColor(background_color[0], background_color[1],
                         background_color[2], background_color[3]);
            need_misc_update = false;
          }

          if(projection_needs_update)
          {
            if(!gl_initialized)
            {
              init_gl();
              gl_initialized = true;
            }

            update_gl_projection();
            projection_needs_update = false;
          }

          // The distance is not known acurately until
          // update_gl_projection() has been called.
          if(status_hud_activate_cam_dist)
          {
            set_float_status(L"DIST = ", proj.camera_dist, 2, L"", status_hud_activate_cam_dist_timeout);
            status_hud_activate_cam_dist = false;
          }

          render_frame(time);
          win->swap_buffers(); // Implies glFlush

          private_state->update();

          while(0 < max_gl_errors)
          {
            GLenum const error = glGetError();
            if(!error) break;
            emit_gl_error(error, --max_gl_errors == 0);
          }

          time += time_per_frame;

          Time const now = Time::now();
          if(time < now)
          {
            time = now;
            if(!lagging_frames)
            {
//              cout << "Lagging frames" << endl;
              lagging_frames = true;
            }
          }
          else lagging_frames = false;

          try
          {
            event_proc->process(time);
          }
          catch(InterruptException &)
          {
            if(terminate) break;
            time = Time::now();
          }
        }
      }

      win_pos_set = false;
      win->hide();
    }



    /**
     * \todo FIXME: Lacks propper transcoding from ISO Latin 1 to the
     * wide character encoding of 'wcout'. Not so easy though, because
     * there seems to be no way to query for the encoding of 'wcout'.
     *
     * \todo FIXME: Emit to an abstract logger instead.
     */
    void Application::emit_gl_error(GLenum error, bool last)
    {
#ifdef ARCHON_HAVE_GLU
      GLubyte const *ptr = gluErrorString(error), *end = ptr;
      while(*end) ++end;
      string latin1_str;
      latin1_str.reserve(end - ptr);
      while(ptr < end) latin1_str += char(static_cast<unsigned char>(*ptr++));
      cerr << "OpenGL error: " << latin1_str << endl;
#else
      cerr << "OpenGL error: " << error << endl;
#endif
      if(last) cerr << "No more OpenGL errors will be reported" << endl;
    }



    void Application::get_current_view(math::Vec3 &eye, CoordSystem3x2 &screen)
    {
      update_proj_and_trackball();

      Mat3 rot;
      {
        Rotation3 r = trackball.get_orientation(Time::now());
        r.neg();
        r.get_matrix(rot);
      }
      eye = proj.camera_dist * rot.col(2);

      // Describe the 2-D screen coordinate system relative to the 3-D
      // view coordinate system
      screen.basis.col(0).set(proj.get_near_clip_width(), 0, 0);
      screen.basis.col(1).set(0, proj.get_near_clip_height(), 0);
      screen.origin.set(0, 0, -proj.get_near_clip_dist());
      screen.translate(Vec2(-0.5));

      // Rotate and translate the screen to reflect the actual viewing
      // position and direction direction
      screen.pre_mult(CoordSystem3x3(rot, eye));
    }



    TextureDecl Application::declare_texture(string image_path, bool repeat, bool mipmap)
    {
      return private_state->declare_texture(image_path, repeat, mipmap);
    }


    SharedPtr<TextureCache> Application::get_texture_cache()
    {
      return private_state->get_texture_cache();
    }


    FontProvider *Application::get_font_provider()
    {
      return private_state->get_font_provider();
    }


    Application::Application(string title, Config const &cfg, locale const &loc, Connection::Arg c,
                             TextureCache::Arg tex_cache, FontCache::Arg font_cache):
      conn(c), max_gl_errors(20), win_pos_set(false),
      gl_initialized(false), need_misc_update(false), projection_needs_update(true),
      need_refresh(false), but1_down(false), fullscreen_mode(false),
      headlight(true), headlight_prev(false), headlight_blocked(false),
      wireframe_mode(false), wireframe_mode_prev(false), wireframe_mode_blocked(false),
      axes_display(false), axes_display_first(true), shift_left_down(false),
      terminate(false), first_run(true), quadric(0), one_axis_dpy_list(0),
      status_hud_enabled(true), status_hud_active(false), status_hud_disp_list(0),
      status_hud_dirty(true), status_hud_activate_cam_dist(false),
      private_state(PrivateState::create(cfg, loc, tex_cache, font_cache))
    {
      if(title.empty()) title = "Archon";
      if(!conn) conn = archon::display::get_default_implementation()->new_connection();

      int const vis = conn->choose_gl_visual();

      int const width = cfg.win_size[0], height = cfg.win_size[1];
      win = conn->new_window(width, height, -1, vis);
      win->set_title(title);

      cursor_normal =
        conn->new_cursor(::Image::load(cfg.archon_datadir+"render/viewer_interact.png"),   7,  6);
      cursor_trackball =
        conn->new_cursor(::Image::load(cfg.archon_datadir+"render/viewer_trackball.png"), 14, 14);
      win->set_cursor(cursor_normal);

      event_proc = conn->new_event_processor(this);
      event_proc->register_window(win);

      ctx = conn->new_gl_context(-1, vis, cfg.direct_render);
//      cout << "Direct rendering context: " << (ctx->is_direct() ? "Yes" : "No") << endl;

      set_viewport_size(width, height);

      set_headlight_enabled(cfg.headlight);
      set_frame_rate(cfg.frame_rate);
      if(0 <= cfg.win_pos[0] && 0 <= cfg.win_pos[1])
        set_window_pos(cfg.win_pos[0], cfg.win_pos[1]);
      set_screen_dpcm(cfg.scr_dpcm[0] < 1 ? 0.01 / conn->get_horiz_dot_pitch() : cfg.scr_dpcm[0],
                      cfg.scr_dpcm[1] < 1 ? 0.01 / conn->get_vert_dot_pitch()  : cfg.scr_dpcm[1]);
      set_eye_screen_dist(cfg.eye_scr_dist);
      set_depth_of_field(cfg.depth_of_field);
      set_interest_size(cfg.interest_size);
      set_zoom_factor(cfg.zoom);
      set_detail_level(cfg.detail_level);
      set_fullscreen_enabled(cfg.fullscreen);
      set_global_ambience(cfg.ambience);
      set_background_color(cfg.bgcolor);
    }


    Application::~Application()
    {
      if(one_axis_dpy_list) glDeleteLists(one_axis_dpy_list, 2);
      if(quadric) gluDeleteQuadric(quadric);
    }


    void Application::set_viewport_size(int w, int h)
    {
      viewport_width  = w;
      viewport_height = h;
      projection_needs_update = true;
    }



    void Application::update_gl_projection()
    {
      update_proj_and_trackball();

      double view_plane_dist  = proj.get_near_clip_dist();
      double view_plane_right = proj.get_near_clip_width()  / 2;
      double view_plane_top   = proj.get_near_clip_height() / 2;
      double far_clip_dist    = proj.get_far_clip_dist();

/*
      cout << "Camera distance     = " << proj.camera_dist << " obj" << endl;
      cout << "Near clip distance  = " << view_plane_dist  << " obj" << endl;
      cout << "Far clip distance   = " << far_clip_dist    << " obj" << endl;
*/

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-view_plane_right, view_plane_right,
                -view_plane_top, view_plane_top, view_plane_dist, far_clip_dist);

      glViewport(0, 0, viewport_width, viewport_height);
    }


    void Application::update_proj_and_trackball()
    {
      proj.set_viewport_size_pixels(viewport_width, viewport_height);
      proj.auto_dist(interest_size, proj.get_min_field_factor());
      trackball.set_viewport_size(viewport_width, viewport_height);
    }



    void Application::render_frame(Time now)
    {
      // Handle headlight feature
      if (!headlight_blocked && headlight != headlight_prev) {
        GLboolean params[1];
        glGetBooleanv(GL_LIGHT0, params);
        GLfloat pos_params[4];
        glGetLightfv(GL_LIGHT0, GL_POSITION, pos_params);
        GLfloat pos_on_params[4]  = { 0, 0, 0, 1 };
        GLfloat pos_off_params[4] = { 0, 0, 1, 0 };
        if (params[0] != (headlight_prev ? GL_TRUE : GL_FALSE) ||
            !equal(pos_params, pos_params+4, headlight ? pos_off_params : pos_on_params)) {
          cout << "Warning: Headlight feature blocked due to conflict with application." << endl;
          headlight_blocked = true;
        }
        else {
          // Make the headlight a point light source
          glLightfv(GL_LIGHT0, GL_POSITION, headlight ? pos_on_params : pos_off_params);
          if (headlight) glEnable(GL_LIGHT0);
          else glDisable(GL_LIGHT0);
          headlight_prev = headlight;
        }
      }

      // Handle wireframe mode
      if (!wireframe_mode_blocked && wireframe_mode != wireframe_mode_prev) {
        GLint params[2];
        glGetIntegerv(GL_POLYGON_MODE, params);
        if (wireframe_mode_prev ?
            params[0] != GL_LINE || params[1] != GL_LINE :
            params[0] != GL_FILL || params[1] != GL_FILL) {
          cout << "Warning: Wireframe mode blocked due to conflict with application." << endl;
          wireframe_mode_blocked = true;
        }
        else {
          glPolygonMode(GL_FRONT_AND_BACK, wireframe_mode ? GL_LINE : GL_FILL);
          wireframe_mode_prev = wireframe_mode;
        }
      }

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();

      update_observer(now);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      if (axes_display) {
        if (axes_display_first) {
          axes_display_first = false;
          if (!quadric) {
            quadric = gluNewQuadric();
            if(!quadric) throw bad_alloc();
          }
          one_axis_dpy_list = glGenLists(2);
          all_axes_dpy_list = one_axis_dpy_list+1;
          if (!one_axis_dpy_list) throw runtime_error("glGenLists failed");

          double const back_len = 0.1, head_len = 0.1, shaft_radius = 0.005, head_radius = 0.022;
          int const shaft_slices = adjust_detail(8, 3), head_slices = adjust_detail(16, 3),
            shaft_stacks = adjust_detail(10, 1);

          glNewList(one_axis_dpy_list, GL_COMPILE);
          glTranslated(0, 0, -back_len);
          gluQuadricOrientation(quadric, GLU_INSIDE);
          gluDisk(quadric, 0, shaft_radius, shaft_slices, 1);
          gluQuadricOrientation(quadric, GLU_OUTSIDE);
          gluCylinder(quadric, shaft_radius, shaft_radius, 1, shaft_slices, shaft_stacks);
          glTranslated(0, 0, 1+back_len-head_len);
          gluQuadricOrientation(quadric, GLU_INSIDE);
          gluDisk(quadric, 0, head_radius, head_slices, 1);
          gluQuadricOrientation(quadric, GLU_OUTSIDE);
          gluCylinder(quadric, head_radius, 0, head_len, head_slices, 1);
          glTranslated(0, 0, -1+head_len);
          glEndList();

          glNewList(all_axes_dpy_list, GL_COMPILE_AND_EXECUTE);
          glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT);
          glEnable(GL_LIGHTING);
          glEnable(GL_COLOR_MATERIAL);
          glDisable(GL_TEXTURE_2D);
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
          glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
          glEnable(GL_CULL_FACE);
          glShadeModel(GL_SMOOTH);
          {
            GLfloat params[4] = { 0.5, 0.5, 0.5, 1 };
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, params);
          }
          {
            GLfloat params[4] = { 0.4, 0.4, 0.4, 1 };
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, params);
          }
          glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32);
          // X-axis
          glColor3f(0.9, 0.2, 0.2);
          glRotated(90, 0, 1, 0);
          glCallList(one_axis_dpy_list);
          glRotated(-90, 0, 1, 0);
          // Y-axis
          glColor3f(0.2, 0.9, 0.2);
          glRotated(90, -1, 0, 0);
          glCallList(one_axis_dpy_list);
          glRotated(-90, -1, 0, 0);
          // Z-axis
          glColor3f(0.2, 0.2, 0.9);
          glCallList(one_axis_dpy_list);
          glPopAttrib();
          glEndList();
        }
        else {
          glCallList(all_axes_dpy_list);
        }
      }

      render_scene();

      glPopMatrix();

      if (status_hud_active || private_state->has_open_dialogs()) {
        render_hud();
        if (status_hud_timeout <= now) status_hud_active = false;
      }
    }



    // Render "head-up display"
    void Application::render_hud()
    {
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      glOrtho(0, viewport_width, 0, viewport_height, -1, 1);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();

      glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_LIGHTING);
      glDisable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

      GLint prev_tex;
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &prev_tex);

      private_state->render_hud(viewport_width, viewport_height);

      if (status_hud_active) {
        if (status_hud_dirty) {
          TextFormatter &text_formatter = private_state->get_text_formatter();
          text_formatter.set_font_size(28);
          text_formatter.set_font_boldness(1);
          text_formatter.set_text_color(Vec4F(0.1, 0, 0.376, 1));
          text_formatter.write(status_hud_text);
          text_formatter.format(private_state->status_hud_text_layout);
          text_formatter.clear();

          int const margin = 16, padding_h = 4, padding_v = 1;
          int const width  = ceil(private_state->status_hud_text_layout.get_width())  + 2*padding_h;
          int const height = ceil(private_state->status_hud_text_layout.get_height()) + 2*padding_v;
          int const x = viewport_width - margin - width;
          int const y = margin;

          if (!status_hud_disp_list) {
            status_hud_disp_list = glGenLists(1);
            if (!status_hud_disp_list)
              throw runtime_error("Failed to create a new OpenGL display list");
          }

          glNewList(status_hud_disp_list, GL_COMPILE_AND_EXECUTE);
          glTranslatef(x,y,0);
          glColor4f(1,1,0,0.7);
          glBegin(GL_QUADS);
          glVertex2i(-padding_h, -padding_v);
          glVertex2i(width,      -padding_v);
          glVertex2i(width,      height);
          glVertex2i(-padding_h, height);
          glEnd();
          private_state->status_hud_text_layout.render();
          glEndList();

          status_hud_dirty = false;
        }
        else glCallList(status_hud_disp_list);
      }

      glBindTexture(GL_TEXTURE_2D, prev_tex);
      glPopAttrib();
      glPopMatrix();
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
    }



    void Application::update_observer(Time now)
    {
      glTranslated(0, 0, -proj.camera_dist);

      Rotation3 rot = trackball.get_orientation(now);
      if(rot.angle) glRotated(180/M_PI*rot.angle, rot.axis[0], rot.axis[1], rot.axis[2]);
    }



    void Application::modify_zoom(int diff)
    {
      int const level = archon_round(log(proj.zoom_factor) / log(zoom_step));
      set_zoom_factor(pow(zoom_step, level + diff));
      set_float_status(L"ZOOM = ", proj.zoom_factor, 2, L"x");
    }


    void Application::modify_dist(int diff)
    {
      // The distance modification comes about indirectly. We modify
      // the size of the sphere of interest, and the auto-distance
      // feature makes the corresponding change in distance.
      int const level = archon_round(log(interest_size) / log(camera_dist_step));
      set_interest_size(pow(camera_dist_step, level + diff));
      status_hud_activate_cam_dist = true;
      status_hud_activate_cam_dist_timeout = get_status_hud_timout();
    }


    void Application::set_status(wstring text, Time timeout)
    {
      if(!status_hud_enabled) return;
      status_hud_text = text;
      status_hud_dirty = true;
      activate_status(timeout);
      status_hud_activate_cam_dist = false;
    }

    void Application::set_int_status(wstring prefix, int value, wstring suffix, Time timeout)
    {
      if(!status_hud_enabled) return;
      wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
      out << prefix << value << suffix;
      set_status(out.str(), timeout);
    }

    void Application::set_float_status(wstring prefix, double value, int precision,
                                           wstring suffix, Time timeout)
    {
      if(!status_hud_enabled) return;
      wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
      out << fixed << setprecision(precision) << prefix << value << suffix;
      set_status(out.str(), timeout);
    }

    void Application::set_on_off_status(wstring prefix, bool value, Time timeout)
    {
      if(!status_hud_enabled) return;
      wostringstream out; // FIXME: Must be imbued with a proper locale, or a reusable stream should be used
      out << prefix << L" IS " << (value ? "ON" : "OFF");
      set_status(out.str(), timeout);
    }

    void Application::activate_status(Time timeout)
    {
      if(!status_hud_enabled) return;
      status_hud_active = true;
      if(!timeout) timeout = get_status_hud_timout();
      if(status_hud_timeout < timeout) status_hud_timeout = timeout;
    }

    Time Application::get_status_hud_timout()
    {
      return Time::now() + Time(status_hud_linger_millis, Time::millis);
    }



    void Application::on_resize(SizeEvent const &e)
    {
      set_viewport_size(e.width, e.height);
      need_refresh = true;
      private_state->on_resize();
    }


    void Application::on_close(Event const &)
    {
      terminate = true;
      throw InterruptException();
    }


    void Application::on_keydown(KeyEvent const &e)
    {
      switch(e.key_sym)
      {
      case KeySym_Shift_L: // Modifier
        shift_left_down = true;
        break;

      case KeySym_q:
      case KeySym_Escape: // Quit event loop
        on_close(e);
        break;

      case KeySym_space:  // Reset camera configuration
        trackball.set_orientation(initial_rotation);
        set_interest_size(initial_interest_size);
        set_zoom_factor(initial_zoom_factor);
        set_status(L"RESET VIEW");
        need_refresh = true;
        break;

      case KeySym_KP_Add: // Increase frame rate
        set_frame_rate(frame_rate * 2);
        set_float_status(L"FRAME RATE = ", frame_rate);
        need_refresh = true;
        break;

      case KeySym_KP_Subtract:   // Decrease frame rate
        set_frame_rate(frame_rate / 2);
        set_float_status(L"FRAME RATE = ", frame_rate);
        need_refresh = true;
        break;

      case KeySym_h:      // Open help window
        private_state->open_help_hud();
        break;

      case KeySym_l:      // Toggle headlight
        set_on_off_status(L"HEADLIGHT", headlight ^= true);
        need_refresh = true;
        break;

      case KeySym_f:      // Toggle fullscreen mode
        win->set_fullscreen_enabled(fullscreen_mode ^= true);
        need_refresh = true;
        break;

      case KeySym_w:      // Toggle wireframe mode
        set_on_off_status(L"WIREFRAME", wireframe_mode ^= true);
        need_refresh = true;
        break;

      case KeySym_a:      // Toggle X,Y,Z axes display
        set_on_off_status(L"AXES", axes_display ^= true);
        need_refresh = true;
        break;

      case KeySym_s:      // Toggle status HUD enable
        if(status_hud_enabled)
        {
          set_on_off_status(L"STATUS", false);
          status_hud_enabled = false;
        }
        else
        {
          status_hud_enabled = true;
          set_on_off_status(L"STATUS", true);
        }
        need_refresh = true;
        break;

      default:
        {
          KeyHandlers::iterator i = key_handlers.find(e.key_sym);
          if(i != key_handlers.end() && i->second.first->handle(this, true)) need_refresh = true;
        }
        break;
      }
    }


    void Application::on_keyup(KeyEvent const &e)
    {
      switch(e.key_sym)
      {
      case KeySym_Shift_L: // Modifier
        shift_left_down = false;
        break;
      default:
        {
          KeyHandlers::iterator i = key_handlers.find(e.key_sym);
          if(i != key_handlers.end() && i->second.first->handle(this, false)) need_refresh = true;
        }
      }
    }


    void Application::on_mousedown(MouseButtonEvent const &e)
    {
      if(e.button == 1)
      {
        but1_down = true;
        win->set_cursor(cursor_trackball);
        trackball.acquire(Time::now());
        trackball.track(e.x, e.y, e.time);
      }
      if(e.button == 4) // Mouse wheel scroll up -> approach
      {
        if(shift_left_down) modify_zoom(+1);
        else modify_dist(-1);
        need_refresh = true;
      }
      if(e.button == 5) // Mouse wheel scroll down -> recede
      {
        if(shift_left_down) modify_zoom(-1);
        else modify_dist(+1);
        need_refresh = true;
      }
    }


    void Application::on_mouseup(MouseButtonEvent const &e)
    {
      if(e.button == 1)
      {
        trackball.track(e.x, e.y, e.time);
        trackball.release(Time::now());
        win->set_cursor(cursor_normal);
        but1_down = false;
      }
    }


    void Application::on_mousemove(MouseEvent const &e)
    {
      if(but1_down) trackball.track(e.x, e.y, e.time);
    }


    void Application::on_show(Event const &)
    {
//      cerr << "SHOW" << endl;
    }


    void Application::on_hide(Event const &)
    {
//      cerr << "HIDE" << endl;
    }


    void Application::on_damage(AreaEvent const &)
    {
      need_refresh = true;
    }


    void Application::before_sleep()
    {
      if(need_refresh)
      {
        need_refresh = false;
        throw InterruptException();
      }
    }


    void Application::register_key_handler(KeySym key, UniquePtr<KeyHandlerBase> handler,
                                           string descr)
    {
      pair<KeyHandlers::iterator, bool> const r =
        key_handlers.insert(make_pair(key, make_pair(handler.get(), descr)));
      if(!r.second)
        throw KeyHandlerConflictException("Multiple registrations for key '"+
                                          event_proc->get_key_sym_name(key)+"'");
      try
      {
        key_handler_owner.push_back(handler);
      }
      catch(...)
      {
        key_handlers.erase(r.first);
      }
    }



    Application::Config::Config():
      frame_rate(59.95), win_size(500), win_pos(-1), scr_dpcm(0.0), eye_scr_dist(0.5),
      depth_of_field(1000), interest_size(2), zoom(1), detail_level(1), direct_render(true),
      fullscreen(false), headlight(true), ambience(0.2), bgcolor(0.0), glyph_resol(64,64),
      glyph_mipmap(true), glyph_save(false),
      archon_datadir(get_value_of(build_config_param_DataDir))
    {
      string const v = Sys::getenv("ARCHON_DATADIR");
      if (!v.empty()) {
        archon_datadir = v;
        if (v[v.size()-1] != '/') archon_datadir += "/";
      }
    }


    void Application::Config::populate(core::ConfigBuilder &cfg)
    {
      cfg.add_param("f", "frame-rate", frame_rate,
                    "The initial frame rate. The frame rate marks the upper limit of frames "
                    "per second");
      cfg.add_param("s", "win-size", win_size,
                    "The initial size (width, height) in pixels of the windows contents area");
      cfg.add_param("p", "win-pos", win_pos,
                    "The initial position (x,y) in pixels of the upper left corner of "
                    "the outside window frame, relative to the upper left corner of the screen.\n"
                    "If any of the two coordinates are negative, both coordinates are ignored, "
                    "and the window manager will choose the initial position");
      cfg.add_param("r", "scr-dpcm", scr_dpcm,
                    "The resolution (horizontal, vertical) of the target screen in dots per "
                    "centimeter. If the value in one direction is zero or negative, then the "
                    "effective value in that direction will be determinaed automatically, "
                    "which may, or may not yield an accurate result.\n"
                    "To translate from dots per inch (dpi) to dots per centimeter, divide by "
                    "2.54 cm/in.\n"
                    "Specifying the wrong values here will produce the wrong field of view, "
                    "which in turn will produce the wrong aspect ratio between the Z-axis and "
                    "the X-Y-plane, which in turn leads to the depth effect appearing either "
                    "stretched or squeezed. It may also produce the wrong aspect ratio between "
                    "the X and Y-axes, which will lead to circles in the X-Y-plane appearing "
                    "egg-shaped");
      cfg.add_param("e", "eye-scr-dist", eye_scr_dist,
                    "The initial physical distance in meters between your eyes and the screen. "
                    "Specifying the wrong distance here will produce the wrong field of view, "
                    "which in turn will produce the wrong aspect ratio between the Z-axis "
                    "and the X-Y plane, which in turn leads to the depth effect appearing "
                    "either stretched or squeezed");
      cfg.add_param("d", "depth-of-field", depth_of_field,
                    "The initial depth of field. The depth of field is the ratio between the "
                    "depth of the near and the far clipping planes. It must be greater than 1. "
                    "Smaller values produce more accurate depth tests but makes it more likely "
                    "that your scene will be clipped");
      cfg.add_param("i", "interest-size", interest_size,
                    "The diameter of the initial sphere of interest in global modelview "
                    "coordinates. By default, the viewing frustum will be made as narrow as "
                    "possible while it still contains the sphere of interest completely.");
      cfg.add_param("z", "zoom", zoom, "Set the zoom factor. When you double the zoom factor, "
                    "you double the size of the on-screen projections of scene features.");
      cfg.add_param("l", "detail-level", detail_level,
                    "The initial level of detail. The level of detail controls the general "
                    "quality of the rendering, for example, by adjusting the number of faces "
                    "used to render a curved surface. A value of 1 corresponds to the normal "
                    "level of detail, while a value of 2 corresponds to twice the normal level "
                    "of detail. Any value is allowed");
      cfg.add_param("D", "direct-render", direct_render,
                    "Attempt to create a direct rendering contexts to gain performance. "
                    "This may fail, in which case, there will be a silent fallback to indirect "
                    "rendering");
      cfg.add_param("F", "fullscreen", fullscreen, "Open all windows in fullscreen mode.");
      cfg.add_param("H", "headlight", headlight, "Turn on the headlight.");
      cfg.add_param("a", "ambience", ambience,
                    "The global ambient intencity. For each shaded pixel, this value times the "
                    "ambient color of the material is aded to the final color of the pixel");
      cfg.add_param("b", "bgcolor", bgcolor, "The background color specified as a RGBA quadruple");
      cfg.add_param("R", "glyph-resol", glyph_resol, "Set an alternative glyph resolution to be "
                    "used by the default font provider. This is actually the resulution of "
                    "the EM-square, and fractional values are allowed.");
      cfg.add_param("M", "glyph-mipmap", glyph_mipmap, "Enable mipmapping on glyph textures "
                    "generated by the default font provider.");
      cfg.add_param("T", "glyph-save", glyph_save,
                    "Save all glyph textures generated by the default font provider as images.");
      cfg.add_param("", "archon-datadir", archon_datadir, "The path to the directory in which "
                    "the idiosyncratic read-only architecture-independent data objects used by "
                    "the Archon libraries are installed. It must be specified with a trailing "
                    "slash.");
    }
  }
}
