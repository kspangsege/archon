import archon.test
import archon.dom


def _make_xml_document(content_type = None):
    return archon.dom.create_xml_document(content_type = content_type)

def _make_html_document(content_type = None):
    return archon.dom.create_html_document(content_type = content_type)


def check_elem(context, elem, namespace_uri, prefix, local_name):
    context.check_equal(elem.getNamespaceURI(), namespace_uri)
    context.check_equal(elem.getPrefix(), prefix)
    context.check_equal(elem.getLocalName(), local_name)

def check_attr(context, attr, namespace_uri, prefix, local_name, value):
    context.check_equal(attr.getNamespaceURI(), namespace_uri)
    context.check_equal(attr.getPrefix(), prefix)
    context.check_equal(attr.getLocalName(), local_name)
    context.check_equal(attr.getValue(), value)



def test_CreateXMLDocument(context):
    doc = archon.dom.create_xml_document()
    context.check_equal(doc.getContentType(), "application/xml")
    doc = archon.dom.create_xml_document(content_type = None)
    context.check_equal(doc.getContentType(), "application/xml")
    doc = archon.dom.create_xml_document(content_type = "foo")
    context.check_equal(doc.getContentType(), "foo")
    with context.check_raises(TypeError):
        archon.dom.create_xml_document(content_type = 7)


def test_CreateHTMLDocument(context):
    doc = archon.dom.create_html_document()
    context.check_equal(doc.getContentType(), "text/html")
    doc = archon.dom.create_html_document(content_type = None)
    context.check_equal(doc.getContentType(), "text/html")
    doc = archon.dom.create_html_document(content_type = "foo")
    context.check_equal(doc.getContentType(), "foo")
    with context.check_raises(TypeError):
        archon.dom.create_html_document(content_type = 7)


