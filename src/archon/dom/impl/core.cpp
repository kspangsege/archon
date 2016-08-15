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

/// \file
///
/// \author Kristian Spangsege

#include <stdexcept>
#include <map>

#include <archon/core/assert.hpp>
#include <archon/core/memory.hpp>
#include <archon/dom/impl/util.hpp>
#include <archon/dom/impl/core.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::dom_impl;

namespace dom = archon::dom;


namespace {

inline void validate_xmlns(DOMImplementation* impl,
                           const dom::DOMString& ns, const dom::DOMString& name)
{
    bool ns_is_xmlns   = ns   == impl->str_ns_xmlns;
    bool name_is_xmlns = name == impl->str_xmlns;
    if (ns_is_xmlns != name_is_xmlns)
        throw dom::DOMException(dom::NAMESPACE_ERR, "Namespace must be "
                                "'http://www.w3.org/2000/xmlns/' when, and only when "
                                "the qualified name (or its prefix) is 'xmlns'");
}



/*
class FilteredElemList: public ParentNode::CachedNodeList {
public:
    virtual dom::ref<dom::Node> item(dom::uint32 index) const throw ()
    {
    }

    virtual dom::uint32 getLength() const throw ()
    {
    }

protected:
    virtual bool matches(Element*) const = 0;
};



// With Java DOM getElementsByTagNameNS("*","x") will not return any DOM node created with a DOM Level 1 method because they do not have any local name
// On the other hand getElementsByTagNameNS("*","*") will return DOM nodes created with a DOM Level 1
// Thus, a '*' in place of a local name matches anything, even an undefined value.
// "" in place of a namespace URI does match a DOM node created with a DOM Level 1 method
// "" in place of a local name does not match a DOM node created with a DOM Level 1 method
class TagNameList: public FilteredElemList {
public:
    virtual bool matches(Element* elem) const
    {
    }
};
*/

} // anonymous namespace



namespace archon {
namespace dom_impl {

Element* ElemType::create_element()
{
    return new Element(this);
}



dom::uint16 Node::getNodeType() const throw ()
{
    return type->id;
}



dom::ref<dom::Node> Node::getParentNode() const throw ()
{
    return dom::ref<dom::Node>(parent);
}



dom::ref<dom::NodeList> Node::getChildNodes() const throw ()
{
    // The following ensures that when the application has two child
    // list references, then the references are equal if, and only if
    // the target nodes are the same. Document type nodes need special
    // handling since they are not guaranteed to be attached to a
    // document.
    ARCHON_ASSERT(get_type()->id != DOCUMENT_TYPE_NODE);
    dom::ref<ChildList> l;
    ensure_rare_obj(this, l);
    return l;
}



dom::ref<dom::Node> Node::getPreviousSibling() const throw ()
{
    return dom::ref<dom::Node>(prev->next ? prev : 0);
}



dom::ref<dom::Node> Node::getNextSibling() const throw ()
{
    return dom::ref<dom::Node>(next);
}



dom::ref<dom::Document> Node::getOwnerDocument() const throw ()
{
    return dom::ref<dom::Document>(type->doc);
}



bool Node::isSupported(const dom::DOMString& f, const dom::DOMString& v) const throw ()
{
    return type->doc->impl->hasFeature(f,v);
}



// Overriding dom::Object::on_referenced()
void Node::on_referenced() const throw ()
{
    if (parent) {
        parent->bind_ref();
    }
    else {
        type->doc->bind_ref();
    }
}



// Overriding dom::Object::on_unreferenced()
void Node::on_unreferenced() const throw ()
{
    if (parent) {
        parent->unbind_ref();
    }
    else {
        Document* d = type->doc;
        delete this;
        d->unbind_ref();
    }
}



dom::ref<dom::Node> ParentNode::getFirstChild() const throw ()
{
    return dom::ref<dom::Node>(first_child);
}



dom::ref<dom::Node> ParentNode::getLastChild() const throw ()
{
    return dom::ref<dom::Node>(first_child ? first_child->get_prev_sibling() : 0);
}



dom::ref<dom::Node> ParentNode::insertBefore(const dom::ref<dom::Node>& n,
                                             const dom::ref<dom::Node>& ref)
    throw (dom::DOMException)
{
    if (ref) {
        add_child<add_mode_InsertBefore>(n.get(), ref.get());
    }
    else {
        add_child<add_mode_Append>(n.get());
    }
    return n;
}



dom::ref<dom::Node> ParentNode::replaceChild(const dom::ref<dom::Node>& n,
                                             const dom::ref<dom::Node>& ref)
    throw (dom::DOMException)
{
    add_child<add_mode_Replace>(n.get(), ref.get());
    return ref;
}



dom::ref<dom::Node> ParentNode::removeChild(const dom::ref<dom::Node>& n)
    throw (dom::DOMException)
{
    if (is_read_only())
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR,
                                "Cannot remove child from read-only parent");

    dom::Node* n2 = n.get();
    if (Node* c = dynamic_cast<Node*>(n2)) {
        ParentNode* p = c->get_parent();
        if (p == this) {
            Document* doc = get_doc();
            before_children_change();
            low_level_remove_child(c);
            unbind_ref();
            doc->bind_ref();
            c->parent = 0;
            c->prev = c;
            c->next = 0;
            return n;
        }
    }

