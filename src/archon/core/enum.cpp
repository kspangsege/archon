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

#include <utility>

#include <archon/core/string.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/core/text.hpp>


namespace archon {
namespace core {
namespace _impl {

EnumMapper::EnumMapper(const EnumAssoc* m, bool ignore_case)
{
    if (!m)
        return;
    while (m->name) {
        std::string name = m->name;
        if (!val2name.insert(std::pair<int, std::string>(m->value, name)).second)
            throw std::runtime_error("Multiple names for value "+format_value(m->value));
        if (!name2val.insert(std::pair<std::string, int>(ignore_case ? ascii_tolower(name) : name,
                                                         m->value)).second)
            throw std::runtime_error("Multiple values for name '"+name+"'");
        ++m;
    }
}


bool EnumMapper::parse(std::string s, int &val, bool ignore_case) const
{
    auto i = name2val.find(ignore_case ? ascii_tolower(s) : s); // Throws
    if (i == name2val.end())
        return false;
    val = i->second;
    return true;
}

} // namespace _impl
} // namespace core
} // namespace archon
