# This file is part of the Archon project, a suite of C++ libraries.
#
# Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

from __future__ import annotations

import typing
import dataclasses
import collections.abc as _cabc
import textwrap
import io
import unittest
import pathlib

import archon.text_pos as _tp
import archon.log as _l
import archon.test as _t
import archon.cpp_preprocess as _cp


def test_CppPreprocess_Tokenizer_Basics(context: _t.Context) -> None:
    _check_tokenize(context, "", [], 1, 0)
    _check_tokenize(context, "\n", [
        (_cp.TokenType.NEWLINE, "\n", 1, 0, ()),
    ], 2, 0)
    _check_tokenize(context, " ", [
        (_cp.TokenType.WHITESPACE, " ", 1, 0, ()),
    ], 1, 1)
    _check_tokenize(context, "// foo", [
        (_cp.TokenType.LINE_COMMENT, "// foo", 1, 0, ()),
    ], 1, 6)
    _check_tokenize(context, _trim_cpp_text(r"""
      /*
      foo
      */
    """), [
        (_cp.TokenType.BLOCK_COMMENT, "/*\nfoo\n*/", 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 3, 2, ()),
    ], 4, 0)
    _check_tokenize(context, "#", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
    ], 1, 1)
    _check_tokenize(context, "##", [
        (_cp.TokenType.HASH_HASH, "##", 1, 0, ()),
    ], 1, 2)
    _check_tokenize(context, _trim_cpp_text(r"""
      R"(
      foo
      )"
    """), [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(\nfoo\n)"', 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 3, 2, ()),
    ], 4, 0)
    _check_tokenize(context, '"foo"', [
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 0, ()),
    ], 1, 5)
    _check_tokenize(context, "'f'", [
        (_cp.TokenType.CHAR_LIT, "'f'", 1, 0, ()),
    ], 1, 3)
    _check_tokenize(context, "7", [
        (_cp.TokenType.NUMBER, "7", 1, 0, ()),
    ], 1, 1)
    _check_tokenize(context, "foo", [
        (_cp.TokenType.IDENTIFIER, "foo", 1, 0, ()),
    ], 1, 3)
    _check_tokenize(context, "+=", [
        (_cp.TokenType.PUNCT, "+=", 1, 0, ()),
    ], 1, 2)
    _check_tokenize(context, "$", [
        (_cp.TokenType.STRAY_CHAR, "$", 1, 0, ()),
    ], 1, 1)
    _check_tokenize(context, "\u20AC", [
        (_cp.TokenType.BAD_CHAR, "\u20AC", 1, 0, ()),
    ], 1, 1, [
        ('Invalid character: EURO SIGN', _tp.TextPos(1, 0)),
    ])

    _check_tokenize(context, _trim_cpp_text(r"""
      x /* foo */ 7
      ("bar")
    """), [
        (_cp.TokenType.IDENTIFIER, "x", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 1, ()),
        (_cp.TokenType.BLOCK_COMMENT, "/* foo */", 1, 2, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 11, ()),
        (_cp.TokenType.NUMBER, "7", 1, 12, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 13, ()),
        (_cp.TokenType.PUNCT, "(", 2, 0, ()),
        (_cp.TokenType.STRING_LIT, '"bar"', 2, 1, ()),
        (_cp.TokenType.PUNCT, ")", 2, 6, ()),
        (_cp.TokenType.NEWLINE, "\n", 2, 7, ()),
    ], 3, 0)

    # Line splicing
    _check_tokenize(context, "fo\\\no", [
        (_cp.TokenType.IDENTIFIER, "foo", 1, 0, ()),
    ], 2, 1)
    _check_tokenize(context, "fo\\ \no", [
        (_cp.TokenType.IDENTIFIER, "foo", 1, 0, ()),
    ], 2, 1)


def test_CppPreprocess_Tokenizer_LineComment(context: _t.Context) -> None:
    _check_tokenize(context, "// first line \\\nsecond line\nthird line", [
        (_cp.TokenType.LINE_COMMENT, "// first line second line", 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 2, 11, ()),
        (_cp.TokenType.IDENTIFIER, "third", 3, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 3, 5, ()),
        (_cp.TokenType.IDENTIFIER, "line", 3, 6, ()),
    ], 3, 10)
    _check_tokenize(context, "// foo\\\n\n", [
        (_cp.TokenType.LINE_COMMENT, "// foo", 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 2, 0, ()),
    ], 3, 0)
    _check_tokenize(context, "// foo\\\n", [
        (_cp.TokenType.LINE_COMMENT, "// foo", 1, 0, ()),
    ], 2, 0)
    # Non-nesting
    _check_tokenize(context, "// /*\n", [
        (_cp.TokenType.LINE_COMMENT, "// /*", 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 5, ()),
    ], 2, 0)


def test_CppPreprocess_Tokenizer_BlockComment(context: _t.Context) -> None:
    _check_tokenize(context, "/***/", [
        (_cp.TokenType.BLOCK_COMMENT, "/***/", 1, 0, ()),
    ], 1, 5)
    _check_tokenize(context, "/* foo\n", [
        (_cp.TokenType.BAD_BLOCK_COMMENT, "/* foo\n", 1, 0, ()),
    ], 2, 0, [
        ("Unterminated block comment", _tp.TextPos(1, 0)),
    ])
    # Non-nesting
    _check_tokenize(context, "/* // */", [
        (_cp.TokenType.BLOCK_COMMENT, "/* // */", 1, 0, ()),
    ], 1, 8)
    _check_tokenize(context, "/* foo /* bar */", [
        (_cp.TokenType.BLOCK_COMMENT, "/* foo /* bar */", 1, 0, ()),
    ], 1, 16)


