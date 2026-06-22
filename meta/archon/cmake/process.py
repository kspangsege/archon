from __future__ import annotations
from typing import Protocol, Any, override, assert_never
from abc import ABC, abstractmethod
from collections.abc import Callable, Container, Iterable
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
    def message(self, pos: _cur.Position, occurrence_uncertainty: OccurrenceUncertainty, level: MessageLevel,
                message: str) -> None:
        ...


type OccurrenceUncertainty = _cur.ExpansionUncertaintyReason | None


class PositionResolver:
    def __init__(self) -> None:
        self._files = list[_SourceFile]()

    def resolve_text_pos(self, pos: _cur.Position) -> _tp.FullTextPos:
        return self._files[pos.file_index].pos_tracker.get_text_pos(pos.pos)

    def resolve_file_context(self, pos: _cur.Position) -> _tp.FileContext:
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
    def process_file(cmake_path: pathlib.Path, state: _State, occurrence_uncertainty: OccurrenceUncertainty) -> None:
        tracker = _tp.FilePosTracker(cmake_path)
        file_index = pos_resolver._append_file(_SourceFile(tracker))
        context = _InvocContext(file_index, state, occurrence_uncertainty)
        def warning_handler(pos: int, message: str, *args: Any) -> None:
            warning(file_index, pos, message, *args)
        def error_handler(pos: int, message: str, *args: Any) -> None:
            error(file_index, pos, message, *args)
        for invoc in _clp.parse(tracker, warning_handler, error_handler):
            exec_command(invoc, context)

    def exec_commands(invocations: Iterable[_clp.Invoc], context: _InvocContext) -> None:
        for invoc in invocations:
            exec_command(invoc, context)

    def exec_command(invoc: _clp.Invoc, context: _InvocContext) -> None:
        try:
            match invoc:
                case _clp.SimpleInvoc():
                    exec_simple(invoc, context)
                    return
                case _clp.IfInvoc():
                    exec_if(invoc, context)
                    return
                case _clp.ForeachInvoc():
                    exec_foreach(invoc, context)
                    return
                case _clp.WhileInvoc():
                    exec_while(invoc, context)
                    return
                case _clp.MacroDefInvoc():
                    exec_macro(invoc, context)
                    return
                case _clp.FunctionDefInvoc():
                    exec_function(invoc, context)
                    return
                case _clp.BlockInvoc():
                    exec_block(invoc, context)
                    return
            assert_never(invoc)
        except _UnsupportedInvocSyntaxException as e:
            error(context.file_index, e.invoc.pos, "Unsupported %s() syntax", e.invoc.command_name)
            return
        except _ConditionParseError as e:
            error(context.file_index, e.pos, "Failed to parse %s() condition: %s", e.command_name, e.message)
            return
        except _ConditionEvalError as e:
            error(context.file_index, e.pos, "Failed to evaluate %s() condition: %s", e.command_name, e.message)
            return
        except _ca.UncertainArgumentException as e:
            position = e.reason.expansion_position
            error(position.file_index, position.pos, "Failed to invoke %s() due to expansion of variable %s with "
                  "uncertain value", e.reason.command_name, _b.quote(e.reason.variable_name))
            if e.reason.value_uncertainty_reason:
                trace_value_uncertainty_causes(e.reason.value_uncertainty_reason)
            return

    def exec_simple(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        command = context.state.get_command(invoc.command_name_cf)
        match command:
            case _BuiltInCommand(which):
                match which:
                    case _BuiltInCommand.Which.UNSUPPORTED:
                        error(context.file_index, invoc.pos, "Invocation of unsupported command, %s()",
                              invoc.command_name)
                        return
                    case _BuiltInCommand.Which.SET:
                        exec_set(invoc, context)
                        return
                    case _BuiltInCommand.Which.UNSET:
                        exec_unset(invoc, context)
                        return
                    case _BuiltInCommand.Which.MESSAGE:
                        exec_message(invoc, context)
                        return
                    case _BuiltInCommand.Which.INCLUDE:
                        exec_include(invoc, context)
                        return
                    case _BuiltInCommand.Which.ADD_SUBDIRECTORY:
                        exec_add_subdirectory(invoc, context)
                        return
                assert_never(which)
            case _UncertainCommand(reason):
                error(context.file_index, invoc.pos, "Invocation failed due to uncertain definition of %s()",
                      invoc.command_name)
                if reason:
                    position = reason.definition_position
                    error(position.file_index, position.pos, "Caused by execution of %s() with uncertain occurrence",
                          reason.defining_command_name)
                    trace_expansion_uncertainty_causes(reason.occurrence_uncertainty_reason)
                return
        assert_never(command)

    # FIXME: What is the exact list of command names that cannot be overridden? return() appears to be among them    
    def exec_if(invoc: _clp.IfInvoc, context: _InvocContext) -> None:
        # CMake has short-circuiting evaluation behavior across if-branches, meaning that as
        # soon as a branch condition evaluates to true, the remaining branch conditions are
        # not evaluated.
        #
        # FIXME: Since the evaluation of branches can happen with uncertain occurrence, it
        # is necessary to taint touched variables for those branches (looks like an extra
        # state overlay needs to be injected just for the evaluation of such conditions, at
        # least if the condition can have side effects)                                               
        #
        accumulated = _cc.FalseResult()
        done = False
        def exec_branch(result: _cc.Result, children: Iterable[_clp.Invoc]) -> None:
            nonlocal accumulated, done
            effective = ~accumulated & result
            accumulated |= result
            occurrence_uncertainty: OccurrenceUncertainty = None
            match effective:
                case _cc.FalseResult():
                    return
                case _cc.TrueResult():
                    done = True
                case _cc.UncertainResult(reason):
                    occurrence_uncertainty = reason
            context_2 = context
            if occurrence_uncertainty:
                state = _OccurrenceUncertaintyOverlayState(context.state, occurrence_uncertainty)
                orig_occurrence_uncertainty = context.occurrence_uncertainty or occurrence_uncertainty
                context_2 = _InvocContext(context.file_index, state, orig_occurrence_uncertainty)
            exec_commands(children, context_2)
        exec_branch(evaluate_condition(invoc, context), invoc.children)
        for branch in invoc.elseif_branches:
            if done:
                break
            exec_branch(evaluate_condition(branch, context), branch.children)
        if not done and invoc.else_branch:
            # CMake completely ignores the arguments passed to `else()`
            exec_branch(_cc.TrueResult(), invoc.else_branch.children)
        exec_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_foreach(invoc: _clp.ForeachInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_while(invoc: _clp.WhileInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_macro(invoc: _clp.MacroDefInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_function(invoc: _clp.FunctionDefInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_block(invoc: _clp.BlockInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_closing_invoc(invoc: _clp.ClosingInvoc, opening_invoc: _clp.GeneralizedInvoc,
                           context: _InvocContext) -> None:
        # CMake ignores arguments in a closing invocation but generates a warning unless the
        # closing invocation is either empty or matches the corresponding opening invocation
        # proto-argument for proto-argument. if() is the corresponding opening invocation
        # for endif() even if there are elseif() and/or else() invocations in between.
        if not invoc.arguments:
            return
        opening_args = [a.text for a in opening_invoc.arguments]
        closing_args = [a.text for a in invoc.arguments]
        if opening_args != closing_args:
            opening_position = _cur.Position(context.file_index, opening_invoc.pos)
            opening_line_no = pos_resolver.resolve_text_pos(opening_position).line_no
            warning(context.file_index, invoc.pos, "Closing invocation, %s(), has mismatching arguments (opening "
                    "invocation is on line %s)", invoc.command_name, opening_line_no)

    def exec_set(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
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
                raise _UnsupportedInvocSyntaxException(invoc) from None
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
                    raise _UnsupportedInvocSyntaxException(invoc) from None
                if server.at_end() and arg.string == "PARENT_SCOPE":
                    parent_scope = True
                    break
                values.append(arg.string)
        except _ca.UncertainArgumentException as e:
            context.state.taint_regular_variable(var_name, e.reason, parent_scope=False)
            if not context.state.is_root_scope():
                context.state.taint_regular_variable(var_name, e.reason, parent_scope=True)
            return
        value = ";".join(values) if values else None
        set_regular_variable(var_name, value, parent_scope, invoc, context)

    def exec_unset(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
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
                raise _UnsupportedInvocSyntaxException(invoc) from None
            case _:
                assert_never(var_ref.resolution_type)
        var_name = var_ref.variable_name
        parent_scope = False
        try:
            arg = server.consume()
            if arg:
                if arg.string == "CACHE":
                    raise _UnsupportedInvocSyntaxException(invoc) from None
                if server.at_end() and arg.string == "PARENT_SCOPE":
                    parent_scope = True
                else:
                    error(context.file_index, server.next_pos(), "Too many arguments in %s() invocation",
                          invoc.command_name)
        except _ca.UncertainArgumentException as e:
            context.state.taint_regular_variable(var_name, e.reason, parent_scope=False)
            if not context.state.is_root_scope():
                context.state.taint_regular_variable(var_name, e.reason, parent_scope=True)
            return
        value = None
        set_regular_variable(var_name, value, parent_scope, invoc, context)

    def exec_message(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if server.consume_keyword({"CHECK_START", "CHECK_PASS", "CHECK_FAIL", "CONFIGURE_LOG"}):
            raise _UnsupportedInvocSyntaxException(invoc) from None
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
        application.message(position, context.occurrence_uncertainty, level, message)

    def exec_include(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_add_subdirectory(invoc: _clp.SimpleInvoc, context: _InvocContext) -> None:
        assert False        

    def evaluate_condition(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> _cc.Result:
        try:
            arguments = expand_arguments(invoc, context)
            condition = _cc.parse(arguments, invoc.rparen_pos)
            class State(_cv.VariableState):
                @override
                def get(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> _cv.Value:
                    return resolve_variable(resolution_type, variable_name, pos, context)
                @override
                def set_(self, variable_name: str, value: str | None) -> None:
                    parent_scope = False
                    assigning_command_name = invoc.command_name
                    assignment_position = _cur.Position(context.file_index, invoc.pos)
                    context.state.set_regular_variable(variable_name, value, parent_scope, assigning_command_name,
                                                       assignment_position)
                @override
                def taint(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
                    context.state.taint_regular_variable(variable_name, reason, parent_scope=False)
            state = State()
            return _cc.evaluate(condition, invoc.command_name, context.file_index, state)
        except _cc.FatalParseError as e:
            raise _ConditionParseError(e.pos, invoc.command_name, e.message % e.args) from None
        except _cc.FatalEvalError as e:
            raise _ConditionEvalError(e.pos, invoc.command_name, e.message % e.args) from None

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
                    was_bare = True
                    result = expand_string(string, pos, invoc, context)
                    match result:
                        case _CertainExpansionResult():
                            segments, is_derived = _list_split(result.string, result.is_derived)
                            for segment in segments:
                                arg = _ca.CertainArgument(protoarg.pos, was_bare, segment, is_derived)
                                arguments.append(arg)
                            continue
                        case _UncertainExpansionResult():
                            # Note that an uncertain unquoted proto-argument stands in for
                            # any number of actual arguments, including zero.
                            arg = _ca.UncertainArgument(protoarg.pos, was_bare, result.reason)
                            arguments.append(arg)
                            continue
                    assert_never(result)
                case _clp.Protoargument.Type.QUOTED:
                    was_bare = False
                    result = expand_string(string, pos, invoc, context)
                    match result:
                        case _CertainExpansionResult():
                            arg = _ca.CertainArgument(protoarg.pos, was_bare, result.string, result.is_derived)
                            arguments.append(arg)
                            continue
                        case _UncertainExpansionResult():
                            arg = _ca.UncertainArgument(protoarg.pos, was_bare, result.reason)
                            arguments.append(arg)
                            continue
                    assert_never(result)
                case _clp.Protoargument.Type.BRACKETED:
                    was_bare = False
                    string_2 = _tp.PosMappedString.from_linear_string(string, pos)
                    is_derived = False
                    arg = _ca.CertainArgument(protoarg.pos, was_bare, string_2, is_derived)
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
                value = context.state.get_regular_variable(variable_name, parent_scope_override=None)
                match value:
                    case _cv.CertainValue(string):
                        if string is not None:
                            return value
                    case _cv.UncertainValue():
                        return value
                    case _:
                        assert_never(value)
                return context.state.get_cache_variable(variable_name)
            case _cu.ResolutionType.CACHE:
                return context.state.get_cache_variable(variable_name)
            case _cu.ResolutionType.ENV:
                return context.state.get_environment_variable(variable_name)
        assert_never(resolution_type)

    def set_regular_variable(variable_name: str, value: str | None, parent_scope: bool, invoc: _clp.GeneralizedInvoc,
                             context: _InvocContext) -> None:
        if parent_scope and context.state.is_root_scope():
            warning(context.file_index, invoc.pos, "%s() invocation skipped: No parent scope exists",
                    invoc.command_name)
            return
        assigning_command_name = invoc.command_name
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        context.state.set_regular_variable(variable_name, value, parent_scope, assigning_command_name,
                                           assignment_position)

    def trace_value_uncertainty_causes(cause: _cur.ValueUncertaintyReason) -> None:
        match cause:
            case _cur.ExpansionUncertaintyReason():
                trace_expansion_uncertainty_causes(cause)
                return
            case _cur.AssignmentOccurrenceUncertaintyReason():
                position = cause.assignment_position
                error(position.file_index, position.pos, "Caused by execution of %s() with uncertain occurrence",
                      cause.command_name)
                trace_expansion_uncertainty_causes(cause.occurrence_uncertainty_reason)
                return
        assert_never(cause)

    def trace_expansion_uncertainty_causes(cause: _cur.ExpansionUncertaintyReason) -> None:
        position = cause.expansion_position
        error(position.file_index, position.pos, "Caused by expansion of variable %s with uncertain value in "
              "invocation of %s()", _b.quote(cause.variable_name), cause.command_name)
        if cause.value_uncertainty_reason:
            trace_value_uncertainty_causes(cause.value_uncertainty_reason)

    def warning(file_index: int, pos: int, message: str, *args: Any) -> None:
        context = pos_resolver.resolve_file_context(_cur.Position(file_index, pos))
        _l.FileContextLogger(logger, context).warn(message, *args)

    errors_seen = False
    def error(file_index: int, pos: int, message: str, *args: Any) -> None:
        nonlocal errors_seen
        errors_seen = True
        context = pos_resolver.resolve_file_context(_cur.Position(file_index, pos))
        _l.FileContextLogger(logger, context).error(message, *args)

    state = _RootState()
    occurrence_uncertainty = None
    process_file(cmake_path, state, occurrence_uncertainty)
    return not errors_seen


@dataclass(slots=True, frozen=True)
class _SourceFile:
    pos_tracker: _tp.FilePosTracker


@dataclass(slots=True, frozen=True)
class _InvocContext:
    file_index:             int
    state:                  _State
    occurrence_uncertainty: OccurrenceUncertainty


class _State(ABC):
    @abstractmethod
    def is_root_scope(self) -> bool:
        ...

    @abstractmethod
    def get_regular_variable(self, name: str, parent_scope_override: _cv.CertainValue | None) -> _cv.Value:
        ...

    @abstractmethod
    def set_regular_variable(self, name: str, value: str | None, parent_scope: bool, assigning_command_name: str,
                             assignment_position: _cur.Position) -> None:
        ...

    @abstractmethod
    def taint_regular_variable(self, name: str, reason: _cur.ValueUncertaintyReason, parent_scope: bool) -> None:
        ...

    @abstractmethod
    def get_cache_variable(self, name: str) -> _cv.Value:
        ...

    @abstractmethod
    def get_environment_variable(self, name: str) -> _cv.Value:
        ...

    @abstractmethod
    def get_command(self, name_cf: str) -> _Command:
        ...

    @abstractmethod
    def set_command(self, name_cf: str, command: _CertainCommand, defining_command_name: str,
                    definition_position: _cur.Position) -> None:
        ...

    @abstractmethod
    def taint_command(self, name_cf: str, reason: _CommandDefinitionUncertaintyReason) -> None:
        ...


class _RootState(_State):
    def __init__(self):
        self._commands         = dict[str, _CertainCommand]()
        self._tainted_commands = dict[str, _CommandDefinitionUncertaintyReason]()
        self._environment      = _Environment()
        self._cache            = _Cache()
        self._directory        = _Directory()
        _define_built_in_commands(self._commands)

    @override
    def is_root_scope(self) -> bool:
        return True

    @override
    def get_regular_variable(self, name: str, parent_scope_override: _cv.CertainValue | None) -> _cv.Value:
        assert not parent_scope_override
        return self._directory.get_variable(name)

    @override
    def set_regular_variable(self, name: str, value: str | None, parent_scope: bool, assigning_command_name: str,
                             assignment_position: _cur.Position) -> None:
        assert not parent_scope
        self._directory.set_variable(name, value)

    @override
    def taint_regular_variable(self, name: str, reason: _cur.ValueUncertaintyReason, parent_scope: bool) -> None:
        assert not parent_scope
        self._directory.taint_variable(name, reason)

    @override
    def get_cache_variable(self, name: str) -> _cv.Value:
        assert False    

    @override
    def get_environment_variable(self, name: str) -> _cv.Value:
        assert False    

    @override
    def get_command(self, name_cf: str) -> _Command:
        reason = self._tainted_commands.get(name_cf)
        if reason:
            return _UncertainCommand(reason)
        command = self._commands.get(name_cf)
        if command:
            return command
        reason = None
        return _UncertainCommand(reason)

    @override
    def set_command(self, name_cf: str, command: _CertainCommand, defining_command_name: str,
                    definition_position: _cur.Position) -> None:
        self._commands[name_cf] = command
        self._tainted_commands.pop(name_cf, None)

    @override
    def taint_command(self, name_cf: str, reason: _CommandDefinitionUncertaintyReason) -> None:
        self._tainted_commands[name_cf] = reason
        self._commands.pop(name_cf, None)


class _OccurrenceUncertaintyOverlayState(_State):
    def __init__(self, parent_state: _State, occurrence_uncertainty_reason: _cur.ExpansionUncertaintyReason):
        self._parent_state                  = parent_state
        self._occurrence_uncertainty_reason = occurrence_uncertainty_reason
        self._commands                      = dict[str, _CertainCommand]()
        self._regular_variables             = dict[str, _cv.CertainValue]()
        self._parent_scope_variables        = dict[str, _cv.CertainValue]()

    @override
    def is_root_scope(self) -> bool:
        return self._parent_state.is_root_scope()

    @override
    def get_regular_variable(self, name: str, parent_scope_override: _cv.CertainValue | None) -> _cv.Value:
        value = self._regular_variables.get(name)
        # FIXME: Oooops, if value.value is None and not is_root_scope(), then the parent scope must be consulted                 
        if value:
            return value
        parent_scope_override_2 = parent_scope_override or self._parent_scope_variables.get(name)
        return self._parent_state.get_regular_variable(name, parent_scope_override_2)

    @override
    def set_regular_variable(self, name: str, value: str | None, parent_scope: bool, assigning_command_name: str,
                             assignment_position: _cur.Position) -> None:
        value_2 = _cv.CertainValue(value)
        if parent_scope:
            self._parent_scope_variables[name] = value_2
        else:
            self._regular_variables[name] = value_2
        reason = _cur.AssignmentOccurrenceUncertaintyReason(assigning_command_name, assignment_position,
                                                            self._occurrence_uncertainty_reason)
        self._parent_state.taint_regular_variable(name, reason, parent_scope)

    @override
    def taint_regular_variable(self, name: str, reason: _cur.ValueUncertaintyReason, parent_scope: bool) -> None:
        if parent_scope:
            self._parent_scope_variables.pop(name, None)
        else:
            self._regular_variables.pop(name, None)
        self._parent_state.taint_regular_variable(name, reason, parent_scope)

    @override
    def get_cache_variable(self, name: str) -> _cv.Value:
        assert False    

    @override
    def get_environment_variable(self, name: str) -> _cv.Value:
        assert False    

    @override
    def get_command(self, name_cf: str) -> _Command:
        command = self._commands.get(name_cf)
        if command:
            return command
        return self._parent_state.get_command(name_cf)

    @override
    def set_command(self, name_cf: str, command: _CertainCommand, defining_command_name: str,
                    definition_position: _cur.Position) -> None:
        self._commands[name_cf] = command
        reason = _CommandDefinitionUncertaintyReason(defining_command_name, definition_position,
                                                     self._occurrence_uncertainty_reason)
        self._parent_state.taint_command(name_cf, reason)

    @override
    def taint_command(self, name_cf: str, reason: _CommandDefinitionUncertaintyReason) -> None:
        self._commands.pop(name_cf, None)
        self._parent_state.taint_command(name_cf, reason)


type _Command = _CertainCommand | _UncertainCommand

type _CertainCommand = _BuiltInCommand

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

@dataclass(slots=True, frozen=True)
class _UncertainCommand:
    reason: _CommandDefinitionUncertaintyReason | None


@dataclass(slots=True, frozen=True)
class _CommandDefinitionUncertaintyReason:
    defining_command_name:         str
    definition_position:           _cur.Position
    occurrence_uncertainty_reason: _cur.ExpansionUncertaintyReason


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

    def get_variable(self, name: str) -> _cv.Value:
        reason: _cur.ValueUncertaintyReason | None
        reason = self._tainted_variables.get(name)
        if reason:
            return _cv.UncertainValue(reason)
        value = self._variables.get(name)
        if value:
            if value.string is None and self._parent:
                return self._parent.get_variable(name)
            return value
        reason = None
        return _cv.UncertainValue(reason)

    def set_variable(self, variable_name: str, value: str | None) -> None:
        self._variables[variable_name] = _cv.CertainValue(value)
        self._tainted_variables.pop(variable_name, None)

    def taint_variable(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
        self._variables.pop(variable_name, None)
        self._tainted_variables[variable_name] = reason


class _UnsupportedInvocSyntaxException(Exception):
    def __init__(self, invoc: _clp.GeneralizedInvoc) -> None:
        self.invoc = invoc


class _ConditionParseError(Exception):
    def __init__(self, pos: int, command_name: str, message: str) -> None:
        self.pos          = pos
        self.command_name = command_name
        self.message      = message


class _ConditionEvalError(Exception):
    def __init__(self, pos: int, command_name: str, message: str) -> None:
        self.pos          = pos
        self.command_name = command_name
        self.message      = message


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