    throw dom::DOMException(dom::NOT_FOUND_ERR, "No such child");
}



dom::ref<dom::Node> ParentNode::appendChild(const dom::ref<dom::Node>& n)
    throw (dom::DOMException)
{
    add_child<add_mode_Append>(n.get());
    return n;
}



dom::DOMString ParentNode::getTextContent() const throw (dom::DOMException)
{
    dom::DOMString s;
    accum_text_contents(s);
    return s;
}



void ParentNode::setTextContent(const dom::DOMString& s) throw (dom::DOMException)
{
    if (is_read_only())
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR, "Parent node is read-only");
    dom::uint16 t = get_type()->id;
    ARCHON_ASSERT(t == ELEMENT_NODE || t == ATTRIBUTE_NODE);
    Document* doc = get_doc();
    Text* new_text_node = s.empty() ? 0 : new Text(doc, s, false);

    // No exceptions allowed beyond this point

    before_children_change();
    int num_referenced = 0;
    Node* c = first_child;
    while (c) {
        Node* n = c->get_next_sibling();
        if (c->is_referenced()) {
            c->parent = 0;
            c->prev = c;
            c->next = 0;
            ++num_referenced;
        }
        else {
            delete c;
        }
        c = n;
    }
    unbind_ref_n(num_referenced);
    doc->bind_ref_n(num_referenced);
    first_child = new_text_node;
    if (new_text_node)
        new_text_node->parent = this;
}



inline void ParentNode::approve_child(const Node* child) const
{
    dom::uint16 type_id = child->get_type()->id;
    // The only type of child node that is not accepted as a child of
    // an element is DocumentType
    if (type_id == DOCUMENT_TYPE_NODE)
        throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "Bad child type");
    ARCHON_ASSERT(type_id == ELEMENT_NODE ||
                  type_id == TEXT_NODE ||
                  type_id == COMMENT_NODE ||
                  type_id == PROCESSING_INSTRUCTION_NODE ||
                  type_id == CDATA_SECTION_NODE ||
                  type_id == ENTITY_REFERENCE_NODE);
}



void ParentNode::approve_children(const DocumentFragment* frag) const
{
    Node* c = frag->get_first_child();
    while (c) {
        ParentNode::approve_child(c);
        c = c->get_next_sibling();
    }
}



void ChildList::on_referenced() const throw ()
{
    node->bind_ref();
    ChildListManager& manager = node->get_type()->doc->child_list_manager;
    if (manager.unref_queue.get_first() == this) { // Heuristic search optimization
        manager.unref_queue.remove_first();
    }
    else {
        manager.unref_queue.remove(const_cast<ChildList*>(this));
    }
}



void ChildList::on_unreferenced() const throw ()
{
    const Node* n = node;
    ChildListManager& manager = n->get_type()->doc->child_list_manager;
    if (is_valid()) {
        if (manager.unref_queue.full()) {
            ChildList* clobbered = manager.unref_queue.get_first();
            manager.unref_queue.remove_first();
            if (clobbered->is_bound()) {
                const_cast<Node*>(clobbered->node)->remove_rare_obj<ChildList>();
                if (clobbered->parent_node)
                    clobbered->parent_node->clear_flag(ParentNode::valid_child_list);
            }
            delete clobbered;
        }
        manager.unref_queue.append(const_cast<ChildList*>(this));
    }
    else {
        const_cast<Node*>(n)->remove_rare_obj<ChildList>();
        if (manager.unref_queue.full()) {
            delete this;
        }
        else {
            manager.unref_queue.prepend(const_cast<ChildList*>(this));
            const_cast<ChildList*>(this)->node = 0;
        }
    }
    n->unbind_ref();
}



dom::ref<dom::Node> ChildList::item(dom::uint32 index) const throw ()
{
    if (!parent_node)
        return dom::null;

    dom::uint32 n;
    Node* child;

    if (prev_child) {
        child = prev_child;
        if (prev_index < index) {
            n = index - prev_index;
            goto forward;
        }

        n = prev_index - index;
        while (0 < n) {
            --n;
            child = child->get_prev_sibling();
        }
        goto done;
    }

    n = index;
    child = parent_node->get_first_child();

    for (;;) {
        if (!child)
            return dom::null;
        if (n == 0)
            break;
      forward:
        --n;
        child = child->get_next_sibling();
    }

    if (!prev_child)
        parent_node->set_flag(ParentNode::valid_child_list);

  done:
    prev_index = index;
    prev_child = child;

    return dom::ref<dom::Node>(child);
}



dom::uint32 ChildList::getLength() const throw ()
{
    if (!parent_node)
        return 0;

    if (have_length)
        return length;
    dom::uint32 n = 0;
    Node* child = parent_node->get_first_child();
    while (child) {
        ++n;
        child = child->get_next_sibling();
    }
    length = n;
    have_length = true;
    parent_node->set_flag(ParentNode::valid_child_list);
    return n;
}



dom::DOMString Element::getNodeName() const throw ()
{
    return get_type()->qual.tag_name;
}



dom::DOMString Element::getNamespaceURI() const throw ()
{
    return get_type()->qual.ns_uri;
}



