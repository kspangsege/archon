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

#ifndef ARCHON_DOM_IMPL_CORE_HPP
#define ARCHON_DOM_IMPL_CORE_HPP

/// \file
///
/// \author Kristian Spangsege

#include <map>

#include <archon/core/bind_ref.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/util/hashing.hpp>
#include <archon/util/hash_map.hpp>
#include <archon/dom/core.hpp>
#include <archon/dom/impl/util.hpp>

/*

Dynamic exception specifications
--------------------------------

One of the following statements is true:

- All DOM objects that, according to the IDL, are reachable via
  attributes and non-throwing functions must be allocated up front.
  None can be created on demand. This may well mean that every single
  DOM object needs to be created up front.

- The C++ DOM interfaces cannot use dynamic exception specifications
  that precisely mirror the IDL.

Since it is deemed unacceptable (due to memory footprint
considerations) to create all DOM objects up front, and since dynamic
exception specifications are deprecated in C++11, the best approach is
to drop the dynamic exception specifications from the The C++ DOM
interfaces, and replace them by comments.



    STRATEGY:

No need for virtual clear. ~Node may assert that there is no longer any rare data (in the future if may have to destroy various rare objects). ~ParentNode shall call Manag::discard_if_unref() such that its list is discarded while the parent node is still a ParentNode.






Make a combined ChildList / DegenChildList

When is DEGEN, and gets unreferenced, it gets invalidated immediately

  In ~ParentNode the child list must be removed if it is unreferenced. If it is referenced, it is an internal error.

Therefore, there should never be any child list in the ~Node.

  Get rid of weird ParentNode::destroy semantics by simply calling it from ~ParentNode and also from ~Document.

  Also in rare data:
    UserData
    EventHandlers
    AttrNamedNodeMap
    StyleDecl

  Move Leaf nodes to core.cpp (only Element and Document need be in core.hpp)




Document::elem_types should be converted to a HashSet rather than a HashMap with the value_type being the ElemRef which should contain both the unmodularized hash value and the identifying key, plus any extra fields required. This will rely on the property that pointers to HashMap/HashSet values remain valid under map/set modification. HMM - NOT SO EASY AFTER ALL!!

STRONGLY CONSIDERING TO MOVE ElemType AND AttrType REGISTRIES INTO Implementation
  adoptNode() WOULD BENEFIT GREATLY
  But the problem is that unspecified attributes with DTD specified default values change when nodes are transferred from one document to another.


HMM - HOW CAN THE FOLLOWING TWO XXX BE RECONCILED

1> Derived HTML element classes always remain when nodes are transferred from one document to another, regardless of the DTDs of those document.

2> Unspecified attributes with DTD specified default values change when nodes are transferred from one document to another.

*/


