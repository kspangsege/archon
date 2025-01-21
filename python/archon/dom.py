import weakref
import collections.abc
import archon.core


class Node:
    ELEMENT_NODE = 1
    ATTRIBUTE_NODE = 2
    TEXT_NODE = 3
    CDATA_SECTION_NODE = 4
    ENTITY_REFERENCE_NODE = 5
    ENTITY_NODE = 6
    PROCESSING_INSTRUCTION_NODE = 7
    COMMENT_NODE = 8
    DOCUMENT_NODE = 9
    DOCUMENT_TYPE_NODE = 10
    DOCUMENT_FRAGMENT_NODE = 11
    NOTATION_NODE = 12

    def get_node_type(self):
        raise RuntimeError("Abstract method")

    def get_node_name(self):
        raise RuntimeError("Abstract method")

    def get_owner_document(self):
        raise RuntimeError("Abstract method")

    def get_parent_node(self):
        raise RuntimeError("Abstract method")

    def has_child_nodes(self):
        raise RuntimeError("Abstract method")

    def get_child_nodes(self):
        raise RuntimeError("Abstract method")

    def get_first_child(self):
        raise RuntimeError("Abstract method")

    def get_last_child(self):
        raise RuntimeError("Abstract method")

    def get_previous_sibling(self):
        raise RuntimeError("Abstract method")

    def get_next_sibling(self):
        raise RuntimeError("Abstract method")

    def contains(self, other):
        raise RuntimeError("Abstract method")

    def insert_before(self, node, child):
        raise RuntimeError("Abstract method")

    def append_child(self, node):
        raise RuntimeError("Abstract method")

    def replace_child(self, node, child):
        raise RuntimeError("Abstract method")

    def remove_child(self, node):
        raise RuntimeError("Abstract method")


class ChildNode:
    pass


class ParentNode:
    pass


class Document(ParentNode, Node):
    # FIXME: Add get_implementation()

    def get_content_type(self):
        raise RuntimeError("Abstract method")

    def get_doctype(self):
        raise RuntimeError("Abstract method")

    def get_document_element(self):
        raise RuntimeError("Abstract method")

    def create_element(self, local_name):
        raise RuntimeError("Abstract method")

    # FIXME: Add create_element_ns()

    def create_text_node(self, data):
        raise RuntimeError("Abstract method")

    def create_comment(self, data):
        raise RuntimeError("Abstract method")


class DocumentType(ChildNode, Node):
    def get_name(self):
        raise RuntimeError("Abstract method")

    def get_public_id(self):
        raise RuntimeError("Abstract method")

    def get_system_id(self):
        raise RuntimeError("Abstract method")


class Element(ParentNode, ChildNode, Node):
    def get_namespace_uri(self):
        raise RuntimeError("Abstract method")

    def get_prefix(self):
        raise RuntimeError("Abstract method")

    def get_local_name(self):
        raise RuntimeError("Abstract method")

    def get_tag_name(self):
        raise RuntimeError("Abstract method")

    # FIXME: Add attribute covering getters and setters: `get_id()`, `set_id()` cover `id`
    # attribute; `get_class_name()`, `set_class_name()` cover `class` attribute, etc.

    def has_attributes(self):
        raise RuntimeError("Abstract method")

    def get_attributes(self):
        raise RuntimeError("Abstract method")

    def remove_attribute(self, qualified_name):
        raise RuntimeError("Abstract method")

    def remove_attribute_ns(self, namespace, local_name):
        raise RuntimeError("Abstract method")

    def get_attribute_node(self, qualified_name):
        raise RuntimeError("Abstract method")

    def get_attribute_node_ns(self, namespace, local_name):
        raise RuntimeError("Abstract method")

    def set_attribute_node(self, attr):
        raise RuntimeError("Abstract method")

    def set_attribute_node_ns(self, attr):
        raise RuntimeError("Abstract method")


class CharacterData(ChildNode, Node):
    def get_data(self):
        raise RuntimeError("Abstract method")

    def set_data(self, data):
        raise RuntimeError("Abstract method")


class Text(CharacterData):
    pass


# FIXME: Add CDATASection


class Comment(CharacterData):
    pass


# FIXME: Add ProcessingInstruction