dom::DOMString Element::getPrefix() const throw ()
{
    return get_type()->qual.prefix;
}



void Element::setPrefix(const dom::DOMString& prefix) throw (dom::DOMException)
{
    ElemType* t = get_type();
    if (t->key.dom1)
        return; // No-op if element is created by DOM Level 1 method.

    if (is_read_only())
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR,
                                "Cannot change prefix of read-only element");

    typedef dom::DOMString::traits_type traits;
    traits::char_type colon = traits::to_char_type(0x3A);
    dom::DOMString::size_type i = t->key.tag_name.find(colon);
    dom::DOMString local_name = i == dom::DOMString::npos ? t->key.tag_name :
        t->key.tag_name.substr(i+1);
    ElemTypeRef new_type =
        t->doc->get_elem_type(t->key.ns_uri, prefix + colon + local_name, false);

    // A change of prefix may impact the list returned by
    // getElementByTagName(), so we must consider this a change of
    // children.
    if (ParentNode* p = get_parent())
        p->before_children_change();

    type = new_type.get();
}



dom::DOMString Element::getLocalName() const throw ()
{
    return get_type()->qual.local_name;
}



dom::DOMString Element::getTagName() const throw ()
{
    return get_type()->qual.tag_name;
}



dom::DOMString Element::getAttribute(const dom::DOMString& name) const throw ()
{
    static_cast<void>(name);
    return dom::DOMString(); // FIXME: Implement this!
}



void Element::setAttribute(const dom::DOMString& name, const dom::DOMString& value)
    throw (dom::DOMException)
{
    static_cast<void>(name);
    static_cast<void>(value);
    // FIXME: Implement this!
}



dom::ref<dom::NodeList> Element::getElementsByTagName(const dom::DOMString&) const
    throw ()
{
    // Create an instance of a proper NodeList implementation
    // The NodeList implementation is of a general type with a custom filter function
    // return new FilteredNodeList();
    // "*" is all
    // Idea:
    //   Add cache/rare data flag to ParentNode
    //   As soon as a FilteredNodeList is created on behalf of a ParentNode, add the raw pointer of the NodeList to the cache in the document and set the flag on the target ParentNdoe
    //   The NodeList instance keeps the target ParentNode alive as long as it has a non-zero reference count
    //   When the target node is destroyed, if the flag is set, also remove all cached node lists in the document.
    //   There should be a maximum number of unreferenced node lists in the cache. The oldest one should be discarded, when the limit is reached.
    //   Memory management/reference counting works much like if the NodeList had been a child of the target ParentNode.
    //   We do not have to clear the cache flag just because the caches are discarded.
    //   IMPORTANT: Start by finding the matching element type, then search for elements which has this type. The implcation is that there could be multiple types to look for.
    //   IMPORTANT: All caches must be discarded when the element hierachy changes, also if an element changes type.
    return dom::null; // FIXME: Implement this!
}



dom::ref<dom::NodeList> Element::getElementsByTagNameNS(const dom::DOMString&,
                                                        const dom::DOMString&) const
    throw (dom::DOMException)
{
    return dom::null; // FIXME: Implement this!
}



dom::DOMString DocumentFragment::getNodeName() const throw ()
{
    return get_doc()->impl->str_node_name_doc_frag;
}



dom::DOMString CharacterData::getNodeValue() const throw ()
{
    return data;
}



void CharacterData::setNodeValue(const dom::DOMString& v) throw (dom::DOMException)
{
    set_data(v);
}



dom::DOMString CharacterData::getTextContent() const throw (dom::DOMException)
{
    return data;
}



void CharacterData::setTextContent(const dom::DOMString& v) throw (dom::DOMException)
{
    set_data(v);
}



dom::DOMString CharacterData::getData() const throw ()
{
    return data;
}



void CharacterData::setData(const dom::DOMString& d) throw (dom::DOMException)
{
    set_data(d);
}



dom::DOMString Text::getNodeName() const throw ()
{
    return get_type()->doc->impl->str_node_name_text;
}



bool Text::isElementContentWhitespace() const throw ()
{
    return get_type()->elem_cont_whitespace;
}



dom::DOMString Comment::getNodeName() const throw ()
{
    return get_type()->doc->impl->str_node_name_comment;
}



dom::DOMString CDATASection::getNodeName() const throw ()
{
    return get_type()->doc->impl->str_node_name_cdata;
}



dom::DOMString ProcessingInstruction::getNodeName() const throw ()
{
    return target;
}



dom::DOMString ProcessingInstruction::getNodeValue() const throw ()
{
    return data;
}



void ProcessingInstruction::setNodeValue(const dom::DOMString& v) throw (dom::DOMException)
{
    set_data(v);
}



dom::DOMString ProcessingInstruction::getTextContent() const throw (dom::DOMException)
{
    return data;
}



void ProcessingInstruction::setTextContent(const dom::DOMString& v) throw (dom::DOMException)
{
    set_data(v);
}



dom::DOMString ProcessingInstruction::getTarget() const throw ()
{
    return target;
}



dom::DOMString ProcessingInstruction::getData() const throw ()
{
    return data;
}



