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
import enum
import re
import unicodedata

import archon.base as _b
import archon.text_pos as _tp


# When `synthetic` is True, the token has been synthesized (produced by `#` or `##`
# operator). In that case, the position refers to the position of the operator and
# subpositions cannot be interpolated across the token text.
#
# Non-synthesized tokens carry information about resolved free-standing UCNs in `ucn_map`. A
# free-standing UCN is a UCN that is not between the the quotes of a string or character
# literal. An UCN (Universal Character Name) is an escape sequence that names a universal
# (Unicode) character. The information about resolved UCNs allow for reconstruction of the
# original token text and allows for the mapping of position in the resolved token text to
# positions in the original source code.
#
# INVARIANT: not self.ucn_map or not self.synthetic
#
@dataclasses.dataclass(slots=True, frozen=True)
class Token:
    type_:     TokenType
    text:      str
    pos:       Position
    ucn_map:   ReplacementMap | None = None
    synthetic: bool                  = False

    def is_space(self) -> bool:
        return self.type_ in SPACE

    def is_regular(self) -> bool:
        return self.type_ in REGULAR

    def is_error(self) -> bool:
        return self.type_ in ERROR

    def is_special(self) -> bool:
        return self.type_ in SPECIAL

    def subpos(self, offset: int) -> Position:
        if self.synthetic:
            return self.pos
        offset_2 = offset if self.ucn_map is None else self.ucn_map(offset)
        return self.pos.shift(offset_2)


# A `HIDDEN_IDENTIFIER` token is like an `IDENTIFIER` token except that it cannot be the
# leading identifier of a macro invocation.
#
# `BAD_BLOCK_COMMENT` is an unterminated block comment. It extends to the end of input.
#
# `BAD_RAW_STRING_LIT` is an unterminated raw string literal, the start of a raw string
# literal where the opening parenthesis was not found, a raw string literal with invalid
# delimiter (the part between the opening quotation mark and the opening parenthesis), or a
# raw string literal with a suffix that contains invalid UCNs. In the case of an
# unterminated raw string literal, the token extends to the end of input. In the case of an
# unfound `(`, the token extends to the end of the line, not including the newline
# character.
#
# `BAD_STRING_LIT` is an unterminated string literal or a string literal with a suffix that
# contains invalid UCNs. In the case of missing termination, the token extends to the end of
# the line, not including the newline character.
#
# `BAD_CHAR_LIT` is an unterminated character literal or a character literal with a suffix
# that contains invalid UCNs. In the case of missing termination, the token extends to the
# end of the line, not including the newline character.
#
# `BAD_NUMBER` is a number token that contains invalid UCNs.
#
# `BAD_IDENTIFIER` is an identifier token that contains invalid UCNs.
#
# `BAD_CHAR` is a stray character that is not `@`, `$`, `\`, or the backtick itself.
#
# `BAD_HEADER_NAME` is an unterminated header name. It extends to the end of the line, not
# including the newline character.
#
class TokenType(enum.Enum):
    # Space
    NEWLINE            = enum.auto()
    WHITESPACE         = enum.auto()
    LINE_COMMENT       = enum.auto()
    BLOCK_COMMENT      = enum.auto()

    # Regular
    HASH_HASH          = enum.auto()
    HASH               = enum.auto()
    RAW_STRING_LIT     = enum.auto()
    STRING_LIT         = enum.auto()
    CHAR_LIT           = enum.auto()
    NUMBER             = enum.auto()
    IDENTIFIER         = enum.auto()
    PUNCT              = enum.auto()
    STRAY_CHAR         = enum.auto()  # `@`, `$`, `\`, or the backtick itself

    # Special
    HEADER_NAME        = enum.auto()
    HIDDEN_IDENTIFIER  = enum.auto()
    END_OF_INPUT       = enum.auto()

    # Error
    BAD_BLOCK_COMMENT  = enum.auto()
    BAD_RAW_STRING_LIT = enum.auto()
    BAD_STRING_LIT     = enum.auto()
    BAD_CHAR_LIT       = enum.auto()
    BAD_NUMBER         = enum.auto()
    BAD_IDENTIFIER     = enum.auto()
    BAD_CHAR           = enum.auto()
    BAD_HEADER_NAME    = enum.auto()


SPACE = {TokenType.NEWLINE, TokenType.WHITESPACE, TokenType.LINE_COMMENT, TokenType.BLOCK_COMMENT}

SPECIAL = {TokenType.HEADER_NAME, TokenType.HIDDEN_IDENTIFIER, TokenType.END_OF_INPUT}

ERROR = {TokenType.BAD_BLOCK_COMMENT, TokenType.BAD_RAW_STRING_LIT, TokenType.BAD_STRING_LIT, TokenType.BAD_CHAR_LIT,
         TokenType.BAD_NUMBER, TokenType.BAD_IDENTIFIER, TokenType.BAD_CHAR, TokenType.BAD_HEADER_NAME}

REGULAR = set(TokenType) - SPACE - ERROR - SPECIAL


class ErrorHandler(typing.Protocol):
    def __call__(self, pos: Position, message: str, *args: typing.Any) -> None:
        ...


