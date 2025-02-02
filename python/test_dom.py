import archon.test
import archon.dom


def _make_xml_document(content_type = None):
    return archon.dom.create_xml_document(content_type = content_type)

def _make_html_document(content_type = None):
    return archon.dom.create_html_document(content_type = content_type)


def check_elem(context, elem, namespace_uri, prefix, local_name):
    context.check_equal(elem.namespaceURI, namespace_uri)
    context.check_equal(elem.prefix, prefix)
    context.check_equal(elem.localName, local_name)

def check_attr(context, attr, namespace_uri, prefix, local_name, value):
    context.check_equal(attr.namespaceURI, namespace_uri)
    context.check_equal(attr.prefix, prefix)
    context.check_equal(attr.localName, local_name)
    context.check_equal(attr.value, value)


def check_children(context, parent, children):
    child_nodes = parent.childNodes
    context.check_equal(parent.firstChild, children[0] if children else None)
    context.check_equal(parent.lastChild, children[-1] if children else None)
    context.check_equal(child_nodes.length, len(children))
    prev = None
    for i, child in enumerate(children):
        context.check_equal(child, child_nodes.item(i))
        context.check_equal(child.parentNode, parent)
        if prev:
            context.check_equal(prev.nextSibling, child)
        context.check_equal(child.previousSibling, prev)
        prev = child
    if prev:
        context.check_equal(prev.nextSibling, None)


def check_nonchild(context, child):
        context.check_equal(child.parentNode, None)
        context.check_equal(child.previousSibling, None)
        context.check_equal(child.nextSibling, None)



def test_CreateXMLDocument(context):
    doc = archon.dom.create_xml_document()
    context.check_is_instance(doc, archon.dom.XMLDocument)
    context.check_equal(doc.contentType, "application/xml")
    doc = archon.dom.create_xml_document(content_type = None)
    context.check_equal(doc.contentType, "application/xml")
    doc = archon.dom.create_xml_document(content_type = "foo")
    context.check_equal(doc.contentType, "foo")
    with context.check_raises(TypeError):
        archon.dom.create_xml_document(content_type = 7)


def test_CreateHTMLDocument(context):
    doc = archon.dom.create_html_document()
    context.check_not_is_instance(doc, archon.dom.XMLDocument)
    context.check_equal(doc.contentType, "text/html")
    doc = archon.dom.create_html_document(content_type = None)
    context.check_equal(doc.contentType, "text/html")
    doc = archon.dom.create_html_document(content_type = "foo")
    context.check_equal(doc.contentType, "foo")
    with context.check_raises(TypeError):
        archon.dom.create_html_document(content_type = 7)


