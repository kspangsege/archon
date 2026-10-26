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

import archon.base as _b
import archon.text_pos as _tp
import archon.cpp_preprocess._tokenize as _ct


# Is called with the tokens that make up a preprocessing directive, except the leading `#`
# and the whitespace that immediately follows it. Additionally, since leading and trailing
# whitespace is not considered part of a preprocessing directive, the passed tokens will
# never have trailing whitespace. For a null directive, an empty sequence will be
# passed. The passed position, `pos`, is the position of the `#` token. `end_pos` is the
# position of the first non-whitespace token that follows the directive (`NEWLINE` or
# `END_OF_INPUT`).
#
class DirectiveHandler(typing.Protocol):
    def __call__(self, tokens: _cabc.Sequence[_ct.Token], pos: _ct.Position, end_pos: _ct.Position) -> None:
        ...


# The directive detector operates as a wrapper around a tokenizer and scans the token stream
# for preprocessing directives. Whenever it finds a directive, it removes the tokens from the
# token stream and passes them to the directive handler. The tokens that make up a directive
# is understood as starting with the `#` token and ending with the last non-whitespace token
# on the logical line (produced by line splicing). Thus, the tokens of a directive do not
# include leading and trailing whitespace. In particular, it does not include the newline
# that terminates the logical line. The tokens that are passed to the directive handler are
# the tokens that make up the directive minus the leading `#` and any whitespace that
# immediately follows it.
#
class DirectiveDetector:
    def __init__(self, tokenizer: _ct.Tokenizer, directive_handler: DirectiveHandler) -> None:
        self._tokenizer = tokenizer
        self._directive_handler = directive_handler
        self._pending_tokens: _cabc.Iterator[_ct.Token] | None = None
        self._at_line_start = True

    # The end of the token stream is marked by an END_OF_INPUT token.
    #
    def next_token(self) -> _ct.Token:
        if self._pending_tokens:
            if token := next(self._pending_tokens, None):
                return token
            self._pending_tokens = None
        token = self._tokenizer.next_token()
        if not self._at_line_start:
            if token.type_ is _ct.TokenType.NEWLINE:
                self._at_line_start = True
            return token
        if token.type_ in _ct.SPACE:
            return token
        if token.type_ is _ct.TokenType.NEWLINE or token.type_ is _ct.TokenType.END_OF_INPUT:
            return token
        if token.type_ is not _ct.TokenType.HASH:
            self._at_line_start = False
            return token
        pos = token.pos
        tokens: list[_ct.Token] = []
        while True:
            token = self._tokenizer.next_token()
            if token.type_ is _ct.TokenType.NEWLINE or token.type_ is _ct.TokenType.END_OF_INPUT:
                terminator = token
                break
            tokens.append(token)
        j = len(tokens)
        while j > 0 and tokens[j - 1].type_ in _ct.SPACE:
            j -= 1
        i = 0
        while i < j and tokens[i].type_ in _ct.SPACE:
            i += 1
        self._directive_handler(tokens[i:j], pos, terminator.pos)
        trailing_tokens = tokens[j:]
        trailing_tokens.append(terminator)
        self._pending_tokens = iter(trailing_tokens)
        return next(self._pending_tokens)


# This function determines the type of directive made up by the specified tokens, which must
# be the tokens that follow the leading `#` minus any whitespace that immediately follows
# `#`. If the list of specified tokens is empty, the directive is a null
# directive. Otherwise, the directive type is determined by the first token in the specified
# list. If that token is not an identifier or the identifier does not name one of the
# defined directive types in C++26, this function generates an error and returns
# `None`. When the function is able to determine the directive type, it returns that type
# and the list of remaining tokens minus any whitespace occurring immediately after the
# type-specifying identifier.
#
def preparse_directive(tokens: _cabc.Sequence[_ct.Token],
                       error_handler: _ct.ErrorHandler) -> tuple[DirectiveType, _cabc.Sequence[_ct.Token]] | None:
    return _preparse_directive(tokens, error_handler)


