import weakref
import archon.core


class Node:
    def __init__(self):
        self._weak_parent = None

    def get_parent_node(self):
        return self._weak_parent and self._weak_parent()

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


# FIXME: Hide `children`                 
class ParentNode(Node):
    def __init__(self):
        Node.__init__(self)
        self.children = []

    def insert_before(self, node, before):
        if not self._is_valid_child_insertion(node, before):
            raise HierarchyRequestError()
        parent = node.get_parent_node()
        if parent:
            parent.children.remove(node)
        index = self._get_before_index(before)
        self.children.insert(index, node)
        node._set_parent(self)
        self._child_inserted(node)

    def append_child(self, node):
        before = None
        self.insert_before(node, before)

    def _is_valid_child_insertion(self, node, before):
        # FIXME: This check must be based on the "host-including inclusive ancestor" instead of the "inclusive ancestor"        
        if node.contains(self):
            return False
        if before and before.get_parent_node() != self:
            return False
        return True

    def _child_inserted(self, node):
        return

    def _get_before_index(self, before):
        if before:
            index = self.children.index(before)
            assert index != -1
            return index
        return len(self.children)


class Document(ParentNode):
    def __init__(self):
        ParentNode.__init__(self)
        self._doctype = None
        self._document_element = None

    def _is_valid_child_insertion(self, node, before):
        if not ParentNode._is_valid_child_insertion(self, node, before):
            return False
        if not isinstance(node, Doctype) and not isinstance(node, Element) and not isinstance(node, Comment):
            return False
        if isinstance(node, Doctype):
            if self._doctype:
                return False
            index = self._get_before_index(before)
            if any(isinstance(node, Element) for node in self.children[:index]):
                return False
        elif isinstance(node, Element):
            if self._document_element:
                return False
            index = self._get_before_index(before)
            if any(isinstance(node, Doctype) for node in self.children[index:]):
                return False
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


class DOMException(RuntimeError):
    pass


class HierarchyRequestError(DOMException):
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
        need_quotation = False
        if not namespace:
            need_quotation = True
        else:
            for ch in namespace:
                if not ch.isalnum() and ch not in "-.:/":
                    need_quotation = True
                    break
        if need_quotation:
            return archon.core.quote(namespace)
        return namespace
    def dump(string, level):
        print("%s%s" % (level * "  ", string))
    def visit(node, namespace, level):
        if isinstance(node, Document):
            string = "Document"
            if not node.children:
                dump(string, level)
                return
            dump("%s:" % string, level)
            for child in node.children:
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
            if not node.attributes and not node.children:
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
            for child in node.children:
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
