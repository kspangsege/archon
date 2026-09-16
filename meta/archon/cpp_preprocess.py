from __future__ import annotations

import typing
import abc
import dataclasses
import collections.abc
import enum
import re
import unicodedata

import archon.base as _b
import archon.text_pos as _tp


# Tokenize C++ source code in accordance with a C++26 preprocessor.
#
# The input must be "newline normalized" (text mode).
#
# The C++ source code is assumed to use ASCII only.
#
def tokenize(input_: typing.TextIO, file_index: int, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
    return _tokenize(input_, file_index, tracker)


def parse(tokens: typing.Iterable[Token], error_handler: ErrorHandler) -> collections.abc.Iterator[Token | Directive]:
    return _parse(tokens, error_handler)


def preprocess(tokens: typing.Iterable[Token | Directive], macro_registry: dict[str, MacroDef],
               error_handler: ErrorHandler) -> collections.abc.Iterator[Token]:
    return _preprocess(tokens, macro_registry, error_handler)


# Scan token stream for macro invocations
#
def scan(context: ScanContext, error_handler: ErrorHandler) -> collections.abc.Iterator[Token]:
    return _scan(context, error_handler)


# Resolve UCNs and verify Unicode Normal Form C
#
def resolve_identifier_ucns(identifier: str, pos_info: PositionInfo, error_handler: ErrorHandler) -> str | None:
    return _resolve_identifier_ucns(identifier, pos_info, error_handler)


class ScanContext(abc.ABC):
    # Token sequence ends after an END_OF_INPUT token has been returned
    @abc.abstractmethod
    def next_token(self) -> Token:
        ...

    @abc.abstractmethod
    def lookup_macro(self, name: str) -> MacroDef | None:
        ...

    @abc.abstractmethod
    def handle_macro_invoc(self, name: str, definition: MacroDef, arguments: list[list[Token]] | None,
                           va_args: list[Token] | None) -> None:
        ...


@dataclasses.dataclass(slots=True, frozen=True)
class MacroDef:
    params: list[str] | None
    is_variadic: bool
    replacement: list[Token]


type Directive = DefineDirective | GenericDirective

@dataclasses.dataclass(slots=True, frozen=True)
class DirectiveBase:
    pos: Position

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
    def __call__(self, pos: Position, message: str, *args: typing.Any) -> None:
        ...


@dataclasses.dataclass(slots=True, frozen=True)
class Token:
    type_:    TokenType
    text:     str
    pos_info: PositionInfo


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
    PLACEMARKER    = enum.auto()
    END_OF_INPUT   = enum.auto()


@dataclasses.dataclass(slots=True, frozen=True)
class PositionInfo:
    base_position:    Position
    additional_parts: list[PositionInfoPart] | None = None

    def __call__(self, offset: int) -> Position:
        base_position = self.base_position
        offset_2 = offset
        if self.additional_parts:
            for part in self.additional_parts:
                if offset_2 < part.rel_offset:
                    break
                base_position = part.base_position
                offset_2 -= part.rel_offset
        return base_position.shift(offset_2)


@dataclasses.dataclass(slots=True, frozen=True)
class PositionInfoPart:
    rel_offset:    int
    base_position: Position


@dataclasses.dataclass(slots=True, frozen=True)
class Position:
    file_index:  int
    pos_in_file: int

    def shift(self, offset: int) -> Position:
        return Position(file_index = self.file_index, pos_in_file = self.pos_in_file + offset)

    def to_info(self) -> PositionInfo:
        return PositionInfo(self)








def _tokenize(input_: typing.TextIO, file_index: int, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
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
            position = Position(file_index = file_index, pos_in_file = pos)
            yield Token(token_type, text, position.to_info())
            i = j

    text = ""
    position = Position(file_index = file_index, pos_in_file = tracker.current())
    yield Token(TokenType.END_OF_INPUT, text, position.to_info())


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
    buffer_ = list[Token]()
    tokens_iter = iter(tokens)
    while True:
        token = next(tokens_iter)
        match state:
            case _ParseState.INITIAL:
                if token.type_ is TokenType.END_OF_INPUT:
                    yield token
                    break
                if token.type_ in _IS_SPACE:
                    yield token
                    continue
                if token.type_ is not TokenType.HASH:
                    yield token
                    state = _ParseState.GENERAL
                    continue
                directive_pos = token.pos_info(0)
                state = _ParseState.IN_DIRECTIVE
                continue
            case _ParseState.GENERAL:
                yield token
                if token.type_ is TokenType.END_OF_INPUT:
                    break
                if token.type_ is TokenType.NEWLINE:
                    state = _ParseState.INITIAL
                continue
            case _ParseState.IN_DIRECTIVE:
                if token.type_ is not TokenType.NEWLINE and token.type_ is not TokenType.END_OF_INPUT:
                    buffer_.append(token)
                    continue
                i = len(buffer_)
                while i > 0 and buffer_[i - 1].type_ in _IS_SPACE:
                    i -= 1
                end_pos = token.pos_info(0)
                if directive := _parse_directive(directive_pos, end_pos, buffer_[:i], error_handler):
                    yield directive
                yield from buffer_[i:]
                buffer_.clear()
                yield token
                if token.type_ is TokenType.END_OF_INPUT:
                    break
                state = _ParseState.INITIAL
                continue
        typing.assert_never(state)

    assert not buffer_


class _ParseState(enum.Enum):
    INITIAL      = enum.auto()
    GENERAL      = enum.auto()
    IN_DIRECTIVE = enum.auto()


def _parse_directive(pos: Position, end_pos: Position, tokens: list[Token],
                     error_handler: ErrorHandler) -> Directive | None:
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


def _parse_define_directive(pos: Position, end_pos: Position, tokens: list[Token], i: int,
                            error_handler: ErrorHandler) -> DefineDirective | None:
    while i < len(tokens) and tokens[i].type_ in _IS_SPACE:
        i += 1

    if i == len(tokens):
        error_handler(end_pos, "Missing macro name in #define directive")
        return None

    if tokens[i].type_ is not TokenType.IDENTIFIER:
        error_handler(tokens[i].pos_info(0), "Macro name must be an identifier")
        return None

    token: Token | None
    token = tokens[i]
    name = _resolve_identifier_ucns(token.text, token.pos_info, error_handler)
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
                token_pos = token.pos_info(0)
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
                        param = _resolve_identifier_ucns(token.text, token.pos_info, error_handler)
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


def _preprocess(tokens: typing.Iterable[Token | Directive], macro_registry: dict[str, MacroDef],
                error_handler: ErrorHandler) -> collections.abc.Iterator[Token]:
    # FIXME: Need to also parse and process preprocessor directives (_parse())        
    context = _PreprocessContext(tokens, macro_registry)
    return _scan(context, error_handler)


class _PreprocessContext(ScanContext):
    def __init__(self, tokens: typing.Iterable[Token | Directive], macro_registry: dict[str, MacroDef]) -> None:
        self._tokens         = iter(tokens)
        self._macro_registry = macro_registry

    @typing.override
    def next_token(self) -> Token:
                
        token = next(self._tokens)
        if isinstance(token, Token):
            return token
        assert False                

    @typing.override
    def lookup_macro(self, name: str) -> MacroDef | None:
        return self._macro_registry.get(name)        

    @typing.override
    def handle_macro_invoc(self, name: str, definition: MacroDef, arguments: list[list[Token]] | None,
                           va_args: list[Token] | None) -> None:
        assert False        


def _scan(context: ScanContext, error_handler: ErrorHandler) -> collections.abc.Iterator[Token]:
    @dataclasses.dataclass(slots=True, frozen=True)
    class ArgDelim:
        begin: int
        end:   int
        pos:   Position

    tokens:        list[Token]
    tokens_offset: int
    arg_delims:    list[ArgDelim]

    def record_arg(token: Token) -> int:
        begin = tokens_offset
        end   = len(tokens)
        arg_delims.append(ArgDelim(begin, end, token.pos_info(0)))
        return end

    def get_arg(begin: int, end: int) -> list[Token]:
        assert begin <= end <= len(arg_delims)
        assert end > 0
        if begin < end:
            i = arg_delims[begin].begin
            j = arg_delims[end - 1].end
            while i < j and tokens[i].type_ in _IS_SPACE:
                i += 1
            while i < j and tokens[j - 1].type_ in _IS_SPACE:
                j -= 1
            return tokens[i:j]
        return []

    def get_arg_or_placemarker(begin: int, end: int) -> list[Token]:
        assert begin < len(arg_delims)
        arg = get_arg(begin, end)
        if arg:
            return arg
        text = ""
        pos = arg_delims[min(begin, end - 1)].pos
        token = Token(TokenType.PLACEMARKER, text, pos.to_info())
        return [token]

    token = context.next_token()
    while True:
        if token.type_ is TokenType.END_OF_INPUT:
            yield token
            break
        if token.type_ is not TokenType.IDENTIFIER:
            yield token
            token = context.next_token()
            continue
        name = _resolve_identifier_ucns(token.text, token.pos_info, error_handler)
        if name is None:
            yield token
            token = context.next_token()
            continue
        definition = context.lookup_macro(name)
        if not definition:
            yield token
            token = context.next_token()
            continue
        if definition.params is None:
            context.handle_macro_invoc(name, definition, None, None) # Object-like macro
            token = context.next_token()
            continue
        tokens = [token]
        while True:
            token = context.next_token()
            if token.type_ not in _IS_SPACE:
                break
            tokens.append(token)
        if token.type_ is not TokenType.PUNCT or token.text != "(":
            yield from tokens
            continue
        tokens = []
        tokens_offset = 0
        arg_delims = []
        paren_level = 0
        while True:
            token = context.next_token()
            if token.type_ is TokenType.END_OF_INPUT:
                pos = token.pos_info(0)
                error_handler(pos, "Expected closing parenthesis or comma in invocation of function-like macro %s",
                              _b.clamped_quote(name, 64))
                break
            if token.type_ is TokenType.PUNCT:
                if token.text == ",":
                    if paren_level == 0:
                        tokens_offset = record_arg(token) + 1
                elif token.text == "(":
                    paren_level += 1
                elif token.text == ")":
                    if paren_level == 0:
                        record_arg(token)
                        n = len(definition.params)
                        if len(arg_delims) < n:
                            pos = arg_delims[-1].pos
                            error_handler(pos, "Too few arguments in invocation of function-like macro %s",
                                          _b.clamped_quote(name, 64))
                            token = context.next_token()
                            break
                        if not definition.is_variadic and len(arg_delims) > n and (len(arg_delims) > 1 or
                                                                                   get_arg(0, 1)):
                            if n > 0:
                                pos = arg_delims[n - 1].pos
                            else:
                                arg = get_arg(0, 1)
                                pos = arg[0].pos_info(0) if arg else arg_delims[0].pos
                            error_handler(pos, "Too many arguments in invocation of function-like macro %s",
                                          _b.clamped_quote(name, 64))
                            token = context.next_token()
                            break
                        arguments = list[list[Token]]()
                        for i in range(n):
                            arg = get_arg_or_placemarker(i, i + 1)
                            arguments.append(arg)
                        va_args = None
                        if definition.is_variadic:
                            va_args = get_arg_or_placemarker(n, len(arg_delims))
                        context.handle_macro_invoc(name, definition, arguments, va_args) # Function-like macro
                        token = context.next_token()
                        break
                    paren_level -= 1
            tokens.append(token)


_IS_SPACE = {TokenType.NEWLINE, TokenType.WHITESPACE, TokenType.LINE_COMMENT, TokenType.BLOCK_COMMENT}


def _resolve_identifier_ucns(identifier: str, pos_info: PositionInfo, error_handler: ErrorHandler) -> str | None:
    class Error(Exception):
        pass
    def replace(m: re.Match[str]) -> str:
        which = m.lastindex
        assert which is not None and 1 <= which <= 4
        text = m.group(which)
        offset = m.start()

        if which == 4:
            try:
                return unicodedata.lookup(text)
            except KeyError:
                error_handler(pos_info(offset), "Invalid Unicode character name in UCN: %s",
                              _b.clamped_quote(text, 64))
                raise Error from None

        code_point = int(text, 16)
        if 0xD800 <= code_point < 0xE000:
            error_handler(pos_info(offset), "Illegal surrogate code point in UCN: U+%04X" % code_point)
            raise Error from None

        try:
            return chr(code_point)
        except ValueError:
            error_handler(pos_info(offset), f"UCN code point out of range: U+%04X" % code_point)
            raise Error from None

    try:
        resolved = _UCN_RESOLVE_REGEX.sub(replace, identifier)
    except Error:
        return None

    if unicodedata.is_normalized("NFC", resolved):
        return resolved
    error_handler(pos_info(0), "Identifier does not conform to Unicode NFC")
    return None


_UCN_RESOLVE_REGEX = re.compile(r"\\u([0-9A-Fa-f]{4})|"
                                r"\\u\{([0-9A-Fa-f]+)\}|"
                                r"\\U([0-9A-Fa-f]{8})|"
                                r"\\N\{([^}\n]+)\}")