def test_CppPreprocess_Tokenizer_RawStringLit(context: _t.Context) -> None:
    _check_tokenize(context, 'R"(foo)"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"', 1, 0, ()),
    ], 1, 8)
    _check_tokenize(context, 'R""""(foo)""""', [
        (_cp.TokenType.RAW_STRING_LIT, 'R""""(foo)""""', 1, 0, ()),
    ], 1, 14)
    _check_tokenize(context, 'R"01234567890123456(foo)01234567890123456"', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"01234567890123456(foo)01234567890123456"', 1, 0, ()),
    ], 1, 42, [
        ('Too many characters in raw string literal delimiter', _tp.TextPos(1, 18)),
    ])
    _check_tokenize(context, 'R"foo\nbar', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"foo', 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 5, ()),
        (_cp.TokenType.IDENTIFIER, 'bar', 2, 0, ()),
    ], 2, 3, [
        ('Missing left parenthesis in raw string literal', _tp.TextPos(1, 5)),
    ])
    _check_tokenize(context, 'R"foo(\nbar', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"foo(\nbar', 1, 0, ()),
    ], 2, 3, [
        ('Unterminated raw string literal', _tp.TextPos(1, 0)),
    ])
    _check_tokenize(context, 'R"\\()\\"', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"\\()\\"', 1, 0, ()),
    ], 1, 7, [
        ('Invalid character ("\\\\") in raw string literal delimiter', _tp.TextPos(1, 2)),
    ])
    _check_tokenize(context, 'R"\\()"', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"\\()"', 1, 0, ()),
    ], 1, 6, [
        ('Invalid character ("\\\\") in raw string literal delimiter', _tp.TextPos(1, 2)),
        ('Unterminated raw string literal', _tp.TextPos(1, 0)),
    ])
    # Line splicing turned off immediately after `R"`
    _check_tokenize(context, 'R"\\\n()"', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"\\', 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 3, ()),
        (_cp.TokenType.PUNCT, '(', 2, 0, ()),
        (_cp.TokenType.PUNCT, ')', 2, 1, ()),
        (_cp.TokenType.BAD_STRING_LIT, '"', 2, 2, ()),
    ], 2, 3, [
        ('Missing left parenthesis in raw string literal', _tp.TextPos(1, 3)),
        ('Unterminated string literal', _tp.TextPos(2, 2)),
    ])
    # Prefix and suffix
    _check_tokenize(context, 'LR"(foo)"bar', [
        (_cp.TokenType.RAW_STRING_LIT, 'LR"(foo)"bar', 1, 0, ()),
    ], 1, 12)
    # Invalid prefix
    _check_tokenize(context, 'XR"(foo)"', [
        (_cp.TokenType.IDENTIFIER, 'XR', 1, 0, ()),
        (_cp.TokenType.STRING_LIT, '"(foo)"', 1, 2, ()),
    ], 1, 9)
    # UCN in suffix
    _check_tokenize(context, 'R"(foo)"\\u00e9', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"\u00e9', 1, 0, (_cp.ReplacementMap.Entry(8, 1, "\\u00e9"),)),
    ], 1, 14)

    _check_tokenize(context, 'u8R"()" uR"()" UR"()"', [
        (_cp.TokenType.RAW_STRING_LIT, 'u8R"()"', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 7, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'uR"()"', 1, 8, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 14, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'UR"()"', 1, 15, ()),
    ], 1, 21)


def test_CppPreprocess_Tokenizer_StringLit(context: _t.Context) -> None:
    _check_tokenize(context, '"\\\\\nn"', [
        (_cp.TokenType.STRING_LIT, '"\\n"', 1, 0, ()),
    ], 2, 2)
    _check_tokenize(context, '"foo"123', [
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 0, ()),
        (_cp.TokenType.NUMBER, "123", 1, 5, ()),
    ], 1, 8)
    _check_tokenize(context, '"foo', [
        (_cp.TokenType.BAD_STRING_LIT, '"foo', 1, 0, ()),
    ], 1, 4, [
        ('Unterminated string literal', _tp.TextPos(1, 0)),
    ])
    _check_tokenize(context, '"foo\nbar"', [
        (_cp.TokenType.BAD_STRING_LIT, '"foo', 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 4, ()),
        (_cp.TokenType.IDENTIFIER, 'bar', 2, 0, ()),
        (_cp.TokenType.BAD_STRING_LIT, '"', 2, 3, ()),
    ], 2, 4, [
        ('Unterminated string literal', _tp.TextPos(1, 0)),
        ('Unterminated string literal', _tp.TextPos(2, 3)),
    ])
    # Prefix and suffix
    _check_tokenize(context, 'L"foo"bar', [
        (_cp.TokenType.STRING_LIT, 'L"foo"bar', 1, 0, ()),
    ], 1, 9)
    # Invalid prefix
    _check_tokenize(context, 'X"foo"', [
        (_cp.TokenType.IDENTIFIER, 'X', 1, 0, ()),
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 1, ()),
    ], 1, 6)
    # UCN in suffix
    _check_tokenize(context, '"foo"\\u00e9', [
        (_cp.TokenType.STRING_LIT, '"foo"\u00e9', 1, 0, (_cp.ReplacementMap.Entry(5, 1, "\\u00e9"),)),
    ], 1, 11)

    _check_tokenize(context, 'u8"" u"" U""', [
        (_cp.TokenType.STRING_LIT, 'u8""', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 4, ()),
        (_cp.TokenType.STRING_LIT, 'u""', 1, 5, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.STRING_LIT, 'U""', 1, 9, ()),
    ], 1, 12)


def test_CppPreprocess_Tokenizer_CharLit(context: _t.Context) -> None:
    _check_tokenize(context, "'c'123", [
        (_cp.TokenType.CHAR_LIT, "'c'", 1, 0, ()),
        (_cp.TokenType.NUMBER, "123", 1, 3, ()),
    ], 1, 6)
    _check_tokenize(context, "'c", [
        (_cp.TokenType.BAD_CHAR_LIT, "'c", 1, 0, ()),
    ], 1, 2, [
        ('Unterminated character literal', _tp.TextPos(1, 0)),
    ])
    _check_tokenize(context, "'c\n'", [
        (_cp.TokenType.BAD_CHAR_LIT, "'c", 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 2, ()),
        (_cp.TokenType.BAD_CHAR_LIT, "'", 2, 0, ()),
    ], 2, 1, [
        ('Unterminated character literal', _tp.TextPos(1, 0)),
        ('Unterminated character literal', _tp.TextPos(2, 0)),
    ])
    # Prefix and suffix
    _check_tokenize(context, "L'c'foo", [
        (_cp.TokenType.CHAR_LIT, "L'c'foo", 1, 0, ()),
    ], 1, 7)
    # Invalid prefix
    _check_tokenize(context, "X'c'", [
        (_cp.TokenType.IDENTIFIER, "X", 1, 0, ()),
        (_cp.TokenType.CHAR_LIT, "'c'", 1, 1, ()),
    ], 1, 4)
    # UCN in suffix
    _check_tokenize(context, "'c'\\u00e9", [
        (_cp.TokenType.CHAR_LIT, "'c'\u00e9", 1, 0, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),)),
    ], 1, 9)

    _check_tokenize(context, "u8'' u'' U''", [
        (_cp.TokenType.CHAR_LIT, "u8''", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 4, ()),
        (_cp.TokenType.CHAR_LIT, "u''", 1, 5, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.CHAR_LIT, "U''", 1, 9, ()),
    ], 1, 12)


