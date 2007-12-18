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

#include <iostream>

#include <archon/dom/bootstrap.hpp>
#include <archon/dom/html.hpp>


using namespace Archon::dom;


int main() throw()
{
    ref<bootstrap::DOMImplementationRegistry> registry =
        bootstrap::DOMImplementationRegistry::newInstance();

    ref<DOMImplementation> impl =
        registry->getDOMImplementation(str_from_cloc(L"CORE 3.0 HTML 2.0"));

    ref<Document> doc = impl->createDocument(DOMString(), DOMString(), null);

    ref<html::HTMLDocument> html_doc = dynamic_pointer_cast<html::HTMLDocument>(doc);

    std::cout << html_doc->getBody() << std::endl;

    DOMString ns_xhtml = str_from_cloc(L"http://www.w3.org/1999/xhtml");
    ref<Element> root = doc->createElementNS(ns_xhtml, str_from_cloc(L"html"));
    root->appendChild(doc->createElementNS(ns_xhtml, str_from_cloc(L"body")));
    doc->appendChild(root);

    std::cout << html_doc->getBody() << std::endl;
}
