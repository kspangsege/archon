import weakref
import html.parser
import archon.core
import archon.log
import archon.html_parser


def _generate_tests(add_test):
    n = 0
    def test(string, expect_parse_error, expected):
        nonlocal n
        n += 1
        name = "Parse%s" % n
        descr = "%r --> %r" % (string, expected)
        def func():
            return _test(string, expected, expect_parse_error)
        add_test(name, descr, func)

    def test_head(string, expect_parse_error, expected):
        test("<!doctype html><html><head>%s</head><body></body></html>" % string, expect_parse_error,
             "<!doctype html><html><head>%s</head><body></body></html>" % expected)

    def test_body(string, expect_parse_error, expected):
        test("<!doctype html><html><head></head><body>%s</body></html>" % string, expect_parse_error,
             "<!doctype html><html><head></head><body>%s</body></html>" % expected)

    test("", True,
         "<html><head/><body/></html>")

    test(" ", True,
         "<html><head/><body/></html>")

    test("1", True,
         "<html><head/><body>1</body></html>")

    test(" 1", True,
         "<html><head/><body>1</body></html>")

    test("<!doctype html>", False,
         "<!doctype html><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLIC 'foo'>", False,
         "<!DOCTYPE html PUBLIC 'foo'><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLIC 'foo' 'bar'>", False,
         "<!DOCTYPE html PUBLIC 'foo' 'bar'><html><head/><body/></html>")

    test("<!DOCTYPE html SYSTEM 'bar'>", False,
         "<!DOCTYPE html SYSTEM 'bar'><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLIC>", True,
         "<!DOCTYPE html><html><head/><body/></html>")

    test("<!DOCTYPE html SYSTEM>", True,
         "<!DOCTYPE html><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLICfoo>", True,
         "<!DOCTYPE html><html><head/><body/></html>")

    test("<!DOCTYPE html SYSTEMbar>", True,
         "<!DOCTYPE html><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLIC foo>", True,
         "<!DOCTYPE html><html><head/><body/></html>")

    test("<!DOCTYPE html SYSTEM bar>", True,
         "<!DOCTYPE html><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLIC 'foo'bar>", True,
         "<!DOCTYPE html PUBLIC 'foo'><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLIC 'foo' bar>", True,
         "<!DOCTYPE html PUBLIC 'foo'><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLIC 'foo''bar'>", True,
         "<!DOCTYPE html PUBLIC 'foo' 'bar'><html><head/><body/></html>")

    test("<!DOCTYPE html PUBLIC 'foo' 'bar' 'baz'>", True,
         "<!DOCTYPE html PUBLIC 'foo' 'bar'><html><head/><body/></html>")

    test("<!DOCTYPE html SYSTEM 'foo' 'baz'>", True,
         "<!DOCTYPE html SYSTEM 'foo'><html><head/><body/></html>")

    test("<!doctype html>1", False,
         "<!doctype html><html><head/><body>1</body></html>")

    test("1<!doctype html>", True,
         "<html><head/><body>1</body></html>")

    test("<!doctype html>1<!doctype html>", True,
         "<!doctype html><html><head/><body>1</body></html>")

    test("<html><!doctype html></html>", True,
         "<html><head/><body></body></html>")

    test("<!doctype html><html><!doctype html></html>", True,
         "<!doctype html><html><head/><body></body></html>")

    test("<!-- comment --><!doctype html>", False,
         "<!-- comment --><!doctype html><html><head/><body/></html>")

    test("<!doctype html><!-- comment --><html></html>", False,
         "<!doctype html><!-- comment --><html><head/><body/></html>")

    test("<!doctype html><html><!-- comment --></html>", False,
         "<!doctype html><html><!-- comment --><head></head><body/></html>")

    test("<!doctype html><html></html><!-- comment -->", False,
         "<!doctype html><html><head/><body/></html><!-- comment -->")

    test("<!doctype html><html><head><noscript> x</noscript></head></html>", True,
         "<!doctype html><html><head><noscript> </noscript></head><body>x</body></html>")

    test("<!doctype html><frameset><frame></frameset>", False,
         "<!doctype html><html><head/><frameset><frame/></frameset></html>")

    test("<!doctype html><html><frameset><frame></frameset></html>", False,
         "<!doctype html><html><head/><frameset><frame/></frameset></html>")

    test_head("<base href=\"x\">", False,
              "<base href=\"x\"/>")

    test_head("<base href=\"x\"/>", False,
              "<base href=\"x\"/>")

    test_head("<noscript></noscript>", False,
              "<noscript/>")

    test_head("<noscript><link></noscript>", False,
              "<noscript><link/></noscript>")

    test_body("1<!-- comment -->2", False,
              "1<!-- comment -->2")

    test_body("1<i>2</i>3", False,
              "1<i>2</i>3")

    test_body("1<i>2<b>3</b>4</i>5", False,
              "1<i>2<b>3</b>4</i>5")

    test_body("1<i>2<b foo=7>3</i>4</b>5", True,
              "1<i>2<b foo=7>3</b></i><b foo=7>4</b>5")

    test_body("1<i>2<p>3</p>4</i>5", False,
              "1<i>2<p>3</p>4</i>5")

    test_body("1<i foo=7>2<p>3</i>4</p>5", True,
              "1<i foo=7>2</i><p><i foo=7>3</i>4</p>5")

    test_body("1<p>2<i foo=7>3</p>4</i>5", True,
              "1<p>2<i foo=7>3</i></p><i foo=7>4</i>5")

    # "1<i>2<b></i>" --> "1<i>2<b></b></i>"
    # "1<i>2<b>3</i>" --> "1<i>2<b>3</b></i>"
    # "1<i>2<b>3</i>4" --> "1<i>2<b>3</b></i><b>4</b>"

    # "1<i>2<p>3</i>4" --> "1<i>2</i><p><i>3</i>4</p>"

    # "1<i>2<b>3<p>4</i>5</b>6" --> "1<i>2<b>3</b></i><p><i><b>4</b></i><b>5</b>6</p>"                 

    # "1<i>2<div><p></i>3</div>4" --> "1<i>2</i><div><p>3</p></div>4"
    # "1<i>2<div>3<p></i>4</div>5" --> "1<i>2</i><div><i>3</i><p>4</p></div>5"
    # "1<i>2<div>3<p>4</i>5</div>6" --> "1<i>2</i><div><i>3</i><p><i>4</i>5</p></div>6"

    # "1<i>2<b>3<div>4<p>5</i>6</b>7</div>8" --> "1<i>2<b>3</b></i><div><i><b>4</b></i><p><i><b>5</b></i><b>6</b>7</p></div>8"

    # "1<i>2<b>3<div>4<p>5</i>6</div>7</b>8" --> "1<i>2<b>3</b></i><div><i><b>4</b></i><p><i><b>5</b></i><b>6</b></p></div><b>7</b>8"

    test_body("</i>", True,
              "")

    test_body("<i foo=1><i foo=2></i></i>", False,
              "<i foo=1><i foo=2 /></i>")

    test_body("<span></span>", False,
              "<span/>")

    test_body("<span><i></span>", True,
              "<span><i/></span>")

    # FIXME: Pass argument (`BreakEndTag`) to be included in test name   
    test_body("1</br>2", True,
              "1<br/>2")

    test_body("1<br>2</br>3", True,
              "1<br/>2<br/>3")

    test_body("1<span>2<p>3</span>4", True,
              "1<span>2<p>34</p></span>")

    test_body("1<noscript>2<p>3</noscript>4", True,
              "1<noscript>2<p>34</p></noscript>")

    # FIXME: Pass argument (`SVG`) to be included in test name   
    test_body("<svg><g><path/></g></svg>", False,
              "<svg xmlns=\"http://www.w3.org/2000/svg\"><g><path/></g></svg>")


