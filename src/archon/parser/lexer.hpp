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

#ifndef ARCHON_PARSER_LEXER_HPP
#define ARCHON_PARSER_LEXER_HPP

#include <string>
#include <stdexcept>

namespace archon
{
  namespace Parser
  {
    /**
     * An abstract base class for lexers defining a lexeme object and a
     * virtual method to fetch the next lexeme.
     *
     * A lexer derived from LexerBase is required by LrParserBase for
     * example.
     *
     * \sa LrParserBase::parse(LexerBase &, Logger *)
     */
    template<typename Ch, typename Val> struct LexerBase
    {
      typedef Ch                          CharType;
      typedef std::basic_string<CharType> StringType;
      typedef Val                         ValueType;

      struct LexerException: std::runtime_error
      {
	long lineNumber;
	LexerException(std::string const &message, long lineNumber):
	  std::runtime_error(message), lineNumber(lineNumber) {}
      };

      struct Lexeme
      {
	int type;
	ValueType value;
	Lexeme(int type = -1, ValueType value = ValueType()): type(type), value(value) {}
      };

      /**
       * Extract the next lexeme from the input.
       *
       * \param l The extracted lexeme is stored herein. If l.type is -1
       * upon return this idicates EOI (end of input).
       */
      virtual void getNext(Lexeme &l) = 0;

      /**
       * \return The text corresponding the the last lexeme extracted by
       * getNext(). This method may also be used within
       * Context::lexerError to fetch the faulty character.
       */
      virtual StringType getText() const = 0;

      /**
       * \return The type of the last lexeme extracted by
       * getNext(). -1 indicates EOI.
       */
      virtual int getType() const = 0;

      virtual ~LexerBase() {}
    };
  }
}

#endif // ARCHON_PARSER_LEXER_HPP