# `pos_in_file` marks a position within the position space created through use of a position
# tracker (_tp.TextPosTracker). If the token was produced by a tokenizer, this is the
# position tracker that was passed to the `Tokenizer` constructor. This position space does
# not include honored splicing connectors (backslash + newline), which means that it is
# expressed in terms of logical lines, not physical lines. This is necessary in order to
# allow for accurate position interpolation across the token texts, which do not include the
# splicing connectors. Splicing connectors cannot be dealt with in the same way as resolved
# free-standing UCNs, because such UCNs must be restored by `#x`, but splicing connectors
# must not.
#
@dataclasses.dataclass(slots=True, frozen=True)
class Position:
    file_index:  int
    pos_in_file: int

    def shift(self, offset: int) -> Position:
        return Position(file_index = self.file_index, pos_in_file = self.pos_in_file + offset)


# Given a string where non-overlapping substrings have been replaced by something new, this
# map can be used to map a position in the new string to the corresponding position in the
# original. The map must have an entry for each performed replacement.
#
@dataclasses.dataclass(slots=True)
class ReplacementMap:
    entries: tuple[Entry, ...]  # In order of position and non-overlapping

    @dataclasses.dataclass(slots=True, frozen=True)
    class Entry:
        pos:  int  # Position in new string with replacements
        size: int  # Size of replacement
        orig: str  # Replaced substring

    def __call__(self, offset: int) -> int:
        shift = 0
        for entry in self.entries:
            if entry.pos >= offset:
                break
            if entry.pos + entry.size > offset:
                return entry.pos + shift
            shift += len(entry.orig) - entry.size
        return offset + shift