def test_CreateDocumentType(context):
    def check(doc, is_html):
        doctype = archon.dom.create_document_type(doc, "Foo", "Bar", "Baz")
        context.check_equal(doctype.ownerDocument, doc)

        doctype = archon.dom.create_document_type(doc, "Foo", "Bar", "Baz")
        context.check_equal(doctype.name, "Foo")
        context.check_equal(doctype.publicId, "Bar")
        context.check_equal(doctype.systemId, "Baz")

        doctype = archon.dom.create_document_type(doc, "", "", "")
        context.check_equal(doctype.name, "")
        context.check_equal(doctype.publicId, "")
        context.check_equal(doctype.systemId, "")

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
        context.check_equal(attr.ownerDocument, doc)

        attr = archon.dom.create_attribute(doc, "ns", "p", "Foo", "")
        context.check_equal(attr.namespaceURI, "ns")
        context.check_equal(attr.prefix, "p")
        context.check_equal(attr.localName, "Foo")

        attr = archon.dom.create_attribute(doc, "ns", None, "Foo", "")
        context.check_equal(attr.namespaceURI, "ns")
        context.check_equal(attr.prefix, None)
        context.check_equal(attr.localName, "Foo")

        attr = archon.dom.create_attribute(doc, None, None, "Foo", "")
        context.check_equal(attr.namespaceURI, None)
        context.check_equal(attr.prefix, None)
        context.check_equal(attr.localName, "Foo")

        attr = archon.dom.create_attribute(doc, "ns", None, "@:p:Foo", "")
        context.check_equal(attr.namespaceURI, "ns")
        context.check_equal(attr.prefix, None)
        context.check_equal(attr.localName, "@:p:Foo")

        attr = archon.dom.create_attribute(doc, None, None, "@:p:Foo", "")
        context.check_equal(attr.namespaceURI, None)
        context.check_equal(attr.prefix, None)
        context.check_equal(attr.localName, "@:p:Foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/XML/1998/namespace", "xml", "foo", "")
        context.check_equal(attr.namespaceURI, "http://www.w3.org/XML/1998/namespace")
        context.check_equal(attr.prefix, "xml")
        context.check_equal(attr.localName, "foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", "xmlns", "foo", "")
        context.check_equal(attr.namespaceURI, "http://www.w3.org/2000/xmlns/")
        context.check_equal(attr.prefix, "xmlns")
        context.check_equal(attr.localName, "foo")

        attr = archon.dom.create_attribute(doc, "http://www.w3.org/2000/xmlns/", None, "xmlns", "")
        context.check_equal(attr.namespaceURI, "http://www.w3.org/2000/xmlns/")
        context.check_equal(attr.prefix, None)
        context.check_equal(attr.localName, "xmlns")

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
        context.check_equal(attr.value, "")
        attr = archon.dom.create_attribute(doc, "ns", "p", "foo", "1")
        context.check_equal(attr.value, "1")
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", "p", "foo", None)
        with context.check_raises(TypeError):
            archon.dom.create_attribute(doc, "ns", "p", "foo", 7)

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_CreateElement(context):
    def check(doc, is_html):
        elem = archon.dom.create_element(doc, "ns", "p", "Foo", [])
        context.check_equal(elem.ownerDocument, doc)

        elem = archon.dom.create_element(doc, "ns", "p", "Foo", [])
        context.check_equal(elem.namespaceURI, "ns")
        context.check_equal(elem.prefix, "p")
        context.check_equal(elem.localName, "Foo")

        elem = archon.dom.create_element(doc, "ns", None, "Foo", [])
        context.check_equal(elem.namespaceURI, "ns")
        context.check_equal(elem.prefix, None)
        context.check_equal(elem.localName, "Foo")

        elem = archon.dom.create_element(doc, None, None, "Foo", [])
        context.check_equal(elem.namespaceURI, None)
        context.check_equal(elem.prefix, None)
        context.check_equal(elem.localName, "Foo")

        elem = archon.dom.create_element(doc, "ns", None, "@:p:Foo", [])
        context.check_equal(elem.namespaceURI, "ns")
        context.check_equal(elem.prefix, None)
        context.check_equal(elem.localName, "@:p:Foo")

        elem = archon.dom.create_element(doc, None, None, "@:p:Foo", [])
        context.check_equal(elem.namespaceURI, None)
        context.check_equal(elem.prefix, None)
        context.check_equal(elem.localName, "@:p:Foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/XML/1998/namespace", "xml", "foo", [])
        context.check_equal(elem.namespaceURI, "http://www.w3.org/XML/1998/namespace")
        context.check_equal(elem.prefix, "xml")
        context.check_equal(elem.localName, "foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", "xmlns", "foo", [])
        context.check_equal(elem.namespaceURI, "http://www.w3.org/2000/xmlns/")
        context.check_equal(elem.prefix, "xmlns")
        context.check_equal(elem.localName, "foo")

        elem = archon.dom.create_element(doc, "http://www.w3.org/2000/xmlns/", None, "xmlns", [])
        context.check_equal(elem.namespaceURI, "http://www.w3.org/2000/xmlns/")
        context.check_equal(elem.prefix, None)
        context.check_equal(elem.localName, "xmlns")

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
        attributes = elem.attributes
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

        def subcheck(doc_2, is_html_2):
            attr_7 = archon.dom.create_attribute(doc_2, "ns", None, "bar", "")
            attr_8 = archon.dom.create_attribute(doc_2, "ns", None, "bar", "")
            elem_2 = archon.dom.create_element(doc_2, "ns", "p", "foo", [ attr_8 ])
            with context.check_raises(ValueError):
                archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, attr_7, attr_3 ])
            with context.check_raises(ValueError):
                archon.dom.create_element(doc, "ns", "p", "foo", [ attr_1, attr_8, attr_3 ])

        subcheck(_make_xml_document(), False)
        subcheck(_make_html_document(), True)

    check(_make_xml_document(), False)
    check(_make_html_document(), True)


def test_DOMImplementation(context):
    doc = _make_xml_document()
    impl = doc.implementation
    context.check_is_instance(impl, archon.dom.DOMImplementation)

    doctype = impl.createDocumentType("a", "b", "c")
    context.check_is_instance(doctype, archon.dom.DocumentType)
    context.check_equal(doctype.name, "a")
    context.check_equal(doctype.publicId, "b")
    context.check_equal(doctype.systemId, "c")
    context.check_equal(doctype.ownerDocument, doc)

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
    context.check_equal(doc_2.doctype, doctype)
    # The document type node must have been migrated to the new document
    context.check_equal(doctype.ownerDocument, doc_2)
    root = doc_2.documentElement
    context.check(root)
    context.check_equal(doc_2.documentElement.namespaceURI, "ns")
    context.check_equal(doc_2.documentElement.prefix, "p")
    context.check_equal(doc_2.documentElement.localName, "foo")

    doc_3 = impl.createHTMLDocument("hello")
    context.check_is_instance(doc_3, archon.dom.Document)
    context.check_not_is_instance(doc_3, archon.dom.XMLDocument)
    context.check_equal(doc_3.doctype.name, "html")
    html = doc_3.documentElement
    context.check(html)
    context.check_equal(html.localName, "html")
    context.check_equal(len(html.childNodes), 2)
    head = html.childNodes[0]
    body = html.childNodes[1]
    context.check_equal(head.localName, "head")
    context.check_equal(body.localName, "body")
    context.check_equal(len(head.childNodes), 1)
    title = head.childNodes[0]
    context.check_equal(title.localName, "title")
    context.check_equal(len(title.childNodes), 1)
    text = title.childNodes[0]
    context.check_equal(text.data, "hello")
    context.check_equal(len(body.childNodes), 0)

    context.check(impl.hasFeature())


