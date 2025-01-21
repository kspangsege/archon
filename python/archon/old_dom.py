import weakref
import collections.abc
import archon.core


# FIXME: Implement get_node_type()    
# FIXME: Implement get_node_name()    
class Node:
    def __init__(self, owner_document):
        self._weak_owner_document = weakref.ref(owner_document) if owner_document else None

    def get_owner_document(self):
        self._weak_owner_document and self._weak_owner_document()

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

    # FIXME: Add insert_before() and friends                    


class ChildNode:
    def __init__(self):
        self._weak_parent = None
        self._prev_sibling = None
        self._next_sibling = None

    def get_parent_node(self):
        return self._weak_parent and self._weak_parent()

    def get_previous_sibling(self):
        return self._prev_sibling

    def get_next_sibling(self):
        return self._next_sibling

    def contains(self, other):
        node = other
        while node:
            if node == self:
                return True
            node = node.get_parent_node()

    def _set_parent(self, parent):
        if not parent:
            self._weak_parent = None
            return
        self._weak_parent = weakref.ref(parent)


class ParentNode:
    def __init__(self):
        self._child_nodes = _ChildNodes()

    def has_child_nodes(self):
        return bool(self._child_nodes._first)

    def get_child_nodes(self):
        return self._child_nodes

    def get_first_child(self):
        return self._child_nodes._first

    def get_last_child(self):
        return self._child_nodes._last

    # FIXME: Consider the case where an element created for one document is inserted into another. If it is to be allowed, the document association must be updated          
    # FIXME: Rename `before` -> `child`                       
    def insert_before(self, node, before):
        if not self._is_valid_child_insertion(node, before):
            raise HierarchyRequestError()
        parent = node.get_parent_node()
        if parent:
            parent._child_nodes._remove(node)
        self._child_nodes._insert(node, before)
        node._set_parent(self)
        self._child_inserted(node)

    def append_child(self, node):
        before = None
        self.insert_before(node, before)

    # FIXME: Add replace_child(node, child)                       

    def remove_child(self, node):
        parent = node.get_parent_node()
        if parent != self:
            raise NotFoundError
        self._child_nodes._remove(node)
        node._set_parent(None)
        return node

    def _is_valid_child_insertion(self, node, before):
        # FIXME: This check must be based on the "host-including inclusive ancestor" instead of the "inclusive ancestor"        
        if node.contains(self):
            return False
        # FIXME: Consider storing `_weak_self` in `ParentNode` and then replacing following comparison with `before._weak_parent_node != self._weak_self`       
        if before and before.get_parent_node() != self:
            return False
        return True

    def _child_inserted(self, node):
        return


class Document(ParentNode, Node):
    def __init__(self, content_type, is_html):
        owner_document = None
        Node.__init__(self, owner_document)
        ParentNode.__init__(self)
        self._content_type = content_type
        self._is_html = is_html
        self._doctype = None
        self._document_element = None

    # FIXME: Add get_implementation()      

    def get_content_type(self):
        return self._content_type

    def get_doctype(self):
        return self._doctype

    def get_document_element(self):
        return self._document_element

    # FIXME: Must also take optional `options` argument
    def create_element(self, local_name):
        # FIXME: If localName does not match the Name production, then throw an "InvalidCharacterError" DOMException.
        local_name_2 = local_name
        if self._is_html:
            local_name_2 = _ascii_lowercase(local_name_2)
        namespace_uri = None
        if self._is_html or self._content_type == "application/xhtml+xml":
            namespace_uri = "http://www.w3.org/1999/xhtml"
        prefix = None
        attributes = []
        return Element(self, namespace_uri, prefix, local_name, attributes)

    # FIXME: Add create_element_ns()              

    def create_text_node(self, data):
        return Text(self, data)

    def create_comment(self, data):
        return Comment(self, data)

    def _is_valid_child_insertion(self, node, before):
        if not ParentNode._is_valid_child_insertion(self, node, before):
            return False
        if not isinstance(node, Doctype) and not isinstance(node, Element) and not isinstance(node, Comment):
            return False
        if isinstance(node, Doctype):
            if self._doctype:
                return False
            child = self._child_nodes._first
            while child != before:
                if isinstance(child, Element):
                    return False
                child = child._next_sibling
        elif isinstance(node, Element):
            if self._document_element:
                return False
            if before:
                child = before
                while child:
                    if isinstance(child, Doctype):
                        return False
                    child = child._next_sibling
        return True

    def _child_inserted(self, node):
        if isinstance(node, Doctype):
            assert not self._doctype
            self._doctype = node
        if isinstance(node, Element):
            assert not self._document_element
            self._document_element = node


