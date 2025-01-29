import archon.test
import archon.dom


def _make_xml_document(content_type = None):
    return archon.dom.create_xml_document(content_type = content_type)

def _make_html_document(content_type = None):
    return archon.dom.create_html_document(content_type = content_type)


def check_elem(context, elem, namespace_uri, prefix, local_name):
    context.check_equal(elem.get_namespace_uri(), namespace_uri)
    context.check_equal(elem.get_prefix(), prefix)
    context.check_equal(elem.get_local_name(), local_name)

def check_attr(context, attr, namespace_uri, prefix, local_name, value):
    context.check_equal(attr.get_namespace_uri(), namespace_uri)
    context.check_equal(attr.get_prefix(), prefix)
    context.check_equal(attr.get_local_name(), local_name)
    context.check_equal(attr.get_value(), value)



def test_CreateXMLDocument(context):
    doc = archon.dom.create_xml_document()
    context.check_equal(doc.get_content_type(), "application/xml")
    doc = archon.dom.create_xml_document(content_type = None)
    context.check_equal(doc.get_content_type(), "application/xml")
    doc = archon.dom.create_xml_document(content_type = "foo")
    context.check_equal(doc.get_content_type(), "foo")
    with context.check_raises(TypeError):
        archon.dom.create_xml_document(content_type = 7)


def test_CreateHTMLDocument(context):
    doc = archon.dom.create_html_document()
    context.check_equal(doc.get_content_type(), "text/html")
    doc = archon.dom.create_html_document(content_type = None)
    context.check_equal(doc.get_content_type(), "text/html")
    doc = archon.dom.create_html_document(content_type = "foo")
    context.check_equal(doc.get_content_type(), "foo")
    with context.check_raises(TypeError):
        archon.dom.create_html_document(content_type = 7)