# Tokenize C++ source code in accordance with a C++26 preprocessor.
#
# The input must be "newline normalized" (text mode).
#
# The output is a stream of preprocessing tokens not including token type
# `HIDDEN_IDENTIFIER`. At the end of input, an `END_OF_INPUT` token is generated.
#
# The tokenizer verifies that all generated identifier tokens are on Unicode Normalization
# Form C (NFC). The application ensure that any synthesized identifiers are also Unicode
# Normalization Form C by calling `verify_identifier_nfc()`.
#
class Tokenizer:
    def __init__(self, input_: typing.TextIO, file_index: int, tracker: _tp.TextPosTracker,
                 error_handler: ErrorHandler) -> None:
        self._input = input_
        self._file_index = file_index
        self._position_tracker = tracker
        self._error_handler = error_handler
        self._splicing_disabled = False
        self._physical_line: str
        self._physical_line_begin: int
        self._physical_line_end: int
        self._read_physical_line()
        self._buffer: list[str] = [""]  # Nonempty list of parts
        self._buffer_size = 0  # Combined size of parts
        self._buffer_pos = 0
        self._splices: list[_Splice] = []
        self._header_name_state = _HeaderNameState.START_OF_LINE
        self._expect_header_name = False

    # The end of the token stream is marked by an END_OF_INPUT token.
    #
    def next_token(self) -> Token:
        token = self._next_token()
        token_type = token.type_
        if token_type in SPACE:
            if token_type is TokenType.NEWLINE:
                self._expect_header_name = False
                self._header_name_state = _HeaderNameState.START_OF_LINE
        else:
            self._expect_header_name = False
            match self._header_name_state:
                case _HeaderNameState.START_OF_LINE:
                    if token_type is TokenType.HASH:
                        self._header_name_state = _HeaderNameState.AFTER_HASH
                    else:
                        if token_type is TokenType.IDENTIFIER and token.text == "import":
                            self._expect_header_name = True
                        self._header_name_state = _HeaderNameState.NOT_IN_DIRECTIVE
                case _HeaderNameState.NOT_IN_DIRECTIVE:
                    pass
                case _HeaderNameState.AFTER_HASH:
                    self._header_name_state = _HeaderNameState.IN_DIRECTIVE
                    if token_type is TokenType.IDENTIFIER:
                        if token.text in {"include", "embed"}:
                            self._expect_header_name = True
                        if token.text in {"if", "elif", "embed"}:
                            self._header_name_state = _HeaderNameState.EXPECT_HAS_INCLUDE
                case _HeaderNameState.IN_DIRECTIVE:
                    pass
                case _HeaderNameState.EXPECT_HAS_INCLUDE:
                    if token_type is TokenType.IDENTIFIER and token.text in {"__has_include", "__has_embed"}:
                        self._header_name_state = _HeaderNameState.AFTER_HAS_INCLUDE
                case _HeaderNameState.AFTER_HAS_INCLUDE:
                    if token_type is TokenType.PUNCT and token.text == "(":
                        self._expect_header_name = True
                    self._header_name_state = _HeaderNameState.EXPECT_HAS_INCLUDE
                case _:
                    typing.assert_never(self._header_name_state)
        return token

    def _next_token(self) -> Token:
        pos = self._get_pos()
        ch = self._consume_char()
        if ch is None:
            text = self._commit(last=True)
            assert not text
            return Token(TokenType.END_OF_INPUT, text, pos)
        if ch in "\t\v\f ":
            ch = self._consume_until(_NOT_WHITESPACE_BUT_NEWLINE_REGEX)
            if ch is not None:
                self._unconsume_char()
            text = self._commit()
            return Token(TokenType.WHITESPACE, text, pos)
        if self._expect_header_name:
            if ch == "<":
                ch = self._consume_until(_GREATER_THAN_OR_NEWLINE_REGEX)
                if ch == ">":
                    text = self._commit()
                    return Token(TokenType.HEADER_NAME, text, pos)
                self._unconsume_char()
                self._error_handler(pos, "Unterminated header name (missing closing `>`)")
                text = self._commit()
                return Token(TokenType.BAD_HEADER_NAME, text, pos)
            if ch == '"':
                ch = self._consume_until(_QUOTATION_MARK_OR_NEWLINE_REGEX)
                if ch == '"':
                    text = self._commit()
                    return Token(TokenType.HEADER_NAME, text, pos)
                self._unconsume_char()
                self._error_handler(pos, 'Unterminated header name (missing closing `"`)')
                text = self._commit()
                return Token(TokenType.BAD_HEADER_NAME, text, pos)
            # Intended fall-through
        if ch in _PUNCTUATION_CHARS:
            if ch == "/":
                ch_2 = self._consume_char()
                if ch_2 == "/":
                    ch = self._consume_until(_NEWLINE_REGEX)
                    if ch is not None:
                        self._unconsume_char()
                    text = self._commit()
                    return Token(TokenType.LINE_COMMENT, text, pos)
                if ch_2 == "*":
                    while True:
                        ch = self._consume_until(_ASTERISK_REGEX)
                        if ch is None:
                            break
                        ch = self._consume_char()
                        if ch == "/":
                            text = self._commit()
                            return Token(TokenType.BLOCK_COMMENT, text, pos)
                        if ch is None:
                            break
                        self._unconsume_char()
                    self._error_handler(pos, "Unterminated block comment")
                    text = self._commit()
                    return Token(TokenType.BAD_BLOCK_COMMENT, text, pos)
                if ch_2 is not None:
                    self._unconsume_char()
            elif ch == ".":
                ch_2 = self._consume_char()
                if ch_2 is not None:
                    if ch_2 in "0123456789":
                        return self._consume_number(pos)
                    self._unconsume_char()
            return self._consume_punctuation(ch, pos)
        if _is_xid_start(ch):
            ch = self._consume_until_not_xid_continue()
            if ch is not None:
                self._unconsume_char()
            text = self._commit()
            if ch == "\\":
                state = _IdentFragConsumeState(text)
                self._consume_identifier_fragment(state)
                text = "".join(state.parts)
                bad = state.bad_ucn
                if not bad and not verify_identifier_nfc(text, pos, self._error_handler):
                    bad = True
                token_type = TokenType.BAD_IDENTIFIER if bad else TokenType.IDENTIFIER
                return Token(token_type, text, pos, state.get_ucn_map())
            if ch == "'" or ch == '"':
                if text in {"u8", "u", "U", "L"}:
                    self._reconsume_char()
                    return self._consume_char_or_string_lit(text, ch, pos)
                if ch == '"' and text[-1] == "R" and text[:-1] in {"", "u8", "u", "U", "L"}:
                    self._reconsume_char()
                    return self._consume_raw_string_lit(text, pos)
            bad = False
            if not verify_identifier_nfc(text, pos, self._error_handler):
                bad = True
            token_type = TokenType.BAD_IDENTIFIER if bad else TokenType.IDENTIFIER
            return Token(token_type, text, pos)
        if ch in "0123456789":
            return self._consume_number(pos)
        if ch in "'\"":
            prefix = ""
            return self._consume_char_or_string_lit(prefix, ch, pos)
        if ch == "\n":
            text = self._commit()
            return Token(TokenType.NEWLINE, text, pos)
        if ch == "\\":
            self._roll_back()
            state = _IdentFragConsumeState()
            if not self._try_consume_replaced_ucn(state, first=True):
                # Stray backslash
                self._reconsume_char()
                text = self._commit()
                return Token(TokenType.STRAY_CHAR, text, pos)
            self._consume_identifier_fragment(state)
            text = "".join(state.parts)
            bad = state.bad_ucn
            if not bad and not verify_identifier_nfc(text, pos, self._error_handler):
                bad = True
            token_type = TokenType.BAD_IDENTIFIER if bad else TokenType.IDENTIFIER
            return Token(token_type, text, pos, state.get_ucn_map())
        if ch in _STRAY_CHARS:
            text = self._commit()
            return Token(TokenType.STRAY_CHAR, text, pos)
        self._error_handler(pos, "Invalid character: %s", unicodedata.name(ch))
        text = self._commit()
        return Token(TokenType.BAD_CHAR, text, pos)

    # The read position must be at the current roll-back position (see _commit()).
    #
    def _get_pos(self) -> Position:
        assert self._buffer_pos == 0  # At roll-back position
        pos_in_file = self._position_tracker.current()
        return Position(self._file_index, pos_in_file)

    def _consume_char_or_string_lit(self, prefix: str, first_char: str, pos: Position) -> Token:
        is_string = (first_char == '"')
        pattern = _QUOTATION_MARK_OR_BACKSLASH_OR_NEWLINE_REGEX if is_string else \
            _APOSTROPHE_OR_BACKSLASH_OR_NEWLINE_REGEX
        while True:
            ch = self._consume_until(pattern)
            if ch == first_char:
                break
            if ch == "\\":
                ch = self._consume_char()
            if ch is None or ch == "\n":
                if ch is not None:
                    self._unconsume_char()
                qual = "string" if is_string else "character"
                self._error_handler(pos, "Unterminated %s literal" % qual)
                token_type = TokenType.BAD_STRING_LIT if is_string else TokenType.BAD_CHAR_LIT
                text = prefix + self._commit()
                return Token(token_type, text, pos)
        stem = self._commit()
        state = _IdentFragConsumeState(prefix, stem)
        self._consume_ident_suffix(state)
        if state.bad_ucn:
            token_type = TokenType.BAD_STRING_LIT if is_string else TokenType.BAD_CHAR_LIT
        else:
            token_type = TokenType.STRING_LIT if is_string else TokenType.CHAR_LIT
        text = "".join(state.parts)
        return Token(token_type, text, pos, state.get_ucn_map())

    def _consume_raw_string_lit(self, prefix: str, pos: Position) -> Token:
        parts = [prefix, self._commit()]
        delim_pos = self._get_pos()
        self._disable_splicing()
        ch = self._consume_until(_LPAREN_RPAREN_OR_NEWLINE_REGEX)
        if ch is not None:
            self._unconsume_char()
        delim = self._commit()
        parts.append(delim)
        if ch != "(":
            self._reenable_splicing()
            error_pos = self._get_pos()
            self._error_handler(error_pos, "Missing left parenthesis in raw string literal")
            text = "".join(parts)
            return Token(TokenType.BAD_RAW_STRING_LIT, text, pos)
        bad_delim = False
        for i, ch in enumerate(delim):
            if ch in _RAW_STRING_LIT_DELIM_VALID_CHARS:
                continue
            error_pos = delim_pos.shift(i)
            self._error_handler(error_pos, "Invalid character (%s) in raw string literal delimiter", _b.quote(ch))
            bad_delim = True
            break
        if len(delim) > _RAW_STRING_LIT_DELIM_MAX_LEN:
            error_pos = delim_pos.shift(_RAW_STRING_LIT_DELIM_MAX_LEN)
            self._error_handler(error_pos, "Too many characters in raw string literal delimiter")
            bad_delim = True
        self._reconsume_char()
        while True:
            ch = self._consume_until(_RPAREN_REGEX)
            if ch is not None:
                found = False
                i = 0
                n = len(delim)
                while True:
                    ch = self._consume_char()
                    if i == n:
                        if ch == '"':
                            found = True
                        break
                    if ch != delim[i]:
                        # Because `delim` cannot contain `)`, we know that we have not
                        # passed any `)` that could be the start of the closing sequence, so
                        # the search for `)` can continue from the current read position
                        # minus one.
                        break
                    i += 1
                if found:
                    break
                if ch is not None:
                    self._unconsume_char()
                    continue
            parts.append(self._commit())
            self._reenable_splicing()
            self._error_handler(pos, "Unterminated raw string literal")
            text = "".join(parts)
            return Token(TokenType.BAD_RAW_STRING_LIT, text, pos)
        stem = self._commit()
        self._reenable_splicing()
        state = _IdentFragConsumeState(*parts, stem)
        self._consume_ident_suffix(state)
        token_type = TokenType.BAD_RAW_STRING_LIT if bad_delim or state.bad_ucn else TokenType.RAW_STRING_LIT
        text = "".join(state.parts)
        return Token(token_type, text, pos, state.get_ucn_map())

    def _consume_number(self, pos: Position) -> Token:
        state = _IdentFragConsumeState()
        while True:
            ch = self._consume_until(_NONWORD_OR_EXPONENT_ASCII_REGEX)
            if ch == ".":
                continue
            if ch is None:
                break
            if ch in "eEpP":
                ch = self._consume_char()
                if ch is None:
                    break
                if ch not in "+-":
                    self._unconsume_char()
                continue
            if ch == "'":
                ch = self._consume_char()
                if ch is not None:
                    if not _NONWORD_ASCII_REGEX.fullmatch(ch):
                        continue
                    self._unconsume_char()
                self._unconsume_char()  # Unconsume the `'`
                break
            if _is_xid_continue(ch):
                continue
            if ch != "\\":
                self._unconsume_char()
                break
            self._unconsume_char()
            if part := self._commit():
                state.add_part(part)
            if not self._try_consume_replaced_ucn(state):
                break
        if part := self._commit():
            state.add_part(part)
        token_type = TokenType.BAD_NUMBER if state.bad_ucn else TokenType.NUMBER
        text = "".join(state.parts)
        return Token(token_type, text, pos, state.get_ucn_map())

    def _consume_punctuation(self, first_char: str, pos: Position) -> Token:
            text = first_char
            lexeme_len, valid_prefix = _PUNCTUATION_MAP.get(text, (None, False))
            best_len = lexeme_len
            while valid_prefix:
                ch = self._consume_char()
                if ch is None:
                    break
                text += ch
                lexeme_len, valid_prefix = _PUNCTUATION_MAP.get(text, (None, False))
                if lexeme_len is not None:
                    best_len = lexeme_len
            assert best_len is not None
            assert best_len >= 1
            for _ in range(best_len, len(text)):
                self._unconsume_char()
            text = self._commit()
            token_type = _PUNCTUATION_TYPE_MAP.get(text, TokenType.PUNCT)
            return Token(token_type, text, pos)

    # The read position must be at the current roll-back position (see _commit()). Upon
    # return, the read position will be at the new roll-back position.
    #
    def _consume_ident_suffix(self, state: _IdentFragConsumeState) -> None:
        ch = self._consume_char()
        if ch is None:
            return
        if _is_xid_start(ch):
            text = self._commit()
            state.add_part(text)
            self._consume_identifier_fragment(state)
            return
        self._roll_back()
        if ch == "\\":
            if self._try_consume_replaced_ucn(state, first=True):
                self._consume_identifier_fragment(state)

    # The read position must be at the current roll-back position (see _commit()). Upon
    # return, the read position will be at the new roll-back position.
    #
    def _consume_identifier_fragment(self, state: _IdentFragConsumeState) -> None:
        while True:
            ch = self._consume_until_not_xid_continue()
            if ch is not None:
                self._unconsume_char()
            if part := self._commit():
                state.add_part(part)
            if ch != "\\" or not self._try_consume_replaced_ucn(state):
                break

    # If this function finds a structurally valid UCN (`\uHHHH`, `\UHHHHHHHH`, `\u{H...}`,
    # or `\N{...}`), it consumes the UCN and returns true. Otherwise, this function rolls
    # back and returns false.
    #
    # The read position must be at the current roll-back position (see _commit()). Upon
    # return, the read position will be at the new roll-back position.
    #
    # Fully valid UCNs are resolved and replaced, meaning that an entry is added to
    # `state.ucn_map_entries` and the represented resolved character is added as a part to
    # `state.parts`. A bad UCN (see below) is added verbatim to `state.parts` (in its
    # unresolved form) and causes `state.bad_ucn` to be set to true.
    #
    # A structurally valid UCN on the form `\N{...}` is considered bad if the specified
    # character name is not a valid character name or if it refers to a multi-character
    # alias. A structurally valid UCN on any other form is considered bad if the specified
    # code point is outside the Unicode range, in the UTF-16 surrogate range, or in the
    # ASCII range.
    #
    # If `first` is true, the consumed UCN is considered bad if the represented character is
    # not in `XID_First`. Otherwise, the UCN is considered bad if the represented character
    # is not in `XID_Continue`.
    #
    def _try_consume_replaced_ucn(self, state: _IdentFragConsumeState, first: bool = False) -> bool:
        assert self._buffer_pos == 0  # At roll-back position
        pos = self._get_pos()
        def error(offset: int, message: str, *args: typing.Any) -> None:
            text = self._commit()
            error_pos = pos.shift(offset)
            self._error_handler(error_pos, message, *args)
            state.add_part(text)
            state.bad_ucn = True
        ch = self._consume_char()
        if ch != "\\":
            self._roll_back()
            return False
        chars: list[str] = []
        ch = self._consume_char()
        if ch == "u":
            offset = self._buffer_pos
            ch = self._consume_char()
            if ch == "{":
                offset += 1
                while True:
                    ch = self._consume_char()
                    if ch == "}":
                        if not chars:
                            self._roll_back()
                            return False
                        break
                    if ch is None or ch not in _HEX_DIGIT_CHARS:
                        self._roll_back()
                        return False
                    chars.append(ch)
            else:
                while True:
                    if ch is None or ch not in _HEX_DIGIT_CHARS:
                        self._roll_back()
                        return False
                    chars.append(ch)
                    if len(chars) == 4:
                        break
                    ch = self._consume_char()
            code_point = int("".join(chars), 16)
        elif ch == "U":
            offset = self._buffer_pos
            while len(chars) < 8:
                ch = self._consume_char()
                if ch is None or ch not in _HEX_DIGIT_CHARS:
                    self._roll_back()
                    return False
                chars.append(ch)
            code_point = int("".join(chars), 16)
        elif ch == "N":
            ch = self._consume_char()
            if ch != "{":
                self._roll_back()
                return False
            offset = self._buffer_pos
            ch = self._consume_until(_RBRACE_OR_NEWLINE_REGEX)
            if ch != "}":
                self._roll_back()
                return False
            i = offset
            j = self._buffer_pos - 1
            n = j - i
            if n < 1:
                self._roll_back()
                return False
            buf = self._ensure_contiguous_buffer()
            name = buf[i:j]
            ch = _resolve_character_name(name)
            if ch is None:
                error(offset, "Invalid Unicode character name (%s) in UCN", _b.clamped_quote(name, 64))
                return True
            code_point = ord(ch)
        else:
            self._roll_back()
            return False
        if 0xD800 <= code_point < 0xE000:
            error(offset, "Invalid UCN code point (U+%04X): Refers to character in surrogate range", code_point)
            return True
        if code_point >= 0x110000:
            error(offset, "Invalid UCN code point (U+%04X): Outside Unicode range", code_point)
            return True
        ch = chr(code_point)
        if not (_is_xid_start(ch) if first else _is_xid_continue(ch)):
            what = "XID_Start" if first else "XID_Continue"
            error(offset, "Invalid UCN code point (U+%04X): Must refer to character in %s", code_point, what)
            return True
        if code_point < 0x80:
            error(offset, "Invalid UCN code point (U+%04X): Refers to character in basic character set", code_point)
            return True
        orig = self._commit()
        state.add_ucn_replacement(ch, orig)
        return True

    def _consume_until_not_xid_continue(self) -> str | None:
        while True:
            ch = self._consume_until(_NONWORD_ASCII_REGEX)
            if ch is None or not _is_xid_continue(ch):
                return ch

    # Tentatively consume next character while performing line splicing as necessary (when
    # enabled and read position crosses a splice connector). If the end of input has already
    # been reached, this function returns `None`.
    #
    # See also _consume_until()
    #
    def _consume_char(self) -> str | None:
        if self._buffer_pos < self._buffer_size:
            buf = self._ensure_contiguous_buffer()
            char = buf[self._buffer_pos]
            self._buffer_pos += 1
            return char
        while True:
            i = self._physical_line_begin
            e = self._physical_line_end
            if i < e:
                char = self._physical_line[i]
                self._physical_line_begin += 1
                self._buffer.append(char)
                self._buffer_size += 1
                self._buffer_pos += 1
                return char
            if e < len(self._physical_line):
                connector = self._physical_line[e:]
                self._splices.append(_Splice(self._buffer_size, connector))
            if not self._physical_line:
                return None
            self._read_physical_line()

    # Move the read position back by one character. It is an error to call this function
    # when the read position already coincides with the current roll-back position.
    #
    def _unconsume_char(self) -> None:
        assert self._buffer_pos > 0
        self._buffer_pos -= 1

    # Move the read position forward by one character. It is an error to call this function
    # if the read-ahead buffer is empty. The read ahead buffer is nonempty after read
    # pointer has been moved back such as through an invocation of `_unconsume_char()`.
    #
    def _reconsume_char(self) -> None:
        assert self._buffer_pos < self._buffer_size
        self._buffer_pos += 1

    # Efficient tentative bulk consumption. This function has the same effect as repeated
    # invocations of `_consume_char()` until the extracted character matches the specified
    # pattern or the end of input has been reached. This function returns the matched
    # character or `None` if the end of input was reached before a match was found.
    #
    # The specified pattern must never match anything other than a single character, and it
    # must not use look-ahead, look-behind, or zero-width assertions.
    #
    def _consume_until(self, pattern: re.Pattern[str]) -> str | None:
        if self._buffer_pos < self._buffer_size:
            buf = self._ensure_contiguous_buffer()
            m = pattern.search(buf, self._buffer_pos)
            if m:
                i = m.start()
                self._buffer_pos = i + 1
                return buf[i]
            self._buffer_pos = self._buffer_size
        while True:
            line = self._physical_line
            i = self._physical_line_begin
            e = self._physical_line_end
            m = pattern.search(line, i, e)
            if m:
                j = m.start() + 1
                n = j - i
                self._physical_line_begin = j
                self._buffer.append(line[i:j])
                self._buffer_size += n
                self._buffer_pos += n
                return line[j - 1]
            n = e - i
            if n > 0:
                self._buffer.append(line[i:e])
                self._buffer_size += n
                self._buffer_pos += n
            if e < len(self._physical_line):
                connector = self._physical_line[e:]
                self._splices.append(_Splice(self._buffer_size, connector))
            if not self._physical_line:
                return None
            self._read_physical_line()

    def _read_physical_line(self) -> None:
        line = self._input.readline()
        self._physical_line = line
        self._physical_line_begin = 0
        self._physical_line_end = len(line)
        if not self._splicing_disabled:
            if connector_size := _find_splice_connector(line):
                self._physical_line_end -= connector_size

    # Roll the read position back to the current roll-back position. This has the same
    # effect as calling `_unconsume_char()` N times where N is the number of characters
    # between the current roll-back position and the current read position.
    #
    def _roll_back(self) -> None:
        self._buffer_pos = 0

    # Move the roll-back position forward to the current read position and return the
    # characters that were moved across. This commits character consumption up to the
    # current read position.
    #
    def _commit(self, last: bool = False) -> str:
        k = self._buffer_pos
        assert k <= self._buffer_size
        self._buffer_size -= k
        self._buffer_pos = 0
        buf = self._ensure_contiguous_buffer()
        i = 0
        new_splices: list[_Splice] = []
        for splice in self._splices:
            j = splice.pos
            if j < k or (last and j == k):
                self._position_tracker.scan_chunk(buf[i:j])
                self._position_tracker.new_line()
                i = j
            else:
                new_splices.append(_Splice(j - k, splice.connector))
        self._position_tracker.scan_chunk(buf[i:k])
        self._buffer[0] = buf[k:]
        self._splices = new_splices
        return buf[:k]

    # Disable line splicing for subsequent tentative character consumption.
    #
    # The read position must be at the current roll-back position (see _commit()).
    #
    def _disable_splicing(self) -> None:
        assert not self._splicing_disabled
        assert self._buffer_pos == 0  # At roll-back position
        buf = self._ensure_contiguous_buffer()
        parts: list[str] = []
        i = 0
        for splice in self._splices:
            j = splice.pos
            parts.append(buf[i:j])
            parts.append(splice.connector)
            i = j
        parts.append(buf[i:])
        buf = "".join(parts)
        self._buffer[0] = buf
        self._buffer_size = len(buf)
        self._splices.clear()
        self._physical_line_end = len(self._physical_line)
        self._splicing_disabled = True

    # Re-enable line splicing for subsequent tentative character consumption.
    #
    # The read position must be at the current roll-back position (see _commit()).
    #
    def _reenable_splicing(self) -> None:
        assert self._splicing_disabled
        assert self._buffer_pos == 0  # At roll-back position
        buf = self._ensure_contiguous_buffer()
        parts = []
        splices = []
        pos = 0
        i = 0
        for m in _SPLICE_CONNECTOR_REGEX.finditer(buf):
            j = m.start()
            part = buf[i:j]
            parts.append(part)
            pos += len(part)
            connector = m.group()
            splices.append(_Splice(pos, connector))
            i = m.end()
        parts.append(buf[i:])
        buf = "".join(parts)
        self._buffer[0] = buf
        self._buffer_size = len(buf)
        self._splices = splices
        if connector_size := _find_splice_connector(self._physical_line):
            end = len(self._physical_line) - connector_size
            if end >= self._physical_line_begin:
                self._physical_line_end = end
        self._splicing_disabled = False

    def _ensure_contiguous_buffer(self) -> str:
        if len(self._buffer) == 1:
            return self._buffer[0]
        buf = "".join(self._buffer)
        self._buffer = [buf]
        return buf


