// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <tuple>
#include <string_view>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/algorithm.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/string_buffer_contents.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/integer_formatter.hpp>
#include <archon/core/integer_parser.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/util/color_space.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/css_color.hpp>


using namespace archon;
namespace colors = util::colors;


namespace {


using int_comp_type = util::CssColor::int_comp_type;
using flt_comp_type = util::CssColor::flt_comp_type;
using rgba_type     = std::uint_least32_t;


constexpr auto to_rgba(int_comp_type r, int_comp_type g, int_comp_type b, int_comp_type a) noexcept -> rgba_type
{
    return ((rgba_type(r) << 24) |
            (rgba_type(g) << 16) |
            (rgba_type(b) <<  8) |
            (rgba_type(a) <<  0));
}


// Using flt_to_int() instead of flt_to_int_a() below because it distributes the floating
// point values more evenly across into the available integer "buckets", especially the
// buckets corresponding to 0 and 255.
//
// This choice is safe because 255 as `max_int` is small enough compared to the precision of
// `flt_comp_type` that flt_to_int() is as robust as flt_to_int_a() in practice. I.e.,
// flt_to_int<int_comp_type>(int_to_flt<flt_comp_type>(i)) will always be equal to `i` for
// all values of `i` less than, or equal to 255 assuming that `flt_comp_type` has at least
// as much precision as the single-precision type specified by IEEE 754.


auto to_hex(flt_comp_type r, flt_comp_type g, flt_comp_type b, flt_comp_type a) noexcept -> util::CssColor::Hex
{
    int_comp_type r_2 = util::unit_frac::flt_to_int<int_comp_type>(r, 255);
    int_comp_type g_2 = util::unit_frac::flt_to_int<int_comp_type>(g, 255);
    int_comp_type b_2 = util::unit_frac::flt_to_int<int_comp_type>(b, 255);
    int_comp_type a_2 = util::unit_frac::flt_to_int<int_comp_type>(a, 255);
    return { r_2, g_2, b_2, a_2 };
}


auto to_hex(util::CssColor::Rgb rgb) noexcept -> util::CssColor::Hex
{
    return to_hex(rgb.r / 255,
                  rgb.g / 255,
                  rgb.b / 255,
                  rgb.a);
}


auto to_hex(util::CssColor::RgbP rgb) noexcept -> util::CssColor::Hex
{
    return to_hex(rgb.r / 100,
                  rgb.g / 100,
                  rgb.b / 100,
                  rgb.a);
}


auto to_hex(util::CssColor::Hsl hsl) noexcept -> util::CssColor::Hex
{
    using vector_type = math::Vector<3, flt_comp_type>;
    vector_type hsl_2 = { hsl.h / 360, hsl.s / 100, hsl.l / 100 };
    vector_type rgb = util::cvt_HSL_to_sRGB(hsl_2);
    return to_hex(rgb[0], rgb[1], rgb[2], hsl.a);
}


struct NamedColorEntry {
    std::string_view name;
    util::Color color;
    bool css21;
};


constexpr NamedColorEntry g_named_colors[] = {
    { "transparent",          colors::transparent,          false },
    { "aliceblue",            colors::aliceblue,            false },
    { "antiquewhite",         colors::antiquewhite,         false },
    { "aqua",                 colors::aqua,                 true  },
    { "aquamarine",           colors::aquamarine,           false },
    { "azure",                colors::azure,                false },
    { "beige",                colors::beige,                false },
    { "bisque",               colors::bisque,               false },
    { "black",                colors::black,                true  },
    { "blanchedalmond",       colors::blanchedalmond,       false },
    { "blue",                 colors::blue,                 true  },
    { "blueviolet",           colors::blueviolet,           false },
    { "brown",                colors::brown,                false },
    { "burlywood",            colors::burlywood,            false },
    { "cadetblue",            colors::cadetblue,            false },
    { "chartreuse",           colors::chartreuse,           false },
    { "chocolate",            colors::chocolate,            false },
    { "coral",                colors::coral,                false },
    { "cornflowerblue",       colors::cornflowerblue,       false },
    { "cornsilk",             colors::cornsilk,             false },
    { "crimson",              colors::crimson,              false },
    { "cyan",                 colors::cyan,                 false },
    { "darkblue",             colors::darkblue,             false },
    { "darkcyan",             colors::darkcyan,             false },
    { "darkgoldenrod",        colors::darkgoldenrod,        false },
    { "darkgray",             colors::darkgray,             false },
    { "darkgreen",            colors::darkgreen,            false },
    { "darkgrey",             colors::darkgrey,             false },
    { "darkkhaki",            colors::darkkhaki,            false },
    { "darkmagenta",          colors::darkmagenta,          false },
    { "darkolivegreen",       colors::darkolivegreen,       false },
    { "darkorange",           colors::darkorange,           false },
    { "darkorchid",           colors::darkorchid,           false },
    { "darkred",              colors::darkred,              false },
    { "darksalmon",           colors::darksalmon,           false },
    { "darkseagreen",         colors::darkseagreen,         false },
    { "darkslateblue",        colors::darkslateblue,        false },
    { "darkslategray",        colors::darkslategray,        false },
    { "darkslategrey",        colors::darkslategrey,        false },
    { "darkturquoise",        colors::darkturquoise,        false },
    { "darkviolet",           colors::darkviolet,           false },
    { "deeppink",             colors::deeppink,             false },
    { "deepskyblue",          colors::deepskyblue,          false },
    { "dimgray",              colors::dimgray,              false },
    { "dimgrey",              colors::dimgrey,              false },
    { "dodgerblue",           colors::dodgerblue,           false },
    { "firebrick",            colors::firebrick,            false },
    { "floralwhite",          colors::floralwhite,          false },
    { "forestgreen",          colors::forestgreen,          false },
    { "fuchsia",              colors::fuchsia,              true  },
    { "gainsboro",            colors::gainsboro,            false },
    { "ghostwhite",           colors::ghostwhite,           false },
    { "gold",                 colors::gold,                 false },
    { "goldenrod",            colors::goldenrod,            false },
    { "gray",                 colors::gray,                 true  },
    { "green",                colors::green,                true  },
    { "greenyellow",          colors::greenyellow,          false },
    { "grey",                 colors::grey,                 false },
    { "honeydew",             colors::honeydew,             false },
    { "hotpink",              colors::hotpink,              false },
    { "indianred",            colors::indianred,            false },
    { "indigo",               colors::indigo,               false },
    { "ivory",                colors::ivory,                false },
    { "khaki",                colors::khaki,                false },
    { "lavender",             colors::lavender,             false },
    { "lavenderblush",        colors::lavenderblush,        false },
    { "lawngreen",            colors::lawngreen,            false },
    { "lemonchiffon",         colors::lemonchiffon,         false },
    { "lightblue",            colors::lightblue,            false },
    { "lightcoral",           colors::lightcoral,           false },
    { "lightcyan",            colors::lightcyan,            false },
    { "lightgoldenrodyellow", colors::lightgoldenrodyellow, false },
    { "lightgray",            colors::lightgray,            false },
    { "lightgreen",           colors::lightgreen,           false },
    { "lightgrey",            colors::lightgrey,            false },
    { "lightpink",            colors::lightpink,            false },
    { "lightsalmon",          colors::lightsalmon,          false },
    { "lightseagreen",        colors::lightseagreen,        false },
    { "lightskyblue",         colors::lightskyblue,         false },
    { "lightslategray",       colors::lightslategray,       false },
    { "lightslategrey",       colors::lightslategrey,       false },
    { "lightsteelblue",       colors::lightsteelblue,       false },
    { "lightyellow",          colors::lightyellow,          false },
    { "lime",                 colors::lime,                 true  },
    { "limegreen",            colors::limegreen,            false },
    { "linen",                colors::linen,                false },
    { "magenta",              colors::magenta,              false },
    { "maroon",               colors::maroon,               true  },
    { "mediumaquamarine",     colors::mediumaquamarine,     false },
    { "mediumblue",           colors::mediumblue,           false },
    { "mediumorchid",         colors::mediumorchid,         false },
    { "mediumpurple",         colors::mediumpurple,         false },
    { "mediumseagreen",       colors::mediumseagreen,       false },
    { "mediumslateblue",      colors::mediumslateblue,      false },
    { "mediumspringgreen",    colors::mediumspringgreen,    false },
    { "mediumturquoise",      colors::mediumturquoise,      false },
    { "mediumvioletred",      colors::mediumvioletred,      false },
    { "midnightblue",         colors::midnightblue,         false },
    { "mintcream",            colors::mintcream,            false },
    { "mistyrose",            colors::mistyrose,            false },
    { "moccasin",             colors::moccasin,             false },
    { "navajowhite",          colors::navajowhite,          false },
    { "navy",                 colors::navy,                 true  },
    { "oldlace",              colors::oldlace,              false },
    { "olive",                colors::olive,                true  },
    { "olivedrab",            colors::olivedrab,            false },
    { "orange",               colors::orange,               true  },
    { "orangered",            colors::orangered,            false },
    { "orchid",               colors::orchid,               false },
    { "palegoldenrod",        colors::palegoldenrod,        false },
    { "palegreen",            colors::palegreen,            false },
    { "paleturquoise",        colors::paleturquoise,        false },
    { "palevioletred",        colors::palevioletred,        false },
    { "papayawhip",           colors::papayawhip,           false },
    { "peachpuff",            colors::peachpuff,            false },
    { "peru",                 colors::peru,                 false },
    { "pink",                 colors::pink,                 false },
    { "plum",                 colors::plum,                 false },
    { "powderblue",           colors::powderblue,           false },
    { "purple",               colors::purple,               true  },
    { "red",                  colors::red,                  true  },
    { "rosybrown",            colors::rosybrown,            false },
    { "royalblue",            colors::royalblue,            false },
    { "saddlebrown",          colors::saddlebrown,          false },
    { "salmon",               colors::salmon,               false },
    { "sandybrown",           colors::sandybrown,           false },
    { "seagreen",             colors::seagreen,             false },
    { "seashell",             colors::seashell,             false },
    { "sienna",               colors::sienna,               false },
    { "silver",               colors::silver,               true  },
    { "skyblue",              colors::skyblue,              false },
    { "slateblue",            colors::slateblue,            false },
    { "slategray",            colors::slategray,            false },
    { "slategrey",            colors::slategrey,            false },
    { "snow",                 colors::snow,                 false },
    { "springgreen",          colors::springgreen,          false },
    { "steelblue",            colors::steelblue,            false },
    { "tan",                  colors::tan,                  false },
    { "teal",                 colors::teal,                 true  },
    { "thistle",              colors::thistle,              false },
    { "tomato",               colors::tomato,               false },
    { "turquoise",            colors::turquoise,            false },
    { "violet",               colors::violet,               false },
    { "wheat",                colors::wheat,                false },
    { "white",                colors::white,                true  },
    { "whitesmoke",           colors::whitesmoke,           false },
    { "yellow",               colors::yellow,               true  },
};


constexpr std::size_t g_num_named_colors = std::size(g_named_colors);


struct NameIndexEntry {
    std::string_view name;
    std::size_t index;
};


struct ValueIndexEntry {
    rgba_type value;
    std::size_t index;
};


class NamedColorMaps {
public:
    constexpr NamedColorMaps()
    {
        for (std::size_t i = 0; i < g_num_named_colors; ++i)
            name_map[i] = { g_named_colors[i].name, i };
        {
            auto begin = name_map.begin();
            auto end   = name_map.end();
            core::stable_sort(begin, end, [](const auto& a, const auto& b) {
                return a.name < b.name;
            }); // Throws
        }

        for (std::size_t i = 0; i < g_num_named_colors; ++i) {
            util::Color color = g_named_colors[i].color;
            rgba_type value = to_rgba(color.red(), color.green(), color.blue(), color.alpha());
            value_map[i] = { value, i };
        }
        {
            auto begin = value_map.begin();
            auto end   = value_map.end();
            core::stable_sort(begin, end, [](const auto& a, const auto& b) {
                return a.value < b.value;
            }); // Throws
        }
    }