def test_CreateDocumentType(context):
    def check(doc, is_html):
        doctype = archon.dom.create_document_type(doc, "Foo", "Bar", "Baz")
        context.check_equal(doctype.get_owner_document(), doc)

        doctype = archon.dom.create_document_type(doc, "Foo", "Bar", "Baz")
        context.check_equal(doctype.get_name(), "Foo")
        context.check_equal(doctype.get_public_id(), "Bar")
        context.check_equal(doctype.get_system_id(), "Baz")

        doctype = archon.dom.create_document_type(doc, "", "", "")
        context.check_equal(doctype.get_name(), "")
        context.check_equal(doctype.get_public_id(), "")
        context.check_equal(doctype.get_system_id(), "")

        bad_doc = doc.create_text_node("foo")
        with context.check_raises(TypeError):
            archon.dom.create_document_type(None, "Foo", "Bar", "Baz")
        with context.check_raises(TypeError):
            archon.dom.create_document_type(bad_doc, "Foo", "Bar", "Baz")

        with context.check_raises(TypeError):
            archon.dom.create_document_type(doc, 7, "Bar", "Baz")
        with context.check_raises(TypeError):
            archon.dom.create_document_type(doc, None, "Bar", "Baz")
        with context.check_raises(TypeError):
            archon.dom.create_document_type(doc, "Foo", 7, "Baz")
        with context.check_raises(TypeError):
            archon.dom.create_document_type(doc, "Foo", None, "Baz")
        with context.check_raises(TypeError):
            archon.dom.create_document_type(doc, "Foo", "Bar", 7)
        with context.check_raises(TypeError):
            archon.dom.create_document_type(doc, "Foo", "Bar", None)

        with context.check_raises(ValueError):
            archon.dom.create_document_type(doc, "x x", "Bar", "Baz")
        with context.check_raises(ValueError):
            archon.dom.create_document_type(doc, "x\0x", "Bar", "Baz")

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_CreateAttribute(context):
    def check(doc, is_html):
        attr = archon.dom.create_attribute(doc, "ns", "p", "Foo", "")
        context.check_equal(attr.get_owner_document(), doc)

        attr = archon.dom.create_attribute(doc, "ns", "p", "Foo", "")
        context.check_equal(attr.get_namespace_uri(), "ns")
        context.check_equal(attr.get_prefix(), "p")
        context.check_equal(attr.get_local_name(), "Foo")

        attr = archon.dom.create_attribute(doc, "ns", None, "Foo", "")
        context.check_equal(attr.get_namespace_uri(), "ns")
        context.check_equal(attr.get_prefix(), None)
        context.check_equal(attr.get_local_name(), "Foo")

        attr = archon.dom.create_attribute(doc, None, None, "Foo", "")
        context.check_equal(attr.get_namespace_uri(), None)
        context.check_equal(attr.get_prefix(), None)
        context.check_equal(attr.get_local_name(), "Foo")

        attr = archon.dom.create_attribute(doc, "ns", None, "@:p:Foo", "")
        context.check_equal(attr.get_namespace_uri(), "ns")
        context.check_equal(attr.get_prefix(), None)
        context.check_equal(attr.get_local_name(), "@:p:Foo")

        attr = archon.dom.create_attribute(doc, None, None, "@:p:Foo", "")
        context.check_equal(attr.get_namespace_uri(), None)
        context.check_equal(attr.get_prefix(), None)
        context.check_equal(attr.get_local_name(), "@:p:Foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/XML/1998/namespace", "xml", "foo", "")
        context.check_equal(attr.get_namespace_uri(), "http://www.w3.org/XML/1998/namespace")
        context.check_equal(attr.get_prefix(), "xml")
        context.check_equal(attr.get_local_name(), "foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", "xmlns", "foo", "")
        context.check_equal(attr.get_namespace_uri(), "http://www.w3.org/2000/xmlns/")
        context.check_equal(attr.get_prefix(), "xmlns")
        context.check_equal(attr.get_local_name(), "foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", None, "xmlns", "")
        context.check_equal(attr.get_namespace_uri(), "http://www.w3.org/2000/xmlns/")
        context.check_equal(attr.get_prefix(), None)
        context.check_equal(attr.get_local_name(), "xmlns")

        bad_doc = doc.create_text_node("foo")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(None, "ns", "p", "Foo", "")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(bad_doc, "ns", "p", "Foo", "")

        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, 7, "p", "Foo", "")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", 7, "Foo", "")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", "p", 7, "")

        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, None, None, None, "")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", None, None, "")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", "p", None, "")

        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, None, None, "", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, None, None, "x x", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, None, None, "x\0x", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, None, None, "x/x", "")

        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", None, "", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", None, "x x", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", None, "x\0x", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", None, "x/x", "")

        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "p", "", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "p", "x x", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "p", "x\0x", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "p", "x/x", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "p", "x:x", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "p", "x@x", "")

        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "", "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "x x", "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "x\0x", "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "x/x", "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "x:x", "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "x@x", "Foo", "")

        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "", None, "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "", "p", "Foo", "")

        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, None, "p", "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "xml", "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "ns", "xmlns", "Foo", "")

        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", None, "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", "p", "Foo", "")
        with context.check_raises(ValueError):
            archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", "p", "xmlns", "")

        attr = archon.dom.create_attribute(doc, "ns", "p", "foo", "")
        context.check_equal(attr.get_value(), "")
        attr = archon.dom.create_attribute(doc, "ns", "p", "foo", "1")
        context.check_equal(attr.get_value(), "1")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", "p", "foo", None)
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", "p", "foo", 7)

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_CreateElement(context):
    def check(doc, is_html):
        elem = archon.dom.create_element(doc, "ns", "p", "Foo", [])
        context.check_equal(elem.get_owner_document(), doc)

        elem = archon.dom.create_element(doc, "ns", "p", "Foo", [])
        context.check_equal(elem.get_namespace_uri(), "ns")
        context.check_equal(elem.get_prefix(), "p")
        context.check_equal(elem.get_local_name(), "Foo")

        elem = archon.dom.create_element(doc, "ns", None, "Foo", [])
        context.check_equal(elem.get_namespace_uri(), "ns")
        context.check_equal(elem.get_prefix(), None)
        context.check_equal(elem.get_local_name(), "Foo")

        elem = archon.dom.create_element(doc, None, None, "Foo", [])
        context.check_equal(elem.get_namespace_uri(), None)
        context.check_equal(elem.get_prefix(), None)
        context.check_equal(elem.get_local_name(), "Foo")

        elem = archon.dom.create_element(doc, "ns", None, "@:p:Foo", [])
        context.check_equal(elem.get_namespace_uri(), "ns")
        context.check_equal(elem.get_prefix(), None)
        context.check_equal(elem.get_local_name(), "@:p:Foo")

        elem = archon.dom.create_element(doc, None, None, "@:p:Foo", [])
        context.check_equal(elem.get_namespace_uri(), None)
        context.check_equal(elem.get_prefix(), None)
        context.check_equal(elem.get_local_name(), "@:p:Foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/XML/1998/namespace", "xml", "foo", [])
        context.check_equal(elem.get_namespace_uri(), "http://www.w3.org/XML/1998/namespace")
        context.check_equal(elem.get_prefix(), "xml")
        context.check_equal(elem.get_local_name(), "foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", "xmlns", "foo", [])
        context.check_equal(elem.get_namespace_uri(), "http://www.w3.org/2000/xmlns/")
        context.check_equal(elem.get_prefix(), "xmlns")
        context.check_equal(elem.get_local_name(), "foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", None, "xmlns", [])
        context.check_equal(elem.get_namespace_uri(), "http://www.w3.org/2000/xmlns/")
        context.check_equal(elem.get_prefix(), None)
        context.check_equal(elem.get_local_name(), "xmlns")

        bad_doc = doc.create_text_node("foo")
        with context.check_raises(TypeError):
            archon.dom.create_element(None, "ns", "p", "Foo", [])
        with context.check_raises(TypeError):
            archon.dom.create_element(bad_doc, "ns", "p", "Foo", [])

        with context.check_raises(TypeError):
            archon.dom.create_element(doc, 7, "p", "Foo", [])
        with context.check_raises(TypeError):
            archon.dom.create_element(doc, "ns", 7, "Foo", [])
        with context.check_raises(TypeError):
            archon.dom.create_element(doc, "ns", "p", 7, [])

        with context.check_raises(TypeError):
            archon.dom.create_element(doc, None, None, None, [])
        with context.check_raises(TypeError):
            archon.dom.create_element(doc, "ns", None, None, [])
        with context.check_raises(TypeError):
            archon.dom.create_element(doc, "ns", "p", None, [])

        with context.check_raises(ValueError):
            archon.dom.create_element(doc, None, None, "", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, None, None, "x x", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, None, None, "x\0x", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, None, None, "x/x", [])

        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", None, "", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", None, "x x", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", None, "x\0x", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", None, "x/x", [])

        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "x x", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "x\0x", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "x/x", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "x:x", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "x@x", [])

        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "", "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "x x", "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "x\0x", "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "x/x", "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "x:x", "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "x@x", "Foo", [])

        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "", None, "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "", "p", "Foo", [])

        with context.check_raises(ValueError):
            archon.dom.create_element(doc, None, "p", "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "xml", "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "xmlns", "Foo", [])

        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", None, "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", "p", "Foo", [])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", "p", "xmlns", [])

        attr_1 = archon.dom.create_attribute(doc, "ns", "p", "foo", "")
        attr_2 = archon.dom.create_attribute(doc, "ns", None, "bar", "")
        attr_3 = archon.dom.create_attribute(doc, None, None, "baz", "")
        elem = archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, attr_2, attr_3 ])
        attributes = elem.get_attributes()
        context.check_equal(len(attributes), 3)
        context.check_equal(attributes.item(0), attr_1)
        context.check_equal(attributes.item(1), attr_2)
        context.check_equal(attributes.item(2), attr_3)

        attr_1 = archon.dom.create_attribute(doc, "ns", "p", "foo", "")
        attr_2 = archon.dom.create_attribute(doc, "ns", None, "bar", "")
        attr_3 = archon.dom.create_attribute(doc, None, None, "baz", "")
        attr_4 = archon.dom.create_attribute(doc, "ns", None, "bar", "")
        attr_5 = archon.dom.create_attribute(doc, None, None, "baz", "")
        attr_6 = archon.dom.create_attribute(doc, "ns", None, "baz", "")
        elem = archon.dom.create_element(doc, "ns", "p", "foo", [ attr_6 ])
        bad_attr = doc.create_text_node("foo")
        with context.check_raises(TypeError):
            archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, None, attr_3 ])
        with context.check_raises(TypeError):
            archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, bad_attr, attr_3 ])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, attr_1, attr_3 ])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "foo", [ attr_4, attr_2, attr_3 ])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "foo", [ attr_5, attr_2, attr_3 ])
        with context.check_raises(ValueError):
            archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, attr_6, attr_3 ])

        # FIXME: Reneable once detection of document mismatch has been implemented                                            
        # def subcheck(doc_2, is_html_2):
        #     attr_7 = archon.dom.create_attribute(doc_2, "ns", None, "bar", "")
        #     attr_8 = archon.dom.create_attribute(doc_2, "ns", None, "bar", "")
        #     elem_2 = archon.dom.create_element(doc_2, "ns", "p", "foo", [ attr_8 ])
        #     with context.check_raises(ValueError):
        #         archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, attr_7, attr_3 ])
        #     with context.check_raises(ValueError):
        #         archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, attr_8, attr_3 ])

        # subcheck(_make_xml_document(), False)
        # subcheck(_make_html_document(), True)

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


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

    with context.check_raises(TypeError):
        impl.create_document_type(7, "b", "c")
    with context.check_raises(TypeError):
        impl.create_document_type(None, "b", "c")
    with context.check_raises(TypeError):
        impl.create_document_type("a", 7, "c")
    with context.check_raises(TypeError):
        impl.create_document_type("a", None, "c")
    with context.check_raises(TypeError):
        impl.create_document_type("a", "b", 7)
    with context.check_raises(TypeError):
        impl.create_document_type("a", "b", None)

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


