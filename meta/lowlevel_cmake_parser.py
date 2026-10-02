from __future__ import annotations
from typing import Protocol, Any
from collections.abc import Iterable, Iterator
from dataclasses import dataclass

import enum
import re
import pathlib

import base


def parse(path: pathlib.Path, error_handler: ErrorHandler) -> Iterator[Protoinvoc]:
    return _parse(path, error_handler)


class ErrorHandler(Protocol):
    def __call__(self, line_no: int, message: str, *args: Any) -> None:
        ...


class Protoinvoc:
    def __init__(self, name: str, arguments: Iterable[Protoargument], line_no: int, opening_line_no: int,
                 closing_line_no: int) -> None:
        self.name = name
        self.arguments: list[Protoargument] = list(arguments)
        self.line_no = line_no
        self.opening_line_no = opening_line_no
        self.closing_line_no = closing_line_no


class Protoargument:
    def __init__(self, expr: Expr, was_quoted_or_bracketed: bool, raw_text: str, line_no: int) -> None:
        self.expr = expr
        self.was_quoted_or_bracketed = was_quoted_or_bracketed
        self.raw_text = raw_text
        self.line_no = line_no


type Expr = StringExpr | CompositeExpr | VariableExpansionExpr

@dataclass(slots=True)
class StringExpr:
    string: str

@dataclass(slots=True)
class CompositeExpr:
    parts: list[Expr]

    def __init__(self, parts: Iterable[Expr]) -> None:
        self.parts = list(parts)

@dataclass(slots=True)
class VariableExpansionExpr:
    class Type(enum.Enum):
        REGULAR = 0
        ENV     = 1
        CACHE   = 2
    type_:     Type
    name_expr: Expr


def _parse(path: pathlib.Path, error_handler: ErrorHandler) -> Iterator[Protoinvoc]:
    class State(enum.Enum):
        INITIAL   = 0
        HAVE_NAME = 1
        IN_ARGS   = 2

    state = State.INITIAL
    level: int
    have_args: bool

    name: str = ""
    args: list[Protoargument] = []
    invalid: bool = False
    invoc_line_no: int = 0
    opening_line_no: int = 0

    def start_invoc(line_no: int) -> None:
        nonlocal name, args, invalid, invoc_line_no
        name = ""
        args = []
        invalid = False
        invoc_line_no = line_no
        opening_lin_no = 0

    for token in _tokenize(path, error_handler):
        if state == State.INITIAL:
            if isinstance(token, _UnquotedToken):
                start_invoc(token.line_no)
                if not re.fullmatch(r"[A-Za-z_][0-9A-Za-z_]*", token.value):
                    error_handler(token.line_no, "Invalid command name")
                    invalid = True
                name = token.value
                state = State.HAVE_NAME
                continue
            if isinstance(token, _LParenToken):
                start_invoc(token.line_no)
                error_handler(token.line_no, "Missing command name")
                invalid = True
                state = State.IN_ARGS
                level = 0
                have_args = False
                opening_line_no = token.line_no
                continue
            if isinstance(token, _RParenToken):
                error_handler(token.line_no, "Stray closing parenthesis")
                continue
            start_invoc(token.line_no)
            error_handler(token.line_no, "Invalid command name")
            invalid = True
            state = State.HAVE_NAME
            continue

        if state == State.HAVE_NAME:
            if isinstance(token, _LParenToken):
                state = State.IN_ARGS
                level = 0
                have_args = False
                opening_line_no = token.line_no
                continue
            if isinstance(token, _RParenToken):
                error_handler(token.line_no, "Stray closing parenthesis")
                state = State.INITIAL
                continue
            error_handler(token.line_no, "Stray command argument")
            invalid = True
            continue

        expr: Expr
        if state == State.IN_ARGS:
            require_preceding_whitespace = True
            was_quoted_or_bracketed = False
            reset_have_args = False
            if isinstance(token, _LParenToken):
                level += 1
                reset_have_args = True
                expr = StringExpr(token.text)
            elif isinstance(token, _RParenToken):
                assert level >= 0
                if level == 0:
                    if not invalid:
                        closing_line_no = token.line_no
                        yield Protoinvoc(name, args, invoc_line_no, opening_line_no, closing_line_no)
                    state = State.INITIAL
                    continue
                level -= 1
                expr = StringExpr(token.text)
                require_preceding_whitespace = False
            elif isinstance(token, _UnquotedToken):
                expr = _parse_string(token.value, error_handler, token.line_no)
            elif isinstance(token, _QuotedToken):
                expr = _parse_string(token.value, error_handler, token.line_no)
                was_quoted_or_bracketed = True
            elif isinstance(token, _BracketToken):
                expr = StringExpr(token.value)
                was_quoted_or_bracketed = True
            else:
                assert False
            if have_args and require_preceding_whitespace and not token.preceded_by_whitespace:
                error_handler(token.line_no, "Missing whitespace between arguments")
            args.append(Protoargument(expr, was_quoted_or_bracketed, token.text, token.line_no))
            have_args = True
            if reset_have_args:
                have_args = False

    if state != State.INITIAL:
        error_handler(invoc_line_no, "Unterminated command invocation")


