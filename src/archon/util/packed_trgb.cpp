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

#include <map>
#include <sstream>

#include <archon/util/packed_trgb.hpp>
#include <archon/util/named_colors.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::util;


namespace {

struct NameMapBootEntry {
    const char* const name;
    const PackedTRGB trgb;
    const bool css21;
};

NameMapBootEntry name_map_entries[] = {
    { "aliceblue",            color::aliceblue,            false },
    { "antiquewhite",         color::antiquewhite,         false },
    { "aqua",                 color::aqua,                 true  },
    { "aquamarine",           color::aquamarine,           false },
    { "azure",                color::azure,                false },
    { "beige",                color::beige,                false },
    { "bisque",               color::bisque,               false },
    { "black",                color::black,                true  },
    { "blanchedalmond",       color::blanchedalmond,       false },
    { "blue",                 color::blue,                 true  },
    { "blueviolet",           color::blueviolet,           false },
    { "brown",                color::brown,                false },
    { "burlywood",            color::burlywood,            false },
    { "cadetblue",            color::cadetblue,            false },
    { "chartreuse",           color::chartreuse,           false },
    { "chocolate",            color::chocolate,            false },
    { "coral",                color::coral,                false },
    { "cornflowerblue",       color::cornflowerblue,       false },
    { "cornsilk",             color::cornsilk,             false },
    { "crimson",              color::crimson,              false },
    { "cyan",                 color::cyan,                 false },
    { "darkblue",             color::darkblue,             false },
    { "darkcyan",             color::darkcyan,             false },
    { "darkgoldenrod",        color::darkgoldenrod,        false },
    { "darkgray",             color::darkgray,             false },
    { "darkgreen",            color::darkgreen,            false },
    { "darkgrey",             color::darkgrey,             false },
    { "darkkhaki",            color::darkkhaki,            false },
    { "darkmagenta",          color::darkmagenta,          false },
    { "darkolivegreen",       color::darkolivegreen,       false },
    { "darkorange",           color::darkorange,           false },
    { "darkorchid",           color::darkorchid,           false },
    { "darkred",              color::darkred,              false },
    { "darksalmon",           color::darksalmon,           false },
    { "darkseagreen",         color::darkseagreen,         false },
    { "darkslateblue",        color::darkslateblue,        false },
    { "darkslategray",        color::darkslategray,        false },
    { "darkslategrey",        color::darkslategrey,        false },
    { "darkturquoise",        color::darkturquoise,        false },
    { "darkviolet",           color::darkviolet,           false },
    { "deeppink",             color::deeppink,             false },
    { "deepskyblue",          color::deepskyblue,          false },
    { "dimgray",              color::dimgray,              false },
    { "dimgrey",              color::dimgrey,              false },
    { "dodgerblue",           color::dodgerblue,           false },
    { "firebrick",            color::firebrick,            false },
    { "floralwhite",          color::floralwhite,          false },
    { "forestgreen",          color::forestgreen,          false },
    { "fuchsia",              color::fuchsia,              true  },
    { "gainsboro",            color::gainsboro,            false },
    { "ghostwhite",           color::ghostwhite,           false },
    { "gold",                 color::gold,                 false },
    { "goldenrod",            color::goldenrod,            false },
    { "gray",                 color::gray,                 true  },
    { "green",                color::green,                true  },
    { "greenyellow",          color::greenyellow,          false },
    { "grey",                 color::grey,                 false },
    { "honeydew",             color::honeydew,             false },
    { "hotpink",              color::hotpink,              false },
    { "indianred",            color::indianred,            false },
    { "indigo",               color::indigo,               false },
    { "ivory",                color::ivory,                false },
    { "khaki",                color::khaki,                false },
    { "lavender",             color::lavender,             false },
    { "lavenderblush",        color::lavenderblush,        false },
    { "lawngreen",            color::lawngreen,            false },
    { "lemonchiffon",         color::lemonchiffon,         false },
    { "lightblue",            color::lightblue,            false },
    { "lightcoral",           color::lightcoral,           false },
    { "lightcyan",            color::lightcyan,            false },
    { "lightgoldenrodyellow", color::lightgoldenrodyellow, false },
    { "lightgray",            color::lightgray,            false },
    { "lightgreen",           color::lightgreen,           false },
    { "lightgrey",            color::lightgrey,            false },
    { "lightpink",            color::lightpink,            false },
    { "lightsalmon",          color::lightsalmon,          false },
    { "lightseagreen",        color::lightseagreen,        false },
    { "lightskyblue",         color::lightskyblue,         false },
    { "lightslategray",       color::lightslategray,       false },
    { "lightslategrey",       color::lightslategrey,       false },
    { "lightsteelblue",       color::lightsteelblue,       false },
    { "lightyellow",          color::lightyellow,          false },
    { "lime",                 color::lime,                 true  },
    { "limegreen",            color::limegreen,            false },
    { "linen",                color::linen,                false },
    { "magenta",              color::magenta,              false },
    { "maroon",               color::maroon,               true  },
    { "mediumaquamarine",     color::mediumaquamarine,     false },
    { "mediumblue",           color::mediumblue,           false },
    { "mediumorchid",         color::mediumorchid,         false },
    { "mediumpurple",         color::mediumpurple,         false },
    { "mediumseagreen",       color::mediumseagreen,       false },
    { "mediumslateblue",      color::mediumslateblue,      false },
    { "mediumspringgreen",    color::mediumspringgreen,    false },
    { "mediumturquoise",      color::mediumturquoise,      false },
    { "mediumvioletred",      color::mediumvioletred,      false },
    { "midnightblue",         color::midnightblue,         false },
    { "mintcream",            color::mintcream,            false },
    { "mistyrose",            color::mistyrose,            false },
    { "moccasin",             color::moccasin,             false },
    { "navajowhite",          color::navajowhite,          false },
    { "navy",                 color::navy,                 true  },
    { "oldlace",              color::oldlace,              false },
    { "olive",                color::olive,                true  },
    { "olivedrab",            color::olivedrab,            false },
    { "orange",               color::orange,               true  },
    { "orangered",            color::orangered,            false },
    { "orchid",               color::orchid,               false },
    { "palegoldenrod",        color::palegoldenrod,        false },
    { "palegreen",            color::palegreen,            false },
    { "paleturquoise",        color::paleturquoise,        false },
    { "palevioletred",        color::palevioletred,        false },
    { "papayawhip",           color::papayawhip,           false },
    { "peachpuff",            color::peachpuff,            false },
    { "peru",                 color::peru,                 false },
    { "pink",                 color::pink,                 false },
    { "plum",                 color::plum,                 false },
    { "powderblue",           color::powderblue,           false },
    { "purple",               color::purple,               true  },
    { "red",                  color::red,                  true  },
    { "rosybrown",            color::rosybrown,            false },
    { "royalblue",            color::royalblue,            false },
    { "saddlebrown",          color::saddlebrown,          false },
    { "salmon",               color::salmon,               false },
    { "sandybrown",           color::sandybrown,           false },
    { "seagreen",             color::seagreen,             false },
    { "seashell",             color::seashell,             false },
    { "sienna",               color::sienna,               false },
    { "silver",               color::silver,               true  },
    { "skyblue",              color::skyblue,              false },
    { "slateblue",            color::slateblue,            false },
    { "slategray",            color::slategray,            false },
    { "slategrey",            color::slategrey,            false },
    { "snow",                 color::snow,                 false },
    { "springgreen",          color::springgreen,          false },
    { "steelblue",            color::steelblue,            false },
    { "tan",                  color::tan,                  false },
    { "teal",                 color::teal,                 true  },
    { "thistle",              color::thistle,              false },
    { "tomato",               color::tomato,               false },
    { "turquoise",            color::turquoise,            false },
    { "violet",               color::violet,               false },
    { "wheat",                color::wheat,                false },
    { "white",                color::white,                true  },
    { "whitesmoke",           color::whitesmoke,           false },
    { "yellow",               color::yellow,               true  },
    { "transparent",          color::transparent,          false },
};


class NameMap {
public:
    struct Entry {
        int index;
        string name;
        PackedTRGB color;
    };