class Doctype(ChildNode, Node):
    def __init__(self, owner_document, name, public_id, system_id):
        Node.__init__(self, owner_document)
        ChildNode.__init__(self)
        self._name = name
        self._public_id = public_id
        self._system_id = system_id

    def get_name(self):
        return self._name

    def get_public_id(self):
        return self._public_id

    def get_system_id(self):
        return self._system_id


class Element(ParentNode, ChildNode, Node):
    def __init__(self, owner_document, namespace_uri, prefix, local_name, attributes):
        Node.__init__(self, owner_document)
        ChildNode.__init__(self)
        ParentNode.__init__(self)
        self._is_html = owner_document._is_html and namespace_uri == "http://www.w3.org/1999/xhtml"
        self._namespace_uri = namespace_uri
        self._prefix = prefix
        self._local_name = local_name
        self._attributes = _Attributes(weakref.ref(self), self._is_html, attributes)

    def get_namespace_uri(self):
        return self._namespace_uri

    def get_prefix(self):
        return self._prefix

    def get_local_name(self):
        return self._local_name

    def get_tag_name(self):
        qualified_name = self._get_qualified_name()
        if self._is_html:
            return _ascii_uppercase(qualified_name)
        return qualified_name

    # FIXME: Add attribute covering getters and setters: `get_id()`, `set_id()` cover `id` attribute; `get_class_name()`, `set_class_name()` cover `class` attribute, etc.    

    def has_attributes(self):
        return bool(self._attributes)

    def get_attributes(self):
        return self._attributes

    def _get_qualified_name(self):
        if self._prefix is None:
            return self._local_name
        return "%s:%s" % (self._prefix, self._local_name)

    def _is_valid_child_insertion(self, node, before):
        if not ParentNode._is_valid_child_insertion(self, node, before):
            return False
        if not isinstance(node, Element) and not isinstance(node, CharacterData):
            return False
        return True


class CharacterData(ChildNode, Node):
    def __init__(self, owner_document, data):
        Node.__init__(self, owner_document)
        ChildNode.__init__(self)
        self._data = _default_null_coercion(data) # FIXME: This is good for Text, but what about Comment and ProcessingInstruction?        

    def get_data(self):
        return self._data

    def set_data(self, data):
        self._data = _legacy_null_to_empty_string(data) # FIXME: This is good for Text, but what about Comment and ProcessingInstruction?        


class Text(CharacterData):
    pass


class Comment(CharacterData):
    pass


class Attr(Node):
    def __init__(self, owner_document, namespace_uri, prefix, local_name, value):
        Node.__init__(self, owner_document)
        assert value is not None
        self._namespace_uri = namespace_uri
        self._prefix = prefix
        self._local_name = local_name
        self._value = value
        self._weak_owner_element = None

    def get_namespace_uri(self):
        return self._namespace_uri

    def get_prefix(self):
        return self._prefix

    def get_local_name(self):
        return self._local_name

    def get_name(self):
        if self._prefix is None:
            return self._local_name
        return "%s:%s" % (self._prefix, self._local_name)

    def get_value(self):
        return self._value

    def set_value(self, value):
        self._value = _default_null_coercion(value)

    def get_owner_element(self):
        return self._weak_owner_element and self._weak_owner_element()

    def is_specified(self):
        return True


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

    def remove_named_item_ns(self, namespace, qualified_name):
        raise RuntimeError("Abstract method")


class DOMException(RuntimeError):
    pass


class HierarchyRequestError(DOMException):
    pass

class NotFoundError(DOMException):
    pass