# Bridge to the Archon testing framework
def _run_tests():
    import archon.test
    tests = []
    def add_test(name, descr, func):
        def test(context):
            context.check(func())
        tests.append(archon.test.Test(name, descr, test))
    _generate_tests(add_test)
    archon.test.run(tests)


# Bridge to Python's native testing framework
def load_tests(loader, standard_tests, pattern):
    import unittest
    tests = unittest.TestSuite()
    def add_test(name, descr, func):
        class Test(unittest.TestCase):
            pass
        method_name = "test%s" % name
        def method_func(self):
            self.assertTrue(func())
        method_func.__doc__ = descr
        setattr(Test, method_name, method_func)
        tests.addTest(Test(methodName = method_name))
    _generate_tests(add_test)
    return tests


def _test(string, expected, expect_parse_error):
    parse_errors_occurred = False
    def parse_string(string):
        class ErrorHandler(archon.html_parser.SimpleErrorHandler):
            def __init__(self, logger):
                archon.html_parser.SimpleErrorHandler.__init__(self, "<input>", logger)
            def error(self, location, message):
                nonlocal parse_errors_occurred
                archon.html_parser.SimpleErrorHandler.error(self, location, message)
                parse_errors_occurred = True
        parser = archon.html_parser.Parser()
        source = archon.html_parser.StringSource(string)
        retaining_source = archon.html_parser.RetainingSource(source)
        document = archon.dom.create_html_document()
        tree_builder = archon.html_parser.TreeBuilder(document)
        null_logger = archon.log.NullLogger()
        logger = null_logger if expect_parse_error else archon.log.FileLogger()
        error_handler_1 = ErrorHandler(logger)
        error_handler_2 = archon.html_parser.ContextShowingErrorHandler(retaining_source, error_handler_1, logger)
        parser.parse(retaining_source, document, tree_builder, error_handler_2, null_logger)
        return document

    def parse_expected(string):
        document = archon.dom.create_html_document()
        stack = []
        def append(node):
            parent = stack[-1] if stack else document
            parent.appendChild(node)
        class Tokenizer(html.parser.HTMLParser):
            def handle_decl(self, decl):
                name, public_id, system_id = parse_expected_doctype(decl)
                if name is None:
                    name = ""
                if public_id is None:
                    public_id = ""
                if system_id is None:
                    system_id = ""
                append(archon.dom.create_document_type(document, name, public_id, system_id))
            def handle_data(self, data):
                append(document.createTextNode(data))
            def handle_starttag(self, tag, attrs):
                namespace_uri = "http://www.w3.org/1999/xhtml"
                if stack:
                    namespace_uri = stack[-1].getNamespaceURI()
                prefix = None
                attributes = []
                for name, value in attrs:
                    if name == "xmlns":
                        namespace_uri = value
                        continue
                    namespace_uri_2 = None
                    prefix_2 = None
                    attr = archon.dom.create_attribute(document, namespace_uri_2, prefix_2, name, value)
                    attributes.append(attr)
                elem = archon.dom.create_element(document, namespace_uri, prefix, tag, attributes)
                append(elem)
                stack.append(elem)
            def handle_endtag(self, tag):
                assert tag == stack[-1].getLocalName()
                stack.pop()
            def handle_comment(self, data):
                append(document.createComment(data))
        tokenizer = Tokenizer()
        tokenizer.feed(string)
        assert not stack
        return document

    def parse_expected_doctype(decl):
        i = 0
        n = len(decl)
        def begins_with(prefix):
            j = i + len(prefix)
            return decl[i:j].lower() == prefix.lower()
        def try_consume(prefix):
            nonlocal i
            if begins_with(prefix):
                i += len(prefix)
                return True
            return False
        def skip_space():
            nonlocal i
            while i < n and decl[i] == " ":
                i += 1
        def skip_nonspace():
            nonlocal i
            while i < n and decl[i] != " ":
                i += 1
        name = None
        public_id = None
        system_id = None
        def consume_public_id():
            nonlocal i, public_id
            assert i < n
            mark = decl[i]
            assert mark in "'\""
            i += 1
            j = decl.find(mark, i)
            assert j != -1
            public_id = decl[i:j]
            i = j + 1
        def consume_system_id():
            nonlocal i, system_id
            assert i < n
            mark = decl[i]
            assert mark in "'\""
            i += 1
            j = decl.find(mark, i)
            assert j != -1
            system_id = decl[i:j]
            i = j + 1
            skip_space()
            assert i == n
        def parse():
            nonlocal i, name
            assert try_consume("DOCTYPE")
            skip_space()
            assert i < n
            i_0 = i
            i += 1
            skip_nonspace()
            name = decl[i_0:i].lower()
            skip_space()
            if try_consume("PUBLIC"):
                skip_space()
                assert i < n
                consume_public_id()
                skip_space()
                if i < n:
                    consume_system_id()
                return
            if try_consume("SYSTEM"):
                skip_space()
                assert i < n
                consume_system_id()
                return
            assert i == n
        parse()
        return name, public_id, system_id

    def format_path(path):
        path_2 = []
        parent = document
        for i in path:
            child_nodes = parent.getChildNodes()
            node = child_nodes[i]
            assert isinstance(node, archon.dom.Element)
            elem = node
            n = 0
            for node in child_nodes:
                if not isinstance(node, archon.dom.Element):
                    continue
                elem_2 = node
                if elem_2.getNamespaceURI() != elem.getNamespaceURI():
                    continue
                if elem_2.getLocalName() != elem.getLocalName():
                    continue
                n += 1
            assert n >= 1
            segment = _format_element_name(elem)
            if n > 1:
                segment = "%s[%s]" % (segment, i + 1)
            path_2.append(segment)
            parent = elem
        return "/%s" % "/".join(path_2)

    def compare_children(node, expected_node, path, children, index):
        def type_mismatch():
            path_error("Node type mismatch %s: %s vs %s" % (format_context_qualifier(children, index),
                                                            _format_node(node), _format_node(expected_node)), path)
        if isinstance(node, archon.dom.DocumentType):
            if not isinstance(expected_node, archon.dom.DocumentType):
                type_mismatch()
                return
            name_1 = node.getName()
            name_2 = expected_node.getName()
            if name_1 != name_2:
                path_error("Doctype name mismatch %s: %r vs %r" % (format_context_qualifier(children, index),
                                                                   name_1, name_2), path)
            public_id_1 = node.getPublicId()
            public_id_2 = expected_node.getPublicId()
            if public_id_1 != public_id_2:
                path_error("Doctype public identifier mismatch %s: "
                           "%r vs %r" % (format_context_qualifier(children, index), public_id_1, public_id_2), path)
            system_id_1 = node.getSystemId()
            system_id_2 = expected_node.getSystemId()
            if system_id_1 != system_id_2:
                path_error("Doctype system identifier mismatch %s: "
                           "%r vs %r" % (format_context_qualifier(children, index), system_id_1, system_id_2), path)
            return
        if isinstance(node, archon.dom.Element):
            if not isinstance(expected_node, archon.dom.Element):
                type_mismatch()
                return
            namespace_uri_1 = node.getNamespaceURI()
            namespace_uri_2 = expected_node.getNamespaceURI()
            local_name_1 = node.getLocalName()
            local_name_2 = expected_node.getLocalName()
            if namespace_uri_1 != namespace_uri_2 or local_name_1 != local_name_2:
                path_error("Element type mismatch %s: %s vs %s" % (format_context_qualifier(), _format_node(node),
                                                                   _format_node(expected_node)), path)
                return
            same_attributes = True
            attributes_1 = node.getAttributes()
            attributes_2 = expected_node.getAttributes()
            if len(attributes_1) != len(attributes_2):
                same_attributes = False
            else:
                map = {}
                for attr in attributes_2.values() :
                    key = (attr.getNamespaceURI(), attr.getLocalName())
                    assert key not in map
                    map[key] = attr
                for attr in attributes_1.values():
                    key = (attr.getNamespaceURI(), attr.getLocalName())
                    expected_attr = map.get(key)
                    if not expected_attr or attr.getValue() != expected_attr.getValue():
                        same_attributes = False
                        break
            subpath = path + [index]
            if not same_attributes:
                def format_attributes(attributes):
                    parts = ["%s=%r" % (_format_attribute_name(attr), attr.getValue()) for attr in attributes]
                    return "[%s]" % " ".join(parts)
                formatted_1 = format_attributes(attributes_1)
                formatted_2 = format_attributes(attributes_2)
                path_error("Attribute difference: %s vs %s" % (formatted_1, formatted_2), subpath)
            compare_child_lists(list(node.getChildNodes()), list(expected_node.getChildNodes()), subpath)
            return
        if isinstance(node, archon.dom.Text):
            if not isinstance(expected_node, archon.dom.Text):
                type_mismatch()
                return
            data_1 = node.getData()
            data_2 = expected_node.getData()
            if data_1 != data_2:
                path_error("Text mismatch %s: %r vs %r" % (format_context_qualifier(children, index),
                                                           data_1, data_2), path)
                return
            return
        if isinstance(node, archon.dom.Comment):
            if not isinstance(expected_node, archon.dom.Comment):
                type_mismatch()
                return
            data_1 = node.getData()
            data_2 = expected_node.getData()
            if data_1 != data_2:
                path_error("Comment mismatch %s: %r vs %r" % (format_context_qualifier(children, index),
                                                              data_1, data_2), path)
                return
            return
        assert False

    def compare_child_lists(nodes, expected_nodes, path):
        n = min(len(nodes), len(expected_nodes))
        for i, node in enumerate(nodes[:n]):
            compare_children(node, expected_nodes[i], path, nodes, i)
        if len(nodes) > n:
            path_error("Extraneous nodes %s: %s" % (format_context_qualifier(nodes, n),
                                                    _format_nodes(nodes[n:])), path)
        elif len(expected_nodes) > n:
            path_error("Missing nodes %s: %s" % (format_context_qualifier(nodes, n),
                                                 _format_nodes(expected_nodes[n:])), path)

    def format_context_qualifier(nodes, index):
        if index == 0:
            return "at beginning"
        max_nodes = 3
        if index <= max_nodes:
            return "after %s" % _format_nodes(nodes[:index])
        return "after ...%s" % _format_nodes(nodes[index-max_nodes:index])

    def path_error(message, path):
        error("%s: %s" % (format_path(path), message))

    errors_occurred = False
    def error(message):
        nonlocal errors_occurred
        errors_occurred = True
        print("ERROR: %s" % message)

    document = parse_string(string)

    if expect_parse_error and not parse_errors_occurred:
        error("Expected parse error, but none occurred")

    if not expect_parse_error and parse_errors_occurred:
        errors_occurred = True

    expected_document = parse_expected(expected)

    compare_child_lists(list(document.getChildNodes()), list(expected_document.getChildNodes()), [])

    return not errors_occurred