/*

CONSIDER reimplementing "read only" as a node bit
CONSIDER reimplementing "element content whitespace" as a node bit



WHOOPS: HOW TO KNOW THE NAME OF THE ATTRIBUTE IF THE AttrType IS ONLY AVAILABLE WHEN THE Attr IS BOUND TO AN ELEMENT?
ALSO: HOW TO REPLACE AttrType KEYS IN REGULAR ATTR MAP IN adoptNode()?

An Attr always points to an AttrType. For an unbound Attr, this simply encodes the namespace and name of it.

When the Attr is bound to an Elem, the ElemType is asked if it wants to replace the AttrType.

How about documents with mixed case sensitivity? Is it even possible?



Attributes:

Default attributes are stored in ElemType


struct Element {
    AttrMap* attr_map;

    DOMString getAttribute(in DOMString name)
    {
        return get_attr_val(type);
    }

    void setAttribute(in DOMString name, in DOMString value) throws (DOMException)
    {
        set_attr_val(type, value);
    }

    DOMString get_attr_val(const AttrType* t)
    {
        DOMString v;
        if (attr_map && attr_map->get_val(t, v))
            return v;
        get_type()->get_default_attr_val(t, v);
        return v;
    }

    void set_attr_val(const AttrType* t, const DOMString& v)
    {
        t->on_set_value(v);
    }
};

WHOOPS: How to transfer AttrType's in adoptNode()?


Special attributes:
Any that is declared to be an ID attribute
HTML: "id"
HTML: "class"
HTML: "style"
HTML: event handlers


ref<Node> NamedNodeMap::getNamedItem(in DOMString name)
{
    elem->get_attr(type);
}

ref<Node> NamedNodeMap::setNamedItem(const ref<Node>& arg) throws (DOMException)
{
}

ref<Node> NamedNodeMap::item(in unsigned long index)
{
}

NamedNodeMap::getLength()
{
    return num_specified + num_unspecified_defaults;
}

struct AttrVal
{
    string value;
    flag is_specified;
    flag is_id;
    Attr* attr;   // Non-null while Attr is referenced. Also non-null if value is structured.
};

If at most 8 attribs are specified:
Use a flat fixed size list.

// If more than 8 attribs are specified:
map<AttrType, AttrVal> Element::attribs;

Map type: none, max 6, many

It would be nice if the iteration order of attribs was always according to time of specification with the unspecified coming first.

It seems then that it would be ok for Attr to have next/prev sibling fields. They would be used to remember order when using a regular map.

An attribute that is not bound to an element, will remember its value only through its children.

At least one unused Attr node should be cached.

PROBLEM: It is relatively expensive to construct an attribute node when it is requested.
Can maybe be solved by also caching one unused text node.

PROBLEM: When using regular map, there is still no way to remember the order if there are no associated Attr.
Maybe: When in 'many' mode, there is always an Attr associated with each specified attribute

PROBLEM: The unspecified attrib with default value cannot be stored as Attr instances in ElemType, because that would not allow us to know the target element.
When requesting Attr for unspecified attr with default val, add the attr to the map.
When requesting string for unspecified attr with default val, just return the default value.
Map must shrink but without too much hysteresis: Regular converts to "max 6" when less than 4.

PROBLEM: When iterating over attribs of element with no specified attribs, and with several unspecified attribs, then the map needs to be created and destroyed for each examind Attr instance. If the non-null map is left in place, then all elements will contain stale maps after a complete tree traversal.
Keep one unused "max 6" map cached.






1> Absolutely rudimentary attribute system

2> Implement rudimentary adoptNode().

3> Implement filtered element lists (each getElementsByTagName returns a new list instance)




It is the respopnsibility of the application not to introduce cycles by means of directly or indirectly storing references to DOM objects as DOM node user data.



TODO (new priority):

1> Thread-safety

2> Script considerations (engine context is not owned by a document, thus one does not automatically have a scripting context just because one creates a document)

4> Absolutely rudimentary style

5> Absolutely rudimentary rendering



Important information:

  In general, the implemented methods of the public interfaces in the
  'dom' namespace should not be called internally. However, when they
  are, it is a requirement that the target node has at least one
  external reference to it during the call.



TODO:
Map main
map buckets
map bucket
Attr
Text
string data




Map

  int/size_t num_entries;

  pair<int attr_id, string>






Attributes that need special consideration:

  style (is not a special attribute on some elements)

  event handlers specified as elem.setAttribute("onclick", "xxx"); - also calls elem.onclick = function(event) { xxx }; If compilation fails, issue warning, and clear event handler

  event handlers specified as elem.onclick = function(event) { ... }; - clears the 'onclick' attribute, if any (Presto and WebKit leave the original attribute value unchanged, Gecko considers the two forms as two distinct event handlers that do not interfere with each other, Trident returns the function as a function when calling getAttribute)

  event handlers attached via the Events API (WHOOPS THESE CAN ALSO BE SPECIFIED ON NON-ELEMENT NODES)

  user data (uses dedicated attr_id - maybe one attr_id per user data key, maybe one for an entire map of userdata key/value pairs) (WHOOPS THESE CAN ALSO BE SPECIFIED ON NON-ELEMENT NODES)


Attr is always a wrapper only, and is created only on demand, that is, when Node.getAttributeNode() is called.

When an Attr is not attached to an element, the value is always represented as a simple string.


A NamedNodeMap for attributes caches a list of unspecified attributes with a non-empty default value. These will be returned last (or first) when iterating over the map. The cache is only updated when required.

When lookup by name in a NamedNodeMap for attributes, we first consult the set of specified attributes, then the set of unspecified attributes with nonempty default values.

Capacity change must not exhibit hysteresis.

AttrType impl depends on ElemType in general, so an AttrType cannot be transferred from one element to another.



struct AttrType {
  AttrName name;
  get();
  set();
};


struct ElemType {
    AttrType* lookup_attrib(const string& ns, const string& qname, bool instantiate)
    {
        
    }
};


struct HtmlElemType: ElemType {
    AttrType* lookup_attrib(const string& ns, const string& qname, bool instantiate)
    {
        Probably just: Inject a bit into the hash key that says that ID is an attribute that needs special handling when applied to an HTML elememt. This way if ID is set on a non-HTML element, the bit will not be set. Consider an implementation dependant hash key generator.
        // Return statically allocated AttrType_Id if ns='' and qname=='id'
        // Similar for other HTML specific attributes that need specialized handling
        // return ElemType::lookup_attrib(ns, qname, instantiate);
        All AttrType instances must count references
        Focus on a solution that recognizes ID as a special attribute and all other attributes in the same general way.
        An instance of AttrType_Id
    }
};


struct HtmlAttribs {
    string id;
    string class;
    Style* style;
    EventHandlers* event_handlers;
    OtherHtmlAttribs* other;
};


struct Element {
    Attributes attributes;

    virtual string getAttributeNS(string ns, string qname)
    {
        AttrType* t = get_type()->lookup_attrib(ns, qname, false);
        if (!t)
            return "";
        string value;
        t->get(this, value);
        return value;
    }

    virtual void setAttributeNS(string ns, string qname, string values)
    {
        AttrType* t = get_type()->lookup_attrib(ns, qname, true);
        string value;
        t->get(this, value);
        return value;
    }
};



struct Attributes {
    bool get(int attr_id, string& s) const
    {
        if (!rep)
            return false;
        switch (rep->type) {
            case simple_1: return static_cast<SimpleAttribsRep<1>*>(this)->get(attr_id, s);
            case simple_2: return static_cast<SimpleAttribsRep<2>*>(this)->get(attr_id, s);
            case simple_4: return static_cast<SimpleAttribsRep<4>*>(this)->get(attr_id, s);
            case simple_8: return static_cast<SimpleAttribsRep<8>*>(this)->get(attr_id, s);
            case general:  return static_cast<GeneralAttribsRep*>(this)->get(attr_id, s);
        }
    }

    Rep* rep;

    struct Rep {
        short type; // Decides the capacity of SimpleAttribsRep or if rep is instead GeneralAttribsRep
    };

    template<class Spec> struct SimpleAttribsRep: Rep {
        short num;
        struct Entry {
            int attr_id;
            string value;
        };
        Entry entries[Spec::capacity];

        bool get(int attr_id, string& s)
        {
            for (int i = 0; i < num; ++i) {
                Entry& e = entries[i];
                if (e.attr_id == attr_id) {
                    s = e.value;
                    return true;
                }
            }
            return false;
        }

        void set(int attr_id, string const &s)
        {
            for (int i = 0; i < num; ++i) {
                Entry& e = entries[i];
                if (e.attr_id == attr_id) {
                    e.value = s;
                    return;
                }
            }
            if (num < Spec::capacity) {
                Entry& e = entries[num++];
                e.attr_id = attr_id;
                e.value = s;
                return;
            }
            
        }
    };

    struct GeneralAttribsRep: Rep {
    };
};



  Attributes:
    Namespace-qname pairs must be mapped to AttrType the same way as they are for elements. "No namespace" is to be handled like some other specific namespace that does not depend on anything. In particular, it does not depend on the element on which it is found.
    The map returned from Node.attributes must contain unspecified attributes with default values. The lookup function should first check the set of specified attributes, then consult the ElemType to check for unspecified attributes with default value. How to handle iteration over the map such that the attributes with default value are not included twice?
    The map returned from Node.attributes must be allocated on request, and deallocated when there are no more external references to it. It will keep the target element alive.
    Consider having one preallocated map object available in the document.
    Consider implementing a map (for internal use in an element representation) which uses a simple (maybe sorted) list of attribute pointers, and does brute force linear searching when the number of attributes is low (e.g., at most 8).
    Main problem is how to get fast access to attributes that are essential to rendering/presentation without wasting too much memory.
    All the defaulted attributes are stored statically as Attr objects and return false for Attr.specified.


  Make elaborate test of Node.textContent including Text.isElementContentWhitespace().

  Document.renameNode()

  Node.lookupNamespaceURI() (requires attributes)

  Node.lookupPrefix() (requires attributes)

  Node.isDefaultNamespace()

  Node.cloneNode()

  getElementById()

  UserData

  async parser, job scheduler, libcurl loader

  Parse in context

  Loading and parsing of external entities

  EntityReference

  DOMConfiguration

  Style

  JavaScript



Attributes: attributes that are not bound to an element are handled conventionally. When bound to an element, attribute nodes are registered in a hash map in the document by (Elem *, AttrType *) where AttrType is like ElemType. When last external reference is dropped, an Attr is deleted, and removed from map if in a map.




Consider submitting new DOM 3 tests: Presence of text child node in
internal parsed entities with no markup (violated in Java example
[what parser is used?]). Constancy of children of entities (violated
in Opera).

Defaulted attributes are not included in Node.attributes for HTML documents in Gecko, Presto, and WebKit.
In Gecko and WebKit Attr.specified is incorrectly true for unspecified defaulted attributes in XML documents (with attributes declared in internal subset of DTD)
Gecko, Presto, and WebKit all have trouble with Text.isElementContentWhitespace()


THREAD SAFTY:

Desired:

  - DOM object classes fall into two categories; those that are
    strongly thread-safe and those that are not. Those that are not
    strongly thread-safe are NodeList, NamedNodeMap, and all classes
    that are ultimately derived from Node. All other classes in the
    DOM API are strongly thread-safe.

  - Two distinct objects that are not strongly thread-safe may still
    be accessed by two different threads as long as those objects are
    not associated with the same document. Here, a document is
    considered to be associated with itself, and a NodeList or a
    NamedNodeMap is considered to be associated with the same document
    as the node that is associated with the NodeList or a
    NamedNodeMap.

  - Strongly thread-safe DOM object classes derive from
    ThreadSafeDOMObject and, as a consequence, references to such
    objects are weakly thread-safe.

  - DOM object classes that are not strongly thread-safe are derived
    from DOMObject and, as a consequence, references to such objects
    are not even weakly thread-safe. Two distinct reference objects,
    whose target is a DOM object class that is not strongly thread-safe,
    may still be accessed by two different threads as long as the target
    objects are not associated with the same document.

\sa ThreadSafety


Interestingly, it seems that it is not necessary for DOMObject to have the on_referenced() method! Instead the implementation can check whether the reference count is zero before it creates a new reference, and then perform the on_referenced action if it is. While this is probably a good change in any case, unfortunately, it does not make it any easier to implement thread-safty.


It would be really nice if the entire API could be made thread safe without too high a performance sacrifice - it seems really hard though due to the complex reference counting scheme!

One way it could be done is by adding a mutex to the implementation object and have all methods acquire a lock on it. Also any increment or derecrement of a reference count must be done inside such a lock. That would require addition of an implementation pointer to the object reference type or to the objects themselves.
So, in summmary, the cost of a completely thread-safe API, is the addition of an extra implementation pointer to each reference/object plus the locking of a mutex for each method invocation as well as whenever a reference is copied or destroyed.


Apparently, it is possible to register mutation event handlers on text nodes. Thus, it is necessary to have flags on all nodes - not just parent nodes (not strictly necessary though).





Fresh attrib considerations as of Jan 13, 2014:

Does elem.setAttribute() detach the previous Attr node with that name if it existed?
  WebKit: No
  Gecko: No
  Java 1.7: No

Apparently, WebKit remembers Attrib child structure indefinately, so that seems like the right thing to do.

Node.getAttributes() returns null in general, but for an Element, it returns a "live" map. Create it on demand and store an uncounted pointer to it in the Element. When the map is abandoned, clear this pointer. The only open question is how to handle NamedNodeMap.length and NamedNodeMap.item(). Should it include unspecified attributes? Yes, according to Java DOM, unspecified attributes given by the DTD as `#FIXED` must be included. But what about those given in the DTD as `#DEFAULT`?

In a compact attribute map, an attribut value has no substructure. When the corresponding Attr node is inspected, it has only one Text node child. But how can manipulation of the Text node be made to affect the attribute map? Probably be introducing a new TextType.

No attribute map should be present on elements with zero specified attributes.

All nodes refer to a NodeType. For an Attr node, it is an AttrType. The AttrType encodes the meaning of the attribute, if any. It also encodes the name of the attribute.

Element.setAttribute(name, value)
{
    key = hash(elem_type, name) // Must probably also include namespace to key, and maybe also dom1-flag
    AttrType* attr_type = doc.attr_type_map[key];
    if (!attr_type) {
        attr_type = elem_type->create_attr_type(name);
        doc.attr_type_map[key] = attr_type;
    }
    attr_type->set_value(this, value);
}

class HtmlStyleAttrType: AttrType {
    void set_value(this, value) ARCHON_OVERRIDES
    {
    }
}

Hmm, how can we then know which attributes are specified?

Maybe keep attr values in map, but notify about value changes via virtual call into AttrType object. That way we will know hoe many attributes are specified.

The trick is to know how many unspecified attribute there is with values given by DTD (#FIXED and #DEFAULT).

Maybe by caching this number as `num_unspec` in the attr_map. When there is no map, the total number of attributes, is the number of (#FIXED and #DEFAULT) attribs in the DTD, a fixed number that can be returned by virt func AttrType::gen_num_attribs_with_default_value().

When a new map is added, set num_unspec = attr_type->gen_num_attribs_with_default_value().

Then decrement `num_unspec` whenever adding a new entry to attr_map, and AttrType->has_default_value() returns true.
And increment `num_unspec` whenever removing an entry from attr_map, and AttrType->has_default_value() returns true.

Then AttrNamedNodeMap::getLength() returns the sum of `num_unspec` and the number of entries in attr_map.

And AttrNamedNodeMap::item(i) returns: i < num_unspec ? attr_type->get_default_attr_value(i) : ;   NO NO NO - THIS DOSN'T WORK


Forget about GeneralAttrMap for now. Fail with an exception if compact map would exceed its capacity.

Store a reference to AttrNamedNodeMap in RareNodeData.

The problem is how to handle Attr nodes for unspecified attributes with default value. They need to have the right parent pointer, and a reference to them must be stored somewhere such that if the attribute value is changed by other means, the Attr is properly updated.

Idea:

An attribute is specified if, and only if it occurs in Element::AttrMap.

The order of attributes in Element::AttrMap reflects the order in which they were specified.

A slot in Element::AttrMap stores an Attr pointer if, and only if the attribute has a structured value.

An attribute whose flattened value is the empty string, has a structured value if it has at least one child.
An attribute whose flattened value is non-empty, has a structured value if it has more than one child, or if at has a child that is not a Text node.

KeepNote: Java DOM parser places an empty text node in Attr objects with empty value, but none of the folowing browser engines do: Gecko, WebKit, Presto, and Trident.

Sometimes, an Attr must be destroyed even if it has a parent

void Attr::on_unreferenced() const ARCHON_NOEXCEPT
{
    if (parent && !has_structured_value) {
        // If specified, remove from Element::AttrMap
        // Remove from AttrNamedNodeMap
        type->doc->bind_ref();
        parent->unbind_ref();
    }
    Node::on_unreferenced();
}

dom::ref<dom::Node> Attr::getParentNode() const
{
    return dom::null;
}

dom::ref<dom::Element> Attr::getOwnerElement() const
{
    return static_cast<Element*>(parent);
}


When an attribute value is changed via child node manipulation, that change must somehow be reported back to the element.

When an attribute value transitions between being unstructured to being structured, Attr::has_structured_value msut be updated, and its pointer must be stored into, or removed from Element::AttrMap.


The idea is further that when an Attr is needed for an attribute with unstructured value (including an unspecified attribute with default value), then the first thing that must happend, is that a AttrNamedNodeMap is created and stored as rare node data. Then the AttrNamedNodeMap will be asked to provide the Attr.

Alternative: Create an AttrManager that will allow a number of Attr's to persist even when they have unstructured values, but will reap and reuse old ones that are unreferenced and have unstructured values. It might be that this is a good idea only if it simplifies other aspects of attribute handling.

*/