    std::array<NameIndexEntry, g_num_named_colors> name_map = {};
    std::array<ValueIndexEntry, g_num_named_colors> value_map = {};
};


constexpr NamedColorMaps g_named_color_maps; // Throws



void format_hex(int_comp_type r, int_comp_type g, int_comp_type b, int_comp_type a,
                bool disable_short_form, bool lowercase, core::StringBufferContents<char>& buffer)
{
    bool fully_opaque = a >= 255;
    rgba_type value;
    int n;
    bool collapsible = (!disable_short_form &&
                        (r >> 4) == (r & 0x0F) &&
                        (g >> 4) == (g & 0x0F) &&
                        (b >> 4) == (b & 0x0F) &&
                        (a >> 4) == (a & 0x0F));
    if (collapsible) {
        // Collapse repeated digits
        if (fully_opaque) {
            value = rgba_type(rgba_type(r & 0x0F) << 8 |
                              rgba_type(g & 0x0F) << 4 |
                              rgba_type(b & 0x0F) << 0);
            n = 3;
        }
        else {
            value = rgba_type(rgba_type(r & 0x0F) << 12 |
                              rgba_type(g & 0x0F) <<  8 |
                              rgba_type(b & 0x0F) <<  4 |
                              rgba_type(a & 0x0F) <<  0);
            n = 4;
        }
    }
    else {
        if (fully_opaque) {
            value = rgba_type(rgba_type(r) << 16 |
                              rgba_type(g) <<  8 |
                              rgba_type(b) <<  0);
            n = 6;
        }
        else {
            value = rgba_type(rgba_type(r) << 24 |
                              rgba_type(g) << 16 |
                              rgba_type(b) <<  8 |
                              rgba_type(a) <<  0);
            n = 8;
        }
    }

    const std::locale& locale = std::locale::classic();
    core::CharMapper char_mapper(locale); // Throws
    core::IntegerFormatter integer_formatter(char_mapper);
    if (ARCHON_UNLIKELY(lowercase))
        integer_formatter.use_lowercase(true);
    std::string_view string = integer_formatter.format_hex(value, n);
    buffer.append(1, '#'); // Throws
    buffer.append(string); // Throws
}


void format_name(std::size_t index, bool uppercase, core::StringBufferContents<char>& buffer)
{
    ARCHON_ASSERT(index < g_num_named_colors);
    std::string_view name = g_named_colors[index].name;
    char* base = buffer.end();
    buffer.append(name); // Throws
    if (ARCHON_LIKELY(!uppercase))
        return;
    std::size_t size = name.size();
    const std::locale& locale = std::locale::classic();
    const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(locale); // Throws
    ctype.toupper(base, base + size); // Throws
}


void format_rgb(flt_comp_type r, flt_comp_type g, flt_comp_type b, flt_comp_type a, bool percent,
                bool uppercase, bool no_space_after_comma, core::StringBufferContents<char>& buffer)
{
    std::array<char, 32> seed_memory;
    const std::locale& locale = std::locale::classic();
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view label, string;
    if (a >= 1) {
        if (!uppercase) {
            label = "rgb";
        }
        else {
            label = "RGB";
        }
        if (!percent) {
            if (!no_space_after_comma) {
                string = formatter.format("%s(%s, %s, %s)", label, r, g, b); // Throws
            }
            else {
                string = formatter.format("%s(%s,%s,%s)", label, r, g, b); // Throws
            }
        }
        else {
            if (!no_space_after_comma) {
                string = formatter.format("%s(%s%%, %s%%, %s%%)", label, r, g, b); // Throws
            }
            else {
                string = formatter.format("%s(%s%%,%s%%,%s%%)", label, r, g, b); // Throws
            }
        }
    }
    else {
        if (!uppercase) {
            label = "rgba";
        }
        else {
            label = "RGBA";
        }
        if (!percent) {
            if (!no_space_after_comma) {
                string = formatter.format("%s(%s, %s, %s, %s)", label, r, g, b, a); // Throws
            }
            else {
                string = formatter.format("%s(%s,%s,%s,%s)", label, r, g, b, a); // Throws
            }
        }
        else {
            if (!no_space_after_comma) {
                string = formatter.format("%s(%s%%, %s%%, %s%%, %s)", label, r, g, b, a); // Throws
            }
            else {
                string = formatter.format("%s(%s%%,%s%%,%s%%,%s)", label, r, g, b, a); // Throws
            }
        }
    }
    buffer.append(string); // Throws
}


void format_hsl(flt_comp_type h, flt_comp_type s, flt_comp_type l, flt_comp_type a, bool uppercase,
                bool no_space_after_comma, core::StringBufferContents<char>& buffer)
{
    std::array<char, 32> seed_memory;
    const std::locale& locale = std::locale::classic();
    core::StringFormatter formatter(seed_memory, locale); // Throws
    std::string_view label, string;
    if (a >= 1) {
        if (!uppercase) {
            label = "hsl";
        }
        else {
            label = "HSL";
        }
        if (!no_space_after_comma) {
            string = formatter.format("%s(%s, %s%%, %s%%)", label, h, s, l); // Throws
        }
        else {
            string = formatter.format("%s(%s,%s%%,%s%%)", label, h, s, l); // Throws
        }
    }
    else {
        if (!uppercase) {
            label = "hsla";
        }
        else {
            label = "HSLA";
        }
        if (!no_space_after_comma) {
            string = formatter.format("%s(%s, %s%%, %s%%, %s)", label, h, s, l, a); // Throws
        }
        else {
            string = formatter.format("%s(%s,%s%%,%s%%,%s)", label, h, s, l, a); // Throws
        }
    }
    buffer.append(string); // Throws
}


class ArgsParser {
public:
    ArgsParser(std::string_view string)
        : m_value_parser(std::locale::classic()) // Throws
        , m_begin(string.data())
        , m_end(m_begin + string.size())
        , m_curr(m_begin)
    {
    }