class InUseAttributeError(DOMException):
    pass


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
        if isinstance(node, Doctype):
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



class _DegenNodeList(NodeList):
    def __getitem__(self, index):
        raise IndexError()

    def __len__(self):
        return 0

    def get_length(self):
        return 0

    def item(self, index):
        return None

_degen_node_list = _DegenNodeList()


class _ChildNodes(NodeList):
    def __init__(self):
        NodeList.__init__(self)
        self._first = None
        self._last = None
        self._size = 0
        self._iter_cache_valid = False
        self._iter_cache_index = 0
        self._iter_cache_node = None

    # FIXME: Add support for negative indexes                
    def __getitem__(self, index):
        node = self._get(index)
        if not node:
            raise IndexError()
        return node

    def __len__(self):
        return self._size

    def get_length(self):
        return self._size

    def item(self, index):
        return self._get(index)

    def _get(self, index):
        self._ensure_iter_cache()
        self._adjust_iter_cache(self._iter_cache_index, index)
        self._iter_cache_index = index
        return self._iter_cache_node

    def _insert(self, node, before):
        if before:
            after = before._prev_sibling
            if not after:
                node._next_sibling = before
                before._prev_sibling = node
                self._first = node
            else:
                after._next_sibling = node
                node._prev_sibling = after
                node._next_sibling = before
                before._prev_sibling = node
        else:
            after = self._last
            if not after:
                self._first = node
                self._last = node
            else:
                after._next_sibling = node
                node._prev_sibling = after
                self._last = node
        self._size += 1
        self._iter_cache_valid = False

    def _remove(self, node):
        predecessor = node._prev_sibling
        successor = node._next_sibling
        if predecessor:
            predecessor._next_sibling = successor
        else:
            self._first = successor
        if successor:
            successor._prev_sibling = predecessor
        else:
            self._last = predecessor
        self._size -= 1
        self._iter_cache_valid = False
        node._prev_sibling = None
        node._next_sibling = None

    def _ensure_iter_cache(self):
        if self._iter_cache_valid:
            return
        self._iter_cache_node = self._first
        self._adjust_iter_cache(0, self._iter_cache_index)
        self._iter_cache_valid = True

    def _adjust_iter_cache(self, i, j):
        size = self._size
        node = None
        if j >= 0 and j < size:
            node = self._iter_cache_node
            i_2 = i
            if j >= i:
                # Advance
                if i_2 < 0:
                    i_2 = 0
                    node = self._first
                for _ in range(j - i_2):
                    node = node._next_sibling
            else:
                # Recede
                if i_2 >= size:
                    i_2 = size - 1
                    node = self._last
                for _ in range(i_2 - j):
                    node = node._prev_sibling
        self._iter_cache_node = node