namespace archon {
namespace DomImpl {

class Node;
class ParentNode;
class ChildList;
class ChildListManager;
class Element;
class DocumentFragment;
class Document;
class DOMImplementation;



struct NodeType {
    const dom::uint16 id; // E.g. dom::Node::ELEMENT_NODE

    /// Is null for a DocumentType that is not yet bound to a
    /// document. Otherwise it is never null.
    Document* const doc;

    const bool is_child_node;

    const bool is_parent_node;

    const bool read_only;

    NodeType(dom::uint16 i, Document* d, bool c, bool p, bool r):
        id(i), doc(d), is_child_node(c), is_parent_node(p), read_only(r) {}
};




struct AttrType {
    // It seems that the spec mandates that a dynamically created
    // attribute is never an ID unless it is explicitely made so by
    // calling Element::setIdAttribute() - however, this is
    // incompatible with HTML in all common browsers.
    virtual bool is_id() const throw () { return false; }

    virtual ~AttrType() {}
};




struct ElemKey {
    dom::DOMString ns_uri;
    dom::DOMString tag_name;
    bool dom1; // Element created by a DOM Level 1 method
    ElemKey(const dom::DOMString& n, const dom::DOMString& t, bool d):
        ns_uri(n), tag_name(t), dom1(d) {}
    bool operator==(const ElemKey& k) const
    {
        return tag_name == k.tag_name && dom1 == k.dom1 && ns_uri == k.ns_uri;
    }
};




struct ElemQual {
    dom::DOMString ns_uri;      // Node.namespaceURI
    dom::DOMString tag_name;    // Node.tag_name
    dom::DOMString prefix;      // Node.prefix
    dom::DOMString local_name;  // Node.local_name
};




// Element types, for which NodeType:read_only is false, are
// managed dynamically by the document, and when its reference
// count drops to zero, it must ask the document to unregister
// it. If NodeType::read_only is true, then the instance is
// statically bound to a DocumentType, for example, and nothing
// must happen when the reference count drops to zero.
struct ElemType: NodeType {
    const ElemKey key;
    const ElemQual qual;

    ElemType(Document* d, bool read_only, const ElemKey& k, const ElemQual& q):
        NodeType(dom::Node::ELEMENT_NODE, d, true, true, read_only), key(k), qual(q),
        ref_count(0) {}

    virtual ~ElemType() {}


    void bind_ref() throw () { ++ref_count; }

    void unbind_ref() throw (); // FIXME: For read-only elements, reaching zero must be a no-op because they will be stored statically in the DocumentType.


    virtual Element* create_element();

    virtual bool is_element_content() const { return false; } // FIXME: Should instead be a method that retrieves a schema type information instance owned by the callee.

    bool get_attr(const AttrType*, dom::DOMString&) const throw () { return false; } // FIXME: Implement

private:
    int ref_count; // Number of elements of this type
};


typedef core::BindRef<ElemType*> ElemTypeRef;




struct TextType: NodeType {
    const bool elem_cont_whitespace;

    TextType(dom::uint16 i, Document* d, bool r, bool w):
        NodeType(i, d, true, false, r), elem_cont_whitespace(w) {}
};




/// This class, and derived classes, are supposed to consist only of
/// \c num_objs as well as a number of rare object pointers.
///
/// Invariant: If num_objs == 0, then all object pointers in this
/// class, and in any derived class, are null.
struct RareNodeData {
    RareNodeData(): num_objs(0), child_list(0) {}

    bool is_empty() const { return num_objs == 0; }

private:
    friend class Node;
    friend class ChildList;

    template<class T> T* get_obj()
    {
        return T::subscr(this);
    }

    template<class N, class T> void ensure_obj(N* n, dom::ref<T>& o)
    {
        T*& obj = T::subscr(this);
        if (obj) {
            o.reset(obj);
            return;
        }
        T::acquire(n,o);
        obj = o.get();
        ++num_objs;
    }

    template<class T> bool remove_obj(Node* n)
    {
        T*& obj = T::subscr(this);
        T::release(obj, n);
        obj = 0;
        return --num_objs == 0;
    }

    int num_objs;
    ChildList* child_list;
};




/// The reference count of a node keeps track of the number of direct
/// and indirect external references to it. External references are
/// those that are not part of the DOM structure itself.
///
/// Each direct external reference to a node causes an increment on
/// the reference count of that node.
///
/// A node with a parent causes an increment on the reference count of
/// the parent whenever its own reference count is greater than zero.
///
/// A node without a parent causes an increment on the reference count
/// of the document whenever its own reference count is greater than
/// zero. If this node is a DocumentType, that is not yet bound to a
/// document, and has no parent, then it does not cause an increment
/// on the reference count of any other node.
///
/// A node is destroyed when both of the following becomes true: Its
/// reference count is zero, and it has no parent.
struct Node: virtual dom::Node {
    // Some node types have no value
    virtual dom::DOMString getNodeValue() const throw ()
    {
        return dom::DOMString();
    }

    // This has no effect for some node types
    virtual void setNodeValue(const dom::DOMString&) throw (dom::DOMException) {}

    virtual dom::uint16 getNodeType() const throw ();

    virtual dom::ref<dom::Node> getParentNode() const throw ();

    virtual dom::ref<dom::NodeList> getChildNodes() const throw ();

    virtual dom::ref<dom::Node> getFirstChild() const throw () { return dom::null; }

    virtual dom::ref<dom::Node> getLastChild()  const throw () { return dom::null; }

    virtual dom::ref<dom::Node> getPreviousSibling() const throw ();

    virtual dom::ref<dom::Node> getNextSibling() const throw ();

    virtual dom::ref<dom::Document> getOwnerDocument() const throw ();

    // Some node types cannot have children
    virtual dom::ref<dom::Node> insertBefore(const dom::ref<dom::Node>&,
                                             const dom::ref<dom::Node>&)
        throw (dom::DOMException)
    {
        throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "No children allowed");
    }

    // Some node types cannot have children
    virtual dom::ref<dom::Node> replaceChild(const dom::ref<dom::Node>&,
                                             const dom::ref<dom::Node>&)
        throw (dom::DOMException)
    {
        throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "No children allowed");
    }

    // Some node types cannot have children
    virtual dom::ref<dom::Node> removeChild(const dom::ref<dom::Node>&)
        throw (dom::DOMException)
    {
        throw dom::DOMException(dom::NOT_FOUND_ERR, "No children allowed");
    }

    // Some node types cannot have children
    virtual dom::ref<dom::Node> appendChild(const dom::ref<dom::Node>&)
        throw (dom::DOMException)
    {
        throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "No children allowed");
    }

    // Some node types have no children
    virtual bool hasChildNodes() const throw () { return false; }

    virtual bool isSupported(const dom::DOMString& f, const dom::DOMString& v) const throw ();

    // Some node types have no namespace URI
    virtual dom::DOMString getNamespaceURI() const throw () { return dom::DOMString(); }

    // Some node types have no prefix
    virtual dom::DOMString getPrefix() const throw () { return dom::DOMString(); }

    // This has no effect for some node types
    virtual void setPrefix(const dom::DOMString&) throw (dom::DOMException) {}

    // Some node types have no local name
    virtual dom::DOMString getLocalName() const throw () { return dom::DOMString(); }

    // Some node types have no text contents
    virtual dom::DOMString getTextContent() const throw (dom::DOMException)
    {
        return dom::DOMString();
    }

    // This has no effect for some node types
    virtual void setTextContent(const dom::DOMString&) throw (dom::DOMException) {}

    virtual bool isSameNode(const dom::ref<const dom::Node>& other) const throw ()
    {
        return other.get() == this;
    }


    NodeType *get_type() const { return type; }

    ParentNode *get_parent() const { return parent; }

    // If first child, this points the the last child
    Node *get_prev_sibling() const { return prev; }

    Node *get_next_sibling() const { return next; }


    // Must never return true for a DocumentFragment
    bool is_read_only() const { return type->read_only; }

    virtual ~Node() throw ();


protected:
    Node(NodeType* t): type(t), parent(0), prev(this), next(0), rare_data(0) {}


    /// Overriding method in dom::DOMObject. Must be overriden by
    /// DocumentType because it does not always have access to the
    /// document. Must also be overridden by any node type that does
    /// not have the ordinary notion of a parent node, such as
    /// Document.
    virtual void on_referenced() const throw ();

    /// Overriding method in dom::DOMObject. Must be overriden by
    /// DocumentType because it does not always have access to the
    /// document. Must also be overridden by any node type that does
    /// not have the ordinary notion of a parent node, such as
    /// Document.
    virtual void on_unreferenced() const throw ();


    template<class T> T* get_rare_obj() const
    {
        return rare_data ? rare_data->get_obj<T>() : 0;
    }

    /// Must never be called on a DocumentType node.
    template<class N, class T> static void ensure_rare_obj(const N*, dom::ref<T>&);

    /// Must not be called if the rare object is absent for this node.
    template<class T> void remove_rare_obj();


private:
    friend class ParentNode;
    friend class ChildList;
    friend class ChildListManager;
    friend class Element;
    friend class DocumentType;

    NodeType* type; // May change, but is never null

    // NOTE: For Attr this is the owner element.
    ParentNode* parent;

    // NOTE: Attr redefines the meaning of 'next' and 'prev'.
    Node* prev; // If first child, this points the the last child
    Node* next; // Null if last child

    mutable RareNodeData* rare_data;
};




// A parent node always has an associated document.
struct ParentNode: Node {
    static const int flag_pos_Valid_child_list = 0;
    static const int flag_pos_End              = 1;

    typedef unsigned Flags; // This gives room for at least 16 distinct flags