def test_Node_NodeType(context):
    doc = _make_xml_document()
    doctype = doc.implementation.createDocumentType("a", "b", "c")
    elem = doc.createElement("elem")
    text = doc.createTextNode("text")
    comment = doc.createComment("comment")
    attr = doc.createAttribute("attr")
    context.check_equal(doc.nodeType, archon.dom.Node.DOCUMENT_NODE)
    context.check_equal(elem.nodeType, archon.dom.Node.ELEMENT_NODE)
    context.check_equal(doctype.nodeType, archon.dom.Node.DOCUMENT_TYPE_NODE)
    context.check_equal(text.nodeType, archon.dom.Node.TEXT_NODE)
    context.check_equal(comment.nodeType, archon.dom.Node.COMMENT_NODE)
    context.check_equal(attr.nodeType, archon.dom.Node.ATTRIBUTE_NODE)


def test_Node_NodeName(context):
    doc = _make_xml_document()
    doctype = doc.implementation.createDocumentType("a", "b", "c")
    elem_1 = doc.createElement("xFoo")
    elem_2 = doc.createElementNS("ns1", "p:xFoo")
    text = doc.createTextNode("x")
    comment = doc.createComment("x")
    attr_1 = doc.createAttribute("xBar")
    attr_2 = doc.createAttributeNS("ns2", "p:xBar")
    context.check_equal(doctype.nodeName, "a")
    context.check_equal(doc.nodeName, "#document")
    context.check_equal(elem_1.nodeName, "xFoo")
    context.check_equal(elem_2.nodeName, "p:xFoo")
    context.check_equal(text.nodeName, "#text")
    context.check_equal(comment.nodeName, "#comment")
    context.check_equal(attr_1.nodeName, "xBar")
    context.check_equal(attr_2.nodeName, "p:xBar")

    doc = _make_html_document()
    elem_1 = doc.createElement("xFoo")
    elem_2 = doc.createElementNS("ns1", "p:xFoo")
    attr_1 = doc.createAttribute("xBar")
    attr_2 = doc.createAttributeNS("ns2", "p:xBar")
    context.check_equal(doc.nodeName, "#document")
    context.check_equal(elem_1.nodeName, "XFOO")
    context.check_equal(elem_2.nodeName, "p:xFoo")
    context.check_equal(attr_1.nodeName, "xbar")
    context.check_equal(attr_2.nodeName, "p:xBar")


def test_Node_ChildNodes(context):
    doc = _make_xml_document()
    parent = doc.createElement("parent")
    child_nodes = parent.childNodes
    context.check_equal(parent.firstChild, None)
    context.check_equal(parent.lastChild, None)
    context.check_equal(child_nodes.length, 0)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), None)

    child_1 = doc.createTextNode("1")
    parent.appendChild(child_1)
    context.check_equal(parent.firstChild, child_1)
    context.check_equal(parent.lastChild, child_1)
    context.check_equal(child_nodes.length, 1)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), None)

    child_2 = doc.createTextNode("2")
    parent.appendChild(child_2)
    context.check_equal(parent.firstChild, child_1)
    context.check_equal(parent.lastChild, child_2)
    context.check_equal(child_nodes.length, 2)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_2)
    context.check_equal(child_nodes.item(2), None)

    child_3 = doc.createTextNode("3")
    parent.appendChild(child_3)
    context.check_equal(parent.firstChild, child_1)
    context.check_equal(parent.lastChild, child_3)
    context.check_equal(child_nodes.length, 3)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_2)
    context.check_equal(child_nodes.item(2), child_3)
    context.check_equal(child_nodes.item(3), None)

    parent.removeChild(child_2)
    context.check_equal(parent.firstChild, child_1)
    context.check_equal(parent.lastChild, child_3)
    context.check_equal(child_nodes.length, 2)
    context.check_equal(child_nodes.item(-1), None)
    context.check_equal(child_nodes.item(0), child_1)
    context.check_equal(child_nodes.item(1), child_3)
    context.check_equal(child_nodes.item(2), None)