def test_CreateDocumentType(context):
    def check(doc, is_html):
        doctype = archon.dom.create_document_type(doc, "Foo", "Bar", "Baz")
        context.check_equal(doctype.getOwnerDocument(), doc)

        doctype = archon.dom.create_document_type(doc, "Foo", "Bar", "Baz")
        context.check_equal(doctype.getName(), "Foo")
        context.check_equal(doctype.getPublicId(), "Bar")
        context.check_equal(doctype.getSystemId(), "Baz")

        doctype = archon.dom.create_document_type(doc, "", "", "")
        context.check_equal(doctype.getName(), "")
        context.check_equal(doctype.getPublicId(), "")
        context.check_equal(doctype.getSystemId(), "")

        bad_doc = doc.createTextNode("foo")
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
        context.check_equal(attr.getOwnerDocument(), doc)

        attr = archon.dom.create_attribute(doc, "ns", "p", "Foo", "")
        context.check_equal(attr.getNamespaceURI(), "ns")
        context.check_equal(attr.getPrefix(), "p")
        context.check_equal(attr.getLocalName(), "Foo")

        attr = archon.dom.create_attribute(doc, "ns", None, "Foo", "")
        context.check_equal(attr.getNamespaceURI(), "ns")
        context.check_equal(attr.getPrefix(), None)
        context.check_equal(attr.getLocalName(), "Foo")

        attr = archon.dom.create_attribute(doc, None, None, "Foo", "")
        context.check_equal(attr.getNamespaceURI(), None)
        context.check_equal(attr.getPrefix(), None)
        context.check_equal(attr.getLocalName(), "Foo")

        attr = archon.dom.create_attribute(doc, "ns", None, "@:p:Foo", "")
        context.check_equal(attr.getNamespaceURI(), "ns")
        context.check_equal(attr.getPrefix(), None)
        context.check_equal(attr.getLocalName(), "@:p:Foo")

        attr = archon.dom.create_attribute(doc, None, None, "@:p:Foo", "")
        context.check_equal(attr.getNamespaceURI(), None)
        context.check_equal(attr.getPrefix(), None)
        context.check_equal(attr.getLocalName(), "@:p:Foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/XML/1998/namespace", "xml", "foo", "")
        context.check_equal(attr.getNamespaceURI(), "http://www.w3.org/XML/1998/namespace")
        context.check_equal(attr.getPrefix(), "xml")
        context.check_equal(attr.getLocalName(), "foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", "xmlns", "foo", "")
        context.check_equal(attr.getNamespaceURI(), "http://www.w3.org/2000/xmlns/")
        context.check_equal(attr.getPrefix(), "xmlns")
        context.check_equal(attr.getLocalName(), "foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", None, "xmlns", "")
        context.check_equal(attr.getNamespaceURI(), "http://www.w3.org/2000/xmlns/")
        context.check_equal(attr.getPrefix(), None)
        context.check_equal(attr.getLocalName(), "xmlns")

        bad_doc = doc.createTextNode("foo")
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
        context.check_equal(attr.getValue(), "")
        attr = archon.dom.create_attribute(doc, "ns", "p", "foo", "1")
        context.check_equal(attr.getValue(), "1")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", "p", "foo", None)
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", "p", "foo", 7)

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_CreateElement(context):
    def check(doc, is_html):
        elem = archon.dom.create_element(doc, "ns", "p", "Foo", [])
        context.check_equal(elem.getOwnerDocument(), doc)

        elem = archon.dom.create_element(doc, "ns", "p", "Foo", [])
        context.check_equal(elem.getNamespaceURI(), "ns")
        context.check_equal(elem.getPrefix(), "p")
        context.check_equal(elem.getLocalName(), "Foo")

        elem = archon.dom.create_element(doc, "ns", None, "Foo", [])
        context.check_equal(elem.getNamespaceURI(), "ns")
        context.check_equal(elem.getPrefix(), None)
        context.check_equal(elem.getLocalName(), "Foo")

        elem = archon.dom.create_element(doc, None, None, "Foo", [])
        context.check_equal(elem.getNamespaceURI(), None)
        context.check_equal(elem.getPrefix(), None)
        context.check_equal(elem.getLocalName(), "Foo")

        elem = archon.dom.create_element(doc, "ns", None, "@:p:Foo", [])
        context.check_equal(elem.getNamespaceURI(), "ns")
        context.check_equal(elem.getPrefix(), None)
        context.check_equal(elem.getLocalName(), "@:p:Foo")

        elem = archon.dom.create_element(doc, None, None, "@:p:Foo", [])
        context.check_equal(elem.getNamespaceURI(), None)
        context.check_equal(elem.getPrefix(), None)
        context.check_equal(elem.getLocalName(), "@:p:Foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/XML/1998/namespace", "xml", "foo", [])
        context.check_equal(elem.getNamespaceURI(), "http://www.w3.org/XML/1998/namespace")
        context.check_equal(elem.getPrefix(), "xml")
        context.check_equal(elem.getLocalName(), "foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", "xmlns", "foo", [])
        context.check_equal(elem.getNamespaceURI(), "http://www.w3.org/2000/xmlns/")
        context.check_equal(elem.getPrefix(), "xmlns")
        context.check_equal(elem.getLocalName(), "foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", None, "xmlns", [])
        context.check_equal(elem.getNamespaceURI(), "http://www.w3.org/2000/xmlns/")
        context.check_equal(elem.getPrefix(), None)
        context.check_equal(elem.getLocalName(), "xmlns")

        bad_doc = doc.createTextNode("foo")
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
        attributes = elem.getAttributes()
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
        bad_attr = doc.createTextNode("foo")
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
    impl = doc.getImplementation()
    context.check_is_instance(impl, archon.dom.DOMImplementation)

    doctype = impl.createDocumentType("a", "b", "c")
    context.check_is_instance(doctype, archon.dom.DocumentType)
    context.check_equal(doctype.getName(), "a")
    context.check_equal(doctype.getPublicId(), "b")
    context.check_equal(doctype.getSystemId(), "c")
    context.check_equal(doctype.getOwnerDocument(), doc)

    with context.check_raises(TypeError):
        impl.createDocumentType(7, "b", "c")
    with context.check_raises(TypeError):
        impl.createDocumentType(None, "b", "c")
    with context.check_raises(TypeError):
        impl.createDocumentType("a", 7, "c")
    with context.check_raises(TypeError):
        impl.createDocumentType("a", None, "c")
    with context.check_raises(TypeError):
        impl.createDocumentType("a", "b", 7)
    with context.check_raises(TypeError):
        impl.createDocumentType("a", "b", None)

    doc_2 = impl.createDocument("ns", "p:foo", doctype)
    context.check_is_instance(doc_2, archon.dom.XMLDocument)
    context.check_equal(doc_2.getDoctype(), doctype)
    # The document type node must have been migrated to the new document
    context.check_equal(doctype.getOwnerDocument(), doc_2)
    root = doc_2.getDocumentElement()
    context.check(root)
    context.check_equal(doc_2.getDocumentElement().getNamespaceURI(), "ns")
    context.check_equal(doc_2.getDocumentElement().getPrefix(), "p")
    context.check_equal(doc_2.getDocumentElement().getLocalName(), "foo")

    doc_3 = impl.createHTMLDocument("hello")
    context.check_is_instance(doc_3, archon.dom.Document)
    context.check_not_is_instance(doc_3, archon.dom.XMLDocument)
    context.check_equal(doc_3.getDoctype().getName(), "html")
    html = doc_3.getDocumentElement()
    context.check(html)
    context.check_equal(html.getLocalName(), "html")
    context.check_equal(len(html.getChildNodes()), 2)
    head = html.getChildNodes()[0]
    body = html.getChildNodes()[1]
    context.check_equal(head.getLocalName(), "head")
    context.check_equal(body.getLocalName(), "body")
    context.check_equal(len(head.getChildNodes()), 1)
    title = head.getChildNodes()[0]
    context.check_equal(title.getLocalName(), "title")
    context.check_equal(len(title.getChildNodes()), 1)
    text = title.getChildNodes()[0]
    context.check_equal(text.getData(), "hello")
    context.check_equal(len(body.getChildNodes()), 0)

    context.check(impl.hasFeature())


def test_Node_NodeName(context):
    doc = _make_xml_document()
    doctype = doc.getImplementation().createDocumentType("a", "b", "c")
    elem_1 = doc.createElement("xFoo")
    elem_2 = doc.createElementNS("ns1", "p:xFoo")
    text = doc.createTextNode("x")
    comment = doc.createComment("x")
    attr_1 = doc.createAttribute("xBar")
    attr_2 = doc.createAttributeNS("ns2", "p:xBar")
    context.check_equal(doctype.getNodeName(), "a")
    context.check_equal(doc.getNodeName(), "#document")
    context.check_equal(elem_1.getNodeName(), "xFoo")
    context.check_equal(elem_2.getNodeName(), "p:xFoo")
    context.check_equal(text.getNodeName(), "#text")
    context.check_equal(comment.getNodeName(), "#comment")
    context.check_equal(attr_1.getNodeName(), "xBar")
    context.check_equal(attr_2.getNodeName(), "p:xBar")

    doc = _make_html_document()
    elem_1 = doc.createElement("xFoo")
    elem_2 = doc.createElementNS("ns1", "p:xFoo")
    attr_1 = doc.createAttribute("xBar")
    attr_2 = doc.createAttributeNS("ns2", "p:xBar")
    context.check_equal(doc.getNodeName(), "#document")
    context.check_equal(elem_1.getNodeName(), "XFOO")
    context.check_equal(elem_2.getNodeName(), "p:xFoo")
    context.check_equal(attr_1.getNodeName(), "xbar")
    context.check_equal(attr_2.getNodeName(), "p:xBar")