    // Has an associated child list, and it has a valid cache
    static const Flags valid_child_list = 1 << flag_pos_Valid_child_list;

/*
  has_filtered_elem_list   = 1<<2, // A FilteredElemList may exist rooted at this parent node
  valid_filtered_elem_list = 1<<3  // A FilteredElemList with a valid cache may exist
*/
    // The following are only available for Attr
//        attr_is_spec             = 1<<1, // The attribute is specified
//        attr_is_id               = 1<<2  // The attribute value is an ID



    virtual dom::ref<dom::Node> getFirstChild() const throw ();

    virtual dom::ref<dom::Node> getLastChild() const throw ();

    virtual dom::ref<dom::Node> insertBefore(const dom::ref<dom::Node>&,
                                             const dom::ref<dom::Node>&)
        throw (dom::DOMException);

    virtual dom::ref<dom::Node> replaceChild(const dom::ref<dom::Node>&,
                                             const dom::ref<dom::Node>&)
        throw (dom::DOMException);

    virtual dom::ref<dom::Node> removeChild(const dom::ref<dom::Node>&)
        throw (dom::DOMException);

    virtual dom::ref<dom::Node> appendChild(const dom::ref<dom::Node>&)
        throw (dom::DOMException);

    virtual bool hasChildNodes() const throw () { return first_child; }

    virtual dom::DOMString getTextContent() const throw (dom::DOMException);

    virtual void setTextContent(const dom::DOMString&) throw (dom::DOMException);


    Document* get_doc() const { return get_type()->doc; }

    Node* get_first_child() const { return first_child; }

    // Check that this parent node (in its current state) can accept
    // the specified child node.
    virtual void approve_child(const Node*) const;

    // Check that this parent node (in its current state) can accept
    // the children of the specified document fragment node.
    virtual void approve_children(const DocumentFragment*) const;

    // Fails if this parent node is equal to or has an ancestor that
    // is equal to the specified candidate child.
    void detect_cycle(const ParentNode* child) const;

    // Must be called before the list of children is changed, this
    // includes changing the type of any of the current
    // children. Changing the attributes of this node does not count
    // as a change in children, neither does a change of one of the
    // current children (except when a child type is changed, as
    // noted already.)
    virtual void before_children_change() throw ();

    enum AddMode {
        add_mode_Append,
        add_mode_InsertBefore,
        add_mode_Replace
    };

    // Common implementation of appendChild(), insertBefore(), and
    // replaceChild(). If the 'add' mode is 'append', the
    // 'ref_child' argument is ignored, otherwise it must be
    // specified, and must be a child of this parent. The reference
    // child may in all cases be the same as the new child. The new
    // child must have a reference count greater than zero before
    // this method is called.
    template<AddMode> void add_child(dom::Node* new_child, dom::Node* ref_child = 0);

    void append_child_for_parser(Node*) throw ();

    void accum_text_contents(dom::DOMString&) const;

//      dom::ref<dom::NodeList> get_elems_by_tag_name();

    /// Remove nonessential rare objects such as unreferenced child
    /// lists.
    void clear_nonessential_rare_data() throw ();


    virtual ~ParentNode() throw ();


protected:
    ParentNode(NodeType* t): Node(t), first_child(0), flags(0) {}


    // Detach the specified child from its parent. The 'parent',
    // 'prev', and 'next' fields of the child are left in an
    // undefined state.
    void low_level_remove_child(Node*) throw ();

    // Attach the specified child to this parent assuming nothing
    // about the current state of 'parent', 'prev', and 'next'
    // fields.
    void low_level_append_child(Node*) throw ();

    // Attach the specified child to this parent assuming nothing
    // about the current state of 'parent', 'prev', and 'next'
    // fields. The specified reference child is assumed to be a
    // child of this parent, and the new child is inserted before
    // it.
    void low_level_insert_before(Node*, Node* ref) throw ();

    // Attach the specified child to this parent assuming nothing
    // about the current state of 'parent', 'prev', and 'next'
    // fields. The specified reference child is assumed to be a
    // child of this parent, and it will be replaced by the new
    // child. The 'parent', 'prev', and 'next' fields of the
    // replaced child are left in an undefined state.
    void low_level_replace_child(Node*, Node* ref) throw ();

    // Must be called from the destructor (or similar) of Document
    // such that all children are destroyed before the document
    // ceases to be a Document during its destruction.
    void destroy_children() throw ();


    bool has_flag(Flags f)             const { return flags & f; }
    void set_flag(Flags f)             const { flags |=  f; }
    void set_flag(Flags f, Flags mask) const { flags = flags & ~mask | f; }
    void clear_flag(Flags f)           const { flags &= ~f; }


private:
    friend class ChildList;
    friend class ChildListManager;
    friend class DocumentType;

    Node* first_child;

    mutable Flags flags;
};




struct ChildList: virtual dom::NodeList {
    virtual dom::ref<dom::Node> item(dom::uint32 index) const throw ();

    virtual dom::uint32 getLength() const throw ();


    bool is_referenced() const { return dom::NodeList::is_referenced(); }

    bool is_bound() const { return node; }

    // List must be bound
    bool is_valid() const { return prev_child || have_length; }


private:
    friend class RareNodeData;
    friend class ChildListManager;

    static ChildList*& subscr(RareNodeData* r) throw () { return r->child_list; }

    static void acquire(const Node*, dom::ref<ChildList>&);

    static void release(ChildList*, Node*) {} // Nothing to do

    virtual void on_referenced()   const throw ();
    virtual void on_unreferenced() const throw ();

    // Must be associated with a node
    void invalidate()
    {
        prev_child  = 0;
        have_length = false;
    }

    // Null if this list is not currently bound to a node. When the
    // list is unbound, the values of the other data members are
    // undefined.
    const Node* node;

    // Nonnull then this list is bound to a ParentNode. Null when it
    // is bound to something else.
    const ParentNode* parent_node;

    mutable dom::uint32 prev_index;
    mutable Node* prev_child;
    mutable bool have_length;
    mutable dom::uint32 length;
};




/// Degenerate child lists (those bound to nonparent nodes) are bound
/// to a node only while they are referenced. Thus, a nonparent node
/// should never encounter a child list in its destructor.
///
/// Invariant: A list that is bound to a node and is unreferenced, has
/// a valid cache.
struct ChildListManager {
    void invalidate(const ParentNode* p) throw ()
    {
        if (!p->has_flag(ParentNode::valid_child_list))
            return;
        ChildList* list = p->get_rare_obj<ChildList>();
        ARCHON_ASSERT(list);
        p->clear_flag(ParentNode::valid_child_list);
        if (list->is_referenced()) {
            list->invalidate();
        }
        else {
            unref_queue.remove(list);
            unref_queue.prepend(list);
            const_cast<ParentNode*>(p)->remove_rare_obj<ChildList>();
            list->node = 0;
        }
    }

    void discard_if_unref(const ParentNode* p) throw ()
    {
        ChildList* list = p->get_rare_obj<ChildList>();
        if (!list || list->is_referenced())
            return;
        p->clear_flag(ParentNode::valid_child_list);
        unref_queue.remove(list);
        unref_queue.prepend(list);
        const_cast<ParentNode*>(p)->remove_rare_obj<ChildList>();
        list->node = 0;
    }

    ~ChildListManager();

private:
    friend class ChildList;

    void acquire(const Node* n, dom::ref<ChildList>& l)
    {
        ChildList* list;
        if (!unref_queue.empty()) {
            list = unref_queue.get_first();
            if (!list->is_bound())
                goto have;
            if (min_valid_unrefs < unref_queue.size() || !list->is_valid()) {
                const_cast<Node*>(list->node)->remove_rare_obj<ChildList>();
                if (list->parent_node)
                    list->parent_node->clear_flag(ParentNode::valid_child_list);
                goto have;
            }
        }
        list = new ChildList;
        unref_queue.prepend(list);

      have:
        list->node = n;
        list->parent_node = n->get_type()->is_parent_node ? static_cast<const ParentNode*>(n) : 0;
        list->invalidate();
        l.reset(list);
    }

    // This queue contains any child list that is either not bound to
    // a parent node or not referenced. A list that is bound and has a
    // valid cache comes after any list that is unbound or does not
    // have a valid cache. The lists that are bound and have a valid
    // cache are ordered according to the time they became
    // unreferenced, such that the last list in the queue is the one
    // that became unreferenced at the latest point in time.
    SmallFixedSizeQueue<ChildList*, 8> unref_queue;
    static const int min_valid_unrefs = 4;
};




/// An element refers to an element type that specifies its namespace
/// and tag name. A specific element type exists in the context of a
/// document, only when there are currently elements of that type tied
/// to the document.
///
/// The type can be changed (for example as a result of migration from
/// one document to another,) but at any time, the existence of an
/// element must cause an increment in the reference count of the type
/// that it is currently tied to.
struct Element: ParentNode, virtual dom::Element {
    static const int flag_pos_End = ParentNode::flag_pos_End + 0;


    virtual dom::DOMString getNodeName() const throw ();

    virtual dom::DOMString getNamespaceURI() const throw ();

    virtual dom::DOMString getPrefix() const throw ();

    virtual void setPrefix(const dom::DOMString&) throw (dom::DOMException);

    virtual dom::DOMString getLocalName() const throw ();

    virtual dom::DOMString getTagName() const throw ();

    virtual dom::DOMString getAttribute(const dom::DOMString&) const throw ();

    virtual void setAttribute(const dom::DOMString&, const dom::DOMString&)
        throw (dom::DOMException);

    virtual dom::ref<dom::NodeList> getElementsByTagName(const dom::DOMString&) const throw ();

    virtual dom::ref<dom::NodeList> getElementsByTagNameNS(const dom::DOMString&,
                                                           const dom::DOMString&) const
        throw (dom::DOMException);


    ElemType* get_type() const throw ();

    void get_attr_value(const AttrType* t, dom::DOMString& v) const throw ();

    void set_attr_value(const AttrType* t, const dom::DOMString& v);

    void invalidate_attr_node_map() throw () {} // FIXME: Implement this using flag has_attr_node_map