def test_Node_ParentNode(context):
    doc = _make_xml_document()
    context.check_equal(doc.parentNode, None)
    elem = doc.createElement("elem")
    attr = doc.createAttribute("attr")
    context.check_equal(attr.parentNode, None)
    elem.setAttributeNode(attr)
    context.check_equal(attr.parentNode, None)

    impl = doc.implementation
    doctype = impl.createDocumentType("foo", "bar", "baz")
    context.check_equal(doctype.parentNode, None)
    doc.appendChild(doctype)
    context.check_equal(doctype.parentNode, doc)
    doc.removeChild(doctype)
    context.check_equal(doctype.parentNode, None)

    def check(child):
        context.check_equal(child.parentNode, None)
        elem.appendChild(child)
        context.check_equal(child.parentNode, elem)
        elem.removeChild(child)
        context.check_equal(child.parentNode, None)

    check(doc.createElement("foo"))
    check(doc.createTextNode("foo"))
    check(doc.createComment("foo"))

    # FIXME: Reenable this when implementation is fixed                                            

    # # Check that parent remains reachable through `Node.parenNode` after application drops
    # # all its references to the parent
    # parent = doc.createElement("parent")
    # elem = doc.createElement("elem")
    # parent.appendChild(elem)
    # del parent
    # gc.collect()
    # context.check_is_not_none(elem.parentNode)

    # # Check that the existence of the parent reference does not cause a strong reference
    # # cycle
    # with disabled_gc():                       
    #     weak_elem = weakref.ref(elem)
    #     del elem
    #     context.check_is_none(weak_elem())


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

    impl = doc.implementation
    doctype = impl.createDocumentType("foo", "bar", "baz")
    context.check_not(doctype.hasChildNodes())
    text = doc.createTextNode("text")
    context.check_not(text.hasChildNodes())
    comment = doc.createComment("comment")
    context.check_not(comment.hasChildNodes())


def test_Node_PreviousNextSibling(context):
    doc = _make_xml_document()
    context.check_equal(doc.previousSibling, None)
    context.check_equal(doc.nextSibling, None)
    elem = doc.createElement("elem")
    attr_1 = doc.createAttribute("attr_1")
    attr_2 = doc.createAttribute("attr_2")
    context.check_equal(attr_1.previousSibling, None)
    context.check_equal(attr_1.nextSibling, None)
    context.check_equal(attr_2.previousSibling, None)
    context.check_equal(attr_2.nextSibling, None)
    elem.setAttributeNode(attr_1)
    elem.setAttributeNode(attr_2)
    context.check_equal(attr_1.previousSibling, None)
    context.check_equal(attr_1.nextSibling, None)
    context.check_equal(attr_2.previousSibling, None)
    context.check_equal(attr_2.nextSibling, None)

    comment_1 = doc.createComment("comment 1")
    comment_2 = doc.createComment("comment 2")

    impl = doc.implementation
    doctype = impl.createDocumentType("foo", "bar", "baz")
    context.check_equal(doctype.previousSibling, None)
    context.check_equal(doctype.nextSibling, None)
    doc.appendChild(doctype)
    context.check_equal(doctype.previousSibling, None)
    context.check_equal(doctype.nextSibling, None)
    doc.removeChild(doctype)
    doc.appendChild(comment_1)
    doc.appendChild(doctype)
    context.check_equal(doctype.previousSibling, comment_1)
    context.check_equal(doctype.nextSibling, None)
    doc.appendChild(comment_2)
    context.check_equal(doctype.previousSibling, comment_1)
    context.check_equal(doctype.nextSibling, comment_2)
    doc.removeChild(comment_1)
    context.check_equal(doctype.previousSibling, None)
    context.check_equal(doctype.nextSibling, comment_2)
    doc.removeChild(doctype)
    context.check_equal(doctype.previousSibling, None)
    context.check_equal(doctype.nextSibling, None)
    doc.removeChild(comment_2)

    def check(child):
        context.check_equal(child.previousSibling, None)
        context.check_equal(child.nextSibling, None)
        elem.appendChild(child)
        context.check_equal(child.previousSibling, None)
        context.check_equal(child.nextSibling, None)
        elem.removeChild(child)
        elem.appendChild(comment_1)
        elem.appendChild(child)
        context.check_equal(child.previousSibling, comment_1)
        context.check_equal(child.nextSibling, None)
        elem.appendChild(comment_2)
        context.check_equal(child.previousSibling, comment_1)
        context.check_equal(child.nextSibling, comment_2)
        elem.removeChild(comment_1)
        context.check_equal(child.previousSibling, None)
        context.check_equal(child.nextSibling, comment_2)
        elem.removeChild(child)
        context.check_equal(child.previousSibling, None)
        context.check_equal(child.nextSibling, None)
        elem.removeChild(comment_2)

    check(doc.createElement("foo"))
    check(doc.createTextNode("foo"))
    check(doc.createComment("foo"))


