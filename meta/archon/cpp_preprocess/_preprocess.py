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
import abc
import dataclasses
import collections.abc as _cabc
import bisect
import io

import archon.base as _b
import archon.text_pos as _tp
import archon.cpp_preprocess._position as _cp
import archon.cpp_preprocess._tokenize as _ct
import archon.cpp_preprocess._unpack as _cu
import archon.cpp_preprocess._directive as _cd


# Streaming C++26 preprocessor.
#
# Preprocess source code in accordance with C++26 standard. Output is a stream of
# preprocessing tokens not including token types `HEADER_NAME` and `BLOCKED_IDENTIFIER`. End
# of output is marked by an `END_OF_INPUT` token.
#
# The preprocessor operates in a streaming fashion, meaning that it starts generating tokens
# before having seen the end of input.
#
def preprocess(root_file_index: int, root_source: Source, app: Application,
               config: Config | None = None) -> _cabc.Iterator[_ct.Token]:
    return _preprocess(root_file_index, root_source, app, config or Config())


class Application(abc.ABC):
    # Returns `file_index` of resolved file, or `None` if the file could not be found. The
    # caller generates an error message if the file was not found.
    #
    @abc.abstractmethod
    def resolve_include(self, path: str, resolution_type: _cd.ResolutionType, base_file_index: int, /) -> int | None:
        ...

    @abc.abstractmethod
    def open_file(self, file_index: int) -> Source:
        ...

    # Called with the tokens of the pragma directive minus the leading `#pragma` and minus
    # any minus whitespace that follows immediately after `#pragma`. Since trailing
    # whitespace is not considered part of a directive, the passed token sequence will
    # neither have have leading nor trailing whitespace.
    #
    @abc.abstractmethod
    def handle_pragma(self, tokens: _cabc.Sequence[_ct.Token], pos: _cp.Position, end_pos: _cp.Position, /) -> None:
        ...

    @abc.abstractmethod
    def handle_error(self, pos: _cp.Position, message: str, /, *args: typing.Any) -> None:
        ...


@dataclasses.dataclass(slots=True, frozen=True)
class Source:
    input_:           typing.TextIO
    position_tracker: PositionTracker


class PositionTracker:
    def __init__(self, path: str) -> None:
        self._text_pos_tracker = _tp.TextPosTracker()
        self._overrides: list[PositionTracker._Override] = [PositionTracker._Override(path, 0, 1)]
        # Line index offset for each override entry
        self._offsets: list[int] = [0]

    def __call__(self, pos_in_file: int) -> ResolvedPosition:
        text_pos = self._text_pos_tracker.get_text_pos(pos_in_file)
        line_index = text_pos.line_no - 1
        i = bisect.bisect_right(self._offsets, line_index) - 1
        override = self._overrides[i]
        line_no = override.line_no_base + (line_index - override.line_index_base)
        return ResolvedPosition(override.path, line_no, text_pos.pos_on_line)

    def add_override(self, path: str | None, line_index_base: int, line_no_base: int) -> None:
        assert line_index_base > self._overrides[-1].line_index_base
        path_2 = path if path is not None else self._overrides[-1].path
        self._overrides.append(PositionTracker._Override(path_2, line_index_base, line_no_base))
        self._offsets.append(line_index_base)

    @property
    def text_pos_tracker(self) -> _tp.TextPosTracker:
        return self._text_pos_tracker

    @dataclasses.dataclass(slots=True, frozen=True)
    class _Override:
        path:            str
        line_index_base: int
        line_no_base:    int


@dataclasses.dataclass(slots=True, frozen=True)
class ResolvedPosition:
    path:        str
    line_no:     int
    pos_on_line: int  # Zero based


@dataclasses.dataclass(slots=True)
class Config:
    encoding_info = _cu.EncodingInfo()
    predefined_macros: _cabc.Sequence[BuiltinMacro] | None = None


type Macro = BuiltinMacro | RegularMacro

@dataclasses.dataclass(slots=True, frozen=True)
class MacroBase:
    signature: _cd.MacroSignature

@dataclasses.dataclass(slots=True, frozen=True)
class BuiltinMacro(MacroBase):
    permanent:     bool  # Cannot be undefined
    require_paren: bool  # Occurrence of macro name is always an invocation
    func:          MacroFunction

@dataclasses.dataclass(slots=True, frozen=True)
class RegularMacro(MacroBase):
    replacement:    tuple[_cd.ReplaceElem, ...]
    definition_pos: _cp.Position