class _Attributes(NamedNodeMap):
    def __init__(self, weak_elem, is_html, attributes):
        NamedNodeMap.__init__(self)
        self._weak_elem = weak_elem
        self._is_html = is_html
        self._list = []
        self._map = {}
        for attr in attributes:
            assert isinstance(attr, Attr)
            assert not attr._weak_owner_element
            key = self._get_key_ns(attr._local_name)
            candidates = self._map.setdefault(key, [])
            self._append(candidates, attr)

    def __getitem__(self, qualified_name):
        attr = self.get_named_item(name)
        if not attr:
            raise KeyError
        return attr

    def __setitem__(self, qualified_name, value):
        # FIXME: If qualifiedName does not match the Name production in XML, then throw an "InvalidCharacterError" DOMException.   
        key, adjusted_qualified_name = self._get_key(qualified_name)
        candidates = self._map.setdefault(key, [])
        index = self._search(candidates, adjusted_qualified_name)
        if index is not None:
            attr = candidates[index]
            attr.set_value(value)
            return
        owner_document = self._weak_elem().get_owner_document() # FIXME: Could fail                                                                     
        namespace_uri = None
        prefix = None
        local_name = adjusted_qualified_name
        attr = Attr(owner_document, namespace_uri, prefix, local_name, value)
        self.append(attr)

    def __delitem__(self, qualified_name):
        self.remove_named_item(qualified_name)

    def __iter__(self):
        return iter(self._list)

    def __len__(self):
        return len(self._list)

    def get_length(self):
        return len(self._list)

    def item(self, index):
        if index < 0 or index >= len(self._list):
            return None
        return self._list[index]

    def get_named_item(self, qualified_name):
        candidates, index = self._find(qualified_name)
        return  None if index is None else candidates[index]

    def get_named_item_ns(self, namespace, local_name):
        candidates, index = self._find_ns(namespace, local_name)
        return  None if index is None else candidates[index]

    def set_named_item(self, attr):
        return self.set_named_item_ns(attr)

    def set_named_item_ns(self, attr):
        # FIXME: How to deal with attribute objects from different implementations (WrongDocumentError)? Presumably, it is an error if such an attribute object is passed!?!?                
        if attr._weak_owner_element and attr._weak_owner_element != self._weak_elem:
            raise InUseAttributeError()
        key = self._get_key_ns(attr._local_name)
        candidates = self._map.setdefault(key, [])
        index = self._search_ns(candidates, attr._namespace_uri, attr._local_name)
        if index is None:
            self._append(candidates, attr)
            return None
        old_attr = candidates[index]
        if old_attr != attr:
            self._replace(candidates, index, attr)
        return old_attr

    def remove_named_item(self, qualified_name):
        candidates, index = self._find(qualified_name)
        if index is None:
            return None
        return self._remove(candidates, index)

    def remove_named_item_ns(self, namespace, qualified_name):
        candidates, index = self._find_ns(namespace, qualified_name)
        if index is None:
            return None
        return self._remove(candidates, index)

    def _find(self, qualified_name):
        key, adjusted_qualified_name = self._get_key(qualified_name)
        candidates = self._map.get(key, [])
        index = self._search(candidates, adjusted_qualified_name)
        return candidates, index

    def _find_ns(self, namespace, local_name):
        key = self._get_key_ns(local_name)
        candidates = self._map.get(key, [])
        index = self._search_ns(candidates, namespace, local_name)
        return candidates, index

    def _get_key(self, qualified_name):
        adjusted_qualified_name = qualified_name
        if self._is_html:
            adjusted_qualified_name = _ascii_lowercase(adjusted_qualified_name)
        i = adjusted_qualified_name.find(":")
        key = adjusted_qualified_name if i < 0 else adjusted_qualified_name[i+1:]
        return key, adjusted_qualified_name

    def _get_key_ns(self, local_name):
        key = local_name
        if self._is_html:
            key = _ascii_lowercase(key)
        return key

    def _search(self, candidates, adjusted_qualified_name):
        for index, attr in enumerate(candidates):
            if attr.get_name() == adjusted_qualified_name:
                return candidates, index

    def _search_ns(self, candidates, namespace, local_name):
        adjusted_namespace = namespace or None
        for index, attr in enumerate(candidates):
            if attr.get_namespace_uri() == adjusted_namespace and attr.get_local_name() == local_name:
                return index
        return None

    def _append(self, candidates, attr):
        self._list.append(attr)
        candidates.append(attr)
        attr._weak_owner_element = self._weak_elem

    def _replace(self, candidates, index, attr):
        old_attr = candidates[index]
        for i, attr_2 in enumerate(self._list):
            if attr_2 == old_attr:
                self._list[i] = attr
                break
        candidates[index] = attr
        old_attr._weak_owner_element = None
        attr._weak_owner_element = self._weak_elem

    def _remove(self, candidates, index):
        attr = candidates[index]
        self._list.remove(attr)
        del candidates[index]
        attr._weak_owner_element = None
        return attr

    def _ensure_candidates_ns(self, local_name):
        key = local_name
        if self._is_html:
            key = _ascii_lowercase(key)
        return self._map.setdefault(key, [])


def _default_null_coercion(string):
    return "null" if string is None else string

def _legacy_null_to_empty_string(string):
    return "" if string is None else string


def _ascii_uppercase(string):
    return "".join([ch.upper() if ch.isascii() else ch for ch in string])

def _ascii_lowercase(string):
    return "".join([ch.lower() if ch.isascii() else ch for ch in string])
