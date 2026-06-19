from __future__ import annotations
from typing import Any, assert_never
from collections.abc import Iterator
from dataclasses import dataclass

import enum
import re

import archon.base as _b
import archon.regex as _r


def compile_(cmake_regex_string: str) -> Regex:
    expression = _parse(cmake_regex_string)
    python_regex_string = _r.format_as_python_regex(expression)
    return Regex(re.compile(python_regex_string))


class Regex:
    def __init__(self, regex: re.Pattern) -> None:
        self._regex = regex

    def matches(self, string: str) -> re.Match[str] | None:
        return self._regex.search(string)


class SyntaxError(Exception):
    def __init__(self, pos: int, message: str, *args: Any) -> None:
        Exception.__init__(self, message % args)
        self.pos = pos








# Grammar for CMake regular expression syntax (simplified/modified version of POSIX ERE /
# KWSys):
#
# %token CHAR
# %token CHAR_CLASS
#
# regex          = alternation
# alternation    = sequence ("|" sequence)*
# sequence       = repetition*
# repetition     = primary quantifier?
# quantifier     = "*" | "+" | "?"
# primary        = group | wildcard | anchor | CHAR | CHAR_CLASS
# group          = "(" alternation ")"
# wildcard       = "."
# anchor         = "^" | "$"


def _parse(cmake_regex_string: str) -> _r.Expression:
    tokens = _tokenize(cmake_regex_string)
    token = next(tokens)

    def parse_alternation() -> _r.Expression:
        alternatives = []
        while True:
            alternatives.append(parse_sequence())
            if not isinstance(token, _BarToken):
                break
            advance()
        if len(alternatives) == 1:
            return alternatives[0]
        return _r.Alternation(alternatives)

    def parse_sequence() -> _r.Expression:
        elements = list[_r.Expression]()
        while isinstance(token, (_CharToken, _WildcardToken, _CharClassToken, _AnchorToken, _LParenToken)):
            expression = parse_repetition()
            if elements and isinstance(elements[-1], _r.Literal) and isinstance(expression, _r.Literal):
                elements[-1] = _r.Literal(elements[-1].string + expression.string)
            else:
                elements.append(expression)
        if len(elements) == 1:
            return elements[0]
        return _r.Sequence(elements)

    def parse_repetition() -> _r.Expression:
        expression = parse_primary()
        if not isinstance(token, _QuantifierToken):
            return expression
        min_: int
        max_: int | None
        match token.quantifier:
            case _Quantifier.STAR:
                min_, max_ = 0, None
            case _Quantifier.PLUS:
                min_, max_ = 1, None
            case _Quantifier.QMARK:
                min_, max_ = 0, 1
            case _:
                assert_never(token.quantifier)
        if max_ is None and _r.is_nullable(expression):
            raise SyntaxError(token.pos, "Unbounded repetition of nullable subexpression")
        advance()
        return _r.Repetition(expression, min_, max_)

    def parse_primary() -> _r.Expression:
        if isinstance(token, _CharToken):
            char = token.char
            advance()
            return _r.Literal(char)
        if isinstance(token, _WildcardToken):
            advance()
            return _r.Wildcard()
        if isinstance(token, _CharClassToken):
            text = token.text
            assert text[0] == "[" and text[-1] == "]"
            char_class = parse_char_class(token.pos + 1, text[1:-1])
            advance()
            return char_class
        if isinstance(token, _AnchorToken):
            anchor = token.anchor
            advance()
            return _r.Anchor(anchor)
        if isinstance(token, _LParenToken):
            return parse_group()
        assert False

    def parse_group() -> _r.Expression:
        assert isinstance(token, _LParenToken)
        open_pos = token.pos
        advance()
        expression = parse_alternation()
        if isinstance(token, _EndOfInputToken):
            raise SyntaxError(open_pos, "Unclosed parenthesis")
        if isinstance(token, _QuantifierToken):
            raise SyntaxError(token.pos, "Invalid use of quantifier (%s)", token.text)
        assert isinstance(token, _RParenToken)
        advance()
        return _r.Group(expression)

    def advance() -> None:
        nonlocal token
        assert not isinstance(token, _EndOfInputToken)
        token = next(tokens)

    def parse_char_class(pos: int, string: str) -> _r.CharClass:
        for i, char in enumerate(string):
            if ord(char) > 127:
                raise SyntaxError(pos + i, "Non-ASCII character not supported in character class (%s)", _b.quote(char))
        inverted = False
        items: list[_r.Item] = []
        i = 0
        n = len(string)
        if i < n and string[i] == "^":
            inverted = True
            i += 1
        while i < n:
            char = string[i]
            if not(i < n - 2 and string[i + 1] == "-"):
                items.append(_r.Char(char))
                i += 1
                continue
            first = char
            continuation = False
            while True:
                last  = string[i + 2]
                first_ord = ord(first)
                last_ord  = ord(last)
                if first_ord > last_ord:
                    raise SyntaxError(pos + i, "Invalid range (%s)", _b.quote(string[i:i+3]))
                if continuation:
                    first_ord += 1
                if first_ord <= last_ord:
                    items.append(_r.Range(chr(first_ord), chr(last_ord)))
                if i < n - 4 and string[i + 3] == "-":
                    i += 2
                    first = last
                    continuation = True
                    continue
                i += 3
                break
        return _r.CharClass(inverted, items)

    expression = parse_alternation()
    if isinstance(token, _RParenToken):
        raise SyntaxError(token.pos, "Unmatched closing parenthesis")
    if isinstance(token, _QuantifierToken):
        raise SyntaxError(token.pos, "Invalid use of quantifier (%s)", _b.quote(token.text))
    assert isinstance(token, _EndOfInputToken)
    return expression


