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

#include <cmath>
#include <algorithm>
#include <map>

#include <archon/core/text.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/word_type.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::util;
using namespace archon::image;


namespace {

template<class Source, class Target> void cvt_frac(const void* source, void* target, size_t n)
{
    const Source* s = static_cast<const Source*>(source);
    Target* t = static_cast<Target*>(target);
    frac_any_to_any(s, t, n);
}

template<class Source, class Target> void cvt_clamp(const void* source, void* target, size_t n)
{
    const Source* s = static_cast<const Source*>(source);
    Target* t = static_cast<Target*>(target);
    for (size_t i = 0; i < n; ++i)
        t[i] = clamp_any_to_any<Target, Source>(s[i]);
}


class Cvts {
public:
    WordTypeConverter frac, clamp;

    Cvts() = default;

    Cvts(WordTypeConverter frac, WordTypeConverter clamp):
        frac(frac),
        clamp(clamp)
    {
    }
};

class WordTypeDescriptor {
public:
    WordType type;
    string tag;
    int width; ///< In bytes
    bool is_float;
    vector<Cvts> converters;  // Indexed by target word type

    WordTypeDescriptor() = default;

    WordTypeDescriptor(WordType type, string tag, int width, bool is_float):
        type(type),
        tag(tag),
        width(width),
        is_float(is_float)
    {
    }
};

class WordTypeRegistry {
public:
    vector<WordTypeDescriptor> types; // Indexed by word type
    map<string, WordType> tag_map;
    map<int,    WordType> int_width_map;   ///< Holds only integer word types
    map<int,    WordType> float_width_map; ///< Holds only floating point word types
    map<int,    WordType> mant_width_map;  ///< Indexed by number of bits in mantissa
    int max_width; // Width of widest word type in bytes

    template<class Source, class Target> void add_cvt(WordTypeDescriptor& d, WordType target)
    {
        vector<Cvts>& v = d.converters;
        if (v.size() <= unsigned(target))
            v.resize(target+1);
        v[target] = Cvts(&cvt_frac<Source, Target>, &cvt_clamp<Source, Target>);
    }

    template<class T> void add_descriptor(WordType type, string tag)
    {
        if (types.size() <= unsigned(type))
            types.resize(type+1);
        WordTypeDescriptor &d = types[type];
        d.type = type;
        d.tag  = tag;
        d.width = sizeof (T);
        d.is_float = !numeric_limits<T>::is_integer;

        add_cvt<T, long double>    (d, word_type_LngDbl);
        add_cvt<T, double>         (d, word_type_Double);
        add_cvt<T, float>          (d, word_type_Float);
        add_cvt<T, unsigned long>  (d, word_type_ULong);
        add_cvt<T, unsigned int>   (d, word_type_UInt);
        add_cvt<T, unsigned short> (d, word_type_UShort);
        add_cvt<T, unsigned char>  (d, word_type_UChar);

        tag_map[tag] = type;
        if (d.is_float) {
            float_width_map[d.width] = type;
            // Adding 0.9999 to round up while allowing for a little bit of
            // numeric instability
            int mant = numeric_limits<T>::radix == 2 ? numeric_limits<T>::digits :
                numeric_limits<T>::digits * log(numeric_limits<T>::radix) / log(2) + 0.9999;
            mant_width_map[mant] = type;
        }
        else {
            int_width_map[d.width] = type;
        }
    }

    WordTypeRegistry()
    {
        // Add word type in order of decreasing width to get the
        // logically shortest word type when querying by bit width.
        add_descriptor<long double>    (word_type_LngDbl, "long double");
        add_descriptor<double>         (word_type_Double, "double");
        add_descriptor<float>          (word_type_Float,  "float");
        add_descriptor<unsigned long>  (word_type_ULong,  "unsigned long");
        add_descriptor<unsigned int>   (word_type_UInt,   "unsigned int");
        add_descriptor<unsigned short> (word_type_UShort, "unsigned short");
        add_descriptor<unsigned char>  (word_type_UChar,  "unsigned char");
        max_width = max(int_width_map.rbegin()->first, float_width_map.rbegin()->first);
    }
};

inline const WordTypeRegistry& get_word_type_registry()
{
    static WordTypeRegistry registry;
    return registry;
}

inline const WordTypeDescriptor& get_word_type_descriptor(WordType type)
{
    return get_word_type_registry().types[type];
}

} // unnamed namespace


namespace archon {
namespace image {

WordType get_word_type_by_minimum_bit_width(int width, bool floating_point)
{
    const map<int, WordType>& m = floating_point ?
        get_word_type_registry().float_width_map : get_word_type_registry().int_width_map;
    map<int, WordType>::const_iterator i =
        m.lower_bound((width + numeric_limits<unsigned char>::digits - 1) /
                      numeric_limits<unsigned char>::digits);
    if (i != m.end())
        return i->second;
    throw NoSuchWordTypeException("No "+string(floating_point?"floating point":"integer")+
                                  " types of at least "+Text::print(width)+" bits exist on this "
                                  "platform");
}

WordType get_word_type_by_bit_width(int width, bool floating_point, bool at_least)
{
    if (at_least)
        return get_word_type_by_minimum_bit_width(width, floating_point);
    div_t d = div(width, numeric_limits<unsigned char>::digits);
    if (!d.rem) {
        const map<int, WordType>& m = floating_point ?
            get_word_type_registry().float_width_map : get_word_type_registry().int_width_map;
        map<int, WordType>::const_iterator i = m.find(d.quot);
        if (i != m.end())
            return i->second;
    }
    throw NoSuchWordTypeException("No "+string(floating_point?"floating point":"integer")+
                                  " types of "+Text::print(width)+" bits exist on this "
                                  "platform");
}

WordType get_best_float_type_by_mantissa_bits(int width)
{
    const map<int, WordType>& m = get_word_type_registry().mant_width_map;
    map<int, WordType>::const_iterator i = m.lower_bound(width);
    return i == m.end() ? word_type_LngDbl : i->second;
}

WordType get_word_type_by_name(std::string name)
{
    const map<string, WordType>& m = get_word_type_registry().tag_map;
    map<string, WordType>::const_iterator i = m.find(name);
    if (i != m.end())
        return i->second;
    throw NoSuchWordTypeException("Invalid word type name '"+name+"'");
}

string get_word_type_name(WordType t)
{
    return get_word_type_descriptor(t).tag;
}

int get_max_bytes_per_word()
{
    return get_word_type_registry().max_width;
}

int get_num_word_types()
{
    return get_word_type_registry().types.size();
}

WordType get_word_type_by_index(int index)
{
    return get_word_type_registry().types.at(index).type;
}

WordTypeConverter get_word_type_frac_converter(WordType s, WordType t)
{
    return get_word_type_descriptor(s).converters[t].frac;
}

WordTypeConverter get_word_type_clamp_converter(WordType s, WordType t)
{
    return get_word_type_descriptor(s).converters[t].clamp;
}

} // namespace image
} // namespace archon
