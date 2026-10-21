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
# Behavior is undefined if the passed literal does not contain an opening quotation mark
# (`"`) and a separate closing quotation mark such that all of the following conditions are
# satisfied:
#
# - The prefix, which is what precedes the opening quotation mark, must be ``, `L`, `u8`,
#   `u`, `U`, `R`, `LR`, `u8R`, `uR`, or `UR`; and
#
# - the suffix, which is what follows after the closing quotation mark, must contain no
#   quotation marks; and
#
# - in the case of a regular string literal, the bulk, which is the part between the opening
#   and the closing quotation marks, must not contain an odd number of trailing backslashes
#   (`\`); and
#
# - in the case of a raw string literal, there must be a delimiter, `<delim>`, that does not
#   contain the opening parenthesis (`(`) and also does not contain the closing parenthesis
#   (`)`), and such that the bulk begins with `<delim>(` and ends with `)<delim>`.
#
# A string literal, regular or raw, produced by the tokenizer (`_ct.Tokenizer`) satisfies
# all these criteria. Note that this guarantee does not extend to invalid string literals
# produced by the tokenizer such as `BAD_STRING_LIT`.
#
# If an error occurs during unpacking, which will be due to an invalid escape sequence, this
# function generates an error message and returns `None`. If the specified literal contains
# a valid numeric escape sequence that is not a UCN and the target character encoding is not
# known, this function raises `NumericEscapeError`. The target character encoding is known
# if the prefix is `u8`, `u`, `U`, `u8R`, `uR`, or `UR`. It is also known if `encoding_info`
# is not `None`. If neither is the case, the target encoding is not known. The target
# character encoding never needs to be known to unpack raw string literals, because they do
# not contain escape sequences of any kind.
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
    payload: str  # Unpacked main payload of the string literal
    encoding: StringLiteralEncoding
    suffix: str  # That which follows the closing quotation mark


# Target encoding for string literals. It is determined entirely from the prefix of the
# literal.
#
class StringLiteralEncoding(enum.Enum):
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
# `wide` is the execution character set encoding used for wide string literals.
#
@dataclasses.dataclass(slots=True, frozen=True)
class EncodingInfo:
    plain: str = "utf-16"
    wide: WideCharacterEncoding = WideCharacterEncoding.UTF_32


class NumericEscapeError(Exception):
    pass








def _unpack_string_literal(text: str, pos: _ct.Position, synthetic: bool, error_handler: _ct.ErrorHandler,
                           encoding_info: EncodingInfo | None) -> UnpackedStringLiteral | None:
    i = text.find('"')
    assert i >= 0
    prefix = text[:i]
    is_raw = False
    if prefix and prefix[-1] == "R":
        is_raw = True
        prefix = prefix[:-1]
    encoding = _STRING_LITERAL_ENCODING_MAP[prefix]
    j = text.rfind('"')
    assert j > i
    suffix = text[j+1:]
    begin = i + 1
    end = j

    if is_raw:
        i = text.find("(", begin, end)
        assert i >= 0
        delim = text[begin:i]
        j = end - len(delim) - 1
        assert j > i
        assert text[j+1:end] == delim
        payload = text[i+1:j]
        return UnpackedStringLiteral(payload, encoding, suffix)

    parts: list[str] = []
    def add_part(text: str) -> bool:
        if flush_numerics():
            parts.append(text[i:j])
            return True
        return False

    def add_numeric(value: int, offset: int) -> bool:
        raise NotImplementedError                

    def flush_numerics() -> bool:
        raise NotImplementedError                

    i = begin
    while True:
        j = text.find("\\", i, end)
        if j < 0:
            if not add_part(text[i:]):
                return None
            break
        if j > i and not add_part(text[i:j]):
            return None
        j += 1
        assert j < end  # Caller promises no final stray backslash
        ch = text[j]
        j += 1
        result = _SIMPLE_ESCAPES.get(ch)
        if result is not None:
            parts.append(result)
            i = j
            continue
        if ch in _OCT_DIGIT_CHARS:
            k = j
            j -= 1
            while k < end and k - j < 3 and text[k] in _OCT_DIGIT_CHARS:
                k += 1
            value = int(text[j:k], 8)
            if not add_numeric(value, j):
                return None
            i = k
            continue
        if ch == "o":
            if j == end or text[j] != "{":
                error_handler(pos.shift(j), "Missing opening brace (`{`) in delimited octal escape sequence")
                return None
            j += 1
            k = j
            while k < end and k - j < 3 and text[k] in _OCT_DIGIT_CHARS:
                                    
            ch = text[j]
            j += 1
            if ch != "{"
        raise NotImplementedError                
    payload = "".join(parts)
    return UnpackedStringLiteral(payload, encoding, suffix)


_STRING_LITERAL_ENCODING_MAP = {
    "":   StringLiteralEncoding.PLAIN,
    "L":  StringLiteralEncoding.WIDE,
    "u8": StringLiteralEncoding.UTF_8,
    "u":  StringLiteralEncoding.UTF_16,
    "U":  StringLiteralEncoding.UTF_32,
}


_SIMPLE_ESCAPES = {
    "'": "'",
    '"': '"',
    "?": "?",
    "\\": "\\",
    "a": "\a",
    "b": "\b",
    "f": "\f",
    "n": "\n",
    "r": "\r",
    "t": "\t",
    "v": "\v",
}

_HEXADEC_DIGIT_CHARS = frozenset("0123456789ABCDEFabcdef")

_OCT_DIGIT_CHARS = frozenset("01234567")
