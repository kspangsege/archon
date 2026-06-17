from __future__ import annotations
from typing import Protocol, Any, assert_never
from dataclasses import dataclass

import enum
import pathlib

import archon.text_pos as _tp
import archon.log as _l
import archon.cmake.lowlevel_parser as _clp
import archon.cmake.string_parser as _csp


def process(cmake_path: pathlib.Path, logger: _l.Logger) -> bool:
    return _process(cmake_path, logger)


def _process(cmake_path: pathlib.Path, logger: _l.Logger) -> bool:
    commands    = dict[str, _Command]()
    cache       = dict[str, _CacheEntry]()
    environment = dict[str, _Value]()
    _define_built_in_commands(commands)

    def process_file(cmake_path: pathlib.Path, directory: _Directory) -> None:
        file_pos_tracker = _tp.FilePosTracker(cmake_path)
        def warning_handler(pos: int, message: str, *args: Any) -> None:
            warning(file_pos_tracker, pos, message, *args)
        def error_handler(pos: int, message: str, *args: Any) -> None:
            error(file_pos_tracker, pos, message, *args)
        for invoc in _clp.parse(file_pos_tracker, warning_handler, error_handler):
            match invoc:
                case _clp.SimpleInvoc():
                    process_simple(invoc, file_pos_tracker, directory)
                    continue
                case _clp.IfInvoc():
                    process_if(invoc, file_pos_tracker, directory)
                    continue
                case _clp.ForeachInvoc():
                    process_foreach(invoc, file_pos_tracker, directory)
                    continue
                case _clp.WhileInvoc():
                    process_while(invoc, file_pos_tracker, directory)
                    continue
                case _clp.MacroDefInvoc():
                    process_macro(invoc, file_pos_tracker, directory)
                    continue
                case _clp.FunctionDefInvoc():
                    process_function(invoc, file_pos_tracker, directory)
                    continue
                case _clp.BlockInvoc():
                    process_block(invoc, file_pos_tracker, directory)
                    continue
            assert_never(invoc)

    def process_simple(invoc: _clp.SimpleInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        command = commands.get(invoc.command_name_cf)
        if not command:
            error(tracker, invoc.pos, "Invocation of undefined command %s()", invoc.command_name)
            return
        match command:
            case _BuiltInCommand(which):
                match which:
                    case _BuiltInCommand.Which.UNSUPPORTED:
                        error(tracker, invoc.pos, "Invocation of unsupported command %s()", invoc.command_name)
                        return
                    case _BuiltInCommand.Which.SET:
                        process_set(invoc, tracker, directory)
                        return
                    case _BuiltInCommand.Which.UNSET:
                        process_unset(invoc, tracker, directory)
                        return
                    case _BuiltInCommand.Which.MESSAGE:
                        process_message(invoc, tracker, directory)
                        return
                    case _BuiltInCommand.Which.INCLUDE:
                        process_include(invoc, tracker, directory)
                        return
                    case _BuiltInCommand.Which.ADD_SUBDIRECTORY:
                        process_add_subdirectory(invoc, tracker, directory)
                        return
                assert_never(which)
        assert_never(command)

    def process_if(invoc: _clp.IfInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_foreach(invoc: _clp.ForeachInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_while(invoc: _clp.WhileInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_macro(invoc: _clp.MacroDefInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_function(invoc: _clp.FunctionDefInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_block(invoc: _clp.BlockInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_set(invoc: _clp.SimpleInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        arguments = expand_arguments(invoc, tracker, directory)
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_unset(invoc: _clp.SimpleInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_message(invoc: _clp.SimpleInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_include(invoc: _clp.SimpleInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_add_subdirectory(invoc: _clp.SimpleInvoc, tracker: _tp.FilePosTracker, directory: _Directory) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def expand_arguments(invoc: _clp.InvocBase, tracker: _tp.FilePosTracker, directory: _Directory) -> list[_Argument]:
        arguments = list[_Argument]()
        for protoarg in invoc.arguments:
            i = protoarg.prefix_size
            j = len(protoarg.text) - protoarg.suffix_size
            string = protoarg.text[i:j]
            pos = protoarg.pos + i
            arg: _Argument
            match protoarg.type_:
                case _clp.Protoargument.Type.BARE:
                    was_quoted_or_bracketed = False
                    result = expand_string(string, pos, tracker, directory)
                    match result:
                        case _CertainExpansionResult():
                            segments, is_derived = _list_split(result.string, result.is_derived)
                            for segment in segments:
                                arg = _CertainArgument(was_quoted_or_bracketed, protoarg.pos, segment, is_derived)
                                arguments.append(arg)
                            continue
                        case _UncertainExpansionResult():
                            # Note that an uncertain unquoted proto-argument stands in for
                            # any number of actual arguments, including zero.
                            arg = _UncertainArgument(was_quoted_or_bracketed, protoarg.pos, result.reason)
                            arguments.append(arg)
                            continue
                    assert_never(result)
                case _clp.Protoargument.Type.QUOTED:
                    was_quoted_or_bracketed = True
                    result = expand_string(string, pos, tracker, directory)
                    match result:
                        case _CertainExpansionResult():
                            arg = _CertainArgument(was_quoted_or_bracketed, protoarg.pos, result.string,
                                                   result.is_derived)
                            arguments.append(arg)
                            continue
                        case _UncertainExpansionResult():
                            arg = _UncertainArgument(was_quoted_or_bracketed, protoarg.pos, result.reason)
                            arguments.append(arg)
                            continue
                    assert_never(result)
                case _clp.Protoargument.Type.BRACKETED:
                    was_quoted_or_bracketed = True
                    string_2 = _tp.PosMappedString.from_linear_string(string, pos)
                    is_derived = False
                    arg = _CertainArgument(was_quoted_or_bracketed, protoarg.pos, string_2, is_derived)
                    arguments.append(arg)
                    continue
            assert_never(protoarg.type_)
        return arguments

    def expand_string(string: str, pos: int, tracker: _tp.FilePosTracker, directory: _Directory) -> _ExpansionResult:
        def expand(expr: _csp.Expr) -> _ExpansionResult:
            if isinstance(expr, _csp.StringExpr):
                is_derived = False
                return _CertainExpansionResult(expr.string, is_derived)
            if isinstance(expr, _csp.CompositeExpr):
                string_builder = _tp.PosMappedStringBuilder(expr.pos)
                for part in expr.parts:
                    result = expand(part)
                    if isinstance(result, _CertainExpansionResult):
                        string_builder.add_pos_mapped_string(result.string)
                        continue
                    if isinstance(result, _UncertainExpansionResult):
                        return result  
                    assert_never(result)
                is_derived = True
                return _CertainExpansionResult(string_builder.get(), is_derived)
            if isinstance(expr, _csp.ExpansionExpr):
                result = expand(expr.name_expr)
                if isinstance(result, _CertainExpansionResult):
                    name = result.string.string
                    value = resolve_variable(expr.resolution_type, name, expr.name_expr.pos, tracker, directory)
                    if isinstance(value, _CertainValue):
                        string = _tp.PosMappedString.from_nonlinear_string(value.string or "", expr.pos)
                        is_derived = True
                        return _CertainExpansionResult(string, is_derived)
                    if isinstance(value, _UncertainValue):
                        assert False                
                    assert_never(value)
                if isinstance(result, _UncertainExpansionResult):
                    return result                
                assert_never(result)
            assert_never(expr)
        def error_handler(pos: int, message: str, *args: Any) -> None:
            error(tracker, pos, message, *args)
        expr = _csp.parse(string, pos, error_handler)
        return expand(expr)

    # def expand_arguments(invoc: _clp.InvocBase, cmake_path: pathlib.Path, directory: _Directory) -> list[_Argument]:
    #     def evaluate(expr: _clp.Expr) -> tuple[_Value, bool]:
    #         if isinstance(expr, _clp.StringExpr):
    #             is_derived = False
    #             return _CertainValue(expr.string), is_derived
    #         if isinstance(expr, _clp.CompositeExpr):
    #             string = ""
    #             for part in expr.parts:
    #                 value, _ = evaluate(part)
    #                 if isinstance(value, _CertainValue):
    #                     if value.string is not None:
    #                         string += value.string
    #                     continue
    #                 if isinstance(value, _UncertainValue):
    #                     is_derived = False
    #                     return value, is_derived
    #                 assert_never(value)
    #             is_derived = True
    #             return _CertainValue(string), is_derived
    #         if isinstance(expr, _clp.ExpansionExpr):
    #             value, _ = evaluate(expr.name_expr)
    #             if isinstance(value, _CertainValue):
    #                 name = value.string or ""
    #                 value_2 = resolve_variable(expr.resolution_type, name, cmake_path, expr.pos, directory)
    #                 is_derived = True
    #                 return value_2, is_derived
    #             if isinstance(value, _UncertainValue):
    #                 is_derived = False
    #                 return value, is_derived
    #             assert_never(value)
    #         assert_never(expr)

    #     arguments: list[_Argument] = []
    #     for protoarg in invoc.arguments:
    #         value, is_derived = evaluate(protoarg.expr)
    #         arg: _Argument
    #         if isinstance(value, _CertainValue):
    #             string = value.string or ""
    #             substrings = []
    #             if protoarg.was_quoted_or_bracketed:
    #                 substrings = [string]
    #             else:
    #                 substrings = [s for s in _cu.list_split(string) if s]
    #                 is_derived = True
    #             for substring in substrings:
    #                 arg = _CertainArgument(protoarg.was_quoted_or_bracketed, protoarg.pos, substring, is_derived)
    #                 arguments.append(arg)
    #             continue
    #         if isinstance(value, _UncertainValue):
    #             arg = _UncertainArgument(protoarg.was_quoted_or_bracketed, protoarg.pos, value.reason)
    #             arguments.append(arg)
    #             continue
    #         assert_never(value)
    #     return arguments

    def resolve_variable(resolution_type: _csp.ResolutionType, variable_name: str, pos: int,
                         tracker: _tp.FilePosTracker, directory: _Directory) -> _Value:
        match resolution_type:
            case _csp.ResolutionType.GENERAL:
                value = directory.resolve_variable(variable_name)
                if value:
                    return value
                entry = cache.get(variable_name)
                if entry:
                    return entry.value
                assert False        
            case _csp.ResolutionType.CACHE:
                entry = cache.get(variable_name)
                if entry:
                    return entry.value
                assert False        
            case _csp.ResolutionType.ENV:
                value = environment.get(variable_name)
                if value:
                    return value
                assert False        
        assert_never(resolution_type)

    def warning(tracker: _tp.FilePosTracker, pos: int, message: str, *args: Any):
        context = tracker.get_file_context(pos)
        _l.FileContextLogger(logger, context).warn(message, *args)

    errors_seen = False
    def error(tracker: _tp.FilePosTracker, pos: int, message: str, *args: Any):
        nonlocal errors_seen
        errors_seen = True
        context = tracker.get_file_context(pos)
        _l.FileContextLogger(logger, context).error(message, *args)

    directory = _Directory()
    process_file(cmake_path, directory)
    return not errors_seen


type _Command = _BuiltInCommand

@dataclass(slots=True, frozen=True)
class _BuiltInCommand:
    class Which(enum.Enum):
        UNSUPPORTED      = 0
        SET              = 1
        UNSET            = 2
        MESSAGE          = 3
        INCLUDE          = 4
        ADD_SUBDIRECTORY = 5
    which: Which


def _define_built_in_commands(commands: dict[str, _Command]) -> None:
    def define(name_cf: str, which: _BuiltInCommand.Which):
        commands[name_cf] = _BuiltInCommand(which)
    define("set",                    _BuiltInCommand.Which.SET)
    define("unset",                  _BuiltInCommand.Which.UNSET)
    define("message",                _BuiltInCommand.Which.MESSAGE)
    define("include",                _BuiltInCommand.Which.INCLUDE)
    define("add_subdirectory",       _BuiltInCommand.Which.ADD_SUBDIRECTORY)
    define("cmake_minimum_required", _BuiltInCommand.Which.UNSUPPORTED)
    define("project",                _BuiltInCommand.Which.UNSUPPORTED)
    define("option",                 _BuiltInCommand.Which.UNSUPPORTED)


class _CacheEntry:
    value: _Value


class _Directory:
    def __init__(self, parent: _Directory | None = None) -> None:
        self._parent = parent
        self._variables:         dict[str, _CertainValue]           = {}
        self._tainted_variables: dict[str, _ValueUncertaintyReason] = {}

    def resolve_variable(self, name: str) -> _Value | None:
        reason = self._tainted_variables.get(name)
        if reason:
            return _UncertainValue(reason)
        value = self._variables.get(name)
        if value:
            return value
        if self._parent:
            return self._parent.resolve_variable(name)
        return None


type _Value = _CertainValue | _UncertainValue

@dataclass(slots=True, frozen=True)
class _CertainValue:
    string: str | None

@dataclass(slots=True, frozen=True)
class _UncertainValue:
    reason: _ValueUncertaintyReason


type _Argument = _CertainArgument | _UncertainArgument

@dataclass(slots=True, frozen=True)
class _ArgumentBase:
    was_quoted_or_bracketed: bool
    pos:                     int

@dataclass(slots=True, frozen=True)
class _CertainArgument(_ArgumentBase):
    string:     _tp.PosMappedString
    is_derived: bool

@dataclass(slots=True, frozen=True)
class _UncertainArgument(_ArgumentBase):
    reason: _ExpansionUncertaintyReason


type _ExpansionResult = _CertainExpansionResult | _UncertainExpansionResult

@dataclass(slots=True, frozen=True)
class _CertainExpansionResult:
    string:     _tp.PosMappedString
    is_derived: bool

@dataclass(slots=True, frozen=True)
class _UncertainExpansionResult:
    reason: _ExpansionUncertaintyReason


def _list_split(string: _tp.PosMappedString, is_derived: bool) -> tuple[list[_tp.PosMappedString], bool]:
    if not string.string:
        return [], True
    string_builder = _tp.PosMappedStringBuilder()
    parts = []
    def flush() -> None:
        string_2 = string_builder.get()
        pos_map = string.pos_map.compose_with(string_2.pos_map)
        parts.append(_tp.PosMappedString(string_2.string, pos_map))
    is_derived_2 = is_derived
    i = 0
    while True:
        j = string.string.find(";", i)
        if j == -1:
            string_builder.add_linear(string.string[i:], i)
            break
        is_derived_2 = True
        if j > i and string.string[j-1] == "\\":
            string_builder.add_linear(string.string[i:j-1], i)
            string_builder.add_nonlinear(";", j - 1)
            i = j + 1
            continue
        string_builder.add_linear(string.string[i:j], i)
        flush()
        i = j + 1
        string_builder = _tp.PosMappedStringBuilder(i)
    flush()
    return parts, is_derived_2


@dataclass(slots=True, frozen=True)
class _ValueUncertaintyReason:
    pass

@dataclass(slots=True, frozen=True)
class _ExpansionUncertaintyReason:
    pass
