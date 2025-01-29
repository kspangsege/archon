import weakref
import collections.abc
import archon.core


# Reference snapshot: https://dom.spec.whatwg.org/commit-snapshots/369654b7697f08dfd813aee0ce9064ae7b24e3ec/



class DOMImplementation:
    def create_document_type(self, qualified_name, public_id, system_id):
        raise NotImplementedError

    def create_document(self, namespace, qualified_name, doctype):
        raise NotImplementedError

    def create_html_document(self, title):
        raise NotImplementedError

    def has_feature(self):
        raise NotImplementedError



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
        raise NotImplementedError

    def get_node_name(self):
        raise NotImplementedError

    def get_owner_document(self):
        raise NotImplementedError

    def get_parent_node(self):
        raise NotImplementedError

    def has_child_nodes(self):
        raise NotImplementedError

    def get_child_nodes(self):
        raise NotImplementedError

    def get_first_child(self):
        raise NotImplementedError

    def get_last_child(self):
        raise NotImplementedError

    def get_previous_sibling(self):
        raise NotImplementedError

    def get_next_sibling(self):
        raise NotImplementedError

    def contains(self, other):
        raise NotImplementedError

    def insert_before(self, node, child):
        raise NotImplementedError

    def append_child(self, node):
        raise NotImplementedError

    def replace_child(self, node, child):
        raise NotImplementedError

    def remove_child(self, node):
        raise NotImplementedError


class ChildNode:
    pass


class ParentNode:
    pass


class Document(ParentNode, Node):
    def get_implementation(self):
        raise NotImplementedError

    def get_content_type(self):
        raise NotImplementedError

    def get_doctype(self):
        raise NotImplementedError

    def get_document_element(self):
        raise NotImplementedError

    def create_element(self, local_name):
        raise NotImplementedError

    def create_element_ns(self, namespace, qualified_name):
        raise NotImplementedError

    def create_text_node(self, data):
        raise NotImplementedError

    def create_comment(self, data):
        raise NotImplementedError

    def create_attribute(self, local_name):
        raise NotImplementedError

    def create_attribute_ns(self, namespace, qualified_name):
        raise NotImplementedError


class XMLDocument(Document):
    pass


class DocumentType(ChildNode, Node):
    def get_name(self):
        raise NotImplementedError

    def get_public_id(self):
        raise NotImplementedError

    def get_system_id(self):
        raise NotImplementedError


class Element(ParentNode, ChildNode, Node):
    def get_namespace_uri(self):
        raise NotImplementedError

    def get_prefix(self):
        raise NotImplementedError

    def get_local_name(self):
        raise NotImplementedError

    def get_tag_name(self):
        raise NotImplementedError

    # FIXME: Add attribute covering getters and setters: `get_id()`, `set_id()` cover `id`
    # attribute; `get_class_name()`, `set_class_name()` cover `class` attribute, etc.

    def has_attributes(self):
        raise NotImplementedError

    def get_attributes(self):
        raise NotImplementedError

    def get_attribute_names(self):
        raise NotImplementedError

    def get_attribute(self, qualified_name):
        raise NotImplementedError

    def get_attribute_ns(self, namespace, local_name):
        raise NotImplementedError

    def set_attribute(self, qualified_name, value):
        raise NotImplementedError

    def set_attribute_ns(self, namespace, qualified_name, value):
        raise NotImplementedError

    def remove_attribute(self, qualified_name):
        raise NotImplementedError

    def remove_attribute_ns(self, namespace, local_name):
        raise NotImplementedError

    def toggle_attribute(self, qualified_name, force = None):
        raise NotImplementedError

    def has_attribute(self, qualified_name):
        raise NotImplementedError

    def has_attribute_ns(self, namespace, local_name):
        raise NotImplementedError

    def get_attribute_node(self, qualified_name):
        raise NotImplementedError

    def get_attribute_node_ns(self, namespace, local_name):
        raise NotImplementedError

    def set_attribute_node(self, attr):
        raise NotImplementedError

    def set_attribute_node_ns(self, attr):
        raise NotImplementedError

    def remove_attribute_node(self, attr):
        raise NotImplementedError


class CharacterData(ChildNode, Node):
    def get_data(self):
        raise NotImplementedError

    def set_data(self, data):
        raise NotImplementedError


class Text(CharacterData):
    pass


# FIXME: Add CDATASection


class Comment(CharacterData):
    pass


# FIXME: Add ProcessingInstruction


class Attr(Node):
    def get_namespace_uri(self):
        raise NotImplementedError

    def get_prefix(self):
        raise NotImplementedError

    def get_local_name(self):
        raise NotImplementedError

    def get_name(self):
        raise NotImplementedError

    def get_value(self):
        raise NotImplementedError

    def set_value(self, value):
        raise NotImplementedError

    def get_owner_element(self):
        raise NotImplementedError

    def is_specified(self):
        raise NotImplementedError



class NodeList(collections.abc.Sequence):
    def get_length(self):
        raise NotImplementedError

    def item(self, index):
        raise NotImplementedError


