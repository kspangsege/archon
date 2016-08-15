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

#ifndef ARCHON_FONT_LAYOUT_CFG_HPP
#define ARCHON_FONT_LAYOUT_CFG_HPP

#include <archon/core/config.hpp>
#include <archon/font/text_format.hpp>


namespace archon
{
  namespace font
  {
    struct LayoutConfig
    {
      /**
       * Specifies how multiple lines are aligned against each
       * other. A value of 0 means that the beginnings of each line
       * are aligned, a value of 0.5 means that the centers are
       * aligned, and a value of 1 means that the ends are
       * aligned. Values outside this range are also allowed.
       *
       * The default is 0.
       */
      double align;

      /**
       * Set to 'no' to disable word wrapping. Set to 'yes' to enable
       * it, and set to 'justify' to also enable justification after
       * word wrapping.
       *
       * The default is 'yes'.
       */
      TextFormatter::WordWrapEnum word_wrap;

      /**
       * Break overlong lines into pieces. If word wrapping is also
       * enabled, line wrapping kicks in afterwards, and breaks words
       * that are too long to fit on a line by themselves.
       *
       * The default is 'false'.
       */
      bool line_wrap;

      /**
       * Break overlong pages into several pieces.
       *
       * The default is 'false'.
       */
      bool page_wrap;

      /**
       * Modify the line height (or line width for a vertical layout)
       * by the specified factor.
       *
       * The default is 1.
       */
      double line_spacing;

      /**
       * Extra spacing for space characters. This affects normal space
       * (U+0020), non-breaking space (U+00A0), and ideographic space
       * (U+3000). When disregarding justification, the final width of
       * a space character is its normal width plus the letter spacing
       * plus the value set with this method.
       *
       * The default is 0.
       */
      double word_spacing;

      /**
       * Extra spacing between letters in number of pixels. May be
       * fractional, and may be negative.
       *
       * The default is 0.
       */
      double letter_spacing;

      /**
       * Construct a vertical layout as opposed to a horizontal one.
       *
       * The default is 'horizontal'.
       */
      bool vertical;

      /**
       * Make the layout run from right to left instead of from left
       * to right. For a horizontal layout this means that the first
       * character on a line is the rightmost one. For a vertical
       * layout it means that the first line is the rightmost one.
       *
       * The default is left-to-right.
       */
      bool right_to_left;

      /**
       * Make the layout run from bottom to top instead of from top to
       * bottom. For a horizontal layout this means that the first
       * line is the bottom-most one. For a vertical layout it means
       * that the first character on a line is the bottom-most one.
       *
       * The default is top-to-bottom.
       */
      bool bottom_to_top;

      /**
       * This modifies the distance between some glyphs (e.g. 'AV') to
       * improve quality.
       *
       * The default is 'true'.
       */
      bool kerning;


      void populate(core::ConfigBuilder &cfg);


      void apply_to(TextFormatter &formatter) const;

      LayoutConfig();
    };
  }
}

#endif // ARCHON_FONT_LAYOUT_CFG_HPP