class Attr(Node):
    def get_namespace_uri(self):
        raise RuntimeError("Abstract method")

    def get_prefix(self):
        raise RuntimeError("Abstract method")

    def get_local_name(self):
        raise RuntimeError("Abstract method")

    def get_name(self):
        raise RuntimeError("Abstract method")

    def get_value(self):
        raise RuntimeError("Abstract method")

    def set_value(self, value):
        raise RuntimeError("Abstract method")

    def get_owner_element(self):
        raise RuntimeError("Abstract method")

    def is_specified(self):
        raise RuntimeError("Abstract method")



class NodeList(collections.abc.Sequence):
    def get_length(self):
        raise RuntimeError("Abstract method")

    def item(self, index):
        raise RuntimeError("Abstract method")


class NamedNodeMap(collections.abc.MutableMapping):
    def get_length(self):
        raise RuntimeError("Abstract method")

    def item(self, index):
        raise RuntimeError("Abstract method")

    def get_named_item(self, qualified_name):
        raise RuntimeError("Abstract method")

    def get_named_item_ns(self, namespace, local_name):
        raise RuntimeError("Abstract method")

    def set_named_item(self, attr):
        raise RuntimeError("Abstract method")

    def set_named_item_ns(self, attr):
        raise RuntimeError("Abstract method")

    def remove_named_item(self, qualified_name):
        raise RuntimeError("Abstract method")

    def remove_named_item_ns(self, namespace, local_name):
        raise RuntimeError("Abstract method")



class DOMException(RuntimeError):
    pass


class HierarchyRequestError(DOMException):
    pass

class NotFoundError(DOMException):
    pass

class InUseAttributeError(DOMException):
    pass



def create_xml_document():
    content_type = "application/xml"
    is_html = False
    state = _DocumentState(content_type, is_html)
    return _wrap_node(state, None)

def create_html_document():
    content_type = "text/html"
    is_html = True
    state = _DocumentState(content_type, is_html)
    return _wrap_node(state, None)

def create_document_type(document, name, public_id, system_id):
    state = _DocumentTypeState(name, public_id, system_id)
    return _wrap_node(state, document)

def create_element(document, namespace_uri, prefix, local_name, attributes):
    document_state = _unwrap_node(document)
    document_is_html = document_state.is_html
    element_state = _ElementState(document_is_html, namespace_uri, prefix, local_name)
    element = _wrap_node(element_state, document)
    for attr in attributes:
        element.set_attribute_node(attr)
    return element

def create_attribute(document, namespace_uri, prefix, local_name, value):
    state = _AttrState(namespace_uri, prefix, local_name, value)
    return _wrap_node(state, document)


# This one is implementation agnostic
def dump_document(document, max_string_size = 90):
    assert isinstance(document, Document)
    def format_nullable_string(string):
        if string is None:
            return "null"
        return archon.core.quote(string)
    def format_name(name):
        need_quotation = False
        if not name:
            need_quotation = True
        else:
            for ch in name:
                if not ch.isalnum() and ch != "-":
                    need_quotation = True
                    break
        if need_quotation:
            return archon.core.quote(name)
        return name
    def format_namespace_uri(namespace_uri):
        if namespace_uri is None:
            return "null"
        return archon.core.quote(namespace_uri)
    def dump(string, level):
        print("%s%s" % (level * "  ", string))
    def visit(node, namespace_uri, level):
        if isinstance(node, Document):
            string = "Document(content_type=%s)" % archon.core.quote(node.get_content_type())
            if not node.has_child_nodes():
                dump(string, level)
                return
            dump("%s:" % string, level)
            for child in node.get_child_nodes():
                visit(child, namespace_uri, level + 1)
            return
        if isinstance(node, DocumentType):
            dump("Doctype(%s, %s, %s)" % (format_nullable_string(node.get_name()),
                                          format_nullable_string(node.get_public_id()),
                                          format_nullable_string(node.get_system_id())), level)
            return
        if isinstance(node, Element):
            string = format_name(node.get_local_name())
            prefix = node.get_prefix()
            if prefix is not None:
                string = "%s:%s" % (format_name(prefix), string)
            namespace_uri_2 = node.get_namespace_uri()
            if namespace_uri_2 != namespace_uri:
                string = "%s[%s]" % (string, format_namespace_uri(namespace_uri_2))
            string = "Element(%s)" % string
            if not node.has_attributes() and not node.has_child_nodes():
                dump(string, level)
                return
            dump("%s:" % string, level)
            for attr in node.get_attributes():
                string = format_name(attr.get_local_name())
                prefix = attr.get_prefix()
                if prefix is not None:
                    string = "%s:%s" % (format_name(prefix), string)
                namespace_uri_3 = attr.get_namespace_uri()
                if namespace_uri_3 is not None:
                    string = "%s[%s]" % (string, format_namespace_uri(namespace_uri_3))
                dump("Attr(%s=%s)" % (string, archon.core.clamped_quote(attr.get_value(), max_string_size)), level + 1)
            for child in node.get_child_nodes():
                visit(child, namespace_uri_2, level + 1)
            return
        if isinstance(node, Text):
            dump("Text(%s)" % archon.core.clamped_quote(node.get_data(), max_string_size), level)
            return
        if isinstance(node, Comment):
            dump("Comment(%s)" % archon.core.clamped_quote(node.get_data(), max_string_size), level)
            return
        assert False
    namespace_uri = None
    level = 0
    visit(document, namespace_uri, level)








