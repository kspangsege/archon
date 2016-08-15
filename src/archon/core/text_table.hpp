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

#ifndef ARCHON_CORE_TEXT_TABLE_HPP
#define ARCHON_CORE_TEXT_TABLE_HPP

#include <limits>
#include <locale>
#include <string>
#include <vector>

#include <archon/core/char_enc.hpp>
#include <archon/core/text.hpp>
#include <archon/core/term.hpp>


namespace archon
{
  namespace core
  {
    namespace Text
    {
      template<typename Ch> struct BasicTable
      {
        typedef Ch CharType;
        typedef std::basic_string<CharType> StringType;

        BasicTable(bool enable_ansi_term_attr = true);


      private:
        struct AttrNodeBase
        {
          Term::AnsiAttributes attr;
          bool reverse_set, bold_set, fg_color_set, bg_color_set;
          AttrNodeBase():
            reverse_set(false), bold_set(false), fg_color_set(false), bg_color_set(false) {}
          void apply_to(Term::AnsiAttributes &a) const
          {
            if(reverse_set) a.reverse = attr.reverse;
            if(bold_set)    a.bold    = attr.bold;
            if(fg_color_set) a.fg_color = attr.fg_color;
            if(bg_color_set) a.bg_color = attr.bg_color;
          }
        };


      public:
        template<class F> struct AttrNode: AttrNodeBase
        {
          F &set_reverse(bool reverse = true)
          {
            this->attr.reverse = reverse;
            this->reverse_set = true;
            return static_cast<F &>(*this);
          }

          F &set_bold(bool bold = true)
          {
            this->attr.bold = bold;
            this->bold_set = true;
            return static_cast<F &>(*this);
          }

          F &set_fg_color(Term::AnsiColor color)
          {
            this->attr.fg_color = color;
            this->fg_color_set = true;
            return static_cast<F &>(*this);
          }

          F &set_bg_color(Term::AnsiColor color)
          {
            this->attr.bg_color = color;
            this->bg_color_set = true;
            return static_cast<F &>(*this);
          }
        };


        struct Attr: AttrNode<Attr> {};


        struct Cell: AttrNode<Cell>
        {
          Cell &set_text(StringType t)
          {
            text = t;
            return *this;
          }

          template<typename T> Cell &set_val(T t)
          {
            text = Text::print(t);
            return *this;
          }

        private:
          friend struct BasicTable;

          StringType text;
        };


        struct Row: AttrNode<Row>
        {
        private:
          friend struct BasicTable;

          std::vector<Cell> cells;
        };


        struct Col: AttrNode<Col>
        {
          Col &set_width(double width)
          {
            desired_width = width;
            return *this;
          }

        private:
          friend struct BasicTable;

          double desired_width;
        };


        Cell &get_cell(int row, int col)
        {
          if(col < 0 || 32768 <= col) throw std::range_error("Table column index out of range");
          Row &r = get_row(row);
          // Expand as necessary
          if(r.cells.size() <= size_t(col)) r.cells.resize(col+1);
          return r.cells[col];
        };

        Row &get_row(int row)
        {
          if(row < 0 || 32768 <= row) throw std::range_error("Table row index out of range");
          // Expand as necessary
          if(rows.size() <= size_t(row)) rows.resize(row+1);
          return rows[row];
        }

        Col &get_col(int col)
        {
          if(col < 0 || 32768 <= col) throw std::range_error("Table column index out of range");
          // Expand as necessary
          if(columns.size() <= size_t(col)) columns.resize(col+1);
          return columns[col];
        }

        Attr &get_table_attr()   { return table_attr;  }
        Attr &get_odd_row_attr() { return odd_row_attr; }
        Attr &get_odd_col_attr() { return odd_col_attr; }


        StringType print(int max_total_width = 0,
                         int column_spacing = 2,
                         bool header = false,
                         std::locale const & = std::locale("")) const;


      private:
        static int width(StringType, CharType);
        static int height(StringType, CharType);
        static StringType extract_line(StringType, int, CharType);

        bool const enable_ansi_term_attr;
        Attr table_attr, odd_row_attr, odd_col_attr;
        std::vector<Col> columns;
        std::vector<Row> rows;
      };


      typedef BasicTable<char>    Table;
      typedef BasicTable<wchar_t> WideTable;






      // Template implementations:

      template<typename Ch>
      BasicTable<Ch>::BasicTable(bool enable_ansi_term_attr):
        enable_ansi_term_attr(enable_ansi_term_attr) {}

