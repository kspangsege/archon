from __future__ import annotations

import typing
import abc
import dataclasses
import collections.abc
import enum
import re
import unicodedata
import io

import archon.base as _b
import archon.text_pos as _tp


def preprocess(input_: typing.TextIO, file_index: int, tracker: _tp.TextPosTracker,
               macro_registry: dict[str, MacroDef], error_handler: ErrorHandler) -> collections.abc.Iterator[Token]:
    elements = parse(input_, file_index, tracker, error_handler)
    return _preprocess(elements, macro_registry, error_handler)


def parse(input_: typing.TextIO, file_index: int, tracker: _tp.TextPosTracker,
          error_handler: ErrorHandler) -> collections.abc.Iterator[Token | Directive]:
    tokens = _tokenize(input_, file_index, tracker)
    return _parse(tokens, error_handler)


# Tokenize C++ source code in accordance with a C++26 preprocessor.
#
# The input must be "newline normalized" (text mode).
#
# The C++ source code is assumed to use the basic character set only. UCNs above ASCII are
# allowed.
#
# The output is a stream of preprocessing tokens not including token type `HIDDEN_IDENT`. At
# the end of input, an `END_OF_INPUT` token is generated.
#
def tokenize(input_: typing.TextIO, file_index: int, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
    return _tokenize(input_, file_index, tracker)


# Parse stream of C++26 preprocessing tokens, such as the one produced by `tokenizer()`,
# into a mixed stream of preprocessor directives and preprocessing tokens. Input tokens that
# do not participate in forming preprocessor directives are passed unchanged to the
# output. No other tokens will appear in the output. End of input and output is marked by an
# `END_OF_INPUT` token.
#
def parse_tokens(tokens: typing.Iterable[Token], error_handler: ErrorHandler) -> collections.abc.Iterator[Element]:
    return _parse(tokens, error_handler)


# Preprocess pre-parsed token stream in accordance with C++26. Input is a mixed stream of
# preprocessor directives and preprocessing tokens, such as the stream produced by
# `parse_tokens()`. Output is a stream of preprocessing tokens not including token types
# `HEADER_NAME` and `HIDDEN_IDENT`, and also not including any of the error tokens. End of
# input and output is marked by an `END_OF_INPUT` token.
#
def preprocess_elements(elements: typing.Iterable[Element], macro_registry: dict[str, MacroDef],
                        error_handler: ErrorHandler) -> collections.abc.Iterator[Token]:
    return _preprocess(elements, macro_registry, error_handler)


# Resolve UCNs and verify Unicode Normal Form C
#
def unpack_identifier(text: str, pos: Position, synthetic: bool, error_handler: ErrorHandler) -> str | None:
    unpacker = _TokenUnpacker(pos, synthetic, error_handler)
    return unpacker.unpack_identifier(text)


def unpack_plain_string_lit(text: str, pos: Position, synthetic: bool, error_handler: ErrorHandler) -> str | None:
    unpacker = _TokenUnpacker(pos, synthetic, error_handler)
    return unpacker.unpack_plain_string_lit(text)


@dataclasses.dataclass(slots=True, frozen=True)
class MacroDef:
    params: list[str] | None
    is_variadic: bool
    replacement: list[ReplaceElem]


type Element = Directive | Token

type Directive = NullDirective | DefineDirective | GenericDirective

@dataclasses.dataclass(slots=True, frozen=True)
class DirectiveBase:
    pos: Position

@dataclasses.dataclass(slots=True, frozen=True)
class DefineDirective(DirectiveBase):
    name:        str
    params:      list[str] | None
    is_variadic: bool
    replacement: list[ReplaceElem]

@dataclasses.dataclass(slots=True, frozen=True)
class NullDirective(DirectiveBase):
    pass

@dataclasses.dataclass(slots=True, frozen=True)
class GenericDirective(DirectiveBase):                              
    tokens: list[Token]


type ReplaceElem = ReplaceAtom | FuseOper

@dataclasses.dataclass(slots=True, frozen=True)
class FuseOper:
    left:   ReplaceAtom
    rights: tuple[Right, ...]

    @dataclasses.dataclass(slots=True, frozen=True)
    class Right:
        atom: ReplaceAtom
        pos:  Position


type ReplaceAtom = Token | ParamRef | StringifyOper

@dataclasses.dataclass(slots=True, frozen=True)
class ParamRef:
    param_index: int | None  # None means `__VA_ARGS__`
    pos:         Position

@dataclasses.dataclass(slots=True, frozen=True)
class StringifyOper:
    param_index: int | None  # None means `__VA_ARGS__`
    pos:         Position


@dataclasses.dataclass(slots=True, frozen=True)
class Token:
    type_:     TokenType
    text:      str
    pos:       Position
    synthetic: bool = False

    def is_space(self) -> bool:
        return self.type_ in _SPACE

    def is_regular(self) -> bool:
        return self.type_ in _REGULAR

    def is_error(self) -> bool:
        return self.type_ in _ERROR

    def is_special(self) -> bool:
        return self.type_ in _SPECIAL

    def subpos(self, offset: int) -> Position:
        return self.pos if self.synthetic else self.pos.shift(offset)


class TokenType(enum.Enum):
    # Space
    NEWLINE               = enum.auto()
    WHITESPACE            = enum.auto()
    LINE_COMMENT          = enum.auto()
    BLOCK_COMMENT         = enum.auto()

    # Regular
    HASH_HASH             = enum.auto()
    HASH                  = enum.auto()
    RAW_STRING_LIT        = enum.auto()
    STRING_LIT            = enum.auto()
    CHAR_LIT              = enum.auto()
    NUMBER                = enum.auto()
    IDENTIFIER            = enum.auto()
    PUNCT                 = enum.auto()
    STRAY_CHAR            = enum.auto()

    # Special
    HEADER_NAME           = enum.auto()
    HIDDEN_IDENT          = enum.auto()  # Cannot be leading identifier of macro invocation
    END_OF_INPUT          = enum.auto()

    # Error
    UNTERM_BLOCK_COMMENT  = enum.auto()
    NO_RAW_STRING_LPAREN  = enum.auto()
    BAD_RAW_STRING_DELIM  = enum.auto()
    UNTERM_RAW_STRING_LIT = enum.auto()
    UNTERM_STRING_LIT     = enum.auto()
    UNTERM_CHAR_LIT       = enum.auto()
    UNTERM_HEADER_NAME    = enum.auto()


class ErrorHandler(typing.Protocol):
    def __call__(self, pos: Position, message: str, *args: typing.Any) -> None:
        ...


@dataclasses.dataclass(slots=True, frozen=True)
class Position:
    file_index:  int
    pos_in_file: int

    def shift(self, offset: int) -> Position:
        return Position(file_index = self.file_index, pos_in_file = self.pos_in_file + offset)








# FIXME: Fix tokenization of raw string literals. Line splicing must be disabled between the
# opening and closing quotation marks.           
def _tokenize(input_: typing.TextIO, file_index: int, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
    line_iter = _logical_lines(input_, tracker)

    line:     str
    base_pos: int
    j:        int
    text:     str

    def consume(closing_marker: str, udl_suffix: bool) -> bool:
        nonlocal line, base_pos, j, text
        parts = [text]
        unterminated = False
        while True:
            k = line.find(closing_marker, j)
            if k != -1:
                k += len(closing_marker)
                if udl_suffix:
                    if m := _IDENTIFIER_REGEX.match(line, k):
                        k = m.end()
                parts.append(line[j:k])
                j = k
                break
            parts.append(line[j:])
            line_obj = next(line_iter, None)
            j = 0
            if not line_obj:
                unterminated = True
                line = ""
                break
            line = line_obj.text
            base_pos = line_obj.pos
        text = "".join(parts)
        return not unterminated

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
            text = m.group()
            j = m.end()
            pos = base_pos + i
            token_type: TokenType
            match group_name:
                case "BLOCK_COMMENT_OPEN":
                    if consume(closing_marker="*/", udl_suffix=False):
                        token_type = TokenType.BLOCK_COMMENT
                    else:
                        token_type = TokenType.UNTERM_BLOCK_COMMENT
                case "RAW_STRING_LIT_OPEN":
                    if text[-1] != "(":
                        token_type = TokenType.NO_RAW_STRING_LPAREN
                    else:
                        k = text.find('"')
                        assert k != -1
                        delim = text[k+1:-1]
                        if len(delim) > 16 or not all(33 <= ord(c) <= 126 and c not in ")\\" for c in delim):
                            token_type = TokenType.BAD_RAW_STRING_DELIM
                        else:
                            if consume(closing_marker=')%s"' % delim, udl_suffix=True):
                                token_type = TokenType.RAW_STRING_LIT
                            else:
                                token_type = TokenType.UNTERM_RAW_STRING_LIT
                case _:
                    token_type = TokenType[group_name]
            if token_type not in _SPACE:
                if expect_header and line[i] in '<"':
                    m = _HEADER_REGEX.match(line, i)
                    assert m
                    text = m.group()
                    j = m.end()
                    closing = ">" if text[0] == "<" else '"'
                    if len(text) >= 2 and text[-1] == closing:
                        token_type = TokenType.HEADER_NAME
                    else:
                        token_type = TokenType.UNTERM_HEADER_NAME
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
            yield Token(token_type, text, position)
            i = j

    text = ""
    position = Position(file_index = file_index, pos_in_file = tracker.current())
    yield Token(TokenType.END_OF_INPUT, text, position)


class _TokenizeState(enum.Enum):
    INITIAL           = enum.auto()
    NOT_DIRECTIVE     = enum.auto()
    AFTER_HASH        = enum.auto()
    IN_DIRECTIVE      = enum.auto()
    IN_CONDITION      = enum.auto()
    AFTER_HAS_INCLUDE = enum.auto()



_UCN_REGEX_STRING        = r"(?:\\u[0-9A-Fa-f]{4}|\\u\{[0-9A-Fa-f]+\}|\\U[0-9A-Fa-f]{8}|\\N\{[^}\n]+\})"
_IDENTIFIER_REGEX_STRING = r"(?:(?:[A-Za-z_]|%s)(?:\w+|%s)*)" % (_UCN_REGEX_STRING, _UCN_REGEX_STRING)
_PUNCT_REGEX_STRING      = (r"::|\.\.\.|->\*|->|\+\+|--|<<=|>>=|<<|>>|<=>|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|\/=|%=|&=|"
                            r"\^=|\|=|\.\*|<%|%>|<:(?:(?!:)|(?=::|:>))|:>|[{}()\[\];,.?:+\-*%^&|~!=<>]")

_TOKEN_REGEX = re.compile("|".join("(?P<%s>%s)" % (name, expr) for name, expr in [
    ("NEWLINE",             r"\n"),
    ("WHITESPACE",          r"[ \t\f\v]+"),
    ("LINE_COMMENT",        r"//.*"),
    ("BLOCK_COMMENT_OPEN",  r"/\*"),
    ("HASH_HASH",           r"##|%:%:"),
    ("HASH",                r"#|%:"),
    ("RAW_STRING_LIT_OPEN", r'(?:u8|u|U|L)?R"[^(\n]*\(?'),
    ("STRING_LIT",          r'(?:u8|u|U|L)?"(?:\\.|[^"\\\n])*"(?:%s)?' % _IDENTIFIER_REGEX_STRING),
    ("UNTERM_STRING_LIT",   r'(?:u8|u|U|L)?".*'),
    ("CHAR_LIT",            r"(?:u8|u|U|L)?'(?:\\.|[^'\\\n])*'(?:%s)?" % _IDENTIFIER_REGEX_STRING),
    ("UNTERM_CHAR_LIT",     r"(?:u8|u|U|L)?'.*"),
    ("NUMBER",              r"(?:\d|\.\d)(?:\.|[eEpP][+-]|%s|\'?\w)*" % _UCN_REGEX_STRING),
    ("IDENTIFIER",          _IDENTIFIER_REGEX_STRING),
    ("PUNCT",               _PUNCT_REGEX_STRING),
    ("STRAY_CHAR",          r"."),
]), re.ASCII)


_IDENTIFIER_REGEX = re.compile(_IDENTIFIER_REGEX_STRING, re.ASCII)

_HEADER_REGEX = re.compile(r'<[^>\n]*>?|"[^"\n]*"?')


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


# FIXME: Change this parsing step to perform only shallow parsing of directives
# (NullDirective, RegularDirective, ErrorDirective). Then provide a separate function to
# fully parse directives. This allows the full directive parsing to be delayed until the
# point where the directive needs to be executed and it allows the directive to be ignored
# entirely inside disabled code (`#if 0`)          
def _parse(tokens: typing.Iterable[Token], error_handler: ErrorHandler) -> collections.abc.Iterator[Element]:
    # FIXME: Deal with error tokens inside directives (NO_RAW_STRING_LPAREN, BAD_RAW_STRING_DELIM, UNTERM_RAW_STRING_LIT, UNTERM_STRING_LIT, UNTERM_CHAR_LIT, UNTERM_BLOCK_COMMENT)    
    # FIXME: Detect illegal occurrences of `__VA_ARGS__` and `__VA_OPT__` in directives   
    # FIXME: Find way to deal with `__VA_ARGS__` and `__VA_OPT__` outside directives   
    # FIXME: Detect constructions of `__VA_ARGS__` and `__VA_OPT__` through fusing during macro expansion (all such cases are illegal)     
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
                if token.type_ in _SPACE:
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
                while i > 0 and buffer_[i - 1].type_ in _SPACE:
                    i -= 1
                if directive := _parse_directive(directive_pos, token.pos, buffer_[:i], error_handler):
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
    while i < len(tokens) and tokens[i].type_ in _SPACE:
        i += 1
    if i == len(tokens):
        return NullDirective(pos)
    if tokens[i].type_ is TokenType.IDENTIFIER:
        name = tokens[i].text
        match name:
            case "define":
                return _parse_define_directive(pos, end_pos, tokens, i + 1, error_handler)
    return GenericDirective(pos, tokens[i:])


def _parse_define_directive(pos: Position, end_pos: Position, tokens: list[Token], i: int,
                            error_handler: ErrorHandler) -> DefineDirective | None:
    while i < len(tokens) and tokens[i].type_ in _SPACE:
        i += 1

    if i == len(tokens):
        error_handler(end_pos, "Missing macro name in #define directive")
        return None

    if tokens[i].type_ is not TokenType.IDENTIFIER:
        error_handler(tokens[i].pos, "Macro name must be an identifier")
        return None

    token: Token | None
    token = tokens[i]
    name = unpack_identifier(token.text, token.pos, token.synthetic, error_handler)
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
                i += 1
                if token.type_ in _SPACE:
                    continue
                token_pos = token.pos
            else:
                token = None
                token_pos = end_pos
            if expect_param:
                expect_param = False
                if token:
                    if token.type_ is TokenType.IDENTIFIER:
                        param = unpack_identifier(token.text, token.pos, token.synthetic, error_handler)
                        if param is None:
                            return None
                        if param in params:
                            error_handler(token_pos, "Duplicate name %s in macro parameter list",
                                          _b.clamped_quote(name, 64))
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

    while i < len(tokens) and tokens[i].type_ in _SPACE:
        i += 1
    replacement = _parse_macro_replacement_tokens(name, params, is_variadic, tokens[i:], error_handler)
    if replacement is None:
        return None
    return DefineDirective(pos, name, params, is_variadic, replacement)


def _parse_macro_replacement_tokens(macro_name: str, params: list[str] | None, is_variadic: bool,
                                    replacement_tokens: collections.abc.Sequence[Token],
                                    error_handler: ErrorHandler) -> list[ReplaceElem] | None:
    # FIXME: Also deal with __VA_OPT__            
    elements_1: collections.abc.Sequence[ReplaceAtom] = replacement_tokens
    if params is not None:
        param_indexes = {name: index for index, name in enumerate(params)}
        def lookup_param(name: str) -> tuple[bool, int | None]:
            index = param_indexes.get(name)
            if index is not None:
                return True, index
            if is_variadic and name == _VA_ARGS_NANE:
                return True, None
            return False, None

        elements = list[ReplaceAtom]()
        n = len(replacement_tokens)
        i = 0
        while i < n:
            token = replacement_tokens[i]
            i += 1
            if token.type_ is TokenType.HASH:
                pos = token.pos
                while True:
                    if i == n:
                        error_handler(pos, "Missing operand of `#` (stringify operator) in definition of macro %s",
                                      _b.clamped_quote(macro_name, 64))
                        return None
                    token = replacement_tokens[i]
                    i += 1
                    if token.type_ not in _SPACE:
                        break
                if token.type_ is TokenType.IDENTIFIER:
                    name = unpack_identifier(token.text, token.pos, token.synthetic, _null_error_handler)
                    if name is not None:
                        success, index = lookup_param(name)
                        if success:
                            elements.append(StringifyOper(index, pos))
                            continue
                error_handler(token.pos, "Operand of `#` (stringify operator) is not a parameter name in definition "
                              "of macro %s", _b.clamped_quote(macro_name, 64))
                return None
            if token.type_ is TokenType.IDENTIFIER:
                name = unpack_identifier(token.text, token.pos, token.synthetic, _null_error_handler)
                if name is not None:
                    success, index = lookup_param(name)
                    if success:
                        elements.append(ParamRef(index, token.pos))
                        continue
            elements.append(token)
        elements_1 = elements

    elements_2 = list[ReplaceElem]()
    n = len(elements_1)
    i = 0
    while i < n:
        elem = elements_1[i]
        i += 1
        if not isinstance(elem, Token):
            elements_2.append(elem)
            continue
        token = elem
        if token.type_ is not TokenType.HASH_HASH:
            elements_2.append(elem)
            continue
        while True:
            if not elements_2:
                error_handler(token.pos, "Missing left operand of `##` (fuse operator) in definition of macro %s",
                              _b.clamped_quote(macro_name, 64))
                return None
            left  = elements_2.pop()
            if not isinstance(left, Token) or left.type_ not in _SPACE:
                break
        while True:
            if i == n:
                error_handler(token.pos, "Missing right operand of `##` (fuse operator) in definition of macro %s",
                              _b.clamped_quote(macro_name, 64))
                return None
            right = elements_1[i]
            i += 1
            if not isinstance(right, Token) or right.type_ not in _SPACE:
                break
        rights = (FuseOper.Right(right, token.pos),)
        if not isinstance(left, FuseOper):
            fuse_oper = FuseOper(left, rights)
        else:
            fuse_oper = FuseOper(left.left, left.rights + rights)
        elements_2.append(fuse_oper)

    return elements_2


_VA_ARGS_NANE = "__VA_ARGS__"


def _preprocess(elements: typing.Iterable[Element], macro_registry: dict[str, MacroDef],
                error_handler: ErrorHandler) -> collections.abc.Iterator[Token]:

    @dataclasses.dataclass(slots=True)
    class StackEntry:
        macro_name:  str
        replacement: list[Token]
        pos:         int = 0

    active_set = set[str]()

    @dataclasses.dataclass(slots=True, frozen=True)
    class ArgDelim:
        begin: int
        end:   int
        pos:   Position

    def process(elements: typing.Iterable[Element]) -> collections.abc.Iterator[Token]:
        stack = list[StackEntry]()
        elements_iter = iter(elements)
        def next_elem() -> Element:
            while stack:
                entry = stack[-1]
                if entry.pos < len(entry.replacement):
                    token = entry.replacement[entry.pos]
                    entry.pos += 1
                    return token
                pop()
            return next(elements_iter)

        def push(macro_name: str, replacement: list[Token]) -> None:
            assert macro_name not in active_set
            active_set.add(macro_name)
            entry = StackEntry(macro_name, replacement)
            stack.append(entry)

        def pop() -> None:
            entry = stack.pop()
            active_set.remove(entry.macro_name)

        tokens:        list[Token]
        tokens_offset: int
        arg_delims:    list[ArgDelim]

        def record_arg(token: Token) -> int:
            begin = tokens_offset
            end   = len(tokens)
            arg_delims.append(ArgDelim(begin, end, token.pos))
            return end

        def get_arg(begin: int, end: int) -> list[Token]:
            assert begin <= end <= len(arg_delims)
            assert end > 0
            if begin < end:
                i = arg_delims[begin].begin
                j = arg_delims[end - 1].end
                while i < j and tokens[i].type_ in _SPACE:
                    i += 1
                while i < j and tokens[j - 1].type_ in _SPACE:
                    j -= 1
                return tokens[i:j]
            return []

        elem = next_elem()
        while True:
            match elem:
                case Token():
                    pass
                case NullDirective() as define:
                    continue
                case DefineDirective() as define:
                    macro_registry[define.name] = MacroDef(define.params, define.is_variadic, define.replacement)
                    elem = next_elem()
                    continue
                case GenericDirective() as generic:
                    raise NotImplementedError        
                case _:
                    typing.assert_never(elem)
            token = elem
            if token.type_ is not TokenType.IDENTIFIER:
                yield token
                if token.type_ is TokenType.END_OF_INPUT:
                    break
                elem = next_elem()
                continue
            name = unpack_identifier(token.text, token.pos, token.synthetic, _null_error_handler)
            if name is None:
                # Invalid identifier cannot match name of defined macro
                yield token
                elem = next_elem()
                continue
            macro = macro_registry.get(name)
            if not macro:
                yield token
                elem = next_elem()
                continue
            if name in active_set:
                token = Token(TokenType.HIDDEN_IDENT, token.text, token.pos)
                yield token
                elem = next_elem()
                continue
            if macro.params is None:
                # Object-like macro
                replacement = expand_replacement(macro, arguments=None, va_args=None)
                if replacement is not None:
                    push(name, replacement)
                elem = next_elem()
                continue

            tokens = [token]
            found_lparen = False
            while True:
                elem = next_elem()
                match elem:
                    case Token() as token_2:
                        if token_2.type_ is TokenType.PUNCT and token_2.text == "(":
                            found_lparen = True
                            break
                        if token_2.type_ in _SPACE:
                            tokens.append(token_2)
                            continue
                break
            if not found_lparen:
                yield from tokens
                continue

            # Function-like macro
            tokens = []
            tokens_offset = 0
            arg_delims = []
            paren_level = 1
            while True:
                elem = next_elem()
                match elem:
                    case Token() as token_2:
                        if token_2.type_ is TokenType.IDENTIFIER:
                            name_2 = unpack_identifier(token_2.text, token_2.pos, token_2.synthetic,
                                                        _null_error_handler)
                            if name_2 in active_set:
                                token_2 = Token(TokenType.HIDDEN_IDENT, token_2.text, token_2.pos)
                        elif token_2.type_ is TokenType.PUNCT:
                            if token_2.text == ",":
                                if paren_level == 1:
                                    tokens_offset = record_arg(token_2) + 1
                            elif token_2.text == "(":
                                paren_level += 1
                            elif token_2.text == ")":
                                paren_level -= 1
                                if paren_level == 0:
                                    record_arg(token_2)
                                    break
                        elif token_2.type_ is TokenType.END_OF_INPUT:
                            error_handler(token_2.pos, "Expected closing parenthesis or comma in invocation of "
                                          "function-like macro %s", _b.clamped_quote(name, 64))
                            break
                        tokens.append(token_2)
                        continue
                    case NullDirective() | DefineDirective() | GenericDirective() as directive:
                        pos = directive.pos
                        error_handler(pos, "Preprocessor directive inside macro argument is not allowed")
                        continue
                typing.assert_never(elem)
            if paren_level > 0:
                continue

            n = len(macro.params)
            if len(arg_delims) < n:
                pos = arg_delims[-1].pos
                error_handler(pos, "Too few arguments in invocation of function-like macro %s",
                              _b.clamped_quote(name, 64))
                elem = next_elem()
                continue
            if not macro.is_variadic and len(arg_delims) > n and (len(arg_delims) > 1 or get_arg(0, 1)):
                if n > 0:
                    pos = arg_delims[n - 1].pos
                else:
                    arg = get_arg(0, 1)
                    pos = arg[0].pos if arg else arg_delims[0].pos
                error_handler(pos, "Too many arguments in invocation of function-like macro %s",
                              _b.clamped_quote(name, 64))
                elem = next_elem()
                continue

            arguments = [get_arg(i, i + 1) for i in range(n)]
            va_args = None
            if macro.is_variadic:
                va_args = get_arg(n, len(arg_delims))
            replacement = expand_replacement(macro, arguments, va_args)
            if replacement is not None:
                push(name, replacement)
            elem = next_elem()

    def expand_replacement(macro: MacroDef, arguments: list[list[Token]] | None,
                           va_args: list[Token] | None) -> list[Token] | None:
        @dataclasses.dataclass(slots=True)
        class ArgSlot:
            tokens:      list[Token]
            expanded:    list[Token] | None = None
            stringified: Token | None       = None

        arg_slots = [ArgSlot(a) for a in arguments] if arguments is not None else None
        va_args_slot = None
        if va_args is not None:
            va_args_slot = ArgSlot(va_args)

        def get_arg_slot(index: int | None) -> ArgSlot:
            if index is None:
                assert va_args_slot is not None
                return va_args_slot
            assert arg_slots
            return arg_slots[index]

        def expand_atom(atom: ReplaceAtom, fuse_context: bool) -> collections.abc.Sequence[Token]:
            match atom:
                case Token() as token:
                    return [token]
                case ParamRef() as ref:
                    slot = get_arg_slot(ref.param_index)
                    if fuse_context:
                        return slot.tokens
                    if slot.expanded is None:
                        slot.expanded = preexpand_arg(slot.tokens)
                    return slot.expanded
                case StringifyOper() as oper:
                    slot = get_arg_slot(oper.param_index)
                    if slot.stringified is None:
                        slot.stringified = stringify(slot.tokens, oper.pos)
                    return [slot.stringified]
            typing.assert_never(atom)

        try:
            replacement = list[Token]()
            for elem in macro.replacement:
                if not isinstance(elem, FuseOper):
                    atom = elem
                    replacement += expand_atom(atom, fuse_context=False)
                    continue
                fuse = elem
                accum: Token | None
                composed: bool
                def add(token: Token, operator_pos: Position) -> None:
                    nonlocal accum, composed
                    if accum is None:
                        assert not composed
                        accum = token
                        return
                    # Keep position of first operator that performs a nontrivial fusing operation
                    fuse_pos = token.pos if composed else operator_pos
                    accum = fuse_tokens(accum, token, fuse_pos)
                    composed = True
                assert fuse.rights
                accum = None
                if tokens := expand_atom(fuse.left, fuse_context=True):
                    replacement += tokens[:-1]
                    accum = tokens[-1]
                composed = False
                for right in fuse.rights[:-1]:
                    tokens = expand_atom(right.atom, fuse_context=True)
                    n = len(tokens)
                    if n == 0:
                        continue
                    add(tokens[0], right.pos)
                    if n == 1:
                        continue
                    if accum is not None:
                        replacement.append(accum)
                    replacement += tokens[1:-1]
                    accum = tokens[-1]
                    composed = False
                    continue
                last_right = fuse.rights[-1]
                tokens = expand_atom(last_right.atom, fuse_context=True)
                n = len(tokens)
                if n > 0:
                    add(tokens[0], last_right.pos)
                if accum is not None:
                    replacement.append(accum)
                replacement += tokens[1:]
            return replacement
        except ExpandError:
            return None

    def preexpand_arg(tokens: list[Token]) -> list[Token]:
        return list(process(tokens))

    def stringify(tokens: list[Token], pos: Position) -> Token:
        parts = list[str]()
        pending_space = False
        def flush_space() -> None:
            nonlocal pending_space
            if pending_space:
                parts.append(" ")
                pending_space = False
        for token in tokens:
            if token.type_ in _SPACE:
                pending_space = True
                continue
            flush_space()
            part = token.text
            # C++26 mandates that escaping only takes place in string and character literals
            if token.type_ in {TokenType.RAW_STRING_LIT, TokenType.STRING_LIT, TokenType.CHAR_LIT}:
                part = part.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
            parts.append(part)
        flush_space()
        text = '"%s"' % "".join(parts)
        dummy_pos = Position(0, 0)
        dummy_synthetic = False
        valid = unpack_plain_string_lit(text, dummy_pos, dummy_synthetic, _null_error_handler) is not None
        if valid:
            return Token(TokenType.STRING_LIT, text, pos, synthetic=True)
        error_handler(pos, "Stringification is not a valid preprocessing token")
        raise ExpandError

    def fuse_tokens(left: Token, right: Token, pos: Position) -> Token:
        assert left.type_ in _REGULAR
        assert right.type_ in _REGULAR
        fusion = left.text + right.text
        dummy_file_index = 0
        dummy_tracker = _tp.TextPosTracker()
        with io.StringIO(fusion) as input_:
            tokens = list(_tokenize(input_, dummy_file_index, dummy_tracker))
        assert tokens and tokens[-1].type_ is TokenType.END_OF_INPUT
        if len(tokens) != 2 or tokens[0].type_ not in _REGULAR:
            error_handler(pos, "Fusion %s is not a valid preprocessing token" % _b.clamped_quote(fusion, 64))
            raise ExpandError
        return Token(tokens[0].type_, fusion, pos, synthetic=True)

    class ExpandError(Exception):
        pass

    return process(elements)


_SPACE   = {TokenType.NEWLINE, TokenType.WHITESPACE, TokenType.LINE_COMMENT, TokenType.BLOCK_COMMENT}
_SPECIAL = {TokenType.HEADER_NAME, TokenType.HIDDEN_IDENT, TokenType.END_OF_INPUT}
_ERROR   = {TokenType.UNTERM_BLOCK_COMMENT, TokenType.NO_RAW_STRING_LPAREN, TokenType.BAD_RAW_STRING_DELIM,
            TokenType.UNTERM_RAW_STRING_LIT, TokenType.UNTERM_STRING_LIT, TokenType.UNTERM_CHAR_LIT,
            TokenType.UNTERM_HEADER_NAME}
_REGULAR = set(TokenType) - _SPACE - _ERROR - _SPECIAL


def _null_error_handler(pos: Position, message: str, *args: typing.Any) -> None:
    return


class _TokenUnpacker:
    def __init__(self, pos: Position, synthetic: bool, error_handler: ErrorHandler) -> None:
        self._pos           = pos
        self._synthetic     = synthetic
        self._error_handler = error_handler

    def unpack_identifier(self, text: str) -> str | None:
        # FIXME: Verify that the unpacked identifier conforms to the XID_Start /
        # XID_Continue constraint (unpacked.isidentifier())        

        def replace(m: re.Match[str]) -> str:
            subtext = m.group()
            discr = subtext[1]
            which = m.lastindex
            offset = m.start()
            assert which is not None
            code_point = self._resolve_ucn(discr, m, which, offset)
            assert code_point is not None
            if code_point < 128:
                subpos = self._get_subpos(offset)
                self._error_handler(subpos, "UCN code point less than 128 not allowed in identifier")
                raise _UnpackError from None
            return self._char_from_code_point(code_point, offset)

        try:
            unpacked = _UCN_REGEX.sub(replace, text)
        except _UnpackError:
            return None

        if unicodedata.is_normalized("NFC", unpacked):
            return unpacked
        self._error_handler(self._pos, "Identifier does not conform to Unicode NFC")
        return None

    def unpack_plain_string_lit(self, text: str) -> str | None:
        if len(text) < 2 or text[0] != '"' or text[-1] != '"':
            self._error_handler(self._pos, "Invalid plain string literal")
            return None

        def replace(m: re.Match[str]) -> str:
            subtext = m.group()
            offset = 1 + m.start()
            if len(subtext) == 1:
                pos = self._get_subpos(offset)
                self._error_handler(pos, "Final stray backslash")
                raise _UnpackError from None
            discr = subtext[1]
            unpacked = _SIMPLE_ESCAPES.get(discr)
            if unpacked is not None:
                return unpacked
            which = m.lastindex
            if which is not None:
                code_point: int | None
                if discr == "x":
                    digits = m.group(which)
                    code_point = int(digits, 16)
                    return self._char_from_code_point(code_point, offset)
                if discr in "o01234567":
                    assert which is not None
                    digits = m.group(which)
                    code_point = int(digits, 8)
                    return self._char_from_code_point(code_point, offset)
                code_point = self._resolve_ucn(discr, m, which, offset)
                if code_point is not None:
                    return self._char_from_code_point(code_point, offset)
            subpos = self._get_subpos(offset)
            self._error_handler(subpos, "Invalid escape sequence")
            raise _UnpackError from None

        try:
            return _STRING_ESCAPE_REGEX.sub(replace, text[1:-1])
        except _UnpackError:
            return None

    def _resolve_ucn(self, discr: str, m: re.Match[str], which: int, offset: int) -> int | None:
        if discr == "u" or discr == "U":
            digits = m.group(which)
            code_point = int(digits, 16)
            if 0xD800 <= code_point < 0xE000:
                subpos = self._get_subpos(offset)
                self._error_handler(subpos, "Illegal surrogate code point in UCN: U+%04X" % code_point)
                raise _UnpackError from None
            return code_point

        if discr == "N":
            name = m.group(which)
            try:
                string = unicodedata.lookup(name)
                if len(string) == 1:
                    code_point = ord(string)
                    return code_point
            except KeyError:
                pass
            subpos = self._get_subpos(offset)
            self._error_handler(subpos, "Invalid Unicode character name in UCN: %s", _b.clamped_quote(name, 64))
            raise _UnpackError from None

        return None

    def _char_from_code_point(self, code_point: int, offset: int) -> str:
        try:
            return chr(code_point)
        except ValueError:
            subpos = self._get_subpos(offset)
            self._error_handler(subpos, "Code point out of range: U+%04X" % code_point)
            raise _UnpackError from None

    def _get_subpos(self, offset: int) -> Position:
        return self._pos if self._synthetic else self._pos.shift(offset)


class _UnpackError(Exception):
    pass


_SIMPLE_ESCAPES = {
    "'":  "'",
    '"':  '"',
    "?":  "?",
    "\\": "\\",
    "a":  "\a",
    "b":  "\b",
    "f":  "\f",
    "n":  "\n",
    "r":  "\r",
    "t":  "\t",
    "v":  "\v",
}


_UCN_REGEX_STRING = (r"\\u([0-9A-Fa-f]{4})|"
                     r"\\u\{([0-9A-Fa-f]+)\}|"
                     r"\\U([0-9A-Fa-f]{8})|"
                     r"\\N\{([^}\n]+)\}")

_STRING_ESCAPE_REGEX_STRING = (_UCN_REGEX_STRING + "|"
                               r"\\x([0-9A-Fa-f]+)|"
                               r"\\x\{([0-9A-Fa-f]+)\}|"
                               r"\\([0-7]{1,3})|"
                               r"\\o\{([0-7]+)\}|"
                               r"\\.|"
                               r"\\")  # Fallback for invalid final stray backslash

_UCN_REGEX = re.compile(_UCN_REGEX_STRING)

_STRING_ESCAPE_REGEX = re.compile(_STRING_ESCAPE_REGEX_STRING)