    Element(ElemType*);

    virtual ~Element() throw ();


private:
    friend struct Document;

    struct Attr: ParentNode, virtual dom::Attr {
        virtual dom::DOMString getNodeName() const throw ();

        virtual dom::ref<dom::Node> getParentNode() const throw () { return dom::null; }

        virtual dom::ref<dom::Node> getPreviousSibling() const throw () { return dom::null; }

        virtual dom::ref<dom::Node> getNextSibling() const throw () { return dom::null; }

        virtual dom::DOMString getName() const throw ();

        virtual bool getSpecified() const throw ();

        virtual dom::DOMString getValue() const throw ();

        virtual void setValue(const dom::DOMString& value) throw (dom::DOMException);

        virtual dom::ref<dom::Element> getOwnerElement() const throw ();

        virtual bool isId() const throw () { return is_id(); }


        void get_value(dom::DOMString& v) const { v.clear(); } // FIXME: Implement this!

        void set_value(const dom::DOMString&) {} // FIXME: Implement this!

        bool is_id() const { return false; } // FIXME: Implement this!

        bool is_collapsible() const { return false; } // FIXME: Implement this! Requires not referenced and no attached stuff on Attr itself.


        void init(ParentNode* /*p*/, bool /*is_spec*/, bool /*is_id*/)
        {
/*
            parent = p;
            Flags f = (is_spec ? attr_is_spec : 0) | (is_id ? attr_is_id : 0);
            set_flag(f, attr_is_spec|attr_is_id);
            CONSIDER WHAT MUST HAPPEN WHEN BINDING ATTR TO ELEM
*/
        }


        Attr(Document* /*d*/): ParentNode(0) { /* set_type(&d->node_type_attr); */ }
    };

    struct AttrMap {
        bool is_compact() const { return bits; }

        void destroy()
        {
            if (is_compact()) {
                delete static_cast<CompactAttrMap*>(this);
            }
            else {
                delete static_cast<GeneralAttrMap*>(this);
            }
        }

    protected:
        AttrMap(int b): bits(b) {}

        ~AttrMap() {} // Guard

        // At least 16 bits. Zero for general map, and non-zero for
        // compact map.
        unsigned bits;
    };

    struct CompactAttrMap: AttrMap {
        static const unsigned bit_is_compact = 1<<3;
        static const int max_size = 6;
        static const unsigned bit_mask_size = 7;
        static const int per_slot_bit_offset = 4;
        static const unsigned slot_bit_is_spec = 1<<0;
        static const unsigned slot_bit_is_id   = 1<<1;
        static const int bits_per_slot = 2;
        static const unsigned slot_bit_mask = 3;
        // NOTE: per_slot_bit_offset + max_size*bits_per_slot must not be greater than 16

        CompactAttrMap(): AttrMap(bit_is_compact) {}

        bool full() const { return get_size() == max_size; }

        bool get_value(const AttrType* t, dom::DOMString& v) const
        {
            int i;
            if (!find(t,i))
                return false;
            const Slot& s = slots[i];
            if (s.attr) {
                s.attr->get_value(v);
            }
            else {
                v = s.value;
            }
            return true;
        }

        bool find(const AttrType* t, int& i) const
        {
            int n = get_size();
            for (int j = 0; j < n; ++j) {
                const Slot& s = slots[j];
                if (s.type != t)
                    continue;
                i = j;
                return true;
            }
            return false;
        }

        void set(int i, const dom::DOMString& v)
        {
            Slot& s = slots[i];
            if (!s.attr) {
                s.value = v;
                set_slot_bits(i, slot_bit_is_spec);
                return;
            }
            if (!s.attr->is_collapsible()) {
                s.attr->set_value(v);
                return;
            }
            unsigned slot_bits = slot_bit_is_spec;
            if (s.attr->is_id())
                slot_bits |= slot_bit_is_id;
            replace_slot_bits(i, slot_bits);
            // FIXME: Recycle or delete s.attr <<----------------------------------------------------------------------------------------
// ALSO REMEMBER TO DESTROY MAP WHEN ELEM IS DESTROYED
// ALSO REMEMBER TO DESTROY ATTR IN MAP ENTRIES WHEN MAP IS DESTROYED
// ALSO, IT SHALL BE ALLOWED FOR GENERAL MAP TO HAVE FEWER THAN 4 ENTRIES, THIS WOULD BE NECESSARY WHEN REMOVING AN ATTRIBUTE, AND ALLOC OF COMPACT MAP FAILS (NO THROW ALLOWED)
            s.attr = 0;
        }

        // Map must not be full, and an attribute of the specified
        // type must not already be in the map.
        void add(const AttrType* t, const dom::DOMString& v)
        {
            ARCHON_ASSERT(!full());
            // FIXME: Also assert that the attr type is not already in the map
            int i = get_size();
            Slot& s = slots[i];
            s.type = t;
            s.value = v;
            s.attr = 0;
            set_size(i+1);
            set_slot_bits(i, slot_bit_is_spec);
        }

        int get_size() const { return bits & bit_mask_size; }

        void set_size(int n) { bits = bits & ~bit_mask_size | n; }

        unsigned get_slot_bits(int i) const
        {
            int pos = per_slot_bit_offset + bits_per_slot*i;
            return bits>>pos & slot_bit_mask;
        }

        void set_slot_bits(int i, unsigned b)
        {
            bits |= b << per_slot_bit_offset + bits_per_slot*i;
        }

        void replace_slot_bits(int i, unsigned b)
        {
            int pos = per_slot_bit_offset + bits_per_slot*i;
            bits = bits & ~(slot_bit_mask<<pos) | b<<pos;
        }

        struct Slot {
            const AttrType* type;
            dom::DOMString value;
            Attr* attr; // Non-null if external reference is needed or if value is structured or if manipulated such as addition of event handler or user data or node lists
        };
        Slot slots[max_size];
    };

    struct GeneralAttrMap: AttrMap {
        GeneralAttrMap(): AttrMap(0) {}

        bool get_value(const AttrType* t, dom::DOMString& v) const
        {
            iter i = map.find(t);
            if (i == map.end())
                return false;
            i->second->get_value(v);
            return true;
        }

        typedef std::map<const AttrType*, Attr*> Map;
        typedef Map::const_iterator iter;
        Map map;
    };

    AttrMap* attr_map;

    void upgrade_attr_map()
    {
        const CompactAttrMap* old_map = static_cast<CompactAttrMap*>(attr_map);
        core::UniquePtr<GeneralAttrMap> new_map(new GeneralAttrMap);
        int n = old_map->get_size();
        try {
            Attr* prev_attr = 0;
            for (int i = 0; i < n; ++i) {
                const CompactAttrMap::Slot& s = old_map->slots[i];
                Attr* attr = s.attr;
                if (attr) {
                    new_map->map[s.type] = attr;
                }
                else {
                    unsigned slot_bits = old_map->get_slot_bits(i);
                    bool is_spec = slot_bits & CompactAttrMap::slot_bit_is_spec;
                    bool is_id   = slot_bits & CompactAttrMap::slot_bit_is_id;
                    attr = build_attr(s.type, s.value, is_spec, is_id);
                    core::UniquePtr<Attr> attr_owner(attr);
                    new_map->map[s.type] = attr;
                    attr_owner.release();
                }
                attr->prev = prev_attr;
                if (prev_attr)
                    prev_attr->next = attr;
                prev_attr = attr;
            }
            if (prev_attr)
                prev_attr->next = 0;
        }
        catch (...) {
            for (int i = 0; i < n; ++i) {
                const CompactAttrMap::Slot& s = old_map->slots[i];
                if (s.attr)
                    new_map->map.erase(s.type);
            }
            throw;
        }
        delete old_map;
        attr_map = new_map.release();
    }

    Attr* build_attr(const AttrType* /*t*/, const dom::DOMString& /*v*/, bool /*is_spec*/, bool /*is_id*/)
    {
/*
        Document* doc = get_doc();
        core::UniquePtr<Attr> a(new Attr(doc));
        a->init(this, is_spec, is_id);
        if (!v.empty()) {
            Text* text = new Text(doc, v, false);
            a->first_child = text;
            text->parent = a.get();
        }
        return a.release();
*/
        return 0;
    }
};



struct DocumentFragment: ParentNode, virtual dom::DocumentFragment {
    virtual dom::DOMString getNodeName() const throw ();

    DocumentFragment(Document*);
};




struct CharacterData: Node, virtual dom::CharacterData {
    virtual dom::DOMString getNodeValue() const throw ();

    virtual void setNodeValue(const dom::DOMString&) throw (dom::DOMException);

    virtual dom::DOMString getTextContent() const throw (dom::DOMException);

    virtual void setTextContent(const dom::DOMString&) throw (dom::DOMException);

    virtual dom::DOMString getData() const throw ();

    virtual void setData(const dom::DOMString&) throw (dom::DOMException);

    const dom::DOMString& get_data() const { return data; }

protected:
    CharacterData(NodeType* t, const dom::DOMString& d): Node(t), data(d) {}

    virtual ~CharacterData() throw () {}

private:
    dom::DOMString data;

    void set_data(const dom::DOMString& d);
};




struct Text: virtual dom::Text, CharacterData {
    virtual dom::DOMString getNodeName() const throw ();

    virtual bool isElementContentWhitespace() const throw ();


    Text(Document*, const dom::DOMString&, bool elem_cont_whitespace);

    TextType* get_type() const { return static_cast<TextType*>(Node::get_type()); }

    void accum_text_contents(dom::DOMString&) const;

protected:
    Text(TextType* t, const dom::DOMString& d): CharacterData(t,d) {}
};




struct Comment: virtual dom::Comment, CharacterData {
    virtual dom::DOMString getNodeName() const throw ();

    Comment(Document*, const dom::DOMString&);
};




struct CDATASection: virtual dom::CDATASection, Text {
    virtual dom::DOMString getNodeName() const throw ();

    CDATASection(Document*, const dom::DOMString&, bool elem_cont_whitespace);
};




struct ProcessingInstruction: Node, virtual dom::ProcessingInstruction {
    virtual dom::DOMString getNodeName() const throw ();

