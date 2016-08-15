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
 * This file defines a data structure that models regular expressions
 * as well as a printer and a parser to convert to/from the associated
 * string representation.
 *
 * The format produced by the printer and accepted by the parser is
 * syntactically identical to that of POSIX 1003.2 regular expressions
 * except for the added ability to include a reference to a previously
 * named expression in 'Lex' style - eg. '({foo}|{bar})*' where 'foo'
 * and 'bar' are previously defined expressions.
 *
 * All parts are Unicode enabled.
 */

#ifndef ARCHON_PARSER_REGEX_PARSE_HPP
#define ARCHON_PARSER_REGEX_PARSE_HPP

#include <cwchar>
#include <string>
#include <ostream>
#include <utility>
#include <vector>
#include <map>

#include <archon/core/logger.hpp>
#include <archon/core/refcnt.hpp>
#include <archon/core/text.hpp>

namespace archon
{
  namespace Parser
  {
    /**
     * A reference counting abstract base class for any node in the
     * data structure that makes up a regular expression.
     *
     * Operator precedence:
     *   alternation (|)         0
     *   juxtaposition           1
     *   repeatition (*,+,?,{})  2
     *
     * \todo Considder using a bitset<N> instead of vector<bool> for
     * representing named character classes.
     *
     * \todo Prevent users from using characters in range 0xE000 -
     * 0xF8FF or even better think of a way to represent the anchor
     * edges without using symbol values.
     *
     * \todo Exclude newline characters from "."
     * See http://www.unicode.org/unicode/reports/tr18/tr18-5.1.html#End%20Of%20Line
     */
    template<typename Ch>
    struct Regex: core::CntRefObjectBase
    {

      /**
       * Construct a regular expression from the specified string
       * representation.
       *
       * @param l If null is passed for the logger then no errors are
       * accepted in the string representation, otherwise non-fatal
       * errors are logged, and only a fatal error will result in an
       * exception being thrown.
       */
      static Exp parse(StringType, core::Logger *l=0);



    /*
      Regex(StringType s, core::Logger *l=0)
      {
	parse(s, l, 0);
      }
    */