def test_Node_AppendChild(context):
    doc = _make_xml_document()
    root = doc.createElement("root")

    def check(grandparent, parent):
        check_children(context, parent, [])

        child_1 = doc.createComment("comment 1")
        child = parent.appendChild(child_1)
        context.check_equal(child, child_1)
        check_children(context, parent, [ child_1 ])

        child_2 = doc.createComment("comment 2")
        child = parent.appendChild(child_2)
        context.check_equal(child, child_2)
        check_children(context, parent, [ child_1, child_2 ])

        child_3 = doc.createComment("comment 3")
        child = parent.appendChild(child_3)
        context.check_equal(child, child_3)
        check_children(context, parent, [ child_1, child_2, child_3 ])

        parent.removeChild(child_3)
        parent.removeChild(child_2)
        check_children(context, parent, [ child_1 ])

        def subcheck(child, subchild):
            parent.appendChild(child)
            check_children(context, parent, [ child_1, child ])
            context.check_equal(child.ownerDocument, doc)
            if subchild:
                context.check_equal(subchild.parentNode, child)
                context.check_equal(subchild.ownerDocument, doc)

            parent.removeChild(child)
            check_children(context, parent, [ child_1 ])

        child = doc.createComment("comment")
        subcheck(child, None)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        subcheck(child, subchild)
        child = doc.createComment("comment")
        root.appendChild(child)
        subcheck(child, None)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        root.appendChild(child)
        subcheck(child, subchild)

        child = doc.createComment("comment")
        parent.appendChild(child)
        subcheck(child, None)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        parent.appendChild(child)
        subcheck(child, subchild)

        doc_2 = _make_xml_document()
        root_2 = doc.createElement("root")
        child = doc_2.createComment("comment")
        subcheck(child, None)
        child = doc_2.createElement("elem")
        subchild = doc_2.createTextNode("text")
        child.appendChild(subchild)
        subcheck(child, subchild)
        child = doc_2.createComment("comment")
        root_2.appendChild(child)
        subcheck(child, None)
        child = doc_2.createElement("elem")
        subchild = doc_2.createTextNode("text")
        child.appendChild(subchild)
        root_2.appendChild(child)
        subcheck(child, subchild)

        if grandparent:
            with context.check_raises(archon.dom.HierarchyRequestError):
                parent.appendChild(grandparent)
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.appendChild(parent)

    grandparent = None
    parent = doc
    check(grandparent, parent)
    grandparent = doc.createElement("grandparent")
    parent = doc.createElement("parent")
    grandparent.appendChild(parent)
    check(grandparent, parent)
    grandparent = doc.createElement("grandparent")
    parent = doc.createElement("parent")
    grandparent.appendChild(parent)
    root.appendChild(grandparent)
    check(root, parent)

    # Check append to document

    doc = _make_xml_document()
    context.check_equal(doc.hasChildNodes(), False)
    impl = doc.implementation
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
    impl = doc.implementation
    doctype = impl.createDocumentType("foo", "bar", "baz")
    doc.appendChild(doctype)
    doc.appendChild(doctype) # Must remove, then re-append
    context.check_equal(doc.childNodes.length, 1)
    context.check_equal(doc.childNodes.item(0), doctype)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(impl.createDocumentType("foo", "bar", "baz"))

    doc = _make_xml_document()
    impl = doc.implementation
    elem = doc.createElement("elem")
    doc.appendChild(elem)
    doc.appendChild(elem) # Remove, then re-append
    context.check_equal(doc.childNodes.length, 1)
    context.check_equal(doc.childNodes.item(0), elem)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(impl.createDocumentType("foo", "bar", "baz"))
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.appendChild(doc.createElement("elem"))

    # Check append to element

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
    impl = doc.implementation
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.appendChild(_make_xml_document())
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.appendChild(impl.createDocumentType("foo", "bar", "baz"))
    with context.check_raises(archon.dom.HierarchyRequestError):
        elem.appendChild(doc.createAttribute("attr"))

    # Check that no kinds of nodes can be appended to a non-parent node

    doc = _make_xml_document()
    impl = doc.implementation
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
    check(doc.createTextNode("text"))
    check(doc.createComment("comment"))
    check(doc.createAttribute("attr"))

    # Check that non-node arguments are properly dealt with

    class WrongImplComment(archon.dom.Comment):
        pass

    def check(parent):
        with context.check_raises(TypeError):
            parent.appendChild(None)
        with context.check_raises(TypeError):
            parent.appendChild(7)
        with context.check_raises(TypeError):
            parent.appendChild("foo")
        with context.check_raises(TypeError):
            parent.appendChild(WrongImplComment())
    doc = _make_xml_document()
    check(doc)
    check(doc.createElement("elem"))