# When invoked by the preprocessor, the preprocessor guarantees that `arguments` is not
# `None` when and only when the macro is function-like, and it guarantees that `va_args` is
# not `None` when and only when the macro is variadic. The macro can only be variadic if it
# is function-like.
#
# The function must return `None` if and only if an error occurred.
#
class MacroFunction(typing.Protocol):
    def __call__(self, invoc_pos: _cp.Position, /, arguments: list[MacroArg] | None,
                 va_args: MacroArg | None) -> list[_ct.Token] | None:
        ...


@dataclasses.dataclass(slots=True)
class MacroArg:
    tokens:  list[_ct.Token]
    end_pos: _cp.Position








def _preprocess(root_file_index: int, root_source: Source, app: Application,
                config: Config) -> _cabc.Iterator[_ct.Token]:
    macro_registry: dict[str, Macro] = {}
    include_stack: list[_IncludeStackEntry] = []
    invoc_stack: list[_InvocStackEntry] = []
    active_set: set[str] = set()
    in_argument = False

    def process(tokens_iter: typing.Iterator[_ct.Token], error_occurred: _b.Wrap[bool]) -> _cabc.Iterator[_ct.Token]:
        nonlocal in_argument

        base_level = len(invoc_stack)
        def next_token() -> _ct.Token:
            while len(invoc_stack) > base_level:
                entry = invoc_stack[-1]
                if entry.read_pos < len(entry.replacement):
                    token = entry.replacement[entry.read_pos]
                    entry.read_pos += 1
                    return token
                pop_invoc()
            return next(tokens_iter)

        tokens:        list[_ct.Token]
        tokens_offset: int
        arg_delims:    list[_ArgDelim]

        def record_arg(closing_token: _ct.Token) -> int:
            begin = tokens_offset
            end   = len(tokens)
            arg_delims.append(_ArgDelim(begin, end, closing_token.pos))
            return end

        def get_arg(begin: int, end: int) -> MacroArg:
            assert begin <= end <= len(arg_delims)
            assert end > 0
            closing_delim = arg_delims[end - 1]
            if begin < end:
                i = arg_delims[begin].begin
                j = closing_delim.end
                while i < j and tokens[i].type_ in _ct.SPACE:
                    i += 1
                while i < j and tokens[j - 1].type_ in _ct.SPACE:
                    j -= 1
                arg_tokens = tokens[i:j]
            else:
                arg_tokens = []
            return MacroArg(arg_tokens, closing_delim.pos)

        token = next_token()
        while True:
            if token.type_ is not _ct.TokenType.IDENTIFIER:
                yield token
                if token.type_ is not _ct.TokenType.END_OF_INPUT:
                    token = next_token()
                    continue
                break
            name = token.text
            macro = macro_registry.get(name)
            if macro is None:
                yield token
                token = next_token()
                continue
            if name in active_set:
                yield dataclasses.replace(token, type_=_ct.TokenType.BLOCKED_IDENTIFIER)
                token = next_token()
                continue
            invoc_pos = token.pos
            signature = macro.signature
            if signature.params is None:
                # Object-like macro
                match macro:
                    case BuiltinMacro():
                        replacement = macro.func(invoc_pos, arguments=None, va_args=None)
                    case RegularMacro():
                        replacement = expand_replacement(signature, macro.replacement, arguments=None, va_args=None)
                    case _:
                        typing.assert_never(macro)
                if replacement:
                    push_invoc(name, replacement, invoc_pos)
                else:
                    error_occurred.value = True
                token = next_token()
                continue

            tokens = [token]
            found_lparen = False
            while True:
                token = next_token()
                if token.type_ is _ct.TokenType.PUNCT:
                    if token.text == "(":
                        found_lparen = True
                    break
                if token.type_ in _ct.SPACE:
                    tokens.append(token)
                    continue
                break
            if not found_lparen:
                if isinstance(macro, BuiltinMacro) and macro.require_paren:
                    error(token.pos, "Expected opening parenthesis after `%s`", name)
                    error_occurred.value = True
                yield from tokens
                continue

            # Function-like macro
            in_argument = True
            tokens = []
            tokens_offset = 0
            arg_delims = []
            paren_level = 1
            while True:
                token = next_token()
                if token.type_ is _ct.TokenType.IDENTIFIER:
                    if token.text in active_set:
                        token = dataclasses.replace(token, type_=_ct.TokenType.BLOCKED_IDENTIFIER)
                elif token.type_ is _ct.TokenType.PUNCT:
                    if token.text == ",":
                        if paren_level == 1:
                            tokens_offset = record_arg(token) + 1
                    elif token.text == "(":
                        paren_level += 1
                    elif token.text == ")":
                        paren_level -= 1
                        if paren_level == 0:
                            record_arg(token)
                            break
                elif token.type_ is _ct.TokenType.END_OF_INPUT:
                    error(token.pos, "Expected closing parenthesis or comma in invocation of function-like macro %s",
                          _b.clamped_quote(name, 64))
                    error_occurred.value = True
                    break
                tokens.append(token)
                continue
            if paren_level > 0:
                continue
            in_argument = False

            n = len(signature.params)
            if len(arg_delims) < n:
                pos = arg_delims[-1].pos
                error(pos, "Too few arguments in invocation of function-like macro %s", _b.clamped_quote(name, 64))
                error_occurred.value = True
                token = next_token()
                continue
            is_variadic = signature.params is not None and signature.is_variadic
            if not is_variadic and len(arg_delims) > n and (len(arg_delims) > 1 or get_arg(0, 1).tokens):
                # Determine the position of the delimiter (comma or closing parenthesis)
                # that succeeds the last argument with a corresponding parameter, or, if
                # there are no parameters, the position of the first non-whitespace token of
                # the first argument, or, if the first argument is empty, the position of
                # the closing parenthesis.
                if n > 0:
                    pos = arg_delims[n - 1].pos
                else:
                    arg = get_arg(0, 1)
                    pos = arg.tokens[0].pos if arg.tokens else arg.end_pos
                error(pos, "Too many arguments in invocation of function-like macro %s", _b.clamped_quote(name, 64))
                error_occurred.value = True
                token = next_token()
                continue

            arguments = [get_arg(i, i + 1) for i in range(n)]
            va_args = None
            if is_variadic:
                va_args = get_arg(n, len(arg_delims))
            match macro:
                case BuiltinMacro():
                    replacement = macro.func(invoc_pos, arguments, va_args)
                case RegularMacro():
                    replacement = expand_replacement(signature, macro.replacement, arguments, va_args)
                case _:
                    typing.assert_never(macro)
            if replacement:
                push_invoc(name, replacement, invoc_pos)
            else:
                error_occurred.value = True
            token = next_token()

    def push_invoc(macro_name: str, replacement: list[_ct.Token], invoc_pos: _cp.Position) -> None:
        assert macro_name not in active_set
        active_set.add(macro_name)
        entry = _InvocStackEntry(invoc_pos, macro_name, replacement)
        invoc_stack.append(entry)

    def pop_invoc() -> None:
        entry = invoc_stack.pop()
        active_set.remove(entry.macro_name)

    def expand_replacement(signature: _cd.MacroSignature, replacement: tuple[_cd.ReplaceElem, ...],
                           arguments: list[MacroArg] | None, va_args: MacroArg | None) -> list[_ct.Token] | None:
        assert (signature.params is not None) == (arguments is not None)
        assert (signature.params is not None and signature.is_variadic) == (va_args is not None)

        def expand(elements: _cabc.Sequence[_cd.ReplaceElem]) -> list[_ct.Token]:
            result: list[_ct.Token] = []
            for elem in elements:
                if not isinstance(elem, _cd.FuseOperation):
                    result += expand_primitive(elem, is_fuse_context=False)
                    continue
                fuse_operation = elem
                accum: _ct.Token | None
                composed: bool
                def add(token: _ct.Token, operator_pos: _cp.Position) -> None:
                    nonlocal accum, composed
                    if accum is None:
                        assert not composed
                        accum = token
                        return
                    # Keep position of first operator that performs a nontrivial fusing operation
                    fuse_pos = token.pos if composed else operator_pos
                    accum = fuse_tokens(accum, token, fuse_pos)
                    composed = True
                assert fuse_operation.rights
                accum = None
                if tokens := expand_primitive(fuse_operation.left, is_fuse_context=True):
                    result += tokens[:-1]
                    accum = tokens[-1]
                composed = False
                for right in fuse_operation.rights[:-1]:
                    tokens = expand_primitive(right.operand, is_fuse_context=True)
                    n = len(tokens)
                    if n == 0:
                        continue
                    add(tokens[0], right.pos)
                    if n == 1:
                        continue
                    if accum is not None:
                        result.append(accum)
                    result += tokens[1:-1]
                    accum = tokens[-1]
                    composed = False
                    continue
                last_right = fuse_operation.rights[-1]
                tokens = expand_primitive(last_right.operand, is_fuse_context=True)
                n = len(tokens)
                if n > 0:
                    add(tokens[0], last_right.pos)
                if accum is not None:
                    result.append(accum)
                result += tokens[1:]
            return result

        def expand_primitive(prim: _cd.ReplacePrim, is_fuse_context: bool) -> _cabc.Sequence[_ct.Token]:
            match prim:
                case _ct.Token() as token:
                    return [token]
                case _cd.ParamRef() as ref:
                    return expand_arg(ref.param_index, is_fuse_context)
                case _cd.VaArgs():
                    return expand_arg(None, is_fuse_context)
                case _cd.VaOpt() as va_opt:
                    assert va_args
                    if va_args.tokens:
                        return expand(va_opt.elements)
                    return []
                case _cd.StringifyOperation() as op:
                    match op.operand:
                        case _cd.ParamRef() as ref:
                            return [stringify_arg(ref.param_index, op.pos)]
                        case _cd.VaArgs():
                            return [stringify_arg(None, op.pos)]
                        case _cd.VaOpt() as va_opt:
                            assert va_args
                            if va_args.tokens:
                                tokens = expand(va_opt.elements)
                            else:
                                tokens = []
                            return [stringify(tokens, op.pos)]
                    typing.assert_never(op.operand)
            typing.assert_never(prim)

        def expand_arg(param_index: int | None, is_fuse_context: bool) -> _cabc.Sequence[_ct.Token]:
            slot = get_arg_slot(param_index)
            arg = slot.arg
            if is_fuse_context:
                return arg.tokens
            if slot.expanded is None:
                result = macro_expand(arg.tokens, arg.end_pos)
                if result is None:
                    raise _ExpandError
                slot.expanded = result
            return slot.expanded

        def stringify_arg(param_index: int | None, operator_pos: _cp.Position) -> _ct.Token:
            slot = get_arg_slot(param_index)
            if slot.stringified is None:
                slot.stringified = stringify(slot.arg.tokens, operator_pos)
            return slot.stringified

        arg_slots = [_ArgSlot(a) for a in arguments] if arguments is not None else None
        va_args_slot = None
        if va_args is not None:
            va_args_slot = _ArgSlot(va_args)

        def get_arg_slot(index: int | None) -> _ArgSlot:
            if index is None:
                assert va_args_slot is not None
                return va_args_slot
            assert arg_slots
            return arg_slots[index]

        try:
            return expand(replacement)
        except _ExpandError:
            return None

    def macro_expand(tokens: _cabc.Sequence[_ct.Token], end_pos: _cp.Position) -> list[_ct.Token] | None:
        tokens_2 = list(tokens) + [_ct.Token(_ct.TokenType.END_OF_INPUT, "", end_pos)]
        result: list[_ct.Token] = []
        error_occurred = _b.Wrap(False)
        for token in process(iter(tokens_2), error_occurred):
            if token.type_ is _ct.TokenType.END_OF_INPUT:
                break
            result.append(token)
        if error_occurred.value:
            return None
        return result

    def stringify(tokens: list[_ct.Token], operator_pos: _cp.Position) -> _ct.Token:
        parts: list[str] = []
        pending_space = False
        def flush_space() -> None:
            nonlocal pending_space
            if pending_space:
                parts.append(" ")
                pending_space = False
        token: _ct.Token | None
        for token in tokens:
            if token.type_ in _ct.SPACE:
                pending_space = True
                continue
            flush_space()
            part = token.reconstruct()
            # C++26 mandates that escaping only takes place on string and character literals
            if token.type_ in {_ct.TokenType.RAW_STRING_LIT, _ct.TokenType.STRING_LIT, _ct.TokenType.CHAR_LIT}:
                part = part.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
            parts.append(part)
        flush_space()
        text = '"%s"' % "".join(parts)
        token = synthesize_token(text, operator_pos)
        if token is None:
            error(operator_pos, "Stringification did not produce a valid preprocessing token")
            raise _ExpandError
        assert token.type_ is _ct.TokenType.STRING_LIT
        return token

    def fuse_tokens(left: _ct.Token, right: _ct.Token, operator_pos: _cp.Position) -> _ct.Token:
        text = left.reconstruct() + right.reconstruct()
        token = synthesize_token(text, operator_pos)
        if token is None:
            error(operator_pos, "Fusion %s is not a valid preprocessing token" % _b.clamped_quote(text, 64))
            raise _ExpandError
        if token.type_ is _ct.TokenType.IDENTIFIER:
            if not _cd.verify_no_va_names(token.text, operator_pos, error, synthetic=True):
                raise _ExpandError
        return token

    def synthesize_token(text: str, pos: _cp.Position) -> _ct.Token | None:
        tokens = synthesize_tokens(text, pos)
        return tokens[0] if tokens is not None and len(tokens) == 1 else None

    def synthesize_tokens(text: str, pos: _cp.Position, offset_map: _tp.PosMap | None = None,
                          generate_error_messages: bool = False) -> list[_ct.Token] | None:
        dummy_file_index = 0
        dummy_tracker = _tp.TextPosTracker()
        with io.StringIO(text) as input_:
            has_error = False
            def error_handler(subpos: _cp.Position, message: str, *args: typing.Any) -> None:
                nonlocal has_error
                if generate_error_messages:
                    pos_2 = get_pos(subpos.pos_in_file)
                    error(pos_2, message, *args)
                has_error = True
            def get_pos(offset: int) -> _cp.Position:
                if offset_map is None:
                    return pos
                offset_2 = offset_map.map_(offset)
                return pos.shift(offset_2)
            tokenizer = _ct.Tokenizer(input_, dummy_file_index, dummy_tracker, error_handler)
            tokens: list[_ct.Token] = []
            while True:
                token = tokenizer.next_token()
                if token.type_ is _ct.TokenType.END_OF_INPUT:
                    return tokens if not has_error else None
                pos_2 = get_pos(token.pos.pos_in_file)
                token_2 = dataclasses.replace(token, pos=pos_2, synthetic=True)
                tokens.append(token_2)

    def directive_handler(tokens: _cabc.Sequence[_ct.Token], pos: _cp.Position, end_pos: _cp.Position) -> None:
        if in_argument:
            error(pos, "Preprocessing directive inside macro argument is not allowed")
            return
        result = _cd.preparse_directive(tokens, error)
        if result is None:
            return
        directive_type, remaining_tokens = result
        macro: Macro | None
        match directive_type:
            case _cd.DirectiveType.NULL:
                return
            case _cd.DirectiveType.INCLUDE:
                include_directive = _cd.parse_include_directive(remaining_tokens, pos, end_pos, macro_expand, error)
                if include_directive is None:
                    return
                includer_index = include_stack[-1].file_index if include_stack else root_file_index
                file_index = app.resolve_include(include_directive.path, include_directive.resolution_type,
                                                 includer_index)
                if file_index is None:
                    match include_directive.resolution_type:
                        case _cd.ResolutionType.BRACKETED:
                            error(pos, "Cannot find header <%s>", include_directive.path)
                            return
                        case _cd.ResolutionType.QUOTED:
                            error(pos, 'Cannot find source file "%s"', include_directive.path)
                            return
                    typing.assert_never(include_directive.resolution_type)
                source = app.open_file(file_index)
                text_pos_tracker = source.position_tracker.text_pos_tracker
                tokenizer = _ct.Tokenizer(source.input_, file_index, text_pos_tracker, error)
                directive_filter = _cd.DirectiveFilter(tokenizer, directive_handler)
                entry = _IncludeStackEntry(file_index, source, directive_filter)
                include_stack.append(entry)
                return
            case _cd.DirectiveType.DEFINE:
                define_directive = _cd.parse_define_directive(remaining_tokens, pos, end_pos, error)
                if define_directive is None:
                    return
                signature = define_directive.signature
                name = signature.name
                existing = macro_registry.get(name)
                if existing:
                    # Identical redefinition is a no-op
                    same = (isinstance(existing, RegularMacro) and
                            _same_macro_signature(existing.signature, signature) and
                            _same_replacement_list(existing.replacement, define_directive.replacement))
                    if same:
                        return
                    error(pos, "Redefinition of macro %s", _b.clamped_quote(name, 64))
                    # FIXME: Consider logging separate message ("note" level) pointing to
                    # position of preexisting definition    
                    return
                macro = RegularMacro(signature, define_directive.replacement, define_directive.pos)
                macro_registry[name] = macro
                return
            case _cd.DirectiveType.UNDEF:
                undef_directive = _cd.parse_undef_directive(remaining_tokens, pos, end_pos, error)
                if undef_directive is None:
                    return
                macro = macro_registry.get(undef_directive.name)
                if macro is None:
                    return  # No-op
                if isinstance(macro, BuiltinMacro) and macro.permanent:
                    error(pos, "Cannot undefine permanent built-in macro `%s`", undef_directive.name)
                    return
                del macro_registry[undef_directive.name]
                return
            case _cd.DirectiveType.LINE:
                line_directive = _cd.parse_line_directive(remaining_tokens, pos, end_pos, macro_expand,
                                                          config.encoding_info, error)
                if line_directive is None:
                    return
                # This position override must take effect starting from the first "physical"
                # line that follows the last physical line of the `#line` directive. In
                # general, the current line index as tracked by the associated position
                # tracker is the target line index of the directive, but not if the
                # directive is on the last line of the file and not is followed by a newline
                # character. For that reason, we need to compute the target line index as
                # one plus the line index of the end position of the directive.
                if include_stack:
                    top = include_stack[-1]
                    assert end_pos.file_index == top.file_index
                    source = top.source
                else:
                    assert end_pos.file_index == root_file_index
                    source = root_source
                text_pos = source.position_tracker.text_pos_tracker.get_text_pos(end_pos.pos_in_file)
                directive_line_index = text_pos.line_no - 1
                line_index_base = directive_line_index + 1
                line_no_base = line_directive.line_no
                source.position_tracker.add_override(line_directive.path, line_index_base, line_no_base)
                return
            case _cd.DirectiveType.PRAGMA:
                app.handle_pragma(remaining_tokens, pos, end_pos)
                return
        raise NotImplementedError         

    def error(pos: _cp.Position, message: str, *args: typing.Any) -> None:
        app.handle_error(pos, message, *args)

    class ObjectlikeFunction(typing.Protocol):
        def __call__(self, invoc_pos: _cp.Position) -> list[_ct.Token] | None:
            ...

    class FunctionlikeFunction(typing.Protocol):
        def __call__(self, invoc_pos: _cp.Position, arg: MacroArg) -> list[_ct.Token] | None:
            ...

    def add_objectlike_macro(name: str, func: ObjectlikeFunction) -> None:
        signature = _cd.MacroSignature(name, params=None, is_variadic=False)
        permanent = True
        require_paren = False
        def func_2(invoc_pos: _cp.Position, arguments: list[MacroArg] | None,
                   va_args: MacroArg | None) -> list[_ct.Token] | None:
            return func(invoc_pos)
        macro_registry[name] = BuiltinMacro(signature, permanent, require_paren, func_2)

    def add_functionlike_macro(name: str, func: FunctionlikeFunction) -> None:
        signature = _cd.MacroSignature(name, params=("_",), is_variadic=False)
        permanent = True
        require_paren = True
        def func_2(invoc_pos: _cp.Position, arguments: list[MacroArg] | None,
                   va_args: MacroArg | None) -> list[_ct.Token] | None:
            assert arguments is not None
            return func(invoc_pos, arguments[0])
        macro_registry[name] = BuiltinMacro(signature, permanent, require_paren, func_2)

    def file_func(invoc_pos: _cp.Position) -> list[_ct.Token] | None:
        root_invoc_pos = invoc_stack[0].invoc_pos if invoc_stack else invoc_pos
        if include_stack:
            top = include_stack[-1]
            assert root_invoc_pos.file_index == top.file_index
            source = top.source
        else:
            assert root_invoc_pos.file_index == root_file_index
            source = root_source
        resolved_position = source.position_tracker(root_invoc_pos.pos_in_file)
        text = _make_string_lit(resolved_position.path)
        token = _ct.Token(_ct.TokenType.STRING_LIT, text, invoc_pos, synthetic=True)
        return [token]

    def line_func(invoc_pos: _cp.Position) -> list[_ct.Token] | None:
        root_invoc_pos = invoc_stack[0].invoc_pos if invoc_stack else invoc_pos
        if include_stack:
            top = include_stack[-1]
            assert root_invoc_pos.file_index == top.file_index
            source = top.source
        else:
            assert root_invoc_pos.file_index == root_file_index
            source = root_source
        resolved_position = source.position_tracker(root_invoc_pos.pos_in_file)
        text = str(resolved_position.line_no)
        token = _ct.Token(_ct.TokenType.NUMBER, text, invoc_pos, synthetic=True)
        return [token]

    def pragma_func(invoc_pos: _cp.Position, arg: MacroArg) -> list[_ct.Token] | None:
        if len(arg.tokens) != 1 or arg.tokens[0].type_ is not _ct.TokenType.STRING_LIT:
            pos = arg.tokens[0].pos if arg.tokens else arg.end_pos
            error(pos, "Invalid argument in _Pragma() invocation: Must be a single string literal")
            return None
        token = arg.tokens[0]
        text = token.text
        # Since a string literal cannot have resolved UCNs before the closing quotation
        # mark, `text` and `token.reconstruct()` are equal at least up until the closing
        # quotation mark
        i = text.find('"')
        j = text.rfind('"')
        assert 0 <= i < j
        if j + 1 != len(text):
            pos = token.subpos(j + 1)
            error(pos, "Invalid argument in _Pragma() invocation: String literal must have no suffix")
            return None
        i += 1
        builder = _tp.PosMappedStringBuilder()
        builder.bump_ref_pos_to(i)
        while True:
            k = text.find("\\", i, j)
            if k < 0:
                builder.add_linear(text[i:j])
                break
            assert k + 1 < j  # Stray trailing backslash is not possible
            ch = text[k + 1]
            if ch not in '\\"':
                k += 2
                builder.add_linear(text[i:k])
                i = k
                continue
            builder.add_linear(text[i:k])
            builder.add_nonlinear(ch)
            builder.bump_ref_pos_by(2)
            i = k + 2
        pos_mapped_string = builder.finalize_and_get()
        offset_map = pos_mapped_string.pos_map if not token.synthetic else None
        synthesized_tokens = synthesize_tokens(pos_mapped_string.string, token.pos, offset_map,
                                               generate_error_messages=True)
        if synthesized_tokens is None:
            return None
        end_pos = token.subpos(j)
        app.handle_pragma(synthesized_tokens, invoc_pos, end_pos)
        return []

    add_objectlike_macro("__FILE__", file_func)
    add_objectlike_macro("__LINE__", line_func)
    add_functionlike_macro("_Pragma", pragma_func)

    # FIXME: Set up more built-in macros here        

    # FIXME: Support special built-in macros `__has_embed`, `__has_include`, and
    # `__has_cpp_attribute`        

    if config.predefined_macros is not None:
        for macro in config.predefined_macros:
            macro_registry[macro.signature.name] = macro

    text_pos_tracker = root_source.position_tracker.text_pos_tracker
    tokenizer = _ct.Tokenizer(root_source.input_, root_file_index, text_pos_tracker, error)
    directive_filter = _cd.DirectiveFilter(tokenizer, directive_handler)
    iterator = _TokenIterator(directive_filter, include_stack, error)
    error_occurred = _b.Wrap(False)
    try:
        for token in process(iterator, error_occurred):
            if token.type_ is not _ct.TokenType.BLOCKED_IDENTIFIER:
                yield token
                continue
            yield dataclasses.replace(token, type_=_ct.TokenType.IDENTIFIER)
    finally:
        for entry in include_stack:
            entry.source.input_.close()