# FIXME: Implement `__str__()` / `__repr__()` which should probably just return whatever `get_node_name()` returns       
class _NodeImpl(Node):
    def __init__(self, document, state):
        assert document
        self._document = document
        self._state = state

    def get_owner_document(self):
        return self._document

    def get_parent_node(self):
        return None

    def has_child_nodes(self):
        return False

    def get_child_nodes(self):
        return _degen_node_list

    def get_first_child(self):
        return None

    def get_last_child(self):
        return None

    def get_previous_sibling(self):
        return None

    def get_next_sibling(self):
        return None

    def contains(self, other):
        return False

    def insert_before(self, node, child):
        raise HierarchyRequestError()

    def append_child(self, node):
        raise HierarchyRequestError()

    def replace_child(self, node, child):
        raise HierarchyRequestError()

    def remove_child(self, node):
        raise NotFoundError()

    def _unwrap(self):
        return self._state


class _NodeState:
    def __init__(self):
        self._weak_wrapper = None

    def get_weak_parent_node(self):
        return None

    def contains(self, other):
        return False

    def wrap(self, document):
        if self._weak_wrapper:
            wrapper = self._weak_wrapper()
            if wrapper:
                return wrapper
        wrapper = self._do_wrap(document)
        self._weak_wrapper = weakref.ref(wrapper)
        return wrapper

    def _do_wrap(self, document):
        raise RuntimeError("Abstract method")



class _ChildNodeImpl(ChildNode):
    def get_parent_node(self):
        return _wrap_node(self._state.get_parent_node(), self._document)

    def get_previous_sibling(self):
        return _wrap_node(self._state.previous_sibling, self._document)

    def get_next_sibling(self):
        return _wrap_node(self._state.next_sibling, self._document)


class _ChildNodeState:
    def __init__(self):
        self.weak_parent_node = None
        self.previous_sibling = None
        self.next_sibling = None

    def get_weak_parent_node(self):
        return self.weak_parent_node

    def get_parent_node(self):
        return self.weak_parent_node and self.weak_parent_node()



class _ParentNodeImpl(ParentNode):
    def __init__(self):
        self._weak_child_nodes = None

    def has_child_nodes(self):
        return self._state.num_children > 0

    def get_child_nodes(self):
        if self._weak_child_nodes:
            child_nodes = self._weak_child_nodes()
            if child_nodes:
                return child_nodes
        child_nodes = _ChildNodes(self, self._state.iter_cache)
        self._weak_child_nodes = weakref.ref(child_nodes)
        return child_nodes

    def get_first_child(self):
        return _wrap_node(self._state.first_child, self._document)

    def get_last_child(self):
        return _wrap_node(self._state.last_child, self._document)

    def contains(self, other):
        return self._state.contains(_unwrap_node(other))

    def insert_before(self, node, child):
        self._state.insert_before(_unwrap_node(node), _unwrap_node(child))

    def append_child(self, node):
        child = None
        self._state.insert_before(_unwrap_node(node), child)

    def replace_child(self, node, child):
        self._state.replace_child(_unwrap_node(node), _unwrap_node(child))

    def remove_child(self, node):
        state = self._state.remove_child(_unwrap_node(node))
        return _wrap_node(state, self._document)


