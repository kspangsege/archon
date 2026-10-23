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
import dataclasses
import collections.abc as _cabc
import enum
import re
import sys
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cpp_preprocess as _cp
import archon.cmake.policy as _cp2


MACRO_NAME = "CM_FOR_EACH_POLICY_TABLE"
MACRO_PARAMS = ("POLICY", "SELECT")


help_     = _b.Wrap(False)
log_level = _b.Wrap(_l.LogLevel.INFO)

spec = _cli.Spec()
spec.opt(["--"], _cli.Stop())
spec.opt(["-h", "--help"], _cli.ShortCircuit(help_))
spec.opt(["-l", "--log-level"], _cli.AssignWithArg(_l.parse_log_level, log_level))


root_logger = _l.RootLogger()
success, args = _cli.parse(sys.argv[1:], spec, root_logger)
if not success:
    sys.exit(1)
if help_.value:
    _cli.show_help(spec, root_logger)
    sys.exit(0)
if len(args) != 1:
    root_logger.error("Wrong number of command-line arguments (try --help)")
    sys.exit(1)

path = pathlib.Path(args[0])

logger = _l.LimitLogger(root_logger, log_level.value)

class Error(Exception):
    pass

tracker = _tp.TextPosTracker()

def extract_macro_definition() -> _cp.MacroDef:
    macro_def: _cp.MacroDef | None = None
    class Found(Exception):
        pass
    try:
        with open(path) as file_:
            file_index = 0
            tokenizer = _cp.Tokenizer(file_, file_index, tracker, error_handler)
            def directive_handler(tokens: _cabc.Sequence[_cp.Token], pos: _cp.Position, end_pos: _cp.Position) -> None:
                nonlocal macro_def
                result = _cp.preparse_directive(tokens, error_handler)
                assert result
                directive_type, remaining_tokens = result
                if directive_type is not _cp.DirectiveType.DEFINE:
                    return
                def_ = _cp.parse_define_directive(remaining_tokens, pos, end_pos, error_handler)
                assert def_
                if def_.name != MACRO_NAME:
                    return
                if def_.is_variadic:
                    error_handler(pos, "Unexpected variadic macro")
                    assert False
                if def_.params != MACRO_PARAMS:
                    error_handler(pos, "Unexpected macro parameters")
                    assert False
                macro_def = def_
                raise Found
            detector = _cp.DirectiveDetector(tokenizer, directive_handler)
            while True:
                token = detector.next_token()
                if token.type_ is _cp.TokenType.END_OF_INPUT:
                    error_handler(token.pos, "Macro %s not found" % MACRO_NAME)
                    assert False
    except FileNotFoundError as e:
        logger.error("Failed to open %s: %s", _b.quote(str(path)), e.strerror)
        assert False
    except Found:
        assert macro_def
        return macro_def
    assert False

def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
    assert pos.file_index == 0
    text_pos = tracker.get_text_pos(pos.pos_in_file)
    context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
    _l.FileContextLogger(logger, context).error(message, *args)
    raise Error from None


