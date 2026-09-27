from __future__ import annotations

import typing
import dataclasses
import collections.abc
import enum
import re
import sys
import pathlib
import io

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cpp_preprocess as _cp
import archon.cmake.policy as _cp2


MACRO_NAME = "CM_FOR_EACH_POLICY_TABLE"
MACRO_PARAMS = ["POLICY", "SELECT"]


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

def extract_definition() -> _cp.DefineDirective:
    try:
        with open(path) as file_:
            initial = True
            dummy_file_index = 0
            for elem in _cp.parse(file_, dummy_file_index, tracker, error_handler):
                match elem:
                    case _cp.DefineDirective() as define:
                        if define.name == MACRO_NAME:
                            if define.is_variadic:
                                error_handler(define.pos, "Unexpected variadic macro")
                            if define.params != MACRO_PARAMS:
                                error_handler(define.pos, "Unexpected macro parameters")
                            return define
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.END_OF_INPUT:
                            error_handler(token.pos, "Macro %s not found" % MACRO_NAME)
    except FileNotFoundError as e:
        logger.error("Failed to open %s: %s", _b.quote(str(path)), e.strerror)
    assert False

def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
    assert pos.file_index == 0
    text_pos = tracker.get_text_pos(pos.pos_in_file)
    context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
    _l.FileContextLogger(logger, context).error(message, *args)
    raise Error from None

def get_replace_elem_pos(elem: _cp.ReplaceElem) -> _cp.Position:
    return elem.left.pos if isinstance(elem, _cp.FuseOper) else elem.pos

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

@dataclasses.dataclass(slots=True, frozen=True)
class Policy:
    num:           int
    description:   str
    version_major: int
    version_minor: int
    version_patch: int

policies = list[Policy]()

try:
    define = extract_definition()
    state = State.INITIAL
    for elem in define.replacement:
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
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected start of %s(...) invocation" % MACRO_PARAMS[1])
                assert False
            case State.NEED_LPAREN:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.PUNCT and token.text == "(":
                            state = State.NEED_FIRST_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected opening parenthesis of %s(...) invocation" % MACRO_PARAMS[1])
                assert False
            case State.NEED_FIRST_ARG:
                match elem:
                    case _cp.ParamRef() as ref:
                        if ref.param_index == 0:
                            state = State.NEED_COMMA_AFTER_FIRST_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected %s argument" % MACRO_PARAMS[0])
                assert False
            case State.NEED_COMMA_AFTER_FIRST_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                            state = State.NEED_POLICY_ID_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected comma after %s argument" % MACRO_PARAMS[0])
                assert False
            case State.NEED_POLICY_ID_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.IDENTIFIER:
                            if m := re.fullmatch(r"CMP(\d+)", token.text, re.ASCII):
                                policy_num = int(m.group(1))
                                state = State.NEED_COMMA_AFTER_POLICY_ID_ARG
                                continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected policy ID argument")
                assert False
            case State.NEED_COMMA_AFTER_POLICY_ID_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                            state = State.NEED_DESCRIPTION_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected comma after policy ID argument")
                assert False
            case State.NEED_DESCRIPTION_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.STRING_LIT:
                            string = _cp.unpack_plain_string_lit(token.text, token.pos, token.derived, error_handler)
                            assert string is not None
                            description = string
                            state = State.NEED_COMMA_AFTER_DESCRIPTION_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected description argument")
                assert False
            case State.NEED_COMMA_AFTER_DESCRIPTION_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.STRING_LIT:
                            string = _cp.unpack_plain_string_lit(token.text, token.pos, token.derived, error_handler)
                            assert string is not None
                            description += string
                            continue
                        if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                            state = State.NEED_VERSION_MAJOR_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected comma after description argument")
                assert False
            case State.NEED_VERSION_MAJOR_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.NUMBER and re.fullmatch(r"\d+", token.text, re.ASCII):
                            version_major = int(token.text)
                            state = State.NEED_COMMA_AFTER_VERSION_MAJOR_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected major version component argument")
                assert False
            case State.NEED_COMMA_AFTER_VERSION_MAJOR_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                            state = State.NEED_VERSION_MINOR_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected comma after major version component argument")
                assert False
            case State.NEED_VERSION_MINOR_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.NUMBER and re.fullmatch(r"\d+", token.text, re.ASCII):
                            version_minor = int(token.text)
                            state = State.NEED_COMMA_AFTER_VERSION_MINOR_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected minor version component argument")
                assert False
            case State.NEED_COMMA_AFTER_VERSION_MINOR_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                            state = State.NEED_VERSION_PATCH_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected comma after minor version component argument")
                assert False
            case State.NEED_VERSION_PATCH_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.NUMBER and re.fullmatch(r"\d+", token.text, re.ASCII):
                            version_patch = int(token.text)
                            state = State.NEED_COMMA_AFTER_VERSION_PATCH_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected patch version component argument")
                assert False
            case State.NEED_COMMA_AFTER_VERSION_PATCH_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.PUNCT and token.text == ",":
                            state = State.NEED_STATUS_ARG
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected comma after patch version component argument")
                assert False
            case State.NEED_STATUS_ARG:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.IDENTIFIER and token.text in {"NEW", "WARN"}:
                            state = State.NEED_RPAREN
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected status argument")
                assert False
            case State.NEED_RPAREN:
                match elem:
                    case _cp.Token() as token:
                        if token.type_ is _cp.TokenType.PUNCT and token.text == ")":
                            policies.append(Policy(policy_num, description, version_major, version_minor,
                                                   version_patch))
                            state = State.INITIAL
                            continue
                pos = get_replace_elem_pos(elem)
                error_handler(pos, "Expected closing parenthesis of %s(...) invocation" % MACRO_PARAMS[1])
                assert False
        typing.assert_never(state)
except Error:
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
