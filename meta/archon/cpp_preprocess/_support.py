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

import dataclasses
import enum

import archon.cpp_preprocess._tokenize as _ct


# Unpack regular or raw string literal.
#
# The specified literal must contain one of the valid regular prefixes (`L`, `u8`, `u`,
# `U`), one of the valid raw prefixes (`R`, `LR`, `u8R`, `uR`, `UR`), or no prefix. Also,
# the suffix must contain no quotation marks (`"`). Finally, there must be no stray
# backslash character before the closing quotation mark. A string literal, regular or raw,
# produced by the tokenizer (`_ct.Tokenizer`) satisfies all these criteria.
#
# If an error occurs during unpacking, which will be due to an invalid escape sequence, this
# function generates an error message and returns `None`. If the specified literal contains
# a valid numeric escape sequence that is not a UCN and the target character encoding is not
# known, this function raises `NumericEscapeError`. The target character encoding is known
# if the prefix is `u8`, `u`, `U`, `u8R`, `uR`, or `UR`. It is also known if `encoding_info`
# is not `None`. If neither is the case, the target encoding is not known. The target
# character encoding never needs to be known to decode raw string literals, because they do
# not contain escape sequences.
#
# When the target encoding is known, sequences of immediately adjacent numeric escapes are
# decoded together. If there is a sequence that is not congruent with the encoding of one or
# more code points (characters), this function generates an error message and returns
# `None`. Note that C++26 allows for numeric escapes to produce invalid code units or
# invalid sequences of code units. This function does not.
#
def unpack_string_literal(text: str, pos: _ct.Position, synthetic: bool, error_handler: _ct.ErrorHandler,
                          encoding_info: EncodingInfo | None = None) -> UnpackedStringLiteral | None:
    return _unpack_string_literal(text, pos, synthetic, error_handler, encoding_info)


@dataclasses.dataclass(slots=True, frozen=True)
class UnpackedStringLiteral:
    payload: str  # Unpacked main payload of the literal
    target_encoding: StringLiteralTargetEncoding
    suffix: str  # That which follows the closing quotation mark


# Target encoding is determined from string literal prefix.
#
class StringLiteralTargetEncoding(enum.Enum):
    PLAIN  = enum.auto()  # No prefix or `R`
    WIDE   = enum.auto()  # `L` or `LR`
    UTF_8  = enum.auto()  # `u8` or `u8R`
    UTF_16 = enum.auto()  # `u` or `uR`
    UTF_32 = enum.auto()  # `U` or `UR`


# Currently supported wide character encodings.
#
class WideCharacterEncoding(enum.Enum):
    UTF_16 = enum.auto()
    UTF_32 = enum.auto()


# `plain` is the name of the execution character set encoding used for plain string
# literals. It must be a valid byte-based encoding from the point of view of
# `bytes.decode()`.
#
# `wide` is the name of the execution character set encoding used for wide string literals.
#
@dataclasses.dataclass(slots=True, frozen=True)
class EncodingInfo:
    plain: str = "utf-16"
    wide:  WideCharacterEncoding = WideCharacterEncoding.UTF_32


class NumericEscapeError(Exception):
    pass








def _unpack_string_literal(text: str, pos: _ct.Position, synthetic: bool, error_handler: _ct.ErrorHandler,
                           encoding_info: EncodingInfo | None) -> UnpackedStringLiteral | None:
    raise NotImplementedError                