    virtual dom::DOMString getNodeValue() const throw ();

    virtual void setNodeValue(const dom::DOMString&) throw (dom::DOMException);

    virtual dom::DOMString getTextContent() const throw (dom::DOMException);

    virtual void setTextContent(const dom::DOMString&) throw (dom::DOMException);

    virtual dom::DOMString getTarget() const throw ();

    virtual dom::DOMString getData() const throw ();

    virtual void setData(const dom::DOMString&) throw (dom::DOMException);


    ProcessingInstruction(Document*, const dom::DOMString&, const dom::DOMString&);

    virtual ~ProcessingInstruction() throw () {}

private:
    const dom::DOMString target;
    dom::DOMString data;

    void set_data(const dom::DOMString&);
};




// Note that a document type node does not necessarily have an
// associated document.
struct DocumentType: Node, virtual dom::DocumentType {
    struct NamedNodeMap;

    const dom::ref<DOMImplementation> impl;


    virtual dom::DOMString getNodeName() const throw ();

    virtual dom::ref<dom::NodeList> getChildNodes() const throw ();

    virtual bool isSupported(const dom::DOMString&, const dom::DOMString&) const throw ();

    virtual dom::DOMString getName() const throw ();

    virtual dom::ref<dom::NamedNodeMap> getEntities() const throw ();

    virtual dom::ref<dom::NamedNodeMap> getNotations() const throw ();

    virtual dom::DOMString getPublicId() const throw ();

    virtual dom::DOMString getSystemId() const throw ();

    virtual dom::DOMString getInternalSubset() const throw ();


    // The new document type will not be bound to a document initially.
    DocumentType(DOMImplementation*, const dom::DOMString&,
                 const dom::DOMString&, const dom::DOMString&);

    virtual ~DocumentType() throw ();


    void add_entity(const dom::DOMString& name, const dom::DOMString& public_id,
                    const dom::DOMString& system_id, const dom::DOMString& notation_name);

    void add_notation(const dom::DOMString& name, const dom::DOMString& public_id,
                      const dom::DOMString& system_id);

    void set_internal_subset(const dom::DOMString& v) { internal_subset = v; }


    // Must be called before the document type node is added to a document.
    void bind_to_document(Document*) throw ();


private:
    const dom::DOMString name, public_id, system_id;

    core::UniquePtr<NodeType> node_type_entity, node_type_notation;

    const core::UniquePtr<NamedNodeMap> entities, notations;

    dom::DOMString internal_subset;

    class DegenChildList;
    mutable core::UniquePtr<DegenChildList> degen_child_list;


    // Overriding Node::on_referenced().
    virtual void on_referenced() const throw ();

    // Overriding Node::on_unreferenced().
    virtual void on_unreferenced() const throw ();
};




struct Entity: virtual dom::Entity, ParentNode {
    virtual dom::DOMString getNodeName() const throw ();

    virtual dom::DOMString getPublicId() const throw ();

    virtual dom::DOMString getSystemId() const throw ();

    virtual dom::DOMString getNotationName() const throw ();

    virtual dom::DOMString getInputEncoding() const throw ();

    virtual dom::DOMString getXmlEncoding() const throw ();

    virtual dom::DOMString getXmlVersion() const throw ();

    virtual ~Entity() throw () {}

private:
    DocumentType::NamedNodeMap* const doctype_map;

    const dom::DOMString name, public_id, system_id, notation_name;


    friend class DocumentType;

    Entity(NodeType*, DocumentType::NamedNodeMap*, const dom::DOMString&,
           const dom::DOMString&, const dom::DOMString&, const dom::DOMString&);


    /// Overriding method in Node because the memory management parent
    /// of an entity is the named node map that contains it.
    virtual void on_referenced() const throw ();

    /// Overriding method in Node because the memory management parent
    /// of an entity is the named node map that contains it.
    virtual void on_unreferenced() const throw ();
};




struct Notation: virtual dom::Notation, Node {
    virtual dom::DOMString getNodeName() const throw ();

    virtual dom::DOMString getPublicId() const throw ();

    virtual dom::DOMString getSystemId() const throw ();

    virtual ~Notation() throw () {}

private:
    DocumentType::NamedNodeMap* const doctype_map;

    const dom::DOMString name, public_id, system_id;


    friend class DocumentType;

    Notation(NodeType*, DocumentType::NamedNodeMap*, const dom::DOMString&,
             const dom::DOMString&, const dom::DOMString&);


    /// Overriding method in Node because the memory management parent
    /// of a notation is the named node map that contains it.
    virtual void on_referenced() const throw ();

    /// Overriding method in Node because the memory management parent
    /// of a notation is the named node map that contains it.
    virtual void on_unreferenced() const throw ();
};




struct Document: virtual dom::Document, ParentNode {
    const dom::ref<DOMImplementation> impl;

    NodeType node_type_doc, node_type_doc_frag, node_type_comment, node_type_proc_instr,
        node_type_doctype, node_type_abstract_elem;

    TextType text_type_normal, text_type_elem_cont_whitespace,
        cdata_type_normal, cdata_type_elem_cont_whitespace;


    enum XmlVersion { xml_ver_1_0, xml_ver_1_1 };


    virtual dom::DOMString getNodeName() const throw ();

    virtual dom::ref<dom::Document> getOwnerDocument() const throw ();

    // Overrides method in ParentNode.
    virtual dom::DOMString getTextContent() const throw (dom::DOMException);

    // Overrides method in ParentNode.
    virtual void setTextContent(const dom::DOMString&) throw (dom::DOMException) {}

    virtual dom::ref<dom::DocumentType> getDoctype() const throw ();

    virtual dom::ref<dom::DOMImplementation> getImplementation() const throw ();

    virtual dom::ref<dom::Element> getDocumentElement() const throw ();

    virtual dom::ref<dom::Element> createElement(const dom::DOMString&) const
        throw (dom::DOMException);

    virtual dom::ref<dom::DocumentFragment> createDocumentFragment() const throw ();

    virtual dom::ref<dom::Text> createTextNode(const dom::DOMString&) const throw ();

    virtual dom::ref<dom::Comment> createComment(const dom::DOMString&) const throw ();

    virtual dom::ref<dom::CDATASection> createCDATASection(const dom::DOMString&) const
        throw(dom::DOMException);

    virtual dom::ref<dom::ProcessingInstruction>
    createProcessingInstruction(const dom::DOMString&, const dom::DOMString&) const
        throw (dom::DOMException);

    virtual dom::ref<dom::NodeList> getElementsByTagName(const dom::DOMString&) const throw ();

    virtual dom::ref<dom::Element>
    createElementNS(const dom::DOMString&, const dom::DOMString&) const
        throw (dom::DOMException);

    virtual dom::ref<dom::NodeList> getElementsByTagNameNS(const dom::DOMString&,
                                                           const dom::DOMString&) const
        throw ();

    virtual dom::DOMString getInputEncoding() const throw ();

    virtual dom::DOMString getXmlEncoding() const throw ();

    virtual bool getXmlStandalone() const throw ();

    virtual void setXmlStandalone(bool) throw (dom::DOMException);

    virtual dom::DOMString getXmlVersion() const throw ();

    virtual void setXmlVersion(const dom::DOMString&) throw (dom::DOMException);

    virtual dom::DOMString getDocumentURI() const throw ();

    virtual void setDocumentURI(const dom::DOMString&) throw ();

    virtual dom::ref<dom::Node> adoptNode(const dom::ref<dom::Node>& source) const
        throw (dom::DOMException);


    virtual void approve_child(const Node*) const;

    virtual void approve_children(const DocumentFragment*) const;

    // Overriding method in ParentNode
    virtual void before_children_change() throw ()
    {
        ParentNode::before_children_change();
        valid_doctype_and_root = false;
    }


    XmlVersion get_xml_ver() const { return xml_version; }

    Element* get_root() const throw ()
    {
        if (!valid_doctype_and_root)
            find_doctype_and_root();
        return root;
    }


    // The caller must ensure that this document stays alive for as
    // long as the returned element type stays alive.
    ElemTypeRef get_elem_type(const dom::DOMString& ns,
                              const dom::DOMString& tag_name, bool dom1) const;


    // The caller must ensure that this document stays alive for at
    // least as long as the returned element type stays alive. The
    // specified local name must be empty if and only if the element
    // is being created by a DOM Level 1 method. The final values of
    // the fields of the returned element type must be a function
    // only of the specified arguments and of properties that are
    // guaranteed to be constant throughout the lifetime of the
    // document instance.
    virtual ElemTypeRef create_elem_type(bool read_only, const ElemKey& key,
                                         const dom::DOMString& prefix,
                                         const dom::DOMString& local_name) const;


    // Validate the specified name according the XML version of this
    // document, and split it into a prefix and a local name.
    static void parse_qualified_name(XmlVersion v, const dom::DOMString& n,
                                     dom::DOMString& prefix, dom::DOMString& local_name);


    // Intended to be called by a parser immediately after if has
    // created a document.
    void set_doc_info(const dom::DOMString& doc_uri, const dom::DOMString& input_enc,
                      XmlVersion v, const dom::DOMString& xml_enc, bool standalone);


    /// Create a new element and append it to the list of children of
    /// the specified parent. This method is faster than using the
    /// standard DOM API, but it does no checking. It is intended to be
    /// used by a parser as it constructs the DOM structure. The new
    /// element is returned without any external references to it, and
    /// therefore it is owned only by its parent. For this reason the
    /// application (the parser) must be sure not to manipulate the DOM
    /// in any way that will cause the child to be destroyed, unless it
    /// obtains an extra reference count itself. The caller must ensure
    /// that the specified XML names are valid, that no cycles are
    /// created, and that the parent allows another child element. This
    /// method must not be used for read-only elements.
    ///
    /// If \a local_name is empty, then a DOM Level 1 element is
    /// created, and \a ns and \a prefix must then also be empty.
    Element *create_elem_child_for_parser(ParentNode* parent, const dom::DOMString& ns,
                                          const dom::DOMString& tag_name,
                                          const dom::DOMString& prefix,
                                          const dom::DOMString& local_name);


