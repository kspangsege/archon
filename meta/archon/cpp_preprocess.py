from __future__ import annotations

import typing
import dataclasses
import collections.abc
import enum
import re
import unicodedata

import archon.base as _b
import archon.text_pos as _tp


def parse(input_: typing.TextIO, tracker: _tp.TextPosTracker,
          error_handler: ErrorHandler) -> collections.abc.Iterator[Token | Directive]:
    return _parse(tokenize(input_, tracker), error_handler)


# Tokenize C++ source code in accordance with a C++26 preprocessor.
#
# The input must be "newline normalized" (text mode).
#
# The C++ source code is assumed to use ASCII only.
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
    END_OF_INPUT   = enum.auto()


def parse_from_tokens(tokens: typing.Iterable[Token],
                      error_handler: ErrorHandler) -> collections.abc.Iterator[Token | Directive]:
    return _parse(tokens, error_handler)


type Directive = DefineDirective | GenericDirective

@dataclasses.dataclass(slots=True, frozen=True)
class DirectiveBase:
    pos: int

@dataclasses.dataclass(slots=True, frozen=True)
class DefineDirective(DirectiveBase):
    name:        str
    params:      list[str] | None
    is_variadic: bool
    replacement: list[Token]

@dataclasses.dataclass(slots=True, frozen=True)
class GenericDirective(DirectiveBase):
    tokens: list[Token]


class ErrorHandler(typing.Protocol):
    def __call__(self, pos: int, message: str, *args: typing.Any) -> None:
        ...


# Resolve UCNs and verify Unicode Normal Form C
#
def resolve_identifier_ucns(identifier: str, pos: int, error_handler: ErrorHandler) -> str | None:
    return _resolve_identifier_ucns(identifier, pos, error_handler)








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
        state = _TokenizeState.INITIAL
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
            if token_type not in _IS_SPACE:
                if expect_header and line[i] in '<"':
                    if m := _HEADER_REGEX.match(line, i):
                        token_type = TokenType.HEADER_NAME
                        text = m.group()
                        j = m.end()
                expect_header = False
                match state:
                    case _TokenizeState.INITIAL:
                        if token_type is TokenType.HASH:
                            state = _TokenizeState.AFTER_HASH
                        else:
                            state = _TokenizeState.NOT_DIRECTIVE
                            if token_type is TokenType.IDENTIFIER and text == "import":
                                expect_header = True
                    case _TokenizeState.NOT_DIRECTIVE:
                        if token_type is TokenType.IDENTIFIER and text == "import":
                            expect_header = True
                    case _TokenizeState.AFTER_HASH:
                        state = _TokenizeState.IN_DIRECTIVE
                        if token_type is TokenType.IDENTIFIER:
                            if text in {"include", "embed"}:
                                expect_header = True
                            elif text in {"if", "elif"}:
                                state = _TokenizeState.IN_CONDITION
                    case _TokenizeState.IN_DIRECTIVE:
                        pass
                    case _TokenizeState.IN_CONDITION:
                        if token_type is TokenType.IDENTIFIER and text in {"__has_include", "__has_embed"}:
                            state = _TokenizeState.AFTER_HAS_INCLUDE
                    case _TokenizeState.AFTER_HAS_INCLUDE:
                        if token_type is TokenType.PUNCT and text == "(":
                            expect_header = True
                        state = _TokenizeState.IN_CONDITION
                    case _:
                        typing.assert_never(state)
            yield Token(token_type, text, pos)
            i = j

    yield Token(TokenType.END_OF_INPUT, "", tracker.current())


class _TokenizeState(enum.Enum):
    INITIAL           = enum.auto()
    NOT_DIRECTIVE     = enum.auto()
    AFTER_HASH        = enum.auto()
    IN_DIRECTIVE      = enum.auto()
    IN_CONDITION      = enum.auto()
    AFTER_HAS_INCLUDE = enum.auto()


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


def _parse(tokens: typing.Iterable[Token], error_handler: ErrorHandler) -> collections.abc.Iterator[Token | Directive]:
    state = _ParseState.INITIAL
    directive_pos = 0
    buffer_ = list[Token]()
    for token in tokens:
        match state:
            case _ParseState.INITIAL:
                if token.type_ in _IS_SPACE_OR_END_OF_LINE:
                    yield token
                    continue
                if token.type_ is not TokenType.HASH:
                    yield token
                    state = _ParseState.GENERAL
                    continue
                directive_pos = token.pos
                state = _ParseState.IN_DIRECTIVE
                continue
            case _ParseState.GENERAL:
                yield token
                if token.type_ in _IS_END_OF_LINE:
                    state = _ParseState.INITIAL
                continue
            case _ParseState.IN_DIRECTIVE:
                if token.type_ not in _IS_END_OF_LINE:
                    buffer_.append(token)
                    continue
                i = len(buffer_)
                while i > 0 and buffer_[i - 1].type_ in _IS_SPACE:
                    i -= 1
                end_pos = token.pos
                if dir_ := _parse_directive(directive_pos, end_pos, buffer_[:i], error_handler):
                    yield dir_
                yield from buffer_[i:]
                buffer_.clear()
                yield token
                state = _ParseState.INITIAL
                continue
        typing.assert_never(state)

    assert not buffer_