class NamedNodeMap(collections.abc.Mapping):
    def get_length(self):
        raise NotImplementedError

    def item(self, index):
        raise NotImplementedError

    def get_named_item(self, qualified_name):
        raise NotImplementedError

    def get_named_item_ns(self, namespace, local_name):
        raise NotImplementedError

    def set_named_item(self, attr):
        raise NotImplementedError

    def set_named_item_ns(self, attr):
        raise NotImplementedError

    def remove_named_item(self, qualified_name):
        raise NotImplementedError

    def remove_named_item_ns(self, namespace, local_name):
        raise NotImplementedError



class DOMException(Exception):
    pass


class HierarchyRequestError(DOMException):
    pass

class InvalidCharacterError(DOMException):
    pass

class NotFoundError(DOMException):
    pass

class InUseAttributeError(DOMException):
    pass

class NamespaceError(DOMException):
    pass



# This one is non-standard and implementation agnostic
def dump_document(document, max_string_size = 90):
    assert isinstance(document, Document)
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
        return archon.core.quote(namespace_uri or "")
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
            dump("Doctype(%s, %s, %s)" % (archon.core.quote(node.get_name()),
                                          archon.core.quote(node.get_public_id()),
                                          archon.core.quote(node.get_system_id())), level)
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
            for attr in node.get_attributes().values():
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




# Implementation specific stuff below


# All document type nodes (`DocumentType`) created in the context of this implementation
# are guaranteed to have a name that is minimally valid (see definition below). Note that
# a stronger guarantee holds for document type nodes that were created through the
# standard DOM API (`DOMImplementation.create_document_type()`). These will have a name
# that conforms to the `QName` production of the XML standard.
#
# All element and attribute nodes (`Element`, `Attr`) created in the context of this
# implementation are guaranteed to satisfy the namespaced naming constraints as defined
# below. Note that a stronger guarantee holds for element and attribute node that were
# created through the standard DOM API (`Document.create_element()`,
# `Document.create_attribute()`). These will have a local name (`local_name`) that
# conforms to the `Name` production of the XML standard.
#
# INVARIANT: Two attributes (`Attr`) of an element (`Element`), that was created in the
# context of this implementation, cannot have the same namespace (`namespace_uri`) and
# qualified name (`get_name()`). Note that the *qualified name* is the local name
# (`local_name`) if the prefix (`prefix`) is None. Otherwise, it is the prefix followed by
# `:` followed by the local name.
#
# It follows from the previous invariant that two attributes of an element, that was
# created in the context of this implementation, cannot have the same namespace, prefix,
# and local name.
#
# These are the *namespaced naming constraints*:
#
#  - `namespace_uri` is `None` or not empty
#  - `local_name` is not `None`
#  - `local_name` is minimally valid (see definition below)
#  - If `prefix` is not `None`, it matches the `NCName` production of the XML standard (is
#    not empty and does not contain `:`)
#  - If `prefix` is not `None`, `local_name` matches the `NCName` production of the XML
#    standard (it does not contain `:`)
#  - If `prefix` is not `None`, `namespace_uri` is not `None`
#  - If `prefix` is `"xml"`, `namespace_uri` is
#    `"http://www.w3.org/XML/1998/namespace"`
#  - If `prefix` is `"xmlns"`, `namespace_uri` is `"http://www.w3.org/2000/xmlns/"`
#  - If `namespace_uri` is `"http://www.w3.org/2000/xmlns/"`, either `prefix` is
#    `"xmlns"`, or `prefix` is `None` and `local_name` is `"xmlns"`
#
# A name is *minimally valid* if it is nonempty and does not contain U+0000 NULL, U+0009
# CHARACTER TABULATION, U+000A LINE FEED, U+000C FORM FEED, U+000D CARRIAGE RETURN, U+0020
# SPACE, and U+002F SOLIDUS (/).



# `content_type` must be a string or `None`.
#
def create_xml_document(content_type = None):
    content_type_2 = "application/xml"
    if content_type is not None:
        if type(content_type) != str:
            raise TypeError
        content_type_2 = content_type
    is_html = False
    state = _XMLDocumentState(content_type_2, is_html)
    return _wrap_node(state, None)

# `content_type` must be a string or `None`.
#
def create_html_document(content_type = None):
    content_type_2 = "text/html"
    if content_type is not None:
        if type(content_type) != str:
            raise TypeError
        content_type_2 = content_type
    is_html = True
    state = _DocumentState(content_type_2, is_html)
    return _wrap_node(state, None)

# `document` must be a document node (`Document`) produced in the context of this DOM
# implementation. It cannot be `None`.
#
# `name`, `public_id` and `private_id` must be strings. Neither can be `None`.
#
# `name` must be empty or minimally valid (see definition above).
#
def create_document_type(document, name, public_id, system_id):
    document_state = _unwrap_typed_node(document, Document)
    if type(name) != str or type(public_id) != str or type(system_id) != str:
        raise TypeError
    if name and not _is_minimally_valid_name(name):
        raise ValueError
    state = _DocumentTypeState(document_state, name, public_id, system_id)
    return _wrap_node(state, document)