class DirectiveType(enum.Enum):
    NULL     = enum.auto()
    IF       = enum.auto()
    IFDEF    = enum.auto()
    IFNDEF   = enum.auto()
    ELIF     = enum.auto()
    ELIFDEF  = enum.auto()
    ELIFNDEF = enum.auto()
    ELSE     = enum.auto()
    ENDIF    = enum.auto()
    INCLUDE  = enum.auto()
    DEFINE   = enum.auto()
    UNDEF    = enum.auto()
    LINE     = enum.auto()
    ERROR    = enum.auto()
    WARNING  = enum.auto()
    PRAGMA   = enum.auto()


def parse_define_directive(tokens: _cabc.Sequence[_ct.Token], pos: _ct.Position, end_pos: _ct.Position,
                           error_handler: _ct.ErrorHandler) -> MacroDef | None:
    return _parse_define_directive(tokens, pos, end_pos, error_handler)


@dataclasses.dataclass(slots=True, frozen=True)
class MacroDef:
    pos:         _ct.Position
    name:        str
    params:      tuple[str, ...] | None
    is_variadic: bool
    replacement: tuple[ReplaceElem, ...]


type ReplaceElem = ReplacePrim | FuseOperation

type ReplacePrim = _ct.Token | StringifyOperand | StringifyOperation

type StringifyOperand = ParamRef | VaArgs | VaOpt

@dataclasses.dataclass(slots=True, frozen=True)
class ParamRef:
    param_index: int
    pos:         _ct.Position

@dataclasses.dataclass(slots=True, frozen=True)
class VaArgs:
    pos: _ct.Position

@dataclasses.dataclass(slots=True, frozen=True)
class VaOpt:
    elements: tuple[ReplaceElem, ...]  # Must not contain `VaOpt`, directly or indirectly
    pos:      _ct.Position

@dataclasses.dataclass(slots=True, frozen=True)
class StringifyOperation:
    operand: StringifyOperand
    pos:     _ct.Position  # Operator position

@dataclasses.dataclass(slots=True, frozen=True)
class FuseOperation:
    left:   ReplacePrim
    rights: tuple[Right, ...]
    pos:    _ct.Position  # Of left operand

    @dataclasses.dataclass(slots=True, frozen=True)
    class Right:
        operand: ReplacePrim
        pos:     _ct.Position  # Operator position


# If the specified text is neither `__VA_ARGS__` nor `__VA_OPT__`, this function returns
# true. Otherwise it generates an error and returns false. `text` should be the text
# associated with a token and `pos` should be the position of that token.
#
# This check is generally performed as part of directive parsing. It is the responsibility
# of the application (the preprocessor) to check identifiers that are not part of
# preprocessing directives. This includes identifiers formed through token fusion
# (`##`). C++26 requires that `__VA_ARGS__` and `__VA_OPT__` occur nowhere outside the body
# of a variadic macro definition. Presumably, this applies to both immediate occurrence and
# synthesis through token fusion. GCC and Clang ban both immediate and synthesized
# occurrences. MSVC warns in moth cases.
#
def verify_no_va_names(text: str, pos: _ct.Position, error_handler: _ct.ErrorHandler) -> bool:
    if text == _VA_ARGS_NAME:
        error_handler(pos, "`__VA_ARGS__` can only appear in body of a variadic macro definition")
        return False
    if text == _VA_OPT_NAME:
        error_handler(pos, "`__VA_OPT__` can only appear in body of a variadic macro definition")
        return False
    return True








def _preparse_directive(tokens: _cabc.Sequence[_ct.Token],
                        error_handler: _ct.ErrorHandler) -> tuple[DirectiveType, _cabc.Sequence[_ct.Token]] | None:
    if not tokens:
        return DirectiveType.NULL, []
    token = tokens[0]
    token_type = token.type_
    assert token_type not in _ct.SPACE
    if token_type is not _ct.TokenType.IDENTIFIER:
        error_handler(token.pos, "Invalid token type after `#` in preprocessing directive")
        return None
    directive_type = _DIRECTIVE_MAP.get(token.text)
    if directive_type is None:
        error_handler(token.pos, "Invalid directive type (%s) in preprocessing directive",
                      _b.clamped_quote(token.text, 64))
        return None
    i = 1
    n = len(tokens)
    while i < n and tokens[i].type_ in _ct.SPACE:
        i += 1
    return directive_type, tokens[i:]