@dataclasses.dataclass(slots=True, frozen=True)
class _IncludeStackEntry:
    file_index:       int
    source:           Source
    directive_filter: _cd.DirectiveFilter


@dataclasses.dataclass(slots=True)
class _InvocStackEntry:
    invoc_pos:   _cp.Position
    macro_name:  str
    replacement: list[_ct.Token]
    read_pos:    int = 0


@dataclasses.dataclass(slots=True, frozen=True)
class _ArgDelim:
    begin: int
    end:   int
    pos:   _cp.Position  # Position of comma or closing parenthesis


@dataclasses.dataclass(slots=True)
class _ArgSlot:
    arg:         MacroArg
    expanded:    list[_ct.Token] | None = None
    stringified: _ct.Token | None       = None


class _ExpandError(Exception):
    pass


class _TokenIterator(_cabc.Iterator[_ct.Token]):
    def __init__(self, root_directive_filter: _cd.DirectiveFilter, include_stack: list[_IncludeStackEntry],
                 error_handler: _ct.ErrorHandler) -> None:
        self._root_directive_filter = root_directive_filter
        self._include_stack = include_stack
        self._error_handler = error_handler
        self._at_end = False

    @typing.override
    def __next__(self) -> _ct.Token:
        while True:
            if not self._include_stack:
                if self._at_end:
                    raise StopIteration
                token = self._root_directive_filter.next_token()
                if token is None:
                    continue
                break
            top = self._include_stack[-1]
            token = top.directive_filter.next_token()
            if token is None:
                continue
            if token.type_ is not _ct.TokenType.END_OF_INPUT:
                break
            top.source.input_.close()
            self._include_stack.pop()
        token_type = token.type_
        if token_type is _ct.TokenType.IDENTIFIER:
            if not _cd.verify_no_va_names(token.text, token.pos, self._error_handler, synthetic=False):
                token = dataclasses.replace(token, type_=_ct.TokenType.BAD_IDENTIFIER)
        elif token_type is _ct.TokenType.END_OF_INPUT:
            self._at_end = True
        return token