# `document` must be a document node (`Document`) produced in the context of this DOM
# implementation. It cannot be `None`.
#
# `namespace_uri` and `prefix` must be strings or `None`. `local_name` must be a string and
# cannot be `None`.
#
# `namespace_uri`, `prefix`, and `local_name` must satisfy the namespaced naming constraints
# as defined above.
#
# `attributes` must be an iterable sequence of zero or more attribute nodes (`Attr`). The
# attribute nodes must have been created in the context of this DOM implementation. No two
# of the attributes are allowed to have the same namespace (`namespace_uri`) and local name
# (`local_name`). None of the attributes are allowed to have an owner element
# (`Attr.get_owner_element()`).
#
def create_element(document, namespace_uri, prefix, local_name, attributes):
    document_state = _unwrap_typed_node(document, Document)
    if not document_state:
        raise TypeError
    if namespace_uri is not None and type(namespace_uri) != str:
        raise TypeError
    if prefix is not None and type(prefix) != str:
        raise TypeError
    if type(local_name) != str:
        raise TypeError
    if not _is_valid_naming(namespace_uri, prefix, local_name):
        raise ValueError
    document_is_html = document_state.is_html
    element_state = _ElementState(document_state, document_is_html, namespace_uri, prefix, local_name)
    for i, attr in enumerate(attributes):
        try:
            element_state.append_attribute_node(_unwrap_typed_node(attr, Attr))
        except InUseAttributeError:
            element_state.remove_all_attributes()
            raise ValueError
        except:
            element_state.remove_all_attributes()
            raise
    return _wrap_node(element_state, document)

# `document` must be a document node (`Document`) produced in the context of this DOM
# implementation. It cannot be `None`.
#
# `namespace_uri` and `prefix` must be strings or `None`. `local_name` must be a string and
# cannot be `None`.
#
# `namespace_uri`, `prefix`, and `local_name` must satisfy the namespaced naming constraints
# as defined above.
#
# `value` must be a string and cannot be `None`.
#
def create_attribute(document, namespace_uri, prefix, local_name, value):
    document_state = _unwrap_typed_node(document, Document)
    if not document_state:
        raise TypeError
    if namespace_uri is not None and type(namespace_uri) != str:
        raise TypeError
    if prefix is not None and type(prefix) != str:
        raise TypeError
    if type(local_name) != str:
        raise TypeError
    if type(value) != str:
        raise TypeError
    if not _is_valid_naming(namespace_uri, prefix, local_name):
        raise ValueError
    state = _AttrState(namespace_uri, prefix, local_name, value)
    return _wrap_node(state, document)








class _NodeImpl(Node):
    def __init__(self, document, state):
        assert document
        assert isinstance(document, _DocumentImpl)
        self._document = document
        self._state = state

    def __str__(self):
        return self.get_node_name()

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
        raise HierarchyRequestError

    def append_child(self, node):
        raise HierarchyRequestError

    def replace_child(self, node, child):
        raise HierarchyRequestError

    def remove_child(self, node):
        raise NotFoundError

    def _unwrap(self):
        return self._state


class _NodeState:
    def __init__(self):
        self.weak_wrapper = None

    def get_weak_parent_node(self):
        return None

    def contains(self, other):
        return False

    def wrap(self, document):
        wrapper = self.try_get_wrapper()
        if wrapper:
            return wrapper
        wrapper = self._do_wrap(document)
        self.weak_wrapper = weakref.ref(wrapper)
        return wrapper

    def try_get_wrapper(self):
        return self.weak_wrapper and self.weak_wrapper()

    def _do_wrap(self, document):
        raise NotImplementedError



class _ChildNodeImpl(ChildNode):
    def get_parent_node(self):
        return _wrap_node(self._state.get_parent_node(), self._document)

    def get_previous_sibling(self):
        return _wrap_node(self._state.previous_sibling, self._document)

    def get_next_sibling(self):
        return _wrap_node(self._state.next_sibling, self._document)