def test_Node_ChildNodeBasics(context):
    doc = _make_xml_document()
    parent = doc.create_element("parent")
    child_nodes = parent.get_child_nodes()
    context.check_equal(parent.get_first_child(), None)
    context.check_equal(parent.get_last_child(), None)
    context.check_equal(child_nodes.get_length(), 0)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), None)

    child_1 = doc.create_text_node("1")
    parent.append_child(child_1)
    context.check_equal(parent.get_first_child(), child_1)
    context.check_equal(parent.get_last_child(), child_1)
    context.check_equal(child_nodes.get_length(), 1)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), None)

    child_2 = doc.create_text_node("2")
    parent.append_child(child_2)
    context.check_equal(parent.get_first_child(), child_1)
    context.check_equal(parent.get_last_child(), child_2)
    context.check_equal(child_nodes.get_length(), 2)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_2)
    context.check_equal(child_nodes.item(2), None)

    child_3 = doc.create_text_node("3")
    parent.append_child(child_3)
    context.check_equal(parent.get_first_child(), child_1)
    context.check_equal(parent.get_last_child(), child_3)
    context.check_equal(child_nodes.get_length(), 3)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_2)
    context.check_equal(child_nodes.item(2), child_3)
    context.check_equal(child_nodes.item(3), None)

    parent.remove_child(child_2)
    context.check_equal(parent.get_first_child(), child_1)
    context.check_equal(parent.get_last_child(), child_3)
    context.check_equal(child_nodes.get_length(), 2)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_3)
    context.check_equal(child_nodes.item(2), None)


def test_Node_ParentNode(context):
    doc = _make_xml_document()
    context.check_equal(doc.get_parent_node(), None)
    elem = doc.create_element("elem")
    attr = doc.create_attribute("attr")
    context.check_equal(attr.get_parent_node(), None)
    elem.set_attribute_node(attr)
    context.check_equal(attr.get_parent_node(), None)

    impl = doc.get_implementation()
    doctype = impl.create_document_type("foo", "bar", "baz")
    context.check_equal(doctype.get_parent_node(), None)
    doc.append_child(doctype)
    context.check_equal(doctype.get_parent_node(), doc)
    doc.remove_child(doctype)
    context.check_equal(doctype.get_parent_node(), None)

    def check(child):
        context.check_equal(child.get_parent_node(), None)
        elem.append_child(child)
        context.check_equal(child.get_parent_node(), elem)
        elem.remove_child(child)
        context.check_equal(child.get_parent_node(), None)

    check(doc.create_element("foo"))
    check(doc.create_text_node("foo"))
    check(doc.create_comment("foo"))