    typedef std::vector<Entry> Entries;
    typedef std::map<PackedTRGB::value_type, int> ByValue;
    typedef std::map<string, int> ByName;

    Entries entries;

    ByValue  by_value;
    ByName   by_name;

    ByValue by_value_css21;
    ByName  by_name_css21;

    void add(string name, PackedTRGB color, bool css21)
    {
        int index = entries.size();
        entries.push_back(Entry{index, name, color});

        by_value[PackedTRGB::value_type(color)] = index;
        by_name[name]  = index;

        if (!css21)
            return;
        by_value_css21[PackedTRGB::value_type(color)] = index;
        by_name_css21[name]  = index;
    }

    NameMap()
    {
        int num_entries = sizeof name_map_entries / sizeof *name_map_entries;
        const NameMapBootEntry* end = name_map_entries + num_entries;
        for (const NameMapBootEntry* i = name_map_entries; i != end; ++i)
            add(i->name, i->trgb, i->css21);
    }
};


const NameMap& get_name_map()
{
    static NameMap m;
    return m;
}



string format_hex_form(int r, int g, int b, int a, bool uppercase)
{
    bool translucent = a < 255;
    PackedTRGB::value_type value;
    int n;
    if (r>>4 == (r&15) && g>>4 == (g&15) && b>>4 == (b&15) && a>>4 == (a&15)) {
        // Collapse repeated digits
        if (translucent) {
            value =
                static_cast<unsigned>(r&15) << 12 |
                static_cast<unsigned>(g&15) <<  8 |
                static_cast<unsigned>(b&15) <<  4 |
                static_cast<unsigned>(a&15);
            n = 4;
        }
        else {
            value =
                static_cast<unsigned>(r&15) << 8 |
                static_cast<unsigned>(g&15) << 4 |
                static_cast<unsigned>(b&15);
            n = 3;
        }
    }
    else {
        if (translucent) {
            // Move alpha to least significant bits
            value =
                PackedTRGB::value_type(r) << 24 |
                PackedTRGB::value_type(g) << 16 |
                PackedTRGB::value_type(b) <<  8 |
                PackedTRGB::value_type(a);
            n = 8;
        }
        else {
            value =
                PackedTRGB::value_type(r) << 16 |
                PackedTRGB::value_type(g) <<  8 |
                PackedTRGB::value_type(b);
            n = 6;
        }
    }

    ostringstream out;
    out.imbue(locale::classic());
    out << "#";
    out.setf(ios_base::hex, ios_base::basefield);
    if (uppercase)
        out.setf(ios_base::uppercase);
    out.fill('0');
    out.width(n);
    out << value;
    return out.str();
}



// Returns 0 if parsing was unsuccessful, 1 if the color components
// are expressed as integers in the range [0;255], or 2 if the color
// components are expressed as percentages in the range [0;100]. The
// alpha component is always a value in the range [0;1].
int parse_func_form(string str, int offset, Vec4F& color, bool allow_alpha)
{
    bool parse_alpha = false;
    string::size_type size = str.size();
    if (size <= string::size_type(offset))
        return 0;
    char ch = str[offset];
    if (ch != '(') {
        if (!allow_alpha || ch != 'a' || size <= string::size_type(++offset) || str[offset] != '(')
            return 0;
        parse_alpha = true;
    }

    Vec4F color2;
    bool percentage_form = false;

    str = str.substr(offset+1);
    istringstream in(str);
    in.imbue(locale::classic());
    int i1;
    char c1, c2;
    in >> i1 >> c1;
    if (in && c1 == ',') {
        int i2, i3;
        in >> i2 >> c2 >> i3;
        color2.set(i1, i2, i3, 1);
    }
    else {
        in.str(str);
        float f1, f2, f3;
        char p1, p2, p3;
        in >>
            f1 >> noskipws >> p1 >> skipws >> c1 >>
            f2 >> noskipws >> p2 >> skipws >> c2 >>
            f3 >> noskipws >> p3 >> skipws;
        if (p1 != '%' || p2 != '%' || p3 != '%')
            return 0;
        color2.set(f1, f2, f3, 1);
        percentage_form = true;
    }
    if (c1 != ',' || c2 != ',')
        return 0;
    if (parse_alpha) {
        in >> c1 >> color2[3];
        if (c1 != ',')
            return 0;
    }
    in >> c1;
    if (in.fail() || in.bad() || in.get() != char_traits<char>::eof() || c1 != ')')
        return 0;

    color = color2;
    return percentage_form ? 2 : 1;
}

} // unnamed namespace