class _ChildNodeState:
    def __init__(self, owner_document):
        self.weak_owner_document = owner_document.weak_self
        self.weak_parent_node = None
        self.previous_sibling = None
        self.next_sibling = None

    def get_weak_parent_node(self):
        return self.weak_parent_node

    def get_parent_node(self):
        return self.weak_parent_node and self.weak_parent_node()

    def migrate_if_needed(self, document_wrapper):
        assert isinstance(document_wrapper, _DocumentImpl)
        if self.weak_owner_document == document_wrapper._state.weak_self:
            return
        self._migrate(document_wrapper)

    def _migrate(self, document_wrapper):
        if isinstance(self, _ParentNodeState):
            child = self.first_child
            while child:
                child._migrate(document_wrapper)
                child = child.next_sibling
        document = document_wrapper._state
        self.weak_owner_document = document.weak_self
        wrapper = self.try_get_wrapper()
        if wrapper:
            wrapper._document = document_wrapper



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
        if other is None:
            return False
        return self._state.contains(_unwrap_node(other))

    def insert_before(self, node, child):
        document = self._document or self
        self._state.insert_before(_unwrap_node(node), _unwrap_optional_node(child), document)

    def append_child(self, node):
        document = self._document or self
        self._state.append_child(_unwrap_node(node), document)

    def replace_child(self, node, child):
        document = self._document or self
        self._state.replace_child(_unwrap_node(node), _unwrap_node(child), document)

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

    def insert_before(self, node, child, document_wrapper):
        before = child
        self._validate_child_insertion(node, before)
        parent = node.get_parent_node()
        if parent:
            # FIXME: Oops, something needs to be done here if `node` and `before` are the same node                       
            parent._remove_child(node)
        node.migrate_if_needed(document_wrapper)
        self._insert_child(node, before)
        node.weak_parent_node = self.weak_self
        self._child_inserted(node)

    def append_child(self, node, document_wrapper):
        child = None
        self.insert_before(node, child, document_wrapper)

    def replace_child(self, node, child, document_wrapper):
        orig = child
        self._validate_child_replacement(node, orig)
        parent = node.get_parent_node()
        if parent:
            # FIXME: Oops, something needs to be done here if `node` and `orig` are the same node                       
            parent._remove_child(node)
        node.migrate_if_needed(document_wrapper)
        self._replace_child(node, orig)                                                                                                               
        child.weak_parent_node = None
        node.weak_parent_node = self.weak_self
        self._child_removed(child)
        self._child_inserted(node)

    def remove_child(self, node):
        if node.get_weak_parent_node() != self.weak_self:
            raise NotFoundError
        self._remove_child(node)
        node.weak_parent_node = None
        self._child_removed(node)
        return node

    # `node` may already be a child of `self`
    # `node` and `before` may be the same node
    def _validate_child_insertion(self, node, before):
        # FIXME: Need to take DocumentFragment into account below
        if not isinstance(node, _ChildNodeState):
            raise HierarchyRequestError
        # FIXME: This check must be based on the "host-including inclusive ancestor"
        # instead of the "inclusive ancestor"
        if node.contains(self):
            raise HierarchyRequestError
        if before and before.get_weak_parent_node() != self.weak_self:
            raise NotFoundError
        if not isinstance(self, _DocumentState):
            if isinstance(node, _DocumentTypeState):
                raise HierarchyRequestError
            return
        if isinstance(node, _DocumentTypeState):
            if self.doctype: # FIXME: Not good if `node` is already a child of `self`                    
                raise HierarchyRequestError
            child = self.first_child
            while child != before:
                if isinstance(child, _ElementState):
                    raise HierarchyRequestError
                child = child.next_sibling
        elif isinstance(node, _ElementState):
            if self.document_element: # FIXME: Not good if `node` is already a child of `self`                    
                raise HierarchyRequestError
            if before:
                child = before
                while child:
                    if isinstance(child, _DocumentTypeState):
                        raise HierarchyRequestError
                    child = child.next_sibling
        elif isinstance(node, _TextState):
            raise HierarchyRequestError

    # `node` may already be a child of `self`
    # `node` and `orig` may be the same node
    def _validate_child_replacement(self, node, orig):
        # FIXME: Need to take DocumentFragment into account below
        if not isinstance(node, _ChildNodeState):
            raise HierarchyRequestError
        # FIXME: This check must be based on the "host-including inclusive ancestor"
        # instead of the "inclusive ancestor"
        if node.contains(self):
            raise HierarchyRequestError
        if orig.get_weak_parent_node() != self.weak_self:
            raise NotFoundError
        if not isinstance(self, _DocumentState):
            if isinstance(node, _DocumentTypeState):
                raise HierarchyRequestError
            return
        if isinstance(node, _DocumentTypeState):
            # FIXME: Does this work if `node` is already a child of `self`?                    
            if self.doctype:
                if self.doctype != orig:
                    raise HierarchyRequestError
                return
            child = self.first_child
            while child != orig:
                if isinstance(child, _ElementState):
                    raise HierarchyRequestError
                child = child.next_sibling
        elif isinstance(node, _ElementState):
            # FIXME: Does this work if `node` is already a child of `self`?                    
            if self.document_element:
                if self.document_element != orig:
                    raise HierarchyRequestError
                return
            child = orig.next_sibling
            while child:
                if isinstance(child, _DocumentTypeState):
                    raise HierarchyRequestError
                child = child.next_sibling
        elif isinstance(node, _TextState):
            raise HierarchyRequestError

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
        self._weak_implementation = None

    def get_implementation(self):
        if self._weak_implementation:
            implementation = self._weak_implementation()
            if implementation:
                return implementation
        implementation = _Implementation(self)
        self._weak_implementation = weakref.ref(implementation)
        return implementation

    def get_content_type(self):
        return self._state.content_type

    def get_doctype(self):
        return _wrap_node(self._state.doctype, self)

    def get_document_element(self):
        return _wrap_node(self._state.document_element, self)

    def create_element(self, local_name):
        state = self._state.create_element(local_name)
        return _wrap_node(state, self)

    def create_element_ns(self, namespace, qualified_name):
        state = self._state.create_element_ns(namespace, qualified_name)
        return _wrap_node(state, self)

    def create_text_node(self, data):
        state = self._state.create_text_node(data)
        return _wrap_node(state, self)

    def create_comment(self, data):
        state = self._state.create_comment(data)
        return _wrap_node(state, self)

    def create_attribute(self, local_name):
        state = self._state.create_attribute(local_name)
        return _wrap_node(state, self)

    def create_attribute_ns(self, namespace, qualified_name):
        state = self._state.create_attribute_ns(namespace, qualified_name)
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
        if not _is_valid_general_name(local_name):
            raise InvalidCharacterError
        document_is_html = self.is_html
        local_name_2 = local_name
        if document_is_html:
            local_name_2 = _ascii_lowercase(local_name_2)
        namespace_uri = None
        if document_is_html or self.content_type == "application/xhtml+xml":
            namespace_uri = "http://www.w3.org/1999/xhtml"
        prefix = None
        return _ElementState(self, document_is_html, namespace_uri, prefix, local_name_2)

    # FIXME: Must also take optional `options` argument
    def create_element_ns(self, namespace, qualified_name):
        document_is_html = self.is_html
        adjusted_namespace, prefix, local_name = _parse_qualified_name(namespace, qualified_name)
        return _ElementState(self, document_is_html, adjusted_namespace, prefix, local_name)

    def create_text_node(self, data):
        return _TextState(self, data)

    def create_comment(self, data):
        return _CommentState(self, data)

    def create_attribute(self, local_name):
        if not _is_valid_general_name(local_name):
            raise InvalidCharacterError
        local_name_2 = local_name
        if self.is_html:
            local_name_2 = _ascii_lowercase(local_name_2)
        namespace_uri = None
        prefix = None
        return _AttrState(namespace_uri, prefix, local_name_2)

    def create_attribute_ns(self, namespace, qualified_name):
        adjusted_namespace, prefix, local_name = _parse_qualified_name(namespace, qualified_name)
        return _AttrState(adjusted_namespace, prefix, local_name)

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