def test_Node_HasChildNodes(context):
    doc = _make_xml_document()
    context.check_not(doc.has_child_nodes())
    elem = doc.create_element("elem")
    context.check_not(elem.has_child_nodes())
    doc.append_child(elem)
    context.check(doc.has_child_nodes())
    context.check_not(elem.has_child_nodes())
    elem.append_child(doc.create_text_node("text"))
    context.check(elem.has_child_nodes())

    impl = doc.get_implementation()
    doctype = impl.create_document_type("foo", "bar", "baz")
    context.check_not(doctype.has_child_nodes())
    text = doc.create_text_node("text")
    context.check_not(text.has_child_nodes())
    comment = doc.create_comment("comment")
    context.check_not(comment.has_child_nodes())


def test_Node_PreviousNextSibling(context):
    doc = _make_xml_document()
    context.check_equal(doc.get_previous_sibling(), None)
    context.check_equal(doc.get_next_sibling(), None)
    elem = doc.create_element("elem")
    attr_1 = doc.create_attribute("attr_1")
    attr_2 = doc.create_attribute("attr_2")
    context.check_equal(attr_1.get_previous_sibling(), None)
    context.check_equal(attr_1.get_next_sibling(), None)
    context.check_equal(attr_2.get_previous_sibling(), None)
    context.check_equal(attr_2.get_next_sibling(), None)
    elem.set_attribute_node(attr_1)
    elem.set_attribute_node(attr_2)
    context.check_equal(attr_1.get_previous_sibling(), None)
    context.check_equal(attr_1.get_next_sibling(), None)
    context.check_equal(attr_2.get_previous_sibling(), None)
    context.check_equal(attr_2.get_next_sibling(), None)

    comment_1 = doc.create_comment("comment 1")
    comment_2 = doc.create_comment("comment 2")

    impl = doc.get_implementation()
    doctype = impl.create_document_type("foo", "bar", "baz")
    context.check_equal(doctype.get_previous_sibling(), None)
    context.check_equal(doctype.get_next_sibling(), None)
    doc.append_child(doctype)
    context.check_equal(doctype.get_previous_sibling(), None)
    context.check_equal(doctype.get_next_sibling(), None)
    doc.remove_child(doctype)
    doc.append_child(comment_1)
    doc.append_child(doctype)
    context.check_equal(doctype.get_previous_sibling(), comment_1)
    context.check_equal(doctype.get_next_sibling(), None)
    doc.append_child(comment_2)
    context.check_equal(doctype.get_previous_sibling(), comment_1)
    context.check_equal(doctype.get_next_sibling(), comment_2)
    doc.remove_child(comment_1)
    context.check_equal(doctype.get_previous_sibling(), None)
    context.check_equal(doctype.get_next_sibling(), comment_2)
    doc.remove_child(doctype)
    context.check_equal(doctype.get_previous_sibling(), None)
    context.check_equal(doctype.get_next_sibling(), None)
    doc.remove_child(comment_2)

    def check(child):
        context.check_equal(child.get_previous_sibling(), None)
        context.check_equal(child.get_next_sibling(), None)
        elem.append_child(child)
        context.check_equal(child.get_previous_sibling(), None)
        context.check_equal(child.get_next_sibling(), None)
        elem.remove_child(child)
        elem.append_child(comment_1)
        elem.append_child(child)
        context.check_equal(child.get_previous_sibling(), comment_1)
        context.check_equal(child.get_next_sibling(), None)
        elem.append_child(comment_2)
        context.check_equal(child.get_previous_sibling(), comment_1)
        context.check_equal(child.get_next_sibling(), comment_2)
        elem.remove_child(comment_1)
        context.check_equal(child.get_previous_sibling(), None)
        context.check_equal(child.get_next_sibling(), comment_2)
        elem.remove_child(child)
        context.check_equal(child.get_previous_sibling(), None)
        context.check_equal(child.get_next_sibling(), None)
        elem.remove_child(comment_2)

    check(doc.create_element("foo"))
    check(doc.create_text_node("foo"))
    check(doc.create_comment("foo"))