void ProcessingInstruction::setData(const dom::DOMString& d) throw (dom::DOMException)
{
    set_data(d);
}



class DocumentType::NamedNodeMap: public virtual dom::NamedNodeMap {
public:
    DocumentType* const doctype;


    virtual dom::ref<dom::Node> getNamedItem(const dom::DOMString& n) const throw ()
    {
        Map::const_iterator i = node_map.find(n);
        return dom::ref<dom::Node>(i == node_map.end() ? 0 : i->second);
    }

    virtual dom::ref<dom::Node> setNamedItem(const dom::ref<dom::Node>&)
        throw (dom::DOMException)
    {
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR, "Node map is read-only");
    }

    virtual dom::ref<dom::Node> removeNamedItem(const dom::DOMString&) throw (dom::DOMException)
    {
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR, "Node map is read-only");
    }

    virtual dom::ref<dom::Node> item(dom::uint32 i) const throw ()
    {
        return dom::ref<dom::Node>(i < order.size() ? order[i] : 0);
    }

    virtual dom::uint32 getLength() const throw () { return order.size(); }

    virtual dom::ref<dom::Node> getNamedItemNS(const dom::DOMString&, const dom::DOMString&) const
        throw (dom::DOMException)
    {
        return dom::null;
    }

    virtual dom::ref<dom::Node> setNamedItemNS(const dom::ref<dom::Node>&)
        throw (dom::DOMException)
    {
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR, "Node map is read-only");
    }

    virtual dom::ref<dom::Node> removeNamedItemNS(const dom::DOMString&, const dom::DOMString&)
        throw (dom::DOMException)
    {
        throw dom::DOMException(dom::NO_MODIFICATION_ALLOWED_ERR, "Node map is read-only");
    }


    NamedNodeMap(DocumentType *d): doctype(d) {}

    virtual ~NamedNodeMap() throw () {}


    // The node must have no external references to it, and ownership
    // of it is passed from the caller to the callee.
    void add(const dom::DOMString& name, Node* n)
    {
        UniquePtr<Node> n2(n);
        order.push_back(n2);
        node_map[name] = n;
    }


private:
    friend class Entity;
    friend class Notation;


    core::DeletingVector<Node> order;
    typedef map<dom::DOMString, Node*> Map;
    Map node_map;


    // Overriding method in dom::DOMObject.
    virtual void on_referenced() const throw ()
    {
        doctype->bind_ref();
    }

    // Overriding method in dom::DOMObject.
    virtual void on_unreferenced() const throw ()
    {
        doctype->unbind_ref();
    }
};



class DocumentType::DegenChildList: public virtual dom::NodeList {
public:
    virtual dom::ref<dom::Node> item(dom::uint32) const throw () { return dom::null; }

    virtual dom::uint32 getLength() const throw () { return 0; }

    DegenChildList(const DocumentType* d): doctype(d) {}

protected:
    virtual void on_referenced()   const throw () { doctype->bind_ref(); }

    virtual void on_unreferenced() const throw () { doctype->unbind_ref(); }

private:
    const DocumentType* const doctype;
};



dom::DOMString DocumentType::getNodeName() const throw ()
{
    return name;
}



dom::ref<dom::NodeList> DocumentType::getChildNodes() const throw ()
{
    if (!degen_child_list)
        degen_child_list.reset(new DegenChildList(this));
    return dom::ref<dom::NodeList>(degen_child_list.get());
}



bool DocumentType::isSupported(const dom::DOMString& f, const dom::DOMString& v) const throw ()
{
    return impl->hasFeature(f,v);
}



dom::DOMString DocumentType::getName() const throw ()
{
    return name;
}



dom::ref<dom::NamedNodeMap> DocumentType::getEntities() const throw ()
{
    return dom::ref<dom::NamedNodeMap>(entities.get());
}



dom::ref<dom::NamedNodeMap> DocumentType::getNotations() const throw ()
{
    return dom::ref<dom::NamedNodeMap>(notations.get());
}



dom::DOMString DocumentType::getPublicId() const throw ()
{
    return public_id;
}



dom::DOMString DocumentType::getSystemId() const throw ()
{
    return system_id;
}



dom::DOMString DocumentType::getInternalSubset() const throw ()
{
    return internal_subset;
}



DocumentType::DocumentType(DOMImplementation* i, const dom::DOMString& n,
                           const dom::DOMString& p, const dom::DOMString& s):
    Node(&i->node_type_unbound_doctype), impl(i), name(n), public_id(p), system_id(s),
    entities(new NamedNodeMap(this)), notations(new NamedNodeMap(this))
{
}



DocumentType::~DocumentType() throw () {}



void DocumentType::add_entity(const dom::DOMString& name, const dom::DOMString& public_id,
                              const dom::DOMString& system_id,
                              const dom::DOMString& notation_name)
{
    if (!node_type_entity) {
        Document* d = get_type()->doc;
        ARCHON_ASSERT(d);
        bool is_child_node = false;
        bool is_parent_node = true;
        node_type_entity.reset(new NodeType(ENTITY_NODE, d, is_child_node, is_parent_node, true));
    }
    entities->add(name, new Entity(node_type_entity.get(), entities.get(),
                                   name, public_id, system_id, notation_name));
}