class _XMLDocumentImpl(_DocumentImpl, XMLDocument):
    pass


class _XMLDocumentState(_DocumentState):
    def _do_wrap(self, document):
        return _XMLDocumentImpl(self)



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
    def __init__(self, owner_document, name, public_id, system_id):
        _NodeState.__init__(self)
        _ChildNodeState.__init__(self, owner_document)
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

    def get_attribute_names(self):
        return [attr.get_qualified_name() for attr in self._state.attributes]

    def get_attribute(self, qualified_name):
        return self._state.get_attribute(qualified_name)

    def get_attribute_ns(self, namespace, local_name):
        return self._state.get_attribute_ns(namespace, local_name)

    def set_attribute(self, qualified_name, value):
        self._state.set_attribute(qualified_name, value)

    def set_attribute_ns(self, namespace, qualified_name, value):
        self._state.set_attribute_ns(namespace, qualified_name, value)

    def remove_attribute(self, qualified_name):
        self._state.remove_attribute(qualified_name)

    def remove_attribute_ns(self, namespace, local_name):
        self._state.remove_attribute_ns(namespace, local_name)

    def toggle_attribute(self, qualified_name, force = None):
        return self._state.toggle_attribute(qualified_name, force)

    def has_attribute(self, qualified_name):
        return self._state.has_attribute(qualified_name)

    def has_attribute_ns(self, namespace, local_name):
        return self._state.has_attribute_ns(namespace, local_name)

    def get_attribute_node(self, qualified_name):
        state = self._state.get_attribute_node(qualified_name)
        return _wrap_node(state, self._document)

    def get_attribute_node_ns(self, namespace, local_name):
        state = self._state.get_attribute_node_ns(namespace, local_name)
        return _wrap_node(state, self._document)

    def set_attribute_node(self, attr):
        return self.set_attribute_node_ns(attr)

    def set_attribute_node_ns(self, attr):
        state = self._state.set_attribute_node(_unwrap_typed_node(attr, Attr))
        return _wrap_node(state, self._document)

    def remove_attribute_node(self, attr):
        self._state.remove_attribute_node(_unwrap_typed_node(attr, Attr))
        return attr

    def get_node_type(self):
        return self.ELEMENT_NODE

    def get_node_name(self):
        return self.get_tag_name()


