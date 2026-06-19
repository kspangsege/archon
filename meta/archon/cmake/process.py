from __future__ import annotations
from typing import Protocol, Any, override, assert_never
from collections.abc import Callable, Container
from dataclasses import dataclass

import enum
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.cmake.util as _cu
import archon.cmake.lowlevel_parser as _clp
import archon.cmake.string_parser as _csp
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.variable as _cv
import archon.cmake.argument as _ca
import archon.cmake.condition as _cc


def process(cmake_path: pathlib.Path, application: Application, pos_resolver: PositionResolver,
            logger: _l.Logger) -> bool:
    return _process(cmake_path, application, pos_resolver, logger)


class Application:
    def message(self, pos: _cur.Position, uncertainty: ConditionalUncertainty, level: MessageLevel,
                message: str) -> None:
        ...


type ConditionalUncertainty = _cur.ExpansionUncertaintyReason | None


class PositionResolver:
    def __init__(self) -> None:
        self._files = list[_SourceFile]()

    def resolve(self, pos: _cur.Position) -> _tp.FileContext:
        return self._files[pos.file_index].pos_tracker.get_file_context(pos.pos)

    def _append_file(self, file_: _SourceFile) -> int:
        file_index = len(self._files)
        self._files.append(file_)
        return file_index


class MessageLevel(enum.Enum):
    FATAL_ERROR    = 0
    SEND_ERROR     = 1
    WARNING        = 2
    AUTHOR_WARNING = 3
    DEPRECATION    = 4
    NOTICE         = 5
    STATUS         = 6
    VERBOSE        = 7
    DEBUG          = 8
    TRACE          = 9