def test_Node_AppendChild(context):
    doc = _make_xml_document()
    root = doc.create_element("root")

    def check(grandparent, parent, is_connected):
        child_nodes = parent.get_child_nodes()
        context.check_equal(parent.get_first_child(), None)
        context.check_equal(parent.get_last_child(), None)
        context.check_equal(child_nodes.get_length(), 0)

        child_1 = doc.create_element("elem")
        parent.append_child(child_1)
        context.check_equal(parent.get_first_child(), child_1)
        context.check_equal(parent.get_last_child(), child_1)
        context.check_equal(child_nodes.get_length(), 1)
        context.check_equal(child_nodes.item(0), child_1)
        context.check_equal(child_1.get_parent_node(), parent)
        context.check_equal(child_1.get_previous_sibling(), None)
        context.check_equal(child_1.get_next_sibling(), None)

        child_2 = doc.create_text_node("text")
        parent.append_child(child_2)
        context.check_equal(parent.get_first_child(), child_1)
        context.check_equal(parent.get_last_child(), child_2)
        context.check_equal(child_nodes.get_length(), 2)
        context.check_equal(child_nodes.item(0), child_1)
        context.check_equal(child_nodes.item(1), child_2)
        context.check_equal(child_1.get_parent_node(), parent)
        context.check_equal(child_1.get_previous_sibling(), None)
        context.check_equal(child_1.get_next_sibling(), child_2)
        context.check_equal(child_2.get_parent_node(), parent)
        context.check_equal(child_2.get_previous_sibling(), child_1)
        context.check_equal(child_2.get_next_sibling(), None)

        child_3 = doc.create_comment("comment")
        parent.append_child(child_3)
        context.check_equal(parent.get_first_child(), child_1)
        context.check_equal(parent.get_last_child(), child_3)
        context.check_equal(child_nodes.get_length(), 3)
        context.check_equal(child_nodes.item(0), child_1)
        context.check_equal(child_nodes.item(1), child_2)
        context.check_equal(child_nodes.item(2), child_3)
        context.check_equal(child_1.get_parent_node(), parent)
        context.check_equal(child_1.get_previous_sibling(), None)
        context.check_equal(child_1.get_next_sibling(), child_2)
        context.check_equal(child_2.get_parent_node(), parent)
        context.check_equal(child_2.get_previous_sibling(), child_1)
        context.check_equal(child_2.get_next_sibling(), child_3)
        context.check_equal(child_3.get_parent_node(), parent)
        context.check_equal(child_3.get_previous_sibling(), child_2)
        context.check_equal(child_3.get_next_sibling(), None)

        parent.remove_child(child_3)
        parent.remove_child(child_2)

        def subcheck(child, subchild):
            parent.append_child(child)
            context.check_equal(parent.get_first_child(), child_1)
            context.check_equal(parent.get_last_child(), child)
            context.check_equal(child_nodes.get_length(), 2)
            context.check_equal(child_nodes.item(0), child_1)
            context.check_equal(child_nodes.item(1), child)
            context.check_equal(child_1.get_parent_node(), parent)
            context.check_equal(child_1.get_previous_sibling(), None)
            context.check_equal(child_1.get_next_sibling(), child)
            context.check_equal(child.get_parent_node(), parent)
            context.check_equal(child.get_previous_sibling(), child_1)
            context.check_equal(child.get_next_sibling(), None)
            context.check_equal(child.get_owner_document(), doc)
            if subchild:
                context.check_equal(subchild.get_parent_node(), child)
                context.check_equal(subchild.get_owner_document(), doc)

        child = doc.create_text_node("text")
        subcheck(child, None)
        parent.remove_child(child)
        child = doc.create_element("elem")
        subchild = doc.create_text_node("text")
        child.append_child(subchild)
        subcheck(child, subchild)
        parent.remove_child(child)
        child = doc.create_text_node("text")
        root.append_child(child)
        subcheck(child, None)
        parent.remove_child(child)
        child = doc.create_element("elem")
        subchild = doc.create_text_node("text")
        child.append_child(subchild)
        root.append_child(child)
        subcheck(child, subchild)
        parent.remove_child(child)

        child = doc.create_text_node("text")
        parent.append_child(child)
        subcheck(child, None)
        parent.remove_child(child)
        child = doc.create_element("elem")
        subchild = doc.create_text_node("text")
        child.append_child(subchild)
        parent.append_child(child)
        subcheck(child, subchild)
        parent.remove_child(child)

        doc_2 = _make_xml_document()
        root_2 = doc.create_element("root")
        child = doc_2.create_text_node("text")
        subcheck(child, None)
        parent.remove_child(child)
        child = doc_2.create_element("elem")
        subchild = doc_2.create_text_node("text")
        child.append_child(subchild)
        subcheck(child, subchild)
        parent.remove_child(child)
        child = doc_2.create_text_node("text")
        root_2.append_child(child)
        subcheck(child, None)
        parent.remove_child(child)
        child = doc_2.create_element("elem")
        subchild = doc_2.create_text_node("text")
        child.append_child(subchild)
        root_2.append_child(child)
        subcheck(child, subchild)
        parent.remove_child(child)

        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.append_child(grandparent)
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.append_child(parent)

    grandparent = doc.create_element("grandparent")
    parent = doc.create_element("parent")
    grandparent.append_child(parent)
    check(grandparent, parent, False)
    grandparent = doc.create_element("grandparent")
    parent = doc.create_element("parent")
    grandparent.append_child(parent)
    root.append_child(grandparent)
    check(root, parent, True)

    # Check appended to a document parent

    doc = _make_xml_document()
    context.check_equal(doc.has_child_nodes(), False)
    impl = doc.get_implementation()
    child = impl.create_document_type("foo", "bar", "baz")
    doc.append_child(child)
    context.check_equal(doc.has_child_nodes(), True)

    doc = _make_xml_document()
    context.check_equal(doc.has_child_nodes(), False)
    child = doc.create_element("elem")
    doc.append_child(child)
    context.check_equal(doc.has_child_nodes(), True)

    doc = _make_xml_document()
    context.check_equal(doc.has_child_nodes(), False)
    child = doc.create_comment("comment")
    doc.append_child(child)
    context.check_equal(doc.has_child_nodes(), True)

    doc = _make_xml_document()
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.append_child(_make_xml_document())
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.append_child(doc.create_text_node("text"))
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.append_child(doc.create_attribute("attr"))

    doc = _make_xml_document()
    impl = doc.get_implementation()
    doctype = impl.create_document_type("foo", "bar", "baz")
    doc.append_child(doctype)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.append_child(impl.create_document_type("foo", "bar", "baz"))

    doc = _make_xml_document()
    impl = doc.get_implementation()
    elem = doc.create_element("elem")
    doc.append_child(elem)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.append_child(impl.create_document_type("foo", "bar", "baz"))
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.append_child(doc.create_element("elem"))

    # Check appended to an element parent

    doc = _make_xml_document()
    elem = doc.create_element("parent")
    context.check_equal(elem.has_child_nodes(), False)
    child = doc.create_element("elem")
    elem.append_child(child)
    context.check_equal(elem.has_child_nodes(), True)

    elem = doc.create_element("parent")
    context.check_equal(elem.has_child_nodes(), False)
    child = doc.create_text_node("text")
    elem.append_child(child)
    context.check_equal(elem.has_child_nodes(), True)

    elem = doc.create_element("parent")
    context.check_equal(elem.has_child_nodes(), False)
    child = doc.create_comment("comment")
    elem.append_child(child)
    context.check_equal(elem.has_child_nodes(), True)

    elem = doc.create_element("parent")
    impl = doc.get_implementation()
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.append_child(_make_xml_document())
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.append_child(impl.create_document_type("foo", "bar", "baz"))
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.append_child(doc.create_attribute("attr"))

    # Check that no kinds of nodes can be appended to a non-parent node

    doc = _make_xml_document()
    impl = doc.get_implementation()
    def check(parent):
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.append_child(_make_xml_document())
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.append_child(doc.create_element("elem"))
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.append_child(impl.create_document_type("foo", "bar", "baz"))
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.append_child(doc.create_text_node("text"))
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.append_child(doc.create_comment("comment"))
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.append_child(doc.create_attribute("attr"))
    check(impl.create_document_type("foo", "bar", "baz"))
    check(doc.create_text_node("test"))
    check(doc.create_comment("comment"))
    check(doc.create_attribute("attr"))

    # Check that non-node arguments are properly dealt with

    class WrongImplText(archon.dom.Text):
        pass

    def check(elem):
        with context.check_raises(TypeError):
            elem.append_child(None)
        with context.check_raises(TypeError):
            elem.append_child(7)
        with context.check_raises(TypeError):
            elem.append_child("foo")
        with context.check_raises(TypeError):
            elem.append_child(WrongImplText())
    doc = _make_xml_document()
    check(doc)
    check(doc.create_element("elem"))


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
    # FIXME: More needs to be tested here: len(), argument validity, ...    
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

    elem = doc_1.create_element("foo")
    context.check_equal(elem.get_owner_document(), doc_1)

    check_elem(context, doc_1.create_element("Foo"), None, None, "Foo")
    check_elem(context, doc_1.create_element("p:Foo"), None, None, "p:Foo")
    check_elem(context, doc_2.create_element("Foo"), "http://www.w3.org/1999/xhtml", None, "foo")
    check_elem(context, doc_2.create_element("p:Foo"), "http://www.w3.org/1999/xhtml", None, "p:foo")
    check_elem(context, doc_3.create_element("Foo"), "http://www.w3.org/1999/xhtml", None, "Foo")
    check_elem(context, doc_3.create_element("p:Foo"), "http://www.w3.org/1999/xhtml", None, "p:Foo")


