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

#include <archon/font/layout_cfg.hpp>

using namespace std;
using namespace Archon::Core;


namespace Archon
{
  namespace Font
  {
    void LayoutConfig::populate(ConfigBuilder &cfg)
    {
      cfg.add_param("a", "align", align,
                    "Specifies how multiple lines are aligned against each other. "
                    "A value of 0 means that the beginnings of each line are aligned, "
                    "a value of 0.5 means that the centers are aligned, and a value of 1 means "
                    "that the ends are aligned. Values outside this range are also allowed.");
      cfg.add_param("w", "word-wrap", word_wrap,
                    "Set to 'no' to disable word wrapping. Set to 'yes' to enable it, and set "
                    "to 'justify' to also enable justification after word wrapping.");
      cfg.add_param("W", "line-wrap", line_wrap,
                    "Break overlong lines into pieces. If word wrapping is also enabled, line "
                    "wrapping kicks in afterwards, and breaks words that are too long to fit "
                    "on a line by themselves.");
      cfg.add_param("r", "page-wrap", page_wrap,
                    "Break overlong pages into several pieces.");
      cfg.add_param("L", "line-spacing", line_spacing,
                    "Modify the line height (or line width for a vertical layout) by "
                    "the specified factor.");
      cfg.add_param("E", "word-spacing", word_spacing,
                    "Extra spacing for space characters. This affects normal space (U+0020), "
                    "non-breaking space (U+00A0), and ideographic space (U+3000). When disregarding "
                    "justification, the final width of a space character is its normal width plus "
                    "the letter spacing plus the value set with this method.");
      cfg.add_param("e", "letter-spacing", letter_spacing,
                    "Extra spacing between letters in number of pixels. "
                    "May be fractional, and may be negative.");
      cfg.add_param("v", "vertical", vertical,
                    "Construct a vertical layout as opposed to a horizontal one.");
      cfg.add_param("R", "right-to-left", right_to_left,
                    "Make the layout run from right to left instead of from left to right. "
                    "For a horizontal layout this means that the first character on a line "
                    "is the rightmost one. For a vertical layout it means that "
                    "the first line is the rightmost one.");
      cfg.add_param("B", "bottom-to-top", bottom_to_top,
                    "Make the layout run from bottom to top instead of from top to bottom. "
                    "For a horizontal layout this means that the first line is the bottom-most one. "
                    "For a vertical layout it means that the first character on a line "
                    "is the bottom-most one.");
      cfg.add_param("k", "kerning", kerning, "Enable kerning. "
                    "This modifies the distance between some glyphs (e.g. 'AV') to improve quality.");
    }


    void LayoutConfig::apply_to(TextFormatter &formatter) const
    {
      formatter.set_alignment(align);
      formatter.set_word_wrap_mode(word_wrap);
      formatter.enable_line_wrapping(line_wrap);
      formatter.enable_page_wrapping(page_wrap);
      formatter.set_line_spacing(line_spacing);
      formatter.set_word_spacing(word_spacing);
      formatter.set_letter_spacing(letter_spacing);
      formatter.set_layout_direction(!vertical, !right_to_left, !bottom_to_top);
      formatter.enable_kerning(kerning);
    }


    LayoutConfig::LayoutConfig():
      align(0), word_wrap(TextFormatter::word_wrap_Yes),
      line_wrap(false), page_wrap(false), line_spacing(1), word_spacing(0), letter_spacing(0),
      vertical(false), right_to_left(false), bottom_to_top(false), kerning(true) {}
  }
}