class _ElementState(_ParentNodeState, _ChildNodeState, _NodeState):
    def __init__(self, owner_document, document_is_html, namespace_uri, prefix, local_name):
        _NodeState.__init__(self)
        _ChildNodeState.__init__(self, owner_document)
        _ParentNodeState.__init__(self)
        self.is_html = document_is_html and namespace_uri == "http://www.w3.org/1999/xhtml"
        self.namespace_uri = namespace_uri
        self.prefix = prefix
        self.local_name = local_name
        self.attributes = []
        self.attribute_map = {}

    def get_qualified_name(self):
        if self.prefix is None:
            return self.local_name
        return "%s:%s" % (self.prefix, self.local_name)

    def get_attribute(self, qualified_name):
        candidates, index = self._find_attribute(qualified_name)
        if index is None:
            return None
        attr = candidates[index]
        return attr.value

    def get_attribute_ns(self, namespace, local_name):
        candidates, index = self._find_attribute_ns(namespace, local_name)
        if index is None:
            return None
        attr = candidates[index]
        return attr.value

    def set_attribute(self, qualified_name, value):
        key, adjusted_qualified_name = self._get_attribute_key(qualified_name)
        candidates = self.attribute_map.setdefault(key, [])
        index = self._attribute_search(candidates, adjusted_qualified_name)
        if index is not None:
            attr = candidates[index]
            attr.set_value(value)
            return
        local_name = adjusted_qualified_name
        if not _is_valid_general_name(local_name):
            if not candidates:
                del self.attribute_map[key]
            raise InvalidCharacterError
        namespace_uri = None
        prefix = None
        attr = _AttrState(namespace_uri, prefix, local_name)
        attr.set_value(value)
        self._append_attribute(candidates, attr)

    def set_attribute_ns(self, namespace, qualified_name, value):
        key, prefix, local_name = self._get_attribute_key_ns_q(qualified_name)
        candidates = self.attribute_map.setdefault(key, [])
        index = self._attribute_search_ns(candidates, namespace, local_name)
        if index is not None:
            attr = candidates[index]
            attr.set_value(value)
            return
        if not _is_valid_qualified_name(qualified_name):
            if not candidates:
                del self.attribute_map[key]
            raise InvalidCharacterError
        adjusted_namespace = namespace or None
        if not _is_valid_name_parts_combination(adjusted_namespace, prefix, local_name):
            if not candidates:
                del self.attribute_map[key]
            raise NamespaceError
        attr = _AttrState(adjusted_namespace, prefix, local_name)
        attr.set_value(value)
        self._append_attribute(candidates, attr)

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

    def toggle_attribute(self, qualified_name, force):
        key, adjusted_qualified_name = self._get_attribute_key(qualified_name)
        candidates = self.attribute_map.setdefault(key, [])
        index = self._attribute_search(candidates, adjusted_qualified_name)
        if index is not None:
            if force:
                return True
            self._remove_attribute(candidates, index)
            return False
        local_name = adjusted_qualified_name
        if not _is_valid_general_name(local_name):
            if not candidates:
                del self.attribute_map[key]
            raise InvalidCharacterError
        if force is not None and not force:
            return False
        namespace_uri = None
        prefix = None
        attr = _AttrState(namespace_uri, prefix, local_name)
        self._append_attribute(candidates, attr)
        return True

    def has_attribute(self, qualified_name):
        _, index = self._find_attribute(qualified_name)
        return index is not None

    def has_attribute_ns(self, namespace, local_name):
        _, index = self._find_attribute_ns(namespace, local_name)
        return index is not None

    def get_attribute_node(self, qualified_name):
        candidates, index = self._find_attribute(qualified_name)
        return None if index is None else candidates[index]

    def get_attribute_node_ns(self, namespace, local_name):
        candidates, index = self._find_attribute_ns(namespace, local_name)
        return None if index is None else candidates[index]

    def append_attribute_node(self, attr):
        if attr.weak_owner_element and attr.weak_owner_element != self.weak_self:
            raise InUseAttributeError
        key = self._get_attribute_key_ns(attr.local_name)
        candidates = self.attribute_map.setdefault(key, [])
        index = self._attribute_search_ns(candidates, attr.namespace_uri, attr.local_name)
        if index is not None:
            if not candidates:
                del self.attribute_map[key]
            raise ValueError
        self._append_attribute(candidates, attr)

    def set_attribute_node(self, attr):
        if attr.weak_owner_element and attr.weak_owner_element != self.weak_self:
            raise InUseAttributeError
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

    def remove_attribute_node(self, attr):
        if attr.weak_owner_element != self.weak_self:
            raise NotFoundError
        key = self._get_attribute_key_ns(attr.local_name)
        candidates = self.attribute_map.get(key)
        assert candidates is not None
        self.attributes.remove(attr)
        candidates.remove(attr)
        attr.weak_owner_element = None

    def remove_all_attributes(self):
        for attr in self.attributes:
            attr.weak_owner_element = None
        self.attributes.clear()
        self.attribute_map.clear()

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

    def _get_attribute_key_ns_q(self, qualified_name):
        i = qualified_name.find(":")
        prefix = None if i < 0 else qualified_name[:i]
        local_name = qualified_name if i < 0 else qualified_name[i+1:]
        key = self._get_attribute_key_ns(local_name)
        return key, prefix, local_name

    def _attribute_search(self, candidates, adjusted_qualified_name):
        for index, attr in enumerate(candidates):
            if attr.get_qualified_name() == adjusted_qualified_name:
                return index
        return None

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
    def __init__(self, owner_document, data):
        _NodeState.__init__(self)
        _ChildNodeState.__init__(self, owner_document)
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
        self._state.set_value(value)

    def get_owner_element(self):
        state = self._state.weak_owner_element and self._state.weak_owner_element()
        return _wrap_node(state, self._document)

    def is_specified(self):
        return True

    def get_node_type(self):
        return self.ATTRIBUTE_NODE

    def get_node_name(self):
        return self.get_name()


class _AttrState(_NodeState):
    def __init__(self, namespace_uri, prefix, local_name, value = ""):
        _NodeState.__init__(self)
        self.namespace_uri = namespace_uri
        self.prefix = prefix
        self.local_name = local_name
        self.value = value
        self.weak_owner_element = None

    def set_value(self, value):
        self.value = _default_null_coercion(value)            

    def get_qualified_name(self):
        if self.prefix is None:
            return self.local_name
        return "%s:%s" % (self.prefix, self.local_name)

    def _do_wrap(self, document):
        return _AttrImpl(document, self)