def test_Node_InsertBefore(context):
    doc = _make_xml_document()
    root = doc.createElement("root")

    def check(grandparent, parent):
        check_children(context, parent, [])

        child_1 = doc.createComment("comment 1")
        child = parent.insertBefore(child_1, None)
        context.check_equal(child, child_1)
        check_children(context, parent, [ child_1 ])

        child_2 = doc.createComment("comment 2")
        child = parent.insertBefore(child_2, None)
        context.check_equal(child, child_2)
        check_children(context, parent, [ child_1, child_2 ])

        child_3 = doc.createComment("comment 3")
        child = parent.insertBefore(child_3, None)
        context.check_equal(child, child_3)
        check_children(context, parent, [ child_1, child_2, child_3 ])

        parent.removeChild(child_1)
        parent.removeChild(child_2)
        check_children(context, parent, [ child_3 ])
        check_nonchild(context, child_1)
        check_nonchild(context, child_2)

        child = parent.insertBefore(child_1, child_3)
        context.check_equal(child, child_1)
        check_children(context, parent, [ child_1, child_3 ])

        child = parent.insertBefore(child_2, child_3)
        context.check_equal(child, child_2)
        check_children(context, parent, [ child_1, child_2, child_3 ])

        child = parent.insertBefore(child_2, child_1)
        context.check_equal(child, child_2)
        check_children(context, parent, [ child_2, child_1, child_3 ])

        child = parent.insertBefore(child_1, child_2)
        context.check_equal(child, child_1)
        check_children(context, parent, [ child_1, child_2, child_3 ])

        child = parent.insertBefore(child_2, child_2)
        context.check_equal(child, child_2)
        check_children(context, parent, [ child_1, child_2, child_3 ])

        child = parent.insertBefore(child_2, child_3)
        context.check_equal(child, child_2)
        check_children(context, parent, [ child_1, child_2, child_3 ])

        parent.removeChild(child_3)
        parent.removeChild(child_2)
        check_children(context, parent, [ child_1 ])
        check_nonchild(context, child_2)
        check_nonchild(context, child_3)

        def subcheck(child, subchild):
            parent.insertBefore(child, None)
            check_children(context, parent, [ child_1, child ])
            context.check_equal(child.ownerDocument, doc)
            if subchild:
                context.check_equal(subchild.parentNode, child)
                context.check_equal(subchild.ownerDocument, doc)

            parent.removeChild(child)
            check_children(context, parent, [ child_1 ])
            check_nonchild(context, child)

            parent.insertBefore(child, child_1)
            check_children(context, parent, [ child, child_1 ])
            context.check_equal(child.ownerDocument, doc)
            if subchild:
                context.check_equal(subchild.parentNode, child)
                context.check_equal(subchild.ownerDocument, doc)

            parent.removeChild(child)
            check_children(context, parent, [ child_1 ])

        child = doc.createComment("comment")
        subcheck(child, None)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        subcheck(child, subchild)
        child = doc.createComment("comment")
        root.appendChild(child)
        subcheck(child, None)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        root.appendChild(child)
        subcheck(child, subchild)

        child = doc.createComment("comment")
        parent.appendChild(child)
        subcheck(child, None)
        child = doc.createElement("elem")
        subchild = doc.createTextNode("text")
        child.appendChild(subchild)
        parent.appendChild(child)
        subcheck(child, subchild)

        doc_2 = _make_xml_document()
        root_2 = doc.createElement("root")
        child = doc_2.createComment("comment")
        subcheck(child, None)
        child = doc_2.createElement("elem")
        subchild = doc_2.createTextNode("text")
        child.appendChild(subchild)
        subcheck(child, subchild)
        child = doc_2.createComment("comment")
        root_2.appendChild(child)
        subcheck(child, None)
        child = doc_2.createElement("elem")
        subchild = doc_2.createTextNode("text")
        child.appendChild(subchild)
        root_2.appendChild(child)
        subcheck(child, subchild)

        with context.check_raises(archon.dom.NotFoundError):
            parent.insertBefore(child_2, child_3)
        if grandparent:
            with context.check_raises(archon.dom.HierarchyRequestError):
                parent.insertBefore(grandparent, None)
            with context.check_raises(archon.dom.HierarchyRequestError):
                parent.insertBefore(grandparent, child_1)
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.insertBefore(parent, None)
        with context.check_raises(archon.dom.HierarchyRequestError):
            parent.insertBefore(parent, child_1)

    grandparent = None
    parent = doc
    check(grandparent, parent)
    grandparent = doc.createElement("grandparent")
    parent = doc.createElement("parent")
    grandparent.appendChild(parent)
    check(grandparent, parent)
    grandparent = doc.createElement("grandparent")
    parent = doc.createElement("parent")
    grandparent.appendChild(parent)
    root.appendChild(grandparent)
    check(root, parent)

    # Check insertion into document

    doc = _make_xml_document()
    impl = doc.implementation
    child = doc.createComment("child")
    doc.appendChild(child)

    def check(node):
        check_children(context, doc, [ child ])
        doc.insertBefore(node, None)
        check_children(context, doc, [ child, node ])
        doc.removeChild(node)
        check_children(context, doc, [ child ])
        doc.insertBefore(node, child)
        check_children(context, doc, [ node, child ])
        doc.removeChild(node)

    check(impl.createDocumentType("foo", "bar", "baz"))
    check(doc.createElement("elem"))
    check(doc.createComment("comment"))

    def check(node):
        with context.check_raises(archon.dom.HierarchyRequestError):
            doc.insertBefore(node, None)
        with context.check_raises(archon.dom.HierarchyRequestError):
            doc.insertBefore(node, child)

    check(_make_xml_document())
    check(doc.createTextNode("text"))
    check(doc.createAttribute("attr"))

    doctype = impl.createDocumentType("foo", "bar", "baz")
    doc.appendChild(doctype)
    check_children(context, doc, [ child, doctype ])
    doc.insertBefore(doctype, None)
    check_children(context, doc, [ child, doctype ])
    doc.insertBefore(doctype, doctype)
    check_children(context, doc, [ child, doctype ])
    doc.insertBefore(doctype, child)
    check_children(context, doc, [ doctype, child ])
    doc.insertBefore(doctype, child)
    check_children(context, doc, [ doctype, child ])
    doc.insertBefore(doctype, doctype)
    check_children(context, doc, [ doctype, child ])
    elem = doc.createElement("elem")
    doc.appendChild(elem)
    check_children(context, doc, [ doctype, child, elem ])
    doc.insertBefore(doctype, elem)
    check_children(context, doc, [ child, doctype, elem ])
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.insertBefore(doctype, None)
    doctype_2 = impl.createDocumentType("foo", "bar", "baz")
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.insertBefore(doctype_2, None)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.insertBefore(doctype_2, elem)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.insertBefore(doctype_2, doctype)

    doc.removeChild(doctype)
    check_children(context, doc, [ child, elem ])
    doc.insertBefore(elem, None)
    check_children(context, doc, [ child, elem ])
    doc.insertBefore(elem, elem)
    check_children(context, doc, [ child, elem ])
    doc.insertBefore(elem, child)
    check_children(context, doc, [ elem, child ])
    doc.insertBefore(elem, child)
    check_children(context, doc, [ elem, child ])
    doc.insertBefore(elem, elem)
    check_children(context, doc, [ elem, child ])
    doc.removeChild(child)
    doc.removeChild(elem)
    doc.appendChild(doctype)
    doc.appendChild(child)
    doc.appendChild(elem)
    check_children(context, doc, [ doctype, child, elem ])
    doc.insertBefore(elem, child)
    check_children(context, doc, [ doctype, elem, child ])
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.insertBefore(elem, doctype)
    elem_2 = doc.createElement("elem2")
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.insertBefore(elem_2, None)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.insertBefore(elem_2, elem)
    with context.check_raises(archon.dom.HierarchyRequestError):
        doc.insertBefore(elem_2, doctype)

    # Check insertion into element

    doc = _make_xml_document()
    impl = doc.implementation
    elem = doc.createElement("elem")
    child_nodes = elem.childNodes
    child = doc.createComment("child")
    elem.appendChild(child)

    def check(node):
        check_children(context, elem, [ child ])
        elem.insertBefore(node, None)
        check_children(context, elem, [ child, node ])
        elem.removeChild(node)
        check_children(context, elem, [ child ])
        elem.insertBefore(node, child)
        check_children(context, elem, [ node, child ])
        elem.removeChild(node)

    check(doc.createElement("elem"))
    check(doc.createTextNode("text"))
    check(doc.createComment("comment"))

    def check(node):
        with context.check_raises(archon.dom.HierarchyRequestError):
            elem.insertBefore(node, None)
        with context.check_raises(archon.dom.HierarchyRequestError):
            elem.insertBefore(node, child)

    check(_make_xml_document())
    check(impl.createDocumentType("foo", "bar", "baz"))
    check(doc.createAttribute("attr"))

    # Check that no kinds of nodes can be inserted into a non-parent node

    doc = _make_xml_document()
    impl = doc.implementation
    def check(parent):
        def subcheck(node):
            with context.check_raises(archon.dom.HierarchyRequestError):
                parent.insertBefore(node, None)
        subcheck(_make_xml_document())
        subcheck(doc.createElement("elem"))
        subcheck(impl.createDocumentType("foo", "bar", "baz"))
        subcheck(doc.createTextNode("text"))
        subcheck(doc.createComment("comment"))
        subcheck(doc.createAttribute("attr"))
    check(impl.createDocumentType("foo", "bar", "baz"))
    check(doc.createTextNode("text"))
    check(doc.createComment("comment"))
    check(doc.createAttribute("attr"))

    # Check that non-node arguments are properly dealt with

    class WrongImplComment(archon.dom.Comment):
        pass

    def check(parent, child):
        def subcheck(node):
            with context.check_raises(TypeError):
                parent.insertBefore(node, None)
            with context.check_raises(TypeError):
                parent.insertBefore(node, child)
        subcheck(None)
        subcheck(7)
        subcheck("foo")
        subcheck(WrongImplComment())
    doc = _make_xml_document()
    child = doc.createComment("child")
    doc.appendChild(child)
    check(doc, child)
    elem = doc.createElement("elem")
    elem.appendChild(child)
    check(elem, child)

    doc = _make_xml_document()
    node = doc.createComment("node")
    def check(parent):
        def subcheck(child):
            with context.check_raises(TypeError):
                parent.insertBefore(node, child)
        subcheck(7)
        subcheck("foo")
        subcheck(WrongImplComment())
    doc = _make_xml_document()
    check(doc)
    elem = doc.createElement("elem")
    check(elem)