def _process(cmake_path: pathlib.Path, application, pos_resolver, logger: _l.Logger) -> bool:
    commands    = dict[str, _Command]()
    environment = _Environment()
    cache       = _Cache()
    _define_built_in_commands(commands)

    def process_file(cmake_path: pathlib.Path, directory: _Directory,
                     conditional_uncertainty: ConditionalUncertainty) -> None:
        tracker = _tp.FilePosTracker(cmake_path)
        file_index = pos_resolver._append_file(_SourceFile(tracker))
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
        except _ca.UncertainArgumentException as e:
            position = e.reason.expansion_position
            error(position.file_index, position.pos, "Failed to invoke %s() due to expansion of variable %s with "
                  "uncertain value", e.reason.command_name, _b.quote(e.reason.variable_name))
            reason = e.reason.value_uncertainty_reason
            while reason:
                if isinstance(reason, _cur.ExpansionUncertaintyReason):
                    reason_2 = reason
                elif isinstance(reason, _cur.AssignmentOccurrenceUncertaintyReason):
                    position = reason.assignment_position
                    error(position.file_index, position.pos, "Caused by execution of %s() with uncertain occurrence",
                          reason.command_name)
                    reason_2 = reason.conditional_uncertainty_reason
                position = reason_2.expansion_position
                error(position.file_index, position.pos, "Caused by expansion of variable %s with uncertain value in "
                      "invocation of %s()", _b.quote(reason_2.variable_name), reason_2.command_name)
                reason = reason_2.value_uncertainty_reason
            return

    def process_simple(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        command = commands.get(invoc.command_name_cf)
        if not command:
            error(context.file_index, invoc.pos, "Invocation of undefined command, %s()", invoc.command_name)
            return
        match command:
            case _BuiltInCommand(which):
                match which:
                    case _BuiltInCommand.Which.UNSUPPORTED:
                        error(context.file_index, invoc.pos, "Invocation of unsupported command, %s()",
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
        result = evaluate_condition(invoc, context)
        # If true, execute main branch        
        assert False        

    def process_foreach(invoc: _clp.ForeachInvoc, context: _InvocContext) -> None:
        assert False        

    def process_while(invoc: _clp.WhileInvoc, context: _InvocContext) -> None:
        assert False        

    def process_macro(invoc: _clp.MacroDefInvoc, context: _InvocContext) -> None:
        assert False        

    def process_function(invoc: _clp.FunctionDefInvoc, context: _InvocContext) -> None:
        assert False        

    def process_block(invoc: _clp.BlockInvoc, context: _InvocContext) -> None:
        assert False        

    def process_set(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        # FIXME: Consider picking up the part of the variable name that is specified, if
        # any, and use it as a tainting pattern
        variable = server.consume()
        if not variable:
            error(context.file_index, server.next_pos(), "Missing variable name in %s() invocation",
                  invoc.command_name)
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
        except _ca.UncertainArgumentException as e:
            context.directory.taint_variable(var_name, e.reason)
            if context.directory.parent:
                context.directory.parent.taint_variable(var_name, e.reason)
            return
        set_variable(var_name, values, parent_scope, invoc, context)

    def process_unset(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        variable = server.consume()
        if not variable:
            error(context.file_index, server.next_pos(), "Missing variable name in %s() invocation",
                  invoc.command_name)
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
        parent_scope = False
        try:
            arg = server.consume()
            if arg:
                if arg.string == "CACHE":
                    raise _UnsupportedCommandSyntaxException from None
                if server.at_end() and arg.string == "PARENT_SCOPE":
                    parent_scope = True
                else:
                    error(context.file_index, server.next_pos(), "Too many arguments in %s() invocation",
                          invoc.command_name)
        except _ca.UncertainArgumentException as e:
            context.directory.taint_variable(var_name, e.reason)
            if context.directory.parent:
                context.directory.parent.taint_variable(var_name, e.reason)
            return
        values = list[str]()
        set_variable(var_name, values, parent_scope, invoc, context)

    def process_message(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if server.consume_keyword({"CHECK_START", "CHECK_PASS", "CHECK_FAIL", "CONFIGURE_LOG"}):
            raise _UnsupportedCommandSyntaxException
        level = MessageLevel.NOTICE
        arg = server.consume_keyword(_MESSAGE_LEVEL_MAP.keys())
        if arg:
            level = _MESSAGE_LEVEL_MAP[arg.string]
        message = ""
        while True:
            arg = server.consume()
            if not arg:
                break
            message += arg.string
        position = _cur.Position(context.file_index, invoc.pos)
        application.message(position, context.conditional_uncertainty, level, message)

    def process_include(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        assert False        

    def process_add_subdirectory(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        assert False        

    def evaluate_condition(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> _cc.Result:
        arguments = expand_arguments(invoc, context)
        condition = _cc.parse(arguments, invoc.rparen_pos)
        class State(_cv.VariableState):
            @override
            def get(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> _cv.Value:
                return resolve_variable(resolution_type, variable_name, pos, context)
            @override
            def set_(self, variable_name: str, value: str | None) -> None:
                context.directory.set_variable(variable_name, value)
            @override
            def taint(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
                context.directory.taint_variable(variable_name, reason)
        state = State()
        return _cc.evaluate(condition, invoc.command_name, context.file_index, state)

    def create_argument_server(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> _ca.ArgumentServer:
        arguments = expand_arguments(invoc, context)
        return _ca.ArgumentServer(invoc, arguments, context.file_index)

    def expand_arguments(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> list[_ca.Argument]:
        arguments = list[_ca.Argument]()
        for protoarg in invoc.arguments:
            i = protoarg.prefix_size
            j = len(protoarg.text) - protoarg.suffix_size
            string = protoarg.text[i:j]
            pos = protoarg.pos + i
            arg: _ca.Argument
            match protoarg.type_:
                case _clp.Protoargument.Type.BARE:
                    was_quoted_or_bracketed = False
                    result = expand_string(string, pos, invoc, context)
                    match result:
                        case _CertainExpansionResult():
                            segments, is_derived = _list_split(result.string, result.is_derived)
                            for segment in segments:
                                arg = _ca.CertainArgument(was_quoted_or_bracketed, protoarg.pos, segment, is_derived)
                                arguments.append(arg)
                            continue
                        case _UncertainExpansionResult():
                            # Note that an uncertain unquoted proto-argument stands in for
                            # any number of actual arguments, including zero.
                            arg = _ca.UncertainArgument(was_quoted_or_bracketed, protoarg.pos, result.reason)
                            arguments.append(arg)
                            continue
                    assert_never(result)
                case _clp.Protoargument.Type.QUOTED:
                    was_quoted_or_bracketed = True
                    result = expand_string(string, pos, invoc, context)
                    match result:
                        case _CertainExpansionResult():
                            arg = _ca.CertainArgument(was_quoted_or_bracketed, protoarg.pos, result.string,
                                                   result.is_derived)
                            arguments.append(arg)
                            continue
                        case _UncertainExpansionResult():
                            arg = _ca.UncertainArgument(was_quoted_or_bracketed, protoarg.pos, result.reason)
                            arguments.append(arg)
                            continue
                    assert_never(result)
                case _clp.Protoargument.Type.BRACKETED:
                    was_quoted_or_bracketed = True
                    string_2 = _tp.PosMappedString.from_linear_string(string, pos)
                    is_derived = False
                    arg = _ca.CertainArgument(was_quoted_or_bracketed, protoarg.pos, string_2, is_derived)
                    arguments.append(arg)
                    continue
            assert_never(protoarg.type_)
        return arguments

    def expand_string(string: str, pos: int, invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> _ExpansionResult:
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
                    if isinstance(value, _cv.CertainValue):
                        string = _tp.PosMappedString.from_nonlinear_string(value.string or "", expr.pos)
                        is_derived = True
                        return _CertainExpansionResult(string, is_derived)
                    if isinstance(value, _cv.UncertainValue):
                        position = _cur.Position(context.file_index, expr.pos)
                        reason = _cur.ExpansionUncertaintyReason(invoc.command_name, name, position, value.reason)
                        return _UncertainExpansionResult(reason)
                    assert_never(value)
                if isinstance(result, _UncertainExpansionResult):
                    return result
                assert_never(result)
            assert_never(expr)
        def error_handler(pos: int, message: str, *args: Any) -> None:
            error(context.file_index, pos, message, *args)
        return expand(_csp.parse(string, pos, error_handler))

    def resolve_variable(resolution_type: _cu.ResolutionType, variable_name: str, pos: int,
                         context: _InvocContext) -> _cv.Value:
        match resolution_type:
            case _cu.ResolutionType.GENERAL:
                value = directory.resolve_variable(variable_name)
                match value:
                    case _cv.CertainValue(string):
                        if string is not None:
                            return value
                    case _cv.UncertainValue():
                        return value
                    case _:
                        assert_never(value)
                return cache.resolve(variable_name)
            case _cu.ResolutionType.CACHE:
                return cache.resolve(variable_name)
            case _cu.ResolutionType.ENV:
                return environment.resolve(variable_name)
        assert_never(resolution_type)

    def set_variable(variable_name: str, values: list[str], parent_scope: bool, invoc: _clp.GeneralizedInvoc,
                     context: _InvocContext) -> None:
        target = directory
        if parent_scope:
            if not directory.parent:
                warning(context.file_index, invoc.pos, "set() invocation skipped: No parent scope exists")
                return
            target = directory.parent
        if not context.conditional_uncertainty:
            value: str | None = None
            if values:
                value = ";".join(values)
            target.set_variable(variable_name, value)
            return
        # FIXME: Consider adding new value as alternative specific value
        position = _cur.Position(context.file_index, invoc.pos)
        reason = _cur.AssignmentOccurrenceUncertaintyReason(invoc.command_name, position,
                                                            context.conditional_uncertainty)
        target.taint_variable(variable_name, reason)

    def warning(file_index: int, pos: int, message: str, *args: Any):
        context = pos_resolver.resolve(_cur.Position(file_index, pos))
        _l.FileContextLogger(logger, context).warn(message, *args)

    errors_seen = False
    def error(file_index: int, pos: int, message: str, *args: Any):
        nonlocal errors_seen
        errors_seen = True
        context = pos_resolver.resolve(_cur.Position(file_index, pos))
        _l.FileContextLogger(logger, context).error(message, *args)

    directory = _Directory()
    conditional_uncertainty = None
    process_file(cmake_path, directory, conditional_uncertainty)
    return not errors_seen


@dataclass(slots=True, frozen=True)
class _SourceFile:
    pos_tracker: _tp.FilePosTracker


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
    def resolve(self, name: str) -> _cv.Value:
        assert False        


class _Cache:
    def resolve(self, name: str) -> _cv.Value:
        assert False        


class _Directory:
    def __init__(self, parent: _Directory | None = None) -> None:
        self._parent            = parent
        self._variables         = dict[str, _cv.CertainValue]()
        self._tainted_variables = dict[str, _cur.ValueUncertaintyReason]()

    @property
    def parent(self) -> _Directory | None:
        return self._parent

    def resolve_variable(self, name: str) -> _cv.Value:
        reason: _cur.ValueUncertaintyReason | None
        reason = self._tainted_variables.get(name)
        if reason:
            return _cv.UncertainValue(reason)
        value = self._variables.get(name)
        if value:
            if value.string is None and self._parent:
                return self._parent.resolve_variable(name)
            return value
        reason = None
        return _cv.UncertainValue(reason)

    def set_variable(self, variable_name: str, value: str | None) -> None:
        self._variables[variable_name] = _cv.CertainValue(value)
        self._tainted_variables.pop(variable_name, None)

    def taint_variable(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
        self._variables.pop(variable_name, None)
        self._tainted_variables[variable_name] = reason


@dataclass(slots=True, frozen=True)
class _InvocContext:
    file_index:              int
    directory:               _Directory
    conditional_uncertainty: ConditionalUncertainty


class _UnsupportedCommandSyntaxException(Exception):
    pass


type _ExpansionResult = _CertainExpansionResult | _UncertainExpansionResult

@dataclass(slots=True, frozen=True)
class _CertainExpansionResult:
    string:     _tp.PosMappedString
    is_derived: bool

@dataclass(slots=True, frozen=True)
class _UncertainExpansionResult:
    reason: _cur.ExpansionUncertaintyReason


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


_MESSAGE_LEVEL_MAP = {
    "FATAL_ERROR":    MessageLevel.FATAL_ERROR,
    "SEND_ERROR":     MessageLevel.SEND_ERROR,
    "WARNING":        MessageLevel.WARNING,
    "AUTHOR_WARNING": MessageLevel.AUTHOR_WARNING,
    "DEPRECATION":    MessageLevel.DEPRECATION,
    "NOTICE":         MessageLevel.NOTICE,
    "STATUS":         MessageLevel.STATUS,
    "VERBOSE":        MessageLevel.VERBOSE,
    "DEBUG":          MessageLevel.DEBUG,
    "TRACE":          MessageLevel.TRACE,
}