class _Implementation(DOMImplementation):
    def __init__(self, document):
        self._document = document

    def create_document_type(self, qualified_name, public_id, system_id):
        if type(qualified_name) != str or type(public_id) != str or type(system_id) != str:
            raise TypeError
        if not _is_valid_qualified_name(qualified_name):
            raise InvalidCharacterError
        owner_document = self._document._state
        state = _DocumentTypeState(owner_document, qualified_name, public_id, system_id)
        return _wrap_node(state, self._document)

    def create_document(self, namespace, qualified_name, doctype):
        content_type = "application/xml"
        if namespace == "http://www.w3.org/1999/xhtml":
            content_type = "application/xhtml+xml"
        elif namespace == "http://www.w3.org/2000/svg":
            content_type = "image/svg+xml"
        is_html = False
        document_state = _XMLDocumentState(content_type, is_html)
        document = _wrap_node(document_state, None)
        if doctype:
            document_state.append_child(_unwrap_typed_node(doctype, DocumentType), document)
        if qualified_name:
            element_state = document_state.create_element_ns(namespace, qualified_name)
            document_state.append_child(element_state, document)
        return document

    def create_html_document(self, title):
        content_type = "text/html"
        is_html = True
        document_state = _DocumentState(content_type, is_html)
        document = _wrap_node(document_state, None)
        public_id = None
        system_id = None
        doctype_state = _DocumentTypeState(document_state, "html", public_id, system_id)
        document_state.append_child(doctype_state, document)
        html_state = document_state.create_element("html")
        document_state.append_child(html_state, document)
        head_state = document_state.create_element("head")
        html_state.append_child(head_state, document)
        if title is not None:
            title_state = document_state.create_element("title")
            head_state.append_child(title_state, document)
            text_state = document_state.create_text_node(title)
            title_state.append_child(text_state, document)
        body_state = document_state.create_element("body")
        html_state.append_child(body_state, document)
        return document

    def has_feature(self):
        return True



class _ChildNodes(NodeList):
    def __init__(self, parent_node, iter_cache):
        self._parent_node = parent_node
        self._iter_cache = iter_cache

    def __getitem__(self, index):
        if index < 0:
            index += self.get_length()
        node = self.item(index)
        if not node:
            raise IndexError
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

    def __getitem__(self, key):
        if type(key) != str:
            raise TypeError
        attr = self.get_named_item(key)
        if not attr:
            raise KeyError
        return attr

    def __iter__(self):
        return _AttributeKeyIter(self._element)

    def __len__(self):
        return len(self._element._state.attributes)

    def keys(self):
        return _AttributeKeys(self._element)

    def values(self):
        return _AttributeValues(self._element)

    def items(self):
        return _AttributeItems(self._element)

    def get_length(self):
        return len(self._element._state.attributes)

    def item(self, index):
        attributes = self._element._state.attributes
        if index < 0 or index >= len(attributes):
            return None
        return _wrap_node(attributes[index], self._element._document)

    def get_named_item(self, qualified_name):
        state = self._element._state.get_attribute_node(qualified_name)
        return _wrap_node(state, self._element._document)

    def get_named_item_ns(self, namespace, local_name):
        state = self._element._state.get_attribute_node_ns(namespace, local_name)
        return _wrap_node(state, self._element._document)

    def set_named_item(self, attr):
        return self.set_named_item_ns(attr)

    def set_named_item_ns(self, attr):
        state = self._element._state.set_attribute_node(_unwrap_typed_node(attr, Attr))
        return _wrap_node(state, self._element._document)

    def remove_named_item(self, qualified_name):
        state = self._element._state.remove_attribute(qualified_name)
        return _wrap_node(state, self._element._document)

    def remove_named_item_ns(self, namespace, local_name):
        state = self._element._state.remove_attribute_ns(namespace, local_name)
        return _wrap_node(state, self._element._document)


class _AttributeKeys:
    def __init__(self, element):
        self._element = element

    def __len__(self):
        return len(self._element._state.attributes)

    def __iter__(self):
        return _AttributeKeyIter(self._element)


class _AttributeValues:
    def __init__(self, element):
        self._element = element

    def __len__(self):
        return len(self._element._state.attributes)

    def __iter__(self):
        return _AttributeValueIter(self._element)


class _AttributeItems:
    def __init__(self, element):
        self._element = element

    def __len__(self):
        return len(self._element._state.attributes)

    def __iter__(self):
        return _AttributeItemIter(self._element)


class _AttributeKeyIter:
    def __init__(self, element):
        self._element = element
        self._index = -1

    def __next__(self):
        if self._index is not None:
            self._index += 1
            if self._index < len(self._element._state.attributes):
                state = self._element._state.attributes[self._index]
                return state.get_qualified_name()
            self._index = None
        raise StopIteration

    def __iter__(self):
        return self


