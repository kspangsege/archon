import archon.test
import archon.dom


def _make_xml_document(content_type = None):
    return archon.dom.create_xml_document(content_type = content_type)

def _make_html_document(content_type = None):
    return archon.dom.create_html_document(content_type = content_type)


def test_DOMImplementation(context):
    doc = _make_xml_document()
    impl = doc.get_implementation()
    context.check_is_instance(impl, archon.dom.DOMImplementation)

    doctype = impl.create_document_type("a", "b", "c")
    context.check_is_instance(doctype, archon.dom.DocumentType)
    context.check_equal(doctype.get_name(), "a")
    context.check_equal(doctype.get_public_id(), "b")
    context.check_equal(doctype.get_system_id(), "c")
    context.check_equal(doctype.get_owner_document(), doc)

    doc_2 = impl.create_document("ns", "p:foo", doctype)
    context.check_is_instance(doc_2, archon.dom.XMLDocument)
    context.check_equal(doc_2.get_doctype(), doctype)
    # The document type node must have been migrated to the new document
    context.check_equal(doctype.get_owner_document(), doc_2)
    root = doc_2.get_document_element()
    context.check(root)
    context.check_equal(doc_2.get_document_element().get_namespace_uri(), "ns")
    context.check_equal(doc_2.get_document_element().get_prefix(), "p")
    context.check_equal(doc_2.get_document_element().get_local_name(), "foo")

    doc_3 = impl.create_html_document("hello")
    context.check_is_instance(doc_3, archon.dom.Document)
    context.check_not_is_instance(doc_3, archon.dom.XMLDocument)
    context.check_equal(doc_3.get_doctype().get_name(), "html")
    html = doc_3.get_document_element()
    context.check(html)
    context.check_equal(html.get_local_name(), "html")
    context.check_equal(len(html.get_child_nodes()), 2)
    head = html.get_child_nodes()[0]
    body = html.get_child_nodes()[1]
    context.check_equal(head.get_local_name(), "head")
    context.check_equal(body.get_local_name(), "body")
    context.check_equal(len(head.get_child_nodes()), 1)
    title = head.get_child_nodes()[0]
    context.check_equal(title.get_local_name(), "title")
    context.check_equal(len(title.get_child_nodes()), 1)
    text = title.get_child_nodes()[0]
    context.check_equal(text.get_data(), "hello")
    context.check_equal(len(body.get_child_nodes()), 0)

    context.check(impl.has_feature())


def test_Node_NodeName(context):
    doc = _make_xml_document()
    doctype = doc.get_implementation().create_document_type("a", "b", "c")
    elem_1 = doc.create_element("xFoo")
    elem_2 = doc.create_element_ns("ns1", "p:xFoo")
    text = doc.create_text_node("x")
    comment = doc.create_comment("x")
    attr_1 = doc.create_attribute("xBar")
    attr_2 = doc.create_attribute_ns("ns2", "p:xBar")
    context.check_equal(doctype.get_node_name(), "a")
    context.check_equal(doc.get_node_name(), "#document")
    context.check_equal(elem_1.get_node_name(), "xFoo")
    context.check_equal(elem_2.get_node_name(), "p:xFoo")
    context.check_equal(text.get_node_name(), "#text")
    context.check_equal(comment.get_node_name(), "#comment")
    context.check_equal(attr_1.get_node_name(), "xBar")
    context.check_equal(attr_2.get_node_name(), "p:xBar")

    doc = _make_html_document()
    elem_1 = doc.create_element("xFoo")
    elem_2 = doc.create_element_ns("ns1", "p:xFoo")
    attr_1 = doc.create_attribute("xBar")
    attr_2 = doc.create_attribute_ns("ns2", "p:xBar")
    context.check_equal(doc.get_node_name(), "#document")
    context.check_equal(elem_1.get_node_name(), "XFOO")
    context.check_equal(elem_2.get_node_name(), "p:xFoo")
    context.check_equal(attr_1.get_node_name(), "xbar")
    context.check_equal(attr_2.get_node_name(), "p:xBar")