def _tokenize(cmake_regex_string: str) -> Iterator[_Token]:
    for m in _TOKEN_REGEX.finditer(cmake_regex_string):
        pos  = m.start()
        text = m.group()
        kind = m.lastgroup

        if kind == "CHAR":
            if len(text) == 1:
                char = text
            else:
                assert len(text) == 2
                assert text[0] == "\\"
                char = text[1]
            yield _CharToken(pos, text, char)
            continue

        if kind == "WILDCARD":
            yield _WildcardToken(pos, text)
            continue

        if kind == "CLASS":
            yield _CharClassToken(pos, text)
            continue

        if kind == "QUANTIFIER":
            quantifier = _QUANTIFIER_MAP[text]
            yield _QuantifierToken(pos, text, quantifier)
            continue

        if kind == "ANCHOR":
            anchor = _ANCHOR_MAP[text]
            yield _AnchorToken(pos, text, anchor)
            continue

        if kind == "BAR":
            yield _BarToken(pos, text)
            continue

        if kind == "LPAREN":
            yield _LParenToken(pos, text)
            continue

        if kind == "RPAREN":
            yield _RParenToken(pos, text)
            continue

        if kind == "STRAY_BACKSLASH":
            raise SyntaxError(pos, "Stray backslash")

        if kind == "UNCLOSED_CLASS":
            raise SyntaxError(pos, "Unclosed character class")

        assert False

    pos  = len(cmake_regex_string)
    text = ""
    yield _EndOfInputToken(pos, text)


type _Token = (_CharToken | _WildcardToken | _CharClassToken | _QuantifierToken | _AnchorToken | _BarToken |
               _LParenToken | _RParenToken | _EndOfInputToken)

@dataclass(slots=True, frozen=True)
class _TokenBase:
    pos:  int
    text: str

@dataclass(slots=True, frozen=True)
class _CharToken(_TokenBase):
    char: str

@dataclass(slots=True, frozen=True)
class _WildcardToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _CharClassToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _QuantifierToken(_TokenBase):
    quantifier: _Quantifier

@dataclass(slots=True, frozen=True)
class _AnchorToken(_TokenBase):
    anchor: _r.Anchor.Type

@dataclass(slots=True, frozen=True)
class _BarToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _LParenToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _RParenToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _EndOfInputToken(_TokenBase):
    pass


class _Quantifier(enum.Enum):
        STAR  = 0
        PLUS  = 1
        QMARK = 2


_TOKEN_REGEX = re.compile(
    r"(?P<CHAR>\\.|[^\[\\*+?|()^$.])|"
    r"(?P<WILDCARD>\.)|"
    r"(?P<CLASS>\[\^?\]?[^\]]*\])|"
    r"(?P<QUANTIFIER>[*+?])|"
    r"(?P<ANCHOR>[\^$])|"
    r"(?P<BAR>\|)|"
    r"(?P<LPAREN>\()|"
    r"(?P<RPAREN>\))|"
    r"(?P<STRAY_BACKSLASH>\\$)|"
    r"(?P<UNCLOSED_CLASS>\[\^?\]?[^\]]*$)"
)


_QUANTIFIER_MAP = {
    "*": _Quantifier.STAR,
    "+": _Quantifier.PLUS,
    "?": _Quantifier.QMARK,
}

_ANCHOR_MAP = {
    "^": _r.Anchor.Type.BEGIN,
    "$": _r.Anchor.Type.END,
}