    Element::CompactAttrMap* new_compact_attr_map() // FIXME: Implement caching of a few instances
    {
        return new Element::CompactAttrMap;
    }

    Document(DOMImplementation*);

    virtual ~Document() throw ();


protected:
    // Overriding method in Node due to the document being the
    // memory management root of the node hierarchy.
    virtual void on_referenced() const throw ();

    // Overriding method in Node due to the document being the
    // memory management root of the node hierarchy.
    virtual void on_unreferenced() const throw ();


    void approve_children(const Node*, bool only_one) const;


private:
    friend class Node;
    friend class ParentNode;
    friend class ChildList;
    friend class DOMImplementation;

    struct ElemTypeHashFunc {
        static int hash(const ElemKey& k, int n)
        {
            util::Hash_FNV_1a_32 h;
            h.add_string(k.tag_name);
            return h.get_hash(n);
        }
    };

    typedef util::HashMap<ElemKey, ElemType*, ElemTypeHashFunc> ElemTypes;
    mutable ElemTypes elem_types;

    dom::DOMString document_uri;
    dom::DOMString input_encoding;

    XmlVersion xml_version;
    dom::DOMString xml_encoding;
    bool xml_standalone;

    mutable bool valid_doctype_and_root;
    mutable DocumentType* doctype;
    mutable Element* root;

    struct UnusedRareNodeData {
        UnusedRareNodeData(DOMImplementation* i): impl(i)
        {
            vec.reserve(max_entries);
        }

        ~UnusedRareNodeData();

        RareNodeData* get();

        void put(RareNodeData* r) throw ();

    private:
        typedef std::vector<RareNodeData*> Vec;
        Vec vec;
        static const Vec::size_type max_entries = 16;
        DOMImplementation* const impl;
    };

    UnusedRareNodeData unused_rare_node_data;

    ChildListManager child_list_manager;


/*
    struct FilteredElemListEntry {
        ElemTypeKey elem_type;
        CachedNodeList* list;
        FilteredElemListEntry(const ElemTypeKey& t, CachedNodeList* l):
            elem_type(t), list(l) {}
    };

    typedef std::multimap<const ParentNode*, FilteredElemListEntry> FilteredElemLists;

    ChildNodeLists    child_node_lists;
    FilteredElemLists filtered_elem_lists;
*/


    // Validate the specified XML name according the the specified
    // XML version.
    static void validate_xml_name(XmlVersion v, const dom::DOMString& name);


    friend class ElemType;
    void unregister_elem_type(ElemType* t)
    {
        elem_types.erase(t->key);
    }

    void find_doctype_and_root() const throw ();


    RareNodeData* acquire_rare_node_data()
    {
        return unused_rare_node_data.get();
    }

    // r->num_objs must be zero.
    void release_rare_node_data(RareNodeData* r) throw ()
    {
        unused_rare_node_data.put(r);
    }
};




// All unprotected fields of an implementation must be constant to
// ensure thread-safty.
struct DOMImplementation: virtual dom::DOMImplementation {
    const dom::DOMString str_feat_core, str_feat_xml, str_feat_xml_ver;
    const dom::DOMString str_ver_1_0, str_ver_1_1, str_ver_2_0, str_ver_3_0;
    const dom::DOMString str_node_name_doc_frag, str_node_name_text, str_node_name_comment,
        str_node_name_cdata, str_node_name_doc;
    const dom::DOMString str_ns_namespace, str_ns_xmlns;
    const dom::DOMString str_xml, str_xmlns;

    NodeType node_type_unbound_doctype;

    virtual bool hasFeature(const dom::DOMString&, const dom::DOMString&) const throw ();

    virtual dom::ref<dom::DocumentType> createDocumentType(const dom::DOMString&,
                                                           const dom::DOMString&,
                                                           const dom::DOMString&) const
        throw (dom::DOMException);

    virtual dom::ref<dom::Document>
    createDocument(const dom::DOMString& ns, const dom::DOMString& name,
                   const dom::ref<dom::DocumentType>& doctype) const throw (dom::DOMException);


    // Caller must pass an upper case feature name
    virtual bool has_feature(const dom::DOMString&, const dom::DOMString&) const throw ();

    // Throws NOT_SUPPORTED_ERR if the version is not supported.
    Document::XmlVersion parse_xml_ver(const dom::DOMString& v) const;

    virtual dom::ref<Document> create_document(const dom::DocumentType*) const;

    static bool is_whitespace(const dom::DOMString&); // #x20 | #x9 | #xD | #xA


    /// If you choose to override this method, you must strongly
    /// consider to also override destroy_rare_node_data() and
    /// clear_nonessential_rare_node_data().
    ///
    /// An overriding method should not call the overridden method.
    virtual RareNodeData* create_rare_node_data() const
    {
        return new RareNodeData;
    }

    /// May assume that the specified rare node data container is
    /// empty.
    ///
    /// An overriding method should not call the overridden method.
    virtual void destroy_rare_node_data(RareNodeData* r) const throw ()
    {
        delete r;
    }

    /// An overriding method must call the overridden method.
    virtual void clear_nonessential_rare_node_data(ParentNode* p) throw ()
    {
        p->get_doc()->child_list_manager.discard_if_unref(p);
    }


    DOMImplementation();

    virtual ~DOMImplementation() throw () {}
};







// Implementation:

inline void ElemType::unbind_ref() throw ()
{
    if (--ref_count == 0) {
        doc->unregister_elem_type(this);
        delete this;
    }
}



inline Node::~Node() throw ()
{
    if (rare_data)
        type->doc->release_rare_node_data(rare_data);
}



template<class N, class T> inline void Node::ensure_rare_obj(const N* n, dom::ref<T>& r)
{
    ARCHON_ASSERT(n->type->id != Node::DOCUMENT_TYPE_NODE);
    if (!n->rare_data)
        n->rare_data = n->type->doc->acquire_rare_node_data();
    try {
        n->rare_data->template ensure_obj<N,T>(const_cast<N*>(n), r);
    }
    catch (...) {
        if (n->rare_data->num_objs == 0) {
            n->type->doc->release_rare_node_data(n->rare_data);
            n->rare_data = 0;
        }
        throw;
    }
}



template<class T> inline void Node::remove_rare_obj()
{
    ARCHON_ASSERT(rare_data && rare_data->get_obj<T>());
    if (rare_data->remove_obj<T>(this)) {
        type->doc->release_rare_node_data(rare_data);
        rare_data = 0;
    }
}



inline void ParentNode::detect_cycle(const ParentNode* child) const
{
    const ParentNode* p = this;
    do {
        if (p == child)
            throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR,
                                    "Attempt to create cycle");
        p = p->get_parent();
    }
    while (p);
}



inline void ParentNode::before_children_change() throw ()
{
    Document* doc = get_doc();
    doc->child_list_manager.invalidate(this);
}



template<ParentNode::AddMode mode>
inline void ParentNode::add_child(dom::Node *new_child, dom::Node *ref_child)
{
    Document* doc = get_doc();

    if (Node* c = dynamic_cast<Node*>(new_child)) {
        NodeType* t = c->get_type();
        if (t->is_child_node) {

            ParentNode* p = c->get_parent();
            if (p != this) {
                approve_child(c);

                if (t->is_parent_node)
                    detect_cycle(static_cast<ParentNode*>(c));

                Document* d = t->doc;
                if (d && d != doc)
                    throw dom::DOMException(dom::WRONG_DOCUMENT_ERR,
                                            "Parent and child are tied to different documents");
            }

            if (is_read_only())
                throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR,
                                        "Cannot add child to read-only parent");

            if (p && p->is_read_only())
                throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR,
                                        "Child cannot be removed from its current parent");

            // Find the reference node
            Node* ref;
            if (mode != add_mode_Append) {
                ref = dynamic_cast<Node*>(ref_child);
                if (!ref || ref->get_parent() != this)
                    throw dom::DOMException(dom::NOT_FOUND_ERR,
                                            "Reference child is not a child of this parent");
                if (ref == c)
                    return; // Noting to do
            }

            // Bind unbound document type nodes to the document
            if (!p && !t->doc) {
                ARCHON_ASSERT(dynamic_cast<DocumentType*>(c));
                static_cast<DocumentType*>(c)->bind_to_document(doc);
            }

            // No exceptions allowed beyond this point

            // Notify both parents about the imminent change
            if (p)
                p->before_children_change();
            before_children_change();

            // Detach child from its current parent
            if (p) {
                p->low_level_remove_child(c);
                p->unbind_ref();
            }
            else {
                doc->unbind_ref();
            }

            // Attach child to this parent
            if (mode == add_mode_Append) {
                bind_ref();
                low_level_append_child(c);
            }
            else if (mode == add_mode_InsertBefore) {
                bind_ref();
                low_level_insert_before(c, ref);
            }
            else if (mode == add_mode_Replace) {
                doc->bind_ref();
                low_level_replace_child(c, ref);
                ref->parent = 0;
                ref->prev = ref;
                ref->next = 0;
            }
            else {
                ARCHON_ASSERT(false);
            }

            return;
        }

        if (t->id == Node::DOCUMENT_FRAGMENT_NODE) {

            DocumentFragment* frag = static_cast<DocumentFragment*>(c);

            approve_children(frag);

            if (frag != this)
                detect_cycle(frag);

            if (frag->get_doc() != doc)
                throw dom::DOMException(dom::WRONG_DOCUMENT_ERR,
                                        "Parent and fragment are tied to different documents");

            if (is_read_only())
                throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR,
                                        "Cannot add fragment to read-only parent");

            // Find the reference node
            Node* ref;
            if (mode != add_mode_Append) {
                ref = dynamic_cast<Node*>(ref_child);
                if (!ref || ref->get_parent() != this)
                    throw dom::DOMException(dom::NOT_FOUND_ERR,
                                            "Reference child is not a child of this parent");
            }

            if (frag == this)
                return; // Nothing to do
            Node* c = frag->get_first_child();
            if (!c)
                return; // Nothing to do

            // No exceptions allowed beyond this point

            // Notify both parents about the imminent change
            frag->before_children_change();
            before_children_change();

            // Update parent pointer of transferreed children and count
            // number of referenced children
            int num_referenced_chidren = 0;
            {
                Node* c2 = c;
                do {
                    c2->parent = this;
                    if (c2->is_referenced())
                        ++num_referenced_chidren;
                    c2 = c2->next;
                }
                while (c2);
            }

            // Detach children from document fragment
            frag->first_child = 0;
            frag->unbind_ref_n(num_referenced_chidren);

            // Hook the new children into the current list of children of
            // this parent
            bind_ref_n(num_referenced_chidren);
            if (mode == add_mode_Append) {
                if (first_child) {
                    Node* old_last = first_child->prev;
                    Node* new_last = c->prev;
                    old_last->next = c;
                    c->prev = old_last;
                    first_child->prev = new_last;
                }
                else {
                    first_child = c;
                }
            }
            else if (mode == add_mode_InsertBefore) {
                Node* last = c->prev;
                Node* prev = ref->prev;
                (ref == first_child ? first_child : prev->next) = c;
                c->prev = prev;
                last->next = ref;
                ref->prev = last;
            }
            else if (mode == add_mode_Replace) {
                doc->bind_ref();
                Node* last = c->prev;
                Node* prev = ref->prev;
                Node* next = ref->next;
                (ref == first_child ? first_child : prev->next) = c;
                c->prev = prev;
                last->next = next;
                if (next)
                    next->prev = last;
                ref->parent = 0;
                ref->prev = ref;
                ref->next = 0;
                unbind_ref();
            }
            else {
                ARCHON_ASSERT(false);
            }

            return;
        }
    }

    throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "Bad child type");
}