def test_Node_RemoveChild(context):
    doc = _make_xml_document()
    root = doc.createElement("root")
    foo = doc.createElement("foo")
    root.appendChild(foo)
    bar = doc.createElement("bar")
    root.appendChild(bar)
    baz = doc.createTextNode("baz")
    root.appendChild(baz)

    child = root.removeChild(bar)
    context.check_equal(child, bar)
    context.check(root.hasChildNodes())
    context.check_equal(root.firstChild, foo)
    context.check_equal(root.lastChild, baz)
    context.check_equal(len(root.childNodes), 2)
    context.check_equal(root.childNodes[0], foo)
    context.check_equal(root.childNodes[1], baz)
    context.check_equal(foo.previousSibling, None)
    context.check_equal(foo.nextSibling, baz)
    context.check_equal(bar.previousSibling, None)
    context.check_equal(bar.nextSibling, None)
    context.check_equal(baz.previousSibling, foo)
    context.check_equal(baz.nextSibling, None)

    root.insertBefore(bar, baz)
    child = root.removeChild(baz)
    context.check_equal(child, baz)
    context.check(root.hasChildNodes())
    context.check_equal(root.firstChild, foo)
    context.check_equal(root.lastChild, bar)
    context.check_equal(len(root.childNodes), 2)
    context.check_equal(root.childNodes[0], foo)
    context.check_equal(root.childNodes[1], bar)
    context.check_equal(foo.previousSibling, None)
    context.check_equal(foo.nextSibling, bar)
    context.check_equal(bar.previousSibling, foo)
    context.check_equal(bar.nextSibling, None)
    context.check_equal(baz.previousSibling, None)
    context.check_equal(baz.nextSibling, None)

    child = root.removeChild(foo)
    context.check_equal(child, foo)
    context.check(root.hasChildNodes())
    context.check_equal(root.firstChild, bar)
    context.check_equal(root.lastChild, bar)
    context.check_equal(len(root.childNodes), 1)
    context.check_equal(root.childNodes[0], bar)
    context.check_equal(foo.previousSibling, None)
    context.check_equal(foo.nextSibling, None)
    context.check_equal(bar.previousSibling, None)
    context.check_equal(bar.nextSibling, None)
    context.check_equal(baz.previousSibling, None)
    context.check_equal(baz.nextSibling, None)

    child = root.removeChild(bar)
    context.check_equal(child, bar)
    context.check_not(root.hasChildNodes())
    context.check_equal(root.firstChild, None)
    context.check_equal(root.lastChild, None)
    context.check_equal(len(root.childNodes), 0)
    context.check_equal(foo.previousSibling, None)
    context.check_equal(foo.nextSibling, None)
    context.check_equal(bar.previousSibling, None)
    context.check_equal(bar.nextSibling, None)
    context.check_equal(baz.previousSibling, None)
    context.check_equal(baz.nextSibling, None)


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

    context.check_equal(list(root.childNodes), [foo, bar, baz])
    context.check_equal(list(reversed(root.childNodes)), [baz, bar, foo])
    context.check_in(foo, root.childNodes)
    context.check_not_in(root, root.childNodes)


