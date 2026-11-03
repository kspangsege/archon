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
import io
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.cpp_preprocess._tokenize as _ct
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
def preprocess(source: Source, file_index: int, tracker: _tp.TextPosTracker, context: Context,
               predefined_macros: _cabc.Sequence[BuiltinMacro] | None = None) -> _cabc.Iterator[_ct.Token]:
    return _preprocess(source, file_index, tracker, context, predefined_macros)


@dataclasses.dataclass(slots=True, frozen=True)
class Source:
    input_: typing.TextIO
    path:   pathlib.Path


class Context(abc.ABC):
    @abc.abstractmethod
    def handle_error(self, pos: _ct.Position, message: str, *args: typing.Any) -> None:
        ...

    # Called with the tokens of the pragma directive minus the leading `#pragma` and minus
    # any minus whitespace that follows immediately after `#pragma`. Since trailing
    # whitespace is not considered part of a directive, the passed token sequence will
    # neither have have leading nor trailing whitespace.
    #
    def handle_pragma(self, tokens: _cabc.Sequence[_ct.Token], pos: _ct.Position, end_pos: _ct.Position) -> None:
        pass


type Macro = BuiltinMacro | RegularMacro

@dataclasses.dataclass(slots=True, frozen=True)
class MacroBase:
    signature: _cd.MacroSignature

@dataclasses.dataclass(slots=True, frozen=True)
class BuiltinMacro(MacroBase):
    permanent: bool
    func:      MacroFunction

@dataclasses.dataclass(slots=True, frozen=True)
class RegularMacro(MacroBase):
    replacement:    tuple[_cd.ReplaceElem, ...]
    definition_pos: _ct.Position


# When invoked by the preprocessor, the preprocessor guarantees that `arguments` is not
# `None` when and only when the macro is function-like, and it guarantees that `va_args` is
# not `None` when and only when the macro is variadic. The macro can only be variadic if it
# is function-like.
#
# The function must return `None` if and only if an error occurred.
#
class MacroFunction(typing.Protocol):
    def __call__(self, invoke_pos: _ct.Position, arguments: list[MacroArg] | None,
                 va_args: MacroArg | None) -> list[_ct.Token] | None:
        ...


@dataclasses.dataclass(slots=True)
class MacroArg:
    tokens:  list[_ct.Token]
    end_pos: _ct.Position