void DocumentType::add_notation(const dom::DOMString& name, const dom::DOMString& public_id,
                                const dom::DOMString& system_id)
{
    if (!node_type_notation) {
        Document* d = get_type()->doc;
        ARCHON_ASSERT(d);
        bool is_child_node = false;
        bool is_parent_node = false;
        node_type_notation.reset(new NodeType(NOTATION_NODE, d, is_child_node, is_parent_node,
                                              true));
    }
    notations->add(name, new Notation(node_type_notation.get(), notations.get(),
                                      name, public_id, system_id));
}



void DocumentType::bind_to_document(Document* d) throw ()
{
    ARCHON_ASSERT(!get_type()->doc);
    ARCHON_ASSERT(!get_parent());
    type = &d->node_type_doctype;
    d->bind_ref();
}



// Overriding Node::on_referenced()
void DocumentType::on_referenced() const throw ()
{
    if (ParentNode* p = get_parent())
        p->bind_ref();
}



// Overriding Node::on_unreferenced()
void DocumentType::on_unreferenced() const throw ()
{
    if (ParentNode* p = get_parent()) {
        p->unbind_ref();
    }
    else {
        Document* d = get_type()->doc;
        delete this;
        if (d)
            d->unbind_ref();
    }
}



dom::DOMString Entity::getNodeName() const throw ()
{
    return name;
}



dom::DOMString Entity::getPublicId() const throw ()
{
    return public_id;
}



dom::DOMString Entity::getSystemId() const throw ()
{
    return system_id;
}



dom::DOMString Entity::getNotationName() const throw ()
{
    return notation_name;
}



dom::DOMString Entity::getInputEncoding() const throw ()
{
    return dom::DOMString(); // FIXME: Implement this!
}



dom::DOMString Entity::getXmlEncoding() const throw ()
{
    return dom::DOMString(); // FIXME: Implement this!
}



dom::DOMString Entity::getXmlVersion() const throw ()
{
    return dom::DOMString(); // FIXME: Implement this!
}



Entity::Entity(NodeType* t, DocumentType::NamedNodeMap* m, const dom::DOMString& n,
               const dom::DOMString& p, const dom::DOMString& s, const dom::DOMString& o):
    ParentNode(t), doctype_map(m), name(n), public_id(p), system_id(s), notation_name(o)
{
}



// Overriding dom::DOMObject::on_referenced().
void Entity::on_referenced() const throw ()
{
    doctype_map->bind_ref();
}



// Overriding dom::DOMObject::on_unreferenced().
void Entity::on_unreferenced() const throw ()
{
    doctype_map->unbind_ref();
}



dom::DOMString Notation::getNodeName() const throw ()
{
    return name;
}



dom::DOMString Notation::getPublicId() const throw ()
{
    return public_id;
}



dom::DOMString Notation::getSystemId() const throw ()
{
    return system_id;
}



Notation::Notation(NodeType* t, DocumentType::NamedNodeMap* m,
                   const dom::DOMString& n, const dom::DOMString& p, const dom::DOMString& s):
    Node(t), doctype_map(m), name(n), public_id(p), system_id(s)
{
}



// Overriding dom::DOMObject::on_referenced().
void Notation::on_referenced() const throw ()
{
    doctype_map->bind_ref();
}



// Overriding dom::DOMObject::on_unreferenced().
void Notation::on_unreferenced() const throw ()
{
    doctype_map->unbind_ref();
}



dom::DOMString Document::getNodeName() const throw ()
{
    return impl->str_node_name_doc;
}



dom::ref<dom::Document> Document::getOwnerDocument() const throw ()
{
    return dom::null;
}



// Overrides method in ParentNode.
dom::DOMString Document::getTextContent() const throw (dom::DOMException)
{
    return dom::DOMString();
}



dom::ref<dom::DocumentType> Document::getDoctype() const throw ()
{
    if (!valid_doctype_and_root)
        find_doctype_and_root();
    return dom::ref<dom::DocumentType>(doctype);
}



dom::ref<dom::DOMImplementation> Document::getImplementation() const throw ()
{
    return impl;
}



dom::ref<dom::Element> Document::getDocumentElement() const throw ()
{
    return dom::ref<dom::Element>(get_root());
}



dom::ref<dom::Element> Document::createElement(const dom::DOMString& name) const
    throw (dom::DOMException)
{
    return dom::ref<dom::Element>(get_elem_type(dom::DOMString(), name, true)->create_element());
}



dom::ref<dom::DocumentFragment> Document::createDocumentFragment() const throw ()
{
    return dom::ref<dom::DocumentFragment>(new DocumentFragment(const_cast<Document*>(this)));
}



dom::ref<dom::Text> Document::createTextNode(const dom::DOMString& d) const throw ()
{
    return dom::ref<dom::Text>(new Text(const_cast<Document*>(this), d, false));
}



dom::ref<dom::Comment> Document::createComment(const dom::DOMString& d) const throw ()
{
    return dom::ref<dom::Comment>(new Comment(const_cast<Document*>(this), d));
}



dom::ref<dom::CDATASection> Document::createCDATASection(const dom::DOMString& d) const
    throw(dom::DOMException)
{
    return dom::ref<dom::CDATASection>(new CDATASection(const_cast<Document*>(this), d, false));
}