      template<typename Ch> typename BasicTable<Ch>::StringType
      BasicTable<Ch>::print(int max_table_width, int col_spacing,
                            bool header, std::locale const &loc) const
      {
        if(col_spacing < 0) col_spacing = 0;

        // Widen some fixed strings
        BasicLocaleCharMapper<CharType> mapper(loc);
        StringType const nl   = mapper.widen("\n");
        StringType const sp   = mapper.widen(" ");
        StringType const dash = mapper.widen("-");

        // Determine number of columns
        size_t cols = columns.size();
        for(size_t i=0; i<rows.size(); ++i)
          if(cols < rows[i].cells.size()) cols = rows[i].cells.size();

        if(!cols) return StringType();

        // Produce desired column width fractions
        double sum = 0;
        int n = 0;
        for(size_t i=0; i<columns.size(); ++i)
          if(0 < columns[i].desired_width)
          {
            sum += columns[i].desired_width;
            ++n;
          }
        double average = n ? sum/n : 1;
        sum += (cols-n)*average;
        std::vector<double> col_width_fracs(cols);
        for(size_t i=0; i<cols; ++i)
          col_width_fracs[i] = (i<columns.size() &&  0 < columns[i].desired_width ?
                                columns[i].desired_width : average)/sum;

        // Calculate actual column widths
        std::vector<int> col_widths(cols);
        for(size_t i=0; i<rows.size(); ++i)
        {
          Row const &r = rows[i];
          for(size_t j=0; j<r.cells.size(); ++j)
          {
            Cell const &c = r.cells[j];
            int w = width(c.text, nl[0]);
            if(col_widths[j] < w) col_widths[j] = w;
          }
        }

        // Calculate total width
        int cell_width_sum = 0;
        for(size_t i=0; i<cols; ++i) cell_width_sum += col_widths[i];
        int table_width = cell_width_sum + (cols-1) * col_spacing;

        bool reformat = 0 < max_table_width && max_table_width < table_width;
        if(reformat)
        {
          // Calculate column excesses
          int max_cell_width_sum = max_table_width - int(cols-1) * col_spacing;
          std::vector<double> excess(cols);
          for(size_t i=0; i<cols; ++i)
            excess[i] = col_widths[i] - max_cell_width_sum * col_width_fracs[i];
          while(max_table_width < table_width)
          {
            // Find column with greatest excess
            double e = -std::numeric_limits<double>::infinity();
            int j = -1;
            for(size_t i=0; i<cols; ++i) if(e < excess[i]) e = excess[j = i];
            // Reduce column width by one character
            if(1 < col_widths[j]) --col_widths[j];
            --excess[j];
            --table_width;
          }
        }

        // Render table
        StringType buffer;
        Term::AnsiAttributes reset_attr, running_attr;
        if(enable_ansi_term_attr)
          buffer += mapper.widen(Term::AnsiAttributes::get_reset_seq());

        for(size_t i=0; i<rows.size(); ++i)
        {
          Row const &row = rows[i];
          std::vector<StringType> formatted_row(cols);

          // Format each cell in the row, and determine number of text
          // lines required by this row. We want at least one line,
          // even when all the cells are empty.
          int h = 1;
          for(size_t j=0; j<cols; ++j)
          {
            StringType t = j<row.cells.size() ? row.cells[j].text : StringType();
            if(reformat) t = format(t, col_widths[j], loc);
            formatted_row[j] = t;
            int _h = height(t, nl[0]);
            if(h < _h) h = _h;
          }

          Term::AnsiAttributes row_attr;
          table_attr.apply_to(row_attr);
          if(!(i%2)) odd_row_attr.apply_to(row_attr);
          row.apply_to(row_attr);

          for(int l=0; l<h; ++l)
          {
            for(size_t j=0; j<cols; ++j)
            {
              if(enable_ansi_term_attr)
              {
                Term::AnsiAttributes a = row_attr;
                if(!(j%2)) odd_col_attr.apply_to(a);
                if(j < columns.size()) columns[j].apply_to(a);
                if(j < row.cells.size()) row.cells[j].apply_to(a);
                buffer += mapper.widen(running_attr.update(a));
              }

              StringType cell_line = extract_line(formatted_row[j], l, nl[0]);
              buffer += cell_line;

              if(!enable_ansi_term_attr && j == cols-1) continue;

              int w = cell_line.length();
              while(w<col_widths[j])
              {
                buffer += sp;
                ++w;
              }

              // Reset ANSI attributes to table level if there is
              // color spacing or if this is the last column
              if(enable_ansi_term_attr && (col_spacing || j == cols-1))
                buffer += mapper.widen(running_attr.update(j < cols-1 ? row_attr : reset_attr));

              // Output column spacing if this is not the last column
              if(j<cols-1) for(int k=0; k<col_spacing; ++k) buffer += sp;
            }

            buffer += nl;
          }

          if(header && !i)
          {
            for(int j=0; j<table_width; ++j) buffer += dash;
            buffer += nl;
          }
        }

        return buffer;
      }

      template<typename Ch> int BasicTable<Ch>::width(StringType s, CharType nl)
      {
        int w = 0;
        size_t p = 0;
        do
        {
          size_t _p = s.find(nl, p);
          if(_p == StringType::npos) _p = s.size();
          int _w = _p - p;
          if(w < _w) w = _w;
          p = _p + 1;
        }
        while(p <= s.size());
        return w;
      }

      template<typename Ch> int BasicTable<Ch>::height(StringType s, CharType nl)
      {
        int h = 0, p = 0;
        for(;;)
        {
          size_t _p = s.find(nl, p);
          if(_p == StringType::npos)
          {
            if(0 < s.size() - p) ++h;
            break;
          }
          ++h;
          p = _p + 1;
        }

        return h;
      }

      template<typename Ch> typename BasicTable<Ch>::StringType
      BasicTable<Ch>::extract_line(StringType s, int i, CharType nl)
      {
        size_t p = 0;
        while(0 < i)
        {
          size_t _p = s.find(nl, p);
          if(_p == StringType::npos) return StringType(); // Actually we should throw an exception
          p = _p + 1;
          --i;
        }
        size_t q = s.find(nl, p);
        if(q == StringType::npos) q = s.size();
        if(p == s.size()) return StringType(); // Actually we should throw an exception
        return s.substr(p, q-p);
      }
    }
  }
}

#endif // ARCHON_CORE_TEXT_TABLE_HPP