def test_CppPreprocess_Tokenizer_Number(context: _t.Context) -> None:
    # Basic integer and floating point introducers
    _check_tokenize(context, "0 42 0x1A .5 1. 1.23", [
        (_cp.TokenType.NUMBER, "0", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 1, ()),
        (_cp.TokenType.NUMBER, "42", 1, 2, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 4, ()),
        (_cp.TokenType.NUMBER, "0x1A", 1, 5, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 9, ()),
        (_cp.TokenType.NUMBER, ".5", 1, 10, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 12, ()),
        (_cp.TokenType.NUMBER, "1.", 1, 13, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 15, ()),
        (_cp.TokenType.NUMBER, "1.23", 1, 16, ()),
    ], 1, 20)

    # Exponent signs (`+` and `-`) are consumed ONLY if preceded by `e`, `E`, `p`, or `P`
    _check_tokenize(context, "1e+1 2E-2 0x3p+3 0X4P-4", [
        (_cp.TokenType.NUMBER, "1e+1", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 4, ()),
        (_cp.TokenType.NUMBER, "2E-2", 1, 5, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 9, ()),
        (_cp.TokenType.NUMBER, "0x3p+3", 1, 10, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 16, ()),
        (_cp.TokenType.NUMBER, "0X4P-4", 1, 17, ()),
    ], 1, 23)

    # Suffixes and user-defined literals
    _check_tokenize(context, "123ull 3.14f 42_foo 0xXYZ", [
        (_cp.TokenType.NUMBER, "123ull", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.NUMBER, "3.14f", 1, 7, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 12, ()),
        (_cp.TokenType.NUMBER, "42_foo", 1, 13, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 19, ()),
        (_cp.TokenType.NUMBER, "0xXYZ", 1, 20, ()),
    ], 1, 25)

    # Digit separators
    _check_tokenize(context, "1'000'000 3.14'159", [
        (_cp.TokenType.NUMBER, "1'000'000", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 9, ()),
        (_cp.TokenType.NUMBER, "3.14'159", 1, 10, ()),
    ], 1, 18)

    # UCNs inside a number
    _check_tokenize(context, "123\\u00e9", [
        (_cp.TokenType.NUMBER, "123\u00e9", 1, 0, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),)),
    ], 1, 9)

    # Token separation boundary: `.` must be followed by a digit to introduce a NUMBER
    _check_tokenize(context, ".e", [
        (_cp.TokenType.PUNCT, ".", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "e", 1, 1, ()),
    ], 1, 2)

    # Token separation boundary: Unary signs are PUNCT, not part of the number
    _check_tokenize(context, "+1", [
        (_cp.TokenType.PUNCT, "+", 1, 0, ()),
        (_cp.TokenType.NUMBER, "1", 1, 1, ()),
    ], 1, 2)

    # Token separation boundary: `+` stops the number if not preceded by an exponent letter
    _check_tokenize(context, "1+2", [
        (_cp.TokenType.NUMBER, "1", 1, 0, ()),
        (_cp.TokenType.PUNCT, "+", 1, 1, ()),
        (_cp.TokenType.NUMBER, "2", 1, 2, ()),
    ], 1, 3)

    # Token separation boundary: Combining exponents and regular operators
    _check_tokenize(context, "1e+2+3", [
        (_cp.TokenType.NUMBER, "1e+2", 1, 0, ()),
        (_cp.TokenType.PUNCT, "+", 1, 4, ()),
        (_cp.TokenType.NUMBER, "3", 1, 5, ()),
    ], 1, 6)

    # A pp-number consumes the sign even if not followed by a digit
    _check_tokenize(context, "1e+", [
        (_cp.TokenType.NUMBER, "1e+", 1, 0, ()),
    ], 1, 3)

    # Digit separator boundary rule: A single quote is only part of the pp-number if
    # followed by a digit, letter, or underscore from the basic character set
    _check_tokenize(context, "1'\n", [
        (_cp.TokenType.NUMBER, "1", 1, 0, ()),
        (_cp.TokenType.BAD_CHAR_LIT, "'", 1, 1, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 2, ()),
    ], 2, 0, [
        ('Unterminated character literal', _tp.TextPos(1, 1)),
    ])


def test_CppPreprocess_Tokenizer_Identifier(context: _t.Context) -> None:
    _check_tokenize(context, "\u00e9", [
        (_cp.TokenType.IDENTIFIER, "\u00e9", 1, 0, ()),
    ], 1, 1)
    _check_tokenize(context, "e\u0301 n\\u0303", [
        (_cp.TokenType.BAD_IDENTIFIER, "e\u0301", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 2, ()),
        (_cp.TokenType.BAD_IDENTIFIER, "n\u0303", 1, 3, (_cp.ReplacementMap.Entry(1, 1, "\\u0303"),)),
    ], 1, 10, [
        ("Identifier does not conform to Unicode Normalization Form C (NFC)", _tp.TextPos(1, 0)),
        ("Identifier does not conform to Unicode Normalization Form C (NFC)", _tp.TextPos(1, 3)),
    ])
    _check_tokenize(context, "\u0967ab \\u0967ab", [
        (_cp.TokenType.BAD_CHAR, "\u0967", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "ab", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 3, ()),
        (_cp.TokenType.BAD_IDENTIFIER, "\\u0967ab", 1, 4, ()),
    ], 1, 12, [
        ('Invalid character: DEVANAGARI DIGIT ONE', _tp.TextPos(1, 0)),
        ('Invalid UCN code point (U+0967): Must refer to character in XID_Start / XID_Continue', _tp.TextPos(1, 6)),
    ])

    # If not followed by a quote, these are just normal identifiers
    _check_tokenize(context, "u8 u U L R u8R uR UR LR", [
        (_cp.TokenType.IDENTIFIER, "u8", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 2, ()),
        (_cp.TokenType.IDENTIFIER, "u", 1, 3, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 4, ()),
        (_cp.TokenType.IDENTIFIER, "U", 1, 5, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.IDENTIFIER, "L", 1, 7, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.IDENTIFIER, "R", 1, 9, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 10, ()),
        (_cp.TokenType.IDENTIFIER, "u8R", 1, 11, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 14, ()),
        (_cp.TokenType.IDENTIFIER, "uR", 1, 15, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 17, ()),
        (_cp.TokenType.IDENTIFIER, "UR", 1, 18, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 20, ()),
        (_cp.TokenType.IDENTIFIER, "LR", 1, 21, ()),
    ], 1, 23)


