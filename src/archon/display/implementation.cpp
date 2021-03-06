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

/// \file
///
/// \author Kristian Spangsege

#include <vector>

#include <archon/platform.hpp> // Never include this one in header files
#include <archon/display/implementation.hpp>

#ifdef ARCHON_HAVE_XLIB
#  include <archon/display/x11/implementation.hpp>
#endif


using namespace archon::display;


namespace {

class ImplementationRegistry {
public:
    ImplementationRegistry()
    {
#ifdef ARCHON_HAVE_XLIB
        implementations.push_back(get_implementation_x11()); // Throws
#endif
    }

    std::vector<Implementation::Ptr> implementations;
};

const ImplementationRegistry* get_impl_registry()
{
    static ImplementationRegistry registry;
    return &registry;
}

} // unnamed namespace


namespace archon {
namespace display {

Implementation::Ptr get_default_implementation()
{
    const ImplementationRegistry* r = get_impl_registry();
    if (r->implementations.empty())
        throw NoImplementationException();
    return r->implementations[0];
}

int get_num_implementations()
{
    return get_impl_registry()->implementations.size();
}

Implementation::Ptr get_implementation(int index) throw(std::out_of_range)
{
    return get_impl_registry()->implementations.at(index);
}

} // namespace display
} // namespace archon