def test_Node_ChildNodeBasics(context):
    doc = _make_xml_document()
    parent = doc.createElement("parent")
    child_nodes = parent.getChildNodes()
    context.check_equal(parent.getFirstChild(), None)
    context.check_equal(parent.getLastChild(), None)
    context.check_equal(child_nodes.getLength(), 0)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), None)

    child_1 = doc.createTextNode("1")
    parent.appendChild(child_1)
    context.check_equal(parent.getFirstChild(), child_1)
    context.check_equal(parent.getLastChild(), child_1)
    context.check_equal(child_nodes.getLength(), 1)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), None)

    child_2 = doc.createTextNode("2")
    parent.appendChild(child_2)
    context.check_equal(parent.getFirstChild(), child_1)
    context.check_equal(parent.getLastChild(), child_2)
    context.check_equal(child_nodes.getLength(), 2)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_2)
    context.check_equal(child_nodes.item(2), None)

    child_3 = doc.createTextNode("3")
    parent.appendChild(child_3)
    context.check_equal(parent.getFirstChild(), child_1)
    context.check_equal(parent.getLastChild(), child_3)
    context.check_equal(child_nodes.getLength(), 3)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_2)
    context.check_equal(child_nodes.item(2), child_3)
    context.check_equal(child_nodes.item(3), None)

    parent.removeChild(child_2)
    context.check_equal(parent.getFirstChild(), child_1)
    context.check_equal(parent.getLastChild(), child_3)
    context.check_equal(child_nodes.getLength(), 2)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_3)
    context.check_equal(child_nodes.item(2), None)


def test_Node_ParentNode(context):
    doc = _make_xml_document()
    context.check_equal(doc.getParentNode(), None)
    elem = doc.createElement("elem")
    attr = doc.createAttribute("attr")
    context.check_equal(attr.getParentNode(), None)
    elem.setAttributeNode(attr)
    context.check_equal(attr.getParentNode(), None)

    impl = doc.getImplementation()
    doctype = impl.createDocumentType("foo", "bar", "baz")
    context.check_equal(doctype.getParentNode(), None)
    doc.appendChild(doctype)
    context.check_equal(doctype.getParentNode(), doc)
    doc.removeChild(doctype)
    context.check_equal(doctype.getParentNode(), None)

    def check(child):
        context.check_equal(child.getParentNode(), None)
        elem.appendChild(child)
        context.check_equal(child.getParentNode(), elem)
        elem.removeChild(child)
        context.check_equal(child.getParentNode(), None)

    check(doc.createElement("foo"))
    check(doc.createTextNode("foo"))
    check(doc.createComment("foo"))


def test_Node_HasChildNodes(context):
    doc = _make_xml_document()
    context.check_not(doc.hasChildNodes())
    elem = doc.createElement("elem")
    context.check_not(elem.hasChildNodes())
    doc.appendChild(elem)
    context.check(doc.hasChildNodes())
    context.check_not(elem.hasChildNodes())
    elem.appendChild(doc.createTextNode("text"))
    context.check(elem.hasChildNodes())

    impl = doc.getImplementation()
    doctype = impl.createDocumentType("foo", "bar", "baz")
    context.check_not(doctype.hasChildNodes())
    text = doc.createTextNode("text")
    context.check_not(text.hasChildNodes())
    comment = doc.createComment("comment")
    context.check_not(comment.hasChildNodes())


def test_Node_PreviousNextSibling(context):
    doc = _make_xml_document()
    context.check_equal(doc.getPreviousSibling(), None)
    context.check_equal(doc.getNextSibling(), None)
    elem = doc.createElement("elem")
    attr_1 = doc.createAttribute("attr_1")
    attr_2 = doc.createAttribute("attr_2")
    context.check_equal(attr_1.getPreviousSibling(), None)
    context.check_equal(attr_1.getNextSibling(), None)
    context.check_equal(attr_2.getPreviousSibling(), None)
    context.check_equal(attr_2.getNextSibling(), None)
    elem.setAttributeNode(attr_1)
    elem.setAttributeNode(attr_2)
    context.check_equal(attr_1.getPreviousSibling(), None)
    context.check_equal(attr_1.getNextSibling(), None)
    context.check_equal(attr_2.getPreviousSibling(), None)
    context.check_equal(attr_2.getNextSibling(), None)

    comment_1 = doc.createComment("comment 1")
    comment_2 = doc.createComment("comment 2")

    impl = doc.getImplementation()
    doctype = impl.createDocumentType("foo", "bar", "baz")
    context.check_equal(doctype.getPreviousSibling(), None)
    context.check_equal(doctype.getNextSibling(), None)
    doc.appendChild(doctype)
    context.check_equal(doctype.getPreviousSibling(), None)
    context.check_equal(doctype.getNextSibling(), None)
    doc.removeChild(doctype)
    doc.appendChild(comment_1)
    doc.appendChild(doctype)
    context.check_equal(doctype.getPreviousSibling(), comment_1)
    context.check_equal(doctype.getNextSibling(), None)
    doc.appendChild(comment_2)
    context.check_equal(doctype.getPreviousSibling(), comment_1)
    context.check_equal(doctype.getNextSibling(), comment_2)
    doc.removeChild(comment_1)
    context.check_equal(doctype.getPreviousSibling(), None)
    context.check_equal(doctype.getNextSibling(), comment_2)
    doc.removeChild(doctype)
    context.check_equal(doctype.getPreviousSibling(), None)
    context.check_equal(doctype.getNextSibling(), None)
    doc.removeChild(comment_2)

    def check(child):
        context.check_equal(child.getPreviousSibling(), None)
        context.check_equal(child.getNextSibling(), None)
        elem.appendChild(child)
        context.check_equal(child.getPreviousSibling(), None)
        context.check_equal(child.getNextSibling(), None)
        elem.removeChild(child)
        elem.appendChild(comment_1)
        elem.appendChild(child)
        context.check_equal(child.getPreviousSibling(), comment_1)
        context.check_equal(child.getNextSibling(), None)
        elem.appendChild(comment_2)
        context.check_equal(child.getPreviousSibling(), comment_1)
        context.check_equal(child.getNextSibling(), comment_2)
        elem.removeChild(comment_1)
        context.check_equal(child.getPreviousSibling(), None)
        context.check_equal(child.getNextSibling(), comment_2)
        elem.removeChild(child)
        context.check_equal(child.getPreviousSibling(), None)
        context.check_equal(child.getNextSibling(), None)
        elem.removeChild(comment_2)

    check(doc.createElement("foo"))
    check(doc.createTextNode("foo"))
    check(doc.createComment("foo"))