class _ParentNodeState:
    def __init__(self):
        self.weak_self = weakref.ref(self)
        self.num_children = 0
        self.first_child = None
        self.last_child = None
        self.iter_cache = _ChildNodesIterCache()

    def contains(self, other):
        if other == self:
            return True
        weak_node = other.get_weak_parent_node()
        while weak_node:
            if weak_node == self.weak_self:
                return True
            weak_node = weak_node().get_weak_parent_node()
        return False

    # FIXME: Add test case for parent.insert_before(node, child) where `node` is already a child of `parent`
    # FIXME: Add test case for parent.insert_before(node, child) where `node` and `child` are the same node
    # FIXME: Add test case for parent.replace_child(node, child) where `node` is already a child of `parent`
    # FIXME: Add test case for parent.replace_child(node, child) where `node` and `child` are the same node

    # FIXME: Consider the case where an element created for one document is inserted into
    # another. If it is to be allowed, the document association must be updated
    def insert_before(self, node, child):
        before = child
        self._validate_child_insertion(node, before)
        parent = node.get_parent_node()
        if parent:
            # FIXME: Oops, something needs to be done here if `node` and `before` are the same node                       
            parent._remove_child(node)
        self._insert_child(node, before)
        node.weak_parent_node = self.weak_self
        self._child_inserted(node)

    def replace_child(self, node, child):
        orig = child
        self._validate_child_replacement(node, orig)
        parent = node.get_parent_node()
        if parent:
            # FIXME: Oops, something needs to be done here if `node` and `orig` are the same node                       
            parent._remove_child(node)
        self._replace_child(node, orig)                                               
        child.weak_parent_node = None
        node.weak_parent_node = self.weak_self
        self._child_removed(child)
        self._child_inserted(node)

    def remove_child(self, node):
        if node.get_weak_parent_node() != self.weak_self:
            raise NotFoundError()
        self._remove_child(node)
        node.weak_parent_node = None
        self._child_removed(node)
        return node

    # `node` may already be a child of `self`
    # `node` and `before` may be the same node
    def _validate_child_insertion(self, node, before):
        # FIXME: Need to take DocumentFragment into account below
        if not isinstance(node, _ChildNodeState):
            raise HierarchyRequestError()
        # FIXME: This check must be based on the "host-including inclusive ancestor"
        # instead of the "inclusive ancestor"
        if node.contains(self):
            raise HierarchyRequestError()
        if before and before.get_weak_parent_node() != self.weak_self:
            return NotFoundError()
        if not isinstance(self, _DocumentState):
            if isinstance(node, _DocumentTypeState):
                raise HierarchyRequestError()
            return
        if isinstance(node, _DocumentTypeState):
            if self.doctype: # FIXME: Not good if `node` is already a child of `self`                    
                raise HierarchyRequestError()
            child = self.first_child
            while child != before:
                if isinstance(child, _ElementState):
                    raise HierarchyRequestError()
                child = child.next_sibling
        elif isinstance(node, _ElementState):
            if self.document_element: # FIXME: Not good if `node` is already a child of `self`                    
                raise HierarchyRequestError()
            if before:
                child = before
                while child:
                    if isinstance(child, _DocumentTypeState):
                        raise HierarchyRequestError()
                    child = child.next_sibling
        elif isinstance(node, _TextState):
            raise HierarchyRequestError()

    # `node` may already be a child of `self`
    # `node` and `orig` may be the same node
    def _validate_child_replacement(self, node, orig):
        # FIXME: Need to take DocumentFragment into account below
        if not isinstance(node, _ChildNodeState):
            raise HierarchyRequestError()
        # FIXME: This check must be based on the "host-including inclusive ancestor"
        # instead of the "inclusive ancestor"
        if node.contains(self):
            raise HierarchyRequestError()
        if orig.get_weak_parent_node() != self.weak_self:
            return NotFoundError()
        if not isinstance(self, _DocumentState):
            if isinstance(node, _DocumentTypeState):
                raise HierarchyRequestError()
            return
        if isinstance(node, _DocumentTypeState):
            # FIXME: Does this work if `node` is already a child of `self`?                    
            if self.doctype:
                if self.doctype != orig:
                    raise HierarchyRequestError()
                return
            child = self.first_child
            while child != orig:
                if isinstance(child, _ElementState):
                    raise HierarchyRequestError()
                child = child.next_sibling
        elif isinstance(node, _ElementState):
            # FIXME: Does this work if `node` is already a child of `self`?                    
            if self.document_element:
                if self.document_element != orig:
                    raise HierarchyRequestError()
                return
            child = orig.next_sibling
            while child:
                if isinstance(child, _DocumentTypeState):
                    raise HierarchyRequestError()
                child = child.next_sibling
        elif isinstance(node, _TextState):
            raise HierarchyRequestError()

    def _child_inserted(self, node):
        return

    def _child_removed(self, node):
        return

    def _insert_child(self, node, before):
        if before:
            after = before.previous_sibling
            if not after:
                node.next_sibling = before
                before.previous_sibling = node
                self.first_child = node
            else:
                after.next_sibling = node
                node.previous_sibling = after
                node.next_sibling = before
                before.previous_sibling = node
        else:
            after = self.last_child
            if not after:
                self.first_child = node
                self.last_child = node
            else:
                after.next_sibling = node
                node.previous_sibling = after
                self.last_child = node
        self.num_children += 1
        self.iter_cache.invalidate()

    def _remove_child(self, node):
        predecessor = node.previous_sibling
        successor = node.next_sibling
        if predecessor:
            predecessor.next_sibling = successor
        else:
            self.first_child = successor
        if successor:
            successor.previous_sibling = predecessor
        else:
            self.last_child = predecessor
        node.previous_sibling = None
        node.next_sibling = None
        self.num_children -= 1
        self.iter_cache.invalidate()