def test_Document_CreateElementNS(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()

    elem = doc_1.create_element_ns("ns", "foo")
    context.check_equal(elem.get_owner_document(), doc_1)

    check_elem(context, doc_1.create_element_ns("ns", "Foo"), "ns", None, "Foo")
    check_elem(context, doc_1.create_element_ns("ns", "p:Foo"), "ns", "p", "Foo")
    check_elem(context, doc_2.create_element_ns("ns", "Foo"), "ns", None, "Foo")
    check_elem(context, doc_2.create_element_ns("ns", "p:Foo"), "ns", "p", "Foo")

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

    attr = doc_1.create_attribute("foo")
    context.check_equal(attr.get_owner_document(), doc_1)
    context.check_equal(attr.get_owner_element(), None)

    check_elem(context, doc_1.create_attribute("Foo"), None, None, "Foo")
    check_elem(context, doc_1.create_attribute("p:Foo"), None, None, "p:Foo")
    check_elem(context, doc_2.create_attribute("Foo"), None, None, "foo")
    check_elem(context, doc_2.create_attribute("p:Foo"), None, None, "p:foo")
    check_elem(context, doc_3.create_attribute("Foo"), None, None, "Foo")
    check_elem(context, doc_3.create_attribute("p:Foo"), None, None, "p:Foo")


def test_Document_CreateAttributeNS(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()

    attr = doc_1.create_attribute_ns("ns", "foo")
    context.check_equal(attr.get_owner_document(), doc_1)
    context.check_equal(attr.get_owner_element(), None)

    check_elem(context, doc_1.create_attribute_ns("ns", "Foo"), "ns", None, "Foo")
    check_elem(context, doc_1.create_attribute_ns("ns", "p:Foo"), "ns", "p", "Foo")
    check_elem(context, doc_2.create_attribute_ns("ns", "Foo"), "ns", None, "Foo")
    check_elem(context, doc_2.create_attribute_ns("ns", "p:Foo"), "ns", "p", "Foo")

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


def test_Element_AttributeBasics(context):
    def check(doc, is_html):
        elem = doc.create_element("elem")
        attributes = elem.get_attributes()
        context.check_equal(attributes.get_length(), 0)
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(0), None)

        elem.set_attribute("p:foo", "1")
        context.check_equal(attributes.get_length(), 1)
        attr_1 = attributes.item(0)
        check_attr(context, attr_1, None, None, "p:foo", "1")
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(1), None)

        elem.set_attribute("p:Bar", "2")
        context.check_equal(attributes.get_length(), 2)
        context.check_equal(attributes.item(0), attr_1)
        attr_2 = attributes.item(1)
        check_attr(context, attr_2, None, None, "p:bar" if is_html else "p:Bar", "2")
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(2), None)

        elem.set_attribute_ns("ns", "p:Baz", "3")
        context.check_equal(attributes.get_length(), 3)
        context.check_equal(attributes.item(0), attr_1)
        context.check_equal(attributes.item(1), attr_2)
        attr_3 = attributes.item(2)
        check_attr(context, attr_3, "ns", "p", "Baz", "3")
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(3), None)

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_Element_HasAttributes(context):
    doc = _make_xml_document()
    elem = doc.create_element("elem")
    context.check_not(elem.has_attributes())
    elem.set_attribute("foo", "1")
    context.check(elem.has_attributes())