def test_Node_AppendChild(context):
    doc = _make_xml_document()
    root = doc.create_element("root")
    context.check_not(root.has_child_nodes())
    context.check_equal(root.get_first_child(), None)
    context.check_equal(root.get_last_child(), None)
    context.check_equal(len(root.get_child_nodes()), 0)

    foo = doc.create_element("foo")
    root.append_child(foo)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), foo)
    context.check_equal(root.get_last_child(), foo)
    context.check_equal(len(root.get_child_nodes()), 1)
    context.check_equal(root.get_child_nodes()[0], foo)
    context.check_equal(foo.get_previous_sibling(), None)
    context.check_equal(foo.get_next_sibling(), None)

    bar = doc.create_element("bar")
    root.append_child(bar)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), foo)
    context.check_equal(root.get_last_child(), bar)
    context.check_equal(len(root.get_child_nodes()), 2)
    context.check_equal(root.get_child_nodes()[0], foo)
    context.check_equal(root.get_child_nodes()[1], bar)
    context.check_equal(foo.get_previous_sibling(), None)
    context.check_equal(foo.get_next_sibling(), bar)
    context.check_equal(bar.get_previous_sibling(), foo)
    context.check_equal(bar.get_next_sibling(), None)

    baz = doc.create_text_node("baz")
    root.append_child(baz)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), foo)
    context.check_equal(root.get_last_child(), baz)
    context.check_equal(len(root.get_child_nodes()), 3)
    context.check_equal(root.get_child_nodes()[0], foo)
    context.check_equal(root.get_child_nodes()[1], bar)
    context.check_equal(root.get_child_nodes()[2], baz)
    context.check_equal(foo.get_previous_sibling(), None)
    context.check_equal(foo.get_next_sibling(), bar)
    context.check_equal(bar.get_previous_sibling(), foo)
    context.check_equal(bar.get_next_sibling(), baz)
    context.check_equal(baz.get_previous_sibling(), bar)
    context.check_equal(baz.get_next_sibling(), None)


def test_Node_InsertBefore(context):
    doc = _make_xml_document()
    root = doc.create_element("root")

    foo = doc.create_element("foo")
    root.insert_before(foo, None)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), foo)
    context.check_equal(root.get_last_child(), foo)
    context.check_equal(len(root.get_child_nodes()), 1)
    context.check_equal(root.get_child_nodes()[0], foo)
    context.check_equal(foo.get_previous_sibling(), None)
    context.check_equal(foo.get_next_sibling(), None)

    bar = doc.create_element("bar")
    root.insert_before(bar, foo)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), bar)
    context.check_equal(root.get_last_child(), foo)
    context.check_equal(len(root.get_child_nodes()), 2)
    context.check_equal(root.get_child_nodes()[0], bar)
    context.check_equal(root.get_child_nodes()[1], foo)
    context.check_equal(foo.get_previous_sibling(), bar)
    context.check_equal(foo.get_next_sibling(), None)
    context.check_equal(bar.get_previous_sibling(), None)
    context.check_equal(bar.get_next_sibling(), foo)

    baz = doc.create_text_node("baz")
    root.insert_before(baz, foo)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), bar)
    context.check_equal(root.get_last_child(), foo)
    context.check_equal(len(root.get_child_nodes()), 3)
    context.check_equal(root.get_child_nodes()[0], bar)
    context.check_equal(root.get_child_nodes()[1], baz)
    context.check_equal(root.get_child_nodes()[2], foo)
    context.check_equal(foo.get_previous_sibling(), baz)
    context.check_equal(foo.get_next_sibling(), None)
    context.check_equal(bar.get_previous_sibling(), None)
    context.check_equal(bar.get_next_sibling(), baz)
    context.check_equal(baz.get_previous_sibling(), bar)
    context.check_equal(baz.get_next_sibling(), foo)


def test_Node_RemoveChild(context):
    doc = _make_xml_document()
    root = doc.create_element("root")
    foo = doc.create_element("foo")
    root.append_child(foo)
    bar = doc.create_element("bar")
    root.append_child(bar)
    baz = doc.create_text_node("baz")
    root.append_child(baz)

    root.remove_child(bar)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), foo)
    context.check_equal(root.get_last_child(), baz)
    context.check_equal(len(root.get_child_nodes()), 2)
    context.check_equal(root.get_child_nodes()[0], foo)
    context.check_equal(root.get_child_nodes()[1], baz)
    context.check_equal(foo.get_previous_sibling(), None)
    context.check_equal(foo.get_next_sibling(), baz)
    context.check_equal(bar.get_previous_sibling(), None)
    context.check_equal(bar.get_next_sibling(), None)
    context.check_equal(baz.get_previous_sibling(), foo)
    context.check_equal(baz.get_next_sibling(), None)

    root.insert_before(bar, baz)
    root.remove_child(baz)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), foo)
    context.check_equal(root.get_last_child(), bar)
    context.check_equal(len(root.get_child_nodes()), 2)
    context.check_equal(root.get_child_nodes()[0], foo)
    context.check_equal(root.get_child_nodes()[1], bar)
    context.check_equal(foo.get_previous_sibling(), None)
    context.check_equal(foo.get_next_sibling(), bar)
    context.check_equal(bar.get_previous_sibling(), foo)
    context.check_equal(bar.get_next_sibling(), None)
    context.check_equal(baz.get_previous_sibling(), None)
    context.check_equal(baz.get_next_sibling(), None)

    root.remove_child(foo)
    context.check(root.has_child_nodes())
    context.check_equal(root.get_first_child(), bar)
    context.check_equal(root.get_last_child(), bar)
    context.check_equal(len(root.get_child_nodes()), 1)
    context.check_equal(root.get_child_nodes()[0], bar)
    context.check_equal(foo.get_previous_sibling(), None)
    context.check_equal(foo.get_next_sibling(), None)
    context.check_equal(bar.get_previous_sibling(), None)
    context.check_equal(bar.get_next_sibling(), None)
    context.check_equal(baz.get_previous_sibling(), None)
    context.check_equal(baz.get_next_sibling(), None)

    root.remove_child(bar)
    context.check_not(root.has_child_nodes())
    context.check_equal(root.get_first_child(), None)
    context.check_equal(root.get_last_child(), None)
    context.check_equal(len(root.get_child_nodes()), 0)
    context.check_equal(foo.get_previous_sibling(), None)
    context.check_equal(foo.get_next_sibling(), None)
    context.check_equal(bar.get_previous_sibling(), None)
    context.check_equal(bar.get_next_sibling(), None)
    context.check_equal(baz.get_previous_sibling(), None)
    context.check_equal(baz.get_next_sibling(), None)