class _DocumentImpl(_ParentNodeImpl, _NodeImpl, Document):
    def __init__(self, state):
        _NodeImpl.__init__(self, self, state)
        _ParentNodeImpl.__init__(self)

    def get_content_type(self):
        return self._state.content_type

    def get_doctype(self):
        return _wrap_node(self._state.doctype, self)

    def get_document_element(self):
        return _wrap_node(self._state.document_element, self)

    def create_element(self, local_name):
        state = self._state.create_element(local_name)
        return _wrap_node(state, self)

    def create_text_node(self, data):
        state = _TextState(data)
        return _wrap_node(state, self)

    def create_comment(self, data):
        state = _CommentState(data)
        return _wrap_node(state, self)

    def get_node_type(self):
        return self.DOCUMENT_NODE

    def get_node_name(self):
        return "#document"

    def get_owner_document(self):
        return None


class _DocumentState(_ParentNodeState, _NodeState):
    def __init__(self, content_type, is_html):
        _NodeState.__init__(self)
        _ParentNodeState.__init__(self)
        self.content_type = content_type
        self.is_html = is_html
        self.doctype = None
        self.document_element = None

    # FIXME: Must also take optional `options` argument
    def create_element(self, local_name):
        # FIXME: If localName does not match the Name production, then throw an
        # "InvalidCharacterError" DOMException.
        local_name_2 = local_name
        if self.is_html:
            local_name_2 = _ascii_lowercase(local_name_2)
        namespace_uri = None
        if self.is_html or self.content_type == "application/xhtml+xml":
            namespace_uri = "http://www.w3.org/1999/xhtml"
        prefix = None
        return _ElementState(self.is_html, namespace_uri, prefix, local_name)

    def _child_inserted(self, node):
        if isinstance(node, _DocumentTypeState):
            assert not self.doctype
            self.doctype = node
        if isinstance(node, _ElementState):
            assert not self.document_element
            self.document_element = node

    def _child_removed(self, node):
        if isinstance(node, _DocumentTypeState):
            assert self.doctype == node
            self.doctype = None
        if isinstance(node, _ElementState):
            assert self.document_element == node
            self.document_element = None

    def _do_wrap(self, document):
        return _DocumentImpl(self)



class _DocumentTypeImpl(_ChildNodeImpl, _NodeImpl, DocumentType):
    def __init__(self, document, state):
        _NodeImpl.__init__(self, document, state)

    def get_name(self):
        return self._state.name

    def get_public_id(self):
        return self._state.public_id

    def get_system_id(self):
        return self._state.system_id

    def get_node_type(self):
        return self.DOCUMENT_TYPE_NODE

    def get_node_name(self):
        return self.get_name()


class _DocumentTypeState(_ChildNodeState, _NodeState):
    def __init__(self, name, public_id, system_id):
        _NodeState.__init__(self)
        _ChildNodeState.__init__(self)
        self.name = name
        self.public_id = public_id
        self.system_id = system_id

    def _do_wrap(self, document):
        return _DocumentTypeImpl(document, self)



