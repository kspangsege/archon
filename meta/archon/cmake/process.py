from __future__ import annotations

import typing
import abc
import dataclasses
import collections
import enum
import copy
import string as _string
import re
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.cmake.util as _cu
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.lowlevel_parser as _clp
import archon.cmake.string_parser as _csp
import archon.cmake.variable as _cv
import archon.cmake.argument as _ca
import archon.cmake.condition as _cc


@dataclasses.dataclass(kw_only=True)
class Config:
    define_breakpoint_command: bool = False


def process(cmake_source: Source, application: Application, pos_resolver: PositionResolver,
            config: Config = Config()) -> bool:
    return _process(cmake_source, application, pos_resolver, config)


@dataclasses.dataclass(slots=True, frozen=True)
class Source:
    input_: typing.TextIO
    path:   pathlib.Path


class Application(abc.ABC):
    @abc.abstractmethod
    def message(self, pos: _cur.Position, occurrence_uncertainty: OccurrenceUncertainty, level: MessageLevel,
                message: str) -> None:
        ...

    @abc.abstractmethod
    def warn(self, pos: _cur.Position, message: str, *args: typing.Any) -> None:
        ...

    @abc.abstractmethod
    def error(self, pos: _cur.Position, message: str, *args: typing.Any) -> None:
        ...


type OccurrenceUncertainty = _cur.ExpansionUncertaintyReason | None


class PositionResolver:
    def __init__(self) -> None:
        self._files = list[_SourceFile]()

    def resolve_file_context(self, pos: _cur.Position) -> _l.FileContext:
        file_pos = self.resolve_file_pos(pos)
        return _l.FileContext(file_pos.path, _l.FullTextPos(file_pos.line_no, file_pos.pos_on_line))

    def resolve_file_pos(self, pos: _cur.Position) -> _tp.FilePos:
        return self._files[pos.file_index].pos_tracker.get_file_pos(pos.pos)

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








