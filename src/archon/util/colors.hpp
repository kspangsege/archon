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

#ifndef ARCHON_X_UTIL_X_COLORS_HPP
#define ARCHON_X_UTIL_X_COLORS_HPP

/// \file


#include <archon/util/color.hpp>


/// \brief Named colors.
///
/// This namespace contains a number of named colors (RGBA) corresponding to the named
/// colors of CSS Level 3.
///
/// \sa https://www.w3.org/TR/css-color-3/.
///
namespace archon::util::colors {


inline constexpr auto transparent          = util::Color::from_trgb(0xFF000000);
inline constexpr auto aliceblue            = util::Color::from_trgb(0x00F0F8FF);
inline constexpr auto antiquewhite         = util::Color::from_trgb(0x00FAEBD7);
inline constexpr auto aqua                 = util::Color::from_trgb(0x0000FFFF);
inline constexpr auto aquamarine           = util::Color::from_trgb(0x007FFFD4);
inline constexpr auto azure                = util::Color::from_trgb(0x00F0FFFF);
inline constexpr auto beige                = util::Color::from_trgb(0x00F5F5DC);
inline constexpr auto bisque               = util::Color::from_trgb(0x00FFE4C4);
inline constexpr auto black                = util::Color::from_trgb(0x00000000);
inline constexpr auto blanchedalmond       = util::Color::from_trgb(0x00FFEBCD);
inline constexpr auto blue                 = util::Color::from_trgb(0x000000FF);
inline constexpr auto blueviolet           = util::Color::from_trgb(0x008A2BE2);
inline constexpr auto brown                = util::Color::from_trgb(0x00A52A2A);
inline constexpr auto burlywood            = util::Color::from_trgb(0x00DEB887);
inline constexpr auto cadetblue            = util::Color::from_trgb(0x005F9EA0);
inline constexpr auto chartreuse           = util::Color::from_trgb(0x007FFF00);
inline constexpr auto chocolate            = util::Color::from_trgb(0x00D2691E);
inline constexpr auto coral                = util::Color::from_trgb(0x00FF7F50);
inline constexpr auto cornflowerblue       = util::Color::from_trgb(0x006495ED);
inline constexpr auto cornsilk             = util::Color::from_trgb(0x00FFF8DC);
inline constexpr auto crimson              = util::Color::from_trgb(0x00DC143C);
inline constexpr auto cyan                 = util::Color::from_trgb(0x0000FFFF);
inline constexpr auto darkblue             = util::Color::from_trgb(0x0000008B);
inline constexpr auto darkcyan             = util::Color::from_trgb(0x00008B8B);
inline constexpr auto darkgoldenrod        = util::Color::from_trgb(0x00B8860B);
inline constexpr auto darkgray             = util::Color::from_trgb(0x00A9A9A9);
inline constexpr auto darkgreen            = util::Color::from_trgb(0x00006400);
inline constexpr auto darkgrey             = util::Color::from_trgb(0x00A9A9A9);
inline constexpr auto darkkhaki            = util::Color::from_trgb(0x00BDB76B);
inline constexpr auto darkmagenta          = util::Color::from_trgb(0x008B008B);
inline constexpr auto darkolivegreen       = util::Color::from_trgb(0x00556B2F);
inline constexpr auto darkorange           = util::Color::from_trgb(0x00FF8C00);
inline constexpr auto darkorchid           = util::Color::from_trgb(0x009932CC);
inline constexpr auto darkred              = util::Color::from_trgb(0x008B0000);
inline constexpr auto darksalmon           = util::Color::from_trgb(0x00E9967A);
inline constexpr auto darkseagreen         = util::Color::from_trgb(0x008FBC8F);
inline constexpr auto darkslateblue        = util::Color::from_trgb(0x00483D8B);
inline constexpr auto darkslategray        = util::Color::from_trgb(0x002F4F4F);
inline constexpr auto darkslategrey        = util::Color::from_trgb(0x002F4F4F);
inline constexpr auto darkturquoise        = util::Color::from_trgb(0x0000CED1);
inline constexpr auto darkviolet           = util::Color::from_trgb(0x009400D3);
inline constexpr auto deeppink             = util::Color::from_trgb(0x00FF1493);
inline constexpr auto deepskyblue          = util::Color::from_trgb(0x0000BFFF);
inline constexpr auto dimgray              = util::Color::from_trgb(0x00696969);
inline constexpr auto dimgrey              = util::Color::from_trgb(0x00696969);
inline constexpr auto dodgerblue           = util::Color::from_trgb(0x001E90FF);
inline constexpr auto firebrick            = util::Color::from_trgb(0x00B22222);
inline constexpr auto floralwhite          = util::Color::from_trgb(0x00FFFAF0);
inline constexpr auto forestgreen          = util::Color::from_trgb(0x00228B22);
inline constexpr auto fuchsia              = util::Color::from_trgb(0x00FF00FF);
inline constexpr auto gainsboro            = util::Color::from_trgb(0x00DCDCDC);
inline constexpr auto ghostwhite           = util::Color::from_trgb(0x00F8F8FF);
inline constexpr auto gold                 = util::Color::from_trgb(0x00FFD700);
inline constexpr auto goldenrod            = util::Color::from_trgb(0x00DAA520);
inline constexpr auto gray                 = util::Color::from_trgb(0x00808080);
inline constexpr auto green                = util::Color::from_trgb(0x00008000);
inline constexpr auto greenyellow          = util::Color::from_trgb(0x00ADFF2F);
inline constexpr auto grey                 = util::Color::from_trgb(0x00808080);
inline constexpr auto honeydew             = util::Color::from_trgb(0x00F0FFF0);
inline constexpr auto hotpink              = util::Color::from_trgb(0x00FF69B4);
inline constexpr auto indianred            = util::Color::from_trgb(0x00CD5C5C);
inline constexpr auto indigo               = util::Color::from_trgb(0x004B0082);
inline constexpr auto ivory                = util::Color::from_trgb(0x00FFFFF0);
inline constexpr auto khaki                = util::Color::from_trgb(0x00F0E68C);
inline constexpr auto lavender             = util::Color::from_trgb(0x00E6E6FA);
inline constexpr auto lavenderblush        = util::Color::from_trgb(0x00FFF0F5);
inline constexpr auto lawngreen            = util::Color::from_trgb(0x007CFC00);
inline constexpr auto lemonchiffon         = util::Color::from_trgb(0x00FFFACD);
inline constexpr auto lightblue            = util::Color::from_trgb(0x00ADD8E6);
inline constexpr auto lightcoral           = util::Color::from_trgb(0x00F08080);
inline constexpr auto lightcyan            = util::Color::from_trgb(0x00E0FFFF);
inline constexpr auto lightgoldenrodyellow = util::Color::from_trgb(0x00FAFAD2);
inline constexpr auto lightgray            = util::Color::from_trgb(0x00D3D3D3);
inline constexpr auto lightgreen           = util::Color::from_trgb(0x0090EE90);
inline constexpr auto lightgrey            = util::Color::from_trgb(0x00D3D3D3);
inline constexpr auto lightpink            = util::Color::from_trgb(0x00FFB6C1);
inline constexpr auto lightsalmon          = util::Color::from_trgb(0x00FFA07A);
inline constexpr auto lightseagreen        = util::Color::from_trgb(0x0020B2AA);
inline constexpr auto lightskyblue         = util::Color::from_trgb(0x0087CEFA);
inline constexpr auto lightslategray       = util::Color::from_trgb(0x00778899);
inline constexpr auto lightslategrey       = util::Color::from_trgb(0x00778899);
inline constexpr auto lightsteelblue       = util::Color::from_trgb(0x00B0C4DE);
inline constexpr auto lightyellow          = util::Color::from_trgb(0x00FFFFE0);
inline constexpr auto lime                 = util::Color::from_trgb(0x0000FF00);
inline constexpr auto limegreen            = util::Color::from_trgb(0x0032CD32);
inline constexpr auto linen                = util::Color::from_trgb(0x00FAF0E6);
inline constexpr auto magenta              = util::Color::from_trgb(0x00FF00FF);
inline constexpr auto maroon               = util::Color::from_trgb(0x00800000);
inline constexpr auto mediumaquamarine     = util::Color::from_trgb(0x0066CDAA);
inline constexpr auto mediumblue           = util::Color::from_trgb(0x000000CD);
inline constexpr auto mediumorchid         = util::Color::from_trgb(0x00BA55D3);
inline constexpr auto mediumpurple         = util::Color::from_trgb(0x009370DB);
inline constexpr auto mediumseagreen       = util::Color::from_trgb(0x003CB371);
inline constexpr auto mediumslateblue      = util::Color::from_trgb(0x007B68EE);
inline constexpr auto mediumspringgreen    = util::Color::from_trgb(0x0000FA9A);
inline constexpr auto mediumturquoise      = util::Color::from_trgb(0x0048D1CC);
inline constexpr auto mediumvioletred      = util::Color::from_trgb(0x00C71585);
inline constexpr auto midnightblue         = util::Color::from_trgb(0x00191970);
inline constexpr auto mintcream            = util::Color::from_trgb(0x00F5FFFA);
inline constexpr auto mistyrose            = util::Color::from_trgb(0x00FFE4E1);
inline constexpr auto moccasin             = util::Color::from_trgb(0x00FFE4B5);
inline constexpr auto navajowhite          = util::Color::from_trgb(0x00FFDEAD);
inline constexpr auto navy                 = util::Color::from_trgb(0x00000080);
inline constexpr auto oldlace              = util::Color::from_trgb(0x00FDF5E6);
inline constexpr auto olive                = util::Color::from_trgb(0x00808000);
inline constexpr auto olivedrab            = util::Color::from_trgb(0x006B8E23);
inline constexpr auto orange               = util::Color::from_trgb(0x00FFA500);
inline constexpr auto orangered            = util::Color::from_trgb(0x00FF4500);
inline constexpr auto orchid               = util::Color::from_trgb(0x00DA70D6);
inline constexpr auto palegoldenrod        = util::Color::from_trgb(0x00EEE8AA);
inline constexpr auto palegreen            = util::Color::from_trgb(0x0098FB98);
inline constexpr auto paleturquoise        = util::Color::from_trgb(0x00AFEEEE);
inline constexpr auto palevioletred        = util::Color::from_trgb(0x00DB7093);
inline constexpr auto papayawhip           = util::Color::from_trgb(0x00FFEFD5);
inline constexpr auto peachpuff            = util::Color::from_trgb(0x00FFDAB9);
inline constexpr auto peru                 = util::Color::from_trgb(0x00CD853F);
inline constexpr auto pink                 = util::Color::from_trgb(0x00FFC0CB);
inline constexpr auto plum                 = util::Color::from_trgb(0x00DDA0DD);
inline constexpr auto powderblue           = util::Color::from_trgb(0x00B0E0E6);
inline constexpr auto purple               = util::Color::from_trgb(0x00800080);
inline constexpr auto red                  = util::Color::from_trgb(0x00FF0000);
inline constexpr auto rosybrown            = util::Color::from_trgb(0x00BC8F8F);
inline constexpr auto royalblue            = util::Color::from_trgb(0x004169E1);
inline constexpr auto saddlebrown          = util::Color::from_trgb(0x008B4513);
inline constexpr auto salmon               = util::Color::from_trgb(0x00FA8072);
inline constexpr auto sandybrown           = util::Color::from_trgb(0x00F4A460);
inline constexpr auto seagreen             = util::Color::from_trgb(0x002E8B57);
inline constexpr auto seashell             = util::Color::from_trgb(0x00FFF5EE);
inline constexpr auto sienna               = util::Color::from_trgb(0x00A0522D);
inline constexpr auto silver               = util::Color::from_trgb(0x00C0C0C0);
inline constexpr auto skyblue              = util::Color::from_trgb(0x0087CEEB);
inline constexpr auto slateblue            = util::Color::from_trgb(0x006A5ACD);
inline constexpr auto slategray            = util::Color::from_trgb(0x00708090);
inline constexpr auto slategrey            = util::Color::from_trgb(0x00708090);
inline constexpr auto snow                 = util::Color::from_trgb(0x00FFFAFA);
inline constexpr auto springgreen          = util::Color::from_trgb(0x0000FF7F);
inline constexpr auto steelblue            = util::Color::from_trgb(0x004682B4);
inline constexpr auto tan                  = util::Color::from_trgb(0x00D2B48C);
inline constexpr auto teal                 = util::Color::from_trgb(0x00008080);
inline constexpr auto thistle              = util::Color::from_trgb(0x00D8BFD8);
inline constexpr auto tomato               = util::Color::from_trgb(0x00FF6347);
inline constexpr auto turquoise            = util::Color::from_trgb(0x0040E0D0);
inline constexpr auto violet               = util::Color::from_trgb(0x00EE82EE);
inline constexpr auto wheat                = util::Color::from_trgb(0x00F5DEB3);
inline constexpr auto white                = util::Color::from_trgb(0x00FFFFFF);
inline constexpr auto whitesmoke           = util::Color::from_trgb(0x00F5F5F5);
inline constexpr auto yellow               = util::Color::from_trgb(0x00FFFF00);


} // namespace archon::util::colors

#endif // ARCHON_X_UTIL_X_COLORS_HPP