def _tokenize(path: pathlib.Path, error_handler: ErrorHandler) -> Iterator[_Token]:
    line_no = 1
    input_ = ""
    eof = False
    prev_token_is_whitespace = False
    with open(path, "r") as file_:
        while True:
            line = file_.readline()
            if line:
                input_ += line
            else:
                eof = True
            pos = 0
            while pos < len(input_):
                m = _TOKEN_REGEX.match(input_, pos)
                if m:
                    token_text = m.group(0)
                    token_line_no = line_no
                    line_no += token_text.count("\n")
                    pos = m.end()

                    if m.group("SPACE") or m.group("COMMENT"):
                        prev_token_is_whitespace = True
                        continue

                    preceded_by_whitespace = prev_token_is_whitespace
                    prev_token_is_whitespace = False

                    if m.group("UNQUOTED"):
                        value = token_text
                        yield _UnquotedToken(value, token_text, preceded_by_whitespace, token_line_no)
                        continue

                    if m.group("QUOTED"):
                        value = token_text[1:-1]
                        yield _QuotedToken(value, token_text, preceded_by_whitespace, token_line_no)
                        continue

                    if m.group("LPAREN"):
                        yield _LParenToken(token_text, preceded_by_whitespace, token_line_no)
                        continue

                    if m.group("RPAREN"):
                        yield _RParenToken(token_text, preceded_by_whitespace, token_line_no)
                        continue

                    if m.group("BRACKET"):
                        eqs = m.group("eqs2")
                        prefix_len = 2 + len(eqs)
                        suffix_len = 2 + len(eqs)
                        if len(token_text) > prefix_len + suffix_len and token_text[prefix_len] == "\n":
                            prefix_len += 1
                        value = token_text[prefix_len:-suffix_len]
                        yield _BracketToken(value, token_text, preceded_by_whitespace, token_line_no)
                        continue

                    assert False

                if not eof:
                    break

                m = _ERROR_REGEX.match(input_, pos)
                assert m
                error_text = m.group(0)
                error_line_no = line_no
                line_no += error_text.count("\n")
                pos = m.end()
                prev_token_is_whitespace = False

                if m.group("UNTERM_COMMENT"):
                    error_handler(error_line_no, "Unterminated bracketed comment")
                    continue

                if m.group("UNTERM_BRACKET"):
                    error_handler(error_line_no, "Unterminated bracket string")
                    continue

                if m.group("UNTERM_QUOTED"):
                    error_handler(error_line_no, "Unterminated quoted string")
                    continue

                if m.group("UNEXPECTED_CHAR"):
                    error_handler(error_line_no, "Unexpected character (%s)", base.quote(error_text))
                    continue

                assert False

            if eof:
                break
            input_ = input_[pos:]


