from __future__ import annotations
from typing import Protocol, Any, assert_never
from collections.abc import Callable, Container
from dataclasses import dataclass

import enum
import pathlib

import archon.text_pos as _tp
import archon.log as _l
import archon.cmake.util as _cu
import archon.cmake.lowlevel_parser as _clp
import archon.cmake.string_parser as _csp


def process(cmake_path: pathlib.Path, logger: _l.Logger) -> bool:
    return _process(cmake_path, logger)


def _process(cmake_path: pathlib.Path, logger: _l.Logger) -> bool:
    files       = list[_SourceFile]()
    commands    = dict[str, _Command]()
    environment = _Environment()
    cache       = _Cache()
    _define_built_in_commands(commands)

    def process_file(cmake_path: pathlib.Path, directory: _Directory,
                     conditional_uncertainty: _ConditionalUncertainty) -> None:
        tracker = _tp.FilePosTracker(cmake_path)
        file_index = len(files)
        files.append(_SourceFile(tracker))
        def warning_handler(pos: int, message: str, *args: Any) -> None:
            warning(file_index, pos, message, *args)
        def error_handler(pos: int, message: str, *args: Any) -> None:
            error(file_index, pos, message, *args)
        for invoc in _clp.parse(tracker, warning_handler, error_handler):
            context = _InvocContext(file_index, directory, conditional_uncertainty)
            process_invoc(invoc, context)

    def process_invoc(invoc: _clp.Invoc, context: _InvocContext) -> None:
        try:
            match invoc:
                case _clp.SimpleInvoc():
                    process_simple(invoc, context)
                    return
                case _clp.IfInvoc():
                    process_if(invoc, context)
                    return
                case _clp.ForeachInvoc():
                    process_foreach(invoc, context)
                    return
                case _clp.WhileInvoc():
                    process_while(invoc, context)
                    return
                case _clp.MacroDefInvoc():
                    process_macro(invoc, context)
                    return
                case _clp.FunctionDefInvoc():
                    process_function(invoc, context)
                    return
                case _clp.BlockInvoc():
                    process_block(invoc, context)
                    return
            assert_never(invoc)
        except _UnsupportedCommandSyntaxException:
            error(context.file_index, invoc.pos, "Unsupported %s() syntax", invoc.command_name)
            return
        except _UncertainArgumentException as e:
            assert False                                    

    def process_simple(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        command = commands.get(invoc.command_name_cf)
        if not command:
            error(context.file_index, invoc.pos, "Invocation of undefined command %s()", invoc.command_name)
            return
        match command:
            case _BuiltInCommand(which):
                match which:
                    case _BuiltInCommand.Which.UNSUPPORTED:
                        error(context.file_index, invoc.pos, "Invocation of unsupported command %s()",
                              invoc.command_name)
                        return
                    case _BuiltInCommand.Which.SET:
                        process_set(invoc, context)
                        return
                    case _BuiltInCommand.Which.UNSET:
                        process_unset(invoc, context)
                        return
                    case _BuiltInCommand.Which.MESSAGE:
                        process_message(invoc, context)
                        return
                    case _BuiltInCommand.Which.INCLUDE:
                        process_include(invoc, context)
                        return
                    case _BuiltInCommand.Which.ADD_SUBDIRECTORY:
                        process_add_subdirectory(invoc, context)
                        return
                assert_never(which)
        assert_never(command)

    def process_if(invoc: _clp.IfInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_foreach(invoc: _clp.ForeachInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_while(invoc: _clp.WhileInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_macro(invoc: _clp.MacroDefInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_function(invoc: _clp.FunctionDefInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_block(invoc: _clp.BlockInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_set(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        server = create_argument_server(invoc, context)
        # FIXME: Consider picking up the part of the variable name that is specified, if
        # any, and use it as a tainting pattern
        variable = server.consume()
        if not variable:
            error(context.file_index, server.next_pos(), "Missing variable name in set() invocation")
            return
        var_ref = _cu.parse_variable_reference(variable.string)
        match var_ref.resolution_type:
            case _cu.ResolutionType.GENERAL:
                pass
            case _cu.ResolutionType.CACHE | _cu.ResolutionType.ENV:
                raise _UnsupportedCommandSyntaxException from None
            case _:
                assert_never(var_ref.resolution_type)
        var_name = var_ref.variable_name
        values = []
        parent_scope = False
        try:
            while True:
                arg = server.consume()
                if not arg:
                    break
                if arg.string == "CACHE":
                    raise _UnsupportedCommandSyntaxException from None
                if server.at_end() and arg.string == "PARENT_SCOPE":
                    parent_scope = True
                    break
                values.append(arg.string)
        except _UncertainArgumentException as e:
            context.directory.taint_variable(var_name, e.reason)
            if context.directory.parent:
                context.directory.parent.taint_variable(var_name, e.reason)
            return
        set_variable(var_name, values, parent_scope, invoc.pos, context)

    def process_unset(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_message(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_include(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def process_add_subdirectory(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        logger.info("-----> INVOC: %s", invoc.command_name)
        assert False        

    def create_argument_server(invoc: _clp.InvocBase, context: _InvocContext) -> _ArgumentServer:
        arguments = expand_arguments(invoc, context)
        return _ArgumentServer(invoc, arguments, context.file_index)

    def expand_arguments(invoc: _clp.InvocBase, context: _InvocContext) -> list[_Argument]:
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
                    result = expand_string(string, pos, context)
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
                    result = expand_string(string, pos, context)
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

    def expand_string(string: str, pos: int, context: _InvocContext) -> _ExpansionResult:
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
                    value = resolve_variable(expr.resolution_type, name, expr.name_expr.pos, context)
                    if isinstance(value, _CertainValue):
                        string = _tp.PosMappedString.from_nonlinear_string(value.string or "", expr.pos)
                        is_derived = True
                        return _CertainExpansionResult(string, is_derived)
                    if isinstance(value, _UncertainValue):
                        source_pos = _SourcePosition(context.file_index, expr.pos)
                        reason = _ExpansionUncertaintyReason(name, source_pos, value.reason)
                        return _UncertainExpansionResult(reason)
                    assert_never(value)
                if isinstance(result, _UncertainExpansionResult):
                    return result
                assert_never(result)
            assert_never(expr)
        def error_handler(pos: int, message: str, *args: Any) -> None:
            error(context.file_index, pos, message, *args)
        return expand(_csp.parse(string, pos, error_handler))

    #    

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

    def resolve_variable(resolution_type: _cu.ResolutionType, variable_name: str, pos: int,
                         context: _InvocContext) -> _Value:
        match resolution_type:
            case _cu.ResolutionType.GENERAL:
                value = directory.resolve_variable(variable_name)
                match value:
                    case _CertainValue(string):
                        if string is not None:
                            return value
                    case _UncertainValue():
                        return value
                    case _:
                        assert_never(value)
                return cache.resolve(variable_name)
            case _cu.ResolutionType.CACHE:
                return cache.resolve(variable_name)
            case _cu.ResolutionType.ENV:
                return environment.resolve(variable_name)
        assert_never(resolution_type)

    def set_variable(variable_name: str, values: list[str], parent_scope: bool, invoc_pos: int,
                     context: _InvocContext) -> None:
        target = directory
        if parent_scope:
            if not directory.parent:
                warning(context.file_index, invoc_pos, "set() invocation skipped: No parent scope exists")
                return
            target = directory.parent
        if not context.conditional_uncertainty:
            value: str | None = None
            if values:
                value = ";".join(values)
            target.set_variable(variable_name, value)
            return
        # FIXME: Consider adding new value as alternative specific value
        assignment_pos = _SourcePosition(context.file_index, invoc_pos)
        reason = _AssignmentOccurrenceUncertaintyReason(assignment_pos, context.conditional_uncertainty)
        target.taint_variable(variable_name, reason)

    def warning(file_index: int, pos: int, message: str, *args: Any):
        context = files[file_index].pos_tracker.get_file_context(pos)
        _l.FileContextLogger(logger, context).warn(message, *args)

    errors_seen = False
    def error(file_index: int, pos: int, message: str, *args: Any):
        nonlocal errors_seen
        errors_seen = True
        context = files[file_index].pos_tracker.get_file_context(pos)
        _l.FileContextLogger(logger, context).error(message, *args)

    directory = _Directory()
    conditional_uncertainty = None
    process_file(cmake_path, directory, conditional_uncertainty)
    return not errors_seen


@dataclass(slots=True, frozen=True)
class _SourceFile:
    pos_tracker: _tp.FilePosTracker


@dataclass(slots=True, frozen=True)
class _SourcePosition:
    file_index: int
    pos:        int


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


class _Environment:
    def resolve(self, name: str) -> _Value:
        assert False        


class _Cache:
    def resolve(self, name: str) -> _Value:
        assert False        


class _Directory:
    def __init__(self, parent: _Directory | None = None) -> None:
        self._parent            = parent
        self._variables         = dict[str, _CertainValue]()
        self._tainted_variables = dict[str, _ValueUncertaintyReason]()

    @property
    def parent(self) -> _Directory | None:
        return self._parent

    def resolve_variable(self, name: str) -> _Value:
        reason: _ValueUncertaintyReason | None
        reason = self._tainted_variables.get(name)
        if reason:
            return _UncertainValue(reason)
        value = self._variables.get(name)
        if value:
            if value.string is None and self._parent:
                return self._parent.resolve_variable(name)
            return value
        reason = None
        return _UncertainValue(reason)

    def set_variable(self, variable_name: str, value: str | None) -> None:
        self._variables[variable_name] = _CertainValue(value)
        self._tainted_variables.pop(variable_name, None)

    def taint_variable(self, variable_name: str, reason: _ValueUncertaintyReason) -> None:
        self._variables.pop(variable_name, None)
        self._tainted_variables[variable_name] = reason


@dataclass(slots=True, frozen=True)
class _InvocContext:
    file_index:              int
    directory:               _Directory
    conditional_uncertainty: _ConditionalUncertainty


type _ConditionalUncertainty = _ExpansionUncertaintyReason | None


# FIXME: Consider expanding protoarguments just in time using a yielding scheme    
class _ArgumentServer:
    def __init__(self, invoc: _clp.InvocBase, arguments: list[_Argument], file_index: int):
        self._invoc      = invoc
        self._arguments  = arguments
        self._file_index = file_index
        self._begin         = 0
        self._end           = len(self._arguments)

    def consume(self) -> _tp.PosMappedString | None:
        return self.consume_if(lambda _: True)

    def consume_keyword(self, keywords: Container[str]) -> _tp.PosMappedString | None:
        return self.consume_if(lambda s: s in keywords)

    def consume_not_keyword(self, keywords: Container[str]) -> _tp.PosMappedString | None:
        return self.consume_if(lambda s: s not in keywords)

    def consume_if(self, pred: Callable[[str], bool]) -> _tp.PosMappedString | None:
        if self._begin < self._end:
            arg = self._arguments[self._begin]
            if isinstance(arg, _CertainArgument):
                if pred(arg.string.string):
                    self._begin += 1
                    return arg.string
                return None
            if isinstance(arg, _UncertainArgument):
                raise _UncertainArgumentException(arg.reason) from None
            assert_never(arg)
        return None

    def at_end(self) -> bool:
        assert self._begin <= self._end
        return self._begin == self._end

    def next_pos(self) -> int:
        if self._begin < self._end:
            arg = self._arguments[self._begin]
            return arg.pos
        return self._invoc.rparen_pos


class _UnsupportedCommandSyntaxException(Exception):
    pass


class _UncertainArgumentException(Exception):
    def __init__(self, reason: _ExpansionUncertaintyReason) -> None:
        self.reason = reason


type _Value = _CertainValue | _UncertainValue

@dataclass(slots=True, frozen=True)
class _CertainValue:
    string: str | None

@dataclass(slots=True, frozen=True)
class _UncertainValue:
    reason: _ValueUncertaintyReason | None


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


type _ValueUncertaintyReason = _ExpansionUncertaintyReason | _AssignmentOccurrenceUncertaintyReason

@dataclass(slots=True, frozen=True)
class _ExpansionUncertaintyReason:
    variable_name:            str
    expansion_position:       _SourcePosition
    value_uncertainty_reason: _ValueUncertaintyReason | None

@dataclass(slots=True, frozen=True)
class _AssignmentOccurrenceUncertaintyReason:
    assignment_position:            _SourcePosition
    conditional_uncertainty_reason: _ExpansionUncertaintyReason