_DIRECTIVE_MAP = {
    "if":       DirectiveType.IF,
    "ifdef":    DirectiveType.IFDEF,
    "ifndef":   DirectiveType.IFNDEF,
    "elif":     DirectiveType.ELIF,
    "elifdef":  DirectiveType.ELIFDEF,
    "elifndef": DirectiveType.ELIFNDEF,
    "else":     DirectiveType.ELSE,
    "endif":    DirectiveType.ENDIF,
    "include":  DirectiveType.INCLUDE,
    "define":   DirectiveType.DEFINE,
    "undef":    DirectiveType.UNDEF,
    "line":     DirectiveType.LINE,
    "error":    DirectiveType.ERROR,
    "warning":  DirectiveType.WARNING,
    "pragma":   DirectiveType.PRAGMA,
}


def _parse_define_directive(tokens: _cabc.Sequence[_ct.Token], pos: _ct.Position, end_pos: _ct.Position,
                            error_handler: _ct.ErrorHandler) -> MacroDef | None:
    if not tokens:
        error_handler(end_pos, "Missing macro name in #define directive")
        return None

    token: _ct.Token | None
    token = tokens[0]
    if token.type_ is not _ct.TokenType.IDENTIFIER:
        error_handler(token.pos, "Macro name must be an identifier")
        return None

    name = token.text
    if name == _DEFINED_NAME:
        error_handler(token.pos, "`defined` cannot be used as a macro name")
        return None
    if not verify_no_va_names(name, token.pos, error_handler):
        return None

    params: list[str] | None = None
    is_variadic = False

    i = 1
    n = len(tokens)
    if n >= 2:
        token = tokens[1]
        if token.type_ is _ct.TokenType.PUNCT and token.text == "(":
            params = []
            expect_param = True
            i = 2
            while True:
                if i < n:
                    token = tokens[i]
                    i += 1
                    if token.type_ in _ct.SPACE:
                        continue
                    token_pos = token.pos
                else:
                    token = None
                    token_pos = end_pos
                if expect_param:
                    expect_param = False
                    if token is not None:
                        if token.type_ is _ct.TokenType.IDENTIFIER:
                            param = token.text
                            if not verify_no_va_names(param, token.pos, error_handler):
                                return None
                            if param in params:
                                error_handler(token_pos, "Duplicate name %s in macro parameter list",
                                              _b.clamped_quote(param, 64))
                                return None
                            params.append(param)
                            continue
                        if token.type_ is _ct.TokenType.PUNCT:
                            if token.text == "...":
                                is_variadic = True
                                continue
                            if token.text == ")" and not params:
                                break
                    if not params:
                        error_handler(token_pos, "Expected parameter name, `...` (ellipsis), or `)` after `(` in "
                                      "macro parameter list")
                    else:
                        error_handler(token_pos, "Expected parameter name or `...` (ellipsis) after `,` (comma) in "
                                      "macro parameter list")
                    return None
                if is_variadic:
                    if token and token.type_ is _ct.TokenType.PUNCT and token.text == ")":
                        break
                    error_handler(token_pos, "Expected `)` after `...` (ellipsis) in macro parameter list")
                    return None
                if token and token.type_ is _ct.TokenType.PUNCT:
                    if token.text == ")":
                        break
                    if token.text == ",":
                        expect_param = True
                        continue
                error_handler(token_pos, "Expected `,` (comma) or `)` after parameter name in macro parameter list")
                return None
        elif token.type_ not in _ct.SPACE:
            error_handler(token.pos, "Missing whitespace after macro name in definition of object-like macro")
            return None

    while i < n and tokens[i].type_ in _ct.SPACE:
        i += 1
    params_2 = None if params is None else tuple(params)
    replacement = _parse_macro_replacement_tokens(name, params_2, is_variadic, tokens[i:], end_pos, error_handler)
    if replacement is None:
        return None
    return MacroDef(pos, name, params_2, is_variadic, replacement)