dom::ref<dom::ProcessingInstruction>
Document::createProcessingInstruction(const dom::DOMString& t, const dom::DOMString& d) const
    throw (dom::DOMException)
{
    ProcessingInstruction* p =
        new ProcessingInstruction(const_cast<Document*>(this), t, d);
    return dom::ref<dom::ProcessingInstruction>(p);
}



dom::ref<dom::NodeList> Document::getElementsByTagName(const dom::DOMString&) const
    throw ()
{
    return dom::null; // FIXME: Implement this!
}



dom::ref<dom::Element>
Document::createElementNS(const dom::DOMString& ns, const dom::DOMString& name) const
    throw (dom::DOMException)
{
    return dom::ref<dom::Element>(get_elem_type(ns, name, false)->create_element());
}



dom::ref<dom::NodeList> Document::getElementsByTagNameNS(const dom::DOMString&,
                                                         const dom::DOMString&) const
    throw ()
{
    return dom::null; // FIXME: Implement this!
}



dom::DOMString Document::getInputEncoding() const throw ()
{
    return input_encoding;
}



dom::DOMString Document::getXmlEncoding() const throw ()
{
    return xml_encoding;
}



bool Document::getXmlStandalone() const throw ()
{
    return xml_standalone;
}



void Document::setXmlStandalone(bool v) throw (dom::DOMException)
{
    xml_standalone = v;
}



dom::DOMString Document::getXmlVersion() const throw ()
{
    switch (xml_version) {
        case xml_ver_1_0:
            return impl->str_ver_1_0;
        case xml_ver_1_1:
            return impl->str_ver_1_1;
    }
    throw runtime_error("Unexpected XML version");
}



void Document::setXmlVersion(const dom::DOMString& v) throw (dom::DOMException)
{
    xml_version = impl->parse_xml_ver(v);
}



dom::DOMString Document::getDocumentURI() const throw ()
{
    return document_uri;
}



void Document::setDocumentURI(const dom::DOMString& v) throw ()
{
    document_uri = v;
}



dom::ref<dom::Node> Document::adoptNode(const dom::ref<dom::Node>&) const
    throw (dom::DOMException)
{
      // n = n->next_in_doc_order();
      // If parent node: first child, if any, otherwise, stop if this is root, otherwise next sibling, if any, otherwise, stop if parent is root, otherwise next sibling of parent, if any, otherwise next sibling of parents parent, if any, and so on

/*
   Verify that removal from parent is possible

   Go through nodes iteratively in depth-first order. For each node:
     Add the original node type to a buffer.
     Acquire node type from target doc, and update pointer in node.
     If node has a child node list (utilize flag has_child_list), then call child_list_manager.duplicate_registration().
WHOOPS: What about degenerate child node lists?
     Same for filtered elem lists and for a NamedNodeMap and/or any attribute node.
     If it is an EntityReference node, and it has a value in the target doc, then create a clone of the value and assign it, else remove any children. Also, if it had a value in the source doc, then store the original value in a list as <ent-ref-node-ptr,ptr-to-first-child>.

   In case of failure (out of memory), repeat the iteration up to the same point (maybe backwards) and reset all values by making the necessary registry lookups in the source document. Also revert all the registrations performed on the target document. Also discard new entity reference node values, and reinstate the previously store values.

   If everything goes well:
     Remove from parent, if any
     For each buffered node type:
       Remove a usage count in the source document
     For each node that had a node list (referenced or unreferenced):
       Delete the registry entry in the source document.
     Also handle NamedNodeMap and/or attributes.
     Delete all stored entity reference child plus their siblings.


  ALSO REMEMBER TO UPATE THE ID MAPS OF SOURCE AND TARGET DOCUMENTS
*/

      // FIXME: Must also transfer degenerate child lists and other rare data registered with the document (node lists and attributes).
    return dom::null; // FIXME: Implement this!!!!!!!!!!!!!
}



void Document::approve_child(const Node* c) const
{
    approve_children(c, true);
}



void Document::approve_children(const DocumentFragment* frag) const
{
    if (const Node* c = frag->get_first_child())
        approve_children(c, false);
}



ElemTypeRef Document::get_elem_type(const dom::DOMString& ns_uri,
                                    const dom::DOMString& tag_name, bool dom1) const
{
    ElemKey key(ns_uri, tag_name, dom1);
    ElemType*& type_alias = elem_types[key];
    ElemType* type = type_alias;

    if (type)
        return ElemTypeRef(type);

    try {
        dom::DOMString prefix, local_name;
        if (dom1) {
            validate_xml_name(xml_version, tag_name);
        }
        else {
            parse_qualified_name(xml_version, tag_name, prefix, local_name);
            if (prefix.empty()) {
                validate_xmlns(impl.get(), ns_uri, local_name);
            }
            else {
                if (!ns_uri.empty())
                    throw dom::DOMException(dom::NAMESPACE_ERR, "Prefix without namespace URI");
                if (prefix == impl->str_xml && ns_uri != impl->str_ns_namespace)
                    throw dom::DOMException(dom::NAMESPACE_ERR, "Namespace must be "
                                            "'http://www.w3.org/XML/1998/namespace' when the prefix "
                                            "is 'xml'");
                validate_xmlns(impl.get(), ns_uri, prefix);
            }
        }

        bool read_only = false;
        ElemTypeRef type_ref = create_elem_type(read_only, key, prefix, local_name);
        type_alias = type_ref.get();
        return type_ref;
    }
    catch (...) {
        // Remove the incomplete map entry
        elem_types.erase(key);
        throw;
    }
}



