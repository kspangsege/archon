import weakref
import archon.core


class Node:
    def __init__(self):
        self._weak_parent = None

    def has_parent(self):
        return bool(self._weak_parent)

    def get_parent_node(self):
        return self._weak_parent()

class ParentNode(Node):
    def __init__(self):
        Node.__init__(self)
        self.children = []
    def append_child(self, node):
        if node._weak_parent:
            parent = node._weak_parent()
            if parent:
                parent.children.remove(node)
        self.children.append(node)
        node._weak_parent = weakref.ref(self)

class Document(ParentNode):
    pass

class Doctype(Node):
    def __init__(self, decl):
        Node.__init__(self)
        self.decl = decl

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

class Text(Node):
    def __init__(self, data):
        Node.__init__(self)
        self.data = data

class Comment(Node):
    def __init__(self, data):
        Node.__init__(self)
        self.data = data



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
            dump("Doctype(%s)" % archon.core.quote(node.decl), level)
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