def _parse_macro_replacement_tokens(macro_name: str, params: tuple[str, ...] | None, is_variadic: bool,
                                    tokens: _cabc.Sequence[_ct.Token], end_pos: _ct.Position,
                                    error_handler: _ct.ErrorHandler) -> tuple[ReplaceElem, ...] | None:
    param_indexes = {name: index for index, name in enumerate(params)} if params is not None else {}

    elements_1: list[_ct.Token | ParamRef | VaArgs] = []
    for token in tokens:
        if token.type_ is _ct.TokenType.IDENTIFIER:
            if token.text == _VA_ARGS_NAME:
                if not is_variadic:
                    error_handler(token.pos, "`__VA_ARGS__` can only appear in body of a variadic macro definition")
                    return None
                elements_1.append(VaArgs(token.pos))
                continue
            index = param_indexes.get(token.text)
            if index is not None:
                elements_1.append(ParamRef(index, token.pos))
                continue
        elements_1.append(token)

    elements_2: list[_ct.Token | StringifyOperand] = []
    i = 0
    n = len(elements_1)
    while i < n:
        elem = elements_1[i]
        i += 1
        if not isinstance(elem, _ct.Token) or elem.type_ is not _ct.TokenType.IDENTIFIER or elem.text != _VA_OPT_NAME:
            elements_2.append(elem)
            continue

        if not is_variadic:
            error_handler(elem.pos, "`__VA_OPT__` can only appear in body of a variadic macro definition")
            return None

        va_opt_pos = elem.pos
        while True:
            if i < n:
                elem = elements_1[i]
                if isinstance(elem, _ct.Token):
                    token = elem
                    if token.type_ in _ct.SPACE:
                        i += 1
                        continue
                    if token.type_ is _ct.TokenType.PUNCT and token.text == "(":
                        break
            pos = end_pos if i == n else elem.pos
            error_handler(pos, "`__VA_OPT__` must be followed by `(`")
            return None

        i += 1
        j = i
        depth = 1
        while True:
            if i == n:
                error_handler(end_pos, "Missing `)` after `__VA_OPT__(`")
                return None
            elem = elements_1[i]
            i += 1
            if not isinstance(elem, _ct.Token):
                continue
            if elem.type_ is _ct.TokenType.IDENTIFIER:
                if elem.text != _VA_OPT_NAME:
                    continue
                error_handler(elem.pos, "`__VA_OPT__` is not allowed to occur inside `__VA_OPT__(...)`")
                return None
            if elem.type_ is _ct.TokenType.PUNCT:
                if elem.text == "(":
                    depth += 1
                    continue
                if elem.text == ")":
                    depth -= 1
                    if depth == 0:
                        k = i - 1
                        break

        opt_elements_1 = elements_1[j:k]
        opt_elements_2 = _parse_stringification(opt_elements_1, end_pos, macro_name, error_handler)
        if opt_elements_2 is None:
            return None
        is_nested = True
        opt_elements_3 = _parse_fusion(opt_elements_2, is_nested, macro_name, error_handler)
        if opt_elements_3 is None:
            return None
        elements_2.append(VaOpt(tuple(opt_elements_3), va_opt_pos))

    elements_3: _cabc.Sequence[ReplacePrim] | None
    if params is not None:
        elements_3 = _parse_stringification(elements_2, end_pos, macro_name, error_handler)
        if elements_3 is None:
            return None
    else:
        elements_3 = elements_2

    is_nested = False
    elements_4 = _parse_fusion(elements_3, is_nested, macro_name, error_handler)
    if elements_4 is None:
        return None

    return tuple(elements_4)


