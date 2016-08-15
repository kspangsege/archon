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

#include <algorithm>

#include <archon/dom/bootstrap.hpp>
#include <archon/dom/impl/impl.hpp>


namespace {

using namespace archon::dom;


struct List: DOMImplementationList {
    virtual ref<DOMImplementation> item(uint32 index) const throw ()
    {
        return impls.at(index);
    }

    virtual uint32 getLength() const throw ()
    {
        return impls.size();
    }

    std::vector<ref<DOMImplementation>> impls;

    virtual ~List() throw () {}
};

} // unnamed namespace



namespace archon {
namespace dom {
namespace bootstrap {

ref<DOMImplementationRegistry> DOMImplementationRegistry::newInstance()
{
    return ref<DOMImplementationRegistry>(new DOMImplementationRegistry());
}



ref<DOMImplementation>
DOMImplementationRegistry::getDOMImplementation(const DOMString& features) const throw ()
{
    typedef Sources::const_iterator iter;
    iter e = sources.end();
    for (iter i = sources.begin(); i != e; ++i) {
        if (ref<DOMImplementation> impl = (*i)->getDOMImplementation(features))
            return impl;
    }
    return null;
}



ref<DOMImplementationList>
DOMImplementationRegistry::getDOMImplementationList(const DOMString& features) const throw ()
{
    ref<List> list(new List);

    typedef Sources::const_iterator iter;
    iter e = sources.end();
    for (iter i = sources.begin(); i != e; ++i) {
        ref<DOMImplementationList> l = (*i)->getDOMImplementationList(features);
        uint32 n = l->getLength();
        for (uint32 i = 0; i < n; ++i)
            list->impls.push_back(l->item(i));
    }

    return list;
}



void DOMImplementationRegistry::addSource(const ref<DOMImplementationSource>& s)
{
    Sources::iterator e = sources.end();
    if (find(sources.begin(), e, s) != e)
        return;
    sources.push_back(s);
}



DOMImplementationRegistry::DOMImplementationRegistry()
{
    addSource(dom_impl::get_default_impl_src());
}

} // namespace bootstrap
} // namespace dom
} // namespace archon
