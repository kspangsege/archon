from __future__ import annotations
from typing import Protocol, Any, assert_never
from collections.abc import Iterable, Iterator
from dataclasses import dataclass

import enum
import re
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l


def parse(tracker: _tp.FilePosTracker, warning_handler: ErrorHandler, error_handler: ErrorHandler) -> Iterator[Invoc]:
    return _parse(tracker, warning_handler, error_handler)


def is_block_command(command_name_cf: str):
    return command_name_cf in _BLOCK_COMMAND_MAP


class ErrorHandler(Protocol):
    def __call__(self, pos: int, message: str, *args: Any) -> None:
        ...


type Invoc = SimpleInvoc | IfInvoc | ForeachInvoc | WhileInvoc | MacroDefInvoc | FunctionDefInvoc | BlockInvoc

type GeneralizedInvoc = Invoc | IfBranch | EndMarker

@dataclass(slots=True, frozen=True)
class InvocBase:
    command_name: str
    arguments:    list[Protoargument]
    pos:          int
    lparen_pos:   int
    rparen_pos:   int

@dataclass(slots=True, frozen=True)
class SimpleInvoc(InvocBase):
    command_name_cf: str

@dataclass(slots=True, frozen=True)
class StructuredInvocBase(InvocBase):
    children: list[Invoc]

@dataclass(slots=True, frozen=True)
class IfInvoc(StructuredInvocBase):
    elseif_branches: list[IfBranch]
    else_branch:     IfBranch | None
    end_marker:      EndMarker

@dataclass(slots=True, frozen=True)
class IfBranch(StructuredInvocBase):
    pass

@dataclass(slots=True, frozen=True)
class EndMarker(InvocBase):
    pass

@dataclass(slots=True, frozen=True)
class ForeachInvoc(StructuredInvocBase):
    end_marker: EndMarker

@dataclass(slots=True, frozen=True)
class WhileInvoc(StructuredInvocBase):
    end_marker: EndMarker

@dataclass(slots=True, frozen=True)
class MacroDefInvoc(StructuredInvocBase):
    end_marker: EndMarker

@dataclass(slots=True, frozen=True)
class FunctionDefInvoc(StructuredInvocBase):
    end_marker: EndMarker

@dataclass(slots=True, frozen=True)
class BlockInvoc(StructuredInvocBase):
    end_marker: EndMarker


@dataclass(slots=True, frozen=True)
class Protoargument:
    class Type(enum.Enum):
        BARE      = 0
        QUOTED    = 1
        BRACKETED = 2
    type_:       Type
    text:        str
    prefix_size: int
    suffix_size: int
    pos:         int