def test_CppPreprocess_Tokenizer_Punctuation(context: _t.Context) -> None:
    # Maximal munch
    _check_tokenize(context, "x+++++y", [
        (_cp.TokenType.IDENTIFIER, "x", 1, 0, ()),
        (_cp.TokenType.PUNCT, "++", 1, 1, ()),
        (_cp.TokenType.PUNCT, "++", 1, 3, ()),
        (_cp.TokenType.PUNCT, "+", 1, 5, ()),
        (_cp.TokenType.IDENTIFIER, "y", 1, 6, ()),
    ], 1, 7)
    _check_tokenize(context, "... .* ->*", [
        (_cp.TokenType.PUNCT, "...", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 3, ()),
        (_cp.TokenType.PUNCT, ".*", 1, 4, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.PUNCT, "->*", 1, 7, ()),
    ], 1, 10)

    # Digraphs vs standard punctuation
    _check_tokenize(context, "<% %> <: :> %:%:", [
        (_cp.TokenType.PUNCT, "<%", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 2, ()),
        (_cp.TokenType.PUNCT, "%>", 1, 3, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 5, ()),
        (_cp.TokenType.PUNCT, "<:", 1, 6, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.PUNCT, ":>", 1, 9, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 11, ()),
        (_cp.TokenType.PUNCT, "%:%:", 1, 12, ()), # Digraph for ##
    ], 1, 16)

    # Special handling of `<:` (digraph for `[`)
    _check_tokenize(context, "std::vector<::std::string>", [
        (_cp.TokenType.IDENTIFIER, "std", 1, 0, ()),
        (_cp.TokenType.PUNCT, "::", 1, 3, ()),
        (_cp.TokenType.IDENTIFIER, "vector", 1, 5, ()),
        (_cp.TokenType.PUNCT, "<", 1, 11, ()),   # Must be `<`
        (_cp.TokenType.PUNCT, "::", 1, 12, ()),  # Must be `::`
        (_cp.TokenType.IDENTIFIER, "std", 1, 14, ()),
        (_cp.TokenType.PUNCT, "::", 1, 17, ()),
        (_cp.TokenType.IDENTIFIER, "string", 1, 19, ()),
        (_cp.TokenType.PUNCT, ">", 1, 25, ()),
    ], 1, 26)
    _check_tokenize(context, "a<:::ns::foo:>", [
        (_cp.TokenType.IDENTIFIER, "a", 1, 0, ()),
        (_cp.TokenType.PUNCT, "<:", 1, 1, ()),
        (_cp.TokenType.PUNCT, "::", 1, 3, ()),
        (_cp.TokenType.IDENTIFIER, "ns", 1, 5, ()),
        (_cp.TokenType.PUNCT, "::", 1, 7, ()),
        (_cp.TokenType.IDENTIFIER, "foo", 1, 9, ()),
        (_cp.TokenType.PUNCT, ":>", 1, 12, ()),
    ], 1, 14)
    _check_tokenize(context, "a<::>", [
        (_cp.TokenType.IDENTIFIER, "a", 1, 0, ()),
        (_cp.TokenType.PUNCT, "<:", 1, 1, ()),
        (_cp.TokenType.PUNCT, ":>", 1, 3, ()),
    ], 1, 5)

    # Special handling of `[:` (C++26 reflection splice)
    _check_tokenize(context, "a[::ns::foo]", [
        (_cp.TokenType.IDENTIFIER, "a", 1, 0, ()),
        (_cp.TokenType.PUNCT, "[", 1, 1, ()),
        (_cp.TokenType.PUNCT, "::", 1, 2, ()),
        (_cp.TokenType.IDENTIFIER, "ns", 1, 4, ()),
        (_cp.TokenType.PUNCT, "::", 1, 6, ()),
        (_cp.TokenType.IDENTIFIER, "foo", 1, 8, ()),
        (_cp.TokenType.PUNCT, "]", 1, 11, ()),
    ], 1, 12)
    _check_tokenize(context, "a[:>", [
        (_cp.TokenType.IDENTIFIER, "a", 1, 0, ()),
        (_cp.TokenType.PUNCT, "[", 1, 1, ()),
        (_cp.TokenType.PUNCT, ":>", 1, 2, ()),
    ], 1, 4)
    _check_tokenize(context, "[:::ns::foo:]", [
        (_cp.TokenType.PUNCT, "[:", 1, 0, ()),
        (_cp.TokenType.PUNCT, "::", 1, 2, ()),
        (_cp.TokenType.IDENTIFIER, "ns", 1, 4, ()),
        (_cp.TokenType.PUNCT, "::", 1, 6, ()),
        (_cp.TokenType.IDENTIFIER, "foo", 1, 8, ()),
        (_cp.TokenType.PUNCT, ":]", 1, 11, ()),
    ], 1, 13)


