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
    _check_tokenize(context, 'R"()"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"()"', 1, 0, ()),
    ], 1, 5)
    _check_tokenize(context, 'R"(foo)"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"', 1, 0, ()),
    ], 1, 8)
    _check_tokenize(context, 'R"foo(bar)foo"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"foo(bar)foo"', 1, 0, ()),
    ], 1, 14)
    _check_tokenize(context, 'R""(foo)""', [
        (_cp.TokenType.RAW_STRING_LIT, 'R""(foo)""', 1, 0, ()),
    ], 1, 10)
    _check_tokenize(context, 'R"$(foo)"bar)$"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"$(foo)"bar)$"', 1, 0, ()),
    ], 1, 15)
    _check_tokenize(context, 'R"(a\nb)"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(a\nb)"', 1, 0, ()),
    ], 2, 3)

    # Invalidity
    _check_tokenize(context, 'R"\\()\\"', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"\\()\\"', 1, 0, ()),
    ], 1, 7, [
        ('Invalid character ("\\\\") in raw string literal delimiter', _tp.TextPos(1, 2)),
    ])
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
    _check_tokenize(context, 'R"\\()"', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"\\()"', 1, 0, ()),
    ], 1, 6, [
        ('Invalid character ("\\\\") in raw string literal delimiter', _tp.TextPos(1, 2)),
        ('Unterminated raw string literal', _tp.TextPos(1, 0)),
    ])

    # Line splicing
    _check_tokenize(context, 'R"(a\\\nb)"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(a\\\nb)"', 1, 0, ()),
    ], 2, 3)
    _check_tokenize(context, 'R\\\n"(\\\nfoo\\\n)"\\\nbar', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(\\\nfoo\\\n)"bar', 1, 0, ()),
    ], 5, 3)
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
    _check_tokenize(context, 'R"()\\\n"', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"()\\\n"', 1, 0, ()),
    ], 2, 1, [
        ('Unterminated raw string literal', _tp.TextPos(1, 0)),
    ])
    _check_tokenize(context, 'R"()\\\n"\n)"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"()\\\n"\n)"', 1, 0, ()),
    ], 3, 2)

    # Prefix
    _check_tokenize(context, 'LR"(foo)"', [
        (_cp.TokenType.RAW_STRING_LIT, 'LR"(foo)"', 1, 0, ()),
    ], 1, 9)
    _check_tokenize(context, 'XR"(foo)"', [
        (_cp.TokenType.IDENTIFIER, 'XR', 1, 0, ()),
        (_cp.TokenType.STRING_LIT, '"(foo)"', 1, 2, ()),
    ], 1, 9)
    _check_tokenize(context, 'u8R"()" uR"()" UR"()"', [
        (_cp.TokenType.RAW_STRING_LIT, 'u8R"()"', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 7, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'uR"()"', 1, 8, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 14, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'UR"()"', 1, 15, ()),
    ], 1, 21)

    # Suffix
    _check_tokenize(context, 'R"(foo)"bar', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"bar', 1, 0, ()),
    ], 1, 11)
    _check_tokenize(context, 'LR"(foo)"bar', [
        (_cp.TokenType.RAW_STRING_LIT, 'LR"(foo)"bar', 1, 0, ()),
    ], 1, 12)
    _check_tokenize(context, 'R"(foo)"123', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"', 1, 0, ()),
        (_cp.TokenType.NUMBER, "123", 1, 8, ()),
    ], 1, 11)
    _check_tokenize(context, 'R"(foo)"_ R"(foo)"Z R"(foo)"\u00e9 R"(foo)"\\u00e9', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"_', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 9, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"Z', 1, 10, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 19, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"\u00e9', 1, 20, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 29, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"\u00e9', 1, 30, (_cp.ReplacementMap.Entry(8, 1, "\\u00e9"),)),
    ], 1, 44)
    _check_tokenize(context, 'R"(foo)"\\u00e9_ R"(foo)"\\u00e9Z R"(foo)"\\u00e9\u00e9 R"(foo)"\\u00e9\\u00e9', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"\u00e9_', 1, 0, (_cp.ReplacementMap.Entry(8, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 15, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"\u00e9Z', 1, 16, (_cp.ReplacementMap.Entry(8, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 31, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"\u00e9\u00e9', 1, 32, (_cp.ReplacementMap.Entry(8, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 47, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"\u00e9\u00e9', 1, 48, (_cp.ReplacementMap.Entry(8, 1, "\\u00e9"),
                                                                       _cp.ReplacementMap.Entry(9, 1, "\\u00e9"))),
    ], 1, 68)
    _check_tokenize(context, 'R"(foo)"_\\u00e9 R"(foo)"Z\\u00e9 R"(foo)"\u00e9\\u00e9', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"_\u00e9', 1, 0, (_cp.ReplacementMap.Entry(9, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 15, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"Z\u00e9', 1, 16, (_cp.ReplacementMap.Entry(9, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 31, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"\u00e9\u00e9', 1, 32, (_cp.ReplacementMap.Entry(9, 1, "\\u00e9"),)),
    ], 1, 47)
    _check_tokenize(context, 'R"(foo)"\\u0967Z R"(foo)"Z\\u0967 R"(foo)"Z\\u20AC', [
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"(foo)"\\u0967Z', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 15, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"Z\u0967', 1, 16, (_cp.ReplacementMap.Entry(9, 1, "\\u0967"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 31, ()),
        (_cp.TokenType.BAD_RAW_STRING_LIT, 'R"(foo)"Z\\u20AC', 1, 32, ()),
    ], 1, 47, [
        ('Invalid UCN code point (U+0967): Not in XID_Start', _tp.TextPos(1, 10)),
        ('Invalid UCN code point (U+20AC): Not in XID_Continue', _tp.TextPos(1, 43)),
    ])

    # Transitions
    _check_tokenize(context, 'R"(foo)"Z"bar" R"(foo)"L"bar" R"(foo)"LR"bar"', [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"Z', 1, 0, ()),
        (_cp.TokenType.STRING_LIT, '"bar"', 1, 9, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 14, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"L', 1, 15, ()),
        (_cp.TokenType.STRING_LIT, '"bar"', 1, 24, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 29, ()),
        (_cp.TokenType.RAW_STRING_LIT, 'R"(foo)"LR', 1, 30, ()),
        (_cp.TokenType.STRING_LIT, '"bar"', 1, 40, ()),
    ], 1, 45)


def test_CppPreprocess_Tokenizer_StringLit(context: _t.Context) -> None:
    _check_tokenize(context, '""', [
        (_cp.TokenType.STRING_LIT, '""', 1, 0, ()),
    ], 1, 2)
    _check_tokenize(context, '"foo"', [
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 0, ()),
    ], 1, 5)
    _check_tokenize(context, '"\\"a\\zb\\""', [
        (_cp.TokenType.STRING_LIT, '"\\"a\\zb\\""', 1, 0, ()),
    ], 1, 10)

    # Lack of termination
    _check_tokenize(context, '"foo', [
        (_cp.TokenType.BAD_STRING_LIT, '"foo', 1, 0, ()),
    ], 1, 4, [
        ('Unterminated string literal', _tp.TextPos(1, 0)),
    ])
    _check_tokenize(context, '"foo\\"', [
        (_cp.TokenType.BAD_STRING_LIT, '"foo\\"', 1, 0, ()),
    ], 1, 6, [
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

    # Payload UCNs are not resolved
    _check_tokenize(context, '"\\u00e9"', [
        (_cp.TokenType.STRING_LIT, '"\\u00e9"', 1, 0, ()),
    ], 1, 8)

    # Line splicing inside escape sequence
    _check_tokenize(context, '"\\\\\nn"', [
        (_cp.TokenType.STRING_LIT, '"\\n"', 1, 0, ()),
    ], 2, 2)
    _check_tokenize(context, '"\\\\\n""', [
        (_cp.TokenType.STRING_LIT, '"\\""', 1, 0, ()),
    ], 2, 2)
    _check_tokenize(context, '"\\\\\n"', [
        (_cp.TokenType.BAD_STRING_LIT, '"\\"', 1, 0, ()),
    ], 2, 1, [
        ('Unterminated string literal', _tp.TextPos(1, 0)),
    ])

    # Prefix
    _check_tokenize(context, 'L"foo"', [
        (_cp.TokenType.STRING_LIT, 'L"foo"', 1, 0, ()),
    ], 1, 6)
    _check_tokenize(context, 'X"foo"', [
        (_cp.TokenType.IDENTIFIER, 'X', 1, 0, ()),
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 1, ()),
    ], 1, 6)
    _check_tokenize(context, 'u8"" u"" U""', [
        (_cp.TokenType.STRING_LIT, 'u8""', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 4, ()),
        (_cp.TokenType.STRING_LIT, 'u""', 1, 5, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.STRING_LIT, 'U""', 1, 9, ()),
    ], 1, 12)

    # Suffix
    _check_tokenize(context, '"foo"bar', [
        (_cp.TokenType.STRING_LIT, '"foo"bar', 1, 0, ()),
    ], 1, 8)
    _check_tokenize(context, 'L"foo"bar', [
        (_cp.TokenType.STRING_LIT, 'L"foo"bar', 1, 0, ()),
    ], 1, 9)
    _check_tokenize(context, '"foo"123', [
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 0, ()),
        (_cp.TokenType.NUMBER, "123", 1, 5, ()),
    ], 1, 8)
    _check_tokenize(context, '"foo"_ "foo"Z "foo"\u00e9 "foo"\\u00e9', [
        (_cp.TokenType.STRING_LIT, '"foo"_', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.STRING_LIT, '"foo"Z', 1, 7, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 13, ()),
        (_cp.TokenType.STRING_LIT, '"foo"\u00e9', 1, 14, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 20, ()),
        (_cp.TokenType.STRING_LIT, '"foo"\u00e9', 1, 21, (_cp.ReplacementMap.Entry(5, 1, "\\u00e9"),)),
    ], 1, 32)
    _check_tokenize(context, '"foo"\\u00e9_ "foo"\\u00e9Z "foo"\\u00e9\u00e9 "foo"\\u00e9\\u00e9', [
        (_cp.TokenType.STRING_LIT, '"foo"\u00e9_', 1, 0, (_cp.ReplacementMap.Entry(5, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 12, ()),
        (_cp.TokenType.STRING_LIT, '"foo"\u00e9Z', 1, 13, (_cp.ReplacementMap.Entry(5, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 25, ()),
        (_cp.TokenType.STRING_LIT, '"foo"\u00e9\u00e9', 1, 26, (_cp.ReplacementMap.Entry(5, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 38, ()),
        (_cp.TokenType.STRING_LIT, '"foo"\u00e9\u00e9', 1, 39, (_cp.ReplacementMap.Entry(5, 1, "\\u00e9"),
                                                                _cp.ReplacementMap.Entry(6, 1, "\\u00e9"))),
    ], 1, 56)
    _check_tokenize(context, '"foo"_\\u00e9 "foo"Z\\u00e9 "foo"\u00e9\\u00e9', [
        (_cp.TokenType.STRING_LIT, '"foo"_\u00e9', 1, 0, (_cp.ReplacementMap.Entry(6, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 12, ()),
        (_cp.TokenType.STRING_LIT, '"foo"Z\u00e9', 1, 13, (_cp.ReplacementMap.Entry(6, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 25, ()),
        (_cp.TokenType.STRING_LIT, '"foo"\u00e9\u00e9', 1, 26, (_cp.ReplacementMap.Entry(6, 1, "\\u00e9"),)),
    ], 1, 38)
    _check_tokenize(context, '"foo"\\u0967Z "foo"Z\\u0967 "foo"Z\\u20AC', [
        (_cp.TokenType.BAD_STRING_LIT, '"foo"\\u0967Z', 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 12, ()),
        (_cp.TokenType.STRING_LIT, '"foo"Z\u0967', 1, 13, (_cp.ReplacementMap.Entry(6, 1, "\\u0967"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 25, ()),
        (_cp.TokenType.BAD_STRING_LIT, '"foo"Z\\u20AC', 1, 26, ()),
    ], 1, 38, [
        ('Invalid UCN code point (U+0967): Not in XID_Start', _tp.TextPos(1, 7)),
        ('Invalid UCN code point (U+20AC): Not in XID_Continue', _tp.TextPos(1, 34)),
    ])

    # Transitions
    _check_tokenize(context, '"foo"Z"bar" "foo"L"bar" "foo"LR"bar"', [
        (_cp.TokenType.STRING_LIT, '"foo"Z', 1, 0, ()),
        (_cp.TokenType.STRING_LIT, '"bar"', 1, 6, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 11, ()),
        (_cp.TokenType.STRING_LIT, '"foo"L', 1, 12, ()),
        (_cp.TokenType.STRING_LIT, '"bar"', 1, 18, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 23, ()),
        (_cp.TokenType.STRING_LIT, '"foo"LR', 1, 24, ()),
        (_cp.TokenType.STRING_LIT, '"bar"', 1, 31, ()),
    ], 1, 36)


def test_CppPreprocess_Tokenizer_CharLit(context: _t.Context) -> None:
    _check_tokenize(context, "''", [
        (_cp.TokenType.CHAR_LIT, "''", 1, 0, ()),
    ], 1, 2)
    _check_tokenize(context, "'c'", [
        (_cp.TokenType.CHAR_LIT, "'c'", 1, 0, ()),
    ], 1, 3)
    _check_tokenize(context, "'foo'", [
        (_cp.TokenType.CHAR_LIT, "'foo'", 1, 0, ()),
    ], 1, 5)
    _check_tokenize(context, "'\\'a\\zb\\''", [
        (_cp.TokenType.CHAR_LIT, "'\\'a\\zb\\''", 1, 0, ()),
    ], 1, 10)

    # Lack of termination
    _check_tokenize(context, "'c", [
        (_cp.TokenType.BAD_CHAR_LIT, "'c", 1, 0, ()),
    ], 1, 2, [
        ('Unterminated character literal', _tp.TextPos(1, 0)),
    ])
    _check_tokenize(context, "'\\'", [
        (_cp.TokenType.BAD_CHAR_LIT, "'\\'", 1, 0, ()),
    ], 1, 3, [
        ('Unterminated character literal', _tp.TextPos(1, 0)),
    ])
    _check_tokenize(context, "'\n'", [
        (_cp.TokenType.BAD_CHAR_LIT, "'", 1, 0, ()),
        (_cp.TokenType.NEWLINE, "\n", 1, 1, ()),
        (_cp.TokenType.BAD_CHAR_LIT, "'", 2, 0, ()),
    ], 2, 1, [
        ('Unterminated character literal', _tp.TextPos(1, 0)),
        ('Unterminated character literal', _tp.TextPos(2, 0)),
    ])

    # Payload UCNs are not resolved
    _check_tokenize(context, "'\\u00e9'", [
        (_cp.TokenType.CHAR_LIT, "'\\u00e9'", 1, 0, ()),
    ], 1, 8)

    # Line splicing inside escape sequence
    _check_tokenize(context, "'\\\\\nn'", [
        (_cp.TokenType.CHAR_LIT, "'\\n'", 1, 0, ()),
    ], 2, 2)
    _check_tokenize(context, "'\\\\\n''", [
        (_cp.TokenType.CHAR_LIT, "'\\''", 1, 0, ()),
    ], 2, 2)
    _check_tokenize(context, "'\\\\\n'", [
        (_cp.TokenType.BAD_CHAR_LIT, "'\\'", 1, 0, ()),
    ], 2, 1, [
        ('Unterminated character literal', _tp.TextPos(1, 0)),
    ])

    # Prefix
    _check_tokenize(context, "L'c'", [
        (_cp.TokenType.CHAR_LIT, "L'c'", 1, 0, ()),
    ], 1, 4)
    _check_tokenize(context, "X'c'", [
        (_cp.TokenType.IDENTIFIER, 'X', 1, 0, ()),
        (_cp.TokenType.CHAR_LIT, "'c'", 1, 1, ()),
    ], 1, 4)
    _check_tokenize(context, "u8'' u'' U''", [
        (_cp.TokenType.CHAR_LIT, "u8''", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 4, ()),
        (_cp.TokenType.CHAR_LIT, "u''", 1, 5, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.CHAR_LIT, "U''", 1, 9, ()),
    ], 1, 12)

    # Suffix
    _check_tokenize(context, "'c'foo", [
        (_cp.TokenType.CHAR_LIT, "'c'foo", 1, 0, ()),
    ], 1, 6)
    _check_tokenize(context, "L'c'foo", [
        (_cp.TokenType.CHAR_LIT, "L'c'foo", 1, 0, ()),
    ], 1, 7)
    _check_tokenize(context, "'c'123", [
        (_cp.TokenType.CHAR_LIT, "'c'", 1, 0, ()),
        (_cp.TokenType.NUMBER, "123", 1, 3, ()),
    ], 1, 6)
    _check_tokenize(context, "'c'_ 'c'Z 'c'\u00e9 'c'\\u00e9", [
        (_cp.TokenType.CHAR_LIT, "'c'_", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 4, ()),
        (_cp.TokenType.CHAR_LIT, "'c'Z", 1, 5, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 9, ()),
        (_cp.TokenType.CHAR_LIT, "'c'\u00e9", 1, 10, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 14, ()),
        (_cp.TokenType.CHAR_LIT, "'c'\u00e9", 1, 15, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),)),
    ], 1, 24)
    _check_tokenize(context, "'c'\\u00e9_ 'c'\\u00e9Z 'c'\\u00e9\u00e9 'c'\\u00e9\\u00e9", [
        (_cp.TokenType.CHAR_LIT, "'c'\u00e9_", 1, 0, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 10, ()),
        (_cp.TokenType.CHAR_LIT, "'c'\u00e9Z", 1, 11, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 21, ()),
        (_cp.TokenType.CHAR_LIT, "'c'\u00e9\u00e9", 1, 22, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 32, ()),
        (_cp.TokenType.CHAR_LIT, "'c'\u00e9\u00e9", 1, 33, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),
                                                            _cp.ReplacementMap.Entry(4, 1, "\\u00e9"))),
    ], 1, 48)
    _check_tokenize(context, "'c'_\\u00e9 'c'Z\\u00e9 'c'\u00e9\\u00e9", [
        (_cp.TokenType.CHAR_LIT, "'c'_\u00e9", 1, 0, (_cp.ReplacementMap.Entry(4, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 10, ()),
        (_cp.TokenType.CHAR_LIT, "'c'Z\u00e9", 1, 11, (_cp.ReplacementMap.Entry(4, 1, "\\u00e9"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 21, ()),
        (_cp.TokenType.CHAR_LIT, "'c'\u00e9\u00e9", 1, 22, (_cp.ReplacementMap.Entry(4, 1, "\\u00e9"),)),
    ], 1, 32)
    _check_tokenize(context, "'c'\\u0967Z 'c'Z\\u0967 'c'Z\\u20AC", [
        (_cp.TokenType.BAD_CHAR_LIT, "'c'\\u0967Z", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 10, ()),
        (_cp.TokenType.CHAR_LIT, "'c'Z\u0967", 1, 11, (_cp.ReplacementMap.Entry(4, 1, "\\u0967"),)),
        (_cp.TokenType.WHITESPACE, " ", 1, 21, ()),
        (_cp.TokenType.BAD_CHAR_LIT, "'c'Z\\u20AC", 1, 22, ()),
    ], 1, 32, [
        ('Invalid UCN code point (U+0967): Not in XID_Start', _tp.TextPos(1, 5)),
        ('Invalid UCN code point (U+20AC): Not in XID_Continue', _tp.TextPos(1, 28)),
    ])

    # Transitions
    _check_tokenize(context, "'c'Z\"foo\" 'c'L\"foo\" 'c'LR\"foo\"", [
        (_cp.TokenType.CHAR_LIT, "'c'Z", 1, 0, ()),
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 4, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 9, ()),
        (_cp.TokenType.CHAR_LIT, "'c'L", 1, 10, ()),
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 14, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 19, ()),
        (_cp.TokenType.CHAR_LIT, "'c'LR", 1, 20, ()),
        (_cp.TokenType.STRING_LIT, '"foo"', 1, 25, ()),
    ], 1, 30)


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
    _check_tokenize(context, "123\\u00e9456", [
        (_cp.TokenType.NUMBER, "123\u00e9456", 1, 0, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),)),
    ], 1, 12)
    _check_tokenize(context, "123\\u00e9\\u0967456", [
        (_cp.TokenType.NUMBER, "123\u00e9\u0967456", 1, 0, (_cp.ReplacementMap.Entry(3, 1, "\\u00e9"),
                                                            _cp.ReplacementMap.Entry(4, 1, "\\u0967"))),
    ], 1, 18)

    # Underscores
    _check_tokenize(context, "123_", [
        (_cp.TokenType.NUMBER, "123_", 1, 0, ()),
    ], 1, 4)
    _check_tokenize(context, "123_456", [
        (_cp.TokenType.NUMBER, "123_456", 1, 0, ()),
    ], 1, 7)
    _check_tokenize(context, "123__456", [
        (_cp.TokenType.NUMBER, "123__456", 1, 0, ()),
    ], 1, 8)

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

    # Invalid UCNs
    _check_tokenize(context, "123\\u20AC 456\\u20AC789", [
        (_cp.TokenType.BAD_NUMBER, "123\\u20AC", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 9, ()),
        (_cp.TokenType.BAD_NUMBER, "456\\u20AC789", 1, 10, ()),
    ], 1, 22, [
        ('Invalid UCN code point (U+20AC): Not in XID_Continue', _tp.TextPos(1, 5)),
        ('Invalid UCN code point (U+20AC): Not in XID_Continue', _tp.TextPos(1, 15)),
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
        ('Invalid UCN code point (U+0967): Not in XID_Start', _tp.TextPos(1, 6)),
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

    # Line splicing
    _check_tokenize(context, "+\\\n+", [
        (_cp.TokenType.PUNCT, "++", 1, 0, ()),
    ], 2, 1)
    _check_tokenize(context, "<\\\n::", [
        (_cp.TokenType.PUNCT, "<", 1, 0, ()),
        (_cp.TokenType.PUNCT, "::", 2, 0, ()),
    ], 2, 2)


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
        ('Invalid UCN code point (U+0041): In basic character set', _tp.TextPos(1, 2)),
    ])
    _check_tokenize(context, "\\u20AC", [
        (_cp.TokenType.BAD_IDENTIFIER, "\\u20AC", 1, 0, ()),
    ], 1, 6, [
        ('Invalid UCN code point (U+20AC): Not in XID_Start', _tp.TextPos(1, 2)),
    ])
    _check_tokenize(context, "\\uD800", [
        (_cp.TokenType.BAD_IDENTIFIER, "\\uD800", 1, 0, ()),
    ], 1, 6, [
        ('Invalid UCN code point (U+D800): In surrogate range', _tp.TextPos(1, 2)),
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

    # Allow space
    _check_tokenize(context, "#include <foo / bar.h>", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "include", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.HEADER_NAME, "<foo / bar.h>", 1, 9, ()),
    ], 1, 22)
    _check_tokenize(context, "#embed <foo bar.bin>", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "embed", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.HEADER_NAME, "<foo bar.bin>", 1, 7, ()),
    ], 1, 20)

    # No escaping
    _check_tokenize(context, '#include <"foo\\".h>', [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "include", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.HEADER_NAME, '<"foo\\".h>', 1, 9, ()),
    ], 1, 19)
    _check_tokenize(context, '#embed "foo\\.bin\\"', [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "embed", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.HEADER_NAME, '"foo\\.bin\\"', 1, 7, ()),
    ], 1, 18)

    # Lack of termination
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

    # Comments allowed
    _check_tokenize(context, "#include /* comment */ <foo.h>", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "include", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.BLOCK_COMMENT, "/* comment */", 1, 9, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 22, ()),
        (_cp.TokenType.HEADER_NAME, "<foo.h>", 1, 23, ()),
    ], 1, 30)

    # Allow for `<` or `"` to not be found
    _check_tokenize(context, "#include MACRO_NAME", [
        (_cp.TokenType.HASH, "#", 1, 0, ()),
        (_cp.TokenType.IDENTIFIER, "include", 1, 1, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 8, ()),
        (_cp.TokenType.IDENTIFIER, "MACRO_NAME", 1, 9, ()),
    ], 1, 19)
    _check_tokenize(context, "import std;", [
        (_cp.TokenType.IDENTIFIER, "import", 1, 0, ()),
        (_cp.TokenType.WHITESPACE, " ", 1, 6, ()),
        (_cp.TokenType.IDENTIFIER, "std", 1, 7, ()),
        (_cp.TokenType.PUNCT, ";", 1, 10, ()),
    ], 1, 11)


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
        ("`__VA_ARGS__` must only appear in body of a variadic macro definition", _tp.TextPos(1, 2)),
    ])
    _check_bad_define_directive(context, "M(x) __VA_ARGS__", [
        ("`__VA_ARGS__` must only appear in body of a variadic macro definition", _tp.TextPos(1, 5)),
    ])
    _check_valid_define_directive(context, "M(...) __VA_ARGS__", file_index, "M", (), True, [
        _cp.VaArgs(_cp.Position(file_index, 7)),
    ])
    _check_bad_define_directive(context, "__VA_ARGS__ foo", [
        ("`__VA_ARGS__` must only appear in body of a variadic macro definition", _tp.TextPos(1, 0)),
    ])
    _check_bad_define_directive(context, "M(__VA_ARGS__) foo", [
        ('`__VA_ARGS__` must only appear in body of a variadic macro definition', _tp.TextPos(1, 2)),
    ])

    # `__VA_OPT__`
    _check_bad_define_directive(context, "M __VA_OPT__", [
        ("`__VA_OPT__` must only appear in body of a variadic macro definition", _tp.TextPos(1, 2)),
    ])
    _check_bad_define_directive(context, "M(x) __VA_OPT__", [
        ("`__VA_OPT__` must only appear in body of a variadic macro definition", _tp.TextPos(1, 5)),
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
        ("`__VA_OPT__` is not allowed to occur inside `__VA_OPT__(...)`", _tp.TextPos(1, 18)),
    ])
    _check_bad_define_directive(context, "__VA_OPT__ foo", [
        ("`__VA_OPT__` must only appear in body of a variadic macro definition", _tp.TextPos(1, 0)),
    ])
    _check_bad_define_directive(context, "M(__VA_OPT__) foo", [
        ("`__VA_OPT__` must only appear in body of a variadic macro definition", _tp.TextPos(1, 2)),
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
        ('`__VA_ARGS__` must only appear in body of a variadic macro definition', _tp.TextPos(1, 6)),
    ])
    _check_valid_define_directive(context, "M(...) #__VA_ARGS__", file_index, "M", (), True, [
        _cp.StringifyOperation(_cp.VaArgs(_cp.Position(file_index, 8)), _cp.Position(file_index, 7)),
    ])
    _check_bad_define_directive(context, "M(x) #__VA_OPT__()", [
        ('`__VA_OPT__` must only appear in body of a variadic macro definition', _tp.TextPos(1, 6)),
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


def test_CppPreprocess_UnpackStringLiteral(context: _t.Context) -> None:
    encoding_info       = _cp.EncodingInfo(plain="utf-8", wide=_cp.WideCharacterEncoding.UTF_32)
    encoding_info_utf16 = _cp.EncodingInfo(plain="utf-8", wide=_cp.WideCharacterEncoding.UTF_16)

    # Prefix
    _check_valid_string_literal(context, '""', "", _cp.StringLiteralType.PLAIN, "")
    _check_valid_string_literal(context, '"foo"', "foo", _cp.StringLiteralType.PLAIN, "")
    _check_valid_string_literal(context, 'L"foo"', "foo", _cp.StringLiteralType.WIDE, "")
    _check_valid_string_literal(context, 'u8"foo"', "foo", _cp.StringLiteralType.UTF_8, "")
    _check_valid_string_literal(context, 'u"foo"', "foo", _cp.StringLiteralType.UTF_16, "")
    _check_valid_string_literal(context, 'U"foo"', "foo", _cp.StringLiteralType.UTF_32, "")

    # Suffix
    _check_valid_string_literal(context, '"foo"_suffix', "foo", _cp.StringLiteralType.PLAIN, "_suffix")
    _check_valid_string_literal(context, 'u8"foo"bar', "foo", _cp.StringLiteralType.UTF_8, "bar")

    # Raw
    _check_valid_string_literal(context, 'R"(foo)"', "foo", _cp.StringLiteralType.PLAIN, "")
    _check_valid_string_literal(context, 'LR"*(foo)*"', "foo", _cp.StringLiteralType.WIDE, "")
    _check_valid_string_literal(context, 'u8R"(foo)"', "foo", _cp.StringLiteralType.UTF_8, "")
    _check_valid_string_literal(context, 'uR"(foo)"', "foo", _cp.StringLiteralType.UTF_16, "")
    _check_valid_string_literal(context, 'UR"*(foo)*"', "foo", _cp.StringLiteralType.UTF_32, "")
    _check_valid_string_literal(context, 'u8R"(a\nb)"', "a\nb", _cp.StringLiteralType.UTF_8, "")
    _check_valid_string_literal(context, 'R"(\\u00e9)"', "\\u00e9", _cp.StringLiteralType.PLAIN, "")

    # Simple escapes
    _check_valid_string_literal(context, '"\\n\\t\\\\\\"\\\'"', "\n\t\\\"'", _cp.StringLiteralType.PLAIN, "")

    # Numeric escapes
    _check_valid_string_literal(context, '"\\x41"', "A", _cp.StringLiteralType.PLAIN, "", encoding_info)
    _check_valid_string_literal(context, '"\\x{41}"', "A", _cp.StringLiteralType.PLAIN, "", encoding_info)
    _check_valid_string_literal(context, '"\\101"', "A", _cp.StringLiteralType.PLAIN, "", encoding_info)
    _check_valid_string_literal(context, '"\\o{101}"', "A", _cp.StringLiteralType.PLAIN, "", encoding_info)
    _check_valid_string_literal(context, '"\\xE2\\x82\\xAC"', "\u20AC", _cp.StringLiteralType.PLAIN, "", encoding_info)
    _check_valid_string_literal(context, 'u8"\\xE2\\x82\\xAC"', "\u20AC", _cp.StringLiteralType.UTF_8, "",
                                encoding_info)
    _check_valid_string_literal(context, 'L"\\xD83D\\xDE00"', "\U0001F600", _cp.StringLiteralType.WIDE, "",
                                encoding_info_utf16)
    _check_valid_string_literal(context, 'u"\\xD83D\\xDE00"', "\U0001F600", _cp.StringLiteralType.UTF_16, "")
    _check_valid_string_literal(context, 'L"\\x1F600"', "\U0001F600", _cp.StringLiteralType.WIDE, "", encoding_info)
    _check_valid_string_literal(context, 'U"\\x1F600"', "\U0001F600", _cp.StringLiteralType.UTF_32, "")
    _check_numeric_escape_error(context, '"\\x41"')
    _check_numeric_escape_error(context, 'L"\\101"')

    # UCNs
    _check_valid_string_literal(context, '"\\u00E9"', "\u00E9", _cp.StringLiteralType.PLAIN, "")
    _check_valid_string_literal(context, '"\\U000000E9"', "\u00E9", _cp.StringLiteralType.PLAIN, "")
    _check_valid_string_literal(context, '"\\u{E9}"', "\u00E9", _cp.StringLiteralType.PLAIN, "")
    _check_valid_string_literal(context, '"\\N{LATIN SMALL LETTER E WITH ACUTE}"', "\u00E9",
                                _cp.StringLiteralType.PLAIN, "")

    # Invalid numeric escapes
    _check_invalid_string_literal(context, '"foo\\o"', [
        ('Invalid escape sequence in string literal: Need opening brace (`{`) after `\\o`', 6)
    ], encoding_info)
    _check_invalid_string_literal(context, '"z\\o$"', [
        ('Invalid escape sequence in string literal: Need opening brace (`{`) after `\\o`', 4)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\o{3"', [
        ('Invalid delimited octal escape sequence in string literal: Missing closing brace (`}`)', 1)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\o{3z}"', [
        ('Invalid delimited octal escape sequence in string literal: Invalid digit ("z")', 5)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\o{}"', [
        ('Invalid delimited octal escape sequence in string literal: No digits', 1)
    ], encoding_info)
    _check_invalid_string_literal(context, '"foo\\x"', [
        ('Invalid escape sequence in string literal: Need hexadecimal digit or opening brace (`{`) after `\\x`', 6)
    ], encoding_info)
    _check_invalid_string_literal(context, '"z\\x%"', [
        ('Invalid escape sequence in string literal: Need hexadecimal digit or opening brace (`{`) after `\\x`', 4)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\x{C"', [
        ('Invalid delimited hexadecimal escape sequence in string literal: Missing closing brace (`}`)', 1)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\x{C2z7}"', [
        ('Invalid delimited hexadecimal escape sequence in string literal: Invalid digit ("z")', 6)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\x{}"', [
        ('Invalid delimited hexadecimal escape sequence in string literal: No digits', 1)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\407"', [
        ('Invalid numeric escape sequence in string literal: Byte-sized code unit (0x107) out of range', 1)
    ], encoding_info)
    _check_invalid_string_literal(context, '"foo\\x41\\xC1\\x42zar"', [
        ('Invalid string literal: Failed to decode byte sequence formed from numeric escape sequences', 8)
    ], encoding_info)
    _check_invalid_string_literal(context, 'u"foo\\x10000"', [
        ("Invalid numeric escape sequence in string literal: UTF-16 code unit (U+10000) out of range", 5)
    ])
    _check_invalid_string_literal(context, 'u"foo\\xDC00"', [
        ("Invalid string literal: Invalid UTF-16 low surrogate (U+DC00) without preceding high surrogate", 5)
    ])
    _check_invalid_string_literal(context, 'u"\\xD800\\x41"', [
        ("Invalid string literal: Invalid low surrogate after high surrogate (U+D800) in UTF-16 surrogate pair", 8)
    ])
    _check_invalid_string_literal(context, 'u"foo\\xD800"', [
        ("Invalid string literal: Missing low surrogate after high surrogate (U+D800) in UTF-16 surrogate pair", 5)
    ])
    _check_invalid_string_literal(context, 'U"foo\\xD800"', [
        ("Invalid numeric escape sequence in string literal: UTF-32 code unit (U+D800) in UTF-16 surrogate range", 5)
    ])
    _check_invalid_string_literal(context, 'U"foo\\x110000"', [
        ("Invalid numeric escape sequence in string literal: UTF-32 code unit (U+110000) outside Unicode range", 5)
    ])

    # Invalid UCNs
    _check_invalid_string_literal(context, '"foo\\u"', [
        ('Invalid UCN in string literal: Need hexadecimal digit or opening brace (`{`) after `\\u`', 6)
    ], encoding_info)
    _check_invalid_string_literal(context, '"z\\u$"', [
        ('Invalid UCN in string literal: Need hexadecimal digit or opening brace (`{`) after `\\u`', 4)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\u00A"', [
        ('Invalid 4-digit UCN in string literal: Too few digits', 1)
    ])
    _check_invalid_string_literal(context, '"foo\\u00zA"', [
        ('Invalid 4-digit UCN in string literal: Invalid hexadecimal digit ("z")', 8)
    ])
    _check_invalid_string_literal(context, '"foo\\u{3"', [
        ('Invalid delimited numeric UCN in string literal: Missing closing brace (`}`)', 4)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\u{12G}"', [
        ('Invalid delimited numeric UCN in string literal: Invalid hexadecimal digit ("G")', 6)
    ])
    _check_invalid_string_literal(context, '"z\\u{}"', [
        ('Invalid delimited numeric UCN in string literal: No digits', 2)
    ])
    _check_invalid_string_literal(context, '"z\\U000000A"', [
        ('Invalid 8-digit UCN in string literal: Too few digits', 2)
    ])
    _check_invalid_string_literal(context, '"\\U000000zA"', [
        ('Invalid 8-digit UCN in string literal: Invalid hexadecimal digit ("z")', 9)
    ])
    _check_invalid_string_literal(context, '"foo\\N"', [
        ('Invalid named UCN in string literal: Need opening brace (`{`) after `\\N`', 6)
    ], encoding_info)
    _check_invalid_string_literal(context, '"zz\\N%"', [
        ('Invalid named UCN in string literal: Need opening brace (`{`) after `\\N`', 5)
    ], encoding_info)
    _check_invalid_string_literal(context, '"\\N{foo"', [
        ('Invalid named UCN in string literal: Missing closing brace (`}`)', 1)
    ])
    _check_invalid_string_literal(context, '"z\\N{INVALID NAME}foo"', [
        ('Invalid named UCN in string literal: Invalid character name: INVALID NAME', 5)
    ])
    _check_invalid_string_literal(context, '"\\uD800"', [
        ('Invalid UCN in string literal: Code point (U+D800) in UTF-16 surrogate range', 1)
    ])
    _check_invalid_string_literal(context, '"\\u{110000}"', [
        ('Invalid UCN in string literal: Code point (U+110000) outside Unicode range', 1)
    ])

    # Other invalidity
    _check_invalid_string_literal(context, '"foo\\Z{bar}"', [
        ('Invalid escape sequence in string literal: Invalid character ("Z") after backslash (`\\`)', 5)
    ], encoding_info)








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
        _check_ucn_map(subcontext, token.ucn_map, expected_ucn_map)
        subcontext.check_not(token.synthetic)
    context.check_equal(len(tokens), len(expected_tokens))


def _check_ucn_map(context: _t.Context, ucn_map: _cp.ReplacementMap | None,
                   expected_ucn_map: _cp.ReplacementMap | None) -> None:
    context.check_equal(ucn_map is None, expected_ucn_map is None)
    if ucn_map:
        assert expected_ucn_map
        for i, (entry, expected) in enumerate(zip(ucn_map.entries, expected_ucn_map.entries)):
            subcontext = context.subcontext(1 + i)
            subcontext.check_equal(entry.pos, expected.pos)
            subcontext.check_equal(entry.size, expected.size)
            subcontext.check_equal(entry.orig, expected.orig)
        context.check_equal(len(ucn_map.entries), len(expected_ucn_map.entries))


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
    directive = _cp.parse_define_directive(tokens, pos, end_pos, error_handler)
    context.check_not(error)
    context.check_is_not_none(directive)
    assert directive is not None
    context.check_equal(directive.pos, pos)
    signature = directive.signature
    context.check_equal(signature.name, expected_name)
    context.check_equal(signature.params, expected_params)
    context.check_equal(signature.is_variadic, expected_is_variadic)
    _check_replace_elements(context, directive.replacement, tuple(expected_replacement))


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
    directive = _cp.parse_define_directive(tokens, pos, end_pos, error_handler)
    context.check_is_none(directive)
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


def _check_valid_string_literal(context: _t.Context, text: str, expected_payload: str,
                                expected_type: _cp.StringLiteralType, expected_suffix: str,
                                encoding_info: _cp.EncodingInfo | None = None) -> None:
    error = False
    path = pathlib.Path("test.cpp")
    file_index = 0
    def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
        nonlocal error
        context.check_equal(pos.file_index, file_index)
        text_pos = _tp.TextPos(line_no=1, pos_on_line=pos.pos_in_file)
        file_context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
        _l.FileContextLogger(context.logger, file_context).error(message, *args)
        error = True

    pos = _cp.Position(file_index, 0)
    synthetic = False
    result = _cp.unpack_string_literal(text, pos, synthetic, error_handler, encoding_info)

    context.check_not(error)
    context.check_is_not_none(result)
    assert result is not None
    context.check_equal(result.payload, expected_payload)
    context.check_equal(result.literal_type, expected_type)
    context.check_equal(result.suffix, expected_suffix)


def _check_numeric_escape_error(context: _t.Context, text: str, encoding_info: _cp.EncodingInfo | None = None) -> None:
    def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
        pass

    pos = _cp.Position(0, 0)
    synthetic = False
    with context.check_raises(_cp.NumericEscapeError):
        _cp.unpack_string_literal(text, pos, synthetic, error_handler)


def _check_invalid_string_literal(context: _t.Context, text: str, expected_errors: list[tuple[str, int]],
                                  encoding_info: _cp.EncodingInfo | None = None) -> None:
    errors: list[_Error] = []
    path = pathlib.Path("test.cpp")
    file_index = 0
    def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
        context.check_equal(pos.file_index, file_index)
        text_pos = _tp.TextPos(line_no=1, pos_on_line=pos.pos_in_file)
        file_context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
        _l.FileContextLogger(context.logger, file_context).error(message, *args)
        errors.append(_Error(message % args, text_pos))

    pos = _cp.Position(file_index, 0)
    synthetic = False
    result = _cp.unpack_string_literal(text, pos, synthetic, error_handler, encoding_info)

    context.check_is_none(result)
    for i, (error, (expected_message, expected_offset)) in enumerate(zip(errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected_message)
        subcontext.check_equal(error.pos.pos_on_line, expected_offset)
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