class _ElementImpl(_ParentNodeImpl, _ChildNodeImpl, _NodeImpl, Element):
    def __init__(self, document, state):
        _NodeImpl.__init__(self, document, state)
        _ParentNodeImpl.__init__(self)
        self._weak_attributes = None

    def get_namespace_uri(self):
        return self._state.namespace_uri

    def get_prefix(self):
        return self._state.prefix

    def get_local_name(self):
        return self._state.local_name

    def get_tag_name(self):
        state = self._state
        qualified_name = state.get_qualified_name()
        if state.is_html:
            return _ascii_uppercase(qualified_name)
        return qualified_name

    def has_attributes(self):
        return bool(self._state.attributes)

    def get_attributes(self):
        if self._weak_attributes:
            attributes = self._weak_attributes()
            if attributes:
                return attributes
        attributes = _Attributes(self)
        self._weak_attributes = weakref.ref(attributes)
        return attributes

    def remove_attribute(self, qualified_name):
        self._state.remove_attribute(qualified_name)

    def remove_attribute_ns(self, namespace, local_name):
        self._state.remove_attribute_ns(namespace, local_name)

    def get_attribute_node(self, qualified_name):
        state = self._state.get_attribute_node(qualified_name)
        return _wrap_node(state, self._document)

    def get_attribute_node_ns(self, namespace, local_name):
        state = self._state.get_attribute_node_ns(namespace, local_name)
        return _wrap_node(state, self._document)

    def set_attribute_node(self, attr):
        return self.set_attribute_node_ns(attr)

    def set_attribute_node_ns(self, attr):
        state = self._state.set_attribute_node(_unwrap_node(attr))
        return _wrap_node(state, self._document)

    def get_node_type(self):
        return self.ELEMENT_NODE

    def get_node_name(self):
        return self.get_tag_name()


class _ElementState(_ParentNodeState, _ChildNodeState, _NodeState):
    def __init__(self, document_is_html, namespace_uri, prefix, local_name):
        _NodeState.__init__(self)
        _ChildNodeState.__init__(self)
        _ParentNodeState.__init__(self)
        self.is_html = document_is_html and namespace_uri == "http://www.w3.org/1999/xhtml"
        # FIXME: Prefix and local name validation?                                         
        self.namespace_uri = namespace_uri
        self.prefix = prefix
        self.local_name = local_name
        self.attributes = []
        self.attribute_map = {}

    def get_qualified_name(self):
        if self.prefix is None:
            return self.local_name
        return "%s:%s" % (self.prefix, self.local_name)

    def get_attribute_node(self, qualified_name):
        candidates, index = self._find_attribute(qualified_name)
        return None if index is None else candidates[index]

    def get_attribute_node_ns(self, namespace, local_name):
        candidates, index = self._find_attribute_ns(namespace, local_name)
        return None if index is None else candidates[index]

    def set_attribute(self, qualified_name, value):
        # FIXME: If qualifiedName does not match the Name production in XML, then throw an "InvalidCharacterError" DOMException (but ideally this check should only be done if a new candidates map entry needs to be added).   
        key, adjusted_qualified_name = self._get_attribute_key(qualified_name)
        candidates = self.attribute_map.setdefault(key, [])
        index = self._attribute_search(candidates, adjusted_qualified_name)
        if index is not None:
            attr = candidates[index]
            attr.set_value(value)
            return
        namespace_uri = None
        prefix = None
        local_name = adjusted_qualified_name
        attr = _AttrState(namespace_uri, prefix, local_name, value)
        self._append_attribute(candidates, attr)

    def set_attribute_node(self, attr):
        # FIXME: What should happen if `attr` is the wrong kind of node? Consider requiring a particular base type in _unwrap_node()                                                                                                          
        if attr.weak_owner_element and attr.weak_owner_element != self.weak_self:
            raise InUseAttributeError()
        key = self._get_attribute_key_ns(attr.local_name)
        candidates = self.attribute_map.setdefault(key, [])
        index = self._attribute_search_ns(candidates, attr.namespace_uri, attr.local_name)
        if index is None:
            self._append_attribute(candidates, attr)
            return None
        old_attr = candidates[index]
        if old_attr != attr:
            self._replace_attribute(candidates, index, attr)
        return old_attr

    def remove_attribute(self, qualified_name):
        candidates, index = self._find_attribute(qualified_name)
        if index is None:
            return None
        return self._remove_attribute(candidates, index)

    def remove_attribute_ns(self, namespace, local_name):
        candidates, index = self._find_attribute_ns(namespace, local_name)
        if index is None:
            return None
        return self._remove_attribute(candidates, index)

    def _find_attribute(self, qualified_name):
        key, adjusted_qualified_name = self._get_attribute_key(qualified_name)
        candidates = self.attribute_map.get(key, [])
        index = self._attribute_search(candidates, adjusted_qualified_name)
        return candidates, index

    def _find_attribute_ns(self, namespace, local_name):
        key = self._get_attribute_key_ns(local_name)
        candidates = self.attribute_map.get(key, [])
        index = self._attribute_search_ns(candidates, namespace, local_name)
        return candidates, index

    def _get_attribute_key(self, qualified_name):
        adjusted_qualified_name = qualified_name
        if self.is_html:
            adjusted_qualified_name = _ascii_lowercase(adjusted_qualified_name)
        i = adjusted_qualified_name.find(":")
        key = adjusted_qualified_name if i < 0 else adjusted_qualified_name[i+1:]
        return key, adjusted_qualified_name

    def _get_attribute_key_ns(self, local_name):
        key = local_name
        if self.is_html:
            key = _ascii_lowercase(key)
        return key

    def _attribute_search(self, candidates, adjusted_qualified_name):
        for index, attr in enumerate(candidates):
            if attr.get_qualified_name() == adjusted_qualified_name:
                return candidates, index

    def _attribute_search_ns(self, candidates, namespace, local_name):
        adjusted_namespace = namespace or None
        for index, attr in enumerate(candidates):
            if attr.namespace_uri == adjusted_namespace and attr.local_name == local_name:
                return index
        return None

    def _append_attribute(self, candidates, attr):
        self.attributes.append(attr)
        candidates.append(attr)
        attr.weak_owner_element = self.weak_self

    def _replace_attribute(self, candidates, index, attr):
        old_attr = candidates[index]
        for i, attr_2 in enumerate(self.attributes):
            if attr_2 == old_attr:
                self.attributes[i] = attr
                break
        candidates[index] = attr
        old_attr.weak_owner_element = None
        attr.weak_owner_element = self.weak_self

    def _remove_attribute(self, candidates, index):
        attr = candidates[index]
        self.attributes.remove(attr)
        del candidates[index]
        attr.weak_owner_element = None
        return attr

    def _do_wrap(self, document):
        return _ElementImpl(document, self)



