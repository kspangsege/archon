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
 *
 * This file defines a data structure that models regular expressions.
 */

#ifndef ARCHON_PARSER_REGEX_HPP
#define ARCHON_PARSER_REGEX_HPP

#include <cwchar>
#include <stdexcept>
#include <limits>
#include <utility>
#include <vector>
#include <string>

#include <archon/core/refcnt.hpp>
#include <archon/core/iseq.hpp>

namespace archon {
namespace parser {

struct RegexBase {
    enum NamedClass {
	alnum, alpha, blank, cntrl, digit, graph,
	lower, print, punct, space, upper, xdigit
    };
};


/**
 * A reference counting abstract base class for any node in the
 * data structure that makes up a regular expression.
 *
 * \todo Exclude newline characters from "."
 * See http://www.unicode.org/unicode/reports/tr18/tr18-5.1.html#End%20Of%20Line
 */
template<class Ch> struct BasicRegex: RegexBase, core::CntRefObjectBase {
    typedef Ch                                      CharType;
    typedef std::basic_string<CharType>             StringType;
    typedef std::pair<CharType, CharType>           CharRange;
    typedef core::CntRef<BasicRegex const>          Exp;
    typedef Exp const                              &ExpArg;

    /**
     * Match either \c r1 or \c r2.
     */
    static Exp alt(ExpArg r1, ExpArg r2);

    /**
     * Match the juxtaposition of \c r1 and \c r2.
     */
    static Exp jux(ExpArg r1, ExpArg r2);

    /**
     * Match \c n repetitions of \c r where if \c max != 0 then \c
     * min <= \c n <= \c max. If \c max == 0 then \c min <= \c n.
     */
    static Exp rep(ExpArg r, size_t min = 0, size_t max = 0);

    /**
     * Match zero or more repetitions of \c r.
     */
    static Exp star(ExpArg r);

    /**
     * Match one or more repetitions of \c r.
     */
    static Exp plus(ExpArg r);

    /**
     * Match \c r or the empty string.
     */
    static Exp opt(ExpArg r);

    /**
     * Match the fixed string \c s which may be empty.
     */
    static Exp str(StringType s);

    /**
     * Match the empty string.
     */
    static Exp eps();

    /**
     * Match any character from a set of ranges and named character
     * classes, or if \c invert is true, match any character that is
     * in none of the specified ranges and classe.
     *
     * Each range must have a first component less than or equal to
     * its second component.
     *
     * \param ranges_begin An iterator that points to the first
     * range that should be included (or excluded). The referenced
     * type of this iterator must be \c CharRange.
     *
     * \param ranges_end An iterator that points 
     */
    template<typename RangeSeq, typename ClassSeq>
    static Exp bra(RangeSeq ranges, ClassSeq classes, bool invert = false);

    /**
     * Match one character in the specified range (both inclusive).
     */
    static Exp range(CharRange range, bool invert = false);

    /**
     * Match one character from the named class.
     *
     * \param name \see{bracket}
     */
    static Exp cla(NamedClass name, bool invert = false);

    /**
     * Match one arbitrary character
     */
    static Exp any();

    /**
     * Match the beginning of a line.
     */
    static Exp bol();

    /**
     * Match the end of a line.
     */
    static Exp eol();

    /**
     * Match the beginning of a word.
     */
    static Exp bow();

    /**
     * Match the end of a word.
     */
    static Exp eow();

    struct Alt;
    struct Jux;
    struct Rep;
    struct Str;
    struct Bra;
    struct Bol;
    struct Eol;
    struct Bow;
    struct Eow;
};


typedef BasicRegex<char>    Regex;
typedef BasicRegex<wchar_t> WideRegex;


/**
 * Match either one of the two underlying expressions.
 */
template<class Ch> struct BasicRegex<Ch>::Alt: BasicRegex {
    Exp e1, e2;
    Alt(ExpArg e1, ExpArg e2): e1(e1), e2(e2) {}
};


/**
 * Match the juxtaposition of the two underlying expressions.
 */
template<class Ch> struct BasicRegex<Ch>::Jux: BasicRegex {
    Exp e1, e2;
    Jux(ExpArg e1, ExpArg e2): e1(e1), e2(e2) {}
};


/**
 * Match N repetitions of the underlying expression where N lies
 * in a certain range.
 */
template<class Ch> struct BasicRegex<Ch>::Rep: BasicRegex {
    Exp e;
    size_t min;
    size_t max; //< 0 if no max
    Rep(ExpArg e, size_t min, size_t max);
};


/**
 * Match a fixed string.
 */
template<class Ch> struct BasicRegex<Ch>::Str: BasicRegex {
    StringType s;
    Str(StringType const &s): s(s) {}
};


/**
 * Match one character agains a class of characters.
 */
template<class Ch> struct BasicRegex<Ch>::Bra: BasicRegex {
    std::vector<CharRange> ranges;
    std::vector<NamedClass> classes;
    bool invert;
    template<typename RangeSeq, typename ClassSeq>
    Bra(RangeSeq r, ClassSeq c, bool inv);
};


/**
 * Match the beginning of a line
 */
template<class Ch> struct BasicRegex<Ch>::Bol: BasicRegex
{
};


/**
 * Match the end of a line
 */
template<class Ch> struct BasicRegex<Ch>::Eol: BasicRegex
{
};


/**
 * Match the beginning of a word
 */
template<class Ch> struct BasicRegex<Ch>::Bow: BasicRegex
{
};


/**
 * Match the end of a word
 */
template<class Ch> struct BasicRegex<Ch>::Eow: BasicRegex
{
};




// Template implementations:


template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::alt(ExpArg r1, ExpArg r2)
{
    return Exp(new Alt(r1, r2));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::jux(ExpArg r1, ExpArg r2)
{
    return Exp(new Jux(r1, r2));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::rep(ExpArg r, size_t min, size_t max)
{
    return Exp(new Rep(r, min, max));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::star(ExpArg r)
{
    return Exp(new Rep(r, 0, 0));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::plus(ExpArg r)
{
    return Exp(new Rep(r, 1, 0));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::opt(ExpArg r)
{
    return Exp(new Rep(r, 0,  1));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::str(StringType s)
{
    return Exp(new Str(s));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::eps()
{
    return Exp(new Str(StringType()));
}

template<typename Ch> template<typename RangeSeq, typename ClassSeq>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::bra(RangeSeq r, ClassSeq c, bool i)
{
    return Exp(new Bra(r, c, i));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::range(CharRange r, bool i)
{
    return bra(core::oneSeq(r), core::nullSeq<NamedClass>(), i);
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::cla(NamedClass c, bool i)
{
    return bra(core::nullSeq<CharRange>(), core::oneSeq(c), i);
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::any()
{
    return range(CharRange(std::numeric_limits<CharType>::min(),
                           std::numeric_limits<CharType>::max()));
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::bol()
{
    return Exp(new Bol());
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::eol()
{
    return Exp(new Eol());
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::bow()
{
    return Exp(new Bow());
}

template<typename Ch>
inline typename BasicRegex<Ch>::Exp BasicRegex<Ch>::eow()
{
    return Exp(new Eow());
}


template<typename Ch>
inline BasicRegex<Ch>::Rep::Rep(ExpArg e, size_t min, size_t max): e(e), min(min), max(max)
{
    if(max && max < min) throw std::invalid_argument("Bad repetition range");
}

template<typename Ch> template<typename RangeSeq, typename ClassSeq>
inline BasicRegex<Ch>::Bra::Bra(RangeSeq r, ClassSeq c, bool inv): invert(inv)
{
    for(; r; ++r)
    {
	if(r->second < r->first) throw std::invalid_argument("Bad character range");
        ranges.push_back(*r);
    }
    for(; c; ++c) classes.push_back(*c);
}

} // namespace parser
} // namespace archon

#endif // ARCHON_PARSER_REGEX_HPP