class _AttributeValueIter:
    def __init__(self, element):
        self._element = element
        self._index = -1

    def __next__(self):
        if self._index is not None:
            self._index += 1
            if self._index < len(self._element._state.attributes):
                state = self._element._state.attributes[self._index]
                return _wrap_node(state, self._element._document)
            self._index = None
        raise StopIteration

    def __iter__(self):
        return self


class _AttributeItemIter(_AttributeValueIter):
    def __init__(self, element):
        self._element = element
        self._index = -1

    def __next__(self):
        if self._index is not None:
            self._index += 1
            if self._index < len(self._element._state.attributes):
                state = self._element._state.attributes[self._index]
                qualified_name = state.get_qualified_name()
                attr = _wrap_node(state, self._element._document)
                return qualified_name, attr
            self._index = None
        raise StopIteration

    def __iter__(self):
        return self



def _wrap_node(state, document):
    return state.wrap(document) if state else None

def _unwrap_node(node):
    return _unwrap_typed_node(node, Node)

def _unwrap_optional_node(node):
    if not node:
        return None
    return _unwrap_node(node)

def _unwrap_typed_node(node, type_):
    if not isinstance(node, type_):
        raise TypeError
    if not isinstance(node, _NodeImpl):
        raise TypeError
    return node._unwrap()


def _is_valid_naming(namespace, prefix, local_name):
    if namespace == "":
        return False
    if local_name is None:
        return False
    if prefix is None:
        if not local_name:
            return False
        if not _is_minimally_valid_name(local_name):
            return False
    else:
        if namespace is None:
            return False
        if not _is_valid_ncname(prefix):
            return False
        if not _is_valid_ncname(local_name):
            return False
        if prefix == "xml" and namespace != "http://www.w3.org/XML/1998/namespace":
            return False
        if prefix == "xmlns" and namespace != "http://www.w3.org/2000/xmlns/":
            return False
    return namespace != "http://www.w3.org/2000/xmlns/" or \
        prefix == "xmlns" or (prefix is None and local_name == "xmlns")


def _is_minimally_valid_name(name):
    if not name:
        return False
    for ch in name:
        if ch in "\0\t\n\f\r /":
            return False
    return True


# This function corresponds to the "validate and extract" operation as defined in the DOM
# standard (https://dom.spec.whatwg.org/#validate-and-extract)
def _parse_qualified_name(namespace, qualified_name):
    adjusted_namespace = namespace or None
    if not _is_valid_qualified_name(qualified_name):
        raise InvalidCharacterError
    prefix = None
    local_name = qualified_name
    i = qualified_name.find(":")
    if i >= 0:
        prefix = qualified_name[:i]
        local_name = qualified_name[i+1:]
    if not _is_valid_name_parts_combination(adjusted_namespace, prefix, local_name):
        raise NamespaceError
    return adjusted_namespace, prefix, local_name


def _is_valid_name_parts_combination(namespace, prefix, local_name):
    if prefix is not None and namespace is None:
        return False
    if prefix == "xml" and namespace != "http://www.w3.org/XML/1998/namespace":
        return False
    return ((prefix is None and local_name == "xmlns") or prefix == "xmlns") == \
        (namespace == "http://www.w3.org/2000/xmlns/")


def _is_valid_qualified_name(name):
    return _is_valid_general_name(name) and name.count(":") <= 1


def _is_valid_ncname(name):
    return _is_valid_general_name(name) and name.count(":") == 0


def _is_valid_general_name(name):
    if not name or not _is_valid_name_start_char(name[0]):
        return False
    for ch in name[1:]:
        if not _is_valid_name_char(ch):
            return False
    return True


def _is_valid_name_start_char(ch):
    val = ord(ch)
    if val < 0x80:
        return ch.isalpha() or ch == ":" or ch == "_"
    return ((val >= 0xC0    and val <= 0xD6)   or
            (val >= 0xD8    and val <= 0xF6)   or
            (val >= 0xF8    and val <= 0x2FF)  or
            (val >= 0x370   and val <= 0x37D)  or
            (val >= 0x37F   and val <= 0x1FFF) or
            (val >= 0x200C  and val <= 0x200D) or
            (val >= 0x2070  and val <= 0x218F) or
            (val >= 0x2C00  and val <= 0x2FEF) or
            (val >= 0x3001  and val <= 0xD7FF) or
            (val >= 0xF900  and val <= 0xFDCF) or
            (val >= 0xFDF0  and val <= 0xFFFD) or
            (val >= 0x10000 and val <= 0xEFFFF))

def _is_valid_name_char(ch):
    if _is_valid_name_start_char(ch):
        return True
    val = ord(ch)
    if val < 0x80:
        return ch.isdigit() or ch == "-" or ch == "."
    return (val == 0xB7 or
            (val >= 0x0300 and val <= 0x036F) or
            (val >= 0x203F and val <= 0x2040))



def _default_null_coercion(string):             
    return "null" if string is None else string

def _legacy_null_to_empty_string(string):                
    return "" if string is None else string


def _ascii_uppercase(string):
    return "".join([ch.upper() if ch.isascii() else ch for ch in string])

def _ascii_lowercase(string):
    return "".join([ch.lower() if ch.isascii() else ch for ch in string])
