from __future__ import annotations

import typing
import dataclasses
import collections.abc
import enum
import re

import archon.text_pos as _tp


# Tokenize C++ source code in accordance with a C++26 preprocessor.
#
# The input must be "newline normalized" (text mode).
#
# The C++ source code as assumed to use ASCII only.
#
def tokenize(input_: typing.TextIO, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
    return _tokenize(input_, tracker)


@dataclasses.dataclass(slots=True, frozen=True)
class Token:
    type_: TokenType
    text:  str
    pos:   int


class TokenType(enum.Enum):
    NEWLINE        = enum.auto()
    WHITESPACE     = enum.auto()
    LINE_COMMENT   = enum.auto()
    BLOCK_COMMENT  = enum.auto()
    HASH_HASH      = enum.auto()
    HASH           = enum.auto()
    RAW_STRING_LIT = enum.auto()
    STRING_LIT     = enum.auto()
    CHAR_LIT       = enum.auto()
    NUMBER         = enum.auto()
    IDENTIFIER     = enum.auto()
    PUNCT          = enum.auto()
    BAD_CHAR       = enum.auto()
    HEADER_NAME    = enum.auto()








def _tokenize(input_: typing.TextIO, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
    line_iter = _logical_lines(input_, tracker)

    line:     str
    base_pos: int
    j:        int
    text:     str

    def consume(closing_marker: str, udl_suffix: bool) -> None:
        nonlocal line, base_pos, j, text
        parts = [text]
        while True:
            k = line.find(closing_marker, j)
            if k != -1:
                k += len(closing_marker)
                if udl_suffix:
                    if m := _IDENT_REGEX.match(line, k):
                        k = m.end()
                parts.append(line[j:k])
                j = k
                break
            parts.append(line[j:])
            line_obj = next(line_iter, None)
            j = 0
            if not line_obj:
                line = ""
                break
            line = line_obj.text
            base_pos = line_obj.pos
        text = "".join(parts)

    for line_obj in line_iter:
        line = line_obj.text
        base_pos = line_obj.pos
        state = _State.INITIAL
        expect_header = False
        i = 0
        while i < len(line):
            m = _TOKEN_REGEX.match(line, i)
            assert m
            group_name = m.lastgroup
            assert group_name is not None
            token_type = TokenType[group_name]
            text = m.group()
            j = m.end()
            pos = base_pos + i
            match token_type:
                case TokenType.BLOCK_COMMENT:
                    consume(closing_marker="*/", udl_suffix=False)
                case TokenType.RAW_STRING_LIT:
                    k = text.find('"') + 1
                    delim = text[k:-1]
                    consume(closing_marker=')%s"' % delim, udl_suffix=True)
            if token_type not in _SKIP_TOKENS:
                if expect_header and line[i] in '<"':
                    if m := _HEADER_REGEX.match(line, i):
                        token_type = TokenType.HEADER_NAME
                        text = m.group()
                        j = m.end()
                expect_header = False
                match state:
                    case _State.INITIAL:
                        if token_type is TokenType.HASH:
                            state = _State.AFTER_HASH
                        else:
                            state = _State.NOT_DIRECTIVE
                            if token_type is TokenType.IDENTIFIER and text == "import":
                                expect_header = True
                    case _State.NOT_DIRECTIVE:
                        if token_type is TokenType.IDENTIFIER and text == "import":
                            expect_header = True
                    case _State.AFTER_HASH:
                        state = _State.IN_DIRECTIVE
                        if token_type is TokenType.IDENTIFIER:
                            if text in {"include", "embed"}:
                                expect_header = True
                            elif text in {"if", "elif"}:
                                state = _State.IN_CONDITION
                    case _State.IN_DIRECTIVE:
                        pass
                    case _State.IN_CONDITION:
                        if token_type is TokenType.IDENTIFIER and text in {"__has_include", "__has_embed"}:
                            state = _State.AFTER_HAS_INCLUDE
                    case _State.AFTER_HAS_INCLUDE:
                        if token_type is TokenType.PUNCT and text == "(":
                            expect_header = True
                        state = _State.IN_CONDITION
                    case _:
                        typing.assert_never(state)
            yield Token(token_type, text, pos)
            i = j


class _State(enum.Enum):
    INITIAL           = enum.auto()
    NOT_DIRECTIVE     = enum.auto()
    AFTER_HASH        = enum.auto()
    IN_DIRECTIVE      = enum.auto()
    IN_CONDITION      = enum.auto()
    AFTER_HAS_INCLUDE = enum.auto()


_SKIP_TOKENS = {TokenType.NEWLINE, TokenType.WHITESPACE, TokenType.LINE_COMMENT, TokenType.BLOCK_COMMENT}

_HEADER_REGEX = re.compile(r'<[^>\n]*>|"[^"\n]*"')

_UCN_REGEX_STRING = r"(?:\\u[0-9A-Fa-f]{4}|\\u\{[0-9A-Fa-f]+\}|\\U[0-9A-Fa-f]{8}|\\N\{[^}\n]+\})"

_IDENT_REGEX_STRING = r"(?:(?:[A-Za-z_]|%s)(?:\w+|%s)*)" % (_UCN_REGEX_STRING, _UCN_REGEX_STRING)

_TOKEN_REGEX = re.compile("|".join("(?P<%s>%s)" % (name, expr) for name, expr in [
    ("NEWLINE",        r"\n"),
    ("WHITESPACE",     r"[ \t\f\v]+"),
    ("LINE_COMMENT",   r"//[^\n]*"),
    ("BLOCK_COMMENT",  r"/\*"),
    ("HASH",           r"#|%:"),
    ("HASH_HASH",      r"##|%:%:"),
    ("RAW_STRING_LIT", r'(?:u8|u|U|L)?R"[^()\\\s]*\('),
    ("STRING_LIT",     r'(?:u8|u|U|L)?"(?:\\.|[^"\\\n])*("(?:%s)?|$)' % _IDENT_REGEX_STRING),
    ("CHAR_LIT",       r"(?:u8|u|U|L)?'(?:\\.|[^'\\\n])*('(?:%s)?|$)" % _IDENT_REGEX_STRING),
    ("NUMBER",         r"(?:\d|\.\d)(?:\.|[eEpP][+-]|%s|\'?\w)*" % _UCN_REGEX_STRING),
    ("IDENTIFIER",     _IDENT_REGEX_STRING),
    ("PUNCT",          (r"::|\.\.\.|->\*|->|\+\+|--|<<=|>>=|<<|>>|<=>|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|\/=|%=|&=|\^=|\|=|"
                        r"\.\*|<%|%>|<:(?:(?!:)|(?=::|:>))|:>|[{}()\[\];,.?:+\-*%^&|~!=<>]")),
    ("BAD_CHAR",       r"."),
]), re.ASCII | re.DOTALL)

_IDENT_REGEX = re.compile(_IDENT_REGEX_STRING, re.ASCII)


def _logical_lines(input_: typing.TextIO, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[_Line]:
    pos = 0
    lines = list[str]()
    for line in input_:
        continuation = "\\\n"
        if line.endswith(continuation):
            i = len(line) - len(continuation)
            lines.append(line[:i])
            tracker.advance(i)
            tracker.new_line()
        else:
            lines.append(line)
            tracker.advance(len(line))
            tracker.new_line()
            yield _Line(pos, "".join(lines))
            lines.clear()
            pos = tracker.current()
    if lines:
        yield _Line(pos, "".join(lines))


@dataclasses.dataclass(slots=True, frozen=True)
class _Line:
    pos:  int
    text: str