def _format_nodes(nodes):
    return ", ".join([_format_node(node) for node in nodes])

def _format_node(node):
    if isinstance(node, archon.dom.DocumentType):
        return "#doctype"
    if isinstance(node, archon.dom.Element):
        return _format_element_name(node)
    if isinstance(node, archon.dom.Text):
        return "#text(%s)" % archon.core.clamped_quote(node.getData(), 12)
    if isinstance(node, archon.dom.Comment):
        return "#comment"
    assert False


def _format_element_name(elem):
    assert isinstance(elem, archon.dom.Element)
    namespace_uri = elem.getNamespaceURI()
    local_name = elem.getLocalName()
    if namespace_uri == "http://www.w3.org/1999/xhtml":
        return local_name
    prefix = _get_default_prefix(namespace_uri)
    return "%s:%s" % (prefix, local_name)


def _format_attribute_name(attr):
    assert isinstance(attr, archon.dom.Attr)
    namespace_uri = attr.getNamespaceURI()
    local_name = attr.getLocalName()
    if namespace_uri is None:
        return local_name
    prefix = _get_default_prefix(namespace_uri)
    return "%s:%s" % (prefix, local_name)


def _get_default_prefix(namespace_uri):
    if namespace_uri == "http://www.w3.org/1999/xhtml":
        return "html"
    if namespace_uri == "http://www.w3.org/1998/Math/MathML":
        return "mathml"
    if namespace_uri == "http://www.w3.org/2000/svg":
        return "svg"
    if namespace_uri == "http://www.w3.org/1999/xlink":
        return "xlink"
    if namespace_uri == "http://www.w3.org/XML/1998/namespace":
        return "xml"
    if namespace_uri == "http://www.w3.org/2000/xmlns/":
        return "xmlns"
    assert False


if __name__ == '__main__':
    _run_tests()