def _make_string_lit(string: str) -> str:
    parts: list[str] = []
    for ch in string:
        result = _SIMPLE_ESCAPES.get(ch)
        if result is not None:
            part = result
        elif ch.isprintable():
            part = ch
        else:
            code_point = ord(ch)
            if 0xD800 <= code_point <= 0xDFFF:
                code_point = 0xFFFD  # Unicode replacement character
            if code_point <= 0xFFFF:
                part = "\\u%04X" % code_point
            else:
                part = "\\U%08X" % code_point
        parts.append(part)
    return '"%s"' % "".join(parts)


_SIMPLE_ESCAPES = {
    '\\': '\\\\',
    '"':  '\\"',
    '\a': '\\a',
    '\b': '\\b',
    '\f': '\\f',
    '\n': '\\n',
    '\r': '\\r',
    '\t': '\\t',
    '\v': '\\v',
}


def _same_macro_signature(a: _cd.MacroSignature, b: _cd.MacroSignature) -> bool:
    return (a.name == b.name and
            a.params == b.params and
            (a.params is None or a.is_variadic == b.is_variadic))

def _same_replacement_list(a: _cabc.Sequence[_cd.ReplaceElem], b: _cabc.Sequence[_cd.ReplaceElem]) -> bool:
    def same_seq(a: _cabc.Sequence[_cd.ReplaceElem], b: _cabc.Sequence[_cd.ReplaceElem]) -> bool:
        i = 0
        j = 0
        m = len(a)
        n = len(b)
        while i < m and j < n:
            e = a[i]
            f = b[j]
            i += 1
            j += 1
            # All runs of whitespace need to be considered the same
            s = isinstance(e, _ct.Token) and e.type_ in _ct.SPACE
            t = isinstance(f, _ct.Token) and f.type_ in _ct.SPACE
            if s or t:
                if not (s and t):
                    return False
                while i < m:
                    match a[i]:
                        case _ct.Token() as token:
                            if token.type_ in _ct.SPACE:
                                i += 1
                                continue
                    break
                while j < n:
                    match b[j]:
                        case _ct.Token() as token:
                            if token.type_ in _ct.SPACE:
                                j += 1
                                continue
                    break
                continue
            if not same_elem(e, f):
                return False
        return i == m and j == n

    def same_elem(a: _cd.ReplaceElem, b: _cd.ReplaceElem) -> bool:
        match a:
            case _ct.Token():
                return isinstance(b, _ct.Token) and a.type_ is b.type_ and a.text == b.text
            case _cd.ParamRef():
                return isinstance(b, _cd.ParamRef) and a.param_index == b.param_index
            case _cd.VaArgs():
                return isinstance(b, _cd.VaArgs)
            case _cd.VaOpt():
                return isinstance(b, _cd.VaOpt) and same_seq(a.elements, b.elements)
            case _cd.StringifyOperation():
                return isinstance(b, _cd.StringifyOperation) and same_elem(a.operand, b.operand)
            case _cd.FuseOperation():
                if not isinstance(b, _cd.FuseOperation):
                    return False
                if not same_elem(a.left, b.left):
                    return False
                if len(a.rights) != len(b.rights):
                    return False
                for r, s in zip(a.rights, b.rights):
                    if not same_elem(r.operand, s.operand):
                        return False
                return True
            case _:
                typing.assert_never(a)

    return same_seq(a, b)