def test_CppPreprocess_Tokenizer_UniversalCharacterNames(context: _t.Context) -> None:
    _check_tokenize(context, "\\u00e9", [
        (_cp.TokenType.IDENTIFIER, "\u00e9", 1, 0, (_cp.ReplacementMap.Entry(0, 1, "\\u00e9"),)),
    ], 1, 6)
    _check_tokenize(context, "\\U000000e9", [
        (_cp.TokenType.IDENTIFIER, "\u00e9", 1, 0, (_cp.ReplacementMap.Entry(0, 1, "\\U000000e9"),)),
    ], 1, 10)
    _check_tokenize(context, "\\u{e9}", [
        (_cp.TokenType.IDENTIFIER, "\u00e9", 1, 0, (_cp.ReplacementMap.Entry(0, 1, "\\u{e9}"),)),
    ], 1, 6)
    _check_tokenize(context, "\\N{LATIN SMALL LETTER E WITH ACUTE}", [
        (_cp.TokenType.IDENTIFIER, "\u00e9", 1, 0, (
            _cp.ReplacementMap.Entry(0, 1, "\\N{LATIN SMALL LETTER E WITH ACUTE}"),
        )),
    ], 1, 35)
    _check_tokenize(context, "\\u0041", [
        (_cp.TokenType.BAD_IDENTIFIER, "\\u0041", 1, 0, ()),
    ], 1, 6, [
        ('Invalid UCN code point (U+0041): Refers to character in basic character set', _tp.TextPos(1, 2)),
    ])
    _check_tokenize(context, "\\u20AC", [
        (_cp.TokenType.BAD_IDENTIFIER, "\\u20AC", 1, 0, ()),
    ], 1, 6, [
        ('Invalid UCN code point (U+20AC): Must refer to character in XID_Start / XID_Continue', _tp.TextPos(1, 2)),
    ])
    _check_tokenize(context, "\\uD800", [
        (_cp.TokenType.BAD_IDENTIFIER, "\\uD800", 1, 0, ()),
    ], 1, 6, [
        ('Invalid UCN code point (U+D800): Refers to character in surrogate range', _tp.TextPos(1, 2)),
    ])
    _check_tokenize(context, "\\u{FFFFFFFF}", [
        (_cp.TokenType.BAD_IDENTIFIER, "\\u{FFFFFFFF}", 1, 0, ()),
    ], 1, 12, [
        ('Invalid UCN code point (U+FFFFFFFF): Outside Unicode range', _tp.TextPos(1, 3)),
    ])
    _check_tokenize(context, "\\N{foo}", [
        (_cp.TokenType.BAD_IDENTIFIER, "\\N{foo}", 1, 0, ()),
    ], 1, 7, [
        ('Invalid Unicode character name ("foo") in UCN', _tp.TextPos(1, 3)),
    ])

    # Line splicing must occur despite the `R"`
    _check_tokenize(context, '\\N{R"(\\\n)"}', [
        (_cp.TokenType.BAD_IDENTIFIER, '\\N{R"()"}', 1, 0, ()),
    ], 2, 3, [
        ('Invalid Unicode character name ("R\\"()\\"") in UCN', _tp.TextPos(1, 3)),
    ])
    # But line splicing must not occur if there is no UCN because the closing brace is
    # missing, so `R"` does start a raw string literal (requires backtracking across
    # splicing decision)
    _check_tokenize(context, '\\N{R"(\\\n)"', [
        (_cp.TokenType.STRAY_CHAR, '\\', 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, 'N', 1, 1, ()),
        (_cp.TokenType.PUNCT, '{', 1, 2, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(\\\n)"', 1, 3, ()),
    ], 2, 2)

    _check_tokenize(context, "u\\\n8\"foo\" \\u00\\\ne9", [
        (_cp.TokenType.STRING_LIT, 'u8"foo"', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 2, 6, ()),
        (_cp.TokenType.IDENTIFIER, "\u00e9", 2, 7, (_cp.ReplacementMap.Entry(0, 1, "\\u00e9"),)),
    ], 3, 2)

    _check_tokenize(context, "\\u00E \\u{123 \\N{foo", [
        (_cp.TokenType.STRAY_CHAR, "\\", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "u00E", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 5, ()),
        (_cp.TokenType.STRAY_CHAR, "\\", 1, 6, ()),
        (_cp.TokenType.IDENTIFIER, "u", 1, 7, ()),
        (_cp.TokenType.PUNCT, "{", 1, 8, ()),
        (_cp.TokenType.NUMBER, "123", 1, 9, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 12, ()),
        (_cp.TokenType.STRAY_CHAR, "\\", 1, 13, ()),
        (_cp.TokenType.IDENTIFIER, "N", 1, 14, ()),
        (_cp.TokenType.PUNCT, "{", 1, 15, ()),
        (_cp.TokenType.IDENTIFIER, "foo", 1, 16, ()),
    ], 1, 19)


def test_CppPreprocess_Tokenizer_HeaderName(context: _t.Context) -> None:
    # Standard `#include` directives (angle brackets and quotes)
    _check_tokenize(context, "#include <foo/bar.h>", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "include", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.HEADER_NAME, "<foo/bar.h>", 1, 9, ()),
    ], 1, 20)
    _check_tokenize(context, '# include "my_header.h"', [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 1, ()),
        (_cp.TokenType.IDENTIFIER, "include", 1, 2, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 9, ()),
        (_cp.TokenType.HEADER_NAME, '"my_header.h"', 1, 10, ()),
    ], 1, 23)

    # C++26 `#embed` directive
    _check_tokenize(context, "#embed <data.bin>", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "embed", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.HEADER_NAME, "<data.bin>", 1, 7, ()),
    ], 1, 17)

    # C++20 `import` declaration (starts at the beginning of the line)
    _check_tokenize(context, "import <iostream>;", [
        (_cp.TokenType.IDENTIFIER, "import", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.HEADER_NAME, "<iostream>", 1, 7, ()),
        (_cp.TokenType.PUNCT, ";", 1, 17, ()),
    ], 1, 18)
    _check_tokenize(context, "int import <iostream>;", [
        (_cp.TokenType.IDENTIFIER, "int", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 3, ()),
        (_cp.TokenType.IDENTIFIER, "import", 1, 4, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 10, ()),
        (_cp.TokenType.PUNCT, "<", 1, 11, ()),
        (_cp.TokenType.IDENTIFIER, "iostream", 1, 12, ()),
        (_cp.TokenType.PUNCT, ">", 1, 20, ()),
        (_cp.TokenType.PUNCT, ";", 1, 21, ()),
    ], 1, 22)

    # `__has_include` and `__has_embed` within valid conditional directives
    _check_tokenize(context, "#if __has_include(<vector>)", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "if", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 3, ()),
        (_cp.TokenType.IDENTIFIER, "__has_include", 1, 4, ()),
        (_cp.TokenType.PUNCT, "(", 1, 17, ()),
        (_cp.TokenType.HEADER_NAME, "<vector>", 1, 18, ()),
        (_cp.TokenType.PUNCT, ")", 1, 26, ()),
    ], 1, 27)
    _check_tokenize(context, '#elif __has_embed("data.txt")', [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "elif", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 5, ()),
        (_cp.TokenType.IDENTIFIER, "__has_embed", 1, 6, ()),
        (_cp.TokenType.PUNCT, "(", 1, 17, ()),
        (_cp.TokenType.HEADER_NAME, '"data.txt"', 1, 18, ()),
        (_cp.TokenType.PUNCT, ")", 1, 28, ()),
    ], 1, 29)

    # `__has_include` and `__has_embed` within #embed parameters
    _check_tokenize(context, '#embed "data.bin" limit(__has_include(<foo.h>))', [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "embed", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.HEADER_NAME, '"data.bin"', 1, 7, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 17, ()),
        (_cp.TokenType.IDENTIFIER, "limit", 1, 18, ()),
        (_cp.TokenType.PUNCT, "(", 1, 23, ()),
        (_cp.TokenType.IDENTIFIER, "__has_include", 1, 24, ()),
        (_cp.TokenType.PUNCT, "(", 1, 37, ()),
        (_cp.TokenType.HEADER_NAME, "<foo.h>", 1, 38, ()),
        (_cp.TokenType.PUNCT, ")", 1, 45, ()),
        (_cp.TokenType.PUNCT, ")", 1, 46, ()),
    ], 1, 47)
    _check_tokenize(context, '#embed "main.bin" if_empty(__has_embed(<fallback.bin>))', [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "embed", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.HEADER_NAME, '"main.bin"', 1, 7, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 17, ()),
        (_cp.TokenType.IDENTIFIER, "if_empty", 1, 18, ()),
        (_cp.TokenType.PUNCT, "(", 1, 26, ()),
        (_cp.TokenType.IDENTIFIER, "__has_embed", 1, 27, ()),
        (_cp.TokenType.PUNCT, "(", 1, 38, ()),
        (_cp.TokenType.HEADER_NAME, "<fallback.bin>", 1, 39, ()),
        (_cp.TokenType.PUNCT, ")", 1, 53, ()),
        (_cp.TokenType.PUNCT, ")", 1, 54, ()),
    ], 1, 55)

    # Contexts where `<` and `"` should NOT form a header name
    _check_tokenize(context, "#define FOO <bar.h>", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "define", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 7, ()),
        (_cp.TokenType.IDENTIFIER, "FOO", 1, 8, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 11, ()),
        (_cp.TokenType.PUNCT, "<", 1, 12, ()),
        (_cp.TokenType.IDENTIFIER, "bar", 1, 13, ()),
        (_cp.TokenType.PUNCT, ".", 1, 16, ()),
        (_cp.TokenType.IDENTIFIER, "h", 1, 17, ()),
        (_cp.TokenType.PUNCT, ">", 1, 18, ()),
    ], 1, 19)
    _check_tokenize(context, "#define CHECK __has_include(<foo>)", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "define", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 7, ()),
        (_cp.TokenType.IDENTIFIER, "CHECK", 1, 8, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 13, ()),
        (_cp.TokenType.IDENTIFIER, "__has_include", 1, 14, ()),
        (_cp.TokenType.PUNCT, "(", 1, 27, ()),
        (_cp.TokenType.PUNCT, "<", 1, 28, ()),
        (_cp.TokenType.IDENTIFIER, "foo", 1, 29, ()),
        (_cp.TokenType.PUNCT, ">", 1, 32, ()),
        (_cp.TokenType.PUNCT, ")", 1, 33, ()),
    ], 1, 34)

    # Unterminated header names
    _check_tokenize(context, "#include <unterminated\n", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "include", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.BAD_HEADER_NAME, "<unterminated", 1, 9, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 22, ()),
    ], 2, 0, [
        ('Unterminated header name (missing closing `>`)', _tp.TextPos(1, 9)),
    ])
    _check_tokenize(context, '#embed "unterminated\n', [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "embed", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.BAD_HEADER_NAME, '"unterminated', 1, 7, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 20, ()),
    ], 2, 0, [
        ('Unterminated header name (missing closing `"`)', _tp.TextPos(1, 7)),
    ])