namespace archon {
namespace util {

string PackedTRGB::format(int type, const Vec4F& color, CssLevel css_level)
{
    Vec4F color2;
    double a = color[3];
    if (a < 1 && css_level == css21) {
        // Scale by alpha
        color2.set(color[0]*a, color[1]*a, color[2]*a, 1);
    }
    else {
        color2 = color;
    }

    string colorspace;
    bool percentages = false;
    switch (type) {
        case 2:
            return format_hex_form(int(clamp<double>(color2[0],0,255)   + 0.5),
                                   int(clamp<double>(color2[1],0,255)   + 0.5),
                                   int(clamp<double>(color2[2],0,255)   + 0.5),
                                   int(clamp<double>(color2[3],0,1)*255 + 0.5), true);
        case 3:
            colorspace = "rgb";
            break;
        case 4:
            colorspace = "rgb";
            percentages = true;
            break;
        case 5:
            colorspace = "hsl";
            break;
        case 6:
            colorspace = "hsl";
            percentages = true;
            break;
        default:
            throw invalid_argument("Bad color format");
    }

    ostringstream out;
    out.imbue(locale::classic());
    out << colorspace;
    bool has_alpha = color2[3] < 1;
    if (has_alpha)
        out << 'a';
    out << '(';
    if (percentages) {
        out << color2[0] << "%, " << color2[1] << "%, " << color2[2] << '%';
    }
    else {
        out << lround(color2[0]) << ", " << lround(color2[1]) << ", " << lround(color2[2]);
    }
    if (has_alpha)
        out << ", " << color2[3];
    out << ')';
    return out.str();
}


string PackedTRGB::format2(CssLevel css_level, bool uppercase_hex, bool uppercase_names) const
{
    const NameMap& maps = get_name_map();
    const NameMap::ByValue& m = css_level == css21 ? maps.by_value_css21 : maps.by_value;
    NameMap::ByValue::const_iterator i = m.find(m_value);
    if (i != m.end()) {
        string name = maps.entries[i->second].name;
        return uppercase_names ? Text::toupper(name, locale::classic()) : name;
    }
    value_type value = m_value;
    int r = uint_red(value);
    int g = uint_green(value);
    int b = uint_blue(value);
    int a = uint_alpha(value);
    if (a < 255 && css_level == css21) {
        // Scale by alpha
        double r2 = r * (1.0/255);
        double g2 = g * (1.0/255);
        double b2 = b * (1.0/255);
        double a2 = a * (1.0/255);
        r = int(a2*r2*255 + 0.5);
        g = int(a2*g2*255 + 0.5);
        b = int(a2*b2*255 + 0.5);
        a = 255;
    }
    return format_hex_form(r, g, b, a, uppercase_hex);
}


int PackedTRGB::parse(string str, int& named_index, Vec4F& color, CssLevel css_level)
{
    if (str.empty())
        return 0;
    if (str[0] == '#') {
        string::size_type n = str.size() - 1u;
        if (n != 3 && n != 6 && (css_level != css3_ext || n != 4 && n != 8))
            return 0;
        const ctype<char>& ctype2 = use_facet<ctype<char>>(locale::classic());
        for (string::size_type i = 0; i < n; ++i)
            if (!ctype2.is(ctype_base::xdigit, str[i+1u]))
                return 0;
        istringstream in(str.replace(0, 1, "0x"));
        in.imbue(locale::classic());
        in.setf(ios_base::hex, ios_base::basefield);
        value_type value;
        in >> value;
        int red, green, blue, alpha;
        switch (n) {
            case 6:
                red   = unsigned(value>>16) & 0xFFu;
                green = unsigned(value>>8)  & 0xFFu;
                blue  = unsigned(value)     & 0xFFu;
                alpha = 255;
                break;
            case 8:
                red   = unsigned(value>>24) & 0xFFu;
                green = unsigned(value>>16) & 0xFFu;
                blue  = unsigned(value>>8)  & 0xFFu;
                alpha = unsigned(value)     & 0xFFu;
                break;
            case 3: {
                // Replicate digits
                int r = unsigned(value>>8) & 0xFu;
                int g = unsigned(value>>4) & 0xFu;
                int b = unsigned(value)    & 0xFu;
                red   = r<<4 | r;
                green = g<<4 | g;
                blue  = b<<4 | b;
                alpha = 255;
                break;
            }
            default: { // n == 4
                // replicate digits
                int r = unsigned(value>>12) & 0xFu;
                int g = unsigned(value>>8)  & 0xFu;
                int b = unsigned(value>>4)  & 0xFu;
                int a = unsigned(value)     & 0xFu;
                red   = r<<4 | r;
                green = g<<4 | g;
                blue  = b<<4 | b;
                alpha = a<<4 | a;
                break;
            }
        }
        color.set(red, green, blue, alpha * (1.0/255));
        return 2;
    }

    str = Text::tolower(str, locale::classic());
    if (Text::is_prefix("rgb", str)) {
        int res = parse_func_form(str, 3, color, css_level != css21);
        if (res)
            return 2 + res;
    }
    else if (css_level != css21 && Text::is_prefix("hsl", str)) {
        int res = parse_func_form(str, 3, color, true);
        if (res)
            return 4 + res;
    }

    {
        const NameMap& maps = get_name_map();
        const NameMap::ByName& m = css_level == css21 ? maps.by_name_css21 : maps.by_name;
        NameMap::ByName::const_iterator i = m.find(str);
        if (i != m.end()) {
            named_index = maps.entries[i->second].index;
            return 1;
        }
    }

    // FIXME: We might at this time want to check for non-standard
    // functional forms using other available color spaces.

    return 0;
}



bool PackedTRGB::parse2(string str, PackedTRGB &trgb, CssLevel css_level)
{
    Vec4F color;
    int named_index;
    int res = parse(str, named_index, color, css_level);

    Vec4F rgba;
    switch(res) {
        case 0:
            return false;
        case 1:
            trgb = get_named_color(named_index);
            return true;
        case 2:
        case 3:
            if (color[0] < -0.25 || 255.25 < color[0] ||
                color[1] < -0.25 || 255.25 < color[1] ||
                color[2] < -0.25 || 255.25 < color[2] ||
                color[3] <  0    ||   1 < color[3])
            {
                return false;
            }
            trgb = PackedTRGB(int(color[0]+0.5),
                              int(color[1]+0.5),
                              int(color[2]+0.5),
                              int(color[3]*255+0.5));
            break;
        case 4:
            if (color[0] < 0 || 100 < color[0] ||
                color[1] < 0 || 100 < color[1] ||
                color[2] < 0 || 100 < color[2] ||
                color[3] < 0 ||   1 < color[3]) {
                return false;
            }
            trgb = PackedTRGB(int(color[0]*(255/100)+0.5),
                              int(color[1]*(255/100)+0.5),
                              int(color[2]*(255/100)+0.5),
                              int(color[3]*255+0.5));
            break;
        case 5:
        case 6:
            throw runtime_error("Unfortunately, the HSL color space is not yet available");
            // Convert color such that all components are in rangle [0;1]
            // color.slice<3>() = color::cvt_HSL_to_RGB(color.slice<3>());
            // trgb = PackedTRGB(int(color[0]*255+0.5),
            //                   int(color[1]*255+0.5),
            //                   int(color[2]*255+0.5),
            //                   int(color[3]*255+0.5));
    }
    return true;
}


PackedTRGB PackedTRGB::get_named_color(int index)
{
    return get_name_map().entries.at(index).color;
}

string PackedTRGB::get_color_name(int index)
{
    return get_name_map().entries.at(index).name;
}

} // namespace util
} // namespace archon