def _parse(tracker: _tp.FilePosTracker, warning_handler: ErrorHandler, error_handler: ErrorHandler) -> Iterator[Invoc]:
    protoinvocations = _protoparse(tracker, warning_handler, error_handler)
    current = next(protoinvocations, None)

    def parse(parent_type: _ParentType, silent: bool = False) -> Iterator[Invoc]:
        while True:
            if not current:
                return
            match current.block_command:
                case None:
                    yield SimpleInvoc(current.command_name, current.arguments, current.pos, current.lparen_pos,
                                      current.rparen_pos, current.command_name_cf)
                    advance()
                    continue

                case _BlockCommand.IF:
                    orig = current
                    advance()
                    children = list(parse(_ParentType.IF))
                    last = orig
                    elseif_branches: list[IfBranch]  = []
                    else_branch:     IfBranch | None = None
                    while current and current.block_command is _BlockCommand.ELSEIF:
                        orig_2 = current
                        advance()
                        children_2 = list(parse(_ParentType.ELSEIF))
                        branch = IfBranch(orig_2.command_name, orig_2.arguments, orig_2.pos, orig_2.lparen_pos,
                                          orig_2.rparen_pos, children_2)
                        elseif_branches.append(branch)
                        last = orig_2
                    if current and current.block_command is _BlockCommand.ELSE:
                        orig_2 = current
                        advance()
                        children_2 = list(parse(_ParentType.ELSE))
                        branch = IfBranch(orig_2.command_name, orig_2.arguments, orig_2.pos, orig_2.lparen_pos,
                                          orig_2.rparen_pos, children_2)
                        else_branch = branch
                        last = orig_2
                    while current and current.block_command in (_BlockCommand.ELSEIF, _BlockCommand.ELSE):
                        parent_type_2 = _PARENT_TYPE_MAP[current.block_command]
                        if not silent:
                            error_handler(current.pos, "%s() after %s()", current.command_name, last.command_name)
                        orig_2 = current
                        advance()
                        for _ in parse(parent_type_2, silent=True):
                            pass
                        last = orig_2
                    if not current:
                        if not silent:
                            error_handler(last.pos, "Unclosed %s()", last.command_name)
                        return
                    assert current.block_command is _BlockCommand.ENDIF
                    end_marker = EndMarker(current.command_name, current.arguments, current.pos, current.lparen_pos,
                                           current.rparen_pos)
                    yield IfInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos, orig.rparen_pos,
                                  children, elseif_branches, else_branch, end_marker)
                    advance()
                    continue

                case _BlockCommand.ELSEIF | _BlockCommand.ELSE | _BlockCommand.ENDIF:
                    if parent_type in [_ParentType.IF, _ParentType.ELSEIF, _ParentType.ELSE]:
                        return
                    if not silent:
                        error_handler(current.pos, "Unmatched %s()", current.command_name)
                    advance()
                    continue

                case _BlockCommand.FOREACH | _BlockCommand.WHILE | _BlockCommand.MACRO | _BlockCommand.FUNCTION | \
                     _BlockCommand.BLOCK:
                    orig = current
                    orig_command = current.block_command
                    advance()
                    children = list(parse(_PARENT_TYPE_MAP[orig_command]))
                    if not current:
                        if not silent:
                            error_handler(last.pos, "Unclosed %s()", last.command_name)
                        return
                    assert current.block_command is _BLOCK_COMMAND_END_MAP[orig_command]
                    end_marker = EndMarker(current.command_name, current.arguments, current.pos, current.lparen_pos,
                                           current.rparen_pos)
                    if orig_command == _BlockCommand.FOREACH:
                        yield ForeachInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                           orig.rparen_pos, children, end_marker)
                    elif orig_command == _BlockCommand.WHILE:
                        yield WhileInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                         orig.rparen_pos, children, end_marker)
                    elif orig_command == _BlockCommand.MACRO:
                        yield MacroDefInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                            orig.rparen_pos, children, end_marker)
                    elif orig_command == _BlockCommand.FUNCTION:
                        yield FunctionDefInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                               orig.rparen_pos, children, end_marker)
                    elif orig_command == _BlockCommand.BLOCK:
                        yield BlockInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                         orig.rparen_pos, children, end_marker)
                    else:
                        assert_never(orig_command)
                    advance()
                    continue

                case _BlockCommand.ENDFOREACH | _BlockCommand.ENDWHILE | _BlockCommand.ENDMACRO | \
                     _BlockCommand.ENDFUNCTION | _BlockCommand.ENDBLOCK:
                    if parent_type is _PARENT_TYPE_MAP[current.block_command]:
                        return
                    if not silent:
                        error_handler(current.pos, "Unmatched %s()", current.command_name)
                    advance()
                    continue

            assert_never(current.block_command)

    def advance():
        nonlocal current
        assert current is not None
        current = next(protoinvocations, None)

    return parse(_ParentType.ROOT)


class _ParentType(enum.Enum):
    ROOT     = 0
    IF       = 1
    ELSEIF   = 2
    ELSE     = 3
    FOREACH  = 4
    WHILE    = 5
    MACRO    = 6
    FUNCTION = 7
    BLOCK    = 8