    bool parse(flt_comp_type& var, bool& is_percent)
    {
        const char* p = m_curr;
        if (ARCHON_LIKELY(p != m_end)) {
            if (ARCHON_LIKELY(p != m_begin)) {
                if (ARCHON_LIKELY(*p == ',')) {
                  again:
                    ++p;
                    if (ARCHON_LIKELY(p != m_end)) {
                        // FIXME: Should spaces always be allowed here?                                                        
                        if (ARCHON_LIKELY(*p == ' '))
                            goto again;
                        goto arg;
                    }
                }
                return false; // Failure
            }
            goto arg;
        }
        return false; // Failure

      arg:
        ARCHON_ASSERT(p != m_end);
        const char* begin = p;
        for (;;) {
            if (ARCHON_LIKELY(*p != '%' && *p != ',')) {
                ++p;
                if (ARCHON_LIKELY(p != m_end))
                    continue;
            }
            break;
        }
        std::string_view str = { begin, std::size_t(p - begin) };
        flt_comp_type var_2 = 0;
        if (ARCHON_LIKELY(m_value_parser.parse(str, var_2))) { // Throws
            var = var_2;
            if (p == m_end || *p != '%') {
                is_percent = false;
            }
            else {
                is_percent = true;
                ++p;
            }
            m_curr = p;
            return true; // Success
        }
        return false; // Failure
    }

