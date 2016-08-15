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

#ifndef ARCHON_CORE_ENUM_HPP
#define ARCHON_CORE_ENUM_HPP

#include <stdexcept>
#include <map>
#include <string>
#include <ios>

#include <archon/core/char_enc.hpp>


namespace archon {
namespace Core {

/**
 * This template class allows you to endow a fundamental \c enum type
 * with information about how to print out the individual values, and
 * how to parse them.
 *
 * Here is an example:
 *
 * <pre>
 *
 *   // Module header
 *
 *   enum Color { orange, purple, brown };
 *
 *   struct ColorSpec { static EnumAssoc map[]; };
 *   typedef Enum<Color, ColorSpec> ColorEnum;
 *
 *
 *   // Module implementation
 *
 *   EnumAssoc ColorSpec::map[] =
 *   {
 *     { orange, "orange" },
 *     { purple, "purple" },
 *     { brown,  "brown"  },
 *     { 0, 0 }
 *   };
 *
 *
 *   // Application
 *
 *   ColorEnum color = violet;
 *
 *   cout << color;  // Write a color
 *   cin  >> color;  // Read a color
 *
 * </pre>
 */
template<class E, class S, bool ignore_case = false> struct Enum {
    typedef E base_enum_type;

    Enum() {}
    Enum(E v): value(v) {}

    operator E() const { return value; }

    std::string str() const;

    bool parse(std::string s); ///< \return True iff successful

private:
    E value;
};

template<class C, class T, class E, class S, bool ignore_case>
std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T>&,
                                    const Enum<E, S, ignore_case>&);

template<class C, class T, class E, class S, bool ignore_case>
std::basic_istream<C,T> &operator>>(std::basic_istream<C,T> &,
                                    Enum<E, S, ignore_case> &);


struct EnumAssoc { const int value; const char* const name; };




// Implementation

namespace _Impl {
struct EnumMapper {
    EnumMapper(const EnumAssoc*, bool ignore_case);

    bool parse(std::string s, int& val, bool ignore_case) const;

    std::map<int, std::string> val2name;
    std::map<std::string, int> name2val;
};

template<class S, bool ignore_case> const EnumMapper& get_enum_mapper()
{
    static EnumMapper m(S::map, ignore_case);
    return m;
}

} // namespace _Impl


template<class E, class S, bool ignore_case>
inline std::string Enum<E, S, ignore_case>::str() const
{
    return _Impl::get_enum_mapper<S, ignore_case>().val2name.at(value);
}

template<class E, class S, bool ignore_case>
inline bool Enum<E, S, ignore_case>::parse(std::string s)
{
    int v;
    if (!_Impl::get_enum_mapper<S, ignore_case>().parse(s, v, ignore_case))
        return false;
    value = E(v);
    return true;
}

template<class C, class T, class E, class S, bool ignore_case>
inline std::basic_ostream<C,T>& operator<<(std::basic_ostream<C,T>& o,
                                           const Enum<E, S, ignore_case>& e)
{
    try {
        o << BasicLocaleCodec<C>(true, o.getloc()).decode(e.str());
    }
    catch(std::out_of_range&)
    {
        o << E(e);
    }
    return o;
}

template<class C, class T, class E, class S, bool ignore_case>
std::basic_istream<C,T>& operator>>(std::basic_istream<C,T>& i,
                                    Enum<E, S, ignore_case>& e)
{
    if (i.bad() || i.fail())
        return i;
    std::basic_string<C> s;
    C u(i.widen('_'));
    BasicLocaleCharMapper<C> ctype(i.getloc());
    for(;;) {
        C c;
        // Allow white-spaces to be skipped when stream is configured
        // that way
        if (s.empty()) {
            i >> c;
        }
        else {
            i.get(c);
        }
        if (!i) {
            if (i.bad())
                return i;
            i.clear(i.rdstate() & ~std::ios_base::failbit);
            break;
        }
        if (!ctype.is(c, std::ctype_base::alnum) && c != u) {
            i.unget();
            break;
        }
        s += c;
    }
    if (!e.parse(BasicLocaleCodec<C>(true, i.getloc()).encode(s)))
        i.setstate(std::ios_base::badbit);
    return i;
}

} // namespace Core
} // namespace archon

#endif // ARCHON_CORE_ENUM_HPP