class _CharacterDataImpl(_ChildNodeImpl, _NodeImpl, CharacterData):
    def __init__(self, document, state):
        _NodeImpl.__init__(self, document, state)

    def get_data(self):
        return self._state.data

    def set_data(self, data):
        self._state.data = _legacy_null_to_empty_string(data)


class _CharacterDataState(_ChildNodeState, _NodeState):
    def __init__(self, data):
        _NodeState.__init__(self)
        _ChildNodeState.__init__(self)
        self.data = _default_null_coercion(data)



class _TextImpl(_CharacterDataImpl, Text):
    def get_node_type(self):
        return self.TEXT_NODE

    def get_node_name(self):
        return "#text"


class _TextState(_CharacterDataState):
    def _do_wrap(self, document):
        return _TextImpl(document, self)



class _CommentImpl(_CharacterDataImpl, Comment):
    def get_node_type(self):
        return self.COMMENT_NODE

    def get_node_name(self):
        return "#comment"


class _CommentState(_CharacterDataState):
    def _do_wrap(self, document):
        return _CommentImpl(document, self)



class _AttrImpl(_NodeImpl, Attr):
    def __init__(self, document, state):
        _NodeImpl.__init__(self, document, state)

    def get_namespace_uri(self):
        return self._state.namespace_uri

    def get_prefix(self):
        return self._state.prefix

    def get_local_name(self):
        return self._state.local_name

    def get_name(self):
        return self._state.get_qualified_name()

    def get_value(self):
        return self._state.value

    def set_value(self, value):
        self._state.value = _default_null_coercion(value)

    def get_owner_element(self):
        return self._state.weak_owner_element and self._state.weak_owner_element()

    def is_specified(self):
        return True

    def get_node_type(self):
        return self.ATTRIBUTE_NODE

    def get_node_name(self):
        return self.get_qualified_name()