def _protoparse(tracker: _tp.FilePosTracker, warning_handler: ErrorHandler,
                error_handler: ErrorHandler) -> Iterator[_Protoinvoc]:
    class State(enum.Enum):
        INITIAL   = 0
        HAVE_NAME = 1
        IN_ARGS   = 2

    class InvocInfo:
        def __init__(self, pos: int) -> None:
            self.command_name = ""
            self.args         = list[Protoargument]()
            self.invalid      = False
            self.pos          = pos
            self.lparen_pos   = 0

    state = State.INITIAL
    level:     int
    have_args: bool
    invoc_info: InvocInfo | None = None

    for token in _tokenize(tracker, error_handler):
        match state:
            case State.INITIAL:
                if isinstance(token, _UnquotedToken):
                    invoc_info = InvocInfo(token.pos)
                    if not re.fullmatch(r"[A-Za-z_][0-9A-Za-z_]*", token.text):
                        error_handler(token.pos, "Invalid command name")
                        invoc_info.invalid = True
                    invoc_info.command_name = token.text
                    state = State.HAVE_NAME
                    continue
                if isinstance(token, _LParenToken):
                    invoc_info = InvocInfo(token.pos)
                    error_handler(token.pos, "Missing command name")
                    invoc_info.invalid = True
                    state = State.IN_ARGS
                    level = 0
                    have_args = False
                    invoc_info.lparen_pos = token.pos
                    continue
                if isinstance(token, _RParenToken):
                    error_handler(token.pos, "Stray closing parenthesis")
                    continue
                invoc_info = InvocInfo(token.pos)
                error_handler(token.pos, "Invalid command name")
                invoc_info.invalid = True
                state = State.HAVE_NAME
                continue

            case State.HAVE_NAME:
                assert invoc_info
                if isinstance(token, _LParenToken):
                    state = State.IN_ARGS
                    level = 0
                    have_args = False
                    invoc_info.lparen_pos = token.pos
                    continue
                if isinstance(token, _RParenToken):
                    error_handler(token.pos, "Stray closing parenthesis")
                    state = State.INITIAL
                    continue
                error_handler(token.pos, "Stray command argument")
                invoc_info.invalid = True
                continue

            case State.IN_ARGS:
                assert invoc_info
                require_preceding_whitespace = True
                reset_have_args = False
                type_ = Protoargument.Type.BARE
                prefix_size = 0
                suffix_size = 0
                if isinstance(token, _LParenToken):
                    level += 1
                    reset_have_args = True
                elif isinstance(token, _RParenToken):
                    assert level >= 0
                    if level == 0:
                        if not invoc_info.invalid:
                            command_name_cf = invoc_info.command_name.casefold()
                            command = _BLOCK_COMMAND_MAP.get(command_name_cf)
                            rparen_pos = token.pos
                            yield _Protoinvoc(invoc_info.command_name, command_name_cf, command, invoc_info.args,
                                              invoc_info.pos, invoc_info.lparen_pos, rparen_pos)
                        state = State.INITIAL
                        continue
                    level -= 1
                    require_preceding_whitespace = False
                elif isinstance(token, _UnquotedToken):
                    pass
                elif isinstance(token, _QuotedToken):
                    type_ = Protoargument.Type.QUOTED
                    prefix_size = 1
                    suffix_size = 1
                elif isinstance(token, _BracketToken):
                    type_ = Protoargument.Type.BRACKETED
                    prefix_size = token.prefix_size
                    suffix_size = token.suffix_size
                else:
                    assert False
                if have_args and require_preceding_whitespace and not token.preceded_by_whitespace:
                    warning_handler(token.pos, "Missing whitespace between arguments")
                invoc_info.args.append(Protoargument(type_, token.text, prefix_size, suffix_size, token.pos))
                have_args = True
                if reset_have_args:
                    have_args = False
                continue

        assert_never(state)

    if state != State.INITIAL:
        assert invoc_info
        error_handler(invoc_info.pos, "Unterminated command invocation")


@dataclass(slots=True, frozen=True)
class _Protoinvoc:
    command_name:    str
    command_name_cf: str
    block_command:   _BlockCommand | None
    arguments:       list[Protoargument]
    pos:             int
    lparen_pos:      int
    rparen_pos:      int


class _BlockCommand(enum.Enum):
    IF          =  0
    ELSEIF      =  1
    ELSE        =  2
    ENDIF       =  3
    FOREACH     =  4
    ENDFOREACH  =  5
    WHILE       =  6
    ENDWHILE    =  7
    MACRO       =  8
    ENDMACRO    =  9
    FUNCTION    = 10
    ENDFUNCTION = 11
    BLOCK       = 12
    ENDBLOCK    = 13


_BLOCK_COMMAND_MAP = {
    "if":          _BlockCommand.IF,
    "elseif":      _BlockCommand.ELSEIF,
    "else":        _BlockCommand.ELSE,
    "endif":       _BlockCommand.ENDIF,
    "foreach":     _BlockCommand.FOREACH,
    "endforeach":  _BlockCommand.ENDFOREACH,
    "while":       _BlockCommand.WHILE,
    "endwhile":    _BlockCommand.ENDWHILE,
    "macro":       _BlockCommand.MACRO,
    "endmacro":    _BlockCommand.ENDMACRO,
    "function":    _BlockCommand.FUNCTION,
    "endfunction": _BlockCommand.ENDFUNCTION,
    "block":       _BlockCommand.BLOCK,
    "endblock":    _BlockCommand.ENDBLOCK,
}


_BLOCK_COMMAND_END_MAP = {
    _BlockCommand.IF:       _BlockCommand.ENDIF,
    _BlockCommand.FOREACH:  _BlockCommand.ENDFOREACH,
    _BlockCommand.WHILE:    _BlockCommand.ENDWHILE,
    _BlockCommand.MACRO:    _BlockCommand.ENDMACRO,
    _BlockCommand.FUNCTION: _BlockCommand.ENDFUNCTION,
    _BlockCommand.BLOCK:    _BlockCommand.ENDBLOCK,
}