def extract_policies() -> list[Policy] | None:
    policies: list[Policy] = []
    try:
        macro_def = extract_macro_definition()
        state = State.INITIAL
        for elem in macro_def.replacement:
            match elem:
                case _cp.Token() as token:
                    if token.is_space():
                        continue
            match state:
                case State.INITIAL:
                    match elem:
                        case _cp.ParamRef() as ref:
                            if ref.param_index == 1:
                                state = State.NEED_LPAREN
                                continue
                    error_handler(elem.pos, "Expected start of %s(...) invocation" % MACRO_PARAMS[1])
                    assert False
                case State.NEED_LPAREN:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.PUNCT and token.text == "(":
                                state = State.NEED_FIRST_ARG
                                continue
                    error_handler(elem.pos, "Expected opening parenthesis of %s(...) invocation" % MACRO_PARAMS[1])
                    assert False
                case State.NEED_FIRST_ARG:
                    match elem:
                        case _cp.ParamRef() as ref:
                            if ref.param_index == 0:
                                state = State.NEED_COMMA_AFTER_FIRST_ARG
                                continue
                    error_handler(elem.pos, "Expected %s argument" % MACRO_PARAMS[0])
                    assert False
                case State.NEED_COMMA_AFTER_FIRST_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                                state = State.NEED_POLICY_ID_ARG
                                continue
                    error_handler(elem.pos, "Expected comma after %s argument" % MACRO_PARAMS[0])
                    assert False
                case State.NEED_POLICY_ID_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.IDENTIFIER:
                                if m := re.fullmatch(r"CMP(\d+)", token.text, re.ASCII):
                                    policy_num = int(m.group(1))
                                    state = State.NEED_COMMA_AFTER_POLICY_ID_ARG
                                    continue
                    error_handler(elem.pos, "Expected policy ID argument")
                    assert False
                case State.NEED_COMMA_AFTER_POLICY_ID_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                                state = State.NEED_DESCRIPTION_ARG
                                continue
                    error_handler(elem.pos, "Expected comma after policy ID argument")
                    assert False
                case State.NEED_DESCRIPTION_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.STRING_LIT:
                                string = unpack_string_literal(token.text, token.pos, token.synthetic)
                                description = string
                                state = State.NEED_COMMA_AFTER_DESCRIPTION_ARG
                                continue
                    error_handler(elem.pos, "Expected description argument")
                    assert False
                case State.NEED_COMMA_AFTER_DESCRIPTION_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.STRING_LIT:
                                string = unpack_string_literal(token.text, token.pos, token.synthetic)
                                description += string
                                continue
                            if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                                state = State.NEED_VERSION_MAJOR_ARG
                                continue
                    error_handler(elem.pos, "Expected comma after description argument")
                    assert False
                case State.NEED_VERSION_MAJOR_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.NUMBER and re.fullmatch(r"\d+", token.text, re.ASCII):
                                version_major = int(token.text)
                                state = State.NEED_COMMA_AFTER_VERSION_MAJOR_ARG
                                continue
                    error_handler(elem.pos, "Expected major version component argument")
                    assert False
                case State.NEED_COMMA_AFTER_VERSION_MAJOR_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                                state = State.NEED_VERSION_MINOR_ARG
                                continue
                    error_handler(elem.pos, "Expected comma after major version component argument")
                    assert False
                case State.NEED_VERSION_MINOR_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.NUMBER and re.fullmatch(r"\d+", token.text, re.ASCII):
                                version_minor = int(token.text)
                                state = State.NEED_COMMA_AFTER_VERSION_MINOR_ARG
                                continue
                    error_handler(elem.pos, "Expected minor version component argument")
                    assert False
                case State.NEED_COMMA_AFTER_VERSION_MINOR_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                                state = State.NEED_VERSION_PATCH_ARG
                                continue
                    error_handler(elem.pos, "Expected comma after minor version component argument")
                    assert False
                case State.NEED_VERSION_PATCH_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.NUMBER and re.fullmatch(r"\d+", token.text, re.ASCII):
                                version_patch = int(token.text)
                                state = State.NEED_COMMA_AFTER_VERSION_PATCH_ARG
                                continue
                    error_handler(elem.pos, "Expected patch version component argument")
                    assert False
                case State.NEED_COMMA_AFTER_VERSION_PATCH_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                                state = State.NEED_STATUS_ARG
                                continue
                    error_handler(elem.pos, "Expected comma after patch version component argument")
                    assert False
                case State.NEED_STATUS_ARG:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.IDENTIFIER and token.text in {"NEW", "WARN"}:
                                state = State.NEED_RPAREN
                                continue
                    error_handler(elem.pos, "Expected status argument")
                    assert False
                case State.NEED_RPAREN:
                    match elem:
                        case _cp.Token() as token:
                            if token.type_ is _cp.TokenType.PUNCT and token.text == ")":
                                policies.append(Policy(policy_num, description, version_major, version_minor,
                                                       version_patch))
                                state = State.INITIAL
                                continue
                    error_handler(elem.pos, "Expected closing parenthesis of %s(...) invocation" % MACRO_PARAMS[1])
                    assert False
            typing.assert_never(state)
    except Error:
        return None
    return policies


@dataclasses.dataclass(slots=True, frozen=True)
class Policy:
    num:           int
    description:   str
    version_major: int
    version_minor: int
    version_patch: int


class State(enum.Enum):
    INITIAL                            = enum.auto()
    NEED_LPAREN                        = enum.auto()
    NEED_FIRST_ARG                     = enum.auto()
    NEED_COMMA_AFTER_FIRST_ARG         = enum.auto()
    NEED_POLICY_ID_ARG                 = enum.auto()
    NEED_COMMA_AFTER_POLICY_ID_ARG     = enum.auto()
    NEED_DESCRIPTION_ARG               = enum.auto()
    NEED_COMMA_AFTER_DESCRIPTION_ARG   = enum.auto()
    NEED_VERSION_MAJOR_ARG             = enum.auto()
    NEED_COMMA_AFTER_VERSION_MAJOR_ARG = enum.auto()
    NEED_VERSION_MINOR_ARG             = enum.auto()
    NEED_COMMA_AFTER_VERSION_MINOR_ARG = enum.auto()
    NEED_VERSION_PATCH_ARG             = enum.auto()
    NEED_COMMA_AFTER_VERSION_PATCH_ARG = enum.auto()
    NEED_STATUS_ARG                    = enum.auto()
    NEED_RPAREN                        = enum.auto()


def unpack_string_literal(text: str, pos: _cp.Position, synthetic: bool) -> str:
    unpacked = _cp.unpack_string_literal(text, pos, synthetic, error_handler)
    assert unpacked
    if unpacked.literal_type is not _cp.StringLiteralType.PLAIN:
        error_handler(pos, "Unexpected non-plain string literal")
        assert False
    if unpacked.suffix:
        error_handler(pos, "Unexpected string literal suffix")
        assert False
    return unpacked.payload




policies = extract_policies()
if policies is None:
    sys.exit(1)

toggleable = set(_cp2.Policy.__members__)

for policy in policies:
    policy_id = "CMP%04d" % policy.num
    descr = policy.description
    if descr.endswith("."):
        descr = descr[:-1]
    version = "_cve.Version(%s, %s, %s)" % (policy.version_major, policy.version_minor, policy.version_patch)
    if policy_id in toggleable:
        invoc = "_define_policy(%r, %r, %s, toggleable=Policy.%s)" % (policy_id, descr, version, policy_id)
    else:
        invoc = "_define_policy(%r, %r, %s)" % (policy_id, descr, version)
    print(invoc)