def test_Node_AppendChild(context):
    doc = _make_xml_document()
    root = doc.createElement("root")

    def check(grandparent, parent, is_connected):
        child_nodes = parent.getChildNodes()
        context.check_equal(parent.getFirstChild(), None)
        context.check_equal(parent.getLastChild(), None)
        context.check_equal(child_nodes.getLength(), 0)

        child_1 = doc.createElement("elem")
        parent.appendChild(child_1)
        context.check_equal(parent.getFirstChild(), child_1)
        context.check_equal(parent.getLastChild(), child_1)
        context.check_equal(child_nodes.getLength(), 1)
        context.check_equal(child_nodes.item(0), child_1)
        context.check_equal(child_1.getParentNode(), parent)
        context.check_equal(child_1.getPreviousSibling(), None)
        context.check_equal(child_1.getNextSibling(), None)

        child_2 = doc.createTextNode("text")
        parent.appendChild(child_2)
        context.check_equal(parent.getFirstChild(), child_1)
        context.check_equal(parent.getLastChild(), child_2)
        context.check_equal(child_nodes.getLength(), 2)
        context.check_equal(child_nodes.item(0), child_1)
        context.check_equal(child_nodes.item(1), child_2)
        context.check_equal(child_1.getParentNode(), parent)
        context.check_equal(child_1.getPreviousSibling(), None)
        context.check_equal(child_1.getNextSibling(), child_2)
        context.check_equal(child_2.getParentNode(), parent)
        context.check_equal(child_2.getPreviousSibling(), child_1)
        context.check_equal(child_2.getNextSibling(), None)

        child_3 = doc.createComment("comment")
        parent.appendChild(child_3)
        context.check_equal(parent.getFirstChild(), child_1)
        context.check_equal(parent.getLastChild(), child_3)
        context.check_equal(child_nodes.getLength(), 3)
        context.check_equal(child_nodes.item(0), child_1)
        context.check_equal(child_nodes.item(1), child_2)
        context.check_equal(child_nodes.item(2), child_3)
        context.check_equal(child_1.getParentNode(), parent)
        context.check_equal(child_1.getPreviousSibling(), None)
        context.check_equal(child_1.getNextSibling(), child_2)
        context.check_equal(child_2.getParentNode(), parent)
        context.check_equal(child_2.getPreviousSibling(), child_1)
        context.check_equal(child_2.getNextSibling(), child_3)
        context.check_equal(child_3.getParentNode(), parent)
        context.check_equal(child_3.getPreviousSibling(), child_2)
        context.check_equal(child_3.getNextSibling(), None)

        parent.removeChild(child_3)
        parent.removeChild(child_2)

        def subcheck(child, subchild):
            parent.appendChild(child)
            context.check_equal(parent.getFirstChild(), child_1)
            context.check_equal(parent.getLastChild(), child)
            context.check_equal(child_nodes.getLength(), 2)
            context.check_equal(child_nodes.item(0), child_1)
            context.check_equal(child_nodes.item(1), child)
            context.check_equal(child_1.getParentNode(), parent)
            context.check_equal(child_1.getPreviousSibling(), None)
            context.check_equal(child_1.getNextSibling(), child)
            context.check_equal(child.getParentNode(), parent)
            context.check_equal(child.getPreviousSibling(), child_1)
            context.check_equal(child.getNextSibling(), None)
            context.check_equal(child.getOwnerDocument(), doc)
            if subchild:
                context.check_equal(subchild.getParentNode(), child)
                context.check_equal(subchild.getOwnerDocument(), doc)

        child = doc.createTextNode("text")
        subcheck(child, None)
        parent.removeChild(child)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        subcheck(child, subchild)
        parent.removeChild(child)
        child = doc.createTextNode("text")
        root.appendChild(child)
        subcheck(child, None)
        parent.removeChild(child)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        root.appendChild(child)
        subcheck(child, subchild)
        parent.removeChild(child)

        child = doc.createTextNode("text")
        parent.appendChild(child)
        subcheck(child, None)
        parent.removeChild(child)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        parent.appendChild(child)
        subcheck(child, subchild)
        parent.removeChild(child)

        doc_2 = _make_xml_document()
        root_2 = doc.createElement("root")
        child = doc_2.createTextNode("text")
        subcheck(child, None)
        parent.removeChild(child)
        child = doc_2.createElement("elem")
        subchild = doc_2.createTextNode("text")
        child.appendChild(subchild)
        subcheck(child, subchild)
        parent.removeChild(child)
        child = doc_2.createTextNode("text")
        root_2.appendChild(child)
        subcheck(child, None)
        parent.removeChild(child)
        child = doc_2.createElement("elem")
        subchild = doc_2.createTextNode("text")
        child.appendChild(subchild)
        root_2.appendChild(child)
        subcheck(child, subchild)
        parent.removeChild(child)

        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(grandparent)
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(parent)

    grandparent = doc.createElement("grandparent")
    parent = doc.createElement("parent")
    grandparent.appendChild(parent)
    check(grandparent, parent, False)
    grandparent = doc.createElement("grandparent")
    parent = doc.createElement("parent")
    grandparent.appendChild(parent)
    root.appendChild(grandparent)
    check(root, parent, True)

    # Check appended to a document parent

    doc = _make_xml_document()
    context.check_equal(doc.hasChildNodes(), False)
    impl = doc.getImplementation()
    child = impl.createDocumentType("foo", "bar", "baz")
    doc.appendChild(child)
    context.check_equal(doc.hasChildNodes(), True)

    doc = _make_xml_document()
    context.check_equal(doc.hasChildNodes(), False)
    child = doc.createElement("elem")
    doc.appendChild(child)
    context.check_equal(doc.hasChildNodes(), True)

    doc = _make_xml_document()
    context.check_equal(doc.hasChildNodes(), False)
    child = doc.createComment("comment")
    doc.appendChild(child)
    context.check_equal(doc.hasChildNodes(), True)

    doc = _make_xml_document()
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(_make_xml_document())
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(doc.createTextNode("text"))
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(doc.createAttribute("attr"))

    doc = _make_xml_document()
    impl = doc.getImplementation()
    doctype = impl.createDocumentType("foo", "bar", "baz")
    doc.appendChild(doctype)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(impl.createDocumentType("foo", "bar", "baz"))

    doc = _make_xml_document()
    impl = doc.getImplementation()
    elem = doc.createElement("elem")
    doc.appendChild(elem)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(impl.createDocumentType("foo", "bar", "baz"))
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(doc.createElement("elem"))

    # Check appended to an element parent

    doc = _make_xml_document()
    elem = doc.createElement("parent")
    context.check_equal(elem.hasChildNodes(), False)
    child = doc.createElement("elem")
    elem.appendChild(child)
    context.check_equal(elem.hasChildNodes(), True)

    elem = doc.createElement("parent")
    context.check_equal(elem.hasChildNodes(), False)
    child = doc.createTextNode("text")
    elem.appendChild(child)
    context.check_equal(elem.hasChildNodes(), True)

    elem = doc.createElement("parent")
    context.check_equal(elem.hasChildNodes(), False)
    child = doc.createComment("comment")
    elem.appendChild(child)
    context.check_equal(elem.hasChildNodes(), True)

    elem = doc.createElement("parent")
    impl = doc.getImplementation()
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.appendChild(_make_xml_document())
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.appendChild(impl.createDocumentType("foo", "bar", "baz"))
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.appendChild(doc.createAttribute("attr"))

    # Check that no kinds of nodes can be appended to a non-parent node

    doc = _make_xml_document()
    impl = doc.getImplementation()
    def check(parent):
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(_make_xml_document())
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(doc.createElement("elem"))
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(impl.createDocumentType("foo", "bar", "baz"))
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(doc.createTextNode("text"))
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(doc.createComment("comment"))
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(doc.createAttribute("attr"))
    check(impl.createDocumentType("foo", "bar", "baz"))
    check(doc.createTextNode("test"))
    check(doc.createComment("comment"))
    check(doc.createAttribute("attr"))

    # Check that non-node arguments are properly dealt with

    class WrongImplText(archon.dom.Text):
        pass

    def check(elem):
        with context.check_raises(TypeError):
            elem.appendChild(None)
        with context.check_raises(TypeError):
            elem.appendChild(7)
        with context.check_raises(TypeError):
            elem.appendChild("foo")
        with context.check_raises(TypeError):
            elem.appendChild(WrongImplText())
    doc = _make_xml_document()
    check(doc)
    check(doc.createElement("elem"))