ElemTypeRef Document::create_elem_type(bool read_only, const ElemKey& key,
                                       const dom::DOMString& prefix,
                                       const dom::DOMString& local_name) const
{
    ElemQual qual;
    qual.ns_uri     = key.ns_uri;
    qual.tag_name   = key.tag_name;
    qual.prefix     = prefix;
    qual.local_name = local_name;
    return ElemTypeRef(new ElemType(const_cast<Document*>(this), read_only, key, qual));
}



void Document::parse_qualified_name(XmlVersion v, const dom::DOMString& name,
                                    dom::DOMString& prefix, dom::DOMString& local_name)
{
    validate_xml_name(v, name);
    typedef dom::DOMString::traits_type traits;
    traits::char_type colon = traits::to_char_type(0x3A);
    dom::DOMString::size_type i = name.find(colon);
    if (i == dom::DOMString::npos) {
        local_name = name;
    }
    else {
        if (name.find(colon, i+1) != dom::DOMString::npos)
            throw dom::DOMException(dom::NAMESPACE_ERR, "More than one colon in qualified name");
        prefix     = name.substr(0, i);
        local_name = name.substr(i+1);
        if (prefix.empty())
            throw dom::DOMException(dom::NAMESPACE_ERR, "Empty prefix in qualified name");
        if (local_name.empty())
            throw dom::DOMException(dom::NAMESPACE_ERR, "Empty local name in qualified name");
    }
}



void Document::set_doc_info(const dom::DOMString& doc_uri, const dom::DOMString& input_enc,
                            XmlVersion v, const dom::DOMString& xml_enc, bool standalone)
{
    document_uri   = doc_uri;
    input_encoding = input_enc;

    xml_version    = v;
    xml_encoding   = xml_enc;
    xml_standalone = standalone;
}



Element* Document::create_elem_child_for_parser(ParentNode* parent, const dom::DOMString& ns_uri,
                                                const dom::DOMString& tag_name,
                                                const dom::DOMString& prefix,
                                                const dom::DOMString& local_name)
{
    bool dom1 = local_name.empty();
    ARCHON_ASSERT(!dom1 || ns_uri.empty());
    ARCHON_ASSERT(!dom1 || prefix.empty());

    // We must hold an extra reference count on the element type in
    // case construction of the element fails, because otherwise we
    // would not know whether the element type gets destroyed.
    ElemTypeRef type;
    {
        ElemKey key(ns_uri, tag_name, dom1);
        ElemType*& type2 = elem_types[key];
        if (type2) {
            type.reset(type2);
        }
        else {
            try {
                bool read_only = false;
                type = create_elem_type(read_only, key, prefix, local_name);
                type2 = type.get();
            }
            catch (...) {
                // Remove the incomplete map entry
                elem_types.erase(key);
                throw;
            }
        }
    }

    Element* elem = type->create_element();
    parent->append_child_for_parser(elem);
    return elem;
}



Document::Document(DOMImplementation* i):
    ParentNode(&node_type_doc), impl(i),
    node_type_doc(DOCUMENT_NODE, this, false, true, false),
    node_type_doc_frag(DOCUMENT_FRAGMENT_NODE, this, false, true, false),
    node_type_comment(COMMENT_NODE, this, true, false, false),
    node_type_proc_instr(PROCESSING_INSTRUCTION_NODE, this, true, false, false),
    node_type_doctype(DOCUMENT_TYPE_NODE, this, true, false, true),
    node_type_abstract_elem(ELEMENT_NODE, this, true, true, false),
    text_type_normal(TEXT_NODE, this, false, false),
    text_type_elem_cont_whitespace(TEXT_NODE, this, false, true),
    cdata_type_normal(CDATA_SECTION_NODE, this, false, false),
    cdata_type_elem_cont_whitespace(CDATA_SECTION_NODE, this, false, true),
    xml_version(xml_ver_1_0), xml_standalone(false), valid_doctype_and_root(false),
    unused_rare_node_data(i)
{
}



Document::~Document() throw ()
{
    // Destroy children early such that elements get a chance to
    // unregister their types and all nodes release their rare objects
    // to the document before the document ceases to be a proper
    // Document.
    const_cast<Document*>(this)->destroy_children();
    ARCHON_ASSERT(elem_types.empty());
}



// Overriding method in Node
void Document::on_referenced() const throw ()
{
}



// Overriding method in Node
void Document::on_unreferenced() const throw ()
{
    delete this;
}