def _preprocess(source: Source, file_index: int, tracker: _tp.TextPosTracker, context: Context,
                predefined_macros: _cabc.Sequence[BuiltinMacro] | None) -> _cabc.Iterator[_ct.Token]:
    macro_registry: dict[str, Macro] = {}
    active_set: set[str] = set()
    in_argument = False

    def process(tokens_iter: typing.Iterator[_ct.Token]) -> _cabc.Iterator[_ct.Token]:
        nonlocal in_argument

        stack: list[_StackEntry] = []
        def next_token() -> _ct.Token:
            while stack:
                entry = stack[-1]
                if entry.pos < len(entry.replacement):
                    token = entry.replacement[entry.pos]
                    entry.pos += 1
                    return token
                pop()
            return next(tokens_iter)

        def push(macro_name: str, replacement: list[_ct.Token]) -> None:
            assert macro_name not in active_set
            active_set.add(macro_name)
            entry = _StackEntry(macro_name, replacement)
            stack.append(entry)

        def pop() -> None:
            entry = stack.pop()
            active_set.remove(entry.macro_name)

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
            invoke_pos = token.pos
            signature = macro.signature
            if signature.params is None:
                # Object-like macro
                match macro:
                    case BuiltinMacro():
                        replacement = macro.func(invoke_pos, arguments=None, va_args=None)
                    case RegularMacro():
                        replacement = expand_replacement(signature, macro.replacement, arguments=None, va_args=None)
                    case _:
                        typing.assert_never(macro)
                if replacement:
                    push(name, replacement)
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
                token = next_token()
                continue

            arguments = [get_arg(i, i + 1) for i in range(n)]
            va_args = None
            if is_variadic:
                va_args = get_arg(n, len(arg_delims))
            match macro:
                case BuiltinMacro():
                    replacement = macro.func(invoke_pos, arguments, va_args)
                case RegularMacro():
                    replacement = expand_replacement(signature, macro.replacement, arguments, va_args)
                case _:
                    typing.assert_never(macro)
            if replacement:
                push(name, replacement)
            token = next_token()

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
                def add(token: _ct.Token, operator_pos: _ct.Position) -> None:
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
                slot.expanded = preexpand_arg(arg.tokens, arg.end_pos)
            return slot.expanded

        def stringify_arg(param_index: int | None, operator_pos: _ct.Position) -> _ct.Token:
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

    def preexpand_arg(tokens: _cabc.Sequence[_ct.Token], end_pos: _ct.Position) -> list[_ct.Token]:
        tokens_2 = list(tokens) + [_ct.Token(_ct.TokenType.END_OF_INPUT, "", end_pos)]
        result: list[_ct.Token] = []
        for token in process(iter(tokens_2)):
            if token.type_ is _ct.TokenType.END_OF_INPUT:
                break
            result.append(token)
        return result

    def stringify(tokens: list[_ct.Token], operator_pos: _ct.Position) -> _ct.Token:
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

    def fuse_tokens(left: _ct.Token, right: _ct.Token, operator_pos: _ct.Position) -> _ct.Token:
        text = left.reconstruct() + right.reconstruct()
        token = synthesize_token(text, operator_pos)
        if token is None:
            error(operator_pos, "Fusion %s is not a valid preprocessing token" % _b.clamped_quote(text, 64))
            raise _ExpandError
        if token.type_ is _ct.TokenType.IDENTIFIER:
            if not _cd.verify_no_va_names(token.text, operator_pos, error, synthetic=True):
                raise _ExpandError
        return token

    def synthesize_token(text: str, pos: _ct.Position) -> _ct.Token | None:
        dummy_file_index = 0
        dummy_tracker = _tp.TextPosTracker()
        with io.StringIO(text) as input_:
            has_error = False
            def error_handler(pos: _ct.Position, message: str, *args: typing.Any) -> None:
                nonlocal has_error
                has_error = True
            tokenizer = _ct.Tokenizer(input_, dummy_file_index, dummy_tracker, error_handler)
            token_1 = tokenizer.next_token()
            if token_1.type_ is _ct.TokenType.END_OF_INPUT:
                return None
            token_2 = tokenizer.next_token()
            if token_2.type_ is not _ct.TokenType.END_OF_INPUT:
                return None
            if has_error or token_1.text != text:
                return None
            return dataclasses.replace(token_1, pos=pos, synthetic=True)

    def directive_handler(tokens: _cabc.Sequence[_ct.Token], pos: _ct.Position, end_pos: _ct.Position) -> None:
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
                    error(pos, "Cannot undefine permanent built-in macro %s",
                          _b.clamped_quote(undef_directive.name, 64))
                    return
                del macro_registry[undef_directive.name]
                return
            case _cd.DirectiveType.PRAGMA:
                context.handle_pragma(remaining_tokens, pos, end_pos)
                return
        raise NotImplementedError         

    def error(pos: _ct.Position, message: str, *args: typing.Any) -> None:
        context.handle_error(pos, message, *args)

    def add_objectlike_macro(name: str, func: MacroFunction) -> None:
        signature = _cd.MacroSignature(name, params=None, is_variadic=False)
        permanent = True
        macro_registry[name] = BuiltinMacro(signature, permanent, func)

    def file_func(invoke_pos: _ct.Position, arguments: list[MacroArg] | None,
                  va_args: MacroArg | None) -> list[_ct.Token] | None:
        text = _make_string_lit(str(source.path))
        token = _ct.Token(_ct.TokenType.STRING_LIT, text, invoke_pos, synthetic=True)
        return [token]

    def line_func(invoke_pos: _ct.Position, arguments: list[MacroArg] | None,
                  va_args: MacroArg | None) -> list[_ct.Token] | None:
        text = str(tracker.line_no)
        token = _ct.Token(_ct.TokenType.NUMBER, text, invoke_pos, synthetic=True)
        return [token]

    add_objectlike_macro("__FILE__", file_func)
    add_objectlike_macro("__LINE__", line_func)

    # FIXME: Set up more built-in macros here        

    # FIXME: Support special built-in macros `__has_embed`, `__has_include`, and
    # `__has_cpp_attribute`        

    if predefined_macros is not None:
        for macro in predefined_macros:
            macro_registry[macro.signature.name] = macro

    tokenizer = _ct.Tokenizer(source.input_, file_index, tracker, error)
    directive_filter = _cd.DirectiveFilter(tokenizer, directive_handler)
    iterator = _TokenIterator(directive_filter, error)
    for token in process(iterator):
        if token.type_ is not _ct.TokenType.BLOCKED_IDENTIFIER:
            yield token
            continue
        yield dataclasses.replace(token, type_=_ct.TokenType.IDENTIFIER)


@dataclasses.dataclass(slots=True)
class _StackEntry:
    macro_name:  str
    replacement: list[_ct.Token]
    pos:         int = 0


@dataclasses.dataclass(slots=True, frozen=True)
class _ArgDelim:
    begin: int
    end:   int
    pos:   _ct.Position  # Position of comma or closing parenthesis


@dataclasses.dataclass(slots=True)
class _ArgSlot:
    arg:         MacroArg
    expanded:    list[_ct.Token] | None = None
    stringified: _ct.Token | None       = None


class _ExpandError(Exception):
    pass


class _TokenIterator(_cabc.Iterator[_ct.Token]):
    def __init__(self, directive_filter: _cd.DirectiveFilter, error_handler: _ct.ErrorHandler) -> None:
        self._directive_filter = directive_filter
        self._error_handler = error_handler
        self._at_end = False

    @typing.override
    def __next__(self) -> _ct.Token:
        if self._at_end:
            raise StopIteration
        token = self._directive_filter.next_token()
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
                # FIXME: Should this instead be a hard error?    
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
