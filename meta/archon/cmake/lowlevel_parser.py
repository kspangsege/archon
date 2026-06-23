from __future__ import annotations
from typing import Protocol, Any, assert_never
from collections.abc import Iterator
from dataclasses import dataclass

import enum
import re
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l


def parse(tracker: _tp.FilePosTracker, warning_handler: ErrorHandler, error_handler: ErrorHandler) -> Iterator[Invoc]:
    return _parse(tracker, warning_handler, error_handler)


def is_flow_control_command(command_name_cf: str):
    return command_name_cf in _FLOW_CONTROL_MAP


class ErrorHandler(Protocol):
    def __call__(self, pos: int, message: str, *args: Any) -> None:
        ...


type Invoc = IfInvoc | ForeachInvoc | WhileInvoc | MacroDefInvoc | FunctionDefInvoc | BlockInvoc | ReturnInvoc | \
    BreakInvoc | ContinueInvoc | GenericInvoc

type GeneralizedInvoc = Invoc | IfBranch

@dataclass(slots=True, frozen=True)
class InvocBase:
    command_name: str
    arguments:    list[Protoargument]
    pos:          int
    lparen_pos:   int
    rparen_pos:   int

@dataclass(slots=True, frozen=True)
class StructuredInvocBase(InvocBase):
    children: list[Invoc]

@dataclass(slots=True, frozen=True)
class IfInvoc(StructuredInvocBase):
    elseif_branches: list[IfBranch]
    else_branch:     IfBranch | None
    closing_invoc:   ClosingInvoc

@dataclass(slots=True, frozen=True)
class IfBranch(StructuredInvocBase):
    pass

@dataclass(slots=True, frozen=True)
class ClosingInvoc(InvocBase):
    pass

@dataclass(slots=True, frozen=True)
class ForeachInvoc(StructuredInvocBase):
    closing_invoc: ClosingInvoc

@dataclass(slots=True, frozen=True)
class WhileInvoc(StructuredInvocBase):
    closing_invoc: ClosingInvoc

@dataclass(slots=True, frozen=True)
class MacroDefInvoc(StructuredInvocBase):
    closing_invoc: ClosingInvoc

@dataclass(slots=True, frozen=True)
class FunctionDefInvoc(StructuredInvocBase):
    closing_invoc: ClosingInvoc

@dataclass(slots=True, frozen=True)
class BlockInvoc(StructuredInvocBase):
    closing_invoc: ClosingInvoc

@dataclass(slots=True, frozen=True)
class ReturnInvoc(InvocBase):
    pass

@dataclass(slots=True, frozen=True)
class BreakInvoc(InvocBase):
    pass

@dataclass(slots=True, frozen=True)
class ContinueInvoc(InvocBase):
    pass

