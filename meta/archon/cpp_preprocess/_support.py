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
import enum

import archon.base as _b
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
    literal_type: StringLiteralType
    suffix: str  # That which follows the closing quotation mark


# Type of string literal determined by prefix.
#
class StringLiteralType(enum.Enum):
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
    plain: str = "utf-8"
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
    literal_type = _STRING_LITERAL_TYPE_MAP[prefix]
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
        return UnpackedStringLiteral(payload, literal_type, suffix)

    encoding: str | WideCharacterEncoding | None = None
    match literal_type:
        case StringLiteralType.PLAIN:
            if encoding_info is not None:
                encoding = encoding_info.plain
        case StringLiteralType.WIDE:
            if encoding_info is not None:
                encoding = encoding_info.wide
        case StringLiteralType.UTF_8:
            encoding = "utf-8"
        case StringLiteralType.UTF_16:
            encoding = WideCharacterEncoding.UTF_16
        case StringLiteralType.UTF_32:
            encoding = WideCharacterEncoding.UTF_32
        case _:
            typing.assert_never(literal_type)

    def get_subpos(offset: int) -> _ct.Position:
        return pos if synthetic else pos.shift(offset)

    parts: list[str] = []
    def add_part(text: str, prior_offset: int) -> bool:
        if flush_numerics(prior_offset):
            parts.append(text[i:j])
            return True
        return False

    @dataclasses.dataclass(slots=True, frozen=True)
    class NumericEntry:
        value: int
        offset: int
    numeric_entries: list[NumericEntry] = []
    def flush_numerics(end_offset: int) -> bool:
        if not numeric_entries:
            return True
        match encoding:
            case str():
                for entry in numeric_entries:
                    code_unit = entry.value
                    if code_unit > 0xFF:
                        subpos = get_subpos(entry.offset)
                        error_handler(subpos, "Invalid numeric escape sequence: Byte-sized code unit (%s) out of "
                                      "range", code_unit)
                        return False
                b = bytes([e.value for e in numeric_entries])
                try:
                    string = b.decode(encoding, errors="strict")
                except UnicodeDecodeError as e:
                    i = e.start
                    n = len(numeric_entries)
                    if 0 <= i < n:
                        offset = numeric_entries[i].offset
                    elif i == n:
                        offset = end_offset
                    else:
                        offset = numeric_entries[0].offset
                    subpos = get_subpos(offset)
                    error_handler(subpos, "Failed to decode byte sequence formed from numeric escape sequences")
                    return False
                parts.append(string)
            case WideCharacterEncoding.UTF_16:
                i = 0
                n = len(numeric_entries)
                while i < n:
                    entry = numeric_entries[i]
                    i += 1
                    code_unit = entry.value
                    if 0xD800 <= code_unit < 0xE000:
                        if code_unit < 0xDC00:  # High surrogate
                            if i == n:
                                subpos = get_subpos(end_offset)
                                error_handler(subpos, "Missing low surrogate after high surrogate (U+%04X) in UTF-16 "
                                              "surrogate pair", code_unit)
                                return False
                            entry_2 = numeric_entries[i]
                            i += 1
                            code_unit_2 = entry_2.value
                            if not 0xDC00 <= code_unit_2 < 0xE000:
                                subpos = get_subpos(entry_2.offset)
                                error_handler(subpos, "Invalid low surrogate after high surrogate (U+%04X) in UTF-16 "
                                              "surrogate pair", code_unit)
                                return False
                            code_point = 0x10000 + ((code_unit - 0xD800) << 10) + (code_unit_2 - 0xDC00)
                        else:
                            subpos = get_subpos(entry.offset)
                            error_handler(subpos, "Invalid UTF-16 low surrogate (U+%04X) without preceding high "
                                          "surrogate", code_unit)
                            return False
                    elif code_unit <= 0xFFFF:
                        code_point = code_unit
                    else:
                        subpos = get_subpos(entry.offset)
                        error_handler(subpos, "Invalid numeric escape sequence: UTF-16 code unit (%s) out of range",
                                      code_unit)
                        return False
                    ch = chr(code_point)
                    parts.append(ch)
            case WideCharacterEncoding.UTF_32:
                for entry in numeric_entries:
                    code_unit = entry.value
                    if 0xD800 <= code_unit < 0xE000:
                        subpos = get_subpos(entry.offset)
                        error_handler(subpos, "Invalid numeric escape sequence: UTF-32 code unit (U+%04X) in "
                                      "surrogate range", code_unit)
                        return False
                    if code_unit >= 0x110000:
                        subpos = get_subpos(entry.offset)
                        error_handler(subpos, "Invalid numeric escape sequence: UTF-32 code unit (U+%04X) outside "
                                      "Unicode range", code_unit)
                        return False
                    code_point = code_unit
                    ch = chr(code_point)
                    parts.append(ch)
            case None:
                subpos = get_subpos(numeric_entries[0].offset)
                error_handler(subpos, "Numeric escape sequence not allowed because target encoding is unknown")
                return False
            case _:
                typing.assert_never(encoding)
        numeric_entries.clear()
        return True

    i = begin
    while True:
        j = text.find("\\", i, end)
        if j < 0:
            if not add_part(text[i:], i):
                return None
            break
        if j > i:
            if not add_part(text[i:j], i):
                return None
        i = j
        j += 1
        assert j < end  # Caller promises no final stray backslash
        ch = text[j]
        j += 1
        result = _SIMPLE_ESCAPES.get(ch)
        if result is not None:
            parts.append(result)
            i = j
            continue
        if ch in _OCT_CHARS:
            k = j
            j -= 1
            while k < end and k - j < 3 and text[k] in _OCT_CHARS:
                k += 1
            value = int(text[j:k], 8)
            numeric_entries.append(NumericEntry(value, i))
            i = k
            continue
        if ch == "o":
            if j < end:
                ch = text[j]
                j += 1
                if ch == "{":
                    k = j
                    while True:
                        if k == end:
                            error_handler(get_subpos(i), "Missing closing brace (`}`) in delimited octal escape "
                                          "sequence")
                            return None
                        ch = text[k]
                        if ch == "}":
                            break
                        if ch not in _OCT_CHARS:
                            error_handler(get_subpos(k), "Invalid digit (%s) in delimited octal escape sequence",
                                          _b.quote(ch))
                            return None
                    if j == k:
                        error_handler(get_subpos(i), "Empty delimited octal escape sequence")
                        return None
                    value = int(text[j:k], 8)
                    numeric_entries.append(NumericEntry(value, i))
                    i = k + 1
                    continue
            error_handler(get_subpos(j), "Missing opening brace (`{`) in delimited octal escape sequence")
            return None
        if ch == "x":
            if j < end:
                ch = text[j]
                j += 1
                if ch in _HEXADEC_CHARS:
                    k = j
                    j -= 1
                    while k < end and text[k] in _HEXADEC_CHARS:
                        k += 1
                    value = int(text[j:k], 16)
                    numeric_entries.append(NumericEntry(value, i))
                    i = k
                    continue
                if ch == "{":
                    k = j
                    while True:
                        if k == end:
                            error_handler(get_subpos(i), "Missing closing brace (`}`) in delimited hexadecimal escape "
                                          "sequence")
                            return None
                        ch = text[k]
                        if ch == "}":
                            break
                        if ch not in _HEXADEC_CHARS:
                            error_handler(get_subpos(k), "Invalid digit (%s) in delimited hexadecimal escape sequence",
                                          _b.quote(ch))
                            return None
                    if j == k:
                        error_handler(get_subpos(i), "Empty delimited hexadecimal escape sequence")
                        return None
                    value = int(text[j:k], 16)
                    numeric_entries.append(NumericEntry(value, i))
                    i = k + 1
                    continue
            error_handler(get_subpos(j), "Need hexadecimal digit or opening brace (`{`) after `\\x` in string literal")
            return None
        raise NotImplementedError                
    payload = "".join(parts)
    return UnpackedStringLiteral(payload, literal_type, suffix)


_STRING_LITERAL_TYPE_MAP = {
    "":   StringLiteralType.PLAIN,
    "L":  StringLiteralType.WIDE,
    "u8": StringLiteralType.UTF_8,
    "u":  StringLiteralType.UTF_16,
    "U":  StringLiteralType.UTF_32,
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


_HEXADEC_CHARS = frozenset("0123456789ABCDEFabcdef")

_OCT_CHARS = frozenset("01234567")