def _parse_stringification(elements: _cabc.Sequence[_ct.Token | StringifyOperand], end_pos: _ct.Position,
                           macro_name: str, error_handler: _ct.ErrorHandler) -> list[ReplacePrim] | None:
    result: list[ReplacePrim] = []
    i = 0
    n = len(elements)
    while i < n:
        elem = elements[i]
        i += 1
        if not isinstance(elem, _ct.Token) or elem.type_ is not _ct.TokenType.HASH:
            result.append(elem)
            continue

        hash_pos = elem.pos
        while True:
            if i == n:
                error_handler(end_pos, "Missing operand of `#` (stringify operator) in definition of macro %s",
                              _b.clamped_quote(macro_name, 64))
                return None
            elem = elements[i]
            i += 1
            if not isinstance(elem, _ct.Token):
                result.append(StringifyOperation(elem, hash_pos))
                break
            token = elem
            if token.type_ in _ct.SPACE:
                continue
            error_handler(token.pos, "Invalid operand of `#` (stringify operator) in definition of macro %s: Must be "
                          "a parameter name, `__VA_ARGS__`, or `__VA_OPT__(...)`", _b.clamped_quote(macro_name, 64))
            return None
    return result


def _parse_fusion(elements: _cabc.Sequence[ReplacePrim], is_nested: bool, macro_name: str,
                  error_handler: _ct.ErrorHandler) -> list[ReplaceElem] | None:
    elem: ReplaceElem

    # C++26 requires that `##` does not occur as the first or last non-whitespace token in
    # the replacement list (or in the list of tokens between the parentheses of a
    # `__VA_OPT__(...)` construct).
    i = 0
    n = len(elements)
    while i < n:
        elem = elements[i]
        i += 1
        if not isinstance(elem, _ct.Token):
            break
        token = elem
        if token.type_ in _ct.SPACE:
            continue
        if token.type_ is not _ct.TokenType.HASH_HASH:
            break
        what = "`__VA_OPT__` argument" if is_nested else "replacement list"
        error_handler(token.pos, "`##` (fuse operator) is not allowed at beginning of %s in definition of macro %s",
                      what, _b.clamped_quote(macro_name, 64))
        return None
    j = n
    while j > i:
        elem = elements[j - 1]
        j -= 1
        if not isinstance(elem, _ct.Token):
            break
        token = elem
        if token.type_ in _ct.SPACE:
            continue
        if token.type_ is not _ct.TokenType.HASH_HASH:
            break
        what = "`__VA_OPT__` argument" if is_nested else "replacement list"
        error_handler(token.pos, "`##` (fuse operator) is not allowed at end of %s in definition of macro %s",
                      what, _b.clamped_quote(macro_name, 64))
        return None

    result: list[ReplaceElem] = []
    i = 0
    while i < n:
        elem = elements[i]
        i += 1
        if not (isinstance(elem, _ct.Token) and elem.type_ is _ct.TokenType.HASH_HASH):
            result.append(elem)
            continue
        pos = elem.pos

        # A left operand is guaranteed to exists because we checked initially that the
        # left-most non-whitespace element is not `##`.
        while True:
            elem = result.pop()
            if not (isinstance(elem, _ct.Token) and elem.type_ in _ct.SPACE):
                left = elem
                break

        # A right operand is guaranteed to exists because we checked initially that the
        # right-most non-whitespace element is not `##`.
        while True:
            elem = elements[i]
            i += 1
            if not (isinstance(elem, _ct.Token) and elem.type_ in _ct.SPACE):
                right = elem
                break

        rights = (FuseOperation.Right(right, pos),)
        if isinstance(left, FuseOperation):
            operation = FuseOperation(left.left, left.rights + rights, left.pos)
        else:
            operation = FuseOperation(left, rights, left.pos)
        result.append(operation)

    return result


_DEFINED_NAME = "defined"

_VA_ARGS_NAME = "__VA_ARGS__"

_VA_OPT_NAME = "__VA_OPT__"
