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

#ifndef ARCHON_CORE_PROXY_ITER_HPP
#define ARCHON_CORE_PROXY_ITER_HPP

#include <iterator>

namespace archon
{
  namespace Core
  {
    template<typename Proxy, typename RepIter> struct ProxyBase
    {
      bool operator==(Proxy const &p) const { return i == p.i; }
      bool operator!=(Proxy const &p) const { return i != p.i; }
      void next() { ++i; }

      ProxyBase(RepIter const &i): i(i) {}

    protected:
      RepIter i;
    };

    template<typename Proxy> struct ProxyIter: std::iterator<std::input_iterator_tag, Proxy>
    {
      ProxyIter &operator++() { p.next(); return *this; }
      ProxyIter operator++(int) { ProxyIter i = *this; p.next(); return i; }
      bool operator==(ProxyIter const &i) const { return p == i.p; }
      bool operator!=(ProxyIter const &i) const { return p != i.p; }
      Proxy const &operator*()  const { return  p; }
      Proxy const *operator->() const { return &p; }
      ProxyIter(Proxy const &p): p(p) {}

    private:
      Proxy p;
    };
  }
}

#endif // ARCHON_CORE_PROXY_ITER_HPP