void Document::approve_children(const Node* c, bool only_one) const
{
    if (!valid_doctype_and_root)
        find_doctype_and_root();
    bool has_elem    = root;
    bool has_doctype = doctype;
    do {
        switch (c->get_type()->id) {
            case ELEMENT_NODE:
                if (has_elem)
                    throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "A document is not allowed "
                                            "to have more than one element child");
                has_elem = true;
                break;
            case PROCESSING_INSTRUCTION_NODE:
            case COMMENT_NODE:
                break;
            case DOCUMENT_TYPE_NODE:
                if (has_doctype)
                    throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "A document is not allowed "
                                            "to have more than one document type");
                has_doctype = true;
                break;
            default:
                throw dom::DOMException(dom::HIERARCHY_REQUEST_ERR, "Bad child type for document");
        }
        if (only_one)
            return;
        c = c->get_next_sibling();
    }
    while (c);
}



// Validate the name according the the effective XML version.
void Document::validate_xml_name(XmlVersion, const dom::DOMString& name)
{
    if (name.empty())
        throw dom::DOMException(dom::INVALID_CHARACTER_ERR, "No name spacified");

    // FIXME: Must use alternative version of validation for XML 1.1
    if (!validate_xml_1_0_name(name))
        throw dom::DOMException(dom::INVALID_CHARACTER_ERR, "Bad XML 1.0 name");
}



bool DOMImplementation::hasFeature(const dom::DOMString& f,
                                   const dom::DOMString& v) const throw ()
{
    dom::DOMString f2 = f;

    // Drop leading plus (for now all features can be obtained through
    // casting).
    typedef dom::DOMString::traits_type traits;
    traits::char_type plus = traits::to_char_type(0x2B);
    if (!f2.empty() && f2[0] == plus)
        f2.erase(0,1);

    to_upper_case_ascii(f2);
    return has_feature(f2, v);
}



dom::ref<dom::DocumentType>
DOMImplementation::createDocumentType(const dom::DOMString& n, const dom::DOMString& p,
                                      const dom::DOMString& s) const throw (dom::DOMException)
{
    DOMImplementation* i = const_cast<DOMImplementation*>(this);
    return dom::ref<dom::DocumentType>(new DocumentType(i,n,p,s));
}



dom::ref<dom::Document>
DOMImplementation::createDocument(const dom::DOMString& ns, const dom::DOMString& name,
                                  const dom::ref<dom::DocumentType>& doctype) const
    throw (dom::DOMException)
{
    dom::ref<dom::Document> doc = create_document(doctype.get());
    if (doctype)
        doc->appendChild(doctype); // FIXME: Should there be a default doctype????????????????????????????????????????
    if (!name.empty()) {
        doc->appendChild(doc->createElementNS(ns, name));
    }
    else {
        if (!ns.empty())
            throw dom::DOMException(dom::NAMESPACE_ERR, "Namespace URI specified, "
                                    "but no qualified name");
    }
    return doc;
}



bool DOMImplementation::has_feature(const dom::DOMString& f,
                                    const dom::DOMString& v) const throw ()
{
    if (f == str_feat_core)
        return v.empty() || v == str_ver_1_0 || v == str_ver_2_0 || v == str_ver_3_0;
    if (f == str_feat_xml)
        return v.empty() || v == str_ver_1_0 || v == str_ver_2_0 || v == str_ver_3_0;
    if (f == str_feat_xml_ver)
        return v.empty() || v == str_ver_1_0 || v == str_ver_1_1;
    return false;
}



Document::XmlVersion DOMImplementation::parse_xml_ver(const dom::DOMString& v) const
{
    if (v == str_ver_1_0)
        return Document::xml_ver_1_0;
    if (v == str_ver_1_1)
        return Document::xml_ver_1_1;
    throw dom::DOMException(dom::NOT_SUPPORTED_ERR, "Bad XML version. Must be 1.0 or 1.1.");
}



dom::ref<Document> DOMImplementation::create_document(dom::DocumentType const *) const
{
    DOMImplementation* i = const_cast<DOMImplementation*>(this);
    return dom::ref<Document>(new Document(i));
}



DOMImplementation::DOMImplementation():
    str_feat_core(dom::str_from_cloc(L"CORE")),
    str_feat_xml(dom::str_from_cloc(L"XML")),
    str_feat_xml_ver(dom::str_from_cloc(L"XMLVersion")),
    str_ver_1_0(dom::str_from_cloc(L"1.0")),
    str_ver_1_1(dom::str_from_cloc(L"1.1")),
    str_ver_2_0(dom::str_from_cloc(L"2.0")),
    str_ver_3_0(dom::str_from_cloc(L"3.0")),
    str_node_name_doc_frag(dom::str_from_cloc(L"#document-fragment")),
    str_node_name_text(dom::str_from_cloc(L"#text")),
    str_node_name_comment(dom::str_from_cloc(L"#comment")),
    str_node_name_cdata(dom::str_from_cloc(L"#cdata-section")),
    str_node_name_doc(dom::str_from_cloc(L"#document")),
    str_ns_namespace(dom::str_from_cloc(L"http://www.w3.org/XML/1998/namespace")),
    str_ns_xmlns(dom::str_from_cloc(L"http://www.w3.org/2000/xmlns/")),
    str_xml(dom::str_from_cloc(L"xml")),
    str_xmlns(dom::str_from_cloc(L"xmlns")),
    node_type_unbound_doctype(dom::Node::DOCUMENT_TYPE_NODE, 0, true, false, true)
{
}


} // namespace dom_impl
} // namespace archon