# Verify that the specified identifier is on Unicode Normalization Form C (NFC). If it is,
# this function returns true. Otherwise it generates an error message and returns false.
#
# The tokenizer ensures that all generated identifier tokens are on Unicode Normalization
# Form C (NFC), i.e., it generates error messages if they are not. The application (the
# preprocessor) is responsible for calling this function to verify that any synthesized
# identifiers are also on Unicode Normalization Form C. This includes identifiers resulting
# from splitting off user-defined literal suffixes from character, string, and number
# literals.
#
def verify_identifier_nfc(text: str, pos: Position, error_handler: ErrorHandler) -> bool:
    if unicodedata.is_normalized("NFC", text):
        return True
    error_handler(pos, "Identifier does not conform to Unicode Normalization Form C (NFC)")
    return False








def _is_xid_start(char: str) -> bool:
    return char.isidentifier()

def _is_xid_continue(char: str) -> bool:
    return ("_" + char).isidentifier()


class _HeaderNameState(enum.Enum):
    START_OF_LINE      = enum.auto()
    NOT_IN_DIRECTIVE   = enum.auto()
    AFTER_HASH         = enum.auto()
    IN_DIRECTIVE       = enum.auto()
    EXPECT_HAS_INCLUDE = enum.auto()  # Expect `__has_include` and `__has_embed`
    AFTER_HAS_INCLUDE  = enum.auto()  # After `__has_include` or `__has_embed`