def _process(cmake_source: Source, application: Application, pos_resolver: PositionResolver, config: Config) -> bool:
    errors_seen = False

    # A custom command (macro or function) that is in the current invocation path must be in
    # this map as (I, N), where I is the identifier of the custom command object
    # (`_CustomCommand`) and N is the number of times it is in the path. A custom command
    # that is not in the current invocation path should not be in this map.
    commands_in_invoc_path = dict[int, int]()

    def process_file(cmake_source: Source, state: _State, occurrence_uncertainty: OccurrenceUncertainty,
                     base_path: pathlib.Path) -> None:
        tracker = _tp.FilePosTracker(cmake_source.path)
        file_index = pos_resolver._append_file(_SourceFile(tracker))
        context = _InvocContext(file_index, state, occurrence_uncertainty, base_path)
        def warning_handler(pos: int, message: str, *args: typing.Any) -> None:
            warn(file_index, pos, message, *args)
        def error_handler(pos: int, message: str, *args: typing.Any) -> None:
            error(file_index, pos, message, *args)
        for invoc in _clp.parse(cmake_source.input_, tracker, warning_handler, error_handler):
            exec_command(invoc, context)

    def exec_commands(invocations: collections.abc.Iterable[_clp.Invoc], context: _InvocContext) -> None:
        for invoc in invocations:
            exec_command(invoc, context)

    def exec_command(invoc: _clp.Invoc, context: _InvocContext) -> None:
        try:
            match invoc:
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
                    exec_macro_def(invoc, context)
                    return
                case _clp.FunctionDefInvoc():
                    exec_function_def(invoc, context)
                    return
                case _clp.BlockInvoc():
                    exec_block(invoc, context)
                    return
                case _clp.ReturnInvoc():
                    exec_return(invoc, context)
                    return
                case _clp.BreakInvoc():
                    exec_break(invoc, context)
                    return
                case _clp.ContinueInvoc():
                    exec_continue(invoc, context)
                    return
                case _clp.GenericInvoc():
                    exec_generic(invoc, context)
                    return
            typing.assert_never(invoc)
        except _UnsupportedInvocSyntaxException as e:
            if e.args:
                error(context.file_index, e.invoc.pos, "Unsupported %s() syntax: %s", e.invoc.command_name, e)
            else:
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
            error(position.file_index, position.pos, "Failed to invoke %s() due to expansion of %s with uncertain "
                  "value", e.reason.command_name, e.reason.get_qual_param_ref())
            if e.reason.value_uncertainty_reason:
                trace_value_uncertainty_causes(e.reason.value_uncertainty_reason)
            return

    def exec_if(invoc: _clp.IfInvoc, context: _InvocContext) -> None:
        def exec_level(subinvoc: _clp.IfInvoc | _clp.IfBranch, context: _InvocContext, next_elseif: int) -> None:
            def exec_else(context: _InvocContext) -> None:
                if next_elseif < len(invoc.elseif_branches):
                    exec_level(invoc.elseif_branches[next_elseif], context, next_elseif + 1)
                    return
                assert next_elseif == len(invoc.elseif_branches)
                if invoc.else_branch:
                    # CMake completely ignores any arguments passed to `else()`
                    exec_commands(invoc.else_branch.children, context)
            result = evaluate_condition(subinvoc, context)
            match result:
                case _cc.FalseResult():
                    exec_else(context)
                    return
                case _cc.TrueResult():
                    # CMake has short-circuiting evaluation behavior across if-branches,
                    # meaning that as soon as a branch condition evaluates to true, the
                    # remaining branch conditions are not evaluated.
                    exec_commands(subinvoc.children, context)
                    return
                case _cc.UncertainResult():
                    occurrence_uncertainty = context.occurrence_uncertainty or result.reason
                    if_state = _OccurrenceUncertaintyOverlayState(context.state, result.reason)
                    if_context = _InvocContext(context.file_index, if_state, occurrence_uncertainty, context.base_path)
                    exec_commands(subinvoc.children, if_context)
                    else_state = _OccurrenceUncertaintyOverlayState(context.state, result.reason)
                    else_context = _InvocContext(context.file_index, else_state, occurrence_uncertainty,
                                                 context.base_path)
                    exec_else(else_context)
                    # Must prune taints in both substates before pushing any taints to the
                    # parent state. This is because the pruning operation needs access to
                    # the original untouched parent state.
                    if_state.prune_taints()
                    else_state.prune_taints()
                    if_state.cross_prune_taints(else_state)
                    if_state.push_taints()
                    else_state.push_taints()
                    return
            typing.assert_never(result)
        exec_level(invoc, context, 0)
        exec_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_foreach(invoc: _clp.ForeachInvoc, context: _InvocContext) -> None:
        def iterate_list(loop_var: str, items: list[str]) -> None:
            for item in items:
                state = _VariableOverlayState(context.state, {
                    loop_var: item,
                })
                exec_commands(invoc.children, context.with_state(state))
        server = create_argument_server(invoc, context)
        i = server.find_keyword({"IN"})
        if i == -1:
            loop_var = server.consume()
            if not loop_var:
                error(context.file_index, server.next_pos, "Missing loop variable in %s() invocation",
                      invoc.command_name)
                return
            if server.consume_keyword({"RANGE"}):
                raise _UnsupportedInvocSyntaxException(invoc, "RANGE keyword") from None
            items = []
            while True:
                arg = server.consume()
                if not arg:
                    break
                items.append(arg.string.string)
            iterate_list(loop_var.string.string, items)
            return
        loop_vars = []
        for _ in range(i):
            arg = server.consume()
            assert arg
            loop_vars.append(arg.string.string)
        arg = server.consume_keyword({"IN"})
        assert arg
        arg = server.consume()
        if not arg:
            return
        if arg.string.string in {"LISTS", "ITEMS"}:
            if not loop_vars:
                error(context.file_index, server.next_pos, "Missing loop variable in %s() invocation",
                      invoc.command_name)
                return
            if len(loop_vars) > 1:
                error(context.file_index, server.next_pos, "Too many loop variables in %s() invocation",
                      invoc.command_name)
                return
            items = []
            mode = arg.string.string
            while True:
                arg = server.consume_keyword({"LISTS", "ITEMS", "ZIP_LISTS"})
                if arg:
                    if arg.string.string == "ZIP_LISTS":
                        error(context.file_index, arg.pos, "ZIP_LISTS cannot be used with LISTS or ITEMS in "
                              "%s() invocation", invoc.command_name)
                        return
                    mode = arg.string.string
                arg = server.consume()
                if not arg:
                    break
                if mode == "ITEMS":
                    items.append(arg.string.string)
                    continue
                if mode == "LISTS":
                    var_name = arg.string.string
                    string = resolve_certain_variable(_cu.ResolutionType.GENERAL, var_name, arg.pos, invoc, context)
                    items += _cu.unescaping_list_split(string)
                    continue
                assert False
            iterate_list(loop_vars[0], items)
            return
        if arg.string.string == "ZIP_LISTS":
            raise _UnsupportedInvocSyntaxException(invoc, "ZIP_LISTS keyword") from None
        error(context.file_index, arg.pos, "Unrecognized keyword (%s) after IN in %s() invocation",
              _b.quote(arg.string.string), invoc.command_name)

    def exec_while(invoc: _clp.WhileInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_macro_def(invoc: _clp.MacroDefInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        name = server.consume()
        if not name:
            error(context.file_index, server.next_pos, "Missing macro name in %s() invocation", invoc.command_name)
            return
        name_cf = name.string.string.casefold()
        if _clp.is_flow_control_command(name_cf):
            error(context.file_index, name.pos, "Failed to define macro %s(): Built-in flow control commands cannot "
                  "be overridden", name.string.string)
            return
        parameters = []
        while True:
            arg = server.consume()
            if not arg:
                break
            parameters.append(arg.string.string)
        # FIXME: "Push" old definition, if any, to same name but with underscore prefix          
        defining_command_name = invoc.command_name
        definition_position = _cur.Position(context.file_index, invoc.pos)
        command = _CustomCommand(_CustomCommand.Type.MACRO, parameters, invoc.children, definition_position)
        command_2 = _CertainDefinedCommand(command, defining_command_name, definition_position)
        context.state.set_command(name_cf, command_2)
        exec_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_function_def(invoc: _clp.FunctionDefInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_block(invoc: _clp.BlockInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if server.consume_keyword({"SCOPE_FOR"}):
            raise _UnsupportedInvocSyntaxException(invoc, "SCOPE_FOR keyword") from None
        if not server.at_end:
            error(context.file_index, server.next_pos, "Unrecognized first argument in %s() invocation",
                  invoc.command_name)
            return
        state = _SubscopeState(context.state)
        exec_commands(invoc.children, context.with_state(state))
        exec_empty_closing_invoc(invoc.closing_invoc, invoc, context)

    def exec_return(invoc: _clp.ReturnInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if not server.at_end:
            raise _UnsupportedInvocSyntaxException(invoc) from None
        assert False        

    def exec_break(invoc: _clp.BreakInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_continue(invoc: _clp.ContinueInvoc, context: _InvocContext) -> None:
        assert False        

    def exec_generic(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        command = context.state.get_command(invoc.command_name_cf)
        match command:
            case _BuiltInCommand(which):
                match which:
                    case _BuiltInCommand.Which.UNSUPPORTED:
                        error(context.file_index, invoc.pos, "Invocation of unsupported command %s()",
                              invoc.command_name)
                        return
                    case _BuiltInCommand.Which.SET:
                        exec_set(invoc, context)
                        return
                    case _BuiltInCommand.Which.UNSET:
                        exec_unset(invoc, context)
                        return
                    case _BuiltInCommand.Which.STRING:
                        exec_string(invoc, context)
                        return
                    case _BuiltInCommand.Which.LIST:
                        exec_list(invoc, context)
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
                    case _BuiltInCommand.Which.BREAKPOINT:
                        breakpoint()
                        return
                typing.assert_never(which)
            case _CustomCommand():
                command_id = id(command)
                if errors_seen:
                    # Skip invocation if an error has occurred and this is a recursive
                    # invocation. This is in order to avoid unintended infinite recursion.
                    if command_id in commands_in_invoc_path:
                        return
                commands_in_invoc_path[command_id] = commands_in_invoc_path.get(command_id, 0) + 1
                try:
                    arguments = expand_arguments(invoc, context)
                    match command.type_:
                        case _CustomCommand.Type.MACRO:
                            if len(arguments) < len(command.parameters):
                                error(context.file_index, invoc.rparen_pos, "Too few arguments in invocation of "
                                      "macro %s()", invoc.command_name)
                                position = command.definition_position
                                error(position.file_index, position.pos, "Definition of macro %s()",
                                      invoc.command_name)
                                return
                            substitutions = _get_macro_substitutions(command.parameters, arguments)
                            for subinvoc in command.invocations:
                                subinvoc_2 = _macro_substitute_invoc(subinvoc, context, substitutions)
                                exec_command(subinvoc_2, context)
                            return
                        case _CustomCommand.Type.FUNCTION:
                            assert False                        
                    typing.assert_never(command.type_)
                finally:
                    n = commands_in_invoc_path[command_id]
                    assert n > 0
                    if n == 1:
                        del commands_in_invoc_path[command_id]
                    else:
                        commands_in_invoc_path[command_id] = n - 1
            case _UncertainCommand(reason):
                error(context.file_index, invoc.pos, "Invocation failed due to uncertain definition of %s()",
                      invoc.command_name)
                if reason:
                    position = reason.definition_position
                    error(position.file_index, position.pos, "Caused by execution of %s() with uncertain occurrence",
                          reason.defining_command_name)
                    trace_expansion_uncertainty_causes(reason.occurrence_uncertainty_reason)
                return
        typing.assert_never(command)

    def exec_closing_invoc(invoc: _clp.ClosingInvoc, opening_invoc: _clp.Invoc, context: _InvocContext) -> None:
        # For most block commands (all other than block()), CMake ignores arguments in a
        # closing invocation but generates a warning unless the closing invocation is either
        # empty or matches the corresponding opening invocation proto-argument for
        # proto-argument. In the case of endif(), if() is the corresponding opening
        # invocation, even if there are elseif() and/or else() invocations in between.
        if not invoc.arguments:
            return
        opening_args = [a.orig_text for a in opening_invoc.arguments]
        closing_args = [a.orig_text for a in invoc.arguments]
        if opening_args != closing_args:
            opening_position = _cur.Position(context.file_index, opening_invoc.pos)
            opening_line_no = pos_resolver.resolve_file_pos(opening_position).line_no
            warn(context.file_index, invoc.pos, "Closing invocation, %s(), has mismatching arguments (opening "
                 "invocation is on line %s)", invoc.command_name, opening_line_no)

    def exec_empty_closing_invoc(invoc: _clp.ClosingInvoc, opening_invoc: _clp.Invoc, context: _InvocContext) -> None:
        # For block() specifically, CMake requires that the closing invocation has no
        # arguments.
        if not invoc.arguments:
            return
        pos = invoc.arguments[0].pos
        opening_position = _cur.Position(context.file_index, opening_invoc.pos)
        opening_line_no = pos_resolver.resolve_file_pos(opening_position).line_no
        warn(context.file_index, pos, "Arguments in closing invocation %s() (opening invocation is on line %s)",
             invoc.command_name, opening_line_no)

    def exec_set(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        # FIXME: Consider picking up the part of the variable name that is specified, if
        # any, and use it as a tainting pattern
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing variable name in %s() invocation", invoc.command_name)
            return
        var_ref = _cu.parse_variable_reference(arg.string.string)
        var_ref_pos = arg.pos
        var_name = var_ref.variable_name
        match var_ref.resolution_type:
            case _cu.ResolutionType.GENERAL:
                values = list[str]()
                parent_scope = False
                try:
                    while True:
                        arg = server.consume()
                        if not arg:
                            break
                        if arg.string.string == "CACHE":
                            raise _UnsupportedInvocSyntaxException(invoc, "CACHE keyword") from None
                        if server.at_end and arg.string.string == "PARENT_SCOPE":
                            parent_scope = True
                            break
                        values.append(arg.string.string)
                except _ca.UncertainArgumentException as e:
                    taint_regular_variable(var_name, e.reason, context)
                    if not context.state.is_root_scope():
                        taint_parent_scope_variable(var_name, e.reason, context)
                    return
                value = _cu.nonescaping_list_join(values)
                if not parent_scope:
                    set_regular_variable(var_name, value, invoc, context)
                    return
                set_parent_scope_variable(var_name, value, invoc, context)
                return
            case _cu.ResolutionType.CACHE:
                type_: str | None = None
                force = False
                while True:
                    if server.consume_keyword({"TYPE"}):
                        arg = server.consume()
                        if not arg:
                            error(context.file_index, server.next_pos, "Missing <type> argument after TYPE keyword in "
                                  "%s() invocation", invoc.command_name)
                            return
                        type_ = arg.string.string
                        continue
                    if server.consume_keyword({"HELP"}):
                        raise _UnsupportedInvocSyntaxException(invoc, "HELP keyword") from None
                    if server.consume_keyword({"FORCE"}):
                        force = True
                        continue
                    if server.consume_keyword({"VALUE"}):
                        break
                    arg = server.consume()
                    if arg:
                        error(context.file_index, arg.pos, "Unrecognized argument (%s) before VALUE keyword in %s() "
                              "invocation", _b.quote(arg.string.string), invoc.command_name)
                        return
                    error(context.file_index, server.next_pos, "Missing VALUE keyword in %s() invocation",
                          invoc.command_name)
                    return
                if type_ is not None and type_ != "STRING":
                    raise _UnsupportedInvocSyntaxException(invoc, "Non-STRING type") from None
                if not force:
                    raise _UnsupportedInvocSyntaxException(invoc, "No FORCE keyword") from None
                # CMake accepts an empty list of values. If the list of values is empty, the
                # variable will be set to the empty string. It will not be unset (contrast
                # this with regular variables).
                values = list[str]()
                try:
                    while True:
                        arg = server.consume()
                        if not arg:
                            break
                        values.append(arg.string.string)
                except _ca.UncertainArgumentException as e:
                    taint_cache_variable(var_name, e.reason, context)
                    return
                value = _cu.nonescaping_list_join(values) or ""
                set_cache_variable(var_name, value, invoc, context)
                return
            case _cu.ResolutionType.ENV:
                # CMake accepts zero values. If zero values are specified, the effect is the
                # same as if a single empty string is specified. When an empty string is
                # specified and the environment variable is undefined, the environment
                # variable remains undefined. Otherwise the environment variable is set to
                # the specified string. Arguments beyond a single value are accepted by
                # CMake, but generate a warning.
                value = ""
                try:
                    arg = server.consume()
                    if arg:
                        value = arg.string.string
                        if not server.at_end:
                            warn(context.file_index, server.next_pos, "Extraneous unused arguments in %s() invocation",
                                 invoc.command_name)
                except _ca.UncertainArgumentException as e:
                    taint_env_variable(var_name, e.reason, context)
                    return
                if not value:
                    # Value is empty or absent
                    try:
                        orig_value = resolve_certain_variable(_cu.ResolutionType.ENV, var_name, var_ref_pos, invoc,
                                                              context)
                    except _ca.UncertainArgumentException as e:
                        # If the target variable was tainted, it remains tainted. Nothing
                        # further to do.
                        return
                    if not orig_value:
                        return
                set_env_variable(var_name, value, invoc, context)
                return
        typing.assert_never(var_ref.resolution_type)

    def exec_unset(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Missing variable name in %s() invocation", invoc.command_name)
            return
        var_ref = _cu.parse_variable_reference(arg.string.string)
        var_name = var_ref.variable_name
        match var_ref.resolution_type:
            case _cu.ResolutionType.GENERAL:
                parent_scope = False
                try:
                    arg = server.consume()
                    if arg:
                        if arg.string.string == "CACHE":
                            raise _UnsupportedInvocSyntaxException(invoc, "CACHE keyword") from None
                        if server.at_end and arg.string.string == "PARENT_SCOPE":
                            parent_scope = True
                        else:
                            error(context.file_index, server.next_pos, "Too many arguments in %s() invocation",
                                  invoc.command_name)
                except _ca.UncertainArgumentException as e:
                    # FIXME: If the variable is unset already, then no taint is needed
                    # (applies separately to parent scope)
                    taint_regular_variable(var_name, e.reason, context)
                    if not context.state.is_root_scope():
                        taint_parent_scope_variable(var_name, e.reason, context)
                    return
                value = None
                if not parent_scope:
                    set_regular_variable(var_name, value, invoc, context)
                    return
                set_parent_scope_variable(var_name, value, invoc, context)
                return
            case _cu.ResolutionType.CACHE:
                # CMake bizarrely accepts one extra unused argument without even issuing a
                # warning. If more than one extra argument is passed, CMake errors.
                server.consume()
                if not server.at_end:
                    error(context.file_index, server.next_pos, "Too many arguments in %s(CACHE{}) invocation",
                          invoc.command_name)
                value = None
                set_cache_variable(var_name, value, invoc, context)
                return
            case _cu.ResolutionType.ENV:
                # CMake bizarrely accepts one extra unused argument without even issuing a
                # warning. If more than one extra argument is passed, CMake errors.
                server.consume()
                if not server.at_end:
                    error(context.file_index, server.next_pos, "Too many arguments in %s(ENV{}) invocation",
                          invoc.command_name)
                value = None
                set_env_variable(var_name, value, invoc, context)
                return
        typing.assert_never(var_ref.resolution_type)

    def exec_string(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Too few arguments in %s() invocation", invoc.command_name)
            return
        func = arg.string.string
        if func in {"TOLOWER", "TOUPPER"}:
            uncertainty: _cur.ExpansionUncertaintyReason | None = None
            try:
                arg = server.consume()
                if not arg:
                    error(context.file_index, server.next_pos, "Missing <string> argument in %s(%s) invocation",
                          invoc.command_name, func)
                    return
                string = arg.string.string
            except _ca.UncertainArgumentException as e:
                uncertainty = e.reason
            arg = server.consume()
            if not arg:
                error(context.file_index, server.next_pos, "Missing <variable> argument in %s(%s) invocation",
                      invoc.command_name, func)
                return
            var_name = arg.string.string
            if uncertainty:
                taint_regular_variable(var_name, uncertainty, context)
                return
            # NOTE: CMake completely ignores additional arguments for these signatures
            match func:
                case "TOLOWER":
                    string_2 = string.translate(_ASCII_LOWER_MAP)
                case "TOUPPER":
                    string_2 = string.translate(_ASCII_UPPER_MAP)
                case _:
                    assert False
            set_regular_variable(var_name, string_2, invoc, context)
            return
        raise _UnsupportedInvocSyntaxException(invoc) from None

    def exec_list(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        arg = server.consume()
        if not arg:
            error(context.file_index, server.next_pos, "Too few arguments in %s() invocation", invoc.command_name)
            return
        func = arg.string.string
        if func == "APPEND":
            arg = server.consume()
            if not arg:
                error(context.file_index, server.next_pos, "Missing <variable> argument in %s(%s) invocation",
                      invoc.command_name, func)
                return
            var_name = arg.string.string
            var_name_pos = arg.pos
            try:
                elements = list[str]()
                while True:
                    arg = server.consume()
                    if not arg:
                        break
                    elements.append(arg.string.string)
            except _ca.UncertainArgumentException as e:
                taint_regular_variable(var_name, e.reason, context)
                return
            # In CMake, if no elements are appended, the original value is unchanged. If it
            # was unset, it remains unset.
            if not elements:
                return
            try:
                string = resolve_certain_variable(_cu.ResolutionType.GENERAL, var_name, var_name_pos, invoc, context)
            except _ca.UncertainArgumentException as e:
                # If the target variable was tainted, it remains tainted. Nothing further to
                # do.
                return
            # In CMake, when at least one element is appended and the list variable was
            # unset or its original value was the empty string, the new list value becomes
            # the semicolon-join of the appended elements. Otherwise, when at least one
            # element is appended, the result is the original value plus semicolon plus the
            # semicolon-join of the appended elements.
            string_2 = (string + ";" if string else "")
            string_3 = _cu.nonescaping_list_join(elements)
            assert string_3 is not None
            set_regular_variable(var_name, string_2 + string_3, invoc, context)
            return
        raise _UnsupportedInvocSyntaxException(invoc) from None

    def exec_message(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        if server.consume_keyword({"CHECK_START", "CHECK_PASS", "CHECK_FAIL", "CONFIGURE_LOG"}):
            raise _UnsupportedInvocSyntaxException(invoc, "Non-NOTICE level") from None
        level = MessageLevel.NOTICE
        arg = server.consume_keyword(_MESSAGE_LEVEL_MAP.keys())
        if arg:
            level = _MESSAGE_LEVEL_MAP[arg.string.string]
        message = ""
        while True:
            arg = server.consume()
            if not arg:
                break
            message += arg.string.string
        position = _cur.Position(context.file_index, invoc.pos)
        application.message(position, context.occurrence_uncertainty, level, message)

    def exec_include(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        server = create_argument_server(invoc, context)
        file_or_module = server.consume()
        if not file_or_module:
            error(context.file_index, server.next_pos, "Missing file or module in %s()", invoc.command_name)
            return None
        if not re.fullmatch(r".*\.cmake", file_or_module.string.string):
            raise _UnsupportedInvocSyntaxException(invoc, "Non-path argument") from None
        if not server.at_end:
            raise _UnsupportedInvocSyntaxException(invoc) from None
        cmake_path = context.base_path.parent / file_or_module.string.string
        try:
            with open(cmake_path, "r") as file_:
                cmake_source = Source(file_, cmake_path)
                process_file(cmake_source, context.state, context.occurrence_uncertainty, context.base_path)
        except FileNotFoundError as e:
            error(context.file_index, invoc.pos, "Failed to include %s: %s", _b.quote(str(cmake_path)), e.strerror)

    def exec_add_subdirectory(invoc: _clp.GenericInvoc, context: _InvocContext) -> None:
        assert False        

    def evaluate_condition(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> _cc.Result:
        try:
            arguments = expand_arguments(invoc, context)
            condition = _cc.parse(arguments, invoc.rparen_pos)
            class State(_cv.VariableState):
                @typing.override
                def get(self, resolution_type: _cu.ResolutionType, variable_name: str,
                        pos: int) -> tuple[_cv.Value, _cv.VariableType]:
                    return resolve_variable(resolution_type, variable_name, pos, context)
                @typing.override
                def set_(self, variable_name: str, value: str | None) -> None:
                    set_regular_variable(variable_name, value, invoc, context)
                @typing.override
                def taint(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
                    taint_regular_variable(variable_name, reason, context)
            state = State()
            return _cc.evaluate(condition, invoc.command_name, context.file_index, state)
        except _cc.FatalParseError as e:
            raise _ConditionParseError(e.pos, invoc.command_name, e.message % e.args) from None
        except _cc.FatalEvalError as e:
            raise _ConditionEvalError(e.pos, invoc.command_name, e.message % e.args) from None

    def create_argument_server(invoc: _clp.Invoc, context: _InvocContext) -> _ca.ArgumentServer:
        arguments = expand_arguments(invoc, context)
        return _ca.ArgumentServer(invoc, arguments, context.file_index)

    def expand_arguments(invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> list[_ca.Argument]:
        arguments = list[_ca.Argument]()
        for protoarg in invoc.arguments:
            arg: _ca.Argument
            match protoarg:
                case _clp.CertainProtoargument():
                    match protoarg.type_:
                        case _clp.ProtoargumentType.BARE:
                            was_bare = True
                            result = expand_string(protoarg.string, invoc, context)
                            match result:
                                case _CertainExpansionResult():
                                    is_derived = protoarg.is_derived or result.is_derived
                                    segments, is_derived_2 = _list_split(result.string, is_derived)
                                    for segment in segments:
                                        arg = _ca.CertainArgument(protoarg.pos, was_bare, segment, is_derived_2)
                                        arguments.append(arg)
                                    continue
                                case _UncertainExpansionResult():
                                    # Note that an uncertain unquoted proto-argument stands in for
                                    # any number of actual arguments, including zero.
                                    arg = _ca.UncertainArgument(protoarg.pos, was_bare, result.reason)
                                    arguments.append(arg)
                                    continue
                            typing.assert_never(result)
                        case _clp.ProtoargumentType.QUOTED:
                            was_bare = False
                            result = expand_string(protoarg.string, invoc, context)
                            match result:
                                case _CertainExpansionResult():
                                    is_derived = protoarg.is_derived or result.is_derived
                                    arg = _ca.CertainArgument(protoarg.pos, was_bare, result.string, is_derived)
                                    arguments.append(arg)
                                    continue
                                case _UncertainExpansionResult():
                                    arg = _ca.UncertainArgument(protoarg.pos, was_bare, result.reason)
                                    arguments.append(arg)
                                    continue
                            typing.assert_never(result)
                        case _clp.ProtoargumentType.BRACKETED:
                            assert not protoarg.is_derived
                            was_bare = False
                            is_derived = False
                            arg = _ca.CertainArgument(protoarg.pos, was_bare, protoarg.string, is_derived)
                            arguments.append(arg)
                            continue
                    typing.assert_never(protoarg.type_)
                case _clp.UncertainProtoargument():
                    match protoarg.type_:
                        case _clp.ProtoargumentType.BARE:
                            was_bare = True
                        case _clp.ProtoargumentType.QUOTED:
                            was_bare = False
                        case _clp.ProtoargumentType.BRACKETED:
                            assert False  # Not affected by macro substitutions
                        case _:
                            typing.assert_never(protoarg.type_)
                    arg = _ca.UncertainArgument(protoarg.pos, was_bare, protoarg.reason)
                    arguments.append(arg)
                    continue
            typing.assert_never(protoarg)
        return arguments

    def expand_string(string: _tp.PosMappedString, invoc: _clp.GeneralizedInvoc,
                      context: _InvocContext) -> _ExpansionResult:
        def expand(expr: _csp.Expr) -> _ExpansionResult:
            if isinstance(expr, _csp.StringExpr):
                is_derived = False
                return _CertainExpansionResult(expr.string, is_derived)
            if isinstance(expr, _csp.CompositeExpr):
                string_builder = _tp.PosMappedStringBuilder()
                for part in expr.parts:
                    result = expand(part)
                    if isinstance(result, _CertainExpansionResult):
                        string_builder.add_pos_mapped_string(result.string)
                        continue
                    if isinstance(result, _UncertainExpansionResult):
                        return result
                    typing.assert_never(result)
                is_derived = True
                return _CertainExpansionResult(string_builder.finalize_and_get(), is_derived)
            if isinstance(expr, _csp.ExpansionExpr):
                result = expand(expr.name_expr)
                if isinstance(result, _CertainExpansionResult):
                    name = result.string.string
                    value, variable_type = resolve_variable(expr.resolution_type, name, expr.name_expr.pos, context)
                    if isinstance(value, _cv.CertainValue):
                        string = _tp.PosMappedString.from_nonlinear_string(value.string or "", expr.pos)
                        is_derived = True
                        return _CertainExpansionResult(string, is_derived)
                    if isinstance(value, _cv.UncertainValue):
                        param_type = _cv.variable_to_param_type(variable_type)
                        position = _cur.Position(context.file_index, expr.pos)
                        reason = _cur.ExpansionUncertaintyReason(invoc.command_name, param_type, name, position,
                                                                 value.reason)
                        return _UncertainExpansionResult(reason)
                    typing.assert_never(value)
                if isinstance(result, _UncertainExpansionResult):
                    return result
                typing.assert_never(result)
            typing.assert_never(expr)
        def error_handler(pos: int, message: str, *args: typing.Any) -> None:
            error(context.file_index, pos, message, *args)
        return expand(_csp.parse(string, error_handler))

    def resolve_certain_variable(resolution_type: _cu.ResolutionType, variable_name: str, pos: int,
                                 invoc: _clp.GeneralizedInvoc, context: _InvocContext) -> str | None:
        value, variable_type = resolve_variable(resolution_type, variable_name, pos, context)
        match value:
            case _cv.CertainValue():
                return value.string
            case _cv.UncertainValue():
                param_type = _cv.variable_to_param_type(variable_type)
                expansion_position = _cur.Position(context.file_index, pos)
                reason = _cur.ExpansionUncertaintyReason(invoc.command_name, param_type, variable_name,
                                                         expansion_position, value.reason)
                raise _ca.UncertainArgumentException(reason) from None
        typing.assert_never(value)

    def resolve_variable(resolution_type: _cu.ResolutionType, variable_name: str, pos: int,
                         context: _InvocContext) -> tuple[_cv.Value, _cv.VariableType]:
        match resolution_type:
            case _cu.ResolutionType.GENERAL:
                value = context.state.get_regular_variable(variable_name)
                match value:
                    case _cv.CertainValue(string):
                        if string is not None:
                            return value, _cv.VariableType.REGULAR
                    case _cv.UncertainValue():
                        return value, _cv.VariableType.REGULAR
                    case _:
                        typing.assert_never(value)
                return context.state.get_cache_variable(variable_name), _cv.VariableType.CACHE
            case _cu.ResolutionType.CACHE:
                return context.state.get_cache_variable(variable_name), _cv.VariableType.CACHE
            case _cu.ResolutionType.ENV:
                return context.state.get_env_variable(variable_name), _cv.VariableType.ENV
        typing.assert_never(resolution_type)

    def set_regular_variable(variable_name: str, value: str | None, invoc: _clp.GeneralizedInvoc,
                             context: _InvocContext) -> None:
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value_2 = _CertainAssignedValue(value, invoc.command_name, assignment_position)
        context.state.set_regular_variable(variable_name, value_2)

    def taint_regular_variable(variable_name: str, reason: _cur.ValueUncertaintyReason,
                               context: _InvocContext) -> None:
        context.state.set_regular_variable(variable_name, _cv.UncertainValue(reason))

    def set_parent_scope_variable(variable_name: str, value: str | None, invoc: _clp.GeneralizedInvoc,
                                  context: _InvocContext) -> None:
        if context.state.is_root_scope():
            warn(context.file_index, invoc.pos, "%s() invocation skipped: No parent scope exists", invoc.command_name)
            return
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value_2 = _CertainAssignedValue(value, invoc.command_name, assignment_position)
        context.state.set_parent_scope_variable(variable_name, value_2)

    def taint_parent_scope_variable(variable_name: str, reason: _cur.ValueUncertaintyReason,
                                    context: _InvocContext) -> None:
        assert not context.state.is_root_scope()
        context.state.set_parent_scope_variable(variable_name, _cv.UncertainValue(reason))

    def set_cache_variable(variable_name: str, value: str | None, invoc: _clp.GeneralizedInvoc,
                           context: _InvocContext) -> None:
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value_2 = _CertainAssignedValue(value, invoc.command_name, assignment_position)
        context.state.set_cache_variable(variable_name, value_2)

    def taint_cache_variable(variable_name: str, reason: _cur.ValueUncertaintyReason, context: _InvocContext) -> None:
        context.state.set_cache_variable(variable_name, _cv.UncertainValue(reason))

    def set_env_variable(variable_name: str, value: str | None, invoc: _clp.GeneralizedInvoc,
                         context: _InvocContext) -> None:
        assignment_position = _cur.Position(context.file_index, invoc.pos)
        value_2 = _CertainAssignedValue(value, invoc.command_name, assignment_position)
        context.state.set_env_variable(variable_name, value_2)

    def taint_env_variable(variable_name: str, reason: _cur.ValueUncertaintyReason, context: _InvocContext) -> None:
        context.state.set_env_variable(variable_name, _cv.UncertainValue(reason))

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
        typing.assert_never(cause)

    def trace_expansion_uncertainty_causes(cause: _cur.ExpansionUncertaintyReason) -> None:
        position = cause.expansion_position
        error(position.file_index, position.pos, "Caused by expansion of %s with uncertain value in invocation of "
              "%s()", cause.get_qual_param_ref(), cause.command_name)
        if cause.value_uncertainty_reason:
            trace_value_uncertainty_causes(cause.value_uncertainty_reason)

    def warn(file_index: int, pos: int, message: str, *args: typing.Any) -> None:
        position = _cur.Position(file_index, pos)
        application.warn(position, message, *args)

    def error(file_index: int, pos: int, message: str, *args: typing.Any) -> None:
        nonlocal errors_seen
        errors_seen = True
        position = _cur.Position(file_index, pos)
        application.error(position, message, *args)

    state = _RootState(config.define_breakpoint_command)
    occurrence_uncertainty = None
    base_path = cmake_source.path
    process_file(cmake_source, state, occurrence_uncertainty, base_path)
    return not errors_seen


@dataclasses.dataclass(slots=True, frozen=True)
class _SourceFile:
    pos_tracker: _tp.FilePosTracker


@dataclasses.dataclass(slots=True, frozen=True)
class _InvocContext:
    file_index:             int
    state:                  _State
    occurrence_uncertainty: OccurrenceUncertainty
    base_path:              pathlib.Path

    def with_state(self, state: _State) -> _InvocContext:
        return _InvocContext(self.file_index, state, self.occurrence_uncertainty, self.base_path)


class _State(abc.ABC):
    @abc.abstractmethod
    def is_root_scope(self) -> bool:
        ...

    @abc.abstractmethod
    def get_regular_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        ...

    @abc.abstractmethod
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        ...

    @abc.abstractmethod
    def get_cache_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        ...

    @abc.abstractmethod
    def get_env_variable(self, name: str) -> _cv.Value:
        ...

    @abc.abstractmethod
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        ...

    @abc.abstractmethod
    def get_command(self, name_cf: str) -> _Command:
        ...

    @abc.abstractmethod
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        ...


class _RootState(_State):
    def __init__(self, define_breakpoint_command: bool) -> None:
        self._commands          = dict[str, _Command]()
        self._env_variables     = dict[str, _cv.Value]()
        self._cache_variables   = dict[str, _cv.Value]()
        self._regular_variables = dict[str, _cv.Value]()
        _define_built_in_commands(self._commands, define_breakpoint_command)

    @typing.override
    def is_root_scope(self) -> bool:
        return True

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._regular_variables.get(name)
        if value:
            return value
        reason = None
        return _cv.UncertainValue(reason)

    @typing.override
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        match value:
            case _CertainAssignedValue():
                self._regular_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._regular_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        assert False

    @typing.override
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        assert False

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        value = self._cache_variables.get(name)
        if value:
            return value
        reason = None
        return _cv.UncertainValue(reason)

    @typing.override
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        match value:
            case _CertainAssignedValue():
                self._cache_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._cache_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_env_variable(self, name: str) -> _cv.Value:
        value = self._env_variables.get(name)
        if value:
            return value
        reason = None
        return _cv.UncertainValue(reason)

    @typing.override
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        match value:
            case _CertainAssignedValue():
                self._env_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._env_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        command = self._commands.get(name_cf)
        if command:
            return command
        reason = None
        return _UncertainCommand(reason)

    @typing.override
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        match command:
            case _CertainDefinedCommand():
                self._commands[name_cf] = command.command
                return
            case _UncertainCommand():
                self._commands[name_cf] = command
                return
        typing.assert_never(command)


class _SubscopeState(_State):
    def __init__(self, parent_state: _State) -> None:
        self._parent_state      = parent_state
        self._regular_variables = dict[str, _cv.Value]()

    @typing.override
    def is_root_scope(self) -> bool:
        return False

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._regular_variables.get(name)
        if value:
            return value
        return self._parent_state.get_regular_variable(name)

    @typing.override
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        match value:
            case _CertainAssignedValue():
                self._regular_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._regular_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_regular_variable(name)

    @typing.override
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        # Copy original value from parent scope before changing it in the parent scope. This
        # is necessary in order to uphold the illusion that the subscope holds a complete
        # copy of the original parent variable state.
        if name not in self._regular_variables:
            self._regular_variables[name] = self._parent_state.get_regular_variable(name)
        self._parent_state.set_regular_variable(name, value)

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_cache_variable(name)

    @typing.override
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_state.set_cache_variable(name, value)

    @typing.override
    def get_env_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_env_variable(name)

    @typing.override
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_state.set_env_variable(name, value)

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        return self._parent_state.get_command(name_cf)

    @typing.override
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        self._parent_state.set_command(name_cf, command)


class _VariableOverlayState(_State):
    def __init__(self, parent_state: _State, variables: dict[str, str]) -> None:
        self._parent_state      = parent_state
        self._regular_variables = dict[str, _cv.Value]()
        for name, value in variables.items():
            self._regular_variables[name] = _cv.CertainValue(value)

    @typing.override
    def is_root_scope(self) -> bool:
        return self._parent_state.is_root_scope()

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._regular_variables.get(name)
        if value is not None:
            return value
        return self._parent_state.get_regular_variable(name)

    @typing.override
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        if name not in self._regular_variables:
            self._parent_state.set_regular_variable(name, value)
            return
        match value:
            case _CertainAssignedValue():
                self._regular_variables[name] = _cv.CertainValue(value.string)
                return
            case _cv.UncertainValue():
                self._regular_variables[name] = value
                return
        typing.assert_never(value)

    @typing.override
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_parent_scope_variable(name)

    @typing.override
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_state.set_parent_scope_variable(name, value)

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_cache_variable(name)

    @typing.override
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_state.set_cache_variable(name, value)

    @typing.override
    def get_env_variable(self, name: str) -> _cv.Value:
        return self._parent_state.get_env_variable(name)

    @typing.override
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_state.set_env_variable(name, value)

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        return self._parent_state.get_command(name_cf)

    @typing.override
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        self._parent_state.set_command(name_cf, command)


class _OccurrenceUncertaintyOverlayState(_State):
    def __init__(self, parent_state: _State, occurrence_uncertainty_reason: _cur.ExpansionUncertaintyReason) -> None:
        self._parent_state                   = parent_state
        self._occurrence_uncertainty_reason  = occurrence_uncertainty_reason
        self._commands                       = dict[str, _DefinedCommand]()
        self._tainted_commands               = dict[str, _UncertainCommand]()
        self._env_variables                  = dict[str, _AssignedValue]()
        self._tainted_env_variables          = dict[str, _cv.UncertainValue]()
        self._cache_variables                = dict[str, _AssignedValue]()
        self._tainted_cache_variables        = dict[str, _cv.UncertainValue]()
        self._regular_variables              = dict[str, _AssignedValue]()
        self._tainted_regular_variables      = dict[str, _cv.UncertainValue]()
        self._parent_scope_variables         = dict[str, _AssignedValue]()
        self._tainted_parent_scope_variables = dict[str, _cv.UncertainValue]()

    def prune_taints(self) -> None:
        # Command taints are uncancellable because it is impossible in CMake to assign a
        # command to a name that is the same command as the name once referred to. One can
        # imagine assigning a new macro with the same macro body, but it is still not the
        # same macro because the metadata (position in source code of definition) is
        # necessarily different. Also, the new body may be nested inside another macro body,
        # which means that substitutions may occur.
        for name, value in list(self._env_variables.items()):
            match value:
                case _CertainAssignedValue():
                    parent_value = self._parent_state.get_env_variable(name)
                    match parent_value:
                        case _cv.CertainValue():
                            if parent_value.string == value.string:
                                del self._env_variables[name]
                                del self._tainted_env_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(parent_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._cache_variables.items()):
            match value:
                case _CertainAssignedValue():
                    parent_value = self._parent_state.get_cache_variable(name)
                    match parent_value:
                        case _cv.CertainValue():
                            if parent_value.string == value.string:
                                del self._cache_variables[name]
                                del self._tainted_cache_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(parent_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._regular_variables.items()):
            match value:
                case _CertainAssignedValue():
                    parent_value = self._parent_state.get_regular_variable(name)
                    match parent_value:
                        case _cv.CertainValue():
                            if parent_value.string == value.string:
                                del self._regular_variables[name]
                                del self._tainted_regular_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(parent_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._parent_scope_variables.items()):
            match value:
                case _CertainAssignedValue():
                    parent_value = self._parent_state.get_parent_scope_variable(name)
                    match parent_value:
                        case _cv.CertainValue():
                            if parent_value.string == value.string:
                                del self._parent_scope_variables[name]
                                del self._tainted_parent_scope_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(parent_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)

    def cross_prune_taints(self, else_state: _OccurrenceUncertaintyOverlayState) -> None:
        assert self._parent_state is else_state._parent_state
        # Commands are uncomparable. See prune_taints().
        for name, value in list(self._env_variables.items()):
            match value:
                case _CertainAssignedValue():
                    else_value = else_state._env_variables.get(name)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedValue():
                            if value.string == else_value.string:
                                self._parent_state.set_env_variable(name, value)
                                del self._env_variables[name]
                                del self._tainted_env_variables[name]
                                del else_state._env_variables[name]
                                del else_state._tainted_env_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(else_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._cache_variables.items()):
            match value:
                case _CertainAssignedValue():
                    else_value = else_state._cache_variables.get(name)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedValue():
                            if value.string == else_value.string:
                                self._parent_state.set_cache_variable(name, value)
                                del self._cache_variables[name]
                                del self._tainted_cache_variables[name]
                                del else_state._cache_variables[name]
                                del else_state._tainted_cache_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(else_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._regular_variables.items()):
            match value:
                case _CertainAssignedValue():
                    else_value = else_state._regular_variables.get(name)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedValue():
                            if value.string == else_value.string:
                                self._parent_state.set_regular_variable(name, value)
                                del self._regular_variables[name]
                                del self._tainted_regular_variables[name]
                                del else_state._regular_variables[name]
                                del else_state._tainted_regular_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(else_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)
        for name, value in list(self._parent_scope_variables.items()):
            match value:
                case _CertainAssignedValue():
                    else_value = else_state._parent_scope_variables.get(name)
                    if not else_value:
                        continue
                    match else_value:
                        case _CertainAssignedValue():
                            if value.string == else_value.string:
                                self._parent_state.set_parent_scope_variable(name, value)
                                del self._parent_scope_variables[name]
                                del self._tainted_parent_scope_variables[name]
                                del else_state._parent_scope_variables[name]
                                del else_state._tainted_parent_scope_variables[name]
                            continue
                        case _cv.UncertainValue():
                            continue
                    typing.assert_never(else_value)
                case _cv.UncertainValue():
                    continue
            typing.assert_never(value)

    def push_taints(self) -> None:
        value: typing.Any
        for name_cf, value in self._tainted_commands.items():
            self._parent_state.set_command(name_cf, value)
        for name, value in self._tainted_env_variables.items():
            self._parent_state.set_env_variable(name, value)
        for name, value in self._tainted_cache_variables.items():
            self._parent_state.set_cache_variable(name, value)
        for name, value in self._tainted_regular_variables.items():
            self._parent_state.set_regular_variable(name, value)
        for name, value in self._tainted_parent_scope_variables.items():
            self._parent_state.set_parent_scope_variable(name, value)

    @typing.override
    def is_root_scope(self) -> bool:
        return self._parent_state.is_root_scope()

    @typing.override
    def get_regular_variable(self, name: str) -> _cv.Value:
        value = self._regular_variables.get(name)
        if value:
            match value:
                case _CertainAssignedValue():
                    return _cv.CertainValue(value.string)
                case _cv.UncertainValue():
                    return value
            typing.assert_never(value)
        return self._parent_state.get_regular_variable(name)

    @typing.override
    def set_regular_variable(self, name: str, value: _AssignedValue) -> None:
        self._regular_variables[name] = value
        match value:
            case _CertainAssignedValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _cv.UncertainValue(reason)
            case _cv.UncertainValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_regular_variables[name] = taint_value

    @typing.override
    def get_parent_scope_variable(self, name: str) -> _cv.Value:
        value = self._parent_scope_variables.get(name)
        if value:
            match value:
                case _CertainAssignedValue():
                    return _cv.CertainValue(value.string)
                case _cv.UncertainValue():
                    return value
            typing.assert_never(value)
        return self._parent_state.get_parent_scope_variable(name)

    @typing.override
    def set_parent_scope_variable(self, name: str, value: _AssignedValue) -> None:
        self._parent_scope_variables[name] = value
        match value:
            case _CertainAssignedValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _cv.UncertainValue(reason)
            case _cv.UncertainValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_parent_scope_variables[name] = taint_value

    @typing.override
    def get_cache_variable(self, name: str) -> _cv.Value:
        value = self._cache_variables.get(name)
        if value:
            match value:
                case _CertainAssignedValue():
                    return _cv.CertainValue(value.string)
                case _cv.UncertainValue():
                    return value
            typing.assert_never(value)
        return self._parent_state.get_cache_variable(name)

    @typing.override
    def set_cache_variable(self, name: str, value: _AssignedValue) -> None:
        self._cache_variables[name] = value
        match value:
            case _CertainAssignedValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _cv.UncertainValue(reason)
            case _cv.UncertainValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_cache_variables[name] = taint_value

    @typing.override
    def get_env_variable(self, name: str) -> _cv.Value:
        value = self._env_variables.get(name)
        if value:
            match value:
                case _CertainAssignedValue():
                    return _cv.CertainValue(value.string)
                case _cv.UncertainValue():
                    return value
            typing.assert_never(value)
        return self._parent_state.get_env_variable(name)

    @typing.override
    def set_env_variable(self, name: str, value: _AssignedValue) -> None:
        self._env_variables[name] = value
        match value:
            case _CertainAssignedValue():
                reason = _cur.AssignmentOccurrenceUncertaintyReason(value.assigning_command_name,
                                                                    value.assignment_position,
                                                                    self._occurrence_uncertainty_reason)
                taint_value = _cv.UncertainValue(reason)
            case _cv.UncertainValue():
                taint_value = value
            case _:
                typing.assert_never(value)
        self._tainted_env_variables[name] = taint_value

    @typing.override
    def get_command(self, name_cf: str) -> _Command:
        command = self._commands.get(name_cf)
        if command:
            match command:
                case _CertainDefinedCommand():
                    return command.command
                case _UncertainCommand():
                    return command
            typing.assert_never(command)
        return self._parent_state.get_command(name_cf)

    @typing.override
    def set_command(self, name_cf: str, command: _DefinedCommand) -> None:
        self._commands[name_cf] = command
        match command:
            case _CertainDefinedCommand():
                reason = _CommandDefinitionUncertaintyReason(command.defining_command_name,
                                                             command.definition_position,
                                                             self._occurrence_uncertainty_reason)
                taint_command = _UncertainCommand(reason)
            case _UncertainCommand():
                taint_command = command
            case _:
                typing.assert_never(command)
        self._tainted_commands[name_cf] = taint_command


type _Command = _CertainCommand | _UncertainCommand

type _CertainCommand = _BuiltInCommand | _CustomCommand

type _DefinedCommand = _CertainDefinedCommand | _UncertainCommand

@dataclasses.dataclass(slots=True, frozen=True)
class _BuiltInCommand:
    class Which(enum.Enum):
        UNSUPPORTED      = 0
        SET              = 1
        UNSET            = 2
        STRING           = 3
        LIST             = 4
        MESSAGE          = 5
        INCLUDE          = 6
        ADD_SUBDIRECTORY = 7
        BREAKPOINT       = 8  # Non-standard
    which: Which

@dataclasses.dataclass(slots=True, frozen=True)
class _CustomCommand:
    class Type(enum.Enum):
        MACRO    = 0
        FUNCTION = 1
    type_:               Type
    parameters:          list[str]
    invocations:         list[_clp.Invoc]
    definition_position: _cur.Position

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainDefinedCommand:
    command:               _CertainCommand
    defining_command_name: str
    definition_position:   _cur.Position

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainCommand:
    reason: _CommandDefinitionUncertaintyReason | None


@dataclasses.dataclass(slots=True, frozen=True)
class _CommandDefinitionUncertaintyReason:
    defining_command_name:         str
    definition_position:           _cur.Position
    occurrence_uncertainty_reason: _cur.ExpansionUncertaintyReason


def _define_built_in_commands(commands: dict[str, _Command], define_breakpoint_command: bool) -> None:
    def define(name_cf: str, which: _BuiltInCommand.Which) -> None:
        commands[name_cf] = _BuiltInCommand(which)
    define("set",                    _BuiltInCommand.Which.SET)
    define("unset",                  _BuiltInCommand.Which.UNSET)
    define("string",                 _BuiltInCommand.Which.STRING)
    define("list",                   _BuiltInCommand.Which.LIST)
    define("message",                _BuiltInCommand.Which.MESSAGE)
    define("include",                _BuiltInCommand.Which.INCLUDE)
    define("add_subdirectory",       _BuiltInCommand.Which.ADD_SUBDIRECTORY)
    define("cmake_minimum_required", _BuiltInCommand.Which.UNSUPPORTED)
    define("project",                _BuiltInCommand.Which.UNSUPPORTED)
    define("option",                 _BuiltInCommand.Which.UNSUPPORTED)
    if define_breakpoint_command:
        define("breakpoint", _BuiltInCommand.Which.BREAKPOINT)


type _AssignedValue = _CertainAssignedValue | _cv.UncertainValue

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainAssignedValue:
    string:                 str | None
    assigning_command_name: str
    assignment_position:    _cur.Position


class _UnsupportedInvocSyntaxException(Exception):
    def __init__(self, invoc: _clp.GeneralizedInvoc, *args: typing.Any) -> None:
        Exception.__init__(self, *args)
        self.invoc = invoc


class _ConditionParseError(Exception):
    def __init__(self, pos: int, command_name: str, message: str) -> None:
        Exception.__init__(self)
        self.pos          = pos
        self.command_name = command_name
        self.message      = message


class _ConditionEvalError(Exception):
    def __init__(self, pos: int, command_name: str, message: str) -> None:
        Exception.__init__(self)
        self.pos          = pos
        self.command_name = command_name
        self.message      = message


type _ExpansionResult = _CertainExpansionResult | _UncertainExpansionResult

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainExpansionResult:
    string:     _tp.PosMappedString
    is_derived: bool

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainExpansionResult:
    reason: _cur.ExpansionUncertaintyReason


def _list_split(string: _tp.PosMappedString, is_derived: bool) -> tuple[list[_tp.PosMappedString], bool]:
    if not string.string:
        return [], True
    string_builder = _tp.PosMappedStringBuilder()
    parts = []
    def flush() -> None:
        parts.append(string_builder.finalize_and_get().map_through(string.pos_map))
    is_derived_2 = is_derived
    i = 0
    while True:
        string_builder.bump_ref_pos_to(i)
        j = string.string.find(";", i)
        if j == -1:
            string_builder.add_linear(string.string[i:])
            break
        is_derived_2 = True
        if j > i and string.string[j-1] == "\\":
            string_builder.add_linear(string.string[i:j-1])
            string_builder.add_nonlinear(";")
            i = j + 1
            continue
        string_builder.add_linear(string.string[i:j])
        flush()
        i = j + 1
        string_builder = _tp.PosMappedStringBuilder()
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


def _get_macro_substitutions(parameters: list[str], arguments: list[_ca.Argument]) -> dict[str, _SubstitutionValue]:
    substitutions = dict[str, _SubstitutionValue]()

    for i, param in enumerate(parameters):
        assert i < len(arguments)
        arg = arguments[i]
        match arg:
            case _ca.CertainArgument():
                substitutions[param] = _CertainSubstitutionValue(arg.string.string)
                continue
            case _ca.UncertainArgument():
                substitutions[param] = _UncertainSubstitutionValue(arg.reason)
                continue
        typing.assert_never(arg)

    # Special variables will shadow formal parameters of the same name. This is consistent
    # with CMake behavior.
    all_args   = list[str]()
    extra_args = list[str]()
    all_uncertainty:   _cur.ExpansionUncertaintyReason | None = None
    extra_uncertainty: _cur.ExpansionUncertaintyReason | None = None
    for i, arg in enumerate(arguments):
        name = "ARGV%s" % i
        match arg:
            case _ca.CertainArgument():
                substitutions[name] = _CertainSubstitutionValue(arg.string.string)
                all_args.append(arg.string.string)
                if i >= len(parameters):
                    extra_args.append(arg.string.string)
                continue
            case _ca.UncertainArgument():
                substitutions[name] = _UncertainSubstitutionValue(arg.reason)
                if not all_uncertainty:
                    all_uncertainty = arg.reason
                if i >= len(parameters):
                    if not extra_uncertainty:
                        extra_uncertainty = arg.reason
                continue
        typing.assert_never(arg)
    if all_uncertainty:
        substitutions["ARGV"] = _UncertainSubstitutionValue(all_uncertainty)
    else:
        string = _cu.nonescaping_list_join(all_args) or ""
        substitutions["ARGV"] = _CertainSubstitutionValue(string)
    if extra_uncertainty:
        substitutions["ARGN"] = _UncertainSubstitutionValue(extra_uncertainty)
    else:
        string = _cu.nonescaping_list_join(extra_args) or ""
        substitutions["ARGN"] = _CertainSubstitutionValue(string)
    substitutions["ARGC"] = _CertainSubstitutionValue(str(len(arguments)))

    return substitutions


def _macro_substitute_invoc(invoc: _clp.Invoc, context: _InvocContext,
                            substitutions: dict[str, _SubstitutionValue]) -> _clp.Invoc:
    def subst_invoc[T: _clp.GeneralizedInvoc](invoc: T) -> tuple[T, bool]:
        new_arguments, arguments_changed = subst_arguments(invoc)
        match invoc:
            case _clp.IfInvoc():
                new_children, children_changed = subst_children(invoc)
                new_closing_invoc, closing_invoc_cahnged = subst_invoc(invoc.closing_invoc)
                new_elseif_branches = list[_clp.IfBranch]()
                elseif_branches_changed = False
                for branch in invoc.elseif_branches:
                    new_branch, branch_changed = subst_invoc(branch)
                    new_elseif_branches.append(new_branch)
                    elseif_branches_changed |= branch_changed
                new_else_branch: _clp.IfBranch | None = None
                else_branch_changed = False
                if invoc.else_branch:
                    new_else_branch, else_branch_changed = subst_invoc(invoc.else_branch)
                if not arguments_changed and not children_changed and not elseif_branches_changed and \
                   not else_branch_changed and not closing_invoc_cahnged:
                    return invoc, False
                return dataclasses.replace(invoc, arguments=new_arguments, children=new_children,
                                           closing_invoc=new_closing_invoc, elseif_branches=new_elseif_branches,
                                           else_branch=new_else_branch), True
            case _clp.IfBranch():
                new_children, children_changed = subst_children(invoc)
                if not arguments_changed and not children_changed:
                    return invoc, False
                return dataclasses.replace(invoc, arguments=new_arguments, children=new_children), True
            case _clp.ForeachInvoc() | _clp.WhileInvoc() | _clp.MacroDefInvoc() | _clp.FunctionDefInvoc() | \
                 _clp.BlockInvoc():
                new_children, children_changed = subst_children(invoc)
                new_closing_invoc, closing_invoc_cahnged = subst_invoc(invoc.closing_invoc)
                if not arguments_changed and not children_changed and not closing_invoc_cahnged:
                    return invoc, False
                return dataclasses.replace(invoc, arguments=new_arguments, children=new_children,
                                           closing_invoc=new_closing_invoc), True
            case _clp.ClosingInvoc() | _clp.ReturnInvoc() | _clp.BreakInvoc() | _clp.ContinueInvoc() | \
                 _clp.GenericInvoc():
                if not arguments_changed:
                    return invoc, False
                return dataclasses.replace(invoc, arguments=new_arguments), True
        typing.assert_never(invoc)

    def subst_arguments(invoc: _clp.InvocBase) -> tuple[list[_clp.Protoargument], bool]:
        new_arguments = list[_clp.Protoargument]()
        arguments_changed = False
        for arg in invoc.arguments:
            match arg.type_:
                case _clp.ProtoargumentType.BARE | _clp.ProtoargumentType.QUOTED:
                    new_arg, arg_changed = subst_arg(arg, invoc)
                    new_arguments.append(new_arg)
                    arguments_changed |= arg_changed
                    continue
                case _clp.ProtoargumentType.BRACKETED:
                    # CMake does not apply macro substitutions inside bracketed arguments
                    new_arguments.append(arg)
                    continue
            typing.assert_never(arg.type_)
        if arguments_changed:
            return new_arguments, True
        return invoc.arguments, False

    def subst_arg(arg: _clp.Protoargument, invoc: _clp.InvocBase) -> tuple[_clp.Protoargument, bool]:
        match arg:
            case _clp.CertainProtoargument():
                is_changed = False
                is_derived = arg.is_derived
                builder = _tp.PosMapBuilder()
                pos = 0
                def replacer(m: re.Match[str]) -> str:
                    nonlocal is_changed, is_derived, pos
                    param = m.group(1)
                    value = substitutions.get(param)
                    if value is None:
                        return m.group(0)
                    match_pos = m.start()
                    match value:
                        case _CertainSubstitutionValue():
                            is_changed = True
                            is_derived = True
                            builder.add_linear(match_pos - pos, pos)
                            builder.add_nonlinear(len(value.string), match_pos)
                            pos = m.end()
                            return value.string
                        case _UncertainSubstitutionValue():
                            param_type = _cur.ParamType.MACRO_PARAM
                            ref_pos = arg.string.pos_map.map_(match_pos)
                            expansion_position = _cur.Position(context.file_index, ref_pos)
                            reason = _cur.ExpansionUncertaintyReason(invoc.command_name, param_type, param,
                                                                     expansion_position, value.reason)
                            raise _UncertainSubstitutionException(reason) from None
                    typing.assert_never(value)
                try:
                    new_string = _MACRO_SUBSTITUTE_REGEX.sub(replacer, arg.string.string)
                except _UncertainSubstitutionException as e:
                    return _clp.UncertainProtoargument(arg.type_, arg.orig_text, arg.pos, e.reason), True
                if not is_changed:
                    return arg, False
                builder.add_linear(len(arg.string.string) - pos, pos)
                pos_map = arg.string.pos_map.compose_with(builder.finalize_and_get())
                new_string_2 = _tp.PosMappedString(new_string, pos_map)
                return _clp.CertainProtoargument(arg.type_, arg.orig_text, arg.pos, new_string_2, is_derived), True
            case _clp.UncertainProtoargument():
                return arg, False
        typing.assert_never(arg)

    def subst_children(invoc: _clp.StructuredInvoc) -> tuple[list[_clp.Invoc], bool]:
        new_children = list[_clp.Invoc]()
        children_changed = False
        for child in invoc.children:
            new_child, child_changed = subst_invoc(child)
            new_children.append(new_child)
            children_changed |= child_changed
        if children_changed:
            return new_children, True
        return invoc.children, False

    new_invoc, invoc_changed = subst_invoc(invoc)
    return new_invoc


type _SubstitutionValue = _CertainSubstitutionValue | _UncertainSubstitutionValue

@dataclasses.dataclass(slots=True, frozen=True)
class _CertainSubstitutionValue:
    string: str

@dataclasses.dataclass(slots=True, frozen=True)
class _UncertainSubstitutionValue:
    reason: _cur.ExpansionUncertaintyReason


class _UncertainSubstitutionException(Exception):
    def __init__(self, reason: _cur.ExpansionUncertaintyReason) -> None:
        Exception.__init__(self)
        self.reason = reason


_ASCII_LOWER_MAP = str.maketrans(_string.ascii_uppercase, _string.ascii_lowercase)

_ASCII_UPPER_MAP = str.maketrans(_string.ascii_lowercase, _string.ascii_uppercase)

_MACRO_SUBSTITUTE_REGEX = re.compile(r"\$\{([^{}]+)\}")