    bool at_end() const noexcept
    {
        return (m_curr == m_end);
    }

private:
    core::ValueParser m_value_parser;
    const char* m_begin;
    const char* m_end;
    const char* m_curr;
};


bool parse_rgb_args(std::string_view args, bool with_alpha, util::CssColor& color)
{
    ArgsParser args_parser(args); // Throws
    flt_comp_type r = 0, g = 0, b = 0, a = 1;
    bool r_per = false, g_per = false, b_per = false, a_per = false;
    bool good = (args_parser.parse(r, r_per) && // Throws
                 args_parser.parse(g, g_per) && // Throws
                 args_parser.parse(b, b_per) && // Throws
                 (!with_alpha || args_parser.parse(a, a_per)) && // Throws
                 args_parser.at_end() &&
                 g_per == r_per &&
                 b_per == r_per &&
                 !a_per);
    if (ARCHON_LIKELY(good)) {
        if (!r_per) {
            color = util::CssColor::rgb(r, g, b, a);
        }
        else {
            color = util::CssColor::rgb_p(r, g, b, a);
        }
        return true; // Success
    }
    return false; // Failure
}


bool parse_hsl_args(std::string_view args, bool with_alpha, util::CssColor& color)
{
    ArgsParser args_parser(args); // Throws
    flt_comp_type h = 0, s = 0, l = 0, a = 1;
    bool h_per = false, s_per = false, l_per = false, a_per = false;
    bool good = (args_parser.parse(h, h_per) && // Throws
                 args_parser.parse(s, s_per) && // Throws
                 args_parser.parse(l, l_per) && // Throws
                 (!with_alpha || args_parser.parse(a, a_per)) && // Throws
                 args_parser.at_end() &&
                 !h_per &&
                 s_per &&
                 l_per &&
                 !a_per);
    if (ARCHON_LIKELY(good)) {
        color = util::CssColor::hsl(h, s, l, a);
        return true; // Success
    }
    return false; // Failure
}


} // unnamed namespace