@dataclasses.dataclass(slots=True, frozen=True)
class _Splice:
    pos:       int
    connector: str


def _find_splice_connector(physical_line: str) -> int:
    if len(physical_line) < 2 or not physical_line.endswith("\n"):
        return 0
    ch = physical_line[-2]
    if ch not in " \t\v\f\\":
        return 0
    if ch == "\\":
        return 2
    stripped = physical_line.rstrip(" \t\n\v\f")
    if not stripped.endswith("\\"):
        return 0
    return len(physical_line) - len(stripped) + 1


@dataclasses.dataclass(slots=True)
class _IdentFragConsumeState:
    parts:           list[str]
    offset:          int  # Combined size of parts
    bad_ucn:         bool
    ucn_map_entries: list[ReplacementMap.Entry]

    def __init__(self, *parts: str) -> None:
        self.parts = list(parts)
        self.offset = sum(len(p) for p in parts)
        self.bad_ucn = False
        self.ucn_map_entries = []

    def add_part(self, part: str) -> None:
        self.parts.append(part)
        self.offset += len(part)

    def add_ucn_replacement(self, resolved: str, orig: str) -> None:
        pos = self.offset
        self.add_part(resolved)
        size = len(resolved)
        self.ucn_map_entries.append(ReplacementMap.Entry(pos, size, orig))

    def get_ucn_map(self) -> ReplacementMap | None:
        if self.ucn_map_entries:
            return ReplacementMap(tuple(self.ucn_map_entries))
        return None