def _parse_string(string: str, error_handler: ErrorHandler, line_no: int) -> Expr:
    def unescape(char: str) -> str:
        special = {
            ";":  "\\;", # CMake only unescapes `\;` during unqothed argument expansion
            "n":  "\n",
            "t":  "\t",
            "r":  "\r",
            "\n": "",
        }
        if char.isalnum() and char not in special:
            error_handler(line_no, "Invalid escape sequence `\\%s`", char)
        return special.get(char, char)

    class State(enum.Enum):
        ROOT       = 0
        IN_VAREXP  = 1
        IN_EVAREXP = 2
        IN_CVAREXP = 3

    stack: list[tuple[State, list[Expr]]] = []
    state = State.ROOT
    parts: list[Expr] = []

    def append_literal(string: str) -> None:
        if not parts or not isinstance(parts[-1], StringExpr):
            parts.append(StringExpr(string))
        else:
            parts[-1].string += string

    def get_expr() -> Expr:
        if not parts:
            return StringExpr("")
        if len(parts) == 1:
            return parts[0]
        return CompositeExpr(parts)

    pos = 0
    for m in _STRING_REGEX.finditer(string):
        if m.start() > pos:
            append_literal(string[pos:m.start()])
        pos = m.end()
        if m.group("ESCAPE"):
            append_literal(unescape(string[pos - 1]))
            continue

        if m.group("VAREXP"):
            stack.append((state, parts))
            state = State.IN_VAREXP
            parts = []
            continue

        if m.group("EVAREXP"):
            stack.append((state, parts))
            state = State.IN_EVAREXP
            parts = []
            continue

        if m.group("CVAREXP"):
            stack.append((state, parts))
            state = State.IN_CVAREXP
            parts = []
            continue

        if m.group("RBRACE"):
            if state == State.ROOT:
                append_literal("}")
                continue
            if state == State.IN_VAREXP:
                type_ = VariableExpansionExpr.Type.REGULAR
            elif state == State.IN_EVAREXP:
                type_ = VariableExpansionExpr.Type.ENV
            elif state == State.IN_CVAREXP:
                type_ = VariableExpansionExpr.Type.CACHE
            else:
                assert False
            name_expr = get_expr()
            state, parts = stack.pop()
            parts.append(VariableExpansionExpr(type_, name_expr))
            continue

        assert False

    if len(string) > pos:
        append_literal(string[pos:])

    while state != State.ROOT:
        error_handler(line_no, "Unclosed variable expansion")
        assert stack
        state, parts = stack.pop()

    return get_expr()


class _Token:
    def __init__(self, text: str, preceded_by_whitespace: bool, line_no: int) -> None:
        self.text = text
        self.preceded_by_whitespace = preceded_by_whitespace
        self.line_no = line_no

class _UnquotedToken(_Token):
    def __init__(self, value: str, text: str, preceded_by_whitespace: bool, line_no: int) -> None:
        _Token.__init__(self, text, preceded_by_whitespace, line_no)
        self.value = value

class _QuotedToken(_Token):
    def __init__(self, value: str, text: str, preceded_by_whitespace: bool, line_no: int) -> None:
        _Token.__init__(self, text, preceded_by_whitespace, line_no)
        self.value = value

class _LParenToken(_Token):
    pass

class _RParenToken(_Token):
    pass

class _BracketToken(_Token):
    def __init__(self, value: str, text: str, preceded_by_whitespace: bool, line_no: int) -> None:
        _Token.__init__(self, text, preceded_by_whitespace, line_no)
        self.value = value


_TOKEN_REGEX = re.compile(
    r'(?P<COMMENT>#\[(?P<eqs1>=*)\[.*?\](?P=eqs1)\]|#(?!\[=*\[)[^\n]*)|'
    r'(?P<BRACKET>\[(?P<eqs2>=*)\[.*?\](?P=eqs2)\])|'
    r'(?P<QUOTED>"(?:\\.|[^"\\])*")|'
    r'(?P<LPAREN>\()|'
    r'(?P<RPAREN>\))|'
    r'(?P<SPACE>\s+)|'
    r'(?P<UNQUOTED>(?!\[=*\[)(?:\\[^\n]|[^\s()"#\\])+)',
    re.DOTALL,
)

_ERROR_REGEX = re.compile(
    r'(?P<UNTERM_COMMENT>#\[=*\[.*)|'
    r'(?P<UNTERM_BRACKET>\[=*\[.*)|'
    r'(?P<UNTERM_QUOTED>".*)|'
    r'(?P<UNEXPECTED_CHAR>.)',
    re.DOTALL,
)

_STRING_REGEX = re.compile(
    r'(?P<ESCAPE>\\.)|'
    r'(?P<VAREXP>\$\{)|'
    r'(?P<EVAREXP>\$ENV\{)|'
    r'(?P<CVAREXP>\$CACHE\{)|'
    r'(?P<RBRACE>\})',
    re.DOTALL,
)