def test_CppPreprocess_ParseDefineDirective(context: _t.Context) -> None:
    file_index = 0
    _check_valid_define_directive(context, "M", file_index, "M", None, False, ())
    _check_valid_define_directive(context, "M()", file_index, "M", (), False, ())
    _check_valid_define_directive(context, "M(x)", file_index, "M", ("x",), False, ())
    _check_valid_define_directive(context, "M(...)", file_index, "M", (), True, ())
    _check_valid_define_directive(context, "M(x, ...)", file_index, "M", ("x",), True, ())
    _check_valid_define_directive(context, "M foo", file_index, "M", None, False, (
        _cp.Token(_cp.TokenType.IDENTIFIER, "foo", _cp.Position(file_index, 2)),
    ))
    _check_valid_define_directive(context, "M(x) foo", file_index, "M", ("x",), False, (
        _cp.Token(_cp.TokenType.IDENTIFIER, "foo", _cp.Position(file_index, 5)),
    ))
    _check_valid_define_directive(context, "M(x) x", file_index, "M", ("x",), False, (
        _cp.ParamRef(0, _cp.Position(file_index, 5)),
    ))
    _check_valid_define_directive(context, "M(x, y) y x", file_index, "M", ("x", "y"), False, (
        _cp.ParamRef(1, _cp.Position(file_index, 8)),
        _cp.Token(_cp.TokenType.WHITESPACE, " ", _cp.Position(file_index, 9)),
        _cp.ParamRef(0, _cp.Position(file_index, 10)),
    ))
    _check_bad_define_directive(context, "defined foo", [
        ("`defined` cannot be used as a macro name", _tp.TextPos(1, 0)),
    ])
    _check_bad_define_directive(context, "defined(x) foo", [
        ("`defined` cannot be used as a macro name", _tp.TextPos(1, 0)),
    ])
    _check_bad_define_directive(context, "M'c'", [
        ("Missing whitespace after macro name in definition of object-like macro", _tp.TextPos(1, 1)),
    ])
    _check_valid_define_directive(context, "M(x)'c'", file_index, "M", ("x",), False, [
        _cp.Token(_cp.TokenType.CHAR_LIT, "'c'", _cp.Position(file_index, 4)),
    ])

    # `__VA_ARGS__`
    _check_bad_define_directive(context, "M __VA_ARGS__", [
        ("`__VA_ARGS__` can only appear in body of a variadic macro definition", _tp.TextPos(1, 2)),
    ])
    _check_bad_define_directive(context, "M(x) __VA_ARGS__", [
        ("`__VA_ARGS__` can only appear in body of a variadic macro definition", _tp.TextPos(1, 5)),
    ])
    _check_valid_define_directive(context, "M(...) __VA_ARGS__", file_index, "M", (), True, [
        _cp.VaArgs(_cp.Position(file_index, 7)),
    ])
    _check_bad_define_directive(context, "__VA_ARGS__ foo", [
        ("`__VA_ARGS__` can only appear in body of a variadic macro definition", _tp.TextPos(1, 0)),
    ])
    _check_bad_define_directive(context, "M(__VA_ARGS__) foo", [
        ('`__VA_ARGS__` can only appear in body of a variadic macro definition', _tp.TextPos(1, 2)),
    ])

    # `__VA_OPT__`
    _check_bad_define_directive(context, "M __VA_OPT__", [
        ("`__VA_OPT__` can only appear in body of a variadic macro definition", _tp.TextPos(1, 2)),
    ])
    _check_bad_define_directive(context, "M(x) __VA_OPT__", [
        ("`__VA_OPT__` can only appear in body of a variadic macro definition", _tp.TextPos(1, 5)),
    ])
    _check_bad_define_directive(context, "M(...) __VA_OPT__", [
        ("`__VA_OPT__` must be followed by `(`", _tp.TextPos(1, 17)),
    ])
    _check_bad_define_directive(context, "M(...) __VA_OPT__(foo", [
        ("Missing `)` after `__VA_OPT__(`", _tp.TextPos(1, 21)),
    ])
    _check_bad_define_directive(context, "M(...) __VA_OPT__((foo)", [
        ("Missing `)` after `__VA_OPT__(`", _tp.TextPos(1, 23)),
    ])
    _check_valid_define_directive(context, "M(...) __VA_OPT__(foo)", file_index, "M", (), True, [
        _cp.VaOpt((_cp.Token(_cp.TokenType.IDENTIFIER, "foo", _cp.Position(file_index, 18)),),
                  _cp.Position(file_index, 7)),
    ])
    _check_bad_define_directive(context, "M(...) __VA_OPT__(__VA_OPT__)", [
        ("`__VA_OPT__` cannot occur inside `__VA_OPT__(...)`", _tp.TextPos(1, 18)),
    ])
    _check_bad_define_directive(context, "__VA_OPT__ foo", [
        ("`__VA_OPT__` can only appear in body of a variadic macro definition", _tp.TextPos(1, 0)),
    ])
    _check_bad_define_directive(context, "M(__VA_OPT__) foo", [
        ("`__VA_OPT__` can only appear in body of a variadic macro definition", _tp.TextPos(1, 2)),
    ])

    # Stringification
    _check_valid_define_directive(context, "M #x", file_index, "M", None, False, [
        _cp.Token(_cp.TokenType.HASH, "#", _cp.Position(file_index, 2)),
        _cp.Token(_cp.TokenType.IDENTIFIER, "x", _cp.Position(file_index, 3)),
    ])
    _check_valid_define_directive(context, "M(x) #x", file_index, "M", ("x",), False, [
        _cp.StringifyOperation(_cp.ParamRef(0, _cp.Position(file_index, 6)), _cp.Position(file_index, 5)),
    ])
    _check_bad_define_directive(context, "M(x) #", [
        ('Missing operand of `#` (stringify operator) in definition of macro "M"',
         _tp.TextPos(1, 6)),
    ])
    _check_bad_define_directive(context, "M(x) #7", [
        ('Invalid operand of `#` (stringify operator) in definition of macro "M": '
         'Must be a parameter name, `__VA_ARGS__`, or `__VA_OPT__(...)`', _tp.TextPos(1, 6)),
    ])
    _check_bad_define_directive(context, "M(x) foo #y", [
        ('Invalid operand of `#` (stringify operator) in definition of macro "M": '
         'Must be a parameter name, `__VA_ARGS__`, or `__VA_OPT__(...)`', _tp.TextPos(1, 10)),
    ])
    _check_bad_define_directive(context, "M(x) #__VA_ARGS__", [
        ('`__VA_ARGS__` can only appear in body of a variadic macro definition', _tp.TextPos(1, 6)),
    ])
    _check_valid_define_directive(context, "M(...) #__VA_ARGS__", file_index, "M", (), True, [
        _cp.StringifyOperation(_cp.VaArgs(_cp.Position(file_index, 8)), _cp.Position(file_index, 7)),
    ])
    _check_bad_define_directive(context, "M(x) #__VA_OPT__()", [
        ('`__VA_OPT__` can only appear in body of a variadic macro definition', _tp.TextPos(1, 6)),
    ])
    _check_valid_define_directive(context, "M(...) #__VA_OPT__()", file_index, "M", (), True, [
        _cp.StringifyOperation(_cp.VaOpt((), _cp.Position(file_index, 8)), _cp.Position(file_index, 7)),
    ])

    # Token fusion
    _check_valid_define_directive(context, "M foo ## bar", file_index, "M", None, False, [
        _cp.FuseOperation(_cp.Token(_cp.TokenType.IDENTIFIER, "foo", _cp.Position(file_index, 2)), (
            _cp.FuseOperation.Right(_cp.Token(_cp.TokenType.IDENTIFIER, "bar", _cp.Position(file_index, 9)),
                                    _cp.Position(file_index, 6)),
        ), _cp.Position(file_index, 2)),
    ])
    _check_valid_define_directive(context, "M(x) foo ## x", file_index, "M", ("x",), False, [
        _cp.FuseOperation(_cp.Token(_cp.TokenType.IDENTIFIER, "foo", _cp.Position(file_index, 5)), (
            _cp.FuseOperation.Right(_cp.ParamRef(0, _cp.Position(file_index, 12)), _cp.Position(file_index, 9)),
        ), _cp.Position(file_index, 5)),
    ])
    _check_bad_define_directive(context, "M ## foo", [
        ('`##` (fuse operator) is not allowed at beginning of replacement list in definition of macro "M"',
         _tp.TextPos(1, 2)),
    ])
    _check_bad_define_directive(context, "M foo ##", [
        ('`##` (fuse operator) is not allowed at end of replacement list in definition of macro "M"',
         _tp.TextPos(1, 6)),
    ])
    _check_bad_define_directive(context, "M(x) ## foo", [
        ('`##` (fuse operator) is not allowed at beginning of replacement list in definition of macro "M"',
         _tp.TextPos(1, 5)),
    ])
    _check_bad_define_directive(context, "M(x) foo ##", [
        ('`##` (fuse operator) is not allowed at end of replacement list in definition of macro "M"',
         _tp.TextPos(1, 9)),
    ])
    _check_valid_define_directive(context, "M(x) x ## ## x", file_index, "M", ("x",), False, [
        _cp.FuseOperation(_cp.ParamRef(0, _cp.Position(file_index, 5)), (
            _cp.FuseOperation.Right(_cp.Token(_cp.TokenType.HASH_HASH, "##", _cp.Position(file_index, 10)),
                                    _cp.Position(file_index, 7)),
        ), _cp.Position(file_index, 5)),
        _cp.Token(_cp.TokenType.WHITESPACE, " ", _cp.Position(file_index, 12)),
        _cp.ParamRef(0, _cp.Position(file_index, 13)),
    ])