_NOT_WHITESPACE_BUT_NEWLINE_REGEX             = re.compile(r"[^ \t\v\f]")
_GREATER_THAN_OR_NEWLINE_REGEX                = re.compile(r"[>\n]")
_QUOTATION_MARK_OR_NEWLINE_REGEX              = re.compile(r'["\n]')
_NEWLINE_REGEX                                = re.compile(r"[\n]")
_ASTERISK_REGEX                               = re.compile(r"[*]")
_APOSTROPHE_OR_BACKSLASH_OR_NEWLINE_REGEX     = re.compile(r"['\\\n]")
_QUOTATION_MARK_OR_BACKSLASH_OR_NEWLINE_REGEX = re.compile(r'["\\\n]')
_LPAREN_RPAREN_OR_NEWLINE_REGEX               = re.compile(r"[()\n]")
_RPAREN_REGEX                                 = re.compile(r"[)]")
_NONWORD_ASCII_REGEX                          = re.compile(r"[^\w]", re.ASCII)
_NONWORD_OR_EXPONENT_ASCII_REGEX              = re.compile(r"[^\w]|[eEpP]", re.ASCII)
_RBRACE_OR_NEWLINE_REGEX                      = re.compile(r"[}\n]")


_PUNCTUATION = {"{", "}", "[", "]", "#", "(", ")", "<", ">", "%", ":", ";", ".", "?", "*", "+", "-", "/", "^", "&",
                "|", "~", "!", "=", ",", "##", "<:", ":>", "<%", "%>", "%:", "::", ".*", "->", "+=", "-=", "*=", "/=",
                "%=", "^=", "&=", "|=", "==", "!=", "<=", ">=", "&&", "||", "<<", ">>", "++", "--", "[:", ":]", "...",
                "->*", "<=>", "<<=", ">>=", "%:%:"}