def test_Element_GetAttributeNames(context):
    def check(doc, is_html):
        elem = doc.create_element("elem")
        elem.set_attribute("Foo", "1")
        elem.set_attribute("p:Bar", "2")
        elem.set_attribute_ns("ns1", "Foo", "3")
        elem.set_attribute_ns("ns2", "Foo", "4")
        elem.set_attribute_ns("ns3", "p:Bar", "5")
        names = elem.get_attribute_names()
        context.check_equal(len(names), 5)
        context.check_equal(names[0], "foo" if is_html else "Foo")
        context.check_equal(names[1], "p:bar" if is_html else "p:Bar")
        context.check_equal(names[2], "Foo")
        context.check_equal(names[3], "Foo")
        context.check_equal(names[4], "p:Bar")

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_Element_SetAttribute(context):
    def check(doc, is_html):
        elem = doc.create_element("elem")
        attributes_1 = elem.get_attributes()
        elem.set_attribute("p:foo1", "1")
        elem.set_attribute("p:foo2", "2")
        elem.set_attribute_ns("ns", "p:bar1", "3")
        elem.set_attribute_ns("ns", "p:bar2", "4")
        elem.set_attribute_ns("ns", "p:Baz", "5")

        context.check_equal(len(attributes_1), 5)
        check_attr(context, attributes_1.item(0), None, None, "p:foo1", "1")
        check_attr(context, attributes_1.item(1), None, None, "p:foo2", "2")
        check_attr(context, attributes_1.item(2), "ns", "p", "bar1", "3")
        check_attr(context, attributes_1.item(3), "ns", "p", "bar2", "4")
        check_attr(context, attributes_1.item(4), "ns", "p", "Baz", "5")

        attributes_2 = list(attributes_1.values())
        def check_match(qualified_name, new_value, index):
            elem.set_attribute(qualified_name, new_value)
            context.check_equal(len(attributes_1), len(attributes_2))
            context.check_equal(list(attributes_1.values()), attributes_2)
            context.check_equal(attributes_1.item(index).get_value(), new_value)

        def check_mismatch(qualified_name, new_value):
            nonlocal attributes_2
            elem.set_attribute(qualified_name, new_value)
            context.check_equal(len(attributes_1), len(attributes_2) + 1)
            attributes_3 = list(attributes_1.values())
            context.check_equal(attributes_3[:-1], attributes_2)
            local_name = qualified_name.lower() if is_html else qualified_name
            check_attr(context, attributes_3[-1], None, None, local_name, new_value)
            attributes_2 = attributes_3

        if is_html:
            check_match("p:foo1", "11", 0)
            check_match("p:Foo2", "12", 1)
            check_match("p:bar1", "13", 2)
            check_match("p:Bar2", "14", 3)
            check_mismatch("p:Baz", "15")
        else:
            check_match("p:foo1", "11", 0)
            check_mismatch("p:Foo2", "12")
            check_match("p:bar1", "13", 2)
            check_mismatch("p:Bar2", "14")
            check_match("p:Baz", "15", 4)

        with context.check_raises(archon.dom.InvalidCharacterError):
            elem.set_attribute("", "x")
        with context.check_raises(archon.dom.InvalidCharacterError):
            elem.set_attribute(" ", "x")
        with context.check_raises(archon.dom.InvalidCharacterError):
            elem.set_attribute("@", "x")

        # FIXME: Check argument type coercion rules (None -> "None", ...)      

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


# FIXME: Add test_Element_SetAttributeNS()                


def test_Element_ToggleAttribute(context):
    def check(doc, is_html):
        elem = doc.create_element("elem")
        attributes = elem.get_attributes()

        now_present = elem.toggle_attribute("foo")
        context.check(now_present)
        context.check_equal(len(attributes), 1)
        attr_1 = attributes.item(0)
        check_attr(context, attr_1, None, None, "foo", "")

        now_present = elem.toggle_attribute("bar")
        context.check(now_present)
        context.check_equal(len(attributes), 2)
        context.check_equal(attributes.item(0), attr_1)
        check_attr(context, attributes.item(1), None, None, "bar", "")

        now_present = elem.toggle_attribute("bar")
        context.check_not(now_present)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_1)

        now_present = elem.toggle_attribute("foo", force = True)
        context.check(now_present)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_1)

        now_present = elem.toggle_attribute("bar", force = True)
        context.check(now_present)
        context.check_equal(len(attributes), 2)
        context.check_equal(attributes.item(0), attr_1)
        attr_2 = attributes.item(1)
        check_attr(context, attr_2, None, None, "bar", "")

        now_present = elem.toggle_attribute("baz", force = False)
        context.check_not(now_present)
        context.check_equal(len(attributes), 2)
        context.check_equal(attributes.item(0), attr_1)
        context.check_equal(attributes.item(1), attr_2)

        now_present = elem.toggle_attribute("foo", force = False)
        context.check_not(now_present)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_2)

        # New element for testing matching rules
        elem = doc.create_element("elem")
        elem.set_attribute("p:foo1", "1")
        elem.set_attribute("p:foo2", "2")
        elem.set_attribute_ns("ns", "p:bar1", "3")
        elem.set_attribute_ns("ns", "p:bar2", "4")
        elem.set_attribute_ns("ns", "p:Baz", "5")
        if is_html:
            context.check_not(elem.toggle_attribute("p:foo1"))
            context.check_not(elem.toggle_attribute("p:Foo2"))
            context.check_not(elem.toggle_attribute("p:bar1"))
            context.check_not(elem.toggle_attribute("p:Bar2"))
            context.check(elem.toggle_attribute("p:Baz"))
        else:
            context.check_not(elem.toggle_attribute("p:foo1"))
            context.check(elem.toggle_attribute("p:Foo2"))
            context.check_not(elem.toggle_attribute("p:bar1"))
            context.check(elem.toggle_attribute("p:Bar2"))
            context.check_not(elem.toggle_attribute("p:Baz"))

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_Element_SetAttributeNode(context):
    def check(doc, is_html):
        elem = doc.create_element("elem")
        attributes = elem.get_attributes()

        attr_1 = doc.create_attribute("foo")
        attr = elem.set_attribute_node(attr_1)
        context.check_is_none(attr)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_1)

        attr_2 = doc.create_attribute("foo")
        attr = elem.set_attribute_node(attr_2)
        context.check_equal(attr, attr_1)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_2)

        attr_3 = doc.create_attribute("bar")
        attr = elem.set_attribute_node(attr_3)
        context.check_is_none(attr)
        context.check_equal(len(attributes), 2)
        context.check_equal(attributes.item(0), attr_2)
        context.check_equal(attributes.item(1), attr_3)

        # FIXME: Reneable once migration of attributes has been implemented                      
        # def subcheck(doc_2, is_html_2):
        #     elem = doc.create_element("elem")
        #     attributes = elem.get_attributes()
        #     attr_1 = doc_2.create_attribute("foo")
        #     context.check_equal(attr_1.get_owner_document(), doc_2)
        #     attr_2 = elem.set_attribute_node(attr_1)
        #     context.check_is_none(attr_2)
        #     context.check_equal(len(attributes), 1)
        #     context.check_equal(attributes.item(0), attr_1)
        #     context.check_equal(attr_1.get_owner_document(), doc)

        # subcheck(_make_xml_document(), False)
        # subcheck(_make_html_document(), True)

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


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