def _trim_cpp_text(text: str) -> str:
    return textwrap.dedent(text.removeprefix("\n"))


# (type, text, line_no, pos_on_line, ucn_map_entries)
type _ExpectedToken = tuple[_cp.TokenType, str, int, int, tuple[_cp.ReplacementMap.Entry, ...]]


def _check_tokenize(context: _t.Context, cpp_text: str, expected_tokens: list[_ExpectedToken],
                    expected_end_line_no: int, expected_end_pos_on_line: int,
                    expected_errors: list[tuple[str, _tp.TextPos]] | None = None) -> None:
    errors: list[_Error] = []
    position_tracker = _tp.TextPosTracker()
    path = pathlib.Path("test.cpp")
    file_index = 0
    def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
        context.check_equal(pos.file_index, file_index)
        text_pos = position_tracker.get_text_pos(pos.pos_in_file)
        errors.append(_Error(message % args, text_pos))
        file_context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
        _l.FileContextLogger(context.logger, file_context).error(message, *args)
    tokens, end_pos = _tokenize(path, file_index, cpp_text, position_tracker, error_handler)
    _check_tokens(context, tokens, expected_tokens, file_index, position_tracker)
    context.check_equal(end_pos.file_index, file_index)
    end_text_pos = position_tracker.get_text_pos(end_pos.pos_in_file)
    context.check_equal(end_text_pos.line_no, expected_end_line_no)
    context.check_equal(end_text_pos.pos_on_line, expected_end_pos_on_line)
    _check_errors(context, errors, expected_errors or [])