_BASIC_CHARACTER_SET = frozenset("abcdefghijklmnopqrstuvwxyz"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "0123456789"
                                 " \t\v\f\n"
                                 "!\"#%&'()*+,-./:;<=>?[\\]^_{|}~"
                                 "@$`")

_PUNCTUATION_CHARS = frozenset("".join(_PUNCTUATION))
_STRAY_CHARS       = frozenset("@$\\`")

assert _PUNCTUATION_CHARS.issubset(_BASIC_CHARACTER_SET)
assert _STRAY_CHARS.issubset(_BASIC_CHARACTER_SET)
assert _PUNCTUATION_CHARS.isdisjoint(_STRAY_CHARS)
assert (5  +  # Whitespace (space, horizontal tab, vertical tab, form feed, new-line)
        62 +  # Alphanumeric
        1  +  # Underscore (`_`)
        2  +  # Quotation (`'`, `"`)
        len(_PUNCTUATION_CHARS) +  # Includes `#`
        len(_STRAY_CHARS)) == len(_BASIC_CHARACTER_SET)


_PSEUDOPUNCTUATION = [
    ("<::",  1),
    ("<:::", 2),
    ("<::>", 2),
    ("[::",  1),
    ("[:::", 2),
    ("[:>",  1),
]

def _build_punctuation_map() -> dict[str, tuple[int | None, bool]]:
    rules = {lexeme: len(lexeme) for lexeme in _PUNCTUATION}
    for cand, lexeme_len in _PSEUDOPUNCTUATION:
        assert cand not in rules
        rules[cand] = lexeme_len
    candidates: set[str] = set()
    for lexeme in rules:
        for i in range(1, len(lexeme) + 1):
            candidates.add(lexeme[:i])
    punct_map: dict[str, tuple[int | None, bool]] = {}
    for cand in candidates:
        lexeme_len_2 = rules.get(cand)
        valid_prefix = any(other.startswith(cand) and len(other) > len(cand) for other in rules)
        punct_map[cand] = (lexeme_len_2, valid_prefix)
    return punct_map

_PUNCTUATION_MAP = _build_punctuation_map()


_PUNCTUATION_TYPE_MAP = {
    "#":  TokenType.HASH,
    "##": TokenType.HASH_HASH,
}


_SPLICE_CONNECTOR_REGEX = re.compile(r"\\[ \t\v\f]*\n")


_RAW_STRING_LIT_DELIM_MAX_LEN = 16

_RAW_STRING_LIT_DELIM_VALID_CHARS = _BASIC_CHARACTER_SET.difference(" ()\\\t\v\f\n")


_HEX_DIGIT_CHARS = "0123456789ABCDEFabcdef"


def _resolve_character_name(name: str) -> str | None:
    try:
        string = unicodedata.lookup(name)
        # `unicodedata.lookup()` supports named sequences (`NamedSequences.txt`) such as
        # `KHMER VOWEL SIGN AAM`. C++26 does not (since C++23 proposal P2071), so we need to
        # explicitly reject them.
        if len(string) == 1:
            return string
    except KeyError:
        pass
    return None