@dataclass(slots=True, frozen=True)
class GenericInvoc(InvocBase):
    command_name_cf: str


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
            match current.flow_control:
                case None:
                    yield GenericInvoc(current.command_name, current.arguments, current.pos, current.lparen_pos,
                                       current.rparen_pos, current.command_name_cf)
                    advance()
                    continue

                case _FlowControl.IF:
                    orig = current
                    advance()
                    children = list(parse(_ParentType.IF))
                    last = orig
                    elseif_branches: list[IfBranch]  = []
                    else_branch:     IfBranch | None = None
                    while current and current.flow_control is _FlowControl.ELSEIF:
                        orig_2 = current
                        advance()
                        children_2 = list(parse(_ParentType.ELSEIF))
                        branch = IfBranch(orig_2.command_name, orig_2.arguments, orig_2.pos, orig_2.lparen_pos,
                                          orig_2.rparen_pos, children_2)
                        elseif_branches.append(branch)
                        last = orig_2
                    if current and current.flow_control is _FlowControl.ELSE:
                        orig_2 = current
                        advance()
                        children_2 = list(parse(_ParentType.ELSE))
                        branch = IfBranch(orig_2.command_name, orig_2.arguments, orig_2.pos, orig_2.lparen_pos,
                                          orig_2.rparen_pos, children_2)
                        else_branch = branch
                        last = orig_2
                    while current and current.flow_control in (_FlowControl.ELSEIF, _FlowControl.ELSE):
                        parent_type_2 = _PARENT_TYPE_MAP[current.flow_control]
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
                    assert current.flow_control is _FlowControl.ENDIF
                    closing_invoc = ClosingInvoc(current.command_name, current.arguments, current.pos,
                                                 current.lparen_pos, current.rparen_pos)
                    yield IfInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos, orig.rparen_pos,
                                  children, elseif_branches, else_branch, closing_invoc)
                    advance()
                    continue

                case _FlowControl.ELSEIF | _FlowControl.ELSE | _FlowControl.ENDIF:
                    if parent_type in [_ParentType.IF, _ParentType.ELSEIF, _ParentType.ELSE]:
                        return
                    if not silent:
                        error_handler(current.pos, "Unmatched %s()", current.command_name)
                    advance()
                    continue

                case _FlowControl.FOREACH | _FlowControl.WHILE | _FlowControl.MACRO | _FlowControl.FUNCTION | \
                     _FlowControl.BLOCK:
                    orig = current
                    orig_command = current.flow_control
                    advance()
                    children = list(parse(_PARENT_TYPE_MAP[orig_command]))
                    if not current:
                        if not silent:
                            error_handler(last.pos, "Unclosed %s()", last.command_name)
                        return
                    assert current.flow_control is _FLOW_CONTROL_END_MAP[orig_command]
                    closing_invoc = ClosingInvoc(current.command_name, current.arguments, current.pos,
                                                 current.lparen_pos, current.rparen_pos)
                    if orig_command == _FlowControl.FOREACH:
                        yield ForeachInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                           orig.rparen_pos, children, closing_invoc)
                    elif orig_command == _FlowControl.WHILE:
                        yield WhileInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                         orig.rparen_pos, children, closing_invoc)
                    elif orig_command == _FlowControl.MACRO:
                        yield MacroDefInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                            orig.rparen_pos, children, closing_invoc)
                    elif orig_command == _FlowControl.FUNCTION:
                        yield FunctionDefInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                               orig.rparen_pos, children, closing_invoc)
                    elif orig_command == _FlowControl.BLOCK:
                        yield BlockInvoc(orig.command_name, orig.arguments, orig.pos, orig.lparen_pos,
                                         orig.rparen_pos, children, closing_invoc)
                    else:
                        assert_never(orig_command)
                    advance()
                    continue

                case _FlowControl.ENDFOREACH | _FlowControl.ENDWHILE | _FlowControl.ENDMACRO | \
                     _FlowControl.ENDFUNCTION | _FlowControl.ENDBLOCK:
                    if parent_type is _PARENT_TYPE_MAP[current.flow_control]:
                        return
                    if not silent:
                        error_handler(current.pos, "Unmatched %s()", current.command_name)
                    advance()
                    continue

                case _FlowControl.RETURN:
                    yield ReturnInvoc(current.command_name, current.arguments, current.pos, current.lparen_pos,
                                      current.rparen_pos)
                    advance()
                    continue

                case _FlowControl.BREAK:
                    yield BreakInvoc(current.command_name, current.arguments, current.pos, current.lparen_pos,
                                     current.rparen_pos)
                    advance()
                    continue

                case _FlowControl.CONTINUE:
                    yield ContinueInvoc(current.command_name, current.arguments, current.pos, current.lparen_pos,
                                        current.rparen_pos)
                    advance()
                    continue

            assert_never(current.flow_control)

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
                            command = _FLOW_CONTROL_MAP.get(command_name_cf)
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
    flow_control:    _FlowControl | None
    arguments:       list[Protoargument]
    pos:             int
    lparen_pos:      int
    rparen_pos:      int


class _FlowControl(enum.Enum):
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
    RETURN      = 14
    BREAK       = 15
    CONTINUE    = 16


_FLOW_CONTROL_MAP = {
    "if":          _FlowControl.IF,
    "elseif":      _FlowControl.ELSEIF,
    "else":        _FlowControl.ELSE,
    "endif":       _FlowControl.ENDIF,
    "foreach":     _FlowControl.FOREACH,
    "endforeach":  _FlowControl.ENDFOREACH,
    "while":       _FlowControl.WHILE,
    "endwhile":    _FlowControl.ENDWHILE,
    "macro":       _FlowControl.MACRO,
    "endmacro":    _FlowControl.ENDMACRO,
    "function":    _FlowControl.FUNCTION,
    "endfunction": _FlowControl.ENDFUNCTION,
    "block":       _FlowControl.BLOCK,
    "endblock":    _FlowControl.ENDBLOCK,
    "return":      _FlowControl.RETURN,
    "break":       _FlowControl.BREAK,
    "continue":    _FlowControl.CONTINUE,
}


_FLOW_CONTROL_END_MAP = {
    _FlowControl.IF:       _FlowControl.ENDIF,
    _FlowControl.FOREACH:  _FlowControl.ENDFOREACH,
    _FlowControl.WHILE:    _FlowControl.ENDWHILE,
    _FlowControl.MACRO:    _FlowControl.ENDMACRO,
    _FlowControl.FUNCTION: _FlowControl.ENDFUNCTION,
    _FlowControl.BLOCK:    _FlowControl.ENDBLOCK,
}


_PARENT_TYPE_MAP = {
    _FlowControl.IF:          _ParentType.IF,
    _FlowControl.ELSEIF:      _ParentType.ELSEIF,
    _FlowControl.ELSE:        _ParentType.ELSE,
    _FlowControl.ENDIF:       _ParentType.IF,
    _FlowControl.FOREACH:     _ParentType.FOREACH,
    _FlowControl.ENDFOREACH:  _ParentType.FOREACH,
    _FlowControl.WHILE:       _ParentType.WHILE,
    _FlowControl.ENDWHILE:    _ParentType.WHILE,
    _FlowControl.MACRO:       _ParentType.MACRO,
    _FlowControl.ENDMACRO:    _ParentType.MACRO,
    _FlowControl.FUNCTION:    _ParentType.FUNCTION,
    _FlowControl.ENDFUNCTION: _ParentType.FUNCTION,
    _FlowControl.BLOCK:       _ParentType.BLOCK,
    _FlowControl.ENDBLOCK:    _ParentType.BLOCK,
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
