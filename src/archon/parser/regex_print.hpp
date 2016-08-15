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

#ifndef ARCHON_PARSER_REGEX_PRINT_HPP
#define ARCHON_PARSER_REGEX_PRINT_HPP

#include <cwchar>
#include <stdexcept>
#include <vector>
#include <locale>

#include <archon/core/codec.hpp>
#include <archon/core/text.hpp>

namespace archon
{
  namespace parser
  {
    /**
     * \todo Exclude newline characters from "."
     * See http://www.unicode.org/unicode/reports/tr18/tr18-5.1.html#End%20Of%20Line
     */
    template<typename Ch>
    struct BasicRegexPrinter
    {
      typedef BasicRegex<Ch>                 RegexType;
      typedef typename RegexType::CharType   CharType;
      typedef typename RegexType::StringType StringType;
      typedef typename RegexType::ExpArg     ExpArg;

      /**
       * Construct a string representation of the specified regular
       * expression.
       *
       * \param regex The regular expression whose string
       * representation is to be constructed.
       *
       * \param contextPrecedence The operator precedence of the
       * context of this expression. Non-zero value should generally
       * only be used in recursive invocations.
       *
       * \return The string representation of the specified regular
       * expression.
       *
       * Operator precedence encoding:
       *
       * <PRE>
       *
       *   alternation (|)         0
       *   juxtaposition           1
       *   repeatition (*,+,?,{})  2
       *
       * </PRE>
       */
      StringType print(ExpArg regex, short unsigned contextPrecedence = 0) const;

      BasicRegexPrinter(std::locale loc = std::locale(""))
      {
        // Widen some fixed strings
        core::BasicLocaleCharMapper<CharType> mapper(loc);

        lpar      = mapper.widen("(");
        rpar      = mapper.widen(")");
        lbrace    = mapper.widen("{");
        rbrace    = mapper.widen("}");
        lbrack    = mapper.widen("[");
        rbrack    = mapper.widen("]");
        bar       = mapper.widen("|");
        star      = mapper.widen("*");
        plus      = mapper.widen("+");
        opt       = mapper.widen("?");
        comma     = mapper.widen(",");
        dot       = mapper.widen(".");
        dash      = mapper.widen("-");
        colon     = mapper.widen(":");
        equal     = mapper.widen("=");
        slosh     = mapper.widen("\\");

        caret     = mapper.widen("^");
        dollar    = mapper.widen("$");
        bow       = mapper.widen("[[:<:]]");
        eow       = mapper.widen("[[:>:]]");

        cl_alnum  = mapper.widen("alnum");
        cl_alpha  = mapper.widen("alpha");
        cl_blank  = mapper.widen("blank");
        cl_cntrl  = mapper.widen("cntrl");
        cl_digit  = mapper.widen("digit");
        cl_graph  = mapper.widen("graph");
        cl_lower  = mapper.widen("lower");
        cl_print  = mapper.widen("print");
        cl_punct  = mapper.widen("punct");
        cl_space  = mapper.widen("space");
        cl_upper  = mapper.widen("upper");
        cl_xdigit = mapper.widen("xdigit");

        specials  = mapper.widen("|(){\\$?*+.^[");
      }

    private:
      core::Text::BasicValuePrinter<CharType> s;

      StringType lpar;
      StringType rpar;
      StringType lbrace;
      StringType rbrace;
      StringType lbrack;
      StringType rbrack;
      StringType bar;
      StringType star;
      StringType plus;
      StringType opt;
      StringType comma;
      StringType dot;
      StringType dash;
      StringType colon;
      StringType equal;
      StringType slosh;

      StringType caret;
      StringType dollar;
      StringType bow;
      StringType eow;

      StringType cl_alnum;
      StringType cl_alpha;
      StringType cl_blank;
      StringType cl_cntrl;
      StringType cl_digit;
      StringType cl_graph;
      StringType cl_lower;
      StringType cl_print;
      StringType cl_punct;
      StringType cl_space;
      StringType cl_upper;
      StringType cl_xdigit;

      StringType specials;
    };


    typedef BasicRegexPrinter<char>    RegexPrinter;
    typedef BasicRegexPrinter<wchar_t> WideRegexPrinter;




    // Template implementations:


    template<typename Ch>
    typename BasicRegexPrinter<Ch>::StringType BasicRegexPrinter<Ch>::print(ExpArg e, short unsigned p) const
    {
      if(typename RegexType::Alt const *f = dynamic_cast<typename RegexType::Alt const *>(e.get()))
      {
        StringType t = print(f->e1, 0) + bar + print(f->e2, 0);
        return 0<p ? lpar+t+rpar : t;
      }

      if(typename RegexType::Jux const *f = dynamic_cast<typename RegexType::Jux const *>(e.get()))
      {
        StringType t = print(f->e1, 1) + print(f->e2, 1);
        return 1<p ? lpar+t+rpar : t;
      }

      if(typename RegexType::Rep const *f = dynamic_cast<typename RegexType::Rep const *>(e.get()))
      {
        StringType t = print(f->e, 2) +
          (f->min == 0 ? f->max == 0 ? star : f->max == 1 ? opt : lbrace+s.print(0)+comma+s.print(f->max)+rbrace :
           f->min == 1 ? f->max == 0 ? plus : f->max == 1 ? lbrace+s.print(1)+rbrace : lbrace+s.print(1)+comma+s.print(f->max)+rbrace :
           f->max == f->min ? lbrace+s.print(f->min)+rbrace : f->max == 0 ? lbrace+s.print(f->min)+comma+rbrace :
           lbrace+s.print(f->min)+comma+s.print(f->max)+rbrace);
        return 2<p ? lpar+t+rpar : t;
      }

      if(typename RegexType::Str const *f = dynamic_cast<typename RegexType::Str const *>(e.get()))
      {
        typename StringType::size_type l = f->s.size();
        StringType t;
        t.reserve(l+l/5);
        for(typename StringType::const_iterator i=f->s.begin(); i!=f->s.end(); ++i)
        {
          if(specials.find(*i)!=StringType::npos) t += slosh;
          t += *i;
        }
        return (l==0 ? -1 : l==1 ? 3 : 1) < static_cast<int>(p) ? lpar+t+rpar : t;
      }

      // Things to handle in a special way:
      //
      //  The first range begins with caret in a positive bracket.
      //
      //  The first range ends with an end bracket or any following range
      //  begins or ends with an end bracket
      //
      //  A range begins with a hyphen and is not the first range and if
      //  the range also ends with a hyphen then it is not the last range
      //  either.
      //
      //  One range ends with '[' and the next range starts with '.' or
      //  '=' or ':'.
      if(typename RegexType::Bra const *f = dynamic_cast<typename RegexType::Bra const *>(e.get()))
      {
        StringType t = lbrack;
        if(f->invert) t += caret;
        bool brack = false;
        typedef typename RegexType::CharRange CharRange;
        for(typename std::vector<CharRange>::size_type i=0; i<f->ranges.size(); ++i)
        {
          CharRange const &r = f->ranges[i];
          StringType first(1, r.first);
          if(r.first==caret[0] && i==0 && !f->invert ||
             r.first==rbrack[0] && 0<i ||
             r.first==dash[0] && 0<i && (r.second!=dash[0] || i<f->ranges.size()-1) ||
             (r.first==dot[0] || r.first==equal[0] || r.first==colon[0]) && brack)
            first = lbrack+dot+first+dot+rbrack;
          if(r.first == r.second) t += first;
          else
          {
            StringType last(1, r.second);
            if(r.second==rbrack[0]) last = lbrack+dot+last+dot+rbrack;
            t += first+dash+last;
          }
          brack = r.second==lbrack[0];
        }
        typedef typename RegexType::NamedClass NamedClass;
        typedef typename std::vector<NamedClass>::const_iterator ClassIter;
        for(ClassIter i = f->classes.begin(); i != f->classes.end(); ++i)
        {
          StringType n;
          switch(*i)
          {
          case RegexType::alnum:  n = cl_alnum;  break;
          case RegexType::alpha:  n = cl_alpha;  break;
          case RegexType::blank:  n = cl_blank;  break;
          case RegexType::cntrl:  n = cl_cntrl;  break;
          case RegexType::digit:  n = cl_digit;  break;
          case RegexType::graph:  n = cl_graph;  break;
          case RegexType::lower:  n = cl_lower;  break;
          case RegexType::print:  n = cl_print;  break;
          case RegexType::punct:  n = cl_punct;  break;
          case RegexType::space:  n = cl_space;  break;
          case RegexType::upper:  n = cl_upper;  break;
          case RegexType::xdigit: n = cl_xdigit; break;
          }
          t += lbrack+colon+n+colon+rbrack;
        }
        return t == lbrack+caret ? dot : t+rbrack;
      }

      if(dynamic_cast<typename RegexType::Bol const *>(e.get())) return caret;
      if(dynamic_cast<typename RegexType::Eol const *>(e.get())) return dollar;
      if(dynamic_cast<typename RegexType::Bow const *>(e.get())) return bow;
      if(dynamic_cast<typename RegexType::Eow const *>(e.get())) return eow;

      throw std::invalid_argument("Unsupported Regex node");
    }
  }
}

#endif // ARCHON_PARSER_REGEX_PRINT_HPP
