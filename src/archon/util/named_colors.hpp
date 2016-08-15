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

#ifndef ARCHON_UTIL_NAMED_COLORS_HPP
#define ARCHON_UTIL_NAMED_COLORS_HPP

#include <archon/util/packed_trgb.hpp>


namespace archon {
namespace Util {
namespace Color {

constexpr PackedTRGB aliceblue            ( 240, 248, 255 );   // #f0f8ff
constexpr PackedTRGB antiquewhite         ( 250, 235, 215 );   // #faebd7
constexpr PackedTRGB aqua                 (   0, 255, 255 );   // #00ffff
constexpr PackedTRGB aquamarine           ( 127, 255, 212 );   // #7fffd4
constexpr PackedTRGB azure                ( 240, 255, 255 );   // #f0ffff
constexpr PackedTRGB beige                ( 245, 245, 220 );   // #f5f5dc
constexpr PackedTRGB bisque               ( 255, 228, 196 );   // #ffe4c4
constexpr PackedTRGB black                (   0,   0,   0 );   // #000000
constexpr PackedTRGB blanchedalmond       ( 255, 235, 205 );   // #ffebcd
constexpr PackedTRGB blue                 (   0,   0, 255 );   // #0000ff
constexpr PackedTRGB blueviolet           ( 138,  43, 226 );   // #8a2be2
constexpr PackedTRGB brown                ( 165,  42,  42 );   // #a52a2a
constexpr PackedTRGB burlywood            ( 222, 184, 135 );   // #deb887
constexpr PackedTRGB cadetblue            (  95, 158, 160 );   // #5f9ea0
constexpr PackedTRGB chartreuse           ( 127, 255,   0 );   // #7fff00
constexpr PackedTRGB chocolate            ( 210, 105,  30 );   // #d2691e
constexpr PackedTRGB coral                ( 255, 127,  80 );   // #ff7f50
constexpr PackedTRGB cornflowerblue       ( 100, 149, 237 );   // #6495ed
constexpr PackedTRGB cornsilk             ( 255, 248, 220 );   // #fff8dc
constexpr PackedTRGB crimson              ( 220,  20,  60 );   // #dc143c
constexpr PackedTRGB cyan                 (   0, 255, 255 );   // #00ffff
constexpr PackedTRGB darkblue             (   0,   0, 139 );   // #00008b
constexpr PackedTRGB darkcyan             (   0, 139, 139 );   // #008b8b
constexpr PackedTRGB darkgoldenrod        ( 184, 134,  11 );   // #b8860b
constexpr PackedTRGB darkgray             ( 169, 169, 169 );   // #a9a9a9
constexpr PackedTRGB darkgreen            (   0, 100,   0 );   // #006400
constexpr PackedTRGB darkgrey             ( 169, 169, 169 );   // #a9a9a9
constexpr PackedTRGB darkkhaki            ( 189, 183, 107 );   // #bdb76b
constexpr PackedTRGB darkmagenta          ( 139,   0, 139 );   // #8b008b
constexpr PackedTRGB darkolivegreen       (  85, 107,  47 );   // #556b2f
constexpr PackedTRGB darkorange           ( 255, 140,   0 );   // #ff8c00
constexpr PackedTRGB darkorchid           ( 153,  50, 204 );   // #9932cc
constexpr PackedTRGB darkred              ( 139,   0,   0 );   // #8b0000
constexpr PackedTRGB darksalmon           ( 233, 150, 122 );   // #e9967a
constexpr PackedTRGB darkseagreen         ( 143, 188, 143 );   // #8fbc8f
constexpr PackedTRGB darkslateblue        (  72,  61, 139 );   // #483d8b
constexpr PackedTRGB darkslategray        (  47,  79,  79 );   // #2f4f4f
constexpr PackedTRGB darkslategrey        (  47,  79,  79 );   // #2f4f4f
constexpr PackedTRGB darkturquoise        (   0, 206, 209 );   // #00ced1
constexpr PackedTRGB darkviolet           ( 148,   0, 211 );   // #9400d3
constexpr PackedTRGB deeppink             ( 255,  20, 147 );   // #ff1493
constexpr PackedTRGB deepskyblue          (   0, 191, 255 );   // #00bfff
constexpr PackedTRGB dimgray              ( 105, 105, 105 );   // #696969
constexpr PackedTRGB dimgrey              ( 105, 105, 105 );   // #696969
constexpr PackedTRGB dodgerblue           (  30, 144, 255 );   // #1e90ff
constexpr PackedTRGB firebrick            ( 178,  34,  34 );   // #b22222
constexpr PackedTRGB floralwhite          ( 255, 250, 240 );   // #fffaf0
constexpr PackedTRGB forestgreen          (  34, 139,  34 );   // #228b22
constexpr PackedTRGB fuchsia              ( 255,   0, 255 );   // #ff00ff
constexpr PackedTRGB gainsboro            ( 220, 220, 220 );   // #dcdcdc
constexpr PackedTRGB ghostwhite           ( 248, 248, 255 );   // #f8f8ff
constexpr PackedTRGB gold                 ( 255, 215,   0 );   // #ffd700
constexpr PackedTRGB goldenrod            ( 218, 165,  32 );   // #daa520
constexpr PackedTRGB gray                 ( 128, 128, 128 );   // #808080
constexpr PackedTRGB green                (   0, 128,   0 );   // #008000
constexpr PackedTRGB greenyellow          ( 173, 255,  47 );   // #adff2f
constexpr PackedTRGB grey                 ( 128, 128, 128 );   // #808080
constexpr PackedTRGB honeydew             ( 240, 255, 240 );   // #f0fff0
constexpr PackedTRGB hotpink              ( 255, 105, 180 );   // #ff69b4
constexpr PackedTRGB indianred            ( 205,  92,  92 );   // #cd5c5c
constexpr PackedTRGB indigo               (  75,   0, 130 );   // #4b0082
constexpr PackedTRGB ivory                ( 255, 255, 240 );   // #fffff0
constexpr PackedTRGB khaki                ( 240, 230, 140 );   // #f0e68c
constexpr PackedTRGB lavender             ( 230, 230, 250 );   // #e6e6fa
constexpr PackedTRGB lavenderblush        ( 255, 240, 245 );   // #fff0f5
constexpr PackedTRGB lawngreen            ( 124, 252,   0 );   // #7cfc00
constexpr PackedTRGB lemonchiffon         ( 255, 250, 205 );   // #fffacd
constexpr PackedTRGB lightblue            ( 173, 216, 230 );   // #add8e6
constexpr PackedTRGB lightcoral           ( 240, 128, 128 );   // #f08080
constexpr PackedTRGB lightcyan            ( 224, 255, 255 );   // #e0ffff
constexpr PackedTRGB lightgoldenrodyellow ( 250, 250, 210 );   // #fafad2
constexpr PackedTRGB lightgray            ( 211, 211, 211 );   // #d3d3d3
constexpr PackedTRGB lightgreen           ( 144, 238, 144 );   // #90ee90
constexpr PackedTRGB lightgrey            ( 211, 211, 211 );   // #d3d3d3
constexpr PackedTRGB lightpink            ( 255, 182, 193 );   // #ffb6c1
constexpr PackedTRGB lightsalmon          ( 255, 160, 122 );   // #ffa07a
constexpr PackedTRGB lightseagreen        (  32, 178, 170 );   // #20b2aa
constexpr PackedTRGB lightskyblue         ( 135, 206, 250 );   // #87cefa
constexpr PackedTRGB lightslategray       ( 119, 136, 153 );   // #778899
constexpr PackedTRGB lightslategrey       ( 119, 136, 153 );   // #778899
constexpr PackedTRGB lightsteelblue       ( 176, 196, 222 );   // #b0c4de
constexpr PackedTRGB lightyellow          ( 255, 255, 224 );   // #ffffe0
constexpr PackedTRGB lime                 (   0, 255,   0 );   // #00ff00
constexpr PackedTRGB limegreen            (  50, 205,  50 );   // #32cd32
constexpr PackedTRGB linen                ( 250, 240, 230 );   // #faf0e6
constexpr PackedTRGB magenta              ( 255,   0, 255 );   // #ff00ff
constexpr PackedTRGB maroon               ( 128,   0,   0 );   // #800000
constexpr PackedTRGB mediumaquamarine     ( 102, 205, 170 );   // #66cdaa
constexpr PackedTRGB mediumblue           (   0,   0, 205 );   // #0000cd
constexpr PackedTRGB mediumorchid         ( 186,  85, 211 );   // #ba55d3
constexpr PackedTRGB mediumpurple         ( 147, 112, 219 );   // #9370db
constexpr PackedTRGB mediumseagreen       (  60, 179, 113 );   // #3cb371
constexpr PackedTRGB mediumslateblue      ( 123, 104, 238 );   // #7b68ee
constexpr PackedTRGB mediumspringgreen    (   0, 250, 154 );   // #00fa9a
constexpr PackedTRGB mediumturquoise      (  72, 209, 204 );   // #48d1cc
constexpr PackedTRGB mediumvioletred      ( 199,  21, 133 );   // #c71585
constexpr PackedTRGB midnightblue         (  25,  25, 112 );   // #191970
constexpr PackedTRGB mintcream            ( 245, 255, 250 );   // #f5fffa
constexpr PackedTRGB mistyrose            ( 255, 228, 225 );   // #ffe4e1
constexpr PackedTRGB moccasin             ( 255, 228, 181 );   // #ffe4b5
constexpr PackedTRGB navajowhite          ( 255, 222, 173 );   // #ffdead
constexpr PackedTRGB navy                 (   0,   0, 128 );   // #000080
constexpr PackedTRGB oldlace              ( 253, 245, 230 );   // #fdf5e6
constexpr PackedTRGB olive                ( 128, 128,   0 );   // #808000
constexpr PackedTRGB olivedrab            ( 107, 142,  35 );   // #6b8e23
constexpr PackedTRGB orange               ( 255, 165,   0 );   // #ffa500
constexpr PackedTRGB orangered            ( 255,  69,   0 );   // #ff4500
constexpr PackedTRGB orchid               ( 218, 112, 214 );   // #da70d6
constexpr PackedTRGB palegoldenrod        ( 238, 232, 170 );   // #eee8aa
constexpr PackedTRGB palegreen            ( 152, 251, 152 );   // #98fb98
constexpr PackedTRGB paleturquoise        ( 175, 238, 238 );   // #afeeee
constexpr PackedTRGB palevioletred        ( 219, 112, 147 );   // #db7093
constexpr PackedTRGB papayawhip           ( 255, 239, 213 );   // #ffefd5
constexpr PackedTRGB peachpuff            ( 255, 218, 185 );   // #ffdab9
constexpr PackedTRGB peru                 ( 205, 133,  63 );   // #cd853f
constexpr PackedTRGB pink                 ( 255, 192, 203 );   // #ffc0cb
constexpr PackedTRGB plum                 ( 221, 160, 221 );   // #dda0dd
constexpr PackedTRGB powderblue           ( 176, 224, 230 );   // #b0e0e6
constexpr PackedTRGB purple               ( 128,   0, 128 );   // #800080
constexpr PackedTRGB red                  ( 255,   0,   0 );   // #ff0000
constexpr PackedTRGB rosybrown            ( 188, 143, 143 );   // #bc8f8f
constexpr PackedTRGB royalblue            (  65, 105, 225 );   // #4169e1
constexpr PackedTRGB saddlebrown          ( 139,  69,  19 );   // #8b4513
constexpr PackedTRGB salmon               ( 250, 128, 114 );   // #fa8072
constexpr PackedTRGB sandybrown           ( 244, 164,  96 );   // #f4a460
constexpr PackedTRGB seagreen             (  46, 139,  87 );   // #2e8b57
constexpr PackedTRGB seashell             ( 255, 245, 238 );   // #fff5ee
constexpr PackedTRGB sienna               ( 160,  82,  45 );   // #a0522d
constexpr PackedTRGB silver               ( 192, 192, 192 );   // #c0c0c0
constexpr PackedTRGB skyblue              ( 135, 206, 235 );   // #87ceeb
constexpr PackedTRGB slateblue            ( 106,  90, 205 );   // #6a5acd
constexpr PackedTRGB slategray            ( 112, 128, 144 );   // #708090
constexpr PackedTRGB slategrey            ( 112, 128, 144 );   // #708090
constexpr PackedTRGB snow                 ( 255, 250, 250 );   // #fffafa
constexpr PackedTRGB springgreen          (   0, 255, 127 );   // #00ff7f
constexpr PackedTRGB steelblue            (  70, 130, 180 );   // #4682b4
constexpr PackedTRGB tan                  ( 210, 180, 140 );   // #d2b48c
constexpr PackedTRGB teal                 (   0, 128, 128 );   // #008080
constexpr PackedTRGB thistle              ( 216, 191, 216 );   // #d8bfd8
constexpr PackedTRGB tomato               ( 255,  99,  71 );   // #ff6347
constexpr PackedTRGB turquoise            (  64, 224, 208 );   // #40e0d0
constexpr PackedTRGB violet               ( 238, 130, 238 );   // #ee82ee
constexpr PackedTRGB wheat                ( 245, 222, 179 );   // #f5deb3
constexpr PackedTRGB white                ( 255, 255, 255 );   // #ffffff
constexpr PackedTRGB whitesmoke           ( 245, 245, 245 );   // #f5f5f5
constexpr PackedTRGB yellow               ( 255, 255,   0 );   // #ffff00
constexpr PackedTRGB transparent          (0,0,0,0);

} // namespace Color
} // namespace Util
} // namespace archon

#endif // ARCHON_UTIL_NAMED_COLORS_HPP