_PARENT_TYPE_MAP = {
    _BlockCommand.IF:          _ParentType.IF,
    _BlockCommand.ELSEIF:      _ParentType.ELSEIF,
    _BlockCommand.ELSE:        _ParentType.ELSE,
    _BlockCommand.ENDIF:       _ParentType.IF,
    _BlockCommand.FOREACH:     _ParentType.FOREACH,
    _BlockCommand.ENDFOREACH:  _ParentType.FOREACH,
    _BlockCommand.WHILE:       _ParentType.WHILE,
    _BlockCommand.ENDWHILE:    _ParentType.WHILE,
    _BlockCommand.MACRO:       _ParentType.MACRO,
    _BlockCommand.ENDMACRO:    _ParentType.MACRO,
    _BlockCommand.FUNCTION:    _ParentType.FUNCTION,
    _BlockCommand.ENDFUNCTION: _ParentType.FUNCTION,
    _BlockCommand.BLOCK:       _ParentType.BLOCK,
    _BlockCommand.ENDBLOCK:    _ParentType.BLOCK,
}


def _tokenize(tracker: _tp.FilePosTracker, error_handler: ErrorHandler) -> Iterator[_Token]:
    input_ = ""
    eof = False
    prev_token_is_whitespace = False
    with open(tracker.path, "r") as file_:
        while True:
            line = file_.readline()
            if line:
                input_ += line
            else:
                eof = True
            pos = 0
            while pos < len(input_):
                m = _TOKEN_REGEX.match(input_, pos)
                assert m

                token_text = m.group(0)

                orig_pos = pos
                pos = m.end()

                new_pos = pos = m.end()
                if m.group("SPACE") or m.group("COMMENT"):
                    tracker.track(token_text)
                    prev_token_is_whitespace = True
                    pos = new_pos
                    continue

                if new_pos == len(input_) and not eof:
                    break

                token_pos = tracker.track(token_text)
                preceded_by_whitespace = prev_token_is_whitespace
                prev_token_is_whitespace = False
                pos = new_pos

                if m.group("UNQUOTED"):
                    yield _UnquotedToken(token_text, preceded_by_whitespace, token_pos)
                    continue

                if m.group("QUOTED"):
                    yield _QuotedToken(token_text, preceded_by_whitespace, token_pos)
                    continue

                if m.group("LPAREN"):
                    yield _LParenToken(token_text, preceded_by_whitespace, token_pos)
                    continue

                if m.group("RPAREN"):
                    yield _RParenToken(token_text, preceded_by_whitespace, token_pos)
                    continue

                if m.group("BRACKET"):
                    eqs = m.group("eqs2")
                    prefix_size = 2 + len(eqs)
                    suffix_size = 2 + len(eqs)
                    if len(token_text) > prefix_size + suffix_size and token_text[prefix_size] == "\n":
                        prefix_size += 1
                    yield _BracketToken(token_text, preceded_by_whitespace, token_pos, prefix_size, suffix_size)
                    continue

                if m.group("UNTERM_COMMENT"):
                    error_handler(token_pos, "Unterminated bracketed comment")
                    continue

                if m.group("UNTERM_BRACKET"):
                    error_handler(token_pos, "Unterminated bracket string")
                    continue

                if m.group("UNTERM_QUOTED"):
                    error_handler(token_pos, "Unterminated quoted string")
                    continue

                if m.group("UNTERM_ESCAPE"):
                    error_handler(token_pos, "Unterminated escape sequence")
                    continue

                assert False

            if eof:
                break
            input_ = input_[pos:]


type _Token = _UnquotedToken | _QuotedToken | _BracketToken | _LParenToken | _RParenToken

@dataclass(slots=True, frozen=True)
class _TokenBase:
    text:                   str
    preceded_by_whitespace: bool
    pos:                    int

@dataclass(slots=True, frozen=True)
class _UnquotedToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _QuotedToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _BracketToken(_TokenBase):
    prefix_size: int
    suffix_size: int

@dataclass(slots=True, frozen=True)
class _LParenToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _RParenToken(_TokenBase):
    pass


_TOKEN_REGEX = re.compile(
    r'(?P<COMMENT>#\[(?P<eqs1>=*)\[.*?\](?P=eqs1)\]|#(?!\[=*\[)[^\n]*)|'
    r'(?P<BRACKET>\[(?P<eqs2>=*)\[.*?\](?P=eqs2)\])|'
    r'(?P<QUOTED>"(?:\\.|[^"\\])*")|'
    r'(?P<UNTERM_COMMENT>#\[=*\[.*)|'
    r'(?P<UNTERM_BRACKET>\[=*\[.*)|'
    r'(?P<UNTERM_QUOTED>".*)|'
    r'(?P<LPAREN>\()|'
    r'(?P<RPAREN>\))|'
    r'(?P<SPACE>\s+)|'
    r'(?P<UNQUOTED>(?:\\[^\n]|[^\s()"#\\])+)|'
    r'(?P<UNTERM_ESCAPE>\\)',
    re.DOTALL,
)
