import weakref
import archon.core


class Node:
    def __init__(self):
        self._weak_parent = None
        self._prev_sibling = None
        self._next_sibling = None

    def get_parent_node(self):
        return self._weak_parent and self._weak_parent()

    def has_child_nodes(self):
        return False

    def get_child_nodes(self):
        return _degen_node_list

    def get_first_child(self):
        return None

    def get_last_child(self):
        return None

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


class ParentNode(Node):
    def __init__(self):
        Node.__init__(self)
        self._child_nodes = _ChildNodes()

    def has_child_nodes(self):
        return bool(self._child_nodes._first)

    def get_child_nodes(self):
        return self._child_nodes

    def get_first_child(self):
        return self._child_nodes._first

    def get_last_child(self):
        return self._child_nodes._last

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
        if before and before.get_parent_node() != self:
            return False
        return True

    def _child_inserted(self, node):
        return


class Document(ParentNode):
    def __init__(self, content_type, is_html):
        ParentNode.__init__(self)
        self._content_type = content_type
        self._is_html = is_html
        self._doctype = None
        self._document_element = None

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
            local_name_2 = _ascii_lower(local_name_2)
        namespace = None
        if self._is_html or self._content_type == "application/xhtml+xml":
            namespace = "http://www.w3.org/1999/xhtml"
        prefix = None
        attributes = []
        return Element(namespace, prefix, local_name, attributes)

    def create_text_node(self, data):
        return Text(data)

    def create_comment(self, data):
        return Comment(data)

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


class Doctype(Node):
    def __init__(self, name, public_ident, system_ident):
        Node.__init__(self)
        self.name = name
        self.public_ident = public_ident
        self.system_ident = system_ident


class Attribute:
    def __init__(self, namespace, prefix, local_name, value):
        self.namespace = namespace
        self.prefix = prefix
        self.local_name = local_name
        self.value = value


class Element(ParentNode):
    def __init__(self, namespace, prefix, local_name, attributes):
        ParentNode.__init__(self)
        self.namespace = namespace
        self.prefix = prefix
        self.local_name = local_name
        self.attributes = attributes

    def _is_valid_child_insertion(self, node, before):
        if not ParentNode._is_valid_child_insertion(self, node, before):
            return False
        if not isinstance(node, Element) and not isinstance(node, CharacterData):
            return False
        return True


class CharacterData(Node):
    def __init__(self, data):
        Node.__init__(self)
        self.data = data


class Text(CharacterData):
    pass


class Comment(CharacterData):
    pass


class NodeList:
    def item(self, index):
        raise RuntimeError("Abstract method")

    def get_length(self):
        raise RuntimeError("Abstract method")


class DOMException(RuntimeError):
    pass


class HierarchyRequestError(DOMException):
    pass

class NotFoundError(DOMException):
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
    def format_namespace(namespace):
        if namespace is None:
            return "null"
        return archon.core.quote(namespace)
    def dump(string, level):
        print("%s%s" % (level * "  ", string))
    def visit(node, namespace, level):
        if isinstance(node, Document):
            string = "Document(content_type=%s)" % archon.core.quote(node.get_content_type())
            if not node.has_child_nodes():
                dump(string, level)
                return
            dump("%s:" % string, level)
            for child in node.get_child_nodes():
                visit(child, namespace, level + 1)
            return
        if isinstance(node, Doctype):
            dump("Doctype(%s, %s, %s)" % (format_nullable_string(node.name), format_nullable_string(node.public_ident),
                                          format_nullable_string(node.system_ident)), level)
            return
        if isinstance(node, Element):
            string = format_name(node.local_name)
            if node.prefix is not None:
                string = "%s:%s" % (format_name(node.prefix), string)
            assert node.namespace is not None
            if node.namespace != namespace:
                string = "%s[%s]" % (string, format_namespace(node.namespace))
            string = "Element(%s)" % string
            if not node.attributes and not node.has_child_nodes():
                dump(string, level)
                return
            dump("%s:" % string, level)
            for attr in node.attributes:
                string = format_name(attr.local_name)
                if attr.prefix is not None:
                    string = "%s:%s" % (format_name(attr.prefix), string)
                if attr.namespace is not None:
                    string = "%s[%s]" % (string, format_namespace(attr.namespace))
                dump("Attribute(%s=%s)" % (string, archon.core.clamped_quote(attr.value, max_string_size)), level + 1)
            for child in node.get_child_nodes():
                visit(child, node.namespace, level + 1)
            return
        if isinstance(node, Text):
            dump("Text(%s)" % archon.core.clamped_quote(node.data, max_string_size), level)
            return
        if isinstance(node, Comment):
            dump("Comment(%s)" % archon.core.clamped_quote(node.data, max_string_size), level)
            return
        assert False
    namespace = None
    level = 0
    visit(document, namespace, level)



class _DegenNodeList(NodeList):
    def __len__(self):
        return 0

    def __getitem__(self, index):
        raise IndexError()

    def __iter__(self):
        return iter([])

    def item(self, index):
        return None

    def get_length(self):
        return 0

_degen_node_list = _DegenNodeList()


class _ChildNodes(NodeList):
    def __init__(self):
        NodeList.__init__(self)
        self._first = None
        self._last = None
        self._size = 0
        self._generation = 0
        self._iter_cache = None

    def __len__(self):
        return self._size

    def __getitem__(self, index):
        node = self._get(index)
        if not node:
            raise IndexError()
        return node

    def __iter__(self):
        return _NodeIter(_ChildNodesIterCache(self))

    def item(self, index):
        return self._get(index)

    def get_length(self):
        return self._size

    def _get(self, index):
        if not self._iter_cache:
            self._iter_cache = _ChildNodesIterCache(self)
        return self._iter_cache.get_node(index)

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
        self._generation += 1

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
        self._generation += 1
        node._prev_sibling = None
        node._next_sibling = None


class _NodeIter:
    def __init__(self, iter_cache):
        self._iter_cache = iter_cache
        self._not_first = False
        self._at_end = False

    def __iter__(self):
        return self

    def __next__(self):
        if not self._at_end:
            i = self._iter_cache.get_index() + 1 if self._not_first else 0
            node = self._iter_cache.get_node(i)
            self._not_first = True
            if node:
                return node
            self._at_end = True
        raise StopIteration()


class _ChildNodesIterCache:
    def __init__(self, child_nodes):
        self._child_nodes = child_nodes
        self._generation = None
        self._index = 0
        self._node = None

    def get_node(self, i):
        self._ensure_cache()
        self._adjust(self._index, i)
        self._index = i
        return self._node

    def get_index(self):
        return self._index

    def _ensure_cache(self):
        if self._generation == self._child_nodes._generation:
            return
        self._node = self._child_nodes._first
        self._adjust(0, self._index)
        self._generation = self._child_nodes._generation

    def _adjust(self, i, j):
        size = self._child_nodes._size
        node = None
        if j >= 0 and j < size:
            node = self._node
            i_2 = i
            if j >= i:
                # Advance
                if i_2 < 0:
                    i_2 = 0
                    node = self._child_nodes._first
                for _ in range(j - i_2):
                    node = node._next_sibling
            else:
                # Recede
                if i_2 >= size:
                    i_2 = size - 1
                    node = self._child_nodes._last
                for _ in range(i_2 - j):
                    node = node._prev_sibling
        self._node = node


def _ascii_lower(text):
    return "".join([ch.lower() if ch.isascii() else ch for ch in text])