def test_Document_CreateElement(context):
    doc_1 = _make_xml_document()
    doc_2 = _make_html_document()
    doc_3 = _make_xml_document(content_type = "application/xhtml+xml")

    elem = doc_1.createElement("foo")
    context.check_equal(elem.ownerDocument, doc_1)

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
    context.check_equal(elem.ownerDocument, doc_1)

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
    context.check_equal(attr.ownerDocument, doc_1)
    context.check_equal(attr.ownerElement, None)

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
    context.check_equal(attr.ownerDocument, doc_1)
    context.check_equal(attr.ownerElement, None)

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


def test_Element_Attributes(context):
    def check(doc, is_html):
        elem = doc.createElement("elem")
        attributes = elem.attributes
        context.check_equal(attributes.length, 0)
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(0), None)

        elem.setAttribute("p:foo", "1")
        context.check_equal(attributes.length, 1)
        attr_1 = attributes.item(0)
        check_attr(context, attr_1, None, None, "p:foo", "1")
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(1), None)

        elem.setAttribute("p:Bar", "2")
        context.check_equal(attributes.length, 2)
        context.check_equal(attributes.item(0), attr_1)
        attr_2 = attributes.item(1)
        check_attr(context, attr_2, None, None, "p:bar" if is_html else "p:Bar", "2")
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(2), None)

        elem.setAttributeNS("ns", "p:Baz", "3")
        context.check_equal(attributes.length, 3)
        context.check_equal(attributes.item(0), attr_1)
        context.check_equal(attributes.item(1), attr_2)
        attr_3 = attributes.item(2)
        check_attr(context, attr_3, "ns", "p", "Baz", "3")
        context.check_equal(attributes.item(-1), None)
        context.check_equal(attributes.item(3), None)

        # FIXME: Test rest of methods of NamedNodeMap    

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
        attributes_1 = elem.attributes
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
            context.check_equal(attributes_1.item(index).value, new_value)

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
        attributes = elem.attributes

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
        attributes = elem.attributes

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

        def subcheck(doc_2, is_html_2):
            elem = doc.createElement("elem")
            attributes = elem.attributes
            attr_1 = doc_2.createAttribute("foo")
            context.check_equal(attr_1.ownerDocument, doc_2)
            attr_2 = elem.setAttributeNode(attr_1)
            context.check_is_none(attr_2)
            context.check_equal(len(attributes), 1)
            context.check_equal(attributes.item(0), attr_1)
            context.check_equal(attr_1.ownerDocument, doc)

        subcheck(_make_xml_document(), False)
        subcheck(_make_html_document(), True)

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
        attributes = elem.attributes
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