      /**
       * Construct a regular expression from a string representation.
       * Accept the special syntax extension where {name} stands for a
       * previously defined expression.
       *
       * @param l See Regex(StringType, core::Logger *)
       */
    /*
      Regex(StringType s, Environment const &e, core::Logger *l=0)
      {
	parse(s, l, &e);
      }

//      static Parser const &getParser();

      void parse(StringType, core::Logger *, Environment const *);
    };
    */
/*

    typedef RefObject<ustring> UAttr;
    typedef RefObject<pair<int, int> > IPAttr;



    struct Regex::ParserContext: LrParserBase::Context
    {
      LexerBase *lexer;
      Environment const *env;
      Logger *logger;

      Ref<Class> bracket;

      ParserContext(LexerBase *lexer, Environment const *env, Logger *logger):
	lexer(lexer), env(env), logger(logger) {}

      void warning(string s)
      {
	if(logger) logger->log(s);
	else ARCHON_THROW1(ArgumentException, s);
      }

      void parserError()
      {
	if(lexer->getType() < 0)
	  warning("Syntax error. Unexpected end of string.");
	else
	  warning("Syntax error. Unexpected character " +
		  s.print(Text::escapeNonprintable(lexer->getText())) +
		  ".");
      }

      Ref<Exp const> altern(Ref<Exp const> const &l,
			    Ref<Exp const> const &r)
      {
	if(!l) return r; // An already detected error
	if(!r) return l; // An already detected error
	return new Altern(l, r);
      }

      Ref<Exp const> empty() { return new String(); }

      Ref<Exp const> str(Ref<UAttr const> const &s)
      {
	return new String(s->value);
      }

      Ref<Exp const> any()        { return new Class(); }
      Ref<Exp const> line_begin() { return new LineBegin(); }
      Ref<Exp const> line_end()   { return new LineEnd(); }

      Ref<Exp const> word_begin()
      {
	bracket = 0;
	return new WordBegin();
      }

      Ref<Exp const> word_end()
      {
	bracket = 0;
	return new WordEnd();
      }

      Ref<Exp const> juxta(Ref<const Exp> const &l,
			   Ref<const Exp> const &r)
      {
	if(!l) return r; // An already detected error
	if(!r) return l; // An already detected error
	return new Juxta(l, r);
      }

      Ref<Exp const> repeat(Ref<const Exp> const &e,
			    Ref<const IPAttr> const &r)
      {
	if(!e || !r) return 0; // An already detected error
	return new Repeat(e, r->value.first, r->value.second);
      }

      Ref<IPAttr const> plus()
      {
	return new IPAttr(make_pair(1, -1));
      }

      Ref<IPAttr const> star()
      {
	return new IPAttr(make_pair(0, -1));
      }

      Ref<IPAttr const> option()
      {
	return new IPAttr(make_pair(0, 1));
      }

      static int parseInt(ustring s)
      {
	int v = 0;
	for(unsigned i=0; i<s.size(); ++i)
	{
	  v *= 10;
	  v += s[i] - '0';
	}
	return v;
      }

      Ref<IPAttr const> repeat_range(Ref<UAttr const> const &f,
				     Ref<UAttr const> const &t)
      {
	if(!f) return 0; // An already detected error
	int a = parseInt(f->value);
	int b = t ? f==t ? a : parseInt(t->value) : -1;
	if(t && a > b)
	{
	  warning("Illegal repeat range {" + s.print(a) + "," +
		  s.print(b) + "}: Lower bound is greater "
		  "than upper bound");
	  swap(a, b);
	}
	return new IPAttr(make_pair(a, b));
      }

      void new_bracket()
      {
	bracket = new Class();
      }

      Ref<Class const> pos_bracket()
      {
	Ref<Class> b = bracket;
	bracket = 0;
	bool f = true;
	for(unsigned i=0; i<=Class::name_xdigit; ++i)
	  if(b->namedClasses[i])
	  {
	    f = false;
	    break;
	  }
	if(b->ranges.size()==0 && f) return 0; // An already detected error
	b->invert = false;
	return b;
      }

      Ref<Class const> neg_bracket()
      {
	Ref<Class> b = bracket;
	bracket = 0;
	return b;
      }

      void named_class(Ref<UAttr const> const &n)
      {
	if(!n)
	{
	  warning("Empty character class name");
	  return;
	}

	string const s = s.print(n->value);
	int const i =
	  s == "alnum" ? Class::name_alnum :
	  s == "alpha" ? Class::name_alpha :
	  s == "blank" ? Class::name_blank :
	  s == "cntrl" ? Class::name_cntrl :
	  s == "digit" ? Class::name_digit :
	  s == "graph" ? Class::name_graph :
	  s == "lower" ? Class::name_lower :
	  s == "print" ? Class::name_print :
	  s == "punct" ? Class::name_punct :
	  s == "space" ? Class::name_space :
	  s == "upper" ? Class::name_upper :
	  s == "xdigit" ? Class::name_xdigit : -1;
	if(i==-1)
	{
	  warning("Illegal character class [:" + s + ":]: Unknown name");
	  return;
	}
	bracket->namedClasses[i] = true;
      }

      void bracket_range(Ref<UAttr const> const &f, Ref<UAttr const> const &t)
      {
	if(!f || !t) return; // An already detected error
	uchar u = f->value[0];
	uchar v = t->value[0];
	if(u>v)
	{
	  warning("Illegal range " + s.print(ustring(1, u)) + "-" +
		  s.print(ustring(1, v)) + ": Lower endpoint is greater "
		  "than upper endpoint");
	  swap(u, v);
	}
	bracket->ranges.push_back(make_pair(u, v));
      }

      Ref<UAttr const> collate(Ref<UAttr const> const &e)
      {
	ustring const s(e->value, 0, e->value.size()-1);
	if(s.size() == 0)
	{
	  warning("Empty collation element");
	  return 0;
	}
	if(s.size() == 1) return new UAttr(s);
	warning("Illegal collation element [." + s.print(s) +
		".]: Collation elements of more than one character are not "
		"supported yet");
	return 0;
      }

      void equiv(Ref<UAttr const> const &e)
      {
	ustring const s(e->value, 0, e->value.size()-1);
	if(s.size() == 0)
	{
	  warning("Empty collation element in equivalince class");
	  return;
	}
	warning("Illegal equivalence class [=" + s.print(s) +
		"=]: Equivalence classes are not supported yet");
      }

      Ref<Exp const> named_exp(Ref<UAttr const> const &n)
      {
	if(!n) return 0; // An already detected error
	string const s = s.print(n->value);
	if(!env)
	{
	  warning("Illegal exression reference {" + s + "}: No environment");
	  return 0;
	}
	map<string, Ref<Exp const> >::const_iterator i = env->m.find(s);
	if(i == env->m.end())
	{
	  warning("Illegal expression reference {" + s + "}: Undefined name");
	  return 0;
	}
	return i->second;
      }
    };

    struct Regex::Parser
    {
      struct Printer: LrParserBase::Printer
      {
	virtual string print(RefAnyConst const &a) const
	{
	  if(!a) return "<null>";
	  if(UAttr const *b = dynamic_cast<UAttr const *>(a.get()))
	    return "\"" + s.print(b->value) + "\"";
	  if(IPAttr const *b = dynamic_cast<IPAttr const *>(a.get()))
	    return "(" + s.print(b->value.first) + "," +
	      s.print(b->value.second) + ")";
	  if(Exp const *b = dynamic_cast<Exp const *>(a.get()))
	    return b->print();
	  return "<unknown>";
	}
      };

      core::UniquePtr<LrParserBase::Actor<ParserContext> > actor;
      core::UniquePtr<Printer> printer;
      core::UniquePtr<SlrParser> lrParser;
      int terminalMap[128];

      Parser():
	actor(new LrParserBase::Actor<ParserContext>())
      {
	CFG g(actor.get());

	int const dollar   = g.defineTerminal("$");
	int const lpar     = g.defineTerminal("(");
	int const rpar     = g.defineTerminal(")");
	int const star     = g.defineTerminal("*");
	int const plus     = g.defineTerminal("+");
	int const comma    = g.defineTerminal(",");
	int const hyphen   = g.defineTerminal("-");
	int const flstop   = g.defineTerminal(".");
	int const colon    = g.defineTerminal(":");
	int const less     = g.defineTerminal("<");
	int const equal    = g.defineTerminal("=");
	int const greater  = g.defineTerminal(">");
	int const question = g.defineTerminal("?");
	int const lbrack   = g.defineTerminal("[");
	int const backsl   = g.defineTerminal("\\");
	int const rbrack   = g.defineTerminal("]");
	int const caret    = g.defineTerminal("^");
	int const undersc  = g.defineTerminal("_");
	int const lbrace   = g.defineTerminal("{");
	int const vbar     = g.defineTerminal("|");
	int const rbrace   = g.defineTerminal("}");
	int const ALPHA    = g.defineTerminal("ALPHA");
	int const DIGIT    = g.defineTerminal("DIGIT");
	int const OTHER    = g.defineTerminal("OTHER");

	int const exp            = g.defineNonterminal("exp");
	int const branch         = g.defineNonterminal("branch");
	int const atom           = g.defineNonterminal("atom");
	int const bracket        = g.defineNonterminal("bracket");
	int const br_pos         = g.defineNonterminal("br_pos");
	int const br_char_pos    = g.defineNonterminal("br_char_pos");
	int const br_char_common = g.defineNonterminal("br_char_common");
	int const common_char    = g.defineNonterminal("common_char");
	int const br_opt1        = g.defineNonterminal("br_opt1");
	int const br_start1      = g.defineNonterminal("br_start1");
	int const br_char1       = g.defineNonterminal("br_char1");
	int const br_opt2        = g.defineNonterminal("br_opt2");
	int const br_start2      = g.defineNonterminal("br_start2");
	int const br_char2       = g.defineNonterminal("br_char2");
	int const br_end         = g.defineNonterminal("br_end");
	int const br_char_end    = g.defineNonterminal("br_char_end");
	int const br_start3      = g.defineNonterminal("br_start3");
	int const br_start4      = g.defineNonterminal("br_start4");
	int const br_collate     = g.defineNonterminal("br_collate");
	int const br_collate1    = g.defineNonterminal("br_collate1");
	int const br_collate_ch  = g.defineNonterminal("br_collate_ch");
	int const br_collate2    = g.defineNonterminal("br_collate2");
	int const br_equiv       = g.defineNonterminal("br_equiv");
	int const br_equiv1      = g.defineNonterminal("br_equiv1");
	int const br_equiv_ch    = g.defineNonterminal("br_equiv_ch");
	int const br_equiv2      = g.defineNonterminal("br_equiv2");
	int const br_class       = g.defineNonterminal("br_class");
	int const id_char_seq    = g.defineNonterminal("id_char_seq");
	int const id_char        = g.defineNonterminal("id_char");
	int const id_char_init   = g.defineNonterminal("id_char_init");
	int const br_neg         = g.defineNonterminal("br_neg");
	int const br_char_neg    = g.defineNonterminal("br_char_neg");
	int const exp_name       = g.defineNonterminal("exp_name");
	int const any_char       = g.defineNonterminal("any_char");
	int const atom_char      = g.defineNonterminal("atom_char");
	int const branch2        = g.defineNonterminal("branch2");
	int const repeat         = g.defineNonterminal("repeat");
	int const repeat_range   = g.defineNonterminal("repeat_range");
	int const integer        = g.defineNonterminal("integer");

	int const a_altern        =
	  actor->registerMethod("altern",        &ParserContext::altern);
	int const a_empty         =
	  actor->registerMethod("empty",         &ParserContext::empty);
	int const a_string        =
	  actor->registerMethod("string",        &ParserContext::str);
	int const a_any           =
	  actor->registerMethod("any",           &ParserContext::any);
	int const a_line_begin    =
	  actor->registerMethod("line_begin",    &ParserContext::line_begin);
	int const a_line_end      =
	  actor->registerMethod("line_end",      &ParserContext::line_end);
	int const a_word_begin    =
	  actor->registerMethod("word_begin",    &ParserContext::word_begin);
	int const a_word_end      =
	  actor->registerMethod("word_end",      &ParserContext::word_end);
	int const a_juxta         =
	  actor->registerMethod("juxta",         &ParserContext::juxta);
	int const a_repeat        =
	  actor->registerMethod("repeat",        &ParserContext::repeat);
	int const a_option        =
	  actor->registerMethod("option",        &ParserContext::option);
	int const a_star          =
	  actor->registerMethod("star",          &ParserContext::star);
	int const a_plus          =
	  actor->registerMethod("plus",          &ParserContext::plus);
	int const a_repeat_range  =
	  actor->registerMethod("repeat_range",  &ParserContext::repeat_range);
	int const a_new_bracket   =
	  actor->registerMethod("new_bracket",   &ParserContext::new_bracket);
	int const a_pos_bracket   =
	  actor->registerMethod("pos_bracket",   &ParserContext::pos_bracket);
	int const a_neg_bracket   =
	  actor->registerMethod("neg_bracket",   &ParserContext::neg_bracket);
	int const a_named_class   =
	  actor->registerMethod("named_class",   &ParserContext::named_class);
	int const a_bracket_range =
	  actor->registerMethod("bracket_range", &ParserContext::bracket_range);
	int const a_collate       =
	  actor->registerMethod("collate",       &ParserContext::collate);
	int const a_equiv         =
	  actor->registerMethod("equiv",         &ParserContext::equiv);
	int const a_named_exp     =
	  actor->registerMethod("named_exp",     &ParserContext::named_exp);

	// exp
	g.addProd(exp, CFG::nont(branch));
	g.addProd(exp, CFG::nont(exp), CFG::term(vbar), CFG::nont(branch),
		  CFG::act(a_altern, 2, 0));

	// branch
	g.addProd(branch, CFG::nont(atom));
	g.addProd(branch, CFG::nont(atom), CFG::nont(branch2), CFG::copy(0));

	// atom
	g.addProd(atom, CFG::term(lpar), CFG::term(rpar), CFG::act(a_empty));
	g.addProd(atom, CFG::term(lpar), CFG::nont(exp),
		  CFG::term(rpar), CFG::copy(1));
	g.addProd(atom, CFG::term(lbrack), CFG::act(a_new_bracket),
		  CFG::nont(bracket), CFG::term(rbrack), CFG::copy(1));
	g.addProd(atom, CFG::term(lbrace), CFG::nont(exp_name), CFG::term(rbrace),
		  CFG::act(a_named_exp, 1));
	g.addProd(atom, CFG::term(backsl), CFG::nont(any_char),
		  CFG::act(a_string, 0));
	g.addProd(atom, CFG::term(flstop),    CFG::act(a_any));
	g.addProd(atom, CFG::term(caret),     CFG::act(a_line_begin));
	g.addProd(atom, CFG::term(dollar),    CFG::act(a_line_end));
	g.addProd(atom, CFG::nont(atom_char), CFG::act(a_string, 0));

	// bracket
	g.addProd(bracket, CFG::nont(br_pos), CFG::act(a_pos_bracket));
	g.addProd(bracket, CFG::term(caret), CFG::nont(br_neg),
		  CFG::act(a_neg_bracket));
	g.addProd(bracket, CFG::term(lbrack), CFG::term(colon), CFG::term(less),
		  CFG::term(colon), CFG::term(rbrack), CFG::act(a_word_begin));
	g.addProd(bracket, CFG::term(lbrack), CFG::term(colon), CFG::term(greater),
		  CFG::term(colon), CFG::term(rbrack), CFG::act(a_word_end));

	// br_pos
	g.addProd(br_pos, CFG::nont(br_char_pos), CFG::nont(br_opt1));
	g.addProd(br_pos, CFG::nont(br_opt2));

	// br_char_pos
	g.addProd(br_char_pos, CFG::nont(br_char_common));
	g.addProd(br_char_pos, CFG::term(rbrack));
	g.addProd(br_char_pos, CFG::term(hyphen));
	g.addProd(br_char_pos, CFG::term(flstop));
	g.addProd(br_char_pos, CFG::term(equal));
	g.addProd(br_char_pos, CFG::term(colon));

	// br_char_common
	g.addProd(br_char_common, CFG::nont(common_char));
	g.addProd(br_char_common, CFG::term(vbar));
	g.addProd(br_char_common, CFG::term(lpar));
	g.addProd(br_char_common, CFG::term(rpar));
	g.addProd(br_char_common, CFG::term(lbrace));
	g.addProd(br_char_common, CFG::term(backsl));
	g.addProd(br_char_common, CFG::term(dollar));
	g.addProd(br_char_common, CFG::term(question));
	g.addProd(br_char_common, CFG::term(star));
	g.addProd(br_char_common, CFG::term(plus));

	// common_char
	g.addProd(common_char, CFG::term(rbrace));
	g.addProd(common_char, CFG::term(undersc));
	g.addProd(common_char, CFG::term(comma));
	g.addProd(common_char, CFG::term(less));
	g.addProd(common_char, CFG::term(greater));
	g.addProd(common_char, CFG::term(DIGIT));
	g.addProd(common_char, CFG::term(ALPHA));
	g.addProd(common_char, CFG::term(OTHER));

	// br_opt1
	g.addProd(br_opt1, CFG::act(a_bracket_range, 0, 0), CFG::nont(br_start1));
	g.addProd(br_opt1, CFG::term(hyphen), CFG::nont(br_end));

	// br_start1
	g.addProd(br_start1);
	g.addProd(br_start1, CFG::nont(br_char1), CFG::nont(br_opt1));
	g.addProd(br_start1, CFG::nont(br_opt2));

	// br_char1
	g.addProd(br_char1, CFG::nont(br_char_common));
	g.addProd(br_char1, CFG::term(flstop));
	g.addProd(br_char1, CFG::term(equal));
	g.addProd(br_char1, CFG::term(colon));
	g.addProd(br_char1, CFG::term(caret));

	// br_opt2
	g.addProd(br_opt2, CFG::term(lbrack), CFG::act(a_bracket_range, 0, 0),
		  CFG::nont(br_start2));
	g.addProd(br_opt2, CFG::term(lbrack), CFG::term(hyphen),
		  CFG::nont(br_end));
	g.addProd(br_opt2, CFG::nont(br_collate), CFG::nont(br_opt1));
	g.addProd(br_opt2, CFG::nont(br_equiv), CFG::nont(br_start3));
	g.addProd(br_opt2, CFG::nont(br_class), CFG::nont(br_start3));

	// br_start2
	g.addProd(br_start2);
	g.addProd(br_start2, CFG::nont(br_char2), CFG::nont(br_opt1));
	g.addProd(br_start2, CFG::nont(br_opt2));

	// br_char2
	g.addProd(br_char2, CFG::nont(br_char_common));
	g.addProd(br_char2, CFG::term(caret));

	// br_end
	g.addProd(br_end, CFG::act(a_bracket_range, 1, 1),
		  CFG::act(a_bracket_range, 1, 1));
	g.addProd(br_end, CFG::nont(br_char_end), CFG::act(a_bracket_range, 2, 0),
		  CFG::nont(br_start3));
	g.addProd(br_end, CFG::term(lbrack), CFG::act(a_bracket_range, 2, 0),
		  CFG::nont(br_start4));
	g.addProd(br_end, CFG::nont(br_collate), CFG::act(a_bracket_range, 2, 0),
		  CFG::nont(br_start3));

	// br_char_end
	g.addProd(br_char_end, CFG::nont(br_char1));
	g.addProd(br_char_end, CFG::term(hyphen));

	// br_start3
	g.addProd(br_start3, CFG::term(hyphen), CFG::act(a_bracket_range, 0, 0));
	g.addProd(br_start3, CFG::nont(br_start1));

	// br_start4
	g.addProd(br_start4, CFG::term(hyphen), CFG::act(a_bracket_range, 0, 0));
	g.addProd(br_start4, CFG::nont(br_start2));

	// br_collate
	g.addProd(br_collate, CFG::term(lbrack), CFG::term(flstop),
		  CFG::nont(br_collate1), CFG::act(a_collate, 0));

	// br_collate1
	g.addProd(br_collate1, CFG::nont(br_collate_ch), CFG::nont(br_collate1),
		  CFG::concat(1, 0));
	g.addProd(br_collate1, CFG::term(flstop), CFG::nont(br_collate2),
		  CFG::concat(1, 0));
	g.addProd(br_collate1, CFG::term(rbrack),	CFG::nont(br_collate1),
		  CFG::concat(1, 0));

	// br_collate_ch
	g.addProd(br_collate_ch, CFG::nont(br_char_common));
	g.addProd(br_collate_ch, CFG::term(lbrack));
	g.addProd(br_collate_ch, CFG::term(hyphen));
	g.addProd(br_collate_ch, CFG::term(equal));
	g.addProd(br_collate_ch, CFG::term(colon));
	g.addProd(br_collate_ch, CFG::term(caret));

	// br_collate2
	g.addProd(br_collate2, CFG::nont(br_collate_ch), CFG::nont(br_collate1),
		  CFG::concat(1, 0));
	g.addProd(br_collate2, CFG::term(flstop), CFG::nont(br_collate2),
		  CFG::concat(1, 0));
	g.addProd(br_collate2, CFG::term(rbrack), CFG::null());

	// br_equiv
	g.addProd(br_equiv, CFG::term(lbrack), CFG::term(equal),
		  CFG::nont(br_equiv1), CFG::act(a_equiv, 0));

	// br_equiv1
	g.addProd(br_equiv1, CFG::nont(br_equiv_ch), CFG::nont(br_equiv1),
		  CFG::concat(1, 0));
	g.addProd(br_equiv1, CFG::term(equal), CFG::nont(br_equiv2),
		  CFG::concat(1, 0));
	g.addProd(br_equiv1, CFG::term(rbrack), CFG::nont(br_equiv1),
		  CFG::concat(1, 0));

	// br_equiv_ch
	g.addProd(br_equiv_ch, CFG::nont(br_char_common));
	g.addProd(br_equiv_ch, CFG::term(lbrack));
	g.addProd(br_equiv_ch, CFG::term(hyphen));
	g.addProd(br_equiv_ch, CFG::term(flstop));
	g.addProd(br_equiv_ch, CFG::term(colon));
	g.addProd(br_equiv_ch, CFG::term(caret));

	// br_equiv2
	g.addProd(br_equiv2, CFG::nont(br_equiv_ch), CFG::nont(br_equiv1),
		  CFG::concat(1, 0));
	g.addProd(br_equiv2, CFG::term(equal), CFG::nont(br_equiv2),
		  CFG::concat(1, 0));
	g.addProd(br_equiv2, CFG::term(rbrack), CFG::null());

	// br_class
	g.addProd(br_class, CFG::term(lbrack), CFG::term(colon),
		  CFG::nont(id_char_seq), CFG::term(colon), CFG::term(rbrack),
		  CFG::act(a_named_class, 2));

	// id_char_seq
	g.addProd(id_char_seq);
	g.addProd(id_char_seq, CFG::nont(id_char_seq), CFG::nont(id_char),
		  CFG::concat(1, 0));

	// id_char
	g.addProd(id_char, CFG::nont(id_char_init));
	g.addProd(id_char, CFG::term(DIGIT));

	// id_char_init
	g.addProd(id_char_init, CFG::term(undersc));
	g.addProd(id_char_init, CFG::term(ALPHA));

	// br_neg
	g.addProd(br_neg, CFG::nont(br_char_neg), CFG::nont(br_opt1));
	g.addProd(br_neg, CFG::nont(br_opt2));

	// br_char_neg
	g.addProd(br_char_neg, CFG::nont(br_char_pos));
	g.addProd(br_char_neg, CFG::term(caret));

	// exp_name
	g.addProd(exp_name, CFG::nont(id_char_init), CFG::nont(id_char_seq),
		  CFG::concat(1, 0));

	// any_char
	g.addProd(any_char, CFG::nont(br_char_neg));
	g.addProd(any_char, CFG::term(lbrack));

	// atom_char
	g.addProd(atom_char, CFG::nont(common_char));
	g.addProd(atom_char, CFG::term(rbrack));
	g.addProd(atom_char, CFG::term(hyphen));
	g.addProd(atom_char, CFG::term(colon));
	g.addProd(atom_char, CFG::term(equal));

	// branch2
	g.addProd(branch2, CFG::nont(branch), CFG::act(a_juxta, 1, 0));
	g.addProd(branch2, CFG::nont(repeat), CFG::act(a_repeat, 1, 0));
	g.addProd(branch2, CFG::nont(repeat), CFG::act(a_repeat, 1, 0),
		  CFG::nont(branch), CFG::act(a_juxta, 1, 0));

	// repeat
	g.addProd(repeat, CFG::term(question), CFG::act(a_option));
	g.addProd(repeat, CFG::term(star), CFG::act(a_star));
	g.addProd(repeat, CFG::term(plus), CFG::act(a_plus));
	g.addProd(repeat, CFG::term(lbrace), CFG::nont(repeat_range),
		  CFG::term(rbrace), CFG::copy(1));

	// repeat_range
	g.addProd(repeat_range, CFG::nont(integer),
		  CFG::act(a_repeat_range, 0, 0));
	g.addProd(repeat_range, CFG::nont(integer), CFG::term(comma),
		  CFG::act(a_repeat_range, 1, -1));
	g.addProd(repeat_range, CFG::nont(integer), CFG::term(comma),
		  CFG::nont(integer), CFG::act(a_repeat_range, 2, 0));

	// integer
	g.addProd(integer, CFG::nont(integer), CFG::term(DIGIT),
		  CFG::concat(1, 0));
	g.addProd(integer, CFG::term(DIGIT));

	printer.reset(new Printer);
	lrParser.reset(new SlrParser(g, actor.get(), printer.get()));

	for(int i=0; i<128; ++i) terminalMap[i] = OTHER;
	for(int i='0'; i<='9'; ++i) terminalMap[i] = DIGIT;
	for(int i='A'; i<='Z'; ++i) terminalMap[i] = ALPHA;
	for(int i='a'; i<='z'; ++i) terminalMap[i] = ALPHA;

	terminalMap[static_cast<int>('$')] = dollar;
	terminalMap[static_cast<int>('(')] = lpar;
	terminalMap[static_cast<int>(')')] = rpar;
	terminalMap[static_cast<int>('*')] = star;
	terminalMap[static_cast<int>('+')] = plus;
	terminalMap[static_cast<int>(',')] = comma;
	terminalMap[static_cast<int>('-')] = hyphen;
	terminalMap[static_cast<int>('.')] = flstop;
	terminalMap[static_cast<int>(':')] = colon;
	terminalMap[static_cast<int>('<')] = less;
	terminalMap[static_cast<int>('=')] = equal;
	terminalMap[static_cast<int>('>')] = greater;
	terminalMap[static_cast<int>('?')] = question;
	terminalMap[static_cast<int>('[')] = lbrack;
	terminalMap[static_cast<int>('\\')] = backsl;
	terminalMap[static_cast<int>(']')] = rbrack;
	terminalMap[static_cast<int>('^')] = caret;
	terminalMap[static_cast<int>('_')] = undersc;
	terminalMap[static_cast<int>('{')] = lbrace;
	terminalMap[static_cast<int>('|')] = vbar;
	terminalMap[static_cast<int>('}')] = rbrace;
      }
    };

    struct Regex::Lexer: LexerBase
    {
      ustring s;
      unsigned i;
      Parser const &p;
      ustring text;
      int type;

      Lexer(ustring s, Parser const &p): s(s), i(0), p(p) {}

      void getNext(LexerBase::Lexeme &l)
      {
	if(i == s.size())
	{
	  l.type = type = -1;
	  return;
	}
	uchar c = s[i++];
	text = ustring(1, c);
	l.type  = type = p.terminalMap[c < 128 ? c : 0];
	l.value = new RefObject<ustring>(text);
      }

      ustring getText() const
      {
	return text;
      }

      int getType() const
      {
	return type;
      }
    };

    Regex::Parser const &Regex::getParser()
    {
      static Parser p;
      return p;
    } 

    void Regex::parse(ustring s, Logger *logger, Environment const *e)
    {
      Parser const &parser = getParser();
      Lexer lexer(s, parser);
      ParserContext context(&lexer, e, logger);
      parser.lrParser->parse(lexer, context, exp, logger); // Comment in the logger for debugging
      if(!exp) ARCHON_THROW1(ArgumentException,
			     "Could not make any sense of input. "
			     "Problems were reported to the logger.");
    }
*/
  }
}

#endif // ARCHON_PARSER_REGEX_PARSE_HPP