def test_Node_InsertBefore(context):
    doc = _make_xml_document()
    root = doc.createElement("root")

    foo = doc.createElement("foo")
    root.insertBefore(foo, None)
    context.check(root.hasChildNodes())
    context.check_equal(root.getFirstChild(), foo)
    context.check_equal(root.getLastChild(), foo)
    context.check_equal(len(root.getChildNodes()), 1)
    context.check_equal(root.getChildNodes()[0], foo)
    context.check_equal(foo.getPreviousSibling(), None)
    context.check_equal(foo.getNextSibling(), None)

    bar = doc.createElement("bar")
    root.insertBefore(bar, foo)
    context.check(root.hasChildNodes())
    context.check_equal(root.getFirstChild(), bar)
    context.check_equal(root.getLastChild(), foo)
    context.check_equal(len(root.getChildNodes()), 2)
    context.check_equal(root.getChildNodes()[0], bar)
    context.check_equal(root.getChildNodes()[1], foo)
    context.check_equal(foo.getPreviousSibling(), bar)
    context.check_equal(foo.getNextSibling(), None)
    context.check_equal(bar.getPreviousSibling(), None)
    context.check_equal(bar.getNextSibling(), foo)

    baz = doc.createTextNode("baz")
    root.insertBefore(baz, foo)
    context.check(root.hasChildNodes())
    context.check_equal(root.getFirstChild(), bar)
    context.check_equal(root.getLastChild(), foo)
    context.check_equal(len(root.getChildNodes()), 3)
    context.check_equal(root.getChildNodes()[0], bar)
    context.check_equal(root.getChildNodes()[1], baz)
    context.check_equal(root.getChildNodes()[2], foo)
    context.check_equal(foo.getPreviousSibling(), baz)
    context.check_equal(foo.getNextSibling(), None)
    context.check_equal(bar.getPreviousSibling(), None)
    context.check_equal(bar.getNextSibling(), baz)
    context.check_equal(baz.getPreviousSibling(), bar)
    context.check_equal(baz.getNextSibling(), foo)