inline void ParentNode::append_child_for_parser(Node* c) throw ()
{
    c->parent = this;
    if (first_child) {
        Node* last = first_child->prev;
        c->prev = last;
        first_child->prev = last->next = c;
    }
    else {
        first_child = c;
    }
}



inline void ParentNode::accum_text_contents(dom::DOMString& s) const
{
    Node* c = first_child;
    while (c) {
        switch (c->get_type()->id) {
            case ELEMENT_NODE:
                static_cast<Element*>(c)->accum_text_contents(s);
                break;
            case ENTITY_REFERENCE_NODE:
//                static_cast<EntityReference*>(c)->accum_text_contents(s);         FIXME: Implement this!!!!!!!!
                break;
            case TEXT_NODE:
            case CDATA_SECTION_NODE:
                static_cast<Text*>(c)->accum_text_contents(s);
                break;
        }
        c = c->get_next_sibling();
    }
}



inline void ParentNode::clear_nonessential_rare_data() throw ()
{
    if (rare_data)
        get_doc()->impl->clear_nonessential_rare_node_data(this);
}



inline ParentNode::~ParentNode() throw ()
{
    // This is because when the binding between the list object and
    // this node is broken, the list object needs to access this
    // parent node while it is still a parent node.
    get_doc()->child_list_manager.discard_if_unref(this);

    destroy_children();
}



inline void ParentNode::low_level_remove_child(Node* c) throw ()
{
    if (c->prev->next) { // Not first
        c->prev->next = c->next;
        (c->next ? c->next : first_child)->prev = c->prev;
    }
    else { // Is first
        first_child = c->next;
        if (c->next)
            c->next->prev = c->prev;
    }
}



inline void ParentNode::low_level_append_child(Node* c) throw ()
{
    c->parent = this;
    c->next   = 0;
    if (first_child) {
        Node* last = first_child->prev;
        c->prev = last;
        first_child->prev = last->next = c;
    }
    else {
        first_child = c->prev = c;
    }
}



inline void ParentNode::low_level_insert_before(Node* c, Node* ref) throw ()
{
    c->parent = this;
    Node* prev = ref->prev;
    c->prev = prev;
    c->next = ref;
    (ref == first_child ? first_child : prev->next) = ref->prev = c;
}



inline void ParentNode::low_level_replace_child(Node* c, Node* ref) throw ()
{
    c->parent = this;
    Node* prev = ref->prev;
    Node* next = ref->next;
    (ref == first_child ? first_child : prev->next) = c;
    c->prev = prev;
    c->next = next;
    if (next)
        next->prev = c;
}



inline void ParentNode::destroy_children() throw ()
{
    Node* c = first_child;
    while (c) {
        Node* d = c->get_next_sibling();
        delete c;
        c = d;
    }
    first_child = 0;
}



inline void ChildList::acquire(const Node* n, dom::ref<ChildList>& l)
{
    n->get_type()->doc->child_list_manager.acquire(n,l);
}



inline ChildListManager::~ChildListManager()
{
    int n = unref_queue.size();
    for (int i = 0; i < n; ++i)
        delete unref_queue.get(i);
}



inline ElemType* Element::get_type() const throw ()
{
    return static_cast<ElemType*>(Node::get_type());
}



inline void Element::get_attr_value(const AttrType* t, dom::DOMString& v) const throw ()
{
    if (attr_map) {
        if (attr_map->is_compact()) {
            if (static_cast<CompactAttrMap*>(attr_map)->get_value(t,v)) return;
        }
        else {
            if (static_cast<GeneralAttrMap*>(attr_map)->get_value(t,v)) return;
        }
    }
    get_type()->get_attr(t,v);
}



inline void Element::set_attr_value(const AttrType* t, const dom::DOMString& v)
{
    if (!attr_map) {
        attr_map = get_doc()->new_compact_attr_map();
      add_compact:
        static_cast<CompactAttrMap *>(attr_map)->add(t,v);
        invalidate_attr_node_map();
        return;
    }
    if (attr_map->is_compact()) {
        CompactAttrMap* m = static_cast<CompactAttrMap*>(attr_map);
        int i;
        if (m->find(t,i)) {
            m->set(i,v);
            return;
        }
        if (!m->full())
            goto add_compact;
        upgrade_attr_map();
        invalidate_attr_node_map();
        bool is_spec = true;
        bool is_id   = t->is_id();
        core::UniquePtr<Attr> a(build_attr(t, v, is_spec, is_id));
//        static_cast<GeneralAttrMap*>(attr_map)->add(t, a.get());
        a.release();
        return;
    }
//      if (static_cast<GeneralAttrMap*>(attr_map)->add_or_set(t,v))
//          invalidate_attr_node_map();

//      WHOOPS: how to cache current iter pos in NamedNodeMap - simple, have different ref modes (cache is invalidated if the map type changes)
}



inline Element::Element(ElemType* t): ParentNode(t), attr_map(0)
{
    t->bind_ref();
}



inline Element::~Element() throw ()
{
    ElemType* t = get_type();
    type = &t->doc->node_type_abstract_elem;
    t->unbind_ref();
}



inline DocumentFragment::DocumentFragment(Document* d): ParentNode(&d->node_type_doc_frag) {}



inline void CharacterData::set_data(const dom::DOMString& d)
{
    if (is_read_only())
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR,
                                "Cannot change data of read-only character data node");
    data = d;
}



inline Text::Text(Document* d, const dom::DOMString& e, bool elem_cont_whitespace):
    CharacterData(elem_cont_whitespace ? &d->text_type_elem_cont_whitespace :
                  &d->text_type_normal, e)
{
}



inline void Text::accum_text_contents(dom::DOMString& s) const
{
    if (!get_type()->elem_cont_whitespace)
        s += get_data();
}



inline Comment::Comment(Document* d, const dom::DOMString& e):
    CharacterData(&d->node_type_comment, e)
{
}



inline CDATASection::CDATASection(Document* d, const dom::DOMString& e,
                                  bool elem_cont_whitespace):
    Text(elem_cont_whitespace ? &d->cdata_type_elem_cont_whitespace :
         &d->cdata_type_normal, e)
{
}



inline ProcessingInstruction::ProcessingInstruction(Document* d, const dom::DOMString& t,
                                                    const dom::DOMString& e):
    Node(&d->node_type_proc_instr), target(t), data(e)
{
}



inline void ProcessingInstruction::set_data(const dom::DOMString& d)
{
    if (is_read_only())
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR,
                                "Cannot change data of read-only processing instruction");
    data = d;
}



inline Document::UnusedRareNodeData::~UnusedRareNodeData()
{
    Vec::iterator end = vec.end();
    for (Vec::iterator i = vec.begin(); i != end; ++i)
        impl->destroy_rare_node_data(*i);
}



inline RareNodeData *Document::UnusedRareNodeData::get()
{
    if (vec.empty())
        return impl->create_rare_node_data();
    RareNodeData* r = vec.back();
    vec.pop_back();
    return r;
}



inline void Document::UnusedRareNodeData::put(RareNodeData* r) throw ()
{
    ARCHON_ASSERT(r->is_empty());
    if (vec.size() < max_entries) {
        vec.push_back(r);
    }
    else {
        impl->destroy_rare_node_data(r);
    }
}



inline void Document::find_doctype_and_root() const throw ()
{
    DocumentType* d = 0;
    Element* r = 0;
    Node* c = get_first_child();
    while (c) {
        switch (c->get_type()->id) {
            case DOCUMENT_TYPE_NODE:
                ARCHON_ASSERT(dynamic_cast<DocumentType*>(c));
                d = static_cast<DocumentType*>(c);
                break;
            case ELEMENT_NODE:
                ARCHON_ASSERT(dynamic_cast<Element*>(c));
                r = static_cast<Element*>(c);
                break;
        }
        c = c->get_next_sibling();
    }
    doctype = d;
    root    = r;
    valid_doctype_and_root = true;
}



inline bool DOMImplementation::is_whitespace(const dom::DOMString& s)
{
    typedef dom::DOMString::const_iterator iter;
    typedef dom::DOMString::traits_type traits;
    typedef traits::int_type int_type;
    iter end = s.end();
    for (iter i = s.begin(); i != end; ++i) {
        int_type v = traits::to_int_type(*i);
        if (0x0D < v ? v != 0x20 : v != 0x0A && v != 0x0D && v != 0x0D)
            return false;
    }
    return true;
}


} // namespace DomImpl
} // namespace archon

#endif // ARCHON_DOM_IMPL_CORE_HPP