class _AttrState(_NodeState):
    def __init__(self, namespace_uri, prefix, local_name, value):
        _NodeState.__init__(self)
        # FIXME: Prefix and local name validation?                                         
        self.namespace_uri = namespace_uri
        self.prefix = prefix
        self.local_name = local_name
        self.value = _default_null_coercion(value)
        self.weak_owner_element = None

    def get_qualified_name(self):
        if self.prefix is None:
            return self.local_name
        return "%s:%s" % (self.prefix, self.local_name)

    def _do_wrap(self, document):
        return _AttrImpl(document, self)



class _ChildNodes(NodeList):
    def __init__(self, parent_node, iter_cache):
        self._parent_node = parent_node
        self._iter_cache = iter_cache

    def __getitem__(self, index):
        if index < 0:
            index += self.get_length()
        node = self.item(index)
        if not node:
            raise IndexError()
        return node

    def __len__(self):
        return self.get_length()

    def get_length(self):
        return self._parent_node._state.num_children

    def item(self, index):
        state = self._iter_cache.get(self._parent_node._state, index)
        return _wrap_node(state, self._parent_node._document)



class _ChildNodesIterCache:
    def __init__(self):
        self._valid = False
        self._index = 0
        self._node = None

    def get(self, parent_node, index):
        self._ensure(parent_node)
        self._adjust(parent_node, self._index, index)
        self._index = index
        return self._node

    def invalidate(self):
        self._valid = False

    def _ensure(self, parent_node):
        if self._valid:
            return
        self._node = parent_node.first_child
        self._adjust(parent_node, 0, self._index)
        self._valid = True

    def _adjust(self, parent_node, i, j):
        size = parent_node.num_children
        node = None
        if j >= 0 and j < size:
            node = self._node
            i_2 = i
            if j >= i:
                # Advance
                if i_2 < 0:
                    i_2 = 0
                    node = parent_node.first_child
                for _ in range(j - i_2):
                    node = node.next_sibling
            else:
                # Recede
                if i_2 >= size:
                    i_2 = size - 1
                    node = parent_node.last_child
                for _ in range(i_2 - j):
                    node = node.previous_sibling
        self._node = node



class _Attributes(NamedNodeMap):
    def __init__(self, element):
        self._element = element

    def __getitem__(self, qualified_name):
        attr = self.get_named_item(name)
        if not attr:
            raise KeyError
        return attr

    def __setitem__(self, qualified_name, value):
        self._element._state.set_attribute(qualified_name, value)

    def __delitem__(self, qualified_name):
        self.remove_named_item(qualified_name)

    def __iter__(self):
        return _AttributeIter(self._element)

    def __len__(self):
        return len(self._element._state.attributes)

    def get_length(self):
        return len(self._element._state.attributes)

    def item(self, index):
        if index < 0 or index >= len(self._list):
            return None
        return self._element._state.attributes[index]

    def get_named_item(self, qualified_name):
        state = self._element._state.get_attribute_node(qualified_name)
        return _wrap_node(state, self._element._document)

    def get_named_item_ns(self, namespace, local_name):
        state = self._element._state.get_attribute_node_ns(namespace, local_name)
        return _wrap_node(state, self._element._document)

    def set_named_item(self, attr):
        return self.set_named_item_ns(attr)

    def set_named_item_ns(self, attr):
        state = self._element._state.set_attribute_node(_unwrap_node(attr))
        return _wrap_node(state, self._element._document)

    def remove_named_item(self, qualified_name):
        state = self._element._state.remove_attribute(qualified_name)
        return _wrap_node(state, self._element._document)

    def remove_named_item_ns(self, namespace, local_name):
        state = self._element._state.remove_attribute_ns(namespace, local_name)
        return _wrap_node(state, self._element._document)



class _AttributeIter:
    def __init__(self, element):
        self._element = element
        self._index = -1

    def __next__(self):
        if not self._index is not None:
            self._index += 1
            if self._index < len(self._element._state.attributes):
                state = self._element._state.attributes[self._index]
                return _wrap_node(state, self._element._document)
            self._index = None
        raise StopIteration()



def _wrap_node(state, document):
    return state.wrap(document) if state else None

def _unwrap_node(node):
    if not node:
        return None
    assert isinstance(node, _NodeImpl)
    return node._unwrap()


def _default_null_coercion(string):
    return "null" if string is None else string

def _legacy_null_to_empty_string(string):
    return "" if string is None else string


def _ascii_uppercase(string):
    return "".join([ch.upper() if ch.isascii() else ch for ch in string])

def _ascii_lowercase(string):
    return "".join([ch.lower() if ch.isascii() else ch for ch in string])
