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

#ifndef ARCHON_CORE_TERM_HPP
#define ARCHON_CORE_TERM_HPP

#include <stdexcept>
#include <string>
#include <utility>

namespace Archon
{
  namespace Core
  {
    /**
     * Provides various terminal features.
     */
    namespace Term
    {
      /**
       * Thrown by some functions to indicate that this process has no
       * controlling terminal.
       */
      struct NoTerminalException: std::runtime_error
      {
        NoTerminalException(std::string m): std::runtime_error(m) {}
      };

      /**
       * Returns the width and height of the controlling terminal in
       * number of characters. If this information is not avaible,
       * because the process has no controlling terminal, \c
       * NoTerminalException is thrown. On system where this
       * infomration is not available <tt>(80, 25)</tt> is returned.
       *
       * \return A pair <tt>(width, height)</tt> indicating the width
       * and height of the controlling terminal.
       */
      std::pair<int, int> get_terminal_size();

      enum AnsiColor
      {
        color_Black,
        color_Red,
        color_Green,
        color_Yellow,
        color_Blue,
        color_Magenta,
        color_Cyan,
        color_White,
        color_Default
      };

      struct AnsiAttributes
      {
        bool reverse, bold;
        AnsiColor fg_color, bg_color;

        static std::string get_reset_seq()
        {
          return "\033[0m";
        }

        static std::string get_reverse_seq(bool reverse = true)
        {
          return reverse ? "\033[7m" : "\033[27m";
        }

        static std::string get_bold_seq(bool bold = true)
        {
          return bold ? "\033[1m" : "\033[22m";
        }

        static std::string get_fg_color_seq(AnsiColor color)
        {
          switch(color)
          {
          case color_Black:   return "\033[30m";
          case color_Red:     return "\033[31m";
          case color_Green:   return "\033[32m";
          case color_Yellow:  return "\033[33m";
          case color_Blue:    return "\033[34m";
          case color_Magenta: return "\033[35m";
          case color_Cyan:    return "\033[36m";
          case color_White:   return "\033[37m";
          case color_Default: return "\033[39m";
          }
          throw std::runtime_error("Unexpected terminal ANSI color");
        }

        static std::string get_bg_color_seq(AnsiColor color)
        {
          switch(color)
          {
          case color_Black:   return "\033[40m";
          case color_Red:     return "\033[41m";
          case color_Green:   return "\033[42m";
          case color_Yellow:  return "\033[43m";
          case color_Blue:    return "\033[44m";
          case color_Magenta: return "\033[45m";
          case color_Cyan:    return "\033[46m";
          case color_White:   return "\033[47m";
          case color_Default: return "\033[49m";
          }
          throw std::runtime_error("Unexpected terminal ANSI color");
        }

        std::string update(AnsiAttributes a)
        {
          bool const need_reverse =  reverse  != a.reverse;
          bool const need_bold    =  bold     != a.bold;
          bool const need_fg_color = fg_color != a.fg_color;
          bool const need_bg_color = bg_color != a.bg_color;
          *this = a;

          bool const cst_fg_color = fg_color != color_Default;
          bool const cst_bg_color = bg_color != color_Default;

          int const changes =
            (need_reverse?1:0) + (need_bold?1:0) + (need_fg_color?1:0) + (need_bg_color?1:0);
          int const inits = 1 + (reverse?1:0) + (bold?1:0) + (cst_fg_color?1:0) + (cst_bg_color?1:0);
          if(changes < inits)
          {
            std::string s;
            if(need_reverse)  s += get_reverse_seq(reverse);
            if(need_bold)     s += get_bold_seq(bold);
            if(need_fg_color) s += get_fg_color_seq(fg_color);
            if(need_bg_color) s += get_bg_color_seq(bg_color);
            return s;
          }

          std::string s = get_reset_seq();
          if(reverse)      s += get_reverse_seq();
          if(bold)         s += get_bold_seq();
          if(cst_fg_color) s += get_fg_color_seq(fg_color);
          if(cst_bg_color) s += get_bg_color_seq(bg_color);
          return s;
        }

        AnsiAttributes():
          reverse(false), bold(false), fg_color(color_Default), bg_color(color_Default) {}
      };
    }
  }
}

#endif // ARCHON_CORE_TERM_HPP