class _ParseState(enum.Enum):
    INITIAL      = enum.auto()
    GENERAL      = enum.auto()
    IN_DIRECTIVE = enum.auto()


def _parse_directive(pos: int, end_pos: int, tokens: list[Token], error_handler: ErrorHandler) -> Directive | None:
    i = 0
    while i < len(tokens) and tokens[i].type_ in _IS_SPACE:
        i += 1
    if i == len(tokens):
        return None # Null directive
    if tokens[i].type_ is TokenType.IDENTIFIER:
        name = tokens[i].text
        match name:
            case "define":
                return _parse_define_directive(pos, end_pos, tokens, i + 1, error_handler)
    return GenericDirective(pos, tokens[i:])


def _parse_define_directive(pos: int, end_pos: int, tokens: list[Token], i: int,
                            error_handler: ErrorHandler) -> DefineDirective | None:
    while i < len(tokens) and tokens[i].type_ in _IS_SPACE:
        i += 1

    if i == len(tokens):
        error_handler(end_pos, "Missing macro name in #define directive")
        return None

    if tokens[i].type_ is not TokenType.IDENTIFIER:
        error_handler(tokens[i].pos, "Macro name must be an identifier")
        return None

    token: Token | None
    token = tokens[i]
    name = _resolve_identifier_ucns(token.text, token.pos, error_handler)
    if name is None:
        return None
    i += 1

    params: list[str] | None = None
    is_variadic = False

    if i < len(tokens) and tokens[i].type_ is TokenType.PUNCT and tokens[i].text == "(":
        params = []
        i += 1
        expect_param = True
        while True:
            if i < len(tokens):
                token = tokens[i]
                token_pos = token.pos
                i += 1
                if token.type_ in _IS_SPACE:
                    continue
            else:
                token = None
                token_pos = end_pos
            if expect_param:
                expect_param = False
                if token:
                    if token.type_ is TokenType.IDENTIFIER:
                        param = _resolve_identifier_ucns(token.text, token.pos, error_handler)
                        if param is None:
                            return None
                        params.append(param)
                        continue
                    if token.type_ is TokenType.PUNCT:
                        if token.text == "...":
                            is_variadic = True
                            continue
                        if token.text == ")" and not params and not is_variadic:
                            break
                if not params and not is_variadic:
                    error_handler(token_pos, "Expected parameter name or closing parenthesis after opening "
                                  "parenthesis in macro parameter list")
                else:
                    error_handler(token_pos, "Expected parameter name or ellipsis after comma in macro parameter list")
                return None
            if is_variadic:
                if token and token.type_ is TokenType.PUNCT and token.text == ")":
                    break
                error_handler(token_pos, "Expected closing parenthesis after ellipsis in macro parameter list")
                return None
            if token and token.type_ is TokenType.PUNCT:
                if token.text == ")":
                    break
                if token.text == ",":
                    expect_param = True
                    continue
            error_handler(token_pos, "Expected comma or closing parenthesis after parameter name in macro parameter "
                          "list")
            return None

    while i < len(tokens) and tokens[i].type_ in _IS_SPACE:
        i += 1
    replacement = tokens[i:]
    return DefineDirective(pos, name, params, is_variadic, replacement)


_IS_SPACE = {TokenType.NEWLINE, TokenType.WHITESPACE, TokenType.LINE_COMMENT, TokenType.BLOCK_COMMENT}

_IS_END_OF_LINE = {TokenType.NEWLINE, TokenType.END_OF_INPUT}

_IS_SPACE_OR_END_OF_LINE = _IS_SPACE | _IS_END_OF_LINE


def _resolve_identifier_ucns(identifier: str, pos: int, error_handler: ErrorHandler) -> str | None:
    class Error(Exception):
        pass
    def replace(m: re.Match[str]) -> str:
        which = m.lastindex
        assert which is not None and 1 <= which <= 4
        text = m.group(which)
        subpos = pos + m.start()

        if which == 4:
            try:
                return unicodedata.lookup(text)
            except KeyError:
                error_handler(subpos, "Invalid Unicode character name in UCN: %s", _b.clamped_quote(text, 64))
                raise Error from None

        code_point = int(text, 16)
        if 0xD800 <= code_point < 0xE000:
            error_handler(subpos, "Illegal surrogate code point in UCN: U+%04X" % code_point)
            raise Error from None

        try:
            return chr(code_point)
        except ValueError:
            error_handler(subpos, f"UCN code point out of range: U+%04X" % code_point)
            raise Error from None

    try:
        resolved = _UCN_RESOLVE_REGEX.sub(replace, identifier)
    except Error:
        return None

    if unicodedata.is_normalized("NFC", resolved):
        return resolved
    error_handler(pos, "Identifier does not conform to Unicode NFC")
    return None


_UCN_RESOLVE_REGEX = re.compile(r"\\u([0-9A-Fa-f]{4})|"
                                r"\\u\{([0-9A-Fa-f]+)\}|"
                                r"\\U([0-9A-Fa-f]{8})|"
                                r"\\N\{([^}\n]+)\}")