def test_Node_ChildNodesAsSequence(context):
    doc = _make_xml_document()
    root = doc.create_element("root")
    foo = doc.create_element("foo")
    root.append_child(foo)
    bar = doc.create_element("bar")
    root.append_child(bar)
    baz = doc.create_text_node("baz")
    root.append_child(baz)

    context.check_equal(list(root.get_child_nodes()), [foo, bar, baz])
    context.check_equal(list(reversed(root.get_child_nodes())), [baz, bar, foo])
    context.check_in(foo, root.get_child_nodes())
    context.check_not_in(root, root.get_child_nodes())


def test_Document_CreateElement(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()
    doc_3 = _make_xml_document(content_type = "application/xhtml+xml")
    def check(elem, namespace_uri, prefix, local_name):
        context.check_equal(elem.get_namespace_uri(), namespace_uri)
        context.check_equal(elem.get_prefix(), prefix)
        context.check_equal(elem.get_local_name(), local_name)
    check(doc_1.create_element("Foo"), None, None, "Foo")
    check(doc_1.create_element("p:Foo"), None, None, "p:Foo")
    check(doc_2.create_element("Foo"), "http://www.w3.org/1999/xhtml", None, "foo")
    check(doc_2.create_element("p:Foo"), "http://www.w3.org/1999/xhtml", None, "p:foo")
    check(doc_3.create_element("Foo"), "http://www.w3.org/1999/xhtml", None, "Foo")
    check(doc_3.create_element("p:Foo"), "http://www.w3.org/1999/xhtml", None, "p:Foo")


def test_Document_CreateElementNS(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()
    def check(elem, namespace_uri, prefix, local_name):
        context.check_equal(elem.get_namespace_uri(), namespace_uri)
        context.check_equal(elem.get_prefix(), prefix)
        context.check_equal(elem.get_local_name(), local_name)
    check(doc_1.create_element_ns("ns", "Foo"), "ns", None, "Foo")
    check(doc_1.create_element_ns("ns", "p:Foo"), "ns", "p", "Foo")
    check(doc_2.create_element_ns("ns", "Foo"), "ns", None, "Foo")
    check(doc_2.create_element_ns("ns", "p:Foo"), "ns", "p", "Foo")

    # These must not fail
    doc_1.create_element_ns("http://www.w3.org/XML/1998/namespace", "xml:foo")
    doc_1.create_element_ns("http://www.w3.org/XML/1998/namespace", "p:foo")
    doc_1.create_element_ns("http://www.w3.org/2000/xmlns/", "xmlns")
    doc_1.create_element_ns("http://www.w3.org/2000/xmlns/", "xmlns:foo")

    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_element_ns("ns", "xml:foo")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_element_ns("ns", "xmlns")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_element_ns("ns", "xmlns:foo")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_element_ns("http://www.w3.org/2000/xmlns/", "p")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_element_ns("http://www.w3.org/2000/xmlns/", "p:foo")


def test_Document_CreateAttribute(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()
    doc_3 = _make_xml_document(content_type = "application/xhtml+xml")
    def check(attr, namespace_uri, prefix, local_name):
        context.check_equal(attr.get_namespace_uri(), namespace_uri)
        context.check_equal(attr.get_prefix(), prefix)
        context.check_equal(attr.get_local_name(), local_name)
    check(doc_1.create_attribute("Foo"), None, None, "Foo")
    check(doc_1.create_attribute("p:Foo"), None, None, "p:Foo")
    check(doc_2.create_attribute("Foo"), None, None, "foo")
    check(doc_2.create_attribute("p:Foo"), None, None, "p:foo")
    check(doc_3.create_attribute("Foo"), None, None, "Foo")
    check(doc_3.create_attribute("p:Foo"), None, None, "p:Foo")


def test_Document_CreateAttributeNS(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()
    def check(attr, namespace_uri, prefix, local_name):
        context.check_equal(attr.get_namespace_uri(), namespace_uri)
        context.check_equal(attr.get_prefix(), prefix)
        context.check_equal(attr.get_local_name(), local_name)
    check(doc_1.create_attribute_ns("ns", "Foo"), "ns", None, "Foo")
    check(doc_1.create_attribute_ns("ns", "p:Foo"), "ns", "p", "Foo")
    check(doc_2.create_attribute_ns("ns", "Foo"), "ns", None, "Foo")
    check(doc_2.create_attribute_ns("ns", "p:Foo"), "ns", "p", "Foo")

    # These must not fail
    doc_1.create_attribute_ns("http://www.w3.org/XML/1998/namespace", "xml:foo")
    doc_1.create_attribute_ns("http://www.w3.org/XML/1998/namespace", "p:foo")
    doc_1.create_attribute_ns("http://www.w3.org/2000/xmlns/", "xmlns")
    doc_1.create_attribute_ns("http://www.w3.org/2000/xmlns/", "xmlns:foo")

    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_attribute_ns("ns", "xml:foo")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_attribute_ns("ns", "xmlns")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_attribute_ns("ns", "xmlns:foo")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_attribute_ns("http://www.w3.org/2000/xmlns/", "p")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.create_attribute_ns("http://www.w3.org/2000/xmlns/", "p:foo")


def test_Element_Basics(context):
    doc = _make_xml_document()
    elem = doc.create_element("elem")
    attributes_1 = elem.get_attributes()
    context.check_equal(len(attributes_1), 0)
    attributes_2 = list(attributes_1)
    context.check_equal(len(attributes_2), 0)

    elem.set_attribute("Foo", "1")
    attributes_1 = elem.get_attributes()
    context.check_equal(len(attributes_1), 1)
    context.check_equal(attributes_1[0].get_local_name(), "Foo")
    attributes_2 = list(attributes_1)
    context.check_equal(len(attributes_2), 1)
    context.check_equal(attributes_2[0], attributes_1[0])

    elem.set_attribute("Bar", "2")
    attributes_1 = elem.get_attributes()
    context.check_equal(len(attributes_1), 2)
    context.check_equal(attributes_1[0].get_local_name(), "Foo")
    context.check_equal(attributes_1[1].get_local_name(), "Bar")
    attributes_2 = list(attributes_1)
    context.check_equal(len(attributes_2), 2)
    context.check_equal(attributes_2[0], attributes_1[0])
    context.check_equal(attributes_2[1], attributes_1[1])


def test_Element_HasAttributes(context):
    doc = _make_xml_document()
    elem = doc.create_element("elem")
    context.check_not(elem.has_attributes())
    elem.set_attribute("foo", "1")
    context.check(elem.has_attributes())


def test_Element_SetAttributeNode(context):
    doc = _make_xml_document()
    elem = doc.create_element("elem")
    attributes = elem.get_attributes()
    attr_1 = doc.create_attribute("foo")
    attr = elem.set_attribute_node(attr_1)
    context.check_is_none(attr)
    context.check_equal(len(attributes), 1)
    context.check_equal(attributes[0], attr_1)

    attr_2 = doc.create_attribute("foo")
    attr = elem.set_attribute_node(attr_2)
    context.check_equal(attr, attr_1)
    context.check_equal(len(attributes), 1)
    context.check_equal(attributes[0], attr_2)

    attr_3 = doc.create_attribute("bar")
    attr = elem.set_attribute_node(attr_3)
    context.check_is_none(attr)
    context.check_equal(len(attributes), 2)
    context.check_equal(attributes[0], attr_2)
    context.check_equal(attributes[1], attr_3)


def test_Element_RemoveAttributeNode(context):
    doc = _make_xml_document()
    elem = doc.create_element("elem")
    attr = doc.create_attribute("foo")
    with context.check_raises(archon.dom.NotFoundError):
        elem.remove_attribute_node(attr)
    elem.set_attribute_node(attr)
    attr_2 = elem.remove_attribute_node(attr)
    context.check_equal(attr_2, attr)
    with context.check_raises(archon.dom.NotFoundError):
        elem.remove_attribute_node(attr)


# Bridge to Python's native testing framework
def load_tests(loader, standard_tests, pattern):
    return archon.test.generate_native_tests(__name__)


if __name__ == '__main__':
    archon.test.run_module_tests(__name__)