def test_Node_RemoveChild(context):
    doc = _make_xml_document()
    root = doc.createElement("root")
    foo = doc.createElement("foo")
    root.appendChild(foo)
    bar = doc.createElement("bar")
    root.appendChild(bar)
    baz = doc.createTextNode("baz")
    root.appendChild(baz)

    root.removeChild(bar)
    context.check(root.hasChildNodes())
    context.check_equal(root.getFirstChild(), foo)
    context.check_equal(root.getLastChild(), baz)
    context.check_equal(len(root.getChildNodes()), 2)
    context.check_equal(root.getChildNodes()[0], foo)
    context.check_equal(root.getChildNodes()[1], baz)
    context.check_equal(foo.getPreviousSibling(), None)
    context.check_equal(foo.getNextSibling(), baz)
    context.check_equal(bar.getPreviousSibling(), None)
    context.check_equal(bar.getNextSibling(), None)
    context.check_equal(baz.getPreviousSibling(), foo)
    context.check_equal(baz.getNextSibling(), None)

    root.insertBefore(bar, baz)
    root.removeChild(baz)
    context.check(root.hasChildNodes())
    context.check_equal(root.getFirstChild(), foo)
    context.check_equal(root.getLastChild(), bar)
    context.check_equal(len(root.getChildNodes()), 2)
    context.check_equal(root.getChildNodes()[0], foo)
    context.check_equal(root.getChildNodes()[1], bar)
    context.check_equal(foo.getPreviousSibling(), None)
    context.check_equal(foo.getNextSibling(), bar)
    context.check_equal(bar.getPreviousSibling(), foo)
    context.check_equal(bar.getNextSibling(), None)
    context.check_equal(baz.getPreviousSibling(), None)
    context.check_equal(baz.getNextSibling(), None)

    root.removeChild(foo)
    context.check(root.hasChildNodes())
    context.check_equal(root.getFirstChild(), bar)
    context.check_equal(root.getLastChild(), bar)
    context.check_equal(len(root.getChildNodes()), 1)
    context.check_equal(root.getChildNodes()[0], bar)
    context.check_equal(foo.getPreviousSibling(), None)
    context.check_equal(foo.getNextSibling(), None)
    context.check_equal(bar.getPreviousSibling(), None)
    context.check_equal(bar.getNextSibling(), None)
    context.check_equal(baz.getPreviousSibling(), None)
    context.check_equal(baz.getNextSibling(), None)

    root.removeChild(bar)
    context.check_not(root.hasChildNodes())
    context.check_equal(root.getFirstChild(), None)
    context.check_equal(root.getLastChild(), None)
    context.check_equal(len(root.getChildNodes()), 0)
    context.check_equal(foo.getPreviousSibling(), None)
    context.check_equal(foo.getNextSibling(), None)
    context.check_equal(bar.getPreviousSibling(), None)
    context.check_equal(bar.getNextSibling(), None)
    context.check_equal(baz.getPreviousSibling(), None)
    context.check_equal(baz.getNextSibling(), None)


def test_Node_ChildNodesAsSequence(context):
    # FIXME: More needs to be tested here: len(), argument validity, ...    
    doc = _make_xml_document()
    root = doc.createElement("root")
    foo = doc.createElement("foo")
    root.appendChild(foo)
    bar = doc.createElement("bar")
    root.appendChild(bar)
    baz = doc.createTextNode("baz")
    root.appendChild(baz)

    context.check_equal(list(root.getChildNodes()), [foo, bar, baz])
    context.check_equal(list(reversed(root.getChildNodes())), [baz, bar, foo])
    context.check_in(foo, root.getChildNodes())
    context.check_not_in(root, root.getChildNodes())


