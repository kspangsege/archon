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

#include <archon/core/text.hpp>
#include <archon/dom/bootstrap.hpp>
#include <archon/dom/ls.hpp>


#define TEST(assertion)              if(!(assertion)) throw std::runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw std::runtime_error(message)


using namespace archon::dom;


int main() throw()
{
    ref<bootstrap::DOMImplementationRegistry> registry =
        bootstrap::DOMImplementationRegistry::newInstance();

    ref<DOMImplementation> impl =
        registry->getDOMImplementation(str_from_cloc(L"CORE 3.0 XML +LS"));
    TEST_MSG(impl, "No such implementation");

    ref<ls::DOMImplementationLS> ls = dynamic_pointer_cast<ls::DOMImplementationLS>(impl);
    TEST_MSG(ls, "Wrong implementation");

    ref<ls::LSInput> input = ls->createLSInput();
    input->setSystemId(str_from_cloc(L"/home/kristian/public_html/tests/funny.xml"));

    ref<ls::LSParser> parser =
        ls->createLSParser(ls::DOMImplementationLS::MODE_ASYNCHRONOUS, DOMString());

    parser->parse(input);
}