using util::CssColor;


auto CssColor::name(std::size_t index) -> CssColor
{
    if (index < get_num_named_colors())
        return Name { index };
    throw std::out_of_range("CSS named color index");
}


auto CssColor::get_as_hex() const noexcept -> Hex
{
    switch (m_form) {
        case Form::hex:
            return m_hex;
        case Form::name:
            return get_named_color(m_name);
        case Form::rgb:
            return to_hex(m_rgb);
        case Form::rgb_p:
            return to_hex(m_rgb_p);
        case Form::hsl:
            return to_hex(m_hsl);
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}


auto CssColor::get_named_color(const Name& name) noexcept -> Hex
{
    util::Color color = g_named_colors[name.index].color;
    return { color.red(), color.green(), color.blue(), color.alpha() };
}


bool CssColor::find_named_color_by_name(std::string_view name, Name& name_2) noexcept
{
    auto begin = g_named_color_maps.name_map.begin();
    auto end   = g_named_color_maps.name_map.end();
    auto i = std::lower_bound(begin, end, name, [](const auto& a, const auto& b) noexcept {
        return a.name < b;
    });
    if (i != end && i->name == name) {
        name_2 = { i->index };
        return true;
    }
    return false;
}


bool CssColor::find_named_color_by_value(const Hex& hex, Name& name) noexcept
{
    auto begin = g_named_color_maps.value_map.begin();
    auto end   = g_named_color_maps.value_map.end();
    auto value = to_rgba(hex.r, hex.g, hex.b, hex.a);
    auto i = std::lower_bound(begin, end, value, [](const auto& a, const auto& b) noexcept {
        return a.value < b;
    });
    if (i != end && i->value == value) {
        name = { i->index };
        return true;
    }
    return false;
}


auto CssColor::get_num_named_colors() noexcept -> std::size_t
{
    return g_num_named_colors;
}


void CssColor::format(core::Buffer<char>& buffer, std::size_t& offset, FormatConfig config) const
{
    // FIXME: Actually take config.css_level into account                                                 
    core::StringBufferContents contents(buffer, offset);
    bool lowercase_hex = false;
    bool uppercase_names = false;
    bool uppercase_func = false;
    bool no_space_after_comma = false;
    switch (m_form) {
        case Form::hex:
            format_hex(m_hex.r, m_hex.g, m_hex.b, m_hex.a, config.disable_short_hex_form,
                       lowercase_hex, contents); // Throws
            goto done;
        case Form::name:
            format_name(m_name.index, uppercase_names, contents); // Throws
            goto done;
        case Form::rgb:
            format_rgb(m_rgb.r, m_rgb.g, m_rgb.b, m_rgb.a, false, uppercase_func,
                       no_space_after_comma, contents); // Throws
            goto done;
        case Form::rgb_p:
            format_rgb(m_rgb_p.r, m_rgb_p.g, m_rgb_p.b, m_rgb_p.a, true, uppercase_func,
                       no_space_after_comma, contents); // Throws
            goto done;
        case Form::hsl:
            format_hsl(m_hsl.h, m_hsl.s, m_hsl.l, m_hsl.a, uppercase_func,
                       no_space_after_comma, contents); // Throws
            goto done;
    }
    ARCHON_ASSERT_UNREACHABLE();

  done:
    offset = contents.size();
}


bool CssColor::parse(core::StringSpan<char> string, CssLevel level)
{
    static_cast<void>(level);                                                                                                                                                          

    std::string_view string_2 = { string.data(), string.size() };

    // Check for hex form
    if (ARCHON_LIKELY(string.size() > 0)) {
        char ch = string.front();
        if (ARCHON_LIKELY(ch != '#'))
            goto next_1;
        std::array<char, 8> buffer;
        buffer[6] = 'F';
        buffer[7] = 'F';
        string_2 = string_2.substr(1);
        std::size_t n = string_2.size();
        if (n == 3 || n == 4) {
            buffer[0] = string_2[0];
            buffer[1] = string_2[0];
            buffer[2] = string_2[1];
            buffer[3] = string_2[1];
            buffer[4] = string_2[2];
            buffer[5] = string_2[2];
            if (n == 4) {
                buffer[6] = string_2[3];
                buffer[7] = string_2[3];
            }
        }
        else if (n == 6 || n == 8) {
            std::copy_n(string_2.data(), 6, buffer.data());
            if (n == 8) {
                buffer[6] = string_2[6];
                buffer[7] = string_2[7];
            }
        }
        else {
            return false; // Failure (bad hex form)
        }
        const std::locale& locale = std::locale::classic();
        core::CharMapper char_mapper(locale); // Throws
        core::IntegerParser parser(char_mapper);
        using Sign = core::IntegerParser::Sign;
        std::string_view string_3 = { buffer.data(), buffer.size() };
        rgba_type value = 0;
        if (ARCHON_LIKELY(parser.parse_hex<Sign::reject>(string_3, value))) { // Throws
            int_comp_type r = int_comp_type((value >> (3 * 8)) & 0xFF);
            int_comp_type g = int_comp_type((value >> (2 * 8)) & 0xFF);
            int_comp_type b = int_comp_type((value >> (1 * 8)) & 0xFF);
            int_comp_type a = int_comp_type((value >> (0 * 8)) & 0xFF);
            *this = hex(r, g, b, a);
            return true; // Success
        }
        return false; // Failure (bad hex form)
    }

    // Check for functional forms
  next_1:
    if (ARCHON_LIKELY(string_2.size() > 0)) {
        char ch = string_2.back();
        if (ARCHON_LIKELY(ch != ')'))
            goto next_2;
        std::size_t i = 0;
        std::size_t n = string_2.size() - 1;
        for (;;) {
            if (ARCHON_LIKELY(i < n)) {
                char ch = string_2[i];
                if (ARCHON_LIKELY(ch != '(')) {
                    ++i;
                    continue;
                }
                break;
            }
            return false; // Failure (bad functional form)
        }
        std::string_view label = string_2.substr(0, i);
        std::string_view args = string_2.substr(std::size_t(i + 1), std::size_t(n - (i + 1)));
        if (ARCHON_LIKELY(label == "rgb" || label == "rgba")) {
            bool with_alpha = (label == "rgba");
            CssColor color = {};
            if (ARCHON_LIKELY(parse_rgb_args(args, with_alpha, color))) { // Throws
                *this = color;
                return true; // Success
            }
            return false; // Failure (bad functional RGB form)
        }
        if (ARCHON_LIKELY(label == "hsl" || label == "hsla")) {
            bool with_alpha = (label == "hsla");
            CssColor color = {};
            if (ARCHON_LIKELY(parse_hsl_args(args, with_alpha, color))) { // Throws
                *this = color;
                return true; // Success
            }
            return false; // Failure (bad functional HSL form)
        }
        return false; // Failure (unrecognized functional form)
    }

    // Check for named color
  next_2:
    Name name = {};
    if (ARCHON_LIKELY(find_named_color_by_name(string_2, name))) {
        *this = name;
        return true; // Success
    }

    return false; // Failure (unrecognized form)
}