def _check_tokens(context: _t.Context, tokens: list[_cp.Token], expected_tokens: list[_ExpectedToken],
                  file_index: int, position_tracker: _tp.TextPosTracker) -> None:
    for i, (token, expected) in enumerate(zip(tokens, expected_tokens)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(token.type_, expected[0])
        subcontext.check_equal(token.text, expected[1])
        subcontext.check_equal(token.pos.file_index, file_index)
        text_pos = position_tracker.get_text_pos(token.pos.pos_in_file)
        subcontext.check_equal(text_pos.line_no, expected[2])
        subcontext.check_equal(text_pos.pos_on_line, expected[3])
        expected_ucn_map = _cp.ReplacementMap(expected[4]) if expected[4] else None
        subcontext.check_equal(token.ucn_map, expected_ucn_map)
        subcontext.check_not(token.synthetic)
    context.check_equal(len(tokens), len(expected_tokens))


def _check_valid_define_directive(context: _t.Context, text: str, file_index: int, expected_name: str,
                                  expected_params: tuple[str, ...] | None, expected_is_variadic: bool,
                                  expected_replacement: _cabc.Sequence[_cp.ReplaceElem]) -> None:
    error = False
    position_tracker = _tp.TextPosTracker()
    def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
        nonlocal error
        context.check_equal(pos.file_index, file_index)
        text_pos = position_tracker.get_text_pos(pos.pos_in_file)
        file_context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
        _l.FileContextLogger(context.logger, file_context).error(message, *args)
        error = True
    path = pathlib.Path("test.cpp")
    tokens, end_pos = _tokenize(path, file_index, text, position_tracker, error_handler)
    context.check_not(error)
    pos = _cp.Position(file_index, 0)
    macro_def = _cp.parse_define_directive(tokens, pos, end_pos, error_handler)
    context.check_not(error)
    context.check_is_not_none(macro_def)
    assert macro_def is not None
    context.check_equal(macro_def.pos, pos)
    context.check_equal(macro_def.name, expected_name)
    context.check_equal(macro_def.params, expected_params)
    context.check_equal(macro_def.is_variadic, expected_is_variadic)
    _check_replace_elements(context, macro_def.replacement, tuple(expected_replacement))


def _check_bad_define_directive(context: _t.Context, text: str,
                                expected_errors: list[tuple[str, _tp.TextPos]]) -> None:
    errors: list[_Error] = []
    position_tracker = _tp.TextPosTracker()
    path = pathlib.Path("test.cpp")
    file_index = 0
    def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
        assert pos.file_index == file_index
        text_pos = position_tracker.get_text_pos(pos.pos_in_file)
        errors.append(_Error(message % args, text_pos))
        file_context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
        _l.FileContextLogger(context.logger, file_context).error(message, *args)
    tokens, end_pos = _tokenize(path, file_index, text, position_tracker, error_handler)
    context.check_is_empty(errors)
    pos = _cp.Position(file_index, 0)
    macro_def = _cp.parse_define_directive(tokens, pos, end_pos, error_handler)
    context.check_is_none(macro_def)
    _check_errors(context, errors, expected_errors)


def _check_replace_elements(context: _t.Context, elems: tuple[_cp.ReplaceElem, ...],
                            expected_elems: tuple[_cp.ReplaceElem, ...]) -> None:
    for i, (elem, expected) in enumerate(zip(elems, expected_elems)):
        subcontext = context.subcontext(1 + i)
        match elem:
            case _cp.Token() as token:
                subcontext.check_is_instance(expected, _cp.Token)
                assert isinstance(expected, _cp.Token)
                subcontext.check_equal(token.type_, expected.type_)
                subcontext.check_equal(token.text, expected.text)
                subcontext.check_equal(token.pos, expected.pos)
                subcontext.check_equal(token.ucn_map, expected.ucn_map)
                subcontext.check_equal(token.synthetic, expected.synthetic)
                continue
            case _cp.ParamRef():
                subcontext.check_is_instance(expected, _cp.ParamRef)
                assert isinstance(expected, _cp.ParamRef)
                subcontext.check_equal(elem.param_index, expected.param_index)
                subcontext.check_equal(elem.pos, expected.pos)
                continue
            case _cp.VaArgs():
                subcontext.check_is_instance(expected, _cp.VaArgs)
                assert isinstance(expected, _cp.VaArgs)
                subcontext.check_equal(elem.pos, expected.pos)
                continue
            case _cp.VaOpt():
                subcontext.check_is_instance(expected, _cp.VaOpt)
                assert isinstance(expected, _cp.VaOpt)
                for j, (subelem, subexpected) in enumerate(zip(elem.elements, expected.elements)):
                    subsubcontext = subcontext.subcontext(1 + j)
                    subsubcontext.check_equal(subelem, subexpected)
                subcontext.check_equal(elem.pos, expected.pos)
                continue
            case _cp.StringifyOperation():
                subcontext.check_is_instance(expected, _cp.StringifyOperation)
                assert isinstance(expected, _cp.StringifyOperation)
                subcontext.check_equal(elem.operand, expected.operand)
                subcontext.check_equal(elem.pos, expected.pos)
                continue
            case _cp.FuseOperation():
                subcontext.check_is_instance(expected, _cp.FuseOperation)
                assert isinstance(expected, _cp.FuseOperation)
                subcontext.check_equal(elem.left, expected.left)
                for j, (right, expected_right) in enumerate(zip(elem.rights, expected.rights)):
                    subsubcontext = subcontext.subcontext(1 + j)
                    subsubcontext.check_equal(right.operand, expected_right.operand)
                    subsubcontext.check_equal(right.pos, expected_right.pos)
                continue
        typing.assert_never(elem)
    context.check_equal(len(elems), len(expected_elems))


def _check_errors(context: _t.Context, errors: _cabc.Sequence[_Error],
                  expected_errors: _cabc.Sequence[tuple[str, _tp.TextPos]]) -> None:
    for i, (error, expected) in enumerate(zip(errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.pos, expected[1])
    context.check_equal(len(errors), len(expected_errors))


@dataclasses.dataclass(slots=True, frozen=True)
class _Error:
    message: str
    pos:     _tp.TextPos


def _tokenize(path: pathlib.Path, file_index: int, cpp_text: str, position_tracker: _tp.TextPosTracker,
              error_handler: _cp.ErrorHandler) -> tuple[list[_cp.Token], _cp.Position]:
    tokens: list[_cp.Token] = []
    with io.StringIO(cpp_text) as input_:
        tokenizer = _cp.Tokenizer(input_, file_index, position_tracker, error_handler)
        while True:
            token = tokenizer.next_token()
            if token.type_ is _cp.TokenType.END_OF_INPUT:
                return tokens, token.pos
            tokens.append(token)


# Bridge to Python's native testing framework
def load_tests(loader: unittest.TestLoader, standard_tests: unittest.TestSuite,
               pattern: str | None) -> unittest.TestSuite:
    return _t.generate_native_tests(__name__)


if __name__ == '__main__':
    _t.run_module_tests(__name__)