def test_Document_CreateElement(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()
    doc_3 = _make_xml_document(content_type = "application/xhtml+xml")

    elem = doc_1.createElement("foo")
    context.check_equal(elem.getOwnerDocument(), doc_1)

    check_elem(context, doc_1.createElement("Foo"), None, None, "Foo")
    check_elem(context, doc_1.createElement("p:Foo"), None, None, "p:Foo")
    check_elem(context, doc_2.createElement("Foo"), "http://www.w3.org/1999/xhtml", None, "foo")
    check_elem(context, doc_2.createElement("p:Foo"), "http://www.w3.org/1999/xhtml", None, "p:foo")
    check_elem(context, doc_3.createElement("Foo"), "http://www.w3.org/1999/xhtml", None, "Foo")
    check_elem(context, doc_3.createElement("p:Foo"), "http://www.w3.org/1999/xhtml", None, "p:Foo")


def test_Document_CreateElementNS(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()

    elem = doc_1.createElementNS("ns", "foo")
    context.check_equal(elem.getOwnerDocument(), doc_1)

    check_elem(context, doc_1.createElementNS("ns", "Foo"), "ns", None, "Foo")
    check_elem(context, doc_1.createElementNS("ns", "p:Foo"), "ns", "p", "Foo")
    check_elem(context, doc_2.createElementNS("ns", "Foo"), "ns", None, "Foo")
    check_elem(context, doc_2.createElementNS("ns", "p:Foo"), "ns", "p", "Foo")

    # These must not fail
    doc_1.createElementNS("http://www.w3.org/XML/1998/namespace", "xml:foo")
    doc_1.createElementNS("http://www.w3.org/XML/1998/namespace", "p:foo")
    doc_1.createElementNS("http://www.w3.org/2000/xmlns/", "xmlns")
    doc_1.createElementNS("http://www.w3.org/2000/xmlns/", "xmlns:foo")

    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createElementNS("ns", "xml:foo")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createElementNS("ns", "xmlns")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createElementNS("ns", "xmlns:foo")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createElementNS("http://www.w3.org/2000/xmlns/", "p")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createElementNS("http://www.w3.org/2000/xmlns/", "p:foo")


def test_Document_CreateAttribute(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()
    doc_3 = _make_xml_document(content_type = "application/xhtml+xml")

    attr = doc_1.createAttribute("foo")
    context.check_equal(attr.getOwnerDocument(), doc_1)
    context.check_equal(attr.getOwnerElement(), None)

    check_elem(context, doc_1.createAttribute("Foo"), None, None, "Foo")
    check_elem(context, doc_1.createAttribute("p:Foo"), None, None, "p:Foo")
    check_elem(context, doc_2.createAttribute("Foo"), None, None, "foo")
    check_elem(context, doc_2.createAttribute("p:Foo"), None, None, "p:foo")
    check_elem(context, doc_3.createAttribute("Foo"), None, None, "Foo")
    check_elem(context, doc_3.createAttribute("p:Foo"), None, None, "p:Foo")


def test_Document_CreateAttributeNS(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()

    attr = doc_1.createAttributeNS("ns", "foo")
    context.check_equal(attr.getOwnerDocument(), doc_1)
    context.check_equal(attr.getOwnerElement(), None)

    check_elem(context, doc_1.createAttributeNS("ns", "Foo"), "ns", None, "Foo")
    check_elem(context, doc_1.createAttributeNS("ns", "p:Foo"), "ns", "p", "Foo")
    check_elem(context, doc_2.createAttributeNS("ns", "Foo"), "ns", None, "Foo")
    check_elem(context, doc_2.createAttributeNS("ns", "p:Foo"), "ns", "p", "Foo")

    # These must not fail
    doc_1.createAttributeNS("http://www.w3.org/XML/1998/namespace", "xml:foo")
    doc_1.createAttributeNS("http://www.w3.org/XML/1998/namespace", "p:foo")
    doc_1.createAttributeNS("http://www.w3.org/2000/xmlns/", "xmlns")
    doc_1.createAttributeNS("http://www.w3.org/2000/xmlns/", "xmlns:foo")

    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createAttributeNS("ns", "xml:foo")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createAttributeNS("ns", "xmlns")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createAttributeNS("ns", "xmlns:foo")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createAttributeNS("http://www.w3.org/2000/xmlns/", "p")
    with context.check_raises(archon.dom.NamespaceError):
        doc_1.createAttributeNS("http://www.w3.org/2000/xmlns/", "p:foo")


def test_Element_AttributeBasics(context):
    def check(doc, is_html):
        elem = doc.createElement("elem")
        attributes = elem.getAttributes()
        context.check_equal(attributes.getLength(), 0)
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(0), None)

        elem.setAttribute("p:foo", "1")
        context.check_equal(attributes.getLength(), 1)
        attr_1 = attributes.item(0)
        check_attr(context, attr_1, None, None, "p:foo", "1")
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(1), None)

        elem.setAttribute("p:Bar", "2")
        context.check_equal(attributes.getLength(), 2)
        context.check_equal(attributes.item(0), attr_1)
        attr_2 = attributes.item(1)
        check_attr(context, attr_2, None, None, "p:bar" if is_html else "p:Bar", "2")
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(2), None)

        elem.setAttributeNS("ns", "p:Baz", "3")
        context.check_equal(attributes.getLength(), 3)
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
    elem = doc.createElement("elem")
    context.check_not(elem.hasAttributes())
    elem.setAttribute("foo", "1")
    context.check(elem.hasAttributes())


def test_Element_GetAttributeNames(context):
    def check(doc, is_html):
        elem = doc.createElement("elem")
        elem.setAttribute("Foo", "1")
        elem.setAttribute("p:Bar", "2")
        elem.setAttributeNS("ns1", "Foo", "3")
        elem.setAttributeNS("ns2", "Foo", "4")
        elem.setAttributeNS("ns3", "p:Bar", "5")
        names = elem.getAttributeNames()
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
        elem = doc.createElement("elem")
        attributes_1 = elem.getAttributes()
        elem.setAttribute("p:foo1", "1")
        elem.setAttribute("p:foo2", "2")
        elem.setAttributeNS("ns", "p:bar1", "3")
        elem.setAttributeNS("ns", "p:bar2", "4")
        elem.setAttributeNS("ns", "p:Baz", "5")

        context.check_equal(len(attributes_1), 5)
        check_attr(context, attributes_1.item(0), None, None, "p:foo1", "1")
        check_attr(context, attributes_1.item(1), None, None, "p:foo2", "2")
        check_attr(context, attributes_1.item(2), "ns", "p", "bar1", "3")
        check_attr(context, attributes_1.item(3), "ns", "p", "bar2", "4")
        check_attr(context, attributes_1.item(4), "ns", "p", "Baz", "5")

        attributes_2 = list(attributes_1.values())
        def check_match(qualified_name, new_value, index):
            elem.setAttribute(qualified_name, new_value)
            context.check_equal(len(attributes_1), len(attributes_2))
            context.check_equal(list(attributes_1.values()), attributes_2)
            context.check_equal(attributes_1.item(index).getValue(), new_value)

        def check_mismatch(qualified_name, new_value):
            nonlocal attributes_2
            elem.setAttribute(qualified_name, new_value)
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
            elem.setAttribute("", "x")
        with context.check_raises(archon.dom.InvalidCharacterError):
            elem.setAttribute(" ", "x")
        with context.check_raises(archon.dom.InvalidCharacterError):
            elem.setAttribute("@", "x")

        # FIXME: Check argument type coercion rules (None -> "None", ...)      

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


# FIXME: Add test_Element_SetAttributeNS()                


def test_Element_ToggleAttribute(context):
    def check(doc, is_html):
        elem = doc.createElement("elem")
        attributes = elem.getAttributes()

        now_present = elem.toggleAttribute("foo")
        context.check(now_present)
        context.check_equal(len(attributes), 1)
        attr_1 = attributes.item(0)
        check_attr(context, attr_1, None, None, "foo", "")

        now_present = elem.toggleAttribute("bar")
        context.check(now_present)
        context.check_equal(len(attributes), 2)
        context.check_equal(attributes.item(0), attr_1)
        check_attr(context, attributes.item(1), None, None, "bar", "")

        now_present = elem.toggleAttribute("bar")
        context.check_not(now_present)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_1)

        now_present = elem.toggleAttribute("foo", force = True)
        context.check(now_present)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_1)

        now_present = elem.toggleAttribute("bar", force = True)
        context.check(now_present)
        context.check_equal(len(attributes), 2)
        context.check_equal(attributes.item(0), attr_1)
        attr_2 = attributes.item(1)
        check_attr(context, attr_2, None, None, "bar", "")

        now_present = elem.toggleAttribute("baz", force = False)
        context.check_not(now_present)
        context.check_equal(len(attributes), 2)
        context.check_equal(attributes.item(0), attr_1)
        context.check_equal(attributes.item(1), attr_2)

        now_present = elem.toggleAttribute("foo", force = False)
        context.check_not(now_present)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_2)

        # New element for testing matching rules
        elem = doc.createElement("elem")
        elem.setAttribute("p:foo1", "1")
        elem.setAttribute("p:foo2", "2")
        elem.setAttributeNS("ns", "p:bar1", "3")
        elem.setAttributeNS("ns", "p:bar2", "4")
        elem.setAttributeNS("ns", "p:Baz", "5")
        if is_html:
            context.check_not(elem.toggleAttribute("p:foo1"))
            context.check_not(elem.toggleAttribute("p:Foo2"))
            context.check_not(elem.toggleAttribute("p:bar1"))
            context.check_not(elem.toggleAttribute("p:Bar2"))
            context.check(elem.toggleAttribute("p:Baz"))
        else:
            context.check_not(elem.toggleAttribute("p:foo1"))
            context.check(elem.toggleAttribute("p:Foo2"))
            context.check_not(elem.toggleAttribute("p:bar1"))
            context.check(elem.toggleAttribute("p:Bar2"))
            context.check_not(elem.toggleAttribute("p:Baz"))

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_Element_SetAttributeNode(context):
    def check(doc, is_html):
        elem = doc.createElement("elem")
        attributes = elem.getAttributes()

        attr_1 = doc.createAttribute("foo")
        attr = elem.setAttributeNode(attr_1)
        context.check_is_none(attr)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_1)

        attr_2 = doc.createAttribute("foo")
        attr = elem.setAttributeNode(attr_2)
        context.check_equal(attr, attr_1)
        context.check_equal(len(attributes), 1)
        context.check_equal(attributes.item(0), attr_2)

        attr_3 = doc.createAttribute("bar")
        attr = elem.setAttributeNode(attr_3)
        context.check_is_none(attr)
        context.check_equal(len(attributes), 2)
        context.check_equal(attributes.item(0), attr_2)
        context.check_equal(attributes.item(1), attr_3)

        # FIXME: Reneable once migration of attributes has been implemented                      
        # def subcheck(doc_2, is_html_2):
        #     elem = doc.createElement("elem")
        #     attributes = elem.getAttributes()
        #     attr_1 = doc_2.createAttribute("foo")
        #     context.check_equal(attr_1.getOwnerDocument(), doc_2)
        #     attr_2 = elem.setAttributeNode(attr_1)
        #     context.check_is_none(attr_2)
        #     context.check_equal(len(attributes), 1)
        #     context.check_equal(attributes.item(0), attr_1)
        #     context.check_equal(attr_1.getOwnerDocument(), doc)

        # subcheck(_make_xml_document(), False)
        # subcheck(_make_html_document(), True)

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_Element_RemoveAttributeNode(context):
    doc = _make_xml_document()
    elem = doc.createElement("elem")
    attr = doc.createAttribute("foo")
    with context.check_raises(archon.dom.NotFoundError):
        elem.removeAttributeNode(attr)
    elem.setAttributeNode(attr)
    attr_2 = elem.removeAttributeNode(attr)
    context.check_equal(attr_2, attr)
    with context.check_raises(archon.dom.NotFoundError):
        elem.removeAttributeNode(attr)


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
        elem = doc.createElement("elem")
        attr_1 = doc.createAttributeNS("ns1", "foo")
        attr_2 = doc.createAttributeNS("ns2", "foo")
        attr_3 = doc.createAttributeNS("ns3", "Foo")
        attr_4 = doc.createAttributeNS("ns3", "Bar")
        elem.setAttributeNode(attr_1)
        elem.setAttributeNode(attr_2)
        elem.setAttributeNode(attr_3)
        elem.setAttributeNode(attr_4)
        attributes = elem.getAttributes()
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