def test_Element_AttributesAsMapping(context):
    # Quirkiness abound:
    #
    # * The attribute map may contain multiple attributes with the same key (`attr_1` and
    #   `attr_2` below). In a sense, the attribute map is a multi-map.
    #
    # * In an HTML document, attributes with a qualified name that is not "ASCII lower case"
    #   (`attr_3` and `attr_4` below) cannot be reached through subscription. I.e., if
    #   `attr` is such an attribute and `map` is the attribute map, then there is no `key`
    #   such that `map[key] == attr` or `map.get(key) == attr`. Likewise, there is no `key`
    #   such that `key in map` is true so long as `key` is not the qualified name of some
    #   other attribute in `map`. Further more, the list of keys as returned by `iter(map)`
    #   or `map.keys()` does contain an entry for `attr`, but that key will appear to not be
    #   in the map from the point of view of `map[key]`, `map.get(key)`, and `key in map`.
    #
    # Corresponding quirkiness is present in Firefox and Chrome browsers at the JavaScript
    # level.
    #
    def check(doc, is_html):
        elem = doc.create_element("elem")
        attr_1 = doc.create_attribute_ns("ns1", "foo")
        attr_2 = doc.create_attribute_ns("ns2", "foo")
        attr_3 = doc.create_attribute_ns("ns3", "Foo")
        attr_4 = doc.create_attribute_ns("ns3", "Bar")
        elem.set_attribute_node(attr_1)
        elem.set_attribute_node(attr_2)
        elem.set_attribute_node(attr_3)
        elem.set_attribute_node(attr_4)
        attributes = elem.get_attributes()
        context.check_equal(len(attributes), 4)
        context.check_equal(attributes["foo"], attr_1)
        context.check_equal(attributes["Foo"], attr_1 if is_html else attr_3)
        with context.check_raises(KeyError):
            attributes["bar"]
        if is_html:
            with context.check_raises(KeyError):
                attributes["Bar"]
        else:
            context.check_equal(attributes["Bar"], attr_4)
        with context.check_raises(TypeError):
            attributes[1]
        context.check_equal(attributes.get("foo"), attr_1)
        context.check_equal(attributes.get("Foo"), attr_1 if is_html else attr_3)
        context.check_equal(attributes.get("bar"), None)
        context.check_equal(attributes.get("Bar"), None if is_html else attr_4)
        context.check_equal("foo" in attributes, True)
        context.check_equal("Foo" in attributes, True)
        context.check_equal("bar" in attributes, False)
        context.check_equal("Bar" in attributes, False if is_html else True)

        attributes_2 = list(attributes)
        context.check_equal(attributes_2[0], "foo")
        context.check_equal(attributes_2[1], "foo")
        context.check_equal(attributes_2[2], "Foo")
        context.check_equal(attributes_2[3], "Bar")

        keys = attributes.keys()
        context.check_equal(len(keys), 4)
        keys_2 = list(keys)
        context.check_equal(keys_2[0], "foo")
        context.check_equal(keys_2[1], "foo")
        context.check_equal(keys_2[2], "Foo")
        context.check_equal(keys_2[3], "Bar")

        values = attributes.values()
        context.check_equal(len(values), 4)
        values_2 = list(values)
        context.check_equal(values_2[0], attr_1)
        context.check_equal(values_2[1], attr_2)
        context.check_equal(values_2[2], attr_3)
        context.check_equal(values_2[3], attr_4)

        items = attributes.items()
        context.check_equal(len(items), 4)
        items_2 = list(items)
        context.check_equal(items_2[0], ("foo", attr_1))
        context.check_equal(items_2[1], ("foo", attr_2))
        context.check_equal(items_2[2], ("Foo", attr_3))
        context.check_equal(items_2[3], ("Bar", attr_4))

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


# Bridge to Python's native testing framework
def load_tests(loader, standard_tests, pattern):
    return archon.test.generate_native_tests(__name__)


if __name__ == '__main__':
    archon.test.run_module_tests(__name__)
