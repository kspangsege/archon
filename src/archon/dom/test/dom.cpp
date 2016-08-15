/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <stdexcept>
#include <iostream>

#include <archon/dom/bootstrap.hpp>


#define TEST(assertion)              if(!(assertion)) throw std::runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw std::runtime_error(message)

#define CHECK_EXCEPTION(error_code, expr)                       \
    try {                                                       \
        (expr);                                                 \
        TEST_MSG(false, "Got no exception");                    \
    }                                                           \
    catch (DOMException &e) {                                   \
        TEST_MSG(e.code == error_code, "Got wrong error code"); \
    }


using namespace archon::dom;


namespace {

void clear(ref<Node> parent)
{
    while (ref<Node> n = parent->getFirstChild())
        parent->removeChild(n);
}

/*
void add_n_text(ref<Document> doc, ref<Node> parent, DOMString text, int n)
{
    for (int i = 0; i < n; ++i)
        parent->appendChild(doc->createTextNode(text));
}

void add_n_elem(ref<Document> doc, ref<Node> parent, DOMString ns, DOMString tag, int n)
{
    for (int i = 0; i < n; ++i)
        parent->appendChild(doc->createElementNS(ns, tag));
}
*/

void add_n_comment(ref<Document> doc, ref<Node> parent, DOMString text, int n)
{
    for (int i = 0; i < n; ++i)
        parent->appendChild(doc->createComment(text));
}


void check_freestanding(ref<Node> node)
{
    TEST_MSG(!node->getParentNode(), "Unexpected parent node of freestanding node");
    TEST_MSG(!node->getNextSibling(), "Unexpected next sibling of freestanding node");
    TEST_MSG(!node->getPreviousSibling(), "Unexpected previous sibling of freestanding node");
}


void check_children(ref<Node> parent, ref<Node> children[])
{
    ref<Node> child = children[0];
    if (!child) {
        TEST_MSG(!parent->getFirstChild(), "Unexpected first child of empty parent");
        TEST_MSG(!parent->getLastChild(), "Unexpected last child of empty parent");
        return;
    }
    TEST_MSG(parent->getFirstChild(), "Mising first child of parent");
    TEST_MSG(parent->getFirstChild() == child, "Wrong first child of parent");
    TEST_MSG(!child->getPreviousSibling(), "Unexpected previous sibling of first child");
    int i = 0;
    for (;;) {
        TEST_MSG(child->getParentNode(), "Child has no parent");
        TEST_MSG(child->getParentNode() == parent, "Child has wrong parent");
        ref<Node> child2 = children[++i];
        if (!child2)
            break;
        TEST_MSG(child->getNextSibling(), "Missing next sibling of child");
        TEST_MSG(child->getNextSibling() == child2, "Wrong next sibling of child");
        TEST_MSG(child2->getPreviousSibling(), "Missing previous sibling of child");
        TEST_MSG(child2->getPreviousSibling() == child, "Wrong previous sibling of child");
        child = child2;
    }
    TEST_MSG(parent->getLastChild(), "Mising last child of parent");
    TEST_MSG(parent->getLastChild() == child, "Wrong last child of parent");
    TEST_MSG(!child->getNextSibling(), "Unexpected next sibling of last child");
}


void test_add_remove_child_1(ref<Document> doc, ref<Node> parent)
{
    { ref<Node> list[] = { null }; check_children(parent, list); }

    // Add first
    ref<Node> child1 = doc->createComment(str_from_cloc(L"foo"));
    check_freestanding(child1);
    parent->appendChild(child1);
    { ref<Node> list[] = { child1, null }; check_children(parent, list); }

    // Add second
    ref<Node> child2 = doc->createComment(str_from_cloc(L"bar"));
    parent->appendChild(child2);
    { ref<Node> list[] = { child1, child2, null }; check_children(parent, list); }

    // Remove first
    parent->removeChild(child1);
    { ref<Node> list[] = { child2, null }; check_children(parent, list); }
    check_freestanding(child1);

    // Remove second
    parent->removeChild(child2);
    { ref<Node> list[] = { null }; check_children(parent, list); }
    check_freestanding(child2);

    // Add two then remove last
    parent->appendChild(child1);
    parent->appendChild(child2);
    parent->removeChild(child2);
    { ref<Node> list[] = { child1, null }; check_children(parent, list); }
    check_freestanding(child2);

    // Add a child that is already a child of the same parent
    parent->appendChild(child1);
    { ref<Node> list[] = { child1, null }; check_children(parent, list); }
    parent->appendChild(child2);
    { ref<Node> list[] = { child1, child2, null }; check_children(parent, list); }
    parent->appendChild(child1);
    { ref<Node> list[] = { child2, child1, null }; check_children(parent, list); }
    parent->appendChild(child1);
    { ref<Node> list[] = { child2, child1, null }; check_children(parent, list); }

    // Add a child that is already a child of another parent
    ref<Node> parent2 = doc->createElementNS(str_from_cloc(L"ns"), str_from_cloc(L"parent2"));
    parent2->appendChild(child1);
    { ref<Node> list[] = { child2, null }; check_children(parent, list); }
    { ref<Node> list[] = { child1, null }; check_children(parent2, list); }
    parent->appendChild(child1);
    { ref<Node> list[] = { child2, child1, null }; check_children(parent, list); }
    { ref<Node> list[] = { null }; check_children(parent2, list); }
    parent2->appendChild(child1);
    parent2->appendChild(child2);
    { ref<Node> list[] = { null }; check_children(parent, list); }
    { ref<Node> list[] = { child1, child2, null }; check_children(parent2, list); }

    // Add a document fragment
    ref<Node> frag = doc->createDocumentFragment();
    TEST_MSG(frag->getNodeType() == Node::DOCUMENT_FRAGMENT_NODE,
             "Unexpected type ID of document fragment node");
    parent2->appendChild(frag);
    { ref<Node> list[] = { child1, child2, null }; check_children(parent2, list); }
    { ref<Node> list[] = { null }; check_children(frag, list); }
    ref<Node> child3 = doc->createComment(str_from_cloc(L"baz"));
    frag->appendChild(child3);
    { ref<Node> list[] = { child3, null }; check_children(frag, list); }
    parent2->appendChild(frag);
    { ref<Node> list[] = { child1, child2, child3, null }; check_children(parent2, list); }
    { ref<Node> list[] = { null }; check_children(frag, list); }
    frag->appendChild(child3);
    frag->appendChild(child1);
    { ref<Node> list[] = { child2, null }; check_children(parent2, list); }
    { ref<Node> list[] = { child3, child1, null }; check_children(frag, list); }
    parent2->appendChild(frag);
    { ref<Node> list[] = { child2, child3, child1, null }; check_children(parent2, list); }
    { ref<Node> list[] = { null }; check_children(frag, list); }

    // FIXME: Test insertBefore() incl NOT_FOUND_ERR

    // FIXME: Test replaceChild() incl NOT_FOUND_ERR

    CHECK_EXCEPTION(HIERARCHY_REQUEST_ERR, parent->appendChild(doc));
    CHECK_EXCEPTION(HIERARCHY_REQUEST_ERR, parent->appendChild(parent));
    parent->appendChild(parent2);
    CHECK_EXCEPTION(HIERARCHY_REQUEST_ERR, parent2->appendChild(parent));

    ref<DOMImplementation> impl = doc->getImplementation();
    ref<Document> doc2 = impl->createDocument(str_from_cloc(L"ns"), str_from_cloc(L"root2"), null);
    ref<Node> foreign = doc2->createComment(str_from_cloc(L"foreign"));
    CHECK_EXCEPTION(WRONG_DOCUMENT_ERR, parent->appendChild(foreign));

    ref<Node> doctype = impl->createDocumentType(str_from_cloc(L"alpha"),
                                                 str_from_cloc(L"beta"), str_from_cloc(L"gamma"));
    TEST_MSG(!doctype->getOwnerDocument(), "Unexpected owner document of unbound doctype");
    if (parent == doc) {
        TEST_MSG(!doc->getDoctype(), "Unexpected document type of document");
        parent->appendChild(doctype);
        TEST_MSG(doctype->getOwnerDocument(), "Missing owner document of unbound doctype");
        TEST_MSG(doc->getDoctype(), "Missing document type of document");
        TEST_MSG(doc->getDoctype() == doctype, "Wrong document type of document");
        CHECK_EXCEPTION(WRONG_DOCUMENT_ERR, doc2->appendChild(doctype));
        parent->removeChild(doctype);
        TEST_MSG(!doc->getDoctype(), "Unexpected document type of document after removal");
        TEST_MSG(doctype->getOwnerDocument(),
                 "Missing owner document of unbound doctype after removal");
        CHECK_EXCEPTION(WRONG_DOCUMENT_ERR, doc2->appendChild(doctype));
    }
    else {
        CHECK_EXCEPTION(HIERARCHY_REQUEST_ERR, parent->appendChild(doctype));
    }

    // FIXME: Test that a document can accept at most one doctype and at most one element

    // FIXME: Test read-only
}


void test_add_remove_child(ref<Document> doc)
{
    check_freestanding(doc);
    clear(doc);
    test_add_remove_child_1(doc, doc);
    ref<Element> elem = doc->createElementNS(str_from_cloc(L"ns"), str_from_cloc(L"elem"));
    test_add_remove_child_1(doc, elem);
    clear(elem);
    clear(doc);
    doc->appendChild(elem);
    test_add_remove_child_1(doc, elem);
}


void test_child_list_1(ref<Document> doc, ref<Node> parent)
{
    ref<NodeList> const list = parent->getChildNodes();
    TEST_MSG(list == parent->getChildNodes(), "Different child list instances");
    TEST_MSG(!list->item(0), "Spurious first child node in empty list instance");
    TEST_MSG(list->getLength() == 0, "Spurious number of children in empty list instance");

    add_n_comment(doc, parent, str_from_cloc(L"foo"), 10);

    TEST_MSG(!list->item(11), "Spurious 12th child node in list instance");
    TEST_MSG(!list->item(10), "Spurious 11th child node in list instance");
    TEST_MSG(list->item(9), "Missing 10th child node in list instance");
    TEST_MSG(list->item(9) == parent->getLastChild(), "Wrong 10th child node in list instance");

    ref<Node> child = list->item(8);
    TEST_MSG(child, "Missing 9th child node in list instance");
    TEST_MSG(child == parent->getLastChild()->getPreviousSibling(),
             "Wrong 8th child node in list instance");

    TEST_MSG(list->getLength() == 10, "Wrong number of children in list instance");

    ref<NodeList> const list2 = child->getChildNodes();
    TEST_MSG(list2 == child->getChildNodes(), "Different child list instances of text node");
    TEST_MSG(!list2->item(0), "Spurious first child node in list instance of text node");
    TEST_MSG(list2->getLength() == 0, "Spurious number of children in list instance of text node");
    TEST_MSG(list2 != list->item(9)->getChildNodes(),
             "Same child list instances of distinct text nodes");

    parent->insertBefore(doc->createComment(str_from_cloc(L"bar")), child);
    TEST_MSG(list->item(8) != child, "Wrong 9th child in list instance after insertion");
    TEST_MSG(list->item(8)->getNodeType() == Node::COMMENT_NODE,
             "Wrong type of 9th child in list instance after insertion");
    TEST_MSG(list->item(8)->getNextSibling() == child,
             "Wrong 10th child in list instance after insertion");
    TEST_MSG(list->getLength() == 11, "Wrong number of children in list instance after insertion");

    parent->removeChild(list->item(8));
    TEST_MSG(list->getLength() == 10,
             "Wrong number of children in list instance after insertion then deletion");
    TEST_MSG(list->item(8) == child,
             "Wrong 9th child in list instance after insertion then deletion");

    clear(parent);
    TEST_MSG(list->getLength() == 0, "Spurious number of children in cleared list instance");
    TEST_MSG(!list->item(0), "Spurious first child node in cleared list instance");

    add_n_comment(doc, parent, str_from_cloc(L"foo"), 1);

    TEST_MSG(list->getLength() == 1, "Wrong number of children in refilled list instance");
    TEST_MSG(list->item(0) == parent->getLastChild(),
             "Wrong child node in refilled list instance");
    TEST_MSG(!list->item(1), "Spurious 2nd child node in refilled list instance");
}

void test_child_list(ref<Document> doc)
{
    clear(doc);
    test_child_list_1(doc, doc);
    ref<Element> elem = doc->createElementNS(str_from_cloc(L"ns"), str_from_cloc(L"elem"));
    test_child_list_1(doc, elem);
    clear(elem);
    doc->appendChild(elem);
    test_child_list_1(doc, elem);
}


void test()
{
    ref<bootstrap::DOMImplementationRegistry> registry =
        bootstrap::DOMImplementationRegistry::newInstance();

    ref<DOMImplementation> impl = registry->getDOMImplementation(str_from_cloc(L"CORE 3.0 +XML"));
    TEST_MSG(impl, "No such implementation");

    DOMString ns = str_from_cloc(L"my://namespace");

    ref<Document> doc = impl->createDocument(ns, str_from_cloc(L"root"), null);

    test_add_remove_child(doc);
    test_child_list(doc);
}
}


int main() throw()
{
    try {
        test();
        std::cout << "OK" << std::endl;
    }
    catch (std::exception &e) {
        std::cout << "ERROR: " << e.what() << std::endl;
    }
}
